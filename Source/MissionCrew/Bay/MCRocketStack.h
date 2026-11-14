// Constructible rocket (agent K).
//
// AMCRocketStack: one per bay, placed on the launch pad (where the maquette stands). It reads the installed parts of the
// ASCENT group straight from the AMCPartSocket actors (their Installed def is replicated, so every machine builds the same
// stack locally; there is nothing extra to replicate) and rebuilds a Kerbal-style visual rocket from modular meshes:
//
//   core (bottom -> top):  Engine, Tank, Decoupler, Other, Capsule, Fairing, then the lander model on top.
//   radial:                parts with bRadial / StackKind Booster are placed around the core, UMCPartDef::RadialCount copies each
//                          (all radial copies of the whole stack are spread evenly: 1 -> one side, 2 -> opposite, 4 -> cross).
//
// Everything is data-driven by UMCPartDef (StackKind, StackHeightCm, StackRadiusCm, bRadial, RadialCount, StackMesh,
// StackMeshScale, StackMeshRotation; fall back to Mesh/MeshScale). No asset path is hard-coded.
//
// Smooth visuals: new parts drop/fly into place, the parts above shift up when a segment is added (critically smoothed),
// and the whole stack is scaled to stay under MaxStackHeightCm (hall height) via DisplayScale / bAutoFitHeight.
//
// Picker ghost: ShowGhost(Def) shows the candidate translucent at the slot where it would go (the rest of the stack shifts to
// make room); ClearGhost() removes it. UMCPartPickerWidget calls these.
//
// Sockets: UMCRocketStackLibrary::GetAscentSlots lists the ordered slots (core 0..N, radial 0..3); SpawnAscentSockets spawns
// AMCPartSocket actors for them (group Ascent, correct types). The stack does not need its sockets to be at the pad: it only
// reads installed state. Installed part actors of ascent sockets are hidden (bHideSocketPartActors) because the stack draws them.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Construction/MCConstructionTypes.h"
#include "MCRocketStack.generated.h"

class AMCPartSocket;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/** One ordered slot of the ascent stack. Core slots go bottom -> top, radial slots go around. */
USTRUCT(BlueprintType)
struct FMCStackSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName Name;
	/** 0..N for core slots (bottom first); 0..3 for radial slots. */
	UPROPERTY(BlueprintReadOnly) int32 Index = 0;
	UPROPERTY(BlueprintReadOnly) bool bRadial = false;
	/** Socket type the slot accepts. Engine / Tank / Structure (decoupler, capsule, fairing) / Booster. */
	UPROPERTY(BlueprintReadOnly) EMCSocketType SocketType = EMCSocketType::Tank;
	UPROPERTY(BlueprintReadOnly) EMCStackGroup Group = EMCStackGroup::Ascent;
	/** Offset from the interaction anchor (the stack actor's transform), cm. */
	UPROPERTY(BlueprintReadOnly) FVector LocalOffset = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly) float YawDeg = 0.f;
};

USTRUCT(BlueprintType)
struct FMCStackLayoutParams
{
	GENERATED_BODY()

	/** Core slots, bottom first: engines, then tanks, then structure (decoupler / capsule / fairing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) int32 NumEngineSlots = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) int32 NumTankSlots = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) int32 NumStructureSlots = 2;
	/** Radial booster slots (the visible stack spreads installed copies evenly, so 4 supports 1, 2 or 4 boosters). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0")) int32 NumRadialSlots = 4;
	/** Vertical distance of the core interaction column, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CoreSpacingCm = 220.f;
	/** The core slot column stands beside the rocket (so aiming at it does not need the live stack height). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector CoreColumnOffset = FVector(-420.f, 0.f, 60.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RadialRingRadiusCm = 330.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RadialHeightCm = 100.f;
};

UCLASS()
class MISSIONCREW_API UMCRocketStackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Ordered slot list. Names: Ascent_Engine_0, Ascent_Tank_0..N, Ascent_Structure_0..N, Ascent_Booster_0..3.
	 * Order = core engines, tanks, structure (bottom -> top, Index 0..N-1 across ALL core slots), then radial 0..3.
	 */
	UFUNCTION(BlueprintPure, Category = "Rocket Stack")
	static TArray<FMCStackSlot> GetAscentSlots(const FMCStackLayoutParams& Params);

	/**
	 * Spawns one AMCPartSocket per slot (SocketGroup = Ascent, SocketType per slot) relative to Origin. Server/editor only.
	 * Returns the sockets. Pass SocketClass null for AMCPartSocket (or a Blueprint child with visuals).
	 */
	UFUNCTION(BlueprintCallable, Category = "Rocket Stack", meta = (WorldContext = "WorldContextObject"))
	static TArray<AMCPartSocket*> SpawnAscentSockets(UObject* WorldContextObject, const FTransform& Origin, const FMCStackLayoutParams& Params, TSubclassOf<AMCPartSocket> SocketClass);

	/** Sort rank bottom -> top for a core part (Engine 0, Tank 1, Decoupler 2, Other 3, Capsule 4, Fairing 5). */
	UFUNCTION(BlueprintPure, Category = "Rocket Stack")
	static int32 CoreRank(EMCStackKind Kind);
};

/** Runtime state of one drawn mesh piece. */
USTRUCT()
struct FMCStackVisual
{
	GENERATED_BODY()

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Comp = nullptr;
	TWeakObjectPtr<AMCPartSocket> Socket;
	TWeakObjectPtr<const UMCPartDef> Def;
	int32 Copy = 0;
	bool bGhost = false;
	bool bLander = false;
	FVector Target = FVector::ZeroVector;
	FVector Cur = FVector::ZeroVector;
};

UCLASS(Blueprintable)
class MISSIONCREW_API AMCRocketStack : public AActor
{
	GENERATED_BODY()

public:
	AMCRocketStack();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The stack of this world (first found). Null if none is placed. */
	static AMCRocketStack* Find(const UWorld* World);

	/** Translucent preview of a candidate at the slot where it would be installed (local, not replicated). Lander-group defs are ignored. */
	UFUNCTION(BlueprintCallable, Category = "Rocket Stack") void ShowGhost(const UMCPartDef* Def);
	UFUNCTION(BlueprintCallable, Category = "Rocket Stack") void ClearGhost();
	/** Force a rebuild next tick (after changing properties in a Blueprint). */
	UFUNCTION(BlueprintCallable, Category = "Rocket Stack") void MarkDirty() { bDirty = true; }

	/** Total height of the real (non-ghost) stack including the lander, cm, before display scaling. */
	UFUNCTION(BlueprintPure, Category = "Rocket Stack") float GetStackHeightCm() const { return RealHeightCm; }
	/** Height including the ghost, before display scaling. */
	UFUNCTION(BlueprintPure, Category = "Rocket Stack") float GetHeightWithGhostCm() const { return TotalHeightCm; }
	UFUNCTION(BlueprintPure, Category = "Rocket Stack") float GetDisplayedScale() const { return CurScale; }

	// ---- Config ----
	/** Desired display scale of the whole stack (1 = real data size). Reduced further by bAutoFitHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float DisplayScale = 1.f;
	/** Shrink the stack so its height (ghost included) stays under MaxStackHeightCm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") bool bAutoFitHeight = true;
	/** Hall limit, cm (hall is 40 m; keep margin for the crane). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float MaxStackHeightCm = 3500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float ScaleInterpSpeed = 2.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float PartInterpSpeed = 3.5f;
	/** New parts start this high above their place (radial parts also 500 cm outward). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float DropHeightCm = 1500.f;
	/** Hide the ascent sockets' own installed part actors, since the stack draws the parts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") bool bHideSocketPartActors = true;
	/** Translucent material for the ghost. Defaults to an engine debug material; if it cannot load the ghost pulses instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") TSoftObjectPtr<UMaterialInterface> GhostMaterial;

	/** Height/radius used for a part that has no mesh and no StackHeightCm / StackRadiusCm (a gap, never an invented mesh). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fallback") float EmptyPartHeightCm = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fallback") float EmptyPartRadiusCm = 60.f;
	/** Core radius used for the radial offset when no core part is installed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fallback") float DefaultCoreRadiusCm = 100.f;
	/** Angle of the first radial copy, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial") float RadialStartYawDeg = 45.f;

	// ---- Lander on top ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lander") bool bShowLander = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lander") TSoftObjectPtr<UStaticMesh> LanderMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lander") FVector LanderScale = FVector(1.0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lander") FRotator LanderRotation = FRotator::ZeroRotator;
	/** > 0: fit the lander mesh to this height (cm, uniform scale). 0 = use LanderScale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lander") float LanderHeightCm = 0.f;

	// ---- Sockets ----
	/** Server: if no ascent sockets exist at BeginPlay, spawn them from SocketLayout at this actor's transform and register them with the manager. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets") bool bAutoSpawnSockets = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets") FMCStackLayoutParams SocketLayout;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sockets") TSubclassOf<AMCPartSocket> SocketClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") TObjectPtr<USceneComponent> StackRoot;

private:
	struct FEntry
	{
		const UMCPartDef* Def = nullptr;
		AMCPartSocket* Socket = nullptr;
		bool bGhost = false;
	};

	struct FGeom
	{
		UStaticMesh* Mesh = nullptr;
		FVector Scale = FVector(1.0);
		FRotator Rot = FRotator::ZeroRotator;
		float Height = 0.f;
		float Radius = 0.f;
		/** Component location that puts the mesh's bottom-centre at the origin. */
		FVector Pivot = FVector::ZeroVector;
	};

	static FGeom ComputeGeom(UStaticMesh* Mesh, const FVector& Scale, const FRotator& Rot, float FitHeight, float FitRadius, float EmptyH, float EmptyR);
	FGeom GeomForDef(const UMCPartDef* Def) const;
	FGeom GeomForLander() const;

	void RefreshSocketCache();
	bool CollectSignature(TArray<const void*>& Out) const;
	void Rebuild();
	void ApplyVisual(FMCStackVisual& V, UStaticMesh* Mesh, const FVector& Scale, const FRotator& Rot);
	void HideSocketActors();

	UPROPERTY() TArray<FMCStackVisual> Visuals;
	TArray<TWeakObjectPtr<AMCPartSocket>> CachedSockets;
	TArray<const void*> LastSignature;
	TWeakObjectPtr<const UMCPartDef> Ghost;
	float SocketCacheTimer = 0.f;
	float RealHeightCm = 0.f;
	float TotalHeightCm = 0.f;
	float CurScale = 1.f;
	bool bDirty = true;
	bool bScaleInit = false;
};
