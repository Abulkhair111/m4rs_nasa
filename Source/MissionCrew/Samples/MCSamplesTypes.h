// S5 Sample Preservation & Return Preparation: shared types + tuning asset.
// Roles (slot numbers): 1 Systems Operator, 2 Sample Scientist, 3 Sample Technician, 4 Cargo Operator.
// Action codes = role*100 + action (same scheme as S2):
//   Systems  100..104 cooling mode (OFF/ECO/NOMINAL/BOOST/BACKUP)  105 insulation cover (passive-container warning)  109 routine check
//   Scientist 200..202 type (ICE/REGOLITH/ROCK)  203..205 storage (FROZEN/COOL/AMBIENT)  206 label+store  207 update storage (fault)  209 routine
//   Technician 300..302 container (PASSIVE/ACTIVE/HYBRID)  303 place  304 seal  305 packaging check  309 routine
//   Cargo     400 load  401 secure  409 routine
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MCSamplesTypes.generated.h"

namespace MCSamples
{
	constexpr int32 Systems = 1, Scientist = 2, Technician = 3, Cargo = 4;
	constexpr int32 ActRoutine = 9;
	inline int32 MakeCode(int32 Role, int32 Action) { return Role * 100 + Action; }
}

UENUM(BlueprintType) enum class EMCSampleStage : uint8 { Raw, Labeled, Placed, Sealing, Sealed, Checked, Loaded, Secured };
UENUM(BlueprintType) enum class EMCSampleType : uint8 { Ice, Regolith, Rock };
UENUM(BlueprintType) enum class EMCStorageCond : uint8 { Frozen, Cool, Ambient };
UENUM(BlueprintType) enum class EMCSamplesEvent : uint8 { None, CoolerFault, HoldWarning };

USTRUCT(BlueprintType)
struct FMCSample
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Id = 0;
	/** Instrument readout the Scientist uses to identify the type. */
	UPROPERTY(BlueprintReadOnly) FString Hint;
	UPROPERTY(BlueprintReadOnly) EMCSampleStage Stage = EMCSampleStage::Raw;
	/** Scientist's label: EMCSampleType / EMCStorageCond as int, -1 = not set. */
	UPROPERTY(BlueprintReadOnly) int32 ChosenType = -1;
	UPROPERTY(BlueprintReadOnly) int32 ChosenCond = -1;
	UPROPERTY(BlueprintReadOnly) float SealProgress = 0.f;
};

USTRUCT(BlueprintType)
struct FMCSamplesContainer
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name = TEXT("CONTAINER");
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MassKg = 5.f;
	/** Standing power draw (W) while the container is aboard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float BasePowerW = 0.f;
	/** Active = has a cooler (modes matter, cooler fault event). Passive = hold-time warning event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bActive = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSupportsFrozen = true;
	/** Heat leak towards ambient (1/s). Hold time = how long until the limit is crossed with no cooling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LeakPerSec = 0.01f;
	/** Multiplier on the cooling-mode rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CoolEff = 1.f;
};

USTRUCT(BlueprintType)
struct FMCCoolingMode
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name;
	/** Cooling rate (degC/s) at full effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CoolRate = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PowerW = 0.f;
};

/** Tuning for S5. The manager works without an asset (built-in defaults). All numbers are TBD placeholders. */
UCLASS(BlueprintType)
class MISSIONCREW_API UMCSamplesConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(FPrimaryAssetType(TEXT("MCSamplesConfig")), GetFName()); }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Samples") int32 DefaultSampleCount = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Samples") float SampleMassKg = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thermal") float AmbientC = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thermal") float StartTempC = -30.f;
	/** Max allowed temperature per EMCStorageCond (Frozen, Cool, Ambient). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thermal") TArray<float> CondLimitC = { -15.f, 4.f, 30.f };
	/** The limit rises by this when the Scientist updates storage conditions after a fault. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thermal") float FaultRelaxC = 8.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thermal") float PowerBudgetW = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thermal") TArray<FMCSamplesContainer> Containers;
	/** OFF, ECO, NOMINAL, BOOST, BACKUP (index = action). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Thermal") TArray<FMCCoolingMode> Modes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pipeline") float SealSec = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pipeline") float SealLeakChance = 0.3f;
	/** No-wait rule: a routine-check action is available this often to a role with no pipeline work. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pipeline") float RoutineCooldownSec = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event") float EventMinSec = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event") float EventMaxSec = 70.f;
	/** Systems must answer within this to score the event as handled in time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event") float EventResponseSec = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event") float HoldWarnLeakMul = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event") float InsulationLeakMul = 0.5f;

	/** The phase ends with the current score after this long even if not all samples are secured. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float TimeLimitSec = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float TravelDelaySec = 8.f;
};
