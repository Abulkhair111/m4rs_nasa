// C++ replacements of the old Blueprint bay interactables (BP_BayInteractable, BP_ReadyButton, BP_LaunchButton,
// BP_FuelPump, BP_RocketLever and the tool shelf). Agent I1.
//
// All of them are plain level actors: place them, set the properties, no Blueprint needed. Use / Release run on the
// SERVER (the pawn's ServerInteract / ServerEndInteract RPCs call them); the hover prompt is computed locally on every
// machine from replicated manager state. Nothing here builds geometry: every actor has an invisible box for the E trace,
// an optional Mesh slot for the team's model and an engine text label.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Construction/MCConstructionTypes.h"
#include "MCInteractable.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class AMCBayCharacter;
class AMCConstructionManager;

/** Base: Prompt text, department rule, server Use / Release, collision for the E trace (Visibility channel). */
UCLASS(Blueprintable, Abstract)
class MISSIONCREW_API AMCInteractable : public AActor
{
	GENERATED_BODY()

public:
	AMCInteractable();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact") TObjectPtr<USceneComponent> Root;
	/** Invisible trace volume (blocks Visibility only). Resize it to cover your model. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact") TObjectPtr<UBoxComponent> TraceBox;
	/** Assign the team's model here (left empty on purpose). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact") TObjectPtr<UStaticMeshComponent> Mesh;
	/** World label above the actor. Shows Label (or the prompt name) so unmodelled buttons are still findable. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact") TObjectPtr<UTextRenderComponent> LabelText;

	/** Base prompt, e.g. "Use the thing". Subclasses build a richer one in GetPromptFor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") FText Prompt;
	/** Text of the world label. Empty = hide the label. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") FString Label;
	/** Required department (CrewRole number). 0 = any role. Ignored when the manager has bFreeRoles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact") int32 Department = 0;

	/** Role rule (manager bFreeRoles / role 0 allow everything). */
	bool IsAllowedFor(const AMCBayCharacter* Who) const;
	AMCConstructionManager* GetManager() const;

	/** Hover text for the HUD (any machine). */
	virtual FText GetPromptFor(const AMCBayCharacter* Who) const;
	/** SERVER. E pressed. */
	virtual void Use(AMCBayCharacter* Who);
	/** SERVER. E released (only meaningful for hold interactions). */
	virtual void Release(AMCBayCharacter* Who);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interact") void BP_OnUsed(AMCBayCharacter* Who);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	/** Says why on the server (manager message) and returns false when Who's role is wrong. */
	bool CheckRole(const AMCBayCharacter* Who, const TCHAR* WhatFor) const;
	static const TCHAR* DepartmentName(int32 Dept);

	mutable TObjectPtr<AMCConstructionManager> CachedManager;
};

/** Department READY button. Department 1..4 (the button of your own department; others only with bFreeRoles). */
UCLASS(Blueprintable)
class MISSIONCREW_API AMCReadyButton : public AMCInteractable
{
	GENERATED_BODY()
public:
	AMCReadyButton();
	virtual void Tick(float DeltaSeconds) override;
	virtual FText GetPromptFor(const AMCBayCharacter* Who) const override;
	virtual void Use(AMCBayCharacter* Who) override;
};

/** LAUNCH (Construction Engineer, role 2, or bFreeRoles). All four READY, red blocks, yellow needs a second press. */
UCLASS(Blueprintable)
class MISSIONCREW_API AMCLaunchButton : public AMCInteractable
{
	GENERATED_BODY()
public:
	AMCLaunchButton();
	virtual void Tick(float DeltaSeconds) override;
	virtual FText GetPromptFor(const AMCBayCharacter* Who) const override;
	virtual void Use(AMCBayCharacter* Who) override;
};

/** Fuel pump: hold E to pump Rate kg/s into the tanks (negative Rate drains). Propulsion Engineer (3). */
UCLASS(Blueprintable)
class MISSIONCREW_API AMCFuelPump : public AMCInteractable
{
	GENERATED_BODY()
public:
	AMCFuelPump();
	virtual void Tick(float DeltaSeconds) override;
	virtual FText GetPromptFor(const AMCBayCharacter* Who) const override;
	virtual void Use(AMCBayCharacter* Who) override;
	virtual void Release(AMCBayCharacter* Who) override;

	/** kg per second while E is held. Negative = drain variant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pump") float Rate = 60.f;
	/** The operator walks further than this (cm) = pumping stops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pump") float MaxOperatorDistance = 700.f;

protected:
	UPROPERTY() TWeakObjectPtr<AMCBayCharacter> Operator;
};

/** Launcher lever. LeverIndex = index into the manager's Rockets (0 Falcon 9, 1 Vulcan, 2 Falcon Heavy). Construction Engineer (2). */
UCLASS(Blueprintable)
class MISSIONCREW_API AMCRocketLever : public AMCInteractable
{
	GENERATED_BODY()
public:
	AMCRocketLever();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lever") int32 LeverIndex = 0;
	virtual FText GetPromptFor(const AMCBayCharacter* Who) const override;
	virtual void Use(AMCBayCharacter* Who) override;
};

/** One tool lying around: E picks it up (or puts it down if already held). Wrench = bolts (dept 2), Cable = wires (dept 1). */
UCLASS(Blueprintable)
class MISSIONCREW_API AMCToolItem : public AMCInteractable
{
	GENERATED_BODY()
public:
	AMCToolItem();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tool") EMCToolType ToolType = EMCToolType::Wrench;
	static int32 DepartmentOfTool(EMCToolType T) { return T == EMCToolType::Wrench ? 2 : (T == EMCToolType::Cable ? 1 : 0); }
	static const TCHAR* ToolName(EMCToolType T);
	virtual FText GetPromptFor(const AMCBayCharacter* Who) const override;
	virtual void Use(AMCBayCharacter* Who) override;
};

/** Single tools stand: each E hands you the next tool you may use (wrench, cable reel), then puts it back. G also puts the tool down. */
UCLASS(Blueprintable)
class MISSIONCREW_API AMCToolStand : public AMCInteractable
{
	GENERATED_BODY()
public:
	AMCToolStand();
	virtual FText GetPromptFor(const AMCBayCharacter* Who) const override;
	virtual void Use(AMCBayCharacter* Who) override;
};
