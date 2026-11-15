#include "Return/MCReturnView.h"
#include "Return/MCReturnManager.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

AMCReturnView::AMCReturnView()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	ShipRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ShipRoot"));
	ShipRoot->SetupAttachment(Root);
	CapsuleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CapsuleMesh"));
	CapsuleMesh->SetupAttachment(ShipRoot);
	CapsuleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Space has no ambient light: a soft fill light travelling with the capsule keeps its dark side readable.
	UPointLightComponent* Fill = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	Fill->SetupAttachment(ShipRoot);
	Fill->SetRelativeLocation(FVector(-600.f, 250.f, 350.f));
	Fill->SetIntensity(60000.f);
	Fill->SetAttenuationRadius(2500.f);
	Fill->SetLightColor(FLinearColor(0.85f, 0.92f, 1.f));
	Fill->SetCastShadows(false);
	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(Root);
	ChaseCamera->SetRelativeLocation(FVector(-900.f, 0.f, 250.f));
	ChaseCamera->SetRelativeRotation(FRotator(-12.f, 0.f, 0.f));
}

void AMCReturnView::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Mgr) Mgr = AMCReturnManager::Find(this);
	if (!Mgr) return;
	const float Z = FMath::Lerp(4000.f, 0.f, Mgr->Progress);
	const FVector Target(Mgr->Progress * 6000.f, Mgr->ShipCrossKm * CmPerKm, Z + Mgr->ShipAngleErr * CmPerDeg);
	ShipRoot->SetRelativeLocation(FMath::VInterpTo(ShipRoot->GetRelativeLocation(), Target, Dt, 3.f));
	ChaseCamera->SetRelativeLocation(ShipRoot->GetRelativeLocation() + FVector(-900.f, 0.f, 250.f));
#if ENABLE_DRAW_DEBUG
	const FTransform T = GetActorTransform();
	const float Half = CmPerDeg;
	for (int32 s = -1; s <= 1; s += 2)
		DrawDebugLine(GetWorld(), T.TransformPosition(FVector(0, 0, 4000.f + s * Half)), T.TransformPosition(FVector(6000, 0, s * Half)), FColor::Cyan, false, -1.f, 0, 4.f);
	if (Mgr->bBlackout) DrawDebugSphere(GetWorld(), ShipRoot->GetComponentLocation(), 150.f, 12, FColor::Orange, false, -1.f, 0, 3.f);
#endif
}
