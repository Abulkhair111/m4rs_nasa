#include "Shelf/MCShelfPart.h"
#include "Bay/MCBayCharacter.h"
#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"
#include "Construction/MCPartSocket.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

AMCShelfPart::AMCShelfPart()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // only while the pickup bob plays
	Prompt = FText::FromString(TEXT("Part"));
	LabelText->SetWorldSize(9.f);
}

FString AMCShelfPart::DepartmentTitle(int32 Dept)
{
	switch (Dept)
	{
	case 1: return TEXT("Power Engineer");
	case 2: return TEXT("Construction Engineer");
	case 3: return TEXT("Propulsion Engineer");
	case 4: return TEXT("Science Specialist");
	default: return TEXT("any crew member");
	}
}

void AMCShelfPart::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDef();
}

void AMCShelfPart::BeginPlay()
{
	Super::BeginPlay();
	ApplyDef();
}

void AMCShelfPart::SetPartDef(UMCPartDef* NewDef)
{
	Def = NewDef;
	ApplyDef();
}

void AMCShelfPart::ApplyDef()
{
	if (!Def || !Mesh) return;

	Department = Def->Department;
	Prompt = Def->DisplayName;

	UStaticMesh* SM = Def->Mesh;
	Mesh->SetStaticMesh(SM);

	// Parts with a StackMesh use StackMeshRotation for that mesh; the plain Mesh fallback uses it too (BP MeshRotation).
	const FRotator Rot = Def->StackMesh.IsNull() ? Def->StackMeshRotation : FRotator::ZeroRotator;
	FVector Scale = Def->MeshScale;
	FVector Size(30.f);
	FVector Offset = FVector::ZeroVector;

	if (SM)
	{
		const FBoxSphereBounds LocalBounds = SM->GetBounds();
		const FBox Local(LocalBounds.Origin - LocalBounds.BoxExtent, LocalBounds.Origin + LocalBounds.BoxExtent);
		auto Measure = [&Local, &Rot](const FVector& S) {
			const FTransform T(Rot, FVector::ZeroVector, S);
			FBox Out(ForceInit);
			for (int32 i = 0; i < 8; ++i)
			{
				const FVector Corner((i & 1) ? Local.Max.X : Local.Min.X, (i & 2) ? Local.Max.Y : Local.Min.Y, (i & 4) ? Local.Max.Z : Local.Min.Z);
				Out += T.TransformPosition(Corner);
			}
			return Out;
		};
		FBox B = Measure(Scale);
		if (FitMaxCm > 0.f && B.GetSize().GetMax() > FitMaxCm)
		{
			Scale *= FitMaxCm / B.GetSize().GetMax();
			B = Measure(Scale);
		}
		Size = B.GetSize();
		// Centre of the footprint on the actor X/Y, bottom of the part on the actor origin.
		Offset = FVector(-B.GetCenter().X, -B.GetCenter().Y, -B.Min.Z);
	}

	Mesh->SetRelativeRotation(Rot);
	Mesh->SetRelativeScale3D(Scale);
	Mesh->SetRelativeLocation(Offset);
	MeshBaseOffset = Offset;
	PartSize = Size;

	if (Def->Color.A > 0.f && Mesh->GetNumMaterials() > 0)
	{
		if (UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Def->Color);
		}
	}

	// Trace volume around the part (Visibility), a little fatter than the mesh so it is easy to hit.
	const FVector Half = FVector(FMath::Max(Size.X * 0.5f, 12.f) + 3.f, FMath::Max(Size.Y * 0.5f, 12.f) + 3.f, FMath::Max(Size.Z * 0.5f, 12.f) + 3.f);
	TraceBox->SetBoxExtent(Half);
	TraceBox->SetRelativeLocation(FVector(0.f, 0.f, Size.Z * 0.5f));

	if (LabelText)
	{
		Label = bShowNameLabel ? Def->DisplayName.ToString() : FString();
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetVisibility(!Label.IsEmpty());
		LabelText->SetRelativeLocation(FVector(0.f, 0.f, Size.Z + 14.f));
	}
}

FVector AMCShelfPart::GetPickupLocation() const
{
	return GetActorTransform().TransformPosition(FVector(0.f, 0.f, PartSize.Z * 0.5f));
}

void AMCShelfPart::PlayPickup()
{
	PickupTime = 0.f;
	SetActorTickEnabled(true);
}

void AMCShelfPart::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (PickupTime < 0.f) { SetActorTickEnabled(false); return; }
	PickupTime += DeltaSeconds;
	const float A = FMath::Clamp(PickupTime / PickupDuration, 0.f, 1.f);
	if (Mesh) Mesh->SetRelativeLocation(MeshBaseOffset + FVector(0.f, 0.f, 14.f * FMath::Sin(PI * A)));
	if (A >= 1.f)
	{
		PickupTime = -1.f;
		if (Mesh) Mesh->SetRelativeLocation(MeshBaseOffset);
		SetActorTickEnabled(false);
	}
}

FText AMCShelfPart::GetPromptFor(const AMCBayCharacter* Who) const
{
	if (!Def) return FText::FromString(TEXT("(empty shelf slot)"));

	FString Extra;
	if (!Def->Description.IsEmpty())
	{
		// PartOneLiner shows the data numbers only when there is no description: add them here.
		TArray<FString> Bits;
		if (Def->StorageWh != 0.f)     Bits.Add(FString::Printf(TEXT("stores %.0f Wh"), Def->StorageWh));
		if (Def->DataMbps != 0.f)      Bits.Add(FString::Printf(TEXT("data %.1f Mbps"), Def->DataMbps));
		if (Def->BandwidthMbps != 0.f) Bits.Add(FString::Printf(TEXT("link %.1f Mbps"), Def->BandwidthMbps));
		if (Def->ScienceValue != 0.f)  Bits.Add(FString::Printf(TEXT("science %.0f"), Def->ScienceValue));
		if (Def->bIceCapable)          Bits.Add(TEXT("can detect ice"));
		if (Bits.Num() > 0) Extra = FString::Join(Bits, TEXT(", "));
	}

	FString Hint;
	const bool bAllowed = IsAllowedFor(Who);
	if (!bAllowed)
	{
		Hint = FString::Printf(TEXT("Only the %s can take this part"), *DepartmentTitle(Def->Department));
	}
	else if (Who && Who->SelectedPart == Def)
	{
		Hint = FString::Printf(TEXT("SELECTED: aim at a free %s socket and press E   ([G] cancel)"), MCBayUI::SocketTypeName(Def->SocketType));
	}
	else
	{
		Hint = TEXT("[E] Select for installation");
	}

	FString S = FString::Printf(TEXT("%s   [%s]\n%s"), *Def->DisplayName.ToString(), MCBayUI::GroupName(Def->GetStackGroup()), *MCBayUI::PartOneLiner(Def));
	const FString Numbers = MCBayUI::PartNumbers(Def);
	if (!Numbers.IsEmpty()) S += TEXT("\n") + Numbers;
	if (!Extra.IsEmpty()) S += TEXT("\n") + Extra;
	S += FString::Printf(TEXT("\nFits: %s socket   Dept: %s"), MCBayUI::SocketTypeName(Def->SocketType), *DepartmentTitle(Def->Department));
	S += TEXT("\n") + Hint;
	return FText::FromString(S);
}

void AMCShelfPart::Use(AMCBayCharacter* Who)
{
	Super::Use(Who); // only the BP event; selection is handled locally by the pawn
}

FString AMCShelfPart::WhyNotPlaceable(const UMCPartDef* PartDef, const AMCPartSocket* Socket, const AMCConstructionManager* Manager, int32 CrewRole)
{
	if (!PartDef || !Socket) return TEXT("Nothing to install.");
	if (Manager && (!Manager->bActive || Manager->bLaunched)) return TEXT("Assembly is not active.");
	if (Socket->bReserved) return TEXT("A part is already on its way to this socket.");
	if (!Socket->IsEmpty()) return TEXT("That socket already has a part: remove it first ([R], unbolt with the wrench if needed).");
	if (PartDef->SocketType != Socket->SocketType)
	{
		return FString::Printf(TEXT("%s needs a %s socket, this is a %s socket."), *PartDef->DisplayName.ToString(), MCBayUI::SocketTypeName(PartDef->SocketType), MCBayUI::SocketTypeName(Socket->SocketType));
	}
	if (PartDef->GetStackGroup() != Socket->GetGroup())
	{
		return PartDef->GetStackGroup() == EMCStackGroup::Ascent
			? TEXT("Ascent part (tank, engine, booster): it goes on the ascent vehicle, not the lander.")
			: TEXT("Lander part: it goes on the lander, not the ascent vehicle.");
	}
	if (Manager && !Manager->RoleAllowed((EMCCrewRole)CrewRole, PartDef->Department))
	{
		return FString::Printf(TEXT("That part belongs to the %s."), *DepartmentTitle(PartDef->Department));
	}
	if (Manager && Manager->TotalCostM + PartDef->CostM > Manager->BudgetM) return TEXT("Over budget.");
	return FString();
}
