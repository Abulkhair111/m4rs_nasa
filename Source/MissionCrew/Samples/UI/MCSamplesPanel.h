// S5 role panel (C++ UMG, built in code). Shows ONLY the local player's role section (all four if the manager has bFreeRoles).
// Buttons call AMCSamplesRelay::ServerAction on the local player's relay (server forwards to AMCSamplesManager::HandleAction).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MCSamplesPanel.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class AMCSamplesManager;
class UMCSamplesPanel;

UCLASS()
class MISSIONCREW_API UMCSamplesClickRelay : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<UMCSamplesPanel> Owner;
	int32 Code = 0;
	UFUNCTION() void Fire();
};

UCLASS()
class MISSIONCREW_API UMCSamplesPanel : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient) TObjectPtr<AMCSamplesManager> Manager;
	void SendAction(int32 Code);
	int32 GetLocalRole() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& G, float Dt) override;
	UButton* AddButton(UVerticalBox* Box, const FString& Label, int32 Code);
	UTextBlock* AddText(UVerticalBox* Box, int32 Size, FLinearColor Color);

	UPROPERTY(Transient) TArray<TObjectPtr<UMCSamplesClickRelay>> Relays;
	UPROPERTY(Transient) TArray<TObjectPtr<UVerticalBox>> RoleBoxes; // index 1..4
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MsgText;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RoleText; // index 1..4
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> RoutineBtn;  // index 1..4
	float Timer = 0.f;
};
