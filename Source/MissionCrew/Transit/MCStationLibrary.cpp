#include "Transit/MCStationLibrary.h"
#include "Bay/MCBayCharacter.h"
#include "Transit/MCStation.h"
#include "Transit/MCTransitManager.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UObject/UnrealType.h"


AActor* UMCStationLibrary::FindTransitManager(UObject* WorldContext)
{
	UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!W) return nullptr;
	for (TActorIterator<AMCTransitManager> It(W); It; ++It) return *It;
	return nullptr;
}

int32 UMCStationLibrary::GetLocalRole(UObject* WorldContext)
{
	UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	if (!PC) return 0;
	if (AMCBayCharacter* C = Cast<AMCBayCharacter>(PC->GetPawn())) return C->GetCrewRole();
	if (const APlayerState* PS = PC->PlayerState)
	{
		if (const FProperty* P = PS->GetClass()->FindPropertyByName(TEXT("CrewRole")))
		{
			const void* A = P->ContainerPtrToValuePtr<void>(PS);
			if (const FIntProperty* IP = CastField<FIntProperty>(P)) return IP->GetPropertyValue(A);
			if (const FByteProperty* BP = CastField<FByteProperty>(P)) return BP->GetPropertyValue(A);
		}
	}
	return 0;
}

AMCBayCharacter* UMCStationLibrary::GetLocalStationPawn(UObject* WorldContext)
{
	UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	return PC ? Cast<AMCBayCharacter>(PC->GetPawn()) : nullptr;
}

bool UMCStationLibrary::AreRolesFree(UObject* WorldContext)
{
	AActor* M = FindTransitManager(WorldContext);
	if (!M) return false;
	if (const FBoolProperty* P = CastField<FBoolProperty>(M->GetClass()->FindPropertyByName(TEXT("bFreeRoles"))))
	{
		return P->GetPropertyValue_InContainer(M);
	}
	return false;
}

bool UMCStationLibrary::SendTransitAction(UObject* WorldContext, int32 Code)
{
	if (AMCBayCharacter* C = GetLocalStationPawn(WorldContext))
	{
		C->ServerTransitAction(Code);
		return true;
	}
	return false;
}

int32 UMCStationLibrary::SeatAllPlayers(UObject* WorldContext)
{
	UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!W || W->GetNetMode() == NM_Client) return 0;
	int32 N = 0;
	for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (AMCBayCharacter* C = PC ? Cast<AMCBayCharacter>(PC->GetPawn()) : nullptr)
		{
			if (C->SeatAtOwnStation()) ++N;
		}
	}
	return N;
}

void UMCStationLibrary::StandAllPlayers(UObject* WorldContext)
{
	UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!W || W->GetNetMode() == NM_Client) return;
	for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (AMCBayCharacter* C = PC ? Cast<AMCBayCharacter>(PC->GetPawn()) : nullptr) C->StandUp();
	}
}

bool UMCStationLibrary::CallHandleAction(AActor* Manager, int32 Code, int32 Who)
{
	AMCTransitManager* M = Cast<AMCTransitManager>(Manager);
	return M ? M->HandleAction(Code, Who) : false;
}
