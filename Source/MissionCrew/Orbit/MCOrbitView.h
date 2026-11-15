// S3 3D landing view. READ-ONLY against the replicated AMCOrbitManager values; runs on every machine, holds no logic.
// No custom geometry: the Moon is the ENGINE sphere (/Engine/BasicShapes/Sphere) with MI_Moon (same style as AMCFlightView),
// the lander is the S1 lander mesh (copied from AMCRocketStack::LanderMesh like AMCFlightView). Hazard / target rings are debug lines.
// Scene (actor local space, cm): the landing site is the origin on top of the Moon sphere (centre at Z = -MoonRadius);
// orbit phase = ship circling the Moon, converging over the site; descent = ship at (PosX, PosY, AltM) * 100.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCOrbitView.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UCameraComponent;
class UPointLightComponent;
class AMCOrbitManager;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCOrbitView : public AActor
{
	GENERATED_BODY()
public:
	AMCOrbitView();
	virtual void OnConstruction(const FTransform& T) override;
	virtual void BeginPlay() override;
	virtual void Tick(float Dt) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit") TObjectPtr<UStaticMeshComponent> MoonMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit") TObjectPtr<USceneComponent> ShipRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit") TObjectPtr<UStaticMeshComponent> ShipMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit") TObjectPtr<UPointLightComponent> FlareLight;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit") TObjectPtr<UCameraComponent> ChaseCamera;

	/** Engine sphere by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Assets") TSoftObjectPtr<UStaticMesh> MoonMeshAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Assets") TSoftObjectPtr<UMaterialInterface> MoonMaterialAsset;
	/** Optional: assigned in the level, else the first AMCRocketStack LanderMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Assets") TSoftObjectPtr<UStaticMesh> ShipMeshAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Assets") FVector ShipMeshScale = FVector(1.0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Assets") FRotator ShipMeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Layout") TObjectPtr<AMCOrbitManager> Manager;
	/** Moon sphere radius (cm). Large, so the patch under the descent looks flat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Layout") float MoonRadius = 2000000.f;
	/** Orbit altitude above the site (cm) and the angular rate (rad/s) of the approach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Layout") float OrbitAltitude = 200000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Layout") float OrbitRate = 0.004f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Layout") float StartAngle = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Layout") float FollowSpeed = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Effects") bool bDrawMarkers = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Effects") float FlareIntensity = 80000.f;

	// Camera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Camera") float ChaseDistance = 2500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Camera") float ChaseHeight = 900.f;
	/** Take over the local player's view target after the level starts (the Pilot and all roles watch the scene). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Camera") bool bAutoViewTarget = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit|Camera") float ViewTargetDelaySec = 1.f;

	UFUNCTION(BlueprintPure, Category = "Orbit") FVector GetShipWorldLocation() const;
	/** Dust at low altitude and touchdown: hook a Niagara emitter in a Blueprint child. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Orbit") void BP_OnTouchdown(FVector WorldLocation);

protected:
	void ApplyAssets();
	FVector OrbitPoint(float Angle) const;
	float FindTimer = 0.f, ViewTimer = 0.f;
	bool bViewSet = false, bTouchFired = false;
	FVector ShipLocal = FVector::ZeroVector;
	FRotator ShipRot = FRotator::ZeroRotator;
	bool bInit = false;
	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	bool bCamInit = false;
};
