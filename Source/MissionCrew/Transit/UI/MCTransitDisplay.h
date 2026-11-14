// Shared holo trajectory view for S2 (agent S2b). No self-made geometry: the three meshes are assigned in the editor.
//
// Components (assign StaticMesh in the Blueprint child / level instance):
//   EarthMesh, MoonMesh   spheres from the NASA 3D / LOLA set (placed on the X axis, EarthMoonDistance apart, centred on the actor)
//   ShipMarker            any small existing mesh; moved along the path, faces the direction of travel
//   CountdownText         UTextRenderComponent above the Moon-Earth line (stage, clock)
// The path is drawn with debug lines every frame (not in Shipping builds):
//   cyan   Earth -> ship (flown)         green/amber  ship -> predicted impact point (green inside the corridor)
//   a corridor circle around the Moon, radius CorridorKm * MissScale.
// Reads the replicated AMCTransitManager values (Stage, ElapsedSec, TransitDurationSec, DispX/DispY, CorridorKm).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCTransitDisplay.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class AMCTransitManager;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCTransitDisplay : public AActor
{
	GENERATED_BODY()
public:
	AMCTransitDisplay();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display") TObjectPtr<UStaticMeshComponent> EarthMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display") TObjectPtr<UStaticMeshComponent> MoonMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display") TObjectPtr<UStaticMeshComponent> ShipMarker;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Display") TObjectPtr<UTextRenderComponent> CountdownText;

	/** Optional explicit manager; otherwise the first AMCTransitManager in the world is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") TObjectPtr<AMCTransitManager> Manager;

	/** Distance Earth centre - Moon centre (cm, local space). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float EarthMoonDistance = 400.f;
	/** Radii used for the path start/end (cm); set them to the radii of the assigned meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float EarthRadius = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float MoonRadius = 20.f;
	/** Cm of the hologram per km of predicted miss. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float MissScale = 1.f;
	/** Lift of the ship during the ascent (cm above the Earth surface at the end of the ascent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float AscentLift = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") bool bDrawPath = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display") float LineThickness = 1.5f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	void UpdateText();
	float FindTimer = 0.f;
};
