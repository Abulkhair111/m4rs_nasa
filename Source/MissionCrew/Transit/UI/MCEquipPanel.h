// Role 2 Equipment Technician console (agent S2b).
//
// Reparent a WBP to UMCEquipPanel and add widgets with EXACTLY these names (all optional):
//   DeviceTab0..3    Button     select device i (action 203+i);  DeviceTabLabel0..3 TextBlock = device name + state
//   StepButton0..2   Button     procedure button at position i (action 200+i);  StepLabel0..2 TextBlock = caption (scrambled per seed)
//   ProcedureText    TextBlock  the correct step order of the selected device, progress marked
//   StatusText       TextBlock  testing / passed line
// With no layout in the Blueprint a complete fallback layout is built.
#pragma once

#include "CoreMinimal.h"
#include "Transit/UI/MCTransitPanelBase.h"
#include "MCEquipPanel.generated.h"

UCLASS(Blueprintable)
class MISSIONCREW_API UMCEquipPanel : public UMCTransitPanelBase
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DeviceTab0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DeviceTab1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DeviceTab2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> DeviceTab3;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DeviceTabLabel0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DeviceTabLabel1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DeviceTabLabel2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DeviceTabLabel3;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> StepButton0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> StepButton1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> StepButton2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StepLabel0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StepLabel1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StepLabel2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ProcedureText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;

protected:
	virtual void BuildFallbackLayout() override;
	virtual void BindButtons() override;
	virtual void RefreshPanel() override;

	TArray<UButton*> Tabs;
	TArray<UTextBlock*> TabLabels;
	TArray<UButton*> Steps;
	TArray<UTextBlock*> StepLabels;
};
