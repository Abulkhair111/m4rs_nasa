// S3 Orbit & Landing: shared contract (types, action codes, config asset). Pattern copied from Transit/MCTransitTypes.h.
// Action code = Role * 100 + Action. Roles (slots): 1 Systems Operator, 2 Safety Officer, 3 Pilot, 4 Landing Analyst.
// Special codes (role 0): 90 = GO, 91 = NO-GO. Every number is a TBD placeholder (spec section 1 #7).
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MCOrbitTypes.generated.h"

UENUM(BlueprintType)
enum class EMCOrbitStage : uint8
{
	GoPoll    = 0,   // GO / NO-GO poll (each role may hold with a reason)
	Insertion = 1,   // orbit insertion burn (Pilot), pre-checks, survey starts
	Survey    = 2,   // rank sites, mark hazards, plan approach
	SiteLock  = 3,   // proposal, safety confirm / reject
	Descent   = 4,   // powered descent + Hazard surprise
	Report    = 5    // touchdown result
};

UENUM(BlueprintType)
enum class EMCOrbitRole : uint8 { None = 0, Systems = 1, Safety = 2, Pilot = 3, Analyst = 4 };

namespace MCOrbit
{
	constexpr int32 GoCode = 90;
	constexpr int32 NoGoCode = 91;
	constexpr int32 NumRoles = 5;          // index 0 shared, 1..4 roles
	constexpr int32 MaxSites = 4;
	constexpr int32 HazardsPerSite = 6;
	constexpr int32 NumRails = 4;
	constexpr int32 NumZones = 4;          // 0 N (+Y), 1 E (+X), 2 S (-Y), 3 W (-X)

	inline int32 MakeCode(EMCOrbitRole Role, int32 Action) { return static_cast<int32>(Role) * 100 + Action; }

	// Role 1 Systems: 0..3 toggle rail, 4 report limits
	constexpr int32 SysReportLimits = 4;
	// Role 2 Safety: 0..5 mark/unmark candidate of the viewed site, 10..13 declare divert zone clear,
	//                20..23 view site, 30 confirm proposed site, 31 reject (use backup)
	constexpr int32 SafeMarkBase = 0, SafeZoneBase = 10, SafeViewBase = 20, SafeConfirm = 30, SafeReject = 31;
	// Role 3 Pilot: 0/1 throttle +-10%, 2/3 +-2%, 4 W, 5 E, 6 N, 7 S nudge, 8 hover, 9 cut, 10 stop horizontal,
	//               20 steep approach, 21 shallow approach, 30 insertion burn
	constexpr int32 PilThrUpBig = 0, PilThrDnBig = 1, PilThrUpSm = 2, PilThrDnSm = 3, PilWest = 4, PilEast = 5, PilNorth = 6, PilSouth = 7,
		PilHover = 8, PilCut = 9, PilStop = 10, PilSteep = 20, PilShallow = 21, PilInsertion = 30;
	// Role 4 Analyst: 0..3 rank site (append), 4 reset ranking, 5 propose top site, 10..13 pick divert zone
	constexpr int32 AnaRankBase = 0, AnaReset = 4, AnaPropose = 5, AnaZoneBase = 10;
}

/** Candidate hazard on a site (replicated). Whether it is real is server-only. Position in metres relative to the site centre (+X east, +Y north). */
USTRUCT(BlueprintType)
struct FMCOrbitHazard
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Site = 0;
	/** 0 rock, 1 steep slope, 2 crater. */
	UPROPERTY(BlueprintReadOnly) int32 Type = 0;
	UPROPERTY(BlueprintReadOnly) float X = 0.f;
	UPROPERTY(BlueprintReadOnly) float Y = 0.f;
	UPROPERTY(BlueprintReadOnly) float Radius = 8.f;
	UPROPERTY(BlueprintReadOnly) bool bMarked = false;
};

/** Candidate landing site as the Analyst sees it (replicated). */
USTRUCT(BlueprintType)
struct FMCOrbitSite
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	/** Displayed ice likelihood 0..1 (noisy; -1 = no data without an ice instrument). */
	UPROPERTY(BlueprintReadOnly) float DisplayIce = -1.f;
	/** Position on the shared map in metres (placeholder), for views. */
	UPROPERTY(BlueprintReadOnly) float MapX = 0.f;
	UPROPERTY(BlueprintReadOnly) float MapY = 0.f;
	/** Analyst rank, 0 = not ranked. */
	UPROPERTY(BlueprintReadOnly) int32 Rank = 0;
};

/** Lander rail on the Systems console (replicated). */
USTRUCT(BlueprintType)
struct FMCOrbitRail
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Name;
	UPROPERTY(BlueprintReadOnly) bool bOn = true;
	UPROPERTY(BlueprintReadOnly) bool bTripped = false;
};

/** Site definition of the config (data-driven placeholder, not a claim about real lunar sites). */
USTRUCT(BlueprintType)
struct FMCOrbitSiteDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Name;
	/** True ice likelihood 0..1 (the seed jitters it a little). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float IceTrue = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MapX = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float MapY = 0.f;
	/** Real hazards among the 6 candidates; < 0 = derive from the ice value (richer ice = more hazards). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RealHazards = -1;
};

/** Final numbers of S3 (carried in UMCMissionSubsystem::Landing for S4+). */
USTRUCT(BlueprintType)
struct FMCOrbitReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 Score = 0;
	UPROPERTY(BlueprintReadOnly) bool bLanded = false;
	UPROPERTY(BlueprintReadOnly) bool bCrashed = false;
	UPROPERTY(BlueprintReadOnly) int32 ChosenSite = -1;
	UPROPERTY(BlueprintReadOnly) FString ChosenSiteName;
	/** TRUE ice likelihood of the chosen site (feeds S4/S5 yield). */
	UPROPERTY(BlueprintReadOnly) float IceLikelihood = 0.f;
	/** 0..1 match of the Analyst ranking with the reference. */
	UPROPERTY(BlueprintReadOnly) float RankingMatch = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 MarksCorrect = 0;
	UPROPERTY(BlueprintReadOnly) int32 MarksWrong = 0;
	UPROPERTY(BlueprintReadOnly) int32 HazardsMissed = 0;
	UPROPERTY(BlueprintReadOnly) bool bEventHandled = false;
	UPROPERTY(BlueprintReadOnly) bool bLimitsReported = false;
	UPROPERTY(BlueprintReadOnly) bool bLegsOk = true;
	UPROPERTY(BlueprintReadOnly) float TouchdownVertMps = 0.f;
	UPROPERTY(BlueprintReadOnly) float TouchdownHorizMps = 0.f;
	UPROPERTY(BlueprintReadOnly) float TargetDistM = 0.f;
	UPROPERTY(BlueprintReadOnly) float FuelLeftKg = 0.f;
	UPROPERTY(BlueprintReadOnly) float Health = 100.f;
	UPROPERTY(BlueprintReadOnly) int32 Seed = 0;
	UPROPERTY(BlueprintReadOnly) TArray<int32> RoleGood;   // index 1..4
	UPROPERTY(BlueprintReadOnly) TArray<int32> RoleBad;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMCOrbitCompleteSignature, const FMCOrbitReport&, Report);

/** All S3 numbers (TBD placeholders). The manager works without an asset (default instance), so an asset is only for tuning. */
UCLASS(BlueprintType)
class MISSIONCREW_API UMCOrbitConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UMCOrbitConfig();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType(TEXT("MCOrbitConfig")), GetFName());
	}

	// ---- Timeline ----
	/** The GO poll is forced to continue after this long (seconds); open roles count as a bad GO. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float GoPollTimeoutSec = 40.f;
	/** A correct NO-GO holds the poll this long before the issue is cleared. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float NoGoHoldSec = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float InsertionSec = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float SurveySec = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float SiteLockSec = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float DescentMaxSec = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timeline") float ReportHoldSec = 15.f;

	// ---- Defaults when there is no S2 report (test) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults") float DefaultFuelKg = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defaults") float DefaultHealth = 100.f;

	// ---- Insertion ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Insertion") float InsertionFuelKg = 15.f;
	/** Extra fuel when the S2 course ended outside the corridor, per km of miss. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Insertion") float InsertionFuelPerKmOff = 0.5f;

	// ---- Ice data ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ice") float IceNoiseCalibrated = 0.04f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ice") float IceNoiseUncalibrated = 0.20f;

	// ---- Descent physics (metres, seconds) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float MoonG = 1.62f;
	/** Engine acceleration at full throttle (m/s^2). Hover throttle = g / this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float MaxAccel = 4.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float BurnKgPerSec = 3.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float StartAltM = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float StartVz = -15.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float SteepVz = -22.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float ShallowVz = -10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float ShallowFuelKg = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float StartOffsetM = 120.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float NudgeMps = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float NudgeFuelKg = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float StopFuelKgPerMps = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float ThrottleStepBig = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float ThrottleStepSmall = 0.02f;
	/** Engine pressure rail off: thrust scale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Descent") float LowPressureThrustFrac = 0.6f;

	// ---- Landing limits ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing") float SafeRadiusM = 35.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing") float LegRadiusM = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing") float MaxVertSpeed = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing") float MaxHorizSpeed = 1.5f;
	/** Legs unlock rail off, or S2 equipment tests incomplete: tighter vertical limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing") float LegsFailMaxVert = 1.8f;
	/** Above this multiple of the limits the lander is destroyed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Landing") float CrashFactor = 1.6f;

	// ---- Events ----
	/** Hazard surprise: the boulder field appears this long after the descent starts (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float EventDelayMinSec = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float EventDelayMaxSec = 9.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float BoulderRadiusM = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float ZoneOffsetM = 120.f;
	/** A lander rail trips this long after the descent starts (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float RailTripMinSec = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float RailTripMaxSec = 22.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Events") float TouchdownHealthLossPerMps = 10.f;

	// ---- Scoring weights (sum 100) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring") float WeightRanking = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring") float WeightHazards = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring") float WeightSystems = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring") float WeightLanding = 35.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scoring") float WeightFuelHealth = 10.f;

	// ---- Sites (placeholder data) ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sites") TArray<FMCOrbitSiteDef> Sites;
};
