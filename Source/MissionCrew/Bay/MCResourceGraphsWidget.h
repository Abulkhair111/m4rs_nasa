// C++ base for WBP_ResourceGraphs / the wall board (agent F). Mirrors the Blueprint rows: mass, budget, power, fuel,
// T/W, science, data link, wired, bolted. Each bar is coloured by the manager's Checks[i]; a big T/W gauge shows
// markers at 1.3 and 2.0.
//
// Reparent the WBP and name widgets EXACTLY (all optional). For each row X in
//   Mass Budget Power Fuel TWR Science DataLink Wired Bolted
// add   Bar_X  (ProgressBar, fill colour is set from the check)  and  Txt_X  (TextBlock with the numbers).
// Row -> check: Mass=Checks[1] Budget=2 Power=3 Fuel=5 TWR=6 Science=7 DataLink=4 Wired=8 Bolted=9.
// Extra widgets:
//   TWRGauge        ProgressBar  big gauge, scale 0..GaugeMax (default 3.0)
//   TWRGaugeText    TextBlock    "T/W 1.85"
//   TWRMarkerLow    Widget       thin line at 1.3; anchored/aligned LEFT inside the gauge's overlay; moved with RenderTranslation
//   TWRMarkerHigh   Widget       thin line at 2.0, same rules
//   CriticalText    TextBlock    first red problem ("Fuel: need 900 kg ...") or empty
// With no layout in the Blueprint, a plain fallback layout is built (usable in a WidgetComponent).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MCResourceGraphsWidget.generated.h"

class UWidget;
class UProgressBar;
class UTextBlock;
class AMCConstructionManager;

UCLASS(Blueprintable)
class MISSIONCREW_API UMCResourceGraphsWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Re-reads the manager and updates every bar. Called from NativeTick (every RefreshInterval). */
	UFUNCTION(BlueprintCallable, Category = "Graphs") void Refresh();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphs") float GaugeMax = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphs") float RefreshInterval = 0.1f;
	/** Science value that fills the science bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphs") float ScienceTarget = 100.f;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Bar_Mass;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Mass;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Bar_Budget;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Budget;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Bar_Power;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Power;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Bar_Fuel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Fuel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Bar_TWR;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_TWR;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Bar_Science;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Science;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Bar_DataLink;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_DataLink;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Bar_Wired;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Wired;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> Bar_Bolted;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Txt_Bolted;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> TWRGauge;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TWRGaugeText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> TWRMarkerLow;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> TWRMarkerHigh;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CriticalText;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	void BuildFallbackLayout();
	void SetRow(UProgressBar* Bar, UTextBlock* Txt, float Fraction, const FString& Text, const FLinearColor& Colour);
	void PlaceMarkers();

	UPROPERTY() TObjectPtr<AMCConstructionManager> Manager;
	float Timer = 0.f;
};
