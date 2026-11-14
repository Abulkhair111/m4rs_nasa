// S2 Launch & Transit HUD (agent S2b): top bar, message line, mission board, GO FOR LAUNCH button and the ONE panel of the local role.
//
// Reparent a WBP to UMCTransitHudWidget and add widgets with EXACTLY these names (all optional):
//   TopText      TextBlock   "COAST TO MOON | T+ 45 / 180 s | SEED 4242 | HEALTH 96 | FUEL 540 kg"
//   MessageText  TextBlock   last manager message (Manager->Message)
//   BoardText    TextBlock   mission board (Manager->BoardText; the final ReportText after arrival)
//   GoButton     Button      GO FOR LAUNCH (action 90), visible only in the GO poll;  GoLabel TextBlock = caption with the GO flags
//   PanelHost    PanelWidget the role panel is added here (any panel; the panel is created at runtime)
// Role panels: the class is chosen by the role read from the PlayerState CrewRole (1 Systems, 2 Equipment, 3 Pilot, 4 Science).
// Override the *PanelClass properties to use a styled WBP child instead of the in-code panels.
// Role 0 (unassigned, solo test) gets four tab buttons (in the fallback layout) to switch panels.
// With no layout in the Blueprint a complete fallback layout is built.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Transit/MCTransitTypes.h"
#include "MCTransitHudWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;
class AMCTransitManager;
class UMCTransitPanelBase;
class UMCTransitHudWidget;

/** Click relay of the HUD: Mode 0 = send action Code, Mode 1 = SetTestRole(Code). */
UCLASS()
class MISSIONCREW_API UMCTransitHudRelay : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<UMCTransitHudWidget> Hud;
	int32 Mode = 0;
	int32 Code = 0;
	UFUNCTION() void Fire();
};

UCLASS(Blueprintable)
class MISSIONCREW_API UMCTransitHudWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TopText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> BoardText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> GoButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GoLabel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> PanelHost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit") TSubclassOf<UMCTransitPanelBase> SystemsPanelClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit") TSubclassOf<UMCTransitPanelBase> EquipPanelClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit") TSubclassOf<UMCTransitPanelBase> PilotPanelClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit") TSubclassOf<UMCTransitPanelBase> SciencePanelClass;
	/** How long a manager message stays visible (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit") float MessageSeconds = 8.f;

	/** Role whose panel is shown now (1..4); 0 = none yet. */
	UFUNCTION(BlueprintPure, Category = "Transit") int32 GetShownRole() const { return ShownRole; }
	/** Role-0 test mode: force a role panel (the fallback tab buttons call this through SendTab). */
	UFUNCTION(BlueprintCallable, Category = "Transit") void SetTestRole(int32 Role);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void BuildFallbackLayout();
	void BuildPanel(int32 Role);
	void Refresh();

	UPROPERTY(Transient) TObjectPtr<AMCTransitManager> Manager;
	UPROPERTY(Transient) TObjectPtr<UMCTransitPanelBase> Panel;
	UPROPERTY(Transient) TObjectPtr<UPanelWidget> TabBox;
	UPROPERTY(Transient) TArray<TObjectPtr<UMCTransitHudRelay>> Relays;
	void BindClick(UButton* Button, int32 Mode, int32 Code);
	int32 ShownRole = 0;
	int32 TestRole = 0;
	float FindTimer = 0.f;
	FString LastMessage;
	double MessageTime = -1000.0;
};
