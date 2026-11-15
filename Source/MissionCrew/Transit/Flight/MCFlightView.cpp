#include "Transit/Flight/MCFlightView.h"
#include "Transit/MCTransitManager.h"
#include "Bay/MCRocketStack.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	FVector Bezier2(const FVector& A, const FVector& B, const FVector& C, float T)
	{
		const float U = 1.f - T;
		return A * (U * U) + B * (2.f * U * T) + C * (T * T);
	}
	FVector Bezier2D(const FVector& A, const FVector& B, const FVector& C, float T)
	{
		return ((B - A) * (1.f - T) + (C - B) * T) * 2.f;
	}
	FVector Bezier3(const FVector& A, const FVector& B, const FVector& C, const FVector& D, float T)
	{
		const float U = 1.f - T;
		return A * (U * U * U) + B * (3.f * U * U * T) + C * (3.f * U * T * T) + D * (T * T * T);
	}
	FVector Bezier3D(const FVector& A, const FVector& B, const FVector& C, const FVector& D, float T)
	{
		const float U = 1.f - T;
		return ((B - A) * (U * U) + (C - B) * (2.f * U * T) + (D - C) * (T * T)) * 3.f;
	}
	// Visual weight of the miss offset along the coast: a little at the start, full at the Moon.
	float MissWeight(float P) { return 0.15f + 0.85f * FMath::Clamp(P, 0.f, 1.f); }
}

AMCFlightView::AMCFlightView()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false; // purely local visuals, driven by the replicated manager

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);

	auto MakeMesh = [this](const TCHAR* Name, USceneComponent* Parent) {
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetupAttachment(Parent);
		C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		return C;
	};
	SkyMesh = MakeMesh(TEXT("SkyMesh"), Root);
	SkyMesh->SetCastShadow(false);
	EarthMesh = MakeMesh(TEXT("EarthMesh"), Root);
	MoonMesh = MakeMesh(TEXT("MoonMesh"), Root);

	ShipRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ShipRoot"));
	ShipRoot->SetupAttachment(Root);
	ShipRoot->SetMobility(EComponentMobility::Movable);
	RocketMesh = MakeMesh(TEXT("RocketMesh"), ShipRoot);
	ShipMesh = MakeMesh(TEXT("ShipMesh"), ShipRoot);
	AscentMesh = MakeMesh(TEXT("AscentMesh"), ShipRoot);
	GhostMesh = MakeMesh(TEXT("GhostMesh"), Root);
	GhostMesh->SetVisibility(false);
	GhostMesh->SetCastShadow(false);

	FlareLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlareLight"));
	FlareLight->SetupAttachment(ShipRoot);
	FlareLight->SetMobility(EComponentMobility::Movable);
	FlareLight->SetRelativeLocation(FVector(-300.f, 0.f, 0.f));
	FlareLight->SetLightColor(FLinearColor(1.f, 0.6f, 0.25f));
	FlareLight->SetAttenuationRadius(7000.f);
	FlareLight->SetCastShadows(false);
	FlareLight->SetIntensity(0.f);

	// Space has no ambient light: a soft fill light travels with the ship so its shaded side stays readable.
	UPointLightComponent* Fill = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	Fill->SetupAttachment(ShipRoot);
	Fill->SetMobility(EComponentMobility::Movable);
	Fill->SetRelativeLocation(FVector(-700.f, 350.f, 450.f));
	Fill->SetIntensity(70000.f);
	Fill->SetAttenuationRadius(3500.f);
	Fill->SetLightColor(FLinearColor(0.85f, 0.92f, 1.f));
	Fill->SetCastShadows(false);

	FuelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("FuelText"));
	FuelText->SetupAttachment(Root);
	FuelText->SetMobility(EComponentMobility::Movable);
	FuelText->SetHorizontalAlignment(EHTA_Center);
	FuelText->SetWorldSize(140.f);
	FuelText->SetTextRenderColor(FColor(64, 224, 255));
	FuelText->SetText(FText::FromString(TEXT("")));

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(Root);
	ChaseCamera->SetMobility(EComponentMobility::Movable);
	ChaseCamera->bUsePawnControlRotation = false;
	ChaseCamera->SetFieldOfView(70.f);

	ViewscreenCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("ViewscreenCapture"));
	ViewscreenCapture->SetupAttachment(Root);
	ViewscreenCapture->bCaptureEveryFrame = false;
	ViewscreenCapture->bCaptureOnMovement = false;
	ViewscreenCapture->bAlwaysPersistRenderingState = true;
	ViewscreenCapture->CaptureSource = SCS_FinalColorLDR;
	ViewscreenCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	ViewscreenCapture->SetAutoActivate(false);
}

AMCFlightView* AMCFlightView::Find(const UObject* WorldContextObject)
{
	UWorld* W = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!W) return nullptr;
	for (TActorIterator<AMCFlightView> It(W); It; ++It) { return *It; }
	return nullptr;
}

// ------------------------------------------------------------------ setup

void AMCFlightView::FitToRadius(UStaticMeshComponent* Comp, float Radius) const
{
	if (!Comp || !Comp->GetStaticMesh()) return;
	const float Ext = FMath::Max(1.f, Comp->GetStaticMesh()->GetBounds().BoxExtent.GetMax());
	Comp->SetRelativeScale3D(FVector(Radius / Ext));
}

void AMCFlightView::ApplyAssets()
{
	auto Load = [](UStaticMeshComponent* C, const TSoftObjectPtr<UStaticMesh>& A) {
		if (C && !C->GetStaticMesh() && !A.IsNull())
		{
			if (UStaticMesh* M = A.LoadSynchronous()) C->SetStaticMesh(M);
		}
	};
	Load(EarthMesh, EarthMeshAsset);
	Load(MoonMesh, MoonMeshAsset);
	Load(SkyMesh, SkyMeshAsset);
	Load(RocketMesh, RocketMeshAsset);
	Load(ShipMesh, ShipMeshAsset);
	Load(AscentMesh, AscentMeshAsset);
	Load(GhostMesh, GhostMeshAsset);

	RocketMesh->SetRelativeRotation(RocketMeshRotation);
	RocketMesh->SetRelativeScale3D(RocketMeshScale);
	ShipMesh->SetRelativeRotation(ShipMeshRotation);
	ShipMesh->SetRelativeScale3D(ShipMeshScale);
	AscentMesh->SetRelativeRotation(AscentMeshRotation);
	AscentMesh->SetRelativeScale3D(AscentMeshScale);
}

void AMCFlightView::ApplyLayout()
{
	MoonCentre = FVector(EarthMoonDistance, 0.f, MoonHeight);
	EarthMesh->SetRelativeLocation(FVector::ZeroVector);
	MoonMesh->SetRelativeLocation(MoonCentre);
	if (bAutoScaleBodies)
	{
		FitToRadius(EarthMesh, EarthRadius);
		FitToRadius(MoonMesh, MoonRadius);
		FitToRadius(SkyMesh, SkyRadius);
	}
	RebuildPath();
}

void AMCFlightView::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyAssets();
	ApplyLayout();
	// Editor preview: park the ship on the pad.
	ShipRoot->SetRelativeLocation(Pad);
}

void AMCFlightView::RebuildPath()
{
	Pad = FVector(0.f, 0.f, EarthRadius + 30.f);
	PointA = Pad + FVector(0.6f * AscentLift, 0.f, AscentLift);
	PointE = MoonCentre + FVector(-(MoonRadius + ApproachGap), 0.f, 0.f);

	float Bow = 0.f;
	if (PathSeed > 0)
	{
		FRandomStream S(PathSeed);
		Bow = S.FRandRange(-1.f, 1.f) * ArcBow;
	}
	const float K = 0.35f * (PointE.X - PointA.X);
	CoastP1 = PointA + FVector(K, Bow, ArcHeight);
	CoastP2 = PointE + FVector(-K, Bow * 0.5f, ArcHeight * 0.5f);
	// Ascent control point chosen so that the tangent at the end equals the coast start tangent (no kink at the burn-out).
	AscP1 = PointA - (CoastP1 - PointA).GetSafeNormal() * (0.5f * AscentLift);
}

FVector AMCFlightView::PathPoint(float P) const { return Bezier3(PointA, CoastP1, CoastP2, PointE, FMath::Clamp(P, 0.f, 1.f)); }
FVector AMCFlightView::PathTangent(float P) const { return Bezier3D(PointA, CoastP1, CoastP2, PointE, FMath::Clamp(P, 0.f, 1.f)).GetSafeNormal(); }
FVector AMCFlightView::AscentPoint(float U) const { return Bezier2(Pad, AscP1, PointA, FMath::Clamp(U, 0.f, 1.f)); }
FVector AMCFlightView::AscentTangent(float U) const { return Bezier2D(Pad, AscP1, PointA, FMath::Clamp(U, 0.f, 1.f)).GetSafeNormal(); }
FVector AMCFlightView::MissOffset(float X, float Y, float P) const { return FVector(0.f, X * MissScale, Y * MissScale) * MissWeight(P); }

void AMCFlightView::BeginPlay()
{
	Super::BeginPlay();
	ApplyAssets();

	// Default ship look from the S1 lander (the stack visual in the bay) when nothing was assigned.
	UWorld* StackWorld = GetWorld();
	if (!ShipMesh->GetStaticMesh() && StackWorld)
	{
		for (TActorIterator<AMCRocketStack> It(StackWorld); It; ++It)
		{
			if (UStaticMesh* M = It->LanderMesh.LoadSynchronous())
			{
				ShipMesh->SetStaticMesh(M);
				ShipMesh->SetRelativeScale3D(It->LanderScale);
				ShipMesh->SetRelativeRotation(It->LanderRotation);
				break;
			}
		}
	}
	ApplyLayout();

	if (GetNetMode() == NM_DedicatedServer) { SetActorTickEnabled(false); return; }

	ViewscreenCapture->FOVAngle = ChaseCamera->FieldOfView;
	if (bViewscreenEnabled && ViewscreenTarget)
	{
		ViewscreenCapture->TextureTarget = ViewscreenTarget;
		ViewscreenCapture->ShowOnlyActors.Reset();
		ViewscreenCapture->ShowOnlyActors.Add(this);
		if (AActor* Sky = SkyActor.Get()) ViewscreenCapture->ShowOnlyActors.Add(Sky);
		ViewscreenCapture->SetActive(true);
		ApplyViewscreen();
	}
}

// ------------------------------------------------------------------ viewscreen

UMaterialInstanceDynamic* AMCFlightView::CreateScreenMaterial(UMeshComponent* ScreenMesh, int32 Slot)
{
	if (!ScreenMesh) return nullptr;
	UMaterialInterface* Base = ScreenMesh->GetMaterial(Slot);
	if (!Base) return nullptr;
	UMaterialInstanceDynamic* MID = ScreenMesh->CreateDynamicMaterialInstance(Slot, Base);
	if (MID && ViewscreenTarget) MID->SetTextureParameterValue(ScreenTextureParam, ViewscreenTarget);
	if (MID) ScreenMIDs.Add(MID);
	return MID;
}

void AMCFlightView::ApplyViewscreen()
{
	if (bViewscreenApplied || !GetWorld()) return;
	bViewscreenApplied = true;
	static const FName ScreenTag(TEXT("FlightScreen"));
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		if (A == this) continue;
		TArray<UStaticMeshComponent*> Meshes;
		A->GetComponents<UStaticMeshComponent>(Meshes);
		bool bAny = false;
		for (UStaticMeshComponent* M : Meshes)
		{
			if (M->ComponentHasTag(ScreenTag)) { CreateScreenMaterial(M, ScreenMaterialSlot); bAny = true; }
		}
		if (!bAny && ScreenActors.Contains(A) && Meshes.Num() > 0) { CreateScreenMaterial(Meshes[0], ScreenMaterialSlot); }
	}
}

void AMCFlightView::UpdateViewscreen(float Dt)
{
	if (!bViewscreenEnabled || !ViewscreenTarget || !ViewscreenCapture->IsActive()) return;
	CaptureAcc += Dt;
	if (CaptureAcc < 1.f / FMath::Max(1.f, ViewscreenFps)) return;
	CaptureAcc = 0.f;
	ViewscreenCapture->SetWorldLocationAndRotation(CamLoc, CamRot);
	ViewscreenCapture->CaptureScene();
}

FVector AMCFlightView::GetShipWorldLocation() const
{
	return GetActorTransform().TransformPosition(ShipLocal);
}

// ------------------------------------------------------------------ tick

void AMCFlightView::FindManager(float Dt)
{
	if (Manager) return;
	FindTimer -= Dt;
	if (FindTimer > 0.f) return;
	FindTimer = 0.5f;
	for (TActorIterator<AMCTransitManager> It(GetWorld()); It; ++It) { Manager = *It; break; }
}

void AMCFlightView::TriggerPulse(int32 Dir)
{
	FlareTimer = 0.5f;
	PuffTimer = 0.6f;
	PuffDir = Dir;
	BP_OnPulse(Dir, GetShipWorldLocation());
}

void AMCFlightView::SetShipVisibility(bool bAscentPhase, bool bCoast)
{
	const bool bHaveRocket = RocketMesh->GetStaticMesh() != nullptr;
	RocketMesh->SetVisibility(bHaveRocket && !bCoast);
	ShipMesh->SetVisibility(!bHaveRocket || bCoast);
	if (!bCoast)
	{
		// Without a dedicated pad rocket mesh the lander + ascent stage fly together.
		AscentMesh->SetVisibility(!bHaveRocket && AscentMesh->GetStaticMesh() != nullptr);
		AscentMesh->SetRelativeLocation(FVector::ZeroVector);
	}
}

void AMCFlightView::Tick(float Dt)
{
	Super::Tick(Dt);
	if (GetNetMode() == NM_DedicatedServer) return;
	Dt = FMath::Min(Dt, 0.2f);
	FindManager(Dt);

	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;

	// ---- read the replicated state ----
	EMCTransitStage Stage = EMCTransitStage::GoPoll;
	float ErrX = 0.f, ErrY = 0.f, DispX = 0.f, DispY = 0.f, TransitDur = 180.f;
	bool bStale = false;
	if (Manager)
	{
		Stage = Manager->Stage;
		ErrX = Manager->ShipErrX; ErrY = Manager->ShipErrY;
		DispX = Manager->DispX; DispY = Manager->DispY;
		bStale = Manager->bDisplayStale;
		TransitDur = FMath::Max(1.f, Manager->TransitDurationSec);
		if (Manager->Seed != PathSeed && Manager->Seed > 0)
		{
			PathSeed = Manager->Seed;
			RebuildPath();
			Trail.Reset();
		}
		if (LastPulseSeq < 0) { LastPulseSeq = Manager->PulseSeq; }
		else if (Manager->PulseSeq != LastPulseSeq)
		{
			LastPulseSeq = Manager->PulseSeq;
			TriggerPulse(Manager->LastPulseDir);
		}
	}
	if (static_cast<uint8>(Stage) != PrevStage)
	{
		PrevStage = static_cast<uint8>(Stage);
		StageSeenWorldTime = Now;
		if (Stage == EMCTransitStage::GoPoll || Stage == EMCTransitStage::Countdown) { Trail.Reset(); CoastSecEst = 0.f; LastElapsed = -1.f; }
	}

	// ---- progress ----
	const bool bCoast = Stage == EMCTransitStage::Coast || Stage == EMCTransitStage::Arrival;
	float TimeLeft = Manager ? Manager->GetStageTimeLeft() : 0.f;
	if (Stage == EMCTransitStage::Coast)
	{
		const float E = Manager->ElapsedSec;
		if (E != LastElapsed) { LastElapsed = E; LastElapsedWorldTime = Now; }
		const float Target = E + FMath::Clamp(Now - LastElapsedWorldTime, 0.f, 0.35f);
		const float Err = Target - CoastSecEst;
		CoastSecEst = (FMath::Abs(Err) > 1.f) ? Target : CoastSecEst + Err * FMath::Min(1.f, Dt * 6.f);
		Progress = FMath::Clamp(CoastSecEst / TransitDur, 0.f, 1.f);
	}
	else if (Stage == EMCTransitStage::Arrival) { Progress = 1.f; CoastSecEst = TransitDur; }
	else { Progress = 0.f; }

	// ---- smoothed miss offsets (km, already weighted by the progress) ----
	const float W = bCoast ? MissWeight(Progress) : 0.f;
	const float A = FMath::Min(1.f, Dt * OffsetInterpSpeed);
	OffX = FMath::Lerp(OffX, ErrX * W, A);
	OffY = FMath::Lerp(OffY, ErrY * W, A);
	GhostX = FMath::Lerp(GhostX, DispX * W, A);
	GhostY = FMath::Lerp(GhostY, DispY * W, A);

	// ---- ship placement ----
	FVector Base = Pad;
	FVector Tan = AscentTangent(0.f);
	if (Stage == EMCTransitStage::Ascent)
	{
		const float U = FMath::Clamp(1.f - TimeLeft / FMath::Max(1.f, AscentSec), 0.f, 1.f);
		const float Ue = FMath::Pow(U, 1.7f); // accelerating climb
		Base = AscentPoint(Ue);
		Tan = AscentTangent(Ue);
	}
	else if (bCoast)
	{
		Base = PathPoint(Progress);
		Tan = PathTangent(Progress);
	}
	ShipLocal = Base + FVector(0.f, OffX * MissScale, OffY * MissScale);
	ShipTan = Tan.IsNearlyZero() ? FVector::ForwardVector : Tan;
	ShipRoot->SetRelativeLocationAndRotation(ShipLocal, FRotationMatrix::MakeFromX(ShipTan).Rotator());

	SetShipVisibility(Stage == EMCTransitStage::Ascent, bCoast);

	// Stage separation: the ascent stage drifts back and disappears.
	if (bCoast && AscentMesh->GetStaticMesh())
	{
		const float Sep = CoastSecEst;
		const bool bShow = Sep < 14.f;
		AscentMesh->SetVisibility(bShow);
		AscentMesh->SetRelativeLocation(FVector(-FMath::Min(Sep, 14.f) * 140.f, 0.f, 0.f));
	}

	// ---- engine flare ----
	FlareTimer = FMath::Max(0.f, FlareTimer - Dt);
	PuffTimer = FMath::Max(0.f, PuffTimer - Dt);
	float Flare = PulseFlareIntensity * (FlareTimer / 0.5f);
	if (Stage == EMCTransitStage::Ascent) { Flare = FMath::Max(Flare, AscentFlareIntensity * (0.8f + 0.2f * FMath::Sin(Now * 37.f))); }
	else if (Stage == EMCTransitStage::Countdown && TimeLeft < 3.f && TimeLeft > 0.f)
	{
		Flare = FMath::Max(Flare, AscentFlareIntensity * 0.35f * (1.f - TimeLeft / 3.f) * (0.7f + 0.3f * FMath::Sin(Now * 41.f)));
	}
	FlareLight->SetIntensity(Flare);
	FlareLight->SetVisibility(Flare > 1.f);
	if (PulseScalePop > 0.f && FlareTimer > 0.f)
	{
		ShipMesh->SetRelativeScale3D(ShipMeshScale * (1.f + PulseScalePop * (FlareTimer / 0.5f)));
	}
	else { ShipMesh->SetRelativeScale3D(ShipMeshScale); }

	// ---- ghost marker (stale display) ----
	const FVector GhostLocal = (bCoast ? PathPoint(Progress) : Pad) + FVector(0.f, GhostX * MissScale, GhostY * MissScale);
	const bool bShowGhost = bStale && bCoast;
	GhostMesh->SetVisibility(bShowGhost && GhostMesh->GetStaticMesh() != nullptr);
	GhostMesh->SetRelativeLocation(GhostLocal);

	// ---- trail ----
	if ((Stage == EMCTransitStage::Ascent || Stage == EMCTransitStage::Coast) && (Trail.Num() == 0 || FVector::DistSquared(Trail.Last(), ShipLocal) > 120.f * 120.f))
	{
		Trail.Add(ShipLocal);
		if (Trail.Num() > MaxTrailPoints) Trail.RemoveAt(0);
	}

	UpdateCamera(Dt);
	UpdateFuelText();
	if (bDrawPath) DrawScene(Dt);
	UpdateViewscreen(Dt);
}

// ------------------------------------------------------------------ camera

void AMCFlightView::UpdateCamera(float Dt)
{
	const FTransform T = GetActorTransform();
	FVector Flat(ShipTan.X, ShipTan.Y, 0.f);
	Flat = Flat.SizeSquared() < 0.04f ? FVector::ForwardVector : Flat.GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Flat);
	const FVector WantLocal = ShipLocal - Flat * ChaseDistance + FVector::UpVector * ChaseHeight + Right * ChaseSide;
	const FVector WantLoc = T.TransformPosition(WantLocal);
	const FVector LookAt = T.TransformPosition(ShipLocal + ShipTan * LookAhead);
	const FRotator WantRot = (LookAt - WantLoc).Rotation();
	if (!bCamInit)
	{
		CamLoc = WantLoc; CamRot = WantRot; bCamInit = true;
	}
	else
	{
		CamLoc = FMath::VInterpTo(CamLoc, WantLoc, Dt, ChaseLagSpeed);
		CamRot = FMath::RInterpTo(CamRot, (LookAt - CamLoc).Rotation(), Dt, ChaseRotLagSpeed);
	}
	ChaseCamera->SetWorldLocationAndRotation(CamLoc, CamRot);

	if (SkyMesh->GetStaticMesh()) SkyMesh->SetWorldLocation(CamLoc);
	if (AActor* Sky = SkyActor.Get()) Sky->SetActorLocation(CamLoc);
}

// ------------------------------------------------------------------ in-world readout

void AMCFlightView::UpdateFuelText()
{
	TextAcc += GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f; // text rebuild throttle (visual only)

	FVector TextLoc = GetActorTransform().TransformPosition(ShipLocal + FVector(0.f, 0.f, FuelTextHeight));
	FuelText->SetWorldLocation(TextLoc);
	FVector Cam = CamLoc;
	if (const APlayerCameraManager* PCM = UGameplayStatics::GetPlayerCameraManager(this, 0)) { Cam = PCM->GetCameraLocation(); }
	FRotator Rot = (TextLoc - Cam).Rotation(); // text faces -X, so +X points away from the viewer
	if (bFlipText) Rot.Yaw += 180.f;
	FuelText->SetWorldRotation(Rot);

	if (TextAcc < 0.2f) return;
	TextAcc = 0.f;
	FString S;
	FColor C(64, 224, 255);
	if (!Manager || !Manager->bStarted) { S = TEXT("AWAITING LAUNCH"); }
	else
	{
		S = FString::Printf(TEXT("FUEL %.0f kg"), Manager->FuelKg);
		if (Manager->Stage == EMCTransitStage::Coast || Manager->Stage == EMCTransitStage::Arrival)
		{
			const float Miss = FMath::Sqrt(Manager->DispX * Manager->DispX + Manager->DispY * Manager->DispY);
			S += FString::Printf(TEXT("\nMISS %.1f km"), Miss);
			if (Miss > Manager->CorridorKm) C = FColor(255, 190, 50);
			if (Manager->bDisplayStale) { S += TEXT("\nDISPLAY STALE"); C = FColor(255, 80, 80); }
		}
		else if (Manager->Stage == EMCTransitStage::Countdown) { S += FString::Printf(TEXT("\nT-%.0f"), FMath::CeilToFloat(Manager->GetStageTimeLeft())); }
	}
	FuelText->SetTextRenderColor(C);
	FuelText->SetText(FText::FromString(S));
}

// ------------------------------------------------------------------ debug-drawn path, rings, trail, puffs

void AMCFlightView::DrawScene(float Dt)
{
	UWorld* World = GetWorld();
	if (!World) return;
	const FTransform T = GetActorTransform();
	auto Wp = [&](const FVector& L) { return T.TransformPosition(L); };
	const FVector AxY = T.TransformVectorNoScale(FVector::RightVector);
	const FVector AxZ = T.TransformVectorNoScale(FVector::UpVector);
	const float Th = LineThickness;

	const float CorridorKm = Manager ? Manager->CorridorKm : 15.f;
	const bool bCoast = Manager && (Manager->Stage == EMCTransitStage::Coast || Manager->Stage == EMCTransitStage::Arrival);

	// ideal path (ascent + coast)
	const FColor Ideal(60, 90, 150);
	FVector Prev = Pad;
	for (int32 i = 1; i <= 12; ++i) { const FVector P = AscentPoint(i / 12.f); DrawDebugLine(World, Wp(Prev), Wp(P), Ideal, false, -1.f, 0, Th * 0.5f); Prev = P; }
	for (int32 i = 1; i <= 40; ++i) { const FVector P = PathPoint(i / 40.f); DrawDebugLine(World, Wp(Prev), Wp(P), Ideal, false, -1.f, 0, Th * 0.5f); Prev = P; }

	// flown trail
	for (int32 i = 1; i < Trail.Num(); ++i) { DrawDebugLine(World, Wp(Trail[i - 1]), Wp(Trail[i]), FColor(64, 224, 255), false, -1.f, 0, Th); }

	// corridor ring at the Moon
	const float CorridorR = CorridorKm * MissScale;
	DrawDebugCircle(World, Wp(PointE), CorridorR, 48, FColor(80, 230, 120), false, -1.f, 0, Th, AxY, AxZ, false);
	DrawDebugLine(World, Wp(PointE - FVector(0, CorridorR * 0.15f, 0)), Wp(PointE + FVector(0, CorridorR * 0.15f, 0)), FColor(80, 230, 120), false, -1.f, 0, Th * 0.5f);
	DrawDebugLine(World, Wp(PointE - FVector(0, 0, CorridorR * 0.15f)), Wp(PointE + FVector(0, 0, CorridorR * 0.15f)), FColor(80, 230, 120), false, -1.f, 0, Th * 0.5f);

	// TCM windows: rings along the ideal path
	if (Manager)
	{
		const float Pulse = 0.5f + 0.5f * FMath::Sin(World->GetTimeSeconds() * 6.f);
		for (int32 i = 0; i < Manager->Windows.Num(); ++i)
		{
			const FMCTransitWindow& Win = Manager->Windows[i];
			if (Win.OpenSec < 0.f) continue;
			const float P = Win.OpenSec / FMath::Max(1.f, Manager->TransitDurationSec);
			const FVector C = PathPoint(P);
			const FMatrix Rm =FRotationMatrix::MakeFromX(T.TransformVectorNoScale(PathTangent(P)));
			FColor Col(255, 190, 50);
			float R = WindowRingRadius;
			float Thick = Th;
			if (Win.bOpen) { Col = FColor(80, 255, 120); R *= 1.f + 0.25f * Pulse; Thick = Th * 2.f; }
			else if (Win.bDone) { Col = FColor(90, 90, 100); }
			else if (i == MCTransit::FaultWindowIndex) { Col = FColor(255, 70, 70); }
			DrawDebugCircle(World, Wp(C), R, 32, Col, false, -1.f, 0, Thick, Rm.GetUnitAxis(EAxis::Y), Rm.GetUnitAxis(EAxis::Z), false);
			DrawDebugString(World, Wp(C) + AxZ * (R + 120.f), Win.Label, nullptr, Col, 0.f, true, 1.5f);
		}
	}

	// predicted end point of the TRUE course, and the stale ghost course
	if (bCoast && Manager)
	{
		const float TrueMiss = FMath::Sqrt(Manager->ShipErrX * Manager->ShipErrX + Manager->ShipErrY * Manager->ShipErrY);
		const FColor Pred = TrueMiss <= CorridorKm ? FColor(80, 230, 120) : FColor(255, 190, 50);
		const FVector EndTrue = PointE + FVector(0.f, Manager->ShipErrX * MissScale, Manager->ShipErrY * MissScale);
		DrawDebugLine(World, Wp(ShipLocal), Wp(EndTrue), Pred, false, -1.f, 0, Th);
		DrawDebugSphere(World, Wp(EndTrue), 120.f, 8, Pred, false, -1.f, 0, Th * 0.5f);

		if (Manager->bDisplayStale)
		{
			const FColor Ghost(255, 90, 255);
			const FVector GhostLocal = PathPoint(Progress) + FVector(0.f, GhostX * MissScale, GhostY * MissScale);
			const FVector EndGhost = PointE + FVector(0.f, Manager->DispX * MissScale, Manager->DispY * MissScale);
			DrawDebugLine(World, Wp(GhostLocal), Wp(EndGhost), Ghost, false, -1.f, 0, Th * 0.5f);
			DrawDebugSphere(World, Wp(EndGhost), 100.f, 8, Ghost, false, -1.f, 0, Th * 0.5f);
			if (!GhostMesh->GetStaticMesh()) { DrawDebugSphere(World, Wp(GhostLocal), 150.f, 10, Ghost, false, -1.f, 0, Th * 0.5f); }
		}
	}

	// placeholder marker if no ship mesh is assigned yet
	if (!ShipMesh->GetStaticMesh() && !RocketMesh->GetStaticMesh())
	{
		DrawDebugSphere(World, Wp(ShipLocal), 200.f, 12, FColor(255, 255, 255), false, -1.f, 0, Th * 0.5f);
		DrawDebugLine(World, Wp(ShipLocal), Wp(ShipLocal + ShipTan * 600.f), FColor(255, 255, 255), false, -1.f, 0, Th);
	}

	// RCS puffs: opposite to the push direction (Left/Right/Up/Down = -Y/+Y/+Z/-Z in the ship frame)
	if (PuffTimer > 0.f)
	{
		FVector Push = FVector::ZeroVector;
		switch (PuffDir)
		{
		case 0: Push = FVector(0, -1, 0); break;
		case 1: Push = FVector(0, 1, 0); break;
		case 2: Push = FVector(0, 0, 1); break;
		default: Push = FVector(0, 0, -1); break;
		}
		const float K = PuffTimer / 0.6f;
		const FVector Origin = ShipLocal;
		for (int32 s = -1; s <= 1; ++s)
		{
			FVector Spread = Push.Y != 0.f ? FVector(0, 0, 1) : FVector(0, 1, 0);
			const FVector Dir = (-Push + Spread * (0.25f * s)).GetSafeNormal();
			const FVector From = Origin + Dir * 120.f;
			const FVector To = From + Dir * (350.f * K);
			DrawDebugLine(World, Wp(From), Wp(To), FColor(255, 255, 255), false, -1.f, 0, Th * 0.6f);
			DrawDebugPoint(World, Wp(To), 14.f * K + 4.f, FColor(200, 230, 255), false, -1.f, 0);
		}
	}
}
