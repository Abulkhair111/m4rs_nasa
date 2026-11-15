#include "Orbit/MCOrbitView.h"
#include "Orbit/MCOrbitManager.h"
#include "Bay/MCRocketStack.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"

AMCOrbitView::AMCOrbitView()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	MoonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MoonMesh"));
	MoonMesh->SetupAttachment(Root);
	MoonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MoonMesh->SetCastShadow(false);
	ShipRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ShipRoot"));
	ShipRoot->SetupAttachment(Root);
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	ShipMesh->SetupAttachment(ShipRoot);
	ShipMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlareLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlareLight"));
	FlareLight->SetupAttachment(ShipRoot);
	FlareLight->SetRelativeLocation(FVector(0.f, 0.f, -150.f));
	FlareLight->SetIntensity(0.f);
	FlareLight->SetLightColor(FLinearColor(1.f, 0.7f, 0.4f));
	FlareLight->SetCastShadows(false);
	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(Root);
	MoonMeshAsset = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	MoonMaterialAsset = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/MissionCrew/Art/Meshes/Space/MI_Moon.MI_Moon")));
}

void AMCOrbitView::ApplyAssets()
{
	if (!MoonMesh->GetStaticMesh() && !MoonMeshAsset.IsNull())
		if (UStaticMesh* M = MoonMeshAsset.LoadSynchronous()) MoonMesh->SetStaticMesh(M);
	if (!MoonMaterialAsset.IsNull())
		if (UMaterialInterface* Mat = MoonMaterialAsset.LoadSynchronous()) MoonMesh->SetMaterial(0, Mat);
	if (!ShipMesh->GetStaticMesh() && !ShipMeshAsset.IsNull())
		if (UStaticMesh* M = ShipMeshAsset.LoadSynchronous()) ShipMesh->SetStaticMesh(M);
	// Fit the engine sphere (100 cm diameter) to MoonRadius, centre below the site.
	float Unit = 50.f;
	if (const UStaticMesh* M = MoonMesh->GetStaticMesh()) Unit = FMath::Max(1.f, M->GetBounds().SphereRadius);
	MoonMesh->SetRelativeScale3D(FVector(MoonRadius / Unit));
	MoonMesh->SetRelativeLocation(FVector(0.f, 0.f, -MoonRadius));
	ShipMesh->SetRelativeRotation(ShipMeshRotation);
	ShipMesh->SetRelativeScale3D(ShipMeshScale);
}

void AMCOrbitView::OnConstruction(const FTransform& T)
{
	Super::OnConstruction(T);
	ApplyAssets();
	ShipRoot->SetRelativeLocation(FVector(0.f, 0.f, OrbitAltitude));
}

void AMCOrbitView::BeginPlay()
{
	Super::BeginPlay();
	ApplyAssets();
	if (!ShipMesh->GetStaticMesh() && GetWorld())
		for (TActorIterator<AMCRocketStack> It(GetWorld()); It; ++It)
			if (UStaticMesh* M = It->LanderMesh.LoadSynchronous())
			{
				ShipMesh->SetStaticMesh(M); ShipMesh->SetRelativeScale3D(It->LanderScale); ShipMesh->SetRelativeRotation(It->LanderRotation);
				break;
			}
	if (GetNetMode() == NM_DedicatedServer) SetActorTickEnabled(false);
}

FVector AMCOrbitView::GetShipWorldLocation() const { return ShipRoot->GetComponentLocation(); }

FVector AMCOrbitView::OrbitPoint(float A) const
{
	const float R = MoonRadius + OrbitAltitude;
	return FVector(FMath::Sin(A) * R, 0.f, FMath::Cos(A) * R - MoonRadius);
}

void AMCOrbitView::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!Manager)
	{
		FindTimer -= Dt;
		if (FindTimer <= 0.f && GetWorld())
		{
			FindTimer = 0.5f;
			for (TActorIterator<AMCOrbitManager> It(GetWorld()); It; ++It) { Manager = *It; break; }
		}
	}
	if (bAutoViewTarget && !bViewSet && GetWorld())
	{
		ViewTimer += Dt;
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		if (ViewTimer >= ViewTargetDelaySec && PC && PC->IsLocalController()) { PC->SetViewTargetWithBlend(this, 0.5f); bViewSet = true; }
	}
	if (!Manager || !Manager->bStarted) { return; }

	FVector Want; FRotator WantRot = FRotator::ZeroRotator;
	const bool bDescent = Manager->Stage == EMCOrbitStage::Descent || Manager->Stage == EMCOrbitStage::Report;
	float Throttle = 0.f;
	if (bDescent)
	{
		Want = FVector(Manager->PosX, Manager->PosY, Manager->AltM) * 100.f;
		Throttle = Manager->Throttle;
		WantRot = FRotator(0.f, 0.f, 0.f);
		WantRot.Pitch = FMath::Clamp(-Manager->VelX * 3.f, -20.f, 20.f);
		WantRot.Roll = FMath::Clamp(Manager->VelY * 3.f, -20.f, 20.f);
	}
	else
	{
		const float Ang = FMath::Max(0.f, StartAngle - Manager->ElapsedSec * OrbitRate);
		Want = OrbitPoint(Ang);
		const FVector Tan = (OrbitPoint(Ang - 0.01f) - OrbitPoint(Ang)).GetSafeNormal();
		WantRot = Tan.Rotation();
	}
	if (!bInit) { ShipLocal = Want; ShipRot = WantRot; bInit = true; }
	ShipLocal = FMath::VInterpTo(ShipLocal, Want, Dt, FollowSpeed);
	ShipRot = FMath::RInterpTo(ShipRot, WantRot, Dt, 4.f);
	ShipRoot->SetRelativeLocation(ShipLocal);
	ShipRoot->SetRelativeRotation(ShipRot);
	FlareLight->SetIntensity(Throttle * FlareIntensity);

	if (bDescent && Manager->bTouchedDown && !bTouchFired) { bTouchFired = true; BP_OnTouchdown(GetActorTransform().TransformPosition(ShipLocal)); }

	// camera: behind and above the ship
	const FVector Focus = ShipLocal;
	const FVector Behind = bDescent ? FVector(-ChaseDistance, 0.f, ChaseHeight) : FVector(0.f, -ChaseDistance, ChaseHeight * 0.5f);
	const FVector DesLoc = Focus + (bDescent ? Behind : ShipRot.RotateVector(FVector(-ChaseDistance, 0.f, ChaseHeight)));
	const FRotator DesRot = (Focus - DesLoc).Rotation();
	if (!bCamInit) { CamLoc = DesLoc; CamRot = DesRot; bCamInit = true; }
	CamLoc = FMath::VInterpTo(CamLoc, DesLoc, Dt, 3.f);
	CamRot = FMath::RInterpTo(CamRot, DesRot, Dt, 4.f);
	ChaseCamera->SetRelativeLocationAndRotation(CamLoc, CamRot);

#if !UE_BUILD_SHIPPING
	if (bDrawMarkers && GetWorld())
	{
		const FTransform T = GetActorTransform();
		auto Ring = [&](const FVector2D& P, float RadiusM, const FColor& C) {
			DrawDebugCircle(GetWorld(), T.TransformPosition(FVector(P.X * 100.f, P.Y * 100.f, 20.f)), RadiusM * 100.f, 32, C, false, -1.f, 0, 40.f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		};
		if (bDescent && Manager->ChosenSite >= 0)
		{
			Ring(FVector2D(Manager->TargetX, Manager->TargetY), Manager->SafeRadiusM, FColor::Green);
			if (Manager->bBoulderField) Ring(FVector2D(0.f, 0.f), Manager->BoulderRadiusM, FColor::Red);
			for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i)
			{
				const int32 I = Manager->ChosenSite * MCOrbit::HazardsPerSite + i;
				if (Manager->Hazards.IsValidIndex(I) && Manager->Hazards[I].bMarked)
					Ring(FVector2D(Manager->Hazards[I].X, Manager->Hazards[I].Y), Manager->Hazards[I].Radius, FColor::Orange);
			}
		}
	}
#endif
}
