// S6 3D chase view: camera behind a capsule marker that descends toward Earth; debug lines draw the reentry corridor.
// No geometry is created: assign meshes in a Blueprint child (CapsuleMesh) if wanted.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCReturnView.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class AMCReturnManager;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCReturnView : public AActor
{
	GENERATED_BODY()
public:
	AMCReturnView();
	virtual void Tick(float DeltaSeconds) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> ShipRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> CapsuleMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> ChaseCamera;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CmPerKm = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float CmPerDeg = 200.f;
protected:
	UPROPERTY(Transient) TObjectPtr<AMCReturnManager> Mgr;
};
