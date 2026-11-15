#include "Shelf/MCPartShelfDisplay.h"
#include "Shelf/MCShelfPart.h"
#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"

#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"

AMCPartShelfDisplay::AMCPartShelfDisplay()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false; // level-placed; every machine spawns its own identical items

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	TitleText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Title"));
	TitleText->SetupAttachment(Root);
	TitleText->SetHorizontalAlignment(EHTA_Center);
	TitleText->SetWorldSize(22.f);
	TitleText->SetTextRenderColor(FColor::White);
	TitleText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TitleText->SetRelativeLocation(FVector(0.f, 0.f, 240.f));
}

void AMCPartShelfDisplay::BeginPlay()
{
	Super::BeginPlay();
	if (TitleText)
	{
		TitleText->SetText(FText::FromString(DepartmentLabel));
		TitleText->SetVisibility(!DepartmentLabel.IsEmpty());
		TitleText->SetRelativeLocation(FVector(0.f, 0.f, TitleHeight));
		TitleText->SetRelativeRotation(FRotator(0.f, FacingYaw, 0.f));
	}
	Rebuild();
}

void AMCPartShelfDisplay::EndPlay(const EEndPlayReason::Type Reason)
{
	Clear();
	Super::EndPlay(Reason);
}

void AMCPartShelfDisplay::Clear()
{
	for (AMCShelfPart* P : Spawned)
	{
		if (IsValid(P)) P->Destroy();
	}
	Spawned.Reset();
}

void AMCPartShelfDisplay::Rebuild()
{
	Clear();
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<UMCPartDef*> Defs;
	if (bFillFromCatalog)
	{
		if (const AMCConstructionManager* M = MCBayUI::FindManager(World))
		{
			for (UMCPartDef* D : M->Catalog)
			{
				if (D && !D->IsTool() && (Department <= 0 || D->Department == Department)) Defs.Add(D);
			}
		}
	}
	else
	{
		for (const TSoftObjectPtr<UMCPartDef>& Soft : Parts)
		{
			UMCPartDef* D = Soft.LoadSynchronous();
			if (D && !D->IsTool()) Defs.Add(D);
		}
	}
	if (Defs.Num() == 0) return;

	const int32 NumRows = FMath::Max(1, Rows);
	const int32 PerRow = FMath::DivideAndRoundUp(Defs.Num(), NumRows);
	const FRotator Rot = GetActorRotation() + FRotator(0.f, FacingYaw, 0.f);

	for (int32 i = 0; i < Defs.Num(); ++i)
	{
		const int32 Row = i / PerRow;
		const int32 Col = i % PerRow;
		const int32 InThisRow = FMath::Min(PerRow, Defs.Num() - Row * PerRow);
		const float Y = bCenterRow ? (Col - (InThisRow - 1) * 0.5f) * Spacing : Col * Spacing;
		const FVector Local(-Row * RowSpacing, Y, PartZ);
		const FVector WorldLoc = GetActorTransform().TransformPosition(Local);

		AMCShelfPart* P = World->SpawnActorDeferred<AMCShelfPart>(AMCShelfPart::StaticClass(), FTransform(Rot, WorldLoc), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!P) continue;
		P->Def = Defs[i];
		P->FitMaxCm = MaxPartSizeCm;
		P->bShowNameLabel = bShowNameLabels;
		P->FinishSpawning(FTransform(Rot, WorldLoc));
		Spawned.Add(P);
	}
}
