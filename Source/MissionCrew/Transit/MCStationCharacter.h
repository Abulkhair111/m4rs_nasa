// Transit player (agent S2c): AMCBayCharacter + station use.
//
// Walk up to the station of your own role, aim at it, press E: the pawn is moved to the seat, movement is locked,
// the view blends to the station camera, the transit HUD is shown and the cursor is on. E or Esc stands up.
// Another role's station is refused unless the transit manager has bFreeRoles (CrewRole 0 = unassigned = allowed).
//
// Replicated entry point for UI buttons:  ServerTransitAction(int32 Code) -> AMCTransitManager::HandleAction.
// Use this class as the default pawn (e.g. in BP_MC_BayGameMode) or as parent of the bay pawn Blueprint.
#pragma once

#include "CoreMinimal.h"
#include "Bay/MCBayCharacter.h"
#include "MCStationCharacter.generated.h"

class AMCStation;
class UUserWidget;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCStationCharacter : public AMCBayCharacter
{
	GENERATED_BODY()

public:
	AMCStationCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void UnPossessed() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Station the player sits at (null = standing). Replicated. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Seat, Category = "Station") TObjectPtr<AMCStation> SeatedStation;
	/** Station currently aimed at (local). */
	UPROPERTY(BlueprintReadOnly, Category = "Station") TObjectPtr<AMCStation> AimedStation;
	/** Optional Blueprint child of the transit HUD widget. Null = /Script/MissionCrew.MCTransitHudWidget (agent S2b) if it exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station|UI") TSubclassOf<UUserWidget> TransitHudClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") float ViewBlendTime = 0.5f;
	/** Cone (cos of half angle) used to aim at a station. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") float StationAimCos = 0.8f;
	/** If true, ServerTransitAction is ignored while the player is not seated (unless the manager has bFreeRoles). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") bool bRequireSeat = true;

	UFUNCTION(BlueprintPure, Category = "Station") bool IsSeated() const { return SeatedStation != nullptr; }

	// ---- Replicated entry points ----
	/** Client -> server: sit at a station (validated: free, and own role unless bFreeRoles). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Station") void ServerSitAtStation(AMCStation* Station);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Station") void ServerStandUp();
	/** THE transit button entry point: client -> server, forwarded to AMCTransitManager::HandleAction(Code, PlayerState). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Transit") void ServerTransitAction(int32 Code);

	/** SERVER ONLY: seat this player at the station of its own role (teleport + lock). Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Station") bool SeatAtOwnStation();
	/** SERVER ONLY: seat at a given station (role check unless bIgnoreRole). */
	UFUNCTION(BlueprintCallable, Category = "Station") bool SeatAt(AMCStation* Station, bool bIgnoreRole = false);
	/** SERVER ONLY. */
	UFUNCTION(BlueprintCallable, Category = "Station") void StandUp();

	UFUNCTION() void OnRep_Seat(AMCStation* Previous);

protected:
	void BuildStationInput();
	void AddStationMapping();
	void OnStationInteract();
	void OnStationEscape();
	void UpdateStationAim();
	void ApplySeatState(AMCStation* Previous);
	void ShowTransitHud(bool bShow);

	UPROPERTY() TObjectPtr<UInputMappingContext> StationMapping;
	UPROPERTY() TObjectPtr<UInputAction> StationUseAction;
	UPROPERTY() TObjectPtr<UInputAction> StationEscAction;
	UPROPERTY() TObjectPtr<UUserWidget> TransitHud;
	bool bStationRefused = false;
};
