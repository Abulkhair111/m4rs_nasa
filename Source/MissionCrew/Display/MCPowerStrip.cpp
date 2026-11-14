#include "Display/MCPowerStrip.h"
#include "Display/MCDisplayHelpers.h"
#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"

AMCPowerStrip::AMCPowerStrip()
{
	DrawSize = FIntPoint(800, 450);
	WorldWidthCm = 200.f;
}

void UMCPowerStripWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget && !GenText && !GenBar) BuildFallbackLayout();
}

void UMCPowerStripWidget::BuildFallbackLayout()
{
	UBorder* Panel = MCDisplay::MakePanel(WidgetTree, TEXT("PowerPanel"));
	WidgetTree->RootWidget = Panel;
	UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PowerVB"));
	Panel->SetContent(VB);
	TitleText = MCDisplay::MakeText(WidgetTree, TEXT("TitleText"), 40, FLinearColor(0.6f, 0.8f, 1.f));
	TitleText->SetText(FText::FromString(TEXT("POWER: GENERATION vs USE")));
	VB->AddChildToVerticalBox(TitleText);

	auto AddRow = [&](TObjectPtr<UTextBlock>& Txt, TObjectPtr<UProgressBar>& Bar, const TCHAR* TName, const TCHAR* BName)
	{
		Txt = MCDisplay::MakeText(WidgetTree, TName, 36);
		if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(Txt)) S->SetPadding(FMargin(0, 12, 0, 2));
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetHeightOverride(46.f);
		Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), BName);
		Box->AddChild(Bar);
		VB->AddChildToVerticalBox(Box);
	};
	AddRow(GenText, GenBar, TEXT("GenText"), TEXT("GenBar"));
	AddRow(UseText, UseBar, TEXT("UseText"), TEXT("UseBar"));
	StatusText = MCDisplay::MakeText(WidgetTree, TEXT("StatusText"), 36);
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(StatusText)) S->SetPadding(FMargin(0, 12, 0, 0));
}

void UMCPowerStripWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Timer -= InDeltaTime;
	if (Timer > 0.f) return;
	Timer = 0.2f;
	if (!Manager) Manager = MCBayUI::FindManager(GetWorld());
	Refresh();
}

void UMCPowerStripWidget::Refresh()
{
	const AMCConstructionManager* M = Manager;
	if (!M) { if (StatusText) StatusText->SetText(FText::FromString(TEXT("No manager"))); return; }
	const EMCCheckState S = M->Checks.IsValidIndex((int32)EMCCheck::Power) ? M->Checks[(int32)EMCCheck::Power] : EMCCheckState::Red;
	const FLinearColor C = MCBayUI::CheckColor(S);
	const float Scale = FMath::Max(FMath::Max(M->PowerGenW, M->PowerUseW) * 1.15f, 1.f);
	if (GenText) GenText->SetText(FText::FromString(FString::Printf(TEXT("Generation %.0f W"), M->PowerGenW)));
	if (UseText) UseText->SetText(FText::FromString(FString::Printf(TEXT("Use %.0f W"), M->PowerUseW)));
	if (GenBar) { GenBar->SetPercent(M->PowerGenW / Scale); GenBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.7f, 1.f)); }
	if (UseBar) { UseBar->SetPercent(M->PowerUseW / Scale); UseBar->SetFillColorAndOpacity(C); }
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(FString::Printf(TEXT("Balance %+.0f W   Battery %.0f Wh\n%s"),
			M->PowerGenW - M->PowerUseW, M->StorageWh,
			S == EMCCheckState::Green ? TEXT("Power OK") : (S == EMCCheckState::Yellow ? TEXT("Thin margin or no battery") : TEXT("NOT ENOUGH POWER")))));
		StatusText->SetColorAndOpacity(FSlateColor(C));
	}
}
