// First-person bay player (agent F). Works with no editor assets: input actions / mapping context are created
// at runtime (Enhanced Input), the HUD and part picker fall back to code-built layouts.
//
// Controls: WASD move, mouse look, Space jump, E interact (aimed empty socket -> part picker; with a tool held
// and an installed part -> bolt/wire), R remove the aimed unbolted part, Tab toggles cursor mode
// (mouse cursor shown, Game-and-UI input, look/move paused).
//
// Server RPCs live on the pawn because the manager is not owned by any client: ServerPlacePart,
// ServerInteract, ServerRelease, ServerRemovePart, ServerSetHeldTool.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Construction/MCConstructionTypes.h"
#include "MCBayCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class AMCPartSocket;
class AMCInteractable;
class AMCConstructionManager;
class UMCPartPickerWidget;
class UMCBayHudWidget;
class UMCBayMenuWidget;
class UMCBodyAnimComponent;
class AMCStation;
class AMCShelfPart;
class UUserWidget;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCBayCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMCBayCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void UnPossessed() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// ---- Config ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bay") TObjectPtr<UCameraComponent> Camera;
	/** Single-node body animation / procedural fallback (agent W). Assign sequences in the Blueprint child. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bay") TObjectPtr<UMCBodyAnimComponent> BodyAnim;
	/** Line trace length for non-socket targets (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bay") float InteractRange = 350.f;
	/** Max distance from the eye to a socket that can be aimed at (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bay") float SocketReach = 350.f;
	/** cos of the half-angle of the aim cone used to pick sockets (they have no collision). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bay") float AimConeCos = 0.95f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bay") float LookSensitivity = 1.f;
	/** Optional Blueprint children of the widgets (WBP reparented to the C++ classes). Null = code fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bay|UI") TSubclassOf<UMCPartPickerWidget> PickerClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bay|UI") TSubclassOf<UMCBayHudWidget> HudClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bay|UI") bool bAutoCreateHud = true;
	/** Optional Blueprint child of the pause menu. Null = code fallback. Esc or P toggles it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bay|UI") TSubclassOf<UMCBayMenuWidget> MenuClass;

	// ---- State ----
	UPROPERTY(BlueprintReadOnly, Category = "Bay") bool bCursorMode = false;
	UPROPERTY(BlueprintReadOnly, Category = "Bay") TObjectPtr<AMCPartSocket> AimedSocket;
	UPROPERTY(BlueprintReadOnly, Category = "Bay") TObjectPtr<AActor> AimedActor;
	/** Aimed at a world interactable (button, lever, pump, tool stand). Also stored in AimedActor. */
	UPROPERTY(BlueprintReadOnly, Category = "Bay") TObjectPtr<AMCInteractable> AimedInteractable;
	/** AimedSocket came from aiming at a drawn piece of a rocket stack (real or wall display), not from the socket cone. */
	UPROPERTY(BlueprintReadOnly, Category = "Bay") bool bAimedViaStack = false;
	/** Aiming at launcher-preset hardware (core stage / main boosters): E cycles to the next launcher. */
	UPROPERTY(BlueprintReadOnly, Category = "Bay") bool bAimedLauncherPart = false;
	/** Range for aiming at rocket stack pieces (capped by each stack's AimRangeCm), cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bay") float StackAimRange = 3000.f;
	/** Look-at prompt text for the HUD ("[E] Choose part ...", installed part name + numbers). */
	UPROPERTY(BlueprintReadOnly, Category = "Bay") FText AimPrompt;
	/** Tool in hand (wrench = bolt, cable = grid). Set with ServerSetHeldTool (e.g. from a shelf Blueprint). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Bay") EMCToolType HeldTool = EMCToolType::None;

	/** Part picked up from a shelf (E on an AMCShelfPart). Local to this player. E on a compatible empty socket installs it. */
	UPROPERTY(BlueprintReadOnly, Category = "Bay|Shelf") TObjectPtr<UMCPartDef> SelectedPart;
	/** The shelf item SelectedPart came from (flight start point, pickup effect). */
	UPROPERTY() TWeakObjectPtr<AMCShelfPart> SelectedFrom;

	/** Select a part for installation (local). From may be null. */
	UFUNCTION(BlueprintCallable, Category = "Bay|Shelf") void SelectPart(UMCPartDef* Def, AMCShelfPart* From);
	UFUNCTION(BlueprintCallable, Category = "Bay|Shelf") void ClearSelection(bool bShowNote = true);
	/** Short message under the aim prompt on the HUD (local). */
	UFUNCTION(BlueprintCallable, Category = "Bay|Shelf") void ShowNotice(const FString& Text, float Seconds = 4.f);

	// ---- API ----
	/** CrewRole number of this player (0 = unassigned, everything allowed). Reads PlayerState "CrewRole" by reflection. */
	UFUNCTION(BlueprintPure, Category = "Bay") int32 GetCrewRole() const;
	UFUNCTION(BlueprintPure, Category = "Bay") AMCConstructionManager* GetManager() const;
	UFUNCTION(BlueprintCallable, Category = "Bay") void SetCursorMode(bool bEnable);
	UFUNCTION(BlueprintCallable, Category = "Bay") void ToggleCursorMode();
	UFUNCTION(BlueprintCallable, Category = "Bay") void OpenPicker(AMCPartSocket* Socket);
	void OnPickerClosed(UMCPartPickerWidget* Picker);

	// ---- Server RPCs (no more 'set Run on Server by hand') ----
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerPlacePart(AMCPartSocket* Socket, UMCPartDef* Def);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerInteract(AActor* Target);
	/** Like ServerPlacePart, but the part flies in from From (the shelf item) when bHasFrom. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerPlacePartFrom(AMCPartSocket* Socket, UMCPartDef* Def, FVector From, bool bHasFrom);
	/** Drop the held tool (G). Also ends any hold interaction. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerRelease();
	/** E released: ends a hold interaction (fuel pump). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerEndInteract();
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerRemovePart(AMCPartSocket* Socket);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerSetHeldTool(EMCToolType Tool);
	/** Swap the installed part of a socket for another one (remove + place; the old part is restored if the new one is refused). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerSwapPart(AMCPartSocket* Socket, UMCPartDef* Def);
	/** Choose the launcher (index into the manager's Rockets). Drives SelectedLauncher, so every stack changes. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerSelectLauncher(int32 LeverIndex);

	// ---- Stations (flight deck; moved here from AMCStationCharacter so the Blueprint bay pawn can sit and send console actions) ----
	/** Station the player sits at (null = standing). Replicated. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Seat, Category = "Station") TObjectPtr<AMCStation> SeatedStation;
	/** Station currently aimed at (local). Only set when nothing else (interactable / socket / stack) is aimed. */
	UPROPERTY(BlueprintReadOnly, Category = "Station") TObjectPtr<AMCStation> AimedStation;
	/** Optional Blueprint child of the transit HUD widget. Null = /Script/MissionCrew.MCTransitHudWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station|UI") TSubclassOf<UUserWidget> TransitHudClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") float ViewBlendTime = 0.5f;
	/** Cone (cos of half angle) used to aim at a station. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") float StationAimCos = 0.8f;
	/** If true, ServerTransitAction is ignored while the player is not seated (unless the manager has bFreeRoles). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") bool bRequireSeat = true;

	UFUNCTION(BlueprintPure, Category = "Station") bool IsSeated() const { return SeatedStation != nullptr; }
	/** Client -> server: sit at a station (validated: free, and own role unless bFreeRoles). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Station") void ServerSitAtStation(AMCStation* Station);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Station") void ServerStandUp();
	/** THE transit button entry point: client -> server, forwarded to AMCTransitManager::HandleAction(Code, CrewRole). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Transit") void ServerTransitAction(int32 Code);
	/** SERVER ONLY: seat this player at the station of its own role (teleport + lock). Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Station") bool SeatAtOwnStation();
	/** SERVER ONLY: seat at a given station (role check unless bIgnoreRole). */
	UFUNCTION(BlueprintCallable, Category = "Station") bool SeatAt(AMCStation* Station, bool bIgnoreRole = false);
	/** SERVER ONLY. */
	UFUNCTION(BlueprintCallable, Category = "Station") void StandUp();
	UFUNCTION() void OnRep_Seat(AMCStation* Previous);

	/** Seated view: true = 3D flight chase camera (AMCFlightView), false = station camera. The Pilot starts with the flight view. Key V toggles. */
	UPROPERTY(BlueprintReadOnly, Category = "Station") bool bFlightViewActive = false;
	/** Local: toggle between the station camera and the flight chase camera while seated (needs an AMCFlightView in the level). */
	UFUNCTION(BlueprintCallable, Category = "Station") void ToggleFlightView();

	/** Server: a non-socket actor was interacted with (shelf, button, lever...). Handle it in a Blueprint child. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Bay") void BP_OnInteract(AActor* Target);
	/** Server: Release pressed (drop whatever is held). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Bay") void BP_OnRelease();

protected:
	void BuildInput();
	void AddMappingContext();
	void EnsureHud();
	void UpdateAim();

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnInteractPressed();
	void OnInteractReleased();
	void OnDropPressed();
	void OnRemovePressed();
	void OnCursorPressed();
	void OnMenuPressed();

	// stations
	void BuildStationInput();
	void AddStationMapping();
	void OnStationInteract();
	void OnStationEscape();
	void OnStationViewToggle();
	UPROPERTY() TObjectPtr<UInputAction> StationViewAction;
	void UpdateStationAim();
	void ApplySeatState(AMCStation* Previous);
	void ShowTransitHud(bool bShow);
	UPROPERTY() TObjectPtr<UInputMappingContext> StationMapping;
	UPROPERTY() TObjectPtr<UInputAction> StationUseAction;
	UPROPERTY() TObjectPtr<UInputAction> StationEscAction;
	UPROPERTY() TObjectPtr<UUserWidget> TransitHud;
	bool bStationRefused = false;

	UPROPERTY() TObjectPtr<UInputAction> MenuAction;
	UPROPERTY() TObjectPtr<UMCBayMenuWidget> Menu;
	UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> LookAction;
	UPROPERTY() TObjectPtr<UInputAction> JumpAction;
	UPROPERTY() TObjectPtr<UInputAction> InteractAction;
	UPROPERTY() TObjectPtr<UInputAction> RemoveAction;
	UPROPERTY() TObjectPtr<UInputAction> CursorAction;
	UPROPERTY() TObjectPtr<UInputAction> DropAction;
	/** Server: interactable currently held with E. Client: E is down on an interactable. */
	UPROPERTY() TObjectPtr<AMCInteractable> ActiveInteractable;
	bool bHoldingInteract = false;
	void OnShelfPartPressed(AMCShelfPart* Part);
	void PlaceSelected(AMCPartSocket* Socket);
	FString NoticeText;
	double NoticeUntil = 0.0;

	UPROPERTY() TObjectPtr<UMCPartPickerWidget> Picker;
	UPROPERTY() TObjectPtr<UMCBayHudWidget> Hud;
	UPROPERTY() mutable TObjectPtr<AMCConstructionManager> CachedManager;
};
