#include "Orbit/MCOrbitRelay.h"
#include "Orbit/MCOrbitManager.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"

AMCOrbitRelay::AMCOrbitRelay()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	bAlwaysRelevant = false;
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AMCOrbitRelay::ServerAction_Implementation(int32 Code)
{
	const AController* C = Cast<AController>(GetOwner());
	if (!C || !GetWorld()) return;
	for (TActorIterator<AMCOrbitManager> It(GetWorld()); It; ++It)
	{
		It->HandleAction(Code, AMCOrbitManager::GetCrewRoleOfController(C));
		break;
	}
}
