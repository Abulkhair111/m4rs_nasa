#include "Samples/MCSamplesRelay.h"
#include "Samples/MCSamplesManager.h"
#include "Transit/MCTransitManager.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"

AMCSamplesRelay::AMCSamplesRelay()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	SetReplicatingMovement(false);
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* R = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(R);
}

void AMCSamplesRelay::ServerAction_Implementation(int32 Code)
{
	const int32 Rl = AMCTransitManager::GetCrewRoleOfController(Cast<AController>(GetOwner()));
	for (TActorIterator<AMCSamplesManager> It(GetWorld()); It; ++It) { It->HandleAction(Code, Rl); break; }
}
