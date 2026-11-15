// Kept for compatibility. Since agent X all station behaviour (sit / stand, camera blend, transit HUD, cursor mode,
// SeatedStation, ServerTransitAction) lives on AMCBayCharacter, so a Blueprint child of the bay pawn (BP_MC_BayPlayer)
// works at the stations. This empty subclass only exists so old references/Blueprints keep working.
#pragma once

#include "CoreMinimal.h"
#include "Bay/MCBayCharacter.h"
#include "MCStationCharacter.generated.h"

UCLASS(Blueprintable)
class MISSIONCREW_API AMCStationCharacter : public AMCBayCharacter
{
	GENERATED_BODY()
};
