// Role 4 Science Operator rack (agent S2b).
//
// Reparent a WBP to UMCSciencePanel and add widgets with EXACTLY these names (all optional):
//   InstTab0..2        Button     select instrument i (action 405+i);  InstTabLabel0..2 TextBlock = name + state
//   OffsetMinus5 / OffsetMinus1 / OffsetPlus1 / OffsetPlus5   Button  actions 400..403
//   LockButton         Button     action 404
//   ReadingBar         ProgressBar  reading on a 0..100 scale, green inside CalTarget +- CalTolerance
//   ReadingText        TextBlock  "Reading 49.2  (offset 3)  target 50 +-3"
//   StatusText         TextBlock  locked / calibrated line
// With no layout in the Blueprint a complete fallback layout is built.
#pragma once

#include "CoreMinimal.h"
#include "Transit/UI/MCTransitPanelBase.h"
#include "MCSciencePanel.generated.h"

class UProgressBar;

UCLASS(Blueprintable)
class MISSIONCREW_API UMCSciencePanel : public UMCTransitPanelBase
{
	GENERATED_BODY()
public:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> InstTab0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> InstTab1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> InstTab2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> InstTabLabel0;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> InstTabLabel1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> InstTabLabel2;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> OffsetMinus5;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> OffsetMinus1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> OffsetPlus1;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> OffsetPlus5;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> LockButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> ReadingBar;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ReadingText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;

	/** Reading bar scale (the reading is shown as Reading / BarMax). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Science") float BarMax = 100.f;

protected:
	virtual void BuildFallbackLayout() override;
	virtual void BindButtons() override;
	virtual void RefreshPanel() override;

	TArray<UButton*> Tabs;
	TArray<UTextBlock*> TabLabels;
};
