// Per-player RPC carrier for S3 (the manager is not owned by a client). Spawned by AMCOrbitManager, owned by the PlayerController.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCOrbitRelay.generated.h"

UCLASS()
class MISSIONCREW_API AMCOrbitRelay : public AActor
{
	GENERATED_BODY()
public:
	AMCOrbitRelay();
	/** Client -> server: forwarded to AMCOrbitManager::HandleAction(Code, CrewRole of the owner). */
	UFUNCTION(Server, Reliable) void ServerAction(int32 Code);
};
