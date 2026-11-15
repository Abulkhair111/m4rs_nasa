// S4: the 3D rover. Moved by AMCSurfaceManager (server); movement replicates. The mesh is a soft reference (Perseverance by default).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCSurfaceRover.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class MISSIONCREW_API AMCSurfaceRover : public AActor
{
	GENERATED_BODY()
public:
	AMCSurfaceRover();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Look) TSoftObjectPtr<UStaticMesh> MeshAsset;
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Look) float MeshScale = 1.f;
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Look) float YawOffset = 0.f;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> Arm;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;

protected:
	UFUNCTION() void OnRep_Look();
	void ApplyLook();
};
