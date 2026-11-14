// Role 1 Systems Operator console (agent S2b).
//
// Reparent a WBP to UMCSystemsPanel and add widgets with EXACTLY these names (all optional):
//   SwitchButton0..5  Button     click = action 100+i (HEATERS, NAV SENSOR, TRANSPONDER, WHEEL DESAT, INSTRUMENT RAIL, MECHANISM RAIL)
//   SwitchLabel0..5   TextBlock  label from the manager, e.g. "NAV SENSOR  [B]"
//   AlarmText         TextBlock  alarm list ("! text" open, "ok text" fixed)
// With no layout in the Blueprint a complete fallback layout is built.
#pragma once

#include "CoreMinimal.h"
#include "Transit/UI/MCTransitPanelBase.h"
#include "MCSystemsPanel.generated.h"

UCLASS(Blueprintable)
class MISSIONCREW_API UMCSystemsPanel : public UMCTransitPanelBase
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SwitchButton0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SwitchButton1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SwitchButton2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SwitchButton3;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SwitchButton4;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> SwitchButton5;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SwitchLabel0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SwitchLabel1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SwitchLabel2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SwitchLabel3;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SwitchLabel4;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SwitchLabel5;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AlarmText;

protected:
	virtual void BuildFallbackLayout() override;
	virtual void BindButtons() override;
	virtual void RefreshPanel() override;

	TArray<UButton*> Buttons;
	TArray<UTextBlock*> Labels;
};
