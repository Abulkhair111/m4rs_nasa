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
class AMCConstructionManager;
class UMCPartPickerWidget;
class UMCBayHudWidget;

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

	// ---- Config ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bay") TObjectPtr<UCameraComponent> Camera;
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

	// ---- State ----
	UPROPERTY(BlueprintReadOnly, Category = "Bay") bool bCursorMode = false;
	UPROPERTY(BlueprintReadOnly, Category = "Bay") TObjectPtr<AMCPartSocket> AimedSocket;
	UPROPERTY(BlueprintReadOnly, Category = "Bay") TObjectPtr<AActor> AimedActor;
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
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerRelease();
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerRemovePart(AMCPartSocket* Socket);
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerSetHeldTool(EMCToolType Tool);
	/** Swap the installed part of a socket for another one (remove + place; the old part is restored if the new one is refused). */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerSwapPart(AMCPartSocket* Socket, UMCPartDef* Def);
	/** Choose the launcher (index into the manager's Rockets). Drives SelectedLauncher, so every stack changes. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Bay") void ServerSelectLauncher(int32 LeverIndex);

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
	void OnRemovePressed();
	void OnCursorPressed();

	UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> LookAction;
	UPROPERTY() TObjectPtr<UInputAction> JumpAction;
	UPROPERTY() TObjectPtr<UInputAction> InteractAction;
	UPROPERTY() TObjectPtr<UInputAction> RemoveAction;
	UPROPERTY() TObjectPtr<UInputAction> CursorAction;

	UPROPERTY() TObjectPtr<UMCPartPickerWidget> Picker;
	UPROPERTY() TObjectPtr<UMCBayHudWidget> Hud;
	UPROPERTY() mutable TObjectPtr<AMCConstructionManager> CachedManager;
};
