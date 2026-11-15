#include "Core/MCMissionSubsystem.h"
#include "Construction/MCConstructionManager.h"
#include "Construction/MCPartSocket.h"
#include "MissionCrew.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UObject/UnrealType.h"

UMCMissionSubsystem* UMCMissionSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UMCMissionSubsystem>() : nullptr;
}

int32 UMCMissionSubsystem::ReadCrewRole(const APlayerState* PS)
{
	if (!PS) return 0;
	if (const FProperty* P = PS->GetClass()->FindPropertyByName(TEXT("CrewRole")))
	{
		const void* Addr = P->ContainerPtrToValuePtr<void>(PS);
		if (const FIntProperty* IP = CastField<FIntProperty>(P)) return IP->GetPropertyValue(Addr);
		if (const FByteProperty* BP = CastField<FByteProperty>(P)) return BP->GetPropertyValue(Addr);
		if (const FEnumProperty* EP = CastField<FEnumProperty>(P)) return (int32)EP->GetUnderlyingProperty()->GetSignedIntPropertyValue(Addr);
	}
	return 0;
}

bool UMCMissionSubsystem::WriteCrewRole(APlayerState* PS, int32 Role)
{
	if (!PS) return false;
	if (const FProperty* P = PS->GetClass()->FindPropertyByName(TEXT("CrewRole")))
	{
		void* Addr = P->ContainerPtrToValuePtr<void>(PS);
		if (const FIntProperty* IP = CastField<FIntProperty>(P)) { IP->SetPropertyValue(Addr, Role); }
		else if (const FByteProperty* BP = CastField<FByteProperty>(P)) { BP->SetPropertyValue(Addr, (uint8)Role); }
		else if (const FEnumProperty* EP = CastField<FEnumProperty>(P)) { EP->GetUnderlyingProperty()->SetIntPropertyValue(Addr, (int64)Role); }
		else return false;
		PS->ForceNetUpdate();
		return true;
	}
	return false;
}

FString UMCMissionSubsystem::MakeKey(const APlayerState* PS)
{
	if (!PS) return FString();
	const FUniqueNetIdRepl& Id = PS->GetUniqueId();
	if (Id.IsValid()) return Id.ToString();
	return PS->GetPlayerName();
}

void UMCMissionSubsystem::Clear()
{
	bHasLoadout = false;
	Loadout = FMCLoadout();
	LauncherName = NAME_None;
	FuelKg = FuelRequiredKg = 0.f;
	LaunchReliability = 1.f;
	IceCount = 0;
	bHasIceInstrument = bHasRover = bHasDrill = bHasLidar = false;
	bHasTransit = false; Transit = FMCTransitReport(); bHasLanding = false; Landing = FMCOrbitReport();
	InstrumentNames.Reset();
	S1RoleGood.Reset();
	S1RoleBad.Reset();
	Roles.Reset();
	MissionSeed = 0;
	AccuracyLog.Reset();
	StageResults.Reset();
	bS4Done = bS4Success = false; S4Samples = S4IceSamples = S4Score = 0; S4SampleTypes.Reset(); S4PowerUsedWh = 0.f;
	bHasS5 = false; S5Score = S5SamplesOk = 0; S5ContainerName.Reset(); S5MassKg = S5PowerW = 0.f;
	S5RoleGood.Reset(); S5RoleBad.Reset();
}

void UMCMissionSubsystem::AppendAccuracy(int32 Stage, int32 Role, bool bCorrect, const FString& Note, const FString& PlayerKey)
{
	FMCAccuracyEntry& E = AccuracyLog.AddDefaulted_GetRef();
	E.Stage = Stage; E.Role = Role; E.bCorrect = bCorrect; E.Note = Note; E.PlayerKey = PlayerKey;
	E.Time = FPlatformTime::Seconds();
}

void UMCMissionSubsystem::SetStageResult(int32 Stage, int32 Score, float DurationSec, const FString& Lesson)
{
	FMCStageResult* R = StageResults.FindByPredicate([Stage](const FMCStageResult& X) { return X.Stage == Stage; });
	if (!R) { R = &StageResults.AddDefaulted_GetRef(); R->Stage = Stage; }
	R->Score = Score; R->DurationSec = DurationSec; R->Lesson = Lesson;
}

void UMCMissionSubsystem::LogAccuracy(const UObject* WorldContext, int32 Stage, int32 Role, bool bCorrect, const FString& Note)
{
	if (UMCMissionSubsystem* S = Get(WorldContext)) S->AppendAccuracy(Stage, Role, bCorrect, Note);
}

void UMCMissionSubsystem::RecordLaunch(const AMCConstructionManager* Manager, const FMCLoadout& InLoadout)
{
	if (!Manager || !Manager->HasAuthority()) return;
	Loadout = InLoadout;
	bHasLoadout = true;
	LauncherName = InLoadout.RocketName;
	FuelKg = InLoadout.FuelKg;
	FuelRequiredKg = Manager->FuelRequiredKg;
	LaunchReliability = FMath::Clamp(Manager->RocketReliability > 0.f ? Manager->RocketReliability : 1.f, 0.f, 1.f);
	IceCount = InLoadout.IceCount;
	bHasIceInstrument = InLoadout.IceCount > 0;
	S1RoleGood = Manager->RoleGood;
	S1RoleBad = Manager->RoleBad;

	bHasRover = bHasDrill = bHasLidar = false;
	InstrumentNames.Reset();
	for (const FMCLoadoutEntry& E : InLoadout.Parts)
	{
		switch (E.SocketType)
		{
		case EMCSocketType::Rover: bHasRover = true; break;
		case EMCSocketType::Drill: bHasDrill = true; break;
		case EMCSocketType::Lidar: bHasLidar = true; break;
		case EMCSocketType::Science:
			InstrumentNames.Add(E.Def ? E.Def->DisplayName.ToString().ToUpper() : TEXT("INSTRUMENT"));
			break;
		default: break;
		}
	}
	RecordRoles(Manager->GetWorld());
	UE_LOG(LogMissionCrew, Log, TEXT("Mission: S1 result stored (%s, fuel %.0f kg, %d parts, %d roles)"),
		*LauncherName.ToString(), FuelKg, Loadout.Parts.Num(), Roles.Num());
}

void UMCMissionSubsystem::RecordRoles(UWorld* World)
{
	if (!World || World->GetNetMode() == NM_Client) return;
	Roles.Reset();
	int32 Order = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		const APlayerState* PS = PC ? PC->PlayerState.Get() : nullptr;
		if (!PS) { ++Order; continue; }
		FMCRoleRecord R;
		R.Key = MakeKey(PS);
		R.PlayerName = PS->GetPlayerName();
		R.Role = ReadCrewRole(PS);
		R.Order = Order++;
		Roles.Add(R);
	}
}

void UMCMissionSubsystem::SetRoleRecord(const APlayerState* PS, int32 Role)
{
	if (!PS) return;
	const FString Key = MakeKey(PS);
	for (FMCRoleRecord& R : Roles)
	{
		if (R.Key == Key) { R.Role = Role; return; }
	}
	FMCRoleRecord R;
	R.Key = Key;
	R.PlayerName = PS->GetPlayerName();
	R.Role = Role;
	R.Order = Roles.Num();
	Roles.Add(R);
}

int32 UMCMissionSubsystem::ApplyRoles(UWorld* World)
{
	if (!World || World->GetNetMode() == NM_Client || Roles.Num() == 0) return 0;

	// Players in controller order (host first).
	TArray<APlayerState*> Players;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (PC && PC->PlayerState) Players.Add(PC->PlayerState);
	}

	TArray<bool> RecordUsed;
	RecordUsed.Init(false, Roles.Num());
	// Records already represented by a player that holds its role: mark used so they are not given to someone else.
	for (APlayerState* PS : Players)
	{
		if (ReadCrewRole(PS) == 0) continue;
		const FString Key = MakeKey(PS);
		for (int32 i = 0; i < Roles.Num(); ++i)
		{
			if (!RecordUsed[i] && Roles[i].Key == Key && Roles[i].Role == ReadCrewRole(PS)) { RecordUsed[i] = true; break; }
		}
	}

	int32 Applied = 0;
	TArray<APlayerState*> Left;
	for (APlayerState* PS : Players)
	{
		if (ReadCrewRole(PS) != 0) continue;
		const FString Key = MakeKey(PS);
		bool bDone = false;
		for (int32 i = 0; i < Roles.Num(); ++i)
		{
			if (RecordUsed[i] || Roles[i].Key != Key) continue;
			RecordUsed[i] = true; // matched whether or not the role is 0
			if (Roles[i].Role > 0 && WriteCrewRole(PS, Roles[i].Role)) ++Applied;
			bDone = true;
			break;
		}
		if (!bDone) Left.Add(PS);
	}

	// Fallback: no key matched (same name / no online id on every machine). Match by join order among the leftovers.
	for (APlayerState* PS : Left)
	{
		const int32 Order = Players.IndexOfByKey(PS);
		for (int32 i = 0; i < Roles.Num(); ++i)
		{
			if (RecordUsed[i] || Roles[i].Order != Order) continue;
			RecordUsed[i] = true;
			if (Roles[i].Role > 0 && WriteCrewRole(PS, Roles[i].Role)) ++Applied;
			break;
		}
	}
	return Applied;
}
