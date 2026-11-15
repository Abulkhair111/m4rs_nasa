// S4 Surface Exploration & Ice Detection: shared types + tuning asset. Action code = Role*100 + Action (same scheme as S2).
// Roles: 1 Power Operator, 2 Science Analyst, 3 Rover Driver, 4 Drill Operator. All numbers are TBD placeholders.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MCSurfaceTypes.generated.h"

class UStaticMesh;

UENUM(BlueprintType)
enum class EMCSurfRole : uint8 { None = 0, Power = 1, Science = 2, Driver = 3, Drill = 4 };

/** Role 1 (code 100+n). */
UENUM(BlueprintType)
enum class EMCSurfPowerAction : uint8
{
	None = 0,
	ToggleComms = 1,
	ToggleInstruments = 2, // science instruments: non-essential
	DrillPowerUp = 3,
	DrillPowerDown = 4
};
/** Role 2 (code 200+n): 1..4 = select (rate) site 0..3; 5 = scan selected site (needs instruments on). */
UENUM(BlueprintType)
enum class EMCSurfScienceAction : uint8 { None = 0, SelectSite0 = 1, SelectSite1 = 2, SelectSite2 = 3, SelectSite3 = 4, Scan = 5 };
/** Role 3 (code 300+n); driving itself goes through the link's ServerDrive(throttle, steer). */
UENUM(BlueprintType)
enum class EMCSurfDriverAction : uint8 { None = 0, Stop = 5 };
/** Role 4 (code 400+n): 1..3 depth level, 4 start drilling, 5 extract the sample. */
UENUM(BlueprintType)
enum class EMCSurfDrillAction : uint8 { None = 0, Depth1 = 1, Depth2 = 2, Depth3 = 3, Start = 4, Extract = 5 };

namespace MCSurface
{
	constexpr int32 MaxSites = 4;
	inline int32 MakeCode(EMCSurfRole Role, int32 Action) { return (int32)Role * 100 + Action; }
}

UENUM(BlueprintType)
enum class EMCSurfSampleType : uint8 { Regolith = 0, Ice = 1 };

/** What the Science Analyst sees. The true ice fraction stays on the server. */
USTRUCT(BlueprintType)
struct FMCSurfSite
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) FVector2D Pos = FVector2D::ZeroVector; // cm, relative to the manager
	/** Hydrogen index 0..100 (higher = more ice). Noisy; each scan lowers the noise. */
	UPROPERTY(BlueprintReadOnly) float Hydrogen = 0.f;
	/** Surface temperature K (colder = more ice). */
	UPROPERTY(BlueprintReadOnly) float TempK = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 Scans = 0;
};

USTRUCT(BlueprintType)
struct FMCSurfSample
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Site = 0;
	UPROPERTY(BlueprintReadOnly) EMCSurfSampleType Type = EMCSurfSampleType::Regolith;
	UPROPERTY(BlueprintReadOnly) int32 DepthLevel = 1;
	/** Taken at the site the Science Analyst picked. */
	UPROPERTY(BlueprintReadOnly) bool bAtChosenSite = false;
};

USTRUCT(BlueprintType)
struct FMCSurfReport
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Samples = 0;
	UPROPERTY(BlueprintReadOnly) int32 IceSamples = 0;
	UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
	UPROPERTY(BlueprintReadOnly) float PowerUsedWh = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 RockHits = 0;
	UPROPERTY(BlueprintReadOnly) int32 Score = 0;
	UPROPERTY(BlueprintReadOnly) float TimeSec = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMCSurfaceCompleteSignature, const FMCSurfReport&, Report);

/** Optional tuning asset (the manager works without one: built-in defaults). */
UCLASS(BlueprintType)
class MISSIONCREW_API UMCSurfaceConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(FPrimaryAssetType(TEXT("MCSurfaceConfig")), GetFName()); }

	// Field (cm, relative to the manager actor)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field") int32 SiteCount = 4;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field") float SiteMinDistCm = 3000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field") float SiteMaxDistCm = 5500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field") float SiteRadiusCm = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field") int32 RockCount = 16;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field") float FieldHalfSizeCm = 6500.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field") float RoverRadiusCm = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field") float RoverSpeedCmS = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Field") float RoverTurnDegS = 60.f;

	// Science
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Science") float IceLikelyThreshold = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Science") float ReadingNoise = 22.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Science") float ScanEnergyWh = 4.f;

	// Power (battery Wh is scaled by PowerTimeScale so the numbers move within a short game)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power") float BatteryCapacityWh = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power") float SolarW = 220.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power") float BatteryMaxDischargeW = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power") float PowerTimeScale = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power") float EssentialW = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power") float CommsW = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power") float InstrumentsW = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power") float RoverW = 60.f;
	/** Drill draw = DrillWPerLevel * power level (1..3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Power") float DrillWPerLevel = 60.f;

	// Power drop event
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event") float EventMinSec = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event") float EventMaxSec = 110.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event") float EventDurationSec = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event") float EventCapacityLoss = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Event") float EventSolarFactor = 0.35f;

	// Drill
	/** Seconds to drill one depth level at power level 2. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drill") float SecPerLevel = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drill") float DepthCmPerLevel = 50.f;

	// Flow
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow") float PhaseTimeoutSec = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow") int32 MaxSamples = 3;
	/** Seconds between success and the travel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flow") float CompleteDelaySec = 5.f;

	// Rover look
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rover") TSoftObjectPtr<UStaticMesh> RoverMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Game/MissionCrew/Art/Meshes/Rover/Perseverance.Perseverance")));
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rover") float RoverMeshScale = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rover") float RoverYawOffset = 0.f;
};
