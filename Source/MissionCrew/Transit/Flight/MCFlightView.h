// S2 3D flight view (agent F3). A deep-space scene where the spacecraft flies as the Pilot burns.
//
// Everything is READ-ONLY against the replicated AMCTransitManager values and runs on every machine (listen-server host included,
// not on a dedicated server). Clients hold no logic. No geometry is made in code: all meshes are assigned in the editor
// (components or the soft *Asset pointers) and the path / rings / trail are debug lines (not in Shipping builds).
//
// Scene (actor local space, Earth at the origin, Moon on +X, up = +Z):
//   launch pad on top of the Earth  ->  ascent (gravity turn, Bezier)  ->  coast (cubic Bezier arc to the corridor ring at the Moon).
//   Ship offset from the ideal arc = (ShipErrX, ShipErrY) km * MissScale (cm/km), blended in with progress. Ghost marker = stale display.
// Cameras: ChaseCamera (smooth boom with lag). AMCBayCharacter: Pilot sits -> chase view, key V toggles station / flight view.
// Viewscreen: optional USceneCaptureComponent2D -> render target -> dynamic material on station screen meshes.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCFlightView.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class UCameraComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class AMCTransitManager;
class AMCRocketStack;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCFlightView : public AActor
{
	GENERATED_BODY()

public:
	AMCFlightView();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------------- Components ----------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight") TObjectPtr<USceneComponent> Root;
	/** Sky sphere (inside-facing material). Follows the camera. Or use SkyActor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Scene") TObjectPtr<UStaticMeshComponent> SkyMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Scene") TObjectPtr<UStaticMeshComponent> EarthMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Scene") TObjectPtr<UStaticMeshComponent> MoonMesh;
	/** Moves along the flight path; the ship parts hang under it (+X = nose, +Z = up, rotate with the *MeshRotation properties). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Ship") TObjectPtr<USceneComponent> ShipRoot;
	/** Full stack on the pad and during the ascent (optional; empty = the lander mesh is used). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Ship") TObjectPtr<UStaticMeshComponent> RocketMesh;
	/** The lander (S1 lander). Empty = taken from the first AMCRocketStack in the world (its LanderMesh). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Ship") TObjectPtr<UStaticMeshComponent> ShipMesh;
	/** Ascent stage; stays behind at the separation after the ascent (optional). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Ship") TObjectPtr<UStaticMeshComponent> AscentMesh;
	/** Marker for the stale displayed course (optional mesh; a debug sphere is drawn if empty). Give it a translucent material. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Ship") TObjectPtr<UStaticMeshComponent> GhostMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Ship") TObjectPtr<UPointLightComponent> FlareLight;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Ship") TObjectPtr<UTextRenderComponent> FuelText;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Camera") TObjectPtr<UCameraComponent> ChaseCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flight|Viewscreen") TObjectPtr<USceneCaptureComponent2D> ViewscreenCapture;

	// ---------------- Assets (soft: assign in the level instance / Blueprint child; applied to empty components) ----------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") TSoftObjectPtr<UStaticMesh> EarthMeshAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") TSoftObjectPtr<UStaticMesh> MoonMeshAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") TSoftObjectPtr<UStaticMesh> SkyMeshAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") TSoftObjectPtr<UStaticMesh> RocketMeshAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") TSoftObjectPtr<UStaticMesh> ShipMeshAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") TSoftObjectPtr<UStaticMesh> AscentMeshAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") TSoftObjectPtr<UStaticMesh> GhostMeshAsset;
	/** Optional existing sky sphere actor of the level: kept centred on the flight camera instead of SkyMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") TSoftObjectPtr<AActor> SkyActor;
	/** Scale of the ship meshes / rotation so that +X is the nose (a rocket model with Z as its axis needs Pitch -90). Local units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") FVector ShipMeshScale = FVector(1.0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") FRotator ShipMeshRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") FVector RocketMeshScale = FVector(1.0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") FRotator RocketMeshRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") FVector AscentMeshScale = FVector(1.0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") FRotator AscentMeshRotation = FRotator::ZeroRotator;
	/** Fit the Earth, Moon and sky meshes to the radii below using their bounds. Off = keep the component scale you set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Assets") bool bAutoScaleBodies = true;

	// ---------------- Layout / scale (cm, actor local space) ----------------
	/** Optional explicit manager; otherwise the first AMCTransitManager in the world. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") TObjectPtr<AMCTransitManager> Manager;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float EarthRadius = 8000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float MoonRadius = 2500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float SkyRadius = 800000.f;
	/** Earth centre to Moon centre along +X. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float EarthMoonDistance = 200000.f;
	/** Height of the Moon centre above the Earth centre line (Z). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float MoonHeight = 6000.f;
	/** The corridor ring hangs this far in front of the Moon surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float ApproachGap = 2500.f;
	/** cm of the scene per km of predicted miss (corridor ring radius = CorridorKm * MissScale). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float MissScale = 40.f;
	/** Climb height of the ascent above the pad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float AscentLift = 9000.f;
	/** Extra bulge of the coast arc (Z) and the seeded sideways bow (+-). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float ArcHeight = 12000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float ArcBow = 6000.f;
	/** Durations used to animate the countdown / ascent on the clients (the manager config values; keep them equal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float AscentSec = 20.f;
	/** Speed of the visual follow of the true / displayed miss (1/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Layout") float OffsetInterpSpeed = 2.5f;

	// ---------------- Effects ----------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Effects") bool bDrawPath = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Effects") float LineThickness = 12.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Effects") float WindowRingRadius = 700.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Effects") int32 MaxTrailPoints = 300;
	/** Flare light intensity (candela) for a Pilot pulse / the ascent burn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Effects") float PulseFlareIntensity = 60000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Effects") float AscentFlareIntensity = 120000.f;
	/** The scale of the ship mesh pops by this fraction on a pulse. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Effects") float PulseScalePop = 0.06f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Effects") float FuelTextHeight = 700.f;
	/** Set if the fuel text reads mirrored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Effects") bool bFlipText = true;

	// ---------------- Camera ----------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Camera") float ChaseDistance = 2200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Camera") float ChaseHeight = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Camera") float ChaseSide = 350.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Camera") float LookAhead = 1500.f;
	/** Boom lag: higher = stiffer (1/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Camera") float ChaseLagSpeed = 2.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Camera") float ChaseRotLagSpeed = 4.f;

	// ---------------- Viewscreen (optional) ----------------
	/** Render the chase camera into ViewscreenTarget (needs the render target asset). Off = no capture cost at all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Viewscreen") bool bViewscreenEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Viewscreen") TObjectPtr<UTextureRenderTarget2D> ViewscreenTarget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Viewscreen", meta = (ClampMin = "1", ClampMax = "60")) float ViewscreenFps = 15.f;
	/** Texture parameter name in the screen material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Viewscreen") FName ScreenTextureParam = TEXT("ScreenTexture");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Viewscreen") int32 ScreenMaterialSlot = 0;
	/** Actors with a screen mesh. On each, mesh components tagged "FlightScreen" get the material (none tagged = the first mesh). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight|Viewscreen") TArray<TObjectPtr<AActor>> ScreenActors;

	// ---------------- API ----------------
	/** The flight view of this world (first found), or null. */
	UFUNCTION(BlueprintPure, Category = "Flight", meta = (WorldContext = "WorldContextObject"))
	static AMCFlightView* Find(const UObject* WorldContextObject);
	/** Make a dynamic instance of the material in Slot of the mesh, point ScreenTextureParam at the render target and return it. */
	UFUNCTION(BlueprintCallable, Category = "Flight|Viewscreen")
	UMaterialInstanceDynamic* CreateScreenMaterial(UMeshComponent* ScreenMesh, int32 Slot = 0);
	/** Apply the viewscreen material to ScreenActors (called in BeginPlay). */
	UFUNCTION(BlueprintCallable, Category = "Flight|Viewscreen") void ApplyViewscreen();
	UFUNCTION(BlueprintPure, Category = "Flight") FVector GetShipWorldLocation() const;
	UFUNCTION(BlueprintPure, Category = "Flight") UCameraComponent* GetChaseCamera() const { return ChaseCamera; }

	/** A Pilot pulse was seen (Dir = EMCPilotAction). Use it from a Blueprint child to add a Niagara burst. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Flight") void BP_OnPulse(int32 Dir, FVector WorldLocation);

protected:
	void ApplyAssets();
	void ApplyLayout();
	void FitToRadius(UStaticMeshComponent* Comp, float Radius) const;
	void RebuildPath();
	FVector PathPoint(float P) const;
	FVector PathTangent(float P) const;
	FVector AscentPoint(float U) const;
	FVector AscentTangent(float U) const;
	FVector MissOffset(float X, float Y, float P) const;
	void FindManager(float Dt);
	void TriggerPulse(int32 Dir);
	void UpdateCamera(float Dt);
	void UpdateFuelText();
	void DrawScene(float Dt);
	void UpdateViewscreen(float Dt);
	void SetShipVisibility(bool bAscentPhase, bool bCoast);

	// cached path (local space)
	FVector Pad, AscP1, PointA, CoastP1, CoastP2, PointE, MoonCentre;
	int32 PathSeed = -1;

	// smoothed state
	float OffX = 0.f, OffY = 0.f, GhostX = 0.f, GhostY = 0.f;
	FVector ShipLocal = FVector::ZeroVector, ShipTan = FVector::UpVector;
	float Progress = 0.f;       // 0..1 along the coast
	float CoastSecEst = 0.f;    // smoothed coast seconds
	float LastElapsed = -1.f, LastElapsedWorldTime = 0.f;
	TArray<FVector> Trail;
	bool bTrailLastValid = false;

	// effects
	int32 LastPulseSeq = -1;
	float FlareTimer = 0.f;
	int32 PuffDir = 0;
	float PuffTimer = 0.f;
	float FindTimer = 0.f;
	float CaptureAcc = 0.f;
	float TextAcc = 0.f;
	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	bool bCamInit = false;
	uint8 PrevStage = 255;
	float StageSeenWorldTime = 0.f;
	bool bViewscreenApplied = false;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> ScreenMIDs;
};
