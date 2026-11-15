// Per-player RPC carrier for S5. The manager is not owned by a client, so it cannot receive Server RPCs; the server spawns one
// relay per PlayerController (Owner = PC) and the panel calls ServerAction on its own relay. No pawn/controller changes needed.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCSamplesRelay.generated.h"

class AMCSamplesManager;

UCLASS()
class MISSIONCREW_API AMCSamplesRelay : public AActor
{
	GENERATED_BODY()
public:
	AMCSamplesRelay();
	/** client -> server: forwarded to AMCSamplesManager::HandleAction(Code, CrewRole of the owner). */
	UFUNCTION(Server, Reliable) void ServerAction(int32 Code);
};
