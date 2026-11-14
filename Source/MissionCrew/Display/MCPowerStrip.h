// Power strip (agent I2): power generation vs use (check 3) with storage. Small world-space widget actor
// (default 800x450 px shown at 200 cm wide). Faces actor +X (see MCWidgetDisplayActor.h).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Display/MCWidgetDisplayActor.h"
#include "MCPowerStrip.generated.h"

class UTextBlock;
class UProgressBar;
class AMCConstructionManager;

/** Optional Blueprint names: TitleText, GenText, UseText, StatusText (TextBlocks), GenBar, UseBar (ProgressBars). */
UCLASS(Blueprintable)
class MISSIONCREW_API UMCPowerStripWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Display") void Refresh();

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> GenText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> UseText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> GenBar;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> UseBar;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void BuildFallbackLayout();

	UPROPERTY() TObjectPtr<AMCConstructionManager> Manager;
	float Timer = 0.f;
};

UCLASS(Blueprintable)
class MISSIONCREW_API AMCPowerStrip : public AMCWidgetDisplayActor
{
	GENERATED_BODY()
public:
	AMCPowerStrip();
protected:
	virtual TSubclassOf<UUserWidget> GetDefaultWidgetClass() const override { return UMCPowerStripWidget::StaticClass(); }
};
