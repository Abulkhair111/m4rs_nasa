// Helpers for the transit UI and flow (agent S2c). All static, usable from C++ and Blueprints.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MCStationLibrary.generated.h"

class AMCBayCharacter;
class APlayerState;
class AController;

UCLASS()
class MISSIONCREW_API UMCStationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The AMCTransitManager in the level (typed AActor so this file does not depend on it). Null if none. */
	UFUNCTION(BlueprintPure, Category = "Transit", meta = (WorldContext = "WorldContext")) static AActor* FindTransitManager(UObject* WorldContext);
	/** CrewRole (1..4, 0 = unknown) of the local player. */
	UFUNCTION(BlueprintPure, Category = "Transit", meta = (WorldContext = "WorldContext")) static int32 GetLocalRole(UObject* WorldContext);
	/** The local player's station pawn (null if the pawn is not an AMCBayCharacter). */
	UFUNCTION(BlueprintPure, Category = "Transit", meta = (WorldContext = "WorldContext")) static AMCBayCharacter* GetLocalStationPawn(UObject* WorldContext);
	/** Manager bFreeRoles (read by reflection). False if there is no manager. */
	UFUNCTION(BlueprintPure, Category = "Transit", meta = (WorldContext = "WorldContext")) static bool AreRolesFree(UObject* WorldContext);
	/** Button entry point: calls ServerTransitAction(Code) on the local pawn. Returns false if there is no station pawn. */
	UFUNCTION(BlueprintCallable, Category = "Transit", meta = (WorldContext = "WorldContext")) static bool SendTransitAction(UObject* WorldContext, int32 Code);
	/** SERVER: seat every player at the station of its own role (call after the S1 LAUNCH). Returns how many were seated. */
	UFUNCTION(BlueprintCallable, Category = "Transit", meta = (WorldContext = "WorldContext")) static int32 SeatAllPlayers(UObject* WorldContext);
	/** SERVER: stand every player up. */
	UFUNCTION(BlueprintCallable, Category = "Transit", meta = (WorldContext = "WorldContext")) static void StandAllPlayers(UObject* WorldContext);

	/** Server: AMCTransitManager::HandleAction(Code, Who = sender CrewRole). */
	static bool CallHandleAction(AActor* Manager, int32 Code, int32 Who);
};
