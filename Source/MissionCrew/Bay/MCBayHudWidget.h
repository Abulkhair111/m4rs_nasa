// Always-on bay HUD (agent F): T/W + fuel readout, phase timer, manager message and the look-at prompt.
//
// Reparent a WBP to UMCBayHudWidget and add widgets with EXACTLY these names (all optional):
//   TWRText     TextBlock  "T/W 1.85" coloured with the 1.3 / 2.0 limits
//   FuelText    TextBlock  "Fuel 540 / 1100 kg (need 900)"
//   TimeText    TextBlock  "08:42"
//   MessageText TextBlock  last manager Say() message
//   PromptText  TextBlock  look-at prompt from AMCBayCharacter::AimPrompt
//   TWRBar      ProgressBar  T/W on a 0..3 scale, filled with the same colour
// With no layout in the Blueprint, a plain fallback layout is built (top-left readout, prompt bottom centre).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MCBayHudWidget.generated.h"

class UTextBlock;
class UProgressBar;
class AMCConstructionManager;

UCLASS(Blueprintable)
class MISSIONCREW_API UMCBayHudWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "HUD") void Refresh();

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TWRText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FuelText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TimeText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PromptText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> TWRBar;

	/** T/W bar scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD") float TWRBarMax = 3.f;
	/** How long a manager message stays visible (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD") float MessageSeconds = 6.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void BuildFallbackLayout();

	UPROPERTY() TObjectPtr<AMCConstructionManager> Manager;
	float FindTimer = 0.f;
};
