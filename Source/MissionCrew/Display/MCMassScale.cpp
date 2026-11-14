#include "Display/MCMassScale.h"
#include "Display/MCDisplayHelpers.h"
#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"

AMCMassScale::AMCMassScale()
{
	DrawSize = FIntPoint(800, 450);
	WorldWidthCm = 200.f;
}

void UMCMassScaleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget && !ValueText && !MassBar) BuildFallbackLayout();
}

void UMCMassScaleWidget::BuildFallbackLayout()
{
	UBorder* Panel = MCDisplay::MakePanel(WidgetTree, TEXT("ScalePanel"));
	WidgetTree->RootWidget = Panel;
	UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ScaleVB"));
	Panel->SetContent(VB);
	TitleText = MCDisplay::MakeText(WidgetTree, TEXT("TitleText"), 40, FLinearColor(0.6f, 0.8f, 1.f));
	TitleText->SetText(FText::FromString(TEXT("MASS vs LAUNCHER")));
	VB->AddChildToVerticalBox(TitleText);
	ValueText = MCDisplay::MakeText(WidgetTree, TEXT("ValueText"), 44);
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(ValueText)) S->SetPadding(FMargin(0, 16, 0, 8));
	USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	BarBox->SetHeightOverride(70.f);
	MassBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("MassBar"));
	BarBox->AddChild(MassBar);
	VB->AddChildToVerticalBox(BarBox);
	PercentText = MCDisplay::MakeText(WidgetTree, TEXT("PercentText"), 40);
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(PercentText)) S->SetPadding(FMargin(0, 8, 0, 0));
}

void UMCMassScaleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Timer -= InDeltaTime;
	if (Timer > 0.f) return;
	Timer = 0.2f;
	if (!Manager) Manager = MCBayUI::FindManager(GetWorld());
	Refresh();
}

void UMCMassScaleWidget::Refresh()
{
	const AMCConstructionManager* M = Manager;
	if (!M) { if (ValueText) ValueText->SetText(FText::FromString(TEXT("No manager"))); return; }
	const EMCCheckState S = M->Checks.IsValidIndex((int32)EMCCheck::Mass) ? M->Checks[(int32)EMCCheck::Mass] : EMCCheckState::Red;
	const FLinearColor C = MCBayUI::CheckColor(S);
	const bool bLauncher = M->RocketPayloadKg > 0.f;
	const float Frac = bLauncher ? M->TotalMassKg / M->RocketPayloadKg : 0.f;
	if (ValueText)
	{
		ValueText->SetText(FText::FromString(bLauncher
			? FString::Printf(TEXT("%.0f / %.0f kg  (%s)"), M->TotalMassKg, M->RocketPayloadKg, *M->RocketName.ToString())
			: FString::Printf(TEXT("%.0f kg  (no launcher)"), M->TotalMassKg)));
		ValueText->SetColorAndOpacity(FSlateColor(C));
	}
	if (MassBar) { MassBar->SetPercent(FMath::Clamp(Frac, 0.f, 1.f)); MassBar->SetFillColorAndOpacity(C); }
	if (PercentText)
	{
		PercentText->SetText(FText::FromString(!bLauncher ? FString(TEXT("Choose a launcher")) :
			(Frac > 1.f ? FString::Printf(TEXT("OVERWEIGHT by %.0f kg"), M->TotalMassKg - M->RocketPayloadKg)
			            : FString::Printf(TEXT("%.0f%% of payload, %.0f kg spare"), Frac * 100.f, M->RocketPayloadKg - M->TotalMassKg))));
		PercentText->SetColorAndOpacity(FSlateColor(C));
	}
}
