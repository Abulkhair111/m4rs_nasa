// Shared contract for the Mission Crew construction (S1 Design & Assembly) C++ code.
// Everything other C++ files (part actors, movers, pawn, HUD) need to agree on lives here.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MCConstructionTypes.generated.h"

class UStaticMesh;

/** Lunar gravity used by the T/W formula (m/s^2). */
namespace MCConst
{
	constexpr float MoonG = 1.62f;
	constexpr float TWRRed = 1.3f;
	constexpr float TWRGreen = 2.0f;
	constexpr int32 NumChecks = 10;
	constexpr int32 NumRoles = 5;      // index 0 unused-by-convention (role 0 = free/unassigned), 1..4 real roles
	constexpr int32 MaxSocketType = 10;
}

/** Matches BP_PartSocket.SocketType. 99 = tool (never installed). */
UENUM(BlueprintType)
enum class EMCSocketType : uint8
{
	Power    = 0,
	Battery  = 1,
	Comms    = 2,
	Science  = 3,
	Drill    = 4,
	Rover    = 5,
	Engine   = 6,
	Lidar    = 7,
	Tank     = 8,
	/** Solid rocket booster (ascent group). Self-powered: needs no cable. */
	Booster  = 9,
	/** Landing gear / legs (lander group). */
	Gear     = 10,
	/** Passive ascent-stack structure: decoupler/interstage, capsule, fairing. Self-powered, no cable (see AMCRocketStack). */
	Structure = 11,
	Tool     = 99
};

/**
 * Which half of the vehicle a part or socket belongs to (design change 2026-11-14, see Vault/Decisions.md).
 * Lander = what lands on the Moon (science, gear, rover, drill, lidar, power, batteries, comms, RTG).
 * Ascent = what takes off from the Moon and returns (boosters, fuel tanks, engines).
 * A part only installs on a socket of the same group.
 */
UENUM(BlueprintType)
enum class EMCStackGroup : uint8
{
	Lander = 0,
	Ascent = 1
};

/** Tanks, engines and boosters are always ascent hardware, whatever a data asset says. */
inline bool MCIsAscentOnlyType(EMCSocketType T)
{
	return T == EMCSocketType::Tank || T == EMCSocketType::Engine || T == EMCSocketType::Booster || T == EMCSocketType::Structure;
}

/** What a part looks like in the visible rocket (AMCRocketStack). Other = inferred from SocketType (Engine/Tank/Booster), else a plain core segment. */
UENUM(BlueprintType)
enum class EMCStackKind : uint8
{
	Other     = 0,
	Capsule   = 1,
	Fairing   = 2,
	Tank      = 3,
	Engine    = 4,
	Booster   = 5,
	Decoupler = 6
};

/** CrewRole number == department number. 0 = unassigned / solo (everything allowed). */
UENUM(BlueprintType)
enum class EMCCrewRole : uint8
{
	None         = 0,
	Power        = 1,
	Construction = 2,
	Propulsion   = 3,
	Science      = 4
};

/** Matches PDA_PartDef.ToolType. */
UENUM(BlueprintType)
enum class EMCToolType : uint8
{
	None   = 0,  // an installable part
	Wrench = 1,  // toggles socket bBolted
	Cable  = 2   // toggles socket bGrid
};

/** Preflight check state: 0 green, 1 yellow, 2 red (same numbers as the Blueprint). */
UENUM(BlueprintType)
enum class EMCCheckState : uint8
{
	Green  = 0,
	Yellow = 1,
	Red    = 2
};

/** Indices into AMCConstructionManager::Checks. */
UENUM(BlueprintType)
enum class EMCCheck : uint8
{
	Launcher  = 0,
	Mass      = 1,
	Budget    = 2,
	Power     = 3,
	Comms     = 4,
	Fuel      = 5,
	Thrust    = 6,
	IceScience = 7,
	Grid      = 8,
	Structure = 9
};

/**
 * Part definition. C++ equivalent of the Blueprint PrimaryDataAsset /Game/MissionCrew/Construction/Data/PDA_PartDef.
 * Property names intentionally match the Blueprint (same spelling, minus the Blueprint-only quirks) so a later
 * reparent / data migration can map fields 1:1. Existing DA_Part_* assets are NOT touched by this class.
 */
UCLASS(BlueprintType)
class MISSIONCREW_API UMCPartDef : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType(TEXT("MCPartDef")), GetFName());
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part") FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part") FText RealAnalog;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part", meta = (MultiLine = true)) FText Description;

	/** Department == CrewRole number (1 Power, 2 Construction, 3 Propulsion, 4 Science). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part") int32 Department = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part") EMCSocketType SocketType = EMCSocketType::Power;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part") EMCToolType ToolType = EMCToolType::None;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part") bool bHeavy = false;
	/** Lander (default) or Ascent. Must equal the target socket's SocketGroup. Tanks, engines and boosters are Ascent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Part") EMCStackGroup Group = EMCStackGroup::Lander;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float MassKg = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float CostM = 0.f;
	/** +generates, -consumes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float PowerW = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float StorageWh = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float DataMbps = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float BandwidthMbps = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float ScienceValue = 0.f;
	/** 0..1; 0 is treated as "no effect" (1.0) like the Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float Reliability = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float ThrustN = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") float FuelCapKg = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats") bool bIceCapable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual") TObjectPtr<UStaticMesh> Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual") FVector MeshScale = FVector(1.0);
	/** Alpha 0 = keep the mesh's own materials. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual") FLinearColor Color = FLinearColor(0, 0, 0, 0);

	// ---- Rocket stack visual (AMCRocketStack). All data-driven; the shelf/in-hand look still uses Mesh. ----
	/** Role in the visible stack. Other = inferred from SocketType (see GetEffectiveStackKind). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack") EMCStackKind StackKind = EMCStackKind::Other;
	/** Height this part adds to the core stack, cm (unscaled). 0 = measure the mesh. If > 0 the mesh is fitted to it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack") float StackHeightCm = 0.f;
	/** Body radius, cm. 0 = measure the mesh. Used for the radial offset of side boosters. If > 0 the mesh XY is fitted to it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack") float StackRadiusCm = 0.f;
	/** Attach beside the core instead of on top (side boosters; the Booster kind is always radial). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack") bool bRadial = false;
	/** Copies placed around the core by ONE installed part when radial (1, 2 or 4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack", meta = (ClampMin = "1", ClampMax = "8")) int32 RadialCount = 2;
	/** Mesh used in the rocket stack. Empty = fall back to Mesh (with MeshScale). Soft so it loads on demand. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack") TSoftObjectPtr<UStaticMesh> StackMesh;
	/** Scale of StackMesh (ignored for the Mesh fallback, which uses MeshScale). Rotation applies to both; local Z must end up as the rocket axis. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack") FVector StackMeshScale = FVector(1.0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack") FRotator StackMeshRotation = FRotator::ZeroRotator;

	/** Stack role with the fallbacks: explicit StackKind, else from SocketType, else Other. */
	EMCStackKind GetEffectiveStackKind() const
	{
		if (StackKind != EMCStackKind::Other) return StackKind;
		switch (SocketType)
		{
		case EMCSocketType::Engine:  return EMCStackKind::Engine;
		case EMCSocketType::Tank:    return EMCStackKind::Tank;
		case EMCSocketType::Booster: return EMCStackKind::Booster;
		default:                     return EMCStackKind::Other;
		}
	}
	bool IsRadialInStack() const { return bRadial || GetEffectiveStackKind() == EMCStackKind::Booster; }

	/** Group actually used by the rules: tank/engine/booster parts are forced to Ascent so a forgotten Group value cannot put fuel on the lander. */
	EMCStackGroup GetStackGroup() const { return MCIsAscentOnlyType(SocketType) ? EMCStackGroup::Ascent : Group; }
	bool IsTool() const { return ToolType != EMCToolType::None; }
	float GetReliabilityFactor() const { return Reliability > 0.f ? Reliability : 1.f; }
};

/** Sub-totals of one group (lander or ascent). Replicated inside AMCConstructionManager. */
USTRUCT(BlueprintType)
struct FMCGroupTotals
{
	GENERATED_BODY()

	/** Lander: LanderBaseMassKg + parts. Ascent: parts + the fuel currently loaded. */
	UPROPERTY(BlueprintReadOnly) float MassKg = 0.f;
	UPROPERTY(BlueprintReadOnly) float CostM = 0.f;
	UPROPERTY(BlueprintReadOnly) float PowerGenW = 0.f;
	UPROPERTY(BlueprintReadOnly) float PowerUseW = 0.f;
	UPROPERTY(BlueprintReadOnly) float StorageWh = 0.f;
	UPROPERTY(BlueprintReadOnly) float DataMbps = 0.f;
	UPROPERTY(BlueprintReadOnly) float BandwidthMbps = 0.f;
	UPROPERTY(BlueprintReadOnly) float ScienceValue = 0.f;
	UPROPERTY(BlueprintReadOnly) float ThrustN = 0.f;
	UPROPERTY(BlueprintReadOnly) float FuelMaxKg = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 PartCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 IceCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 UnboltedCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 OffGridCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 BoosterCount = 0;
	UPROPERTY(BlueprintReadOnly) int32 EngineCount = 0;
};

/** Per-group part of a Preview. Only the fields of the group the previewed part belongs to change. */
USTRUCT(BlueprintType)
struct FMCGroupPreview
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) float MassKg = 0.f;
	UPROPERTY(BlueprintReadOnly) float CostM = 0.f;
	/** Lander: generated - used (W). */
	UPROPERTY(BlueprintReadOnly) float PowerBalance = 0.f;
	UPROPERTY(BlueprintReadOnly) float ScienceValue = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 IceCount = 0;
	/** Ascent: thrust, tank capacity and booster count after the install. */
	UPROPERTY(BlueprintReadOnly) float ThrustN = 0.f;
	UPROPERTY(BlueprintReadOnly) float FuelMaxKg = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 BoosterCount = 0;
};

/** Predicted result of installing one more part, returned by AMCConstructionManager::Preview. No state is written. */
USTRUCT(BlueprintType)
struct FMCPreviewResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) float TWR = 0.f;
	/** (fuel - required) / required, required = 0.9 * dry mass. Negative = short. */
	UPROPERTY(BlueprintReadOnly) float FuelMargin = 0.f;
	/** Generated - used (W). */
	UPROPERTY(BlueprintReadOnly) float PowerBalance = 0.f;
	UPROPERTY(BlueprintReadOnly) float TotalMassKg = 0.f;
	UPROPERTY(BlueprintReadOnly) float TotalCostM = 0.f;
	/** TWR colour class with the 1.3 / 2.0 thresholds (needs a live engine; Preview assumes the new part is wired). */
	UPROPERTY(BlueprintReadOnly) EMCCheckState TWRState = EMCCheckState::Red;
	UPROPERTY(BlueprintReadOnly) EMCCheckState FuelState = EMCCheckState::Red;
	UPROPERTY(BlueprintReadOnly) EMCCheckState PowerState = EMCCheckState::Red;
	UPROPERTY(BlueprintReadOnly) bool bAffordable = true;
	/** Group of the previewed part, and the group sub-results after installing it. */
	UPROPERTY(BlueprintReadOnly) EMCStackGroup Group = EMCStackGroup::Lander;
	UPROPERTY(BlueprintReadOnly) FMCGroupPreview Lander;
	UPROPERTY(BlueprintReadOnly) FMCGroupPreview Ascent;
};

/** Launch rocket option (lever). Placeholder numbers until the design doc fixes them. */
USTRUCT(BlueprintType)
struct FMCRocketDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PayloadKg = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CostM = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Reliability = 1.f;
};

/** One line of the loadout handed to S2 (Launch & Transit) when launch completes. */
USTRUCT(BlueprintType)
struct FMCLoadoutEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) EMCSocketType SocketType = EMCSocketType::Power;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UMCPartDef> Def = nullptr;
	UPROPERTY(BlueprintReadOnly) bool bBolted = false;
	UPROPERTY(BlueprintReadOnly) bool bGrid = false;
};

/** Everything S2 needs after TryLaunch. */
USTRUCT(BlueprintType)
struct FMCLoadout
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName RocketName;
	UPROPERTY(BlueprintReadOnly) TArray<FMCLoadoutEntry> Parts;
	UPROPERTY(BlueprintReadOnly) float FuelKg = 0.f;
	UPROPERTY(BlueprintReadOnly) float TotalMassKg = 0.f;
	UPROPERTY(BlueprintReadOnly) float TWR = 0.f;
	UPROPERTY(BlueprintReadOnly) float Reliability = 1.f;
	UPROPERTY(BlueprintReadOnly) float ScienceValue = 0.f;
	UPROPERTY(BlueprintReadOnly) int32 IceCount = 0;
};

/** Fired on the server after a successful launch. The game mode binds this to ConstructionFinished. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMCLaunchCompleteSignature, const FMCLoadout&, Loadout);
