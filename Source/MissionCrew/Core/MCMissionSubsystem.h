// S1 -> S2 glue (agent X). The flight deck is a separate map (L_Transit), so the S1 result must survive the map change.
// A UGameInstanceSubsystem lives as long as the GameInstance, i.e. across ServerTravel (seamless or not).
//
// Filled on the SERVER by AMCConstructionManager::TryLaunch (RecordLaunch). Read by AMCTransitManager (LoadLoadout)
// and by the deck flow (ApplyRoles re-writes CrewRole into the new PlayerStates, because non-seamless travel
// creates new PlayerStates and the Blueprint CrewRole variable would be back at its default).
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Construction/MCConstructionTypes.h"
#include "Transit/MCTransitTypes.h"
#include "Orbit/MCOrbitTypes.h"
#include "MCMissionSubsystem.generated.h"

class AMCConstructionManager;
class APlayerState;

USTRUCT(BlueprintType)
struct FMCRoleRecord
{
	GENERATED_BODY()

	/** Unique net id string if valid, else the player name. */
	UPROPERTY(BlueprintReadOnly) FString Key;
	UPROPERTY(BlueprintReadOnly) FString PlayerName;
	UPROPERTY(BlueprintReadOnly) int32 Role = 0;
	/** Join order at record time (host = 0): fallback match when keys do not match (same name, no online id). */
	UPROPERTY(BlueprintReadOnly) int32 Order = 0;
};

/** One accuracy-log entry (any stage). Stage 1..6, Role 1..4 (0 = unknown). */
USTRUCT(BlueprintType)
struct FMCAccuracyEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Stage = 0;
	UPROPERTY(BlueprintReadOnly) int32 Role = 0;
	UPROPERTY(BlueprintReadOnly) FString PlayerKey;
	UPROPERTY(BlueprintReadOnly) bool bCorrect = false;
	UPROPERTY(BlueprintReadOnly) FString Note;
	UPROPERTY(BlueprintReadOnly) double Time = 0.0;
};

/** Result of one submission (stage 1..6) for the final summary. */
USTRUCT(BlueprintType)
struct FMCStageResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Stage = 0;
	UPROPERTY(BlueprintReadOnly) int32 Score = 0;
	UPROPERTY(BlueprintReadOnly) float DurationSec = 0.f;
	UPROPERTY(BlueprintReadOnly) FString Lesson;
};

UCLASS()
class MISSIONCREW_API UMCMissionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Convenience: the subsystem of the world's GameInstance (null if none). */
	UFUNCTION(BlueprintPure, Category = "Mission", meta = (WorldContext = "WorldContext")) static UMCMissionSubsystem* Get(const UObject* WorldContext);

	// ---- S1 result ----
	UPROPERTY(BlueprintReadOnly, Category = "Mission") bool bHasLoadout = false;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") FMCLoadout Loadout;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") FName LauncherName;
	/** Fuel loaded (kg) = Loadout.FuelKg. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission") float FuelKg = 0.f;
	/** The landing fuel requirement from S1 (0.9 x dry mass), used for the landing reserve. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission") float FuelRequiredKg = 0.f;
	/** Launcher reliability 0..1 (ConstructionManager::RocketReliability). */
	UPROPERTY(BlueprintReadOnly, Category = "Mission") float LaunchReliability = 1.f;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") int32 IceCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") bool bHasIceInstrument = false;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") bool bHasRover = false;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") bool bHasDrill = false;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") bool bHasLidar = false;
	/** Names of the science instruments (upper case, as the transit shows them). */
	UPROPERTY(BlueprintReadOnly, Category = "Mission") TArray<FString> InstrumentNames;
	/** S1 score per role (index 1..4). */
	UPROPERTY(BlueprintReadOnly, Category = "Mission") TArray<int32> S1RoleGood;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") TArray<int32> S1RoleBad;

	// ---- S4 result (written by AMCSurfaceManager; not cleared by Clear()) ----
	UPROPERTY(BlueprintReadOnly, Category = "Mission") bool bS4Done = false;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") bool bS4Success = false;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") int32 S4Samples = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") int32 S4IceSamples = 0;
	/** One entry per sample: ICE or REGOLITH. */
	UPROPERTY(BlueprintReadOnly, Category = "Mission") TArray<FString> S4SampleTypes;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") float S4PowerUsedWh = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") int32 S4Score = 0;

	// ---- S5 result (written by AMCSamplesManager; S5 reads S4Samples / S4SampleTypes above, default 3 samples if empty) ----
	UPROPERTY(BlueprintReadOnly, Category = "Mission") bool bHasS5 = false;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") int32 S5Score = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") int32 S5SamplesOk = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") FString S5ContainerName;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") float S5MassKg = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") float S5PowerW = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") TArray<int32> S5RoleGood;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") TArray<int32> S5RoleBad;

	// ---- S2 / S3 results (ADDED for S3; server side, survive ServerTravel) ----
	UPROPERTY(BlueprintReadOnly, Category = "Mission") bool bHasTransit = false;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") FMCTransitReport Transit;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") bool bHasLanding = false;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") FMCOrbitReport Landing;
	void RecordTransit(const FMCTransitReport& R) { Transit = R; bHasTransit = true; }
	void RecordLanding(const FMCOrbitReport& R) { Landing = R; bHasLanding = true; }

	// ---- Crew roles ----
	UPROPERTY(BlueprintReadOnly, Category = "Mission") TArray<FMCRoleRecord> Roles;

	// ---- Mission-wide log (added for S6 / Summary; SERVER-side data, clients' copy stays empty) ----
	UPROPERTY(BlueprintReadOnly, Category = "Mission") int32 MissionSeed = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") TArray<FMCAccuracyEntry> AccuracyLog;
	UPROPERTY(BlueprintReadOnly, Category = "Mission") TArray<FMCStageResult> StageResults;
	/** Log one correct / wrong action of a role in a stage (1..6). PlayerKey optional (UMCMissionSubsystem::MakeKey). */
	UFUNCTION(BlueprintCallable, Category = "Mission") void AppendAccuracy(int32 Stage, int32 Role, bool bCorrect, const FString& Note, const FString& PlayerKey = FString());
	/** Store/overwrite the result of a submission: score, time taken (s), one lesson line. */
	UFUNCTION(BlueprintCallable, Category = "Mission") void SetStageResult(int32 Stage, int32 Score, float DurationSec, const FString& Lesson);
	/** Fills MissionSeed once (first non-zero seed wins). */
	UFUNCTION(BlueprintCallable, Category = "Mission") void SetMissionSeed(int32 Seed) { if (MissionSeed == 0) MissionSeed = Seed; }
	/** Static convenience: finds the subsystem of WorldContext and appends (no-op if none). */
	static void LogAccuracy(const UObject* WorldContext, int32 Stage, int32 Role, bool bCorrect, const FString& Note);

	/** SERVER: store the S1 result from the manager (called by TryLaunch before OnLaunchComplete fires) and record the roles. */
	void RecordLaunch(const AMCConstructionManager* Manager, const FMCLoadout& InLoadout);
	/** SERVER: snapshot CrewRole of every connected player. */
	UFUNCTION(BlueprintCallable, Category = "Mission") void RecordRoles(UWorld* World);
	/**
	 * SERVER: write the recorded roles back into the PlayerStates' CrewRole (by reflection). Only players whose current role is
	 * 0 are touched; match by Key first, then by join order. Returns how many players got a role. Safe to call repeatedly.
	 */
	UFUNCTION(BlueprintCallable, Category = "Mission") int32 ApplyRoles(UWorld* World);
	/** Manual override / test: set the role of a player state in the records (and nothing else). */
	UFUNCTION(BlueprintCallable, Category = "Mission") void SetRoleRecord(const APlayerState* PS, int32 Role);
	/** Forget everything (called by a new S1 StartPhase). */
	UFUNCTION(BlueprintCallable, Category = "Mission") void Clear();

	/** Reflection helpers on a PlayerState (CrewRole as int / byte / enum). */
	static int32 ReadCrewRole(const APlayerState* PS);
	static bool WriteCrewRole(APlayerState* PS, int32 Role);
	static FString MakeKey(const APlayerState* PS);
};
