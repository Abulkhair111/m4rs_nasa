// S6 Return to Earth & Debrief: shared contract (manager, UI, view). Action code = Role * 100 + Action (same scheme as S2).
// Roles per slot: 1 Systems Operator, 2 Data Specialist, 3 Pilot, 4 Recovery Specialist.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MCReturnTypes.generated.h"

UENUM(BlueprintType)
enum class EMCReturnStage : uint8
{
	Cruise   = 0,  // transit home: data report, warnings, equipment checks
	Reentry  = 1,  // reentry corridor; radio BLACKOUT in the first BlackoutSec
	Landing  = 2,  // chutes, beacon window, touchdown, sample handover
	Complete = 3
};

namespace MCReturn
{
	constexpr int32 MakeCode(int32 Role, int32 Action) { return Role * 100 + Action; }
	// Role 1 Systems: 100..103 = switch 0..3 (resolve the matching warning)
	constexpr int32 NumSwitches = 4;
	// Role 2 Data
	constexpr int32 DataCheck = 0, DataRefetch0 = 1 /*1..3*/, DataQueue = 4, DataSend = 5;
	// Role 3 Pilot
	constexpr int32 PilotSteeper = 0, PilotShallower = 1, PilotLeft = 2, PilotRight = 3, PilotChutes = 4;
	// Role 4 Recovery
	constexpr int32 RecBeacon = 0, RecCheck0 = 1 /*1..3*/, RecSecure = 4, RecPrepare = 5, RecHandover = 6;
	constexpr int32 StageIndex = 6; // submission number used in the mission log
}

/** Tuning asset. Everything has a placeholder default (TBD, like S2). */
UCLASS(BlueprintType)
class MISSIONCREW_API UMCReturnConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/** Seconds of the cruise before the reentry (and the blackout) starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline") float CruiseSec = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline") float ReentrySec = 50.f;
	/** Radio blackout length (from the start of the reentry). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline") float BlackoutSec = 30.f;
	/** Landing stage length (touchdown at its end). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline") float LandingSec = 30.f;
	/** The beacon is "on time" during this window from the start of the landing stage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline") float BeaconWindowSec = 20.f;
	/** After touchdown: time to finish the sample handover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timeline") float HandoverGraceSec = 15.f;
	/** Warnings (Systems): count and time each stays open. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Systems") int32 WarningCount = 4;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Systems") float WarningWindowSec = 25.f;
	/** Pilot: corridor half width (deg of flight-path angle), pulse sizes, drift, zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot") float CorridorDeg = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot") float PulseDeg = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot") float PulseKm = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot") float DriftDegPerSec = 0.06f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot") float DriftKmPerSec = 0.15f;
	/** Landing zone radius (km) and km of downrange miss per degree of angle error. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot") float ZoneKm = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pilot") float KmPerDeg = 6.f;
	/** Scoring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score") int32 PointsGood = 60;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score") int32 PointsBad = 30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score") int32 CorridorBonus = 200;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score") int32 LandingBonus = 300;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Score") int32 ReportBonus = 150;
};
