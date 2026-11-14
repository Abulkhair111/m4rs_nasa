#include "Transit/MCStation.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

AMCStation::AMCStation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetBoxExtent(FVector(120.f, 120.f, 100.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // aimable with the bay's visibility trace
	Trigger->SetGenerateOverlapEvents(false);

	SeatPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SeatPoint"));
	SeatPoint->SetupAttachment(Root);
	SeatPoint->SetRelativeLocation(FVector(-80.f, 0.f, 0.f));

	StationCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("StationCamera"));
	StationCamera->SetupAttachment(Root);
	StationCamera->SetRelativeLocation(FVector(-90.f, 0.f, 150.f));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 230.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(28.f);
	Label->SetText(FText::FromString(TEXT("Station")));
}

void AMCStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCStation, Occupant);
}

FString AMCStation::RoleName(int32 InRole)
{
	switch (InRole)
	{
	case 1: return TEXT("Systems Operator");
	case 2: return TEXT("Equipment Technician");
	case 3: return TEXT("Pilot");
	case 4: return TEXT("Science Operator");
	default: return TEXT("Crew");
	}
}

void AMCStation::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Label)
	{
		const FString S = StationName.IsEmpty() ? RoleName(CrewRole) : StationName.ToString();
		Label->SetText(FText::FromString(S));
	}
}

bool AMCStation::CanBeUsedBy(int32 PlayerRole, bool bFreeRoles) const
{
	return bFreeRoles || PlayerRole == CrewRole;
}

AMCStation* AMCStation::FindForRole(UObject* WorldContext, int32 InRole)
{
	UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!W) return nullptr;
	for (TActorIterator<AMCStation> It(W); It; ++It)
	{
		if (It->CrewRole == InRole) return *It;
	}
	return nullptr;
}
