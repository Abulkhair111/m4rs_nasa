// S6 HUD + role panels + final summary, built in code. Each client shows ONLY its role's panel (role 0 = role 1 panel for solo tests).
// Buttons -> AMCReturnLink::ServerAction(Code). On completion the summary board replaces the panel; a button opens L_MainMenu.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MCReturnHud.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class AMCReturnManager;
class UMCReturnHudWidget;

UCLASS()
class MISSIONCREW_API UMCReturnClickRelay : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<UMCReturnHudWidget> Hud;
	int32 Code = 0;   // >= 0 action code, -1 = main menu
	UFUNCTION() void Fire();
};

UCLASS()
class MISSIONCREW_API UMCReturnHudWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Click(int32 Code);
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& G, float Dt) override;
	void BuildPanel(int32 Role);
	UButton* AddButton(UVerticalBox* VB, const FString& Label, int32 Code);
	UTextBlock* MakeText(int32 Size, FLinearColor C);

	UPROPERTY(Transient) TObjectPtr<AMCReturnManager> Mgr;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TopText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MsgText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BoardText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PanelBody;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SummaryText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> PanelBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SummaryBox;
	UPROPERTY(Transient) TArray<TObjectPtr<UMCReturnClickRelay>> Relays;
	int32 Shown = -1;
	bool bSummaryShown = false;
	float FindTimer = 0.f;
};
