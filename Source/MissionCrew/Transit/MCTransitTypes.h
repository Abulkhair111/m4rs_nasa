// Shared contract for the Mission Crew S2 (Launch & Transit) C++ code.
// UI (agent S2b), player (agent S2c) and the manager (S2a) all agree on what lives here.
// Action code = Role * 100 + Action. GO FOR LAUNCH is the special code 90 (role 0).
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MCTransitTypes.generated.h"

/** Stage machine of AMCTransitManager::Stage. */
UENUM(BlueprintType)
enum class EMCTransitStage : uint8
{
	/** Waiting: every role presses GO FOR LAUNCH (or the manager has not started). */
	GoPoll    = 0,
	Countdown = 1,
	Ascent    = 2,
	Coast     = 3,
	Arrival   = 4
};

/** Same numbers as EMCCrewRole / the S1 CrewRole. 0 = unassigned (solo / free roles). */
UENUM(BlueprintType)
enum class EMCTransitRole : uint8
{
	None      = 0,
	Systems   = 1,
	Equipment = 2,
	Pilot     = 3,
	Science   = 4
};

/** Role 1. Action = switch index (code 100 + index). */
UENUM(BlueprintType)
enum class EMCSystemsAction : uint8
{
	Heaters        = 0,
	NavSensor      = 1,   // off = A, on = B
	Transponder    = 2,   // off = A, on = B
	WheelDesat     = 3,
	InstrumentRail = 4,
	MechanismRail  = 5
};

/** Role 2. 0..2 = step buttons (positions as displayed, scrambled), 3..6 = select device 0..3 (code 203..206). */
UENUM(BlueprintType)
enum class EMCEquipAction : uint8
{
	Button0 = 0,
	Button1 = 1,
	Button2 = 2,
	SelectDevice0 = 3,
	SelectDevice1 = 4,
	SelectDevice2 = 5,
	SelectDevice3 = 6
};

/** Role 3 (code 300 + action). One pulse moves the predicted miss by NudgeKm. */
UENUM(BlueprintType)
enum class EMCPilotAction : uint8
{
	Left  = 0,
	Right = 1,
	Up    = 2,
	Down  = 3
};

/** Role 4. 0..3 = offset delta (-5, -1, +1, +5), 4 = LOCK, 5..7 = select instrument 0..2 (code 405..407). */
UENUM(BlueprintType)
enum class EMCScienceAction : uint8
{
	Minus5 = 0,
	Minus1 = 1,
	Plus1  = 2,
	Plus5  = 3,
	Lock   = 4,
	SelectInstrument0 = 5,
	SelectInstrument1 = 6,
	SelectInstrument2 = 7
};

namespace MCTransit
{
	constexpr int32 GoCode = 90;
	constexpr int32 NumSwitches = 6;
	constexpr int32 MaxDevices = 4;
	constexpr int32 MaxInstruments = 3;
	constexpr int32 NumWindows = 5;      // TCM-1..4 + TCM-FAULT
	constexpr int32 FaultWindowIndex = 4;
	constexpr int32 NumAlarms = 4;
	constexpr int32 NumRoles = 5;        // index 0 shared, 1..4 roles

	inline int32 MakeCode(EMCTransitRole Role, int32 Action) { return static_cast<int32>(Role) * 100 + Action; }
	inline int32 RoleOfCode(int32 Code) { return Code / 100; }
	inline int32 ActionOfCode(int32 Code) { return Code % 100; }
}

/** Systems console switch (replicated). */
USTRUCT(BlueprintType)
struct FMCTransitSwitch
{
	GENERATED_BODY()

	/** Full label with the current state, e.g. "NAV SENSOR  [B]". */
	UPROPERTY(BlueprintReadOnly) FString Label;
	UPROPERTY(BlueprintReadOnly) bool bOn = false;
};

/** Systems alarm (replicated). The right switch is NOT replicated: the player must work it out from the text. */
USTRUCT(BlueprintType)
struct FMCTransitAlarm
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Text;
	UPROPERTY(BlueprintReadOnly) bool bActive = false;
	UPROPERTY(BlueprintReadOnly) bool bFixed = false;
};

/** Equipment device (replicated). State: 0 idle, 1 testing, 2 passed. */
USTRUCT(BlueprintType)
struct FMCTransitDevice
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	/** The procedure in the correct order (shown to the technician as text). */
	UPROPERTY(BlueprintReadOnly) TArray<FString> Steps;
	/** Button captions as displayed left to right; scrambled per seed. Button p performs the step with this caption. */
	UPROPERTY(BlueprintReadOnly) TArray<FString> ButtonLabels;
	/** Steps already done 0..3. */
	UPROPERTY(BlueprintReadOnly) int32 Progress = 0;
	UPROPERTY(BlueprintReadOnly) int32 State = 0;
};

/** Science instrument (replicated). Goal: Reading within CalTarget +- CalTolerance, then LOCK. */
USTRUCT(BlueprintType)
struct FMCTransitInstrument
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) float Offset = 0.f;
	/** Live reading = Offset + wobble. Updated about 5x per second. */
	UPROPERTY(BlueprintReadOnly) float Reading = 0.f;
	UPROPERTY(BlueprintReadOnly) bool bLocked = false;
	/** Locked inside the tolerance. */
	UPROPERTY(BlueprintReadOnly) bool bCalibrated = false;
};

/** Pilot burn window (replicated). OpenSec < 0 means "not scheduled" (TCM-FAULT before the fault is fixed). */
USTRUCT(BlueprintType)
struct FMCTransitWindow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Label;
	UPROPERTY(BlueprintReadOnly) float OpenSec = -1.f;
	UPROPERTY(BlueprintReadOnly) float CloseSec = -1.f;
	UPROPERTY(BlueprintReadOnly) bool bOpen = false;
	UPROPERTY(BlueprintReadOnly) bool bDone = false;
	UPROPERTY(BlueprintReadOnly) int32 Pulses = 0;
};

/** Final numbers of S2, handed to S3. */
USTRUCT(BlueprintType)
struct FMCTransitReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Score = 0;
	UPROPERTY(BlueprintReadOnly) float CourseScore = 0.f;
	UPROPERTY(BlueprintReadOnly) float InstrumentScore = 0.f;
	UPROPERTY(BlueprintReadOnly) float EquipmentScore = 0.f;
	UPROPERTY(BlueprintReadOnly) float HealthScore = 0.f;
	UPROPERTY(BlueprintReadOnly) float FuelScore = 0.f;
	UPROPERTY(BlueprintReadOnly) float MissKm = 0.f;
	UPROPERTY(BlueprintReadOnly) bool bInCorridor = false;
	UPROPERTY(BlueprintReadOnly) float FuelLeftKg = 0.f;
	UPROPERTY(BlueprintReadOnly) float Health = 100.f;
	UPROPERTY(BlueprintReadOnly) int32 InstrumentsCalibrated = 0;
	UPROPERTY(BlueprintReadOnly) int32 InstrumentsTotal = 0;
	UPROPERTY(BlueprintReadOnly) int32 DevicesPassed = 0;
	UPROPERTY(BlueprintReadOnly) int32 DevicesTotal = 0;
	UPROPERTY(BlueprintReadOnly) bool bAnomaly = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMCTransitCompleteSignature, const FMCTransitReport&, Report);

/**
 * All S2 numbers. Every value is a PLACEHOLDER (TBD, per brief section 1 #7) until the design doc fixes balanced ones.
 * The manager works without an asset (it creates a default instance), so this class only needs an asset to tune values.
 */
UCLASS(BlueprintType)
class MISSIONCREW_API UMCTransitConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType(TEXT("MCTransitConfig")), GetFName());
	}

	// ---- Timeline (TBD) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float CountdownSec = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float AscentSec = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float TransitSec = 180.f;
	/** Show the final report this long before the delegate/phase hand-off is considered final (informational). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float ArrivalHoldSec = 15.f;

	// ---- Course (TBD) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course") float CorridorKm = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course") float NudgeKm = 6.f;
	/** Initial predicted miss after ascent is drawn in +-this (km). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course") float InitialErrMaxKm = 25.f;
	/** Course drift per segment between windows is drawn in +-this (km). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course") float DriftMaxKm = 10.f;
	/** Extra error when the ascent anomaly triggers (km, random direction). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course") float AnomalyErrKm = 18.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course") float AnomalyFuelFrac = 0.05f;
	/** Course score falls to 0 this far outside the corridor (km). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course") float CourseFalloffKm = 30.f;

	// ---- Burn windows (TCM-1..4) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Windows") TArray<float> WindowOpenSec = { 20.f, 65.f, 110.f, 150.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Windows") float WindowSec = 15.f;

	// ---- Fuel ----
	/** One pulse costs NudgeFuelKg * (1 + 2 * progress), progress = coast elapsed / TransitSec. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fuel") float NudgeFuelKg = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fuel") float OffCorridorFuelKgPerKm = 1.5f;
	/** Landing reserve = this fraction of S1 FuelRequiredKg. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fuel") float LandingReserveFrac = 0.5f;
	/** Fallback reserve when there is no S1 manager (test mode). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fuel") float DefaultReserveKg = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fuel") float DefaultFuelKg = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fuel") float DefaultLaunchReliability = 0.9f;

	// ---- Sensor fault + alarms ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float FaultTimeMinSec = 45.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float FaultTimeMaxSec = 95.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float FaultKickMinKm = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float FaultKickMaxKm = 30.f;
	/** The 3 ordinary alarms are scheduled in [min, max] coast seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float AlarmMinSec = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float AlarmMaxSec = 140.f;
	/** Health lost per second per open alarm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float HealthDrainPerAlarmSec = 0.4f;

	// ---- Equipment / science ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment") float DeviceTestSec = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Science") float CalTarget = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Science") float CalTolerance = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Science") float WobbleAmp = 1.f;

	// ---- Scoring weights (sum 100, TBD) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring") float WeightCourse = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring") float WeightInstruments = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring") float WeightEquipment = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring") float WeightHealth = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring") float WeightFuel = 15.f;
};
