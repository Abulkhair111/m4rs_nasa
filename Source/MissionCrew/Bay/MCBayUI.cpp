#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"
#include "EngineUtils.h"
#include "Engine/World.h"

namespace MCBayUI
{
	FLinearColor CheckColor(EMCCheckState State)
	{
		switch (State)
		{
		case EMCCheckState::Green:  return FLinearColor(0.10f, 0.80f, 0.30f);
		case EMCCheckState::Yellow: return FLinearColor(1.00f, 0.78f, 0.10f);
		default:                    return FLinearColor(0.90f, 0.15f, 0.15f);
		}
	}

	FLinearColor TWRColor(float TWR)
	{
		if (TWR < MCConst::TWRRed) return CheckColor(EMCCheckState::Red);
		if (TWR < MCConst::TWRGreen) return CheckColor(EMCCheckState::Yellow);
		return CheckColor(EMCCheckState::Green);
	}

	AMCConstructionManager* FindManager(const UWorld* World)
	{
		if (!World) return nullptr;
		for (TActorIterator<AMCConstructionManager> It(const_cast<UWorld*>(World)); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	const TCHAR* SocketTypeName(EMCSocketType Type)
	{
		switch (Type)
		{
		case EMCSocketType::Power:   return TEXT("power");
		case EMCSocketType::Battery: return TEXT("battery");
		case EMCSocketType::Comms:   return TEXT("comms");
		case EMCSocketType::Science: return TEXT("science");
		case EMCSocketType::Drill:   return TEXT("drill");
		case EMCSocketType::Rover:   return TEXT("rover");
		case EMCSocketType::Engine:  return TEXT("engine");
		case EMCSocketType::Lidar:   return TEXT("lidar");
		case EMCSocketType::Tank:    return TEXT("fuel tank");
		default:                     return TEXT("tool");
		}
	}

	FString PartNumbers(const UMCPartDef* D)
	{
		if (!D) return FString();
		TArray<FString> Bits;
		if (D->MassKg != 0.f)       Bits.Add(FString::Printf(TEXT("%.0f kg"), D->MassKg));
		if (D->CostM != 0.f)        Bits.Add(FString::Printf(TEXT("$%.0fM"), D->CostM));
		if (D->PowerW != 0.f)       Bits.Add(FString::Printf(TEXT("%+.0f W"), D->PowerW));
		if (D->ThrustN != 0.f)      Bits.Add(FString::Printf(TEXT("%.1f kN"), D->ThrustN / 1000.f));
		if (D->FuelCapKg != 0.f)    Bits.Add(FString::Printf(TEXT("fuel %.0f kg"), D->FuelCapKg));
		return FString::Join(Bits, TEXT(" | "));
	}

	FString PartOneLiner(const UMCPartDef* D)
	{
		if (!D) return FString();
		if (!D->Description.IsEmpty()) return D->Description.ToString();
		TArray<FString> Bits;
		if (D->StorageWh != 0.f)      Bits.Add(FString::Printf(TEXT("stores %.0f Wh"), D->StorageWh));
		if (D->DataMbps != 0.f)       Bits.Add(FString::Printf(TEXT("data %.1f Mbps"), D->DataMbps));
		if (D->BandwidthMbps != 0.f)  Bits.Add(FString::Printf(TEXT("link %.1f Mbps"), D->BandwidthMbps));
		if (D->ScienceValue != 0.f)   Bits.Add(FString::Printf(TEXT("science %.0f"), D->ScienceValue));
		if (D->bIceCapable)           Bits.Add(TEXT("can detect ice"));
		if (Bits.Num() == 0)          Bits.Add(FString::Printf(TEXT("%s part"), SocketTypeName(D->SocketType)));
		return FString::Join(Bits, TEXT(", "));
	}
}
