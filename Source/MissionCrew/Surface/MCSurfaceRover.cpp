#include "Surface/MCSurfaceRover.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"

AMCSurfaceRover::AMCSurfaceRover()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.f);
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	Arm->SetupAttachment(RootComponent);
	Arm->TargetArmLength = 1100.f;
	Arm->SetRelativeRotation(FRotator(-28.f, 0.f, 0.f));
	Arm->bDoCollisionTest = false;
	Arm->bEnableCameraLag = true;
	Arm->CameraLagSpeed = 6.f;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Arm);
}

void AMCSurfaceRover::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCSurfaceRover, MeshAsset);
	DOREPLIFETIME(AMCSurfaceRover, MeshScale);
	DOREPLIFETIME(AMCSurfaceRover, YawOffset);
}

void AMCSurfaceRover::BeginPlay() { Super::BeginPlay(); ApplyLook(); }
void AMCSurfaceRover::OnRep_Look() { ApplyLook(); }

void AMCSurfaceRover::ApplyLook()
{
	if (UStaticMesh* M = MeshAsset.LoadSynchronous()) { Mesh->SetStaticMesh(M); }
	Mesh->SetRelativeScale3D(FVector(MeshScale));
	Mesh->SetRelativeRotation(FRotator(0.f, YawOffset, 0.f));
}
