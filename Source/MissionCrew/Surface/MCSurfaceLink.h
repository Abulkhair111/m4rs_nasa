// S4: per-player RPC channel. The manager is not owned by a client, so it spawns one link per PlayerController (owner = that controller,
// relevant only to its owner). The owning client's link also creates the role HUD and points the camera at the rover.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCSurfaceLink.generated.h"

class AMCSurfaceManager;
class UMCSurfaceHudWidget;

UCLASS()
class MISSIONCREW_API AMCSurfaceLink : public AActor
{
	GENERATED_BODY()
public:
	AMCSurfaceLink();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Code = role*100+action. The role is read on the server from the sender's PlayerState. */
	UFUNCTION(Server, Reliable) void ServerAction(int32 Code);
	/** Rover Driver: throttle and steer in -1..1. */
	UFUNCTION(Server, Reliable) void ServerDrive(float Throttle, float Steer);

	AMCSurfaceManager* GetManager();
	UPROPERTY(EditAnywhere, Category = "Surface") TSubclassOf<UMCSurfaceHudWidget> HudClass;

protected:
	UPROPERTY(Transient) TObjectPtr<AMCSurfaceManager> Manager;
	UPROPERTY(Transient) TObjectPtr<UMCSurfaceHudWidget> Hud;
	bool bViewSet = false;
};
