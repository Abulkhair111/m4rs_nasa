// Wall check board (agent I2), C++ replacement of WBP_WallBoard. Shows the 10 preflight checks colour-coded, a header
// (thrust, T/W, tanks, ice), failing checks, accuracy per role and the last manager message.
//
// Optional Blueprint names (all BindWidgetOptional / found by name): HeaderText, FailText, AccuracyText, MessageText,
// CheckText_0..CheckText_9 (TextBlocks), CheckSwatch_0..CheckSwatch_9 (Border). With no layout a code layout is built.
// Check index order: 0 launcher, 1 mass, 2 budget, 3 power, 4 comms, 5 fuel, 6 thrust T/W, 7 ice science, 8 power grid, 9 structure.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MCWallBoardWidget.generated.h"

class UTextBlock;
class UBorder;
class AMCConstructionManager;

UCLASS(Blueprintable)
class MISSIONCREW_API UMCWallBoardWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Board") void Refresh();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board") float RefreshInterval = 0.2f;
	/** How long a manager message stays visible (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board") float MessageSeconds = 20.f;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HeaderText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FailText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AccuracyText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MessageText;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void BuildFallbackLayout();

	UPROPERTY() TArray<TObjectPtr<UTextBlock>> CheckTexts;
	UPROPERTY() TArray<TObjectPtr<UBorder>> CheckSwatches;
	UPROPERTY() TObjectPtr<AMCConstructionManager> Manager;
	float Timer = 0.f;
};
