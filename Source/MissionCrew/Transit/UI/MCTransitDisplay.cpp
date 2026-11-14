#include "Transit/UI/MCTransitDisplay.h"
#include "Transit/UI/MCTransitPanelBase.h"
#include "Transit/MCTransitManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Engine/World.h"

AMCTransitDisplay::AMCTransitDisplay()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	EarthMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EarthMesh"));
	EarthMesh->SetupAttachment(Root);
	EarthMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MoonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MoonMesh"));
	MoonMesh->SetupAttachment(Root);
	MoonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShipMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMarker"));
	ShipMarker->SetupAttachment(Root);
	ShipMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CountdownText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CountdownText"));
	CountdownText->SetupAttachment(Root);
	CountdownText->SetHorizontalAlignment(EHTA_Center);
	CountdownText->SetWorldSize(24.f);
	CountdownText->SetTextRenderColor(FColor(64, 224, 255));
	CountdownText->SetText(FText::FromString(TEXT("S2 TRANSIT")));
	CountdownText->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	CountdownText->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
}

void AMCTransitDisplay::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Manager)
	{
		FindTimer -= DeltaSeconds;
		if (FindTimer <= 0.f)
		{
			FindTimer = 0.5f;
			for (TActorIterator<AMCTransitManager> It(GetWorld()); It; ++It) { Manager = *It; break; }
		}
	}

	const FVector EarthL(-EarthMoonDistance * 0.5f, 0.f, 0.f);
	const FVector MoonL(EarthMoonDistance * 0.5f, 0.f, 0.f);
	EarthMesh->SetRelativeLocation(EarthL);
	MoonMesh->SetRelativeLocation(MoonL);

	const FTransform T = GetActorTransform();
	auto W = [&](const FVector& L) { return T.TransformPosition(L); };

	// predicted impact point: Moon surface point offset by the displayed miss (Y = DispX, Z = DispY)
	float MissX = 0.f, MissY = 0.f, Corridor = 15.f, Progress = 0.f;
	EMCTransitStage Stage = EMCTransitStage::GoPoll;
	if (Manager)
	{
		MissX = Manager->DispX; MissY = Manager->DispY; Corridor = Manager->CorridorKm; Stage = Manager->Stage;
		if (Manager->Stage == EMCTransitStage::Coast)
			Progress = Manager->TransitDurationSec > 0.f ? FMath::Clamp(Manager->ElapsedSec / Manager->TransitDurationSec, 0.f, 1.f) : 0.f;
		else if (Manager->Stage == EMCTransitStage::Arrival) Progress = 1.f;
	}
	const FVector Start = EarthL + FVector(EarthRadius, 0.f, 0.f);
	const FVector Target = MoonL + FVector(-MoonRadius, MissX * MissScale, MissY * MissScale);
	FVector Ship = FMath::Lerp(Start, Target, Progress);
	if (Stage == EMCTransitStage::Ascent) Ship = Start + FVector(0.f, 0.f, AscentLift);
	ShipMarker->SetRelativeLocation(Ship);
	const FVector Dir = (Target - Ship).GetSafeNormal();
	if (!Dir.IsNearlyZero()) ShipMarker->SetRelativeRotation(Dir.Rotation());

	if (bDrawPath && GetWorld())
	{
		const bool bInside = FMath::Sqrt(MissX * MissX + MissY * MissY) <= Corridor;
		const FColor Pred = bInside ? FColor(80, 230, 120) : FColor(255, 190, 50);
		DrawDebugLine(GetWorld(), W(Start), W(Ship), FColor(64, 224, 255), false, -1.f, 0, LineThickness);
		DrawDebugLine(GetWorld(), W(Ship), W(Target), Pred, false, -1.f, 0, LineThickness);
		DrawDebugSphere(GetWorld(), W(Target), 3.f, 8, Pred, false, -1.f, 0, LineThickness);
		// corridor circle at the Moon (plane perpendicular to the Earth-Moon axis)
		const FVector CircleCentre = W(MoonL + FVector(-MoonRadius, 0.f, 0.f));
		const FVector Y = T.TransformVectorNoScale(FVector::RightVector);
		const FVector Z = T.TransformVectorNoScale(FVector::UpVector);
		DrawDebugCircle(GetWorld(), CircleCentre, Corridor * MissScale * T.GetScale3D().X, 32, FColor(80, 230, 120), false, -1.f, 0, LineThickness, Y, Z, false);
	}
	UpdateText();
}

void AMCTransitDisplay::UpdateText()
{
	if (!CountdownText) return;
	FString S;
	if (!Manager || !Manager->bStarted) S = TEXT("AWAITING LAUNCH");
	else
	{
		S = MCTransitUI::StageName(Manager->Stage);
		switch (Manager->Stage)
		{
		case EMCTransitStage::Countdown: S += FString::Printf(TEXT("\nT-%.0f"), FMath::CeilToFloat(FMath::Max(0.f, Manager->GetStageTimeLeft()))); break;
		case EMCTransitStage::Ascent:    S += FString::Printf(TEXT("\n%.0f s"), FMath::Max(0.f, Manager->GetStageTimeLeft())); break;
		case EMCTransitStage::Coast:     S += FString::Printf(TEXT("\nT+%.0f / %.0f s\nMISS %.0f km"), Manager->ElapsedSec, Manager->TransitDurationSec, FMath::Sqrt(Manager->DispX * Manager->DispX + Manager->DispY * Manager->DispY)); break;
		case EMCTransitStage::Arrival:   S += FString::Printf(TEXT("\nSCORE %d"), Manager->TransitScore); break;
		default: break;
		}
	}
	CountdownText->SetText(FText::FromString(S));
}
