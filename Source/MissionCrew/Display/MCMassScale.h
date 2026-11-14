// Mass scale (agent I2): total take-off mass vs launcher payload (check 1). Small world-space widget actor
// (default 800x450 px shown at 200 cm wide). Faces actor +X (see MCWidgetDisplayActor.h). Text is not mirrored.
// Old BP_MassScale spot: see Vault/S1 Bay Overhaul - Plan.md, "C++ displays and menu".
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Display/MCWidgetDisplayActor.h"
#include "MCMassScale.generated.h"

class UTextBlock;
class UProgressBar;
class AMCConstructionManager;

/** Optional Blueprint names: TitleText, ValueText, PercentText (TextBlocks), MassBar (ProgressBar). */
UCLASS(Blueprintable)
class MISSIONCREW_API UMCMassScaleWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Display") void Refresh();

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ValueText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PercentText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> MassBar;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void BuildFallbackLayout();

	UPROPERTY() TObjectPtr<AMCConstructionManager> Manager;
	float Timer = 0.f;
};

UCLASS(Blueprintable)
class MISSIONCREW_API AMCMassScale : public AMCWidgetDisplayActor
{
	GENERATED_BODY()
public:
	AMCMassScale();
protected:
	virtual TSubclassOf<UUserWidget> GetDefaultWidgetClass() const override { return UMCMassScaleWidget::StaticClass(); }
};
