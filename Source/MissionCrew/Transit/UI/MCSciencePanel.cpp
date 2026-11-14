#include "Transit/UI/MCSciencePanel.h"
#include "Transit/MCTransitManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UMCSciencePanel::BuildFallbackLayout()
{
	UVerticalBox* VB = MakeRootBox(TEXT("SCIENCE RACK"), MCTransitUI::Cyan());

	UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TabRow"));
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(TabRow)) S->SetPadding(FMargin(0.f, 4.f));
	for (int32 i = 0; i < MCTransit::MaxInstruments; ++i)
	{
		UTextBlock* L = nullptr;
		UButton* B = MakeButton(*FString::Printf(TEXT("InstTab%d"), i), TEXT("-"), L, 110.f);
		L->Rename(*FString::Printf(TEXT("InstTabLabel%d"), i));
		if (UHorizontalBoxSlot* S = TabRow->AddChildToHorizontalBox(B)) S->SetPadding(FMargin(2.f, 0.f));
		switch (i)
		{
		case 0: InstTab0 = B; InstTabLabel0 = L; break;
		case 1: InstTab1 = B; InstTabLabel1 = L; break;
		default: InstTab2 = B; InstTabLabel2 = L; break;
		}
	}

	ReadingText = MakeText(TEXT("ReadingText"), 16, MCTransitUI::Cyan());
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(ReadingText)) S->SetPadding(FMargin(0.f, 8.f, 0.f, 2.f));
	ReadingBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ReadingBar"));
	VB->AddChildToVerticalBox(ReadingBar);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("OffsetRow"));
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(Row)) S->SetPadding(FMargin(0.f, 8.f));
	auto AddBtn = [&](TObjectPtr<UButton>& Out, const TCHAR* Name, const TCHAR* Label, float W) {
		UTextBlock* L = nullptr;
		Out = MakeButton(Name, Label, L, W);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Out)) S->SetPadding(FMargin(3.f, 0.f));
	};
	AddBtn(OffsetMinus5, TEXT("OffsetMinus5"), TEXT("-5"), 44.f);
	AddBtn(OffsetMinus1, TEXT("OffsetMinus1"), TEXT("-1"), 44.f);
	AddBtn(OffsetPlus1, TEXT("OffsetPlus1"), TEXT("+1"), 44.f);
	AddBtn(OffsetPlus5, TEXT("OffsetPlus5"), TEXT("+5"), 44.f);
	AddBtn(LockButton, TEXT("LockButton"), TEXT("LOCK"), 80.f);

	StatusText = MakeText(TEXT("StatusText"), 15, MCTransitUI::Amber());
	VB->AddChildToVerticalBox(StatusText);
}

void UMCSciencePanel::BindButtons()
{
	Tabs = { InstTab0, InstTab1, InstTab2 };
	TabLabels = { InstTabLabel0, InstTabLabel1, InstTabLabel2 };
	for (int32 i = 0; i < Tabs.Num(); ++i) BindButton(Tabs[i], MCTransit::MakeCode(EMCTransitRole::Science, (int32)EMCScienceAction::SelectInstrument0 + i));
	BindButton(OffsetMinus5, MCTransit::MakeCode(EMCTransitRole::Science, (int32)EMCScienceAction::Minus5));
	BindButton(OffsetMinus1, MCTransit::MakeCode(EMCTransitRole::Science, (int32)EMCScienceAction::Minus1));
	BindButton(OffsetPlus1, MCTransit::MakeCode(EMCTransitRole::Science, (int32)EMCScienceAction::Plus1));
	BindButton(OffsetPlus5, MCTransit::MakeCode(EMCTransitRole::Science, (int32)EMCScienceAction::Plus5));
	BindButton(LockButton, MCTransit::MakeCode(EMCTransitRole::Science, (int32)EMCScienceAction::Lock));
}

void UMCSciencePanel::RefreshPanel()
{
	for (int32 i = 0; i < Tabs.Num(); ++i)
	{
		const bool bHave = Manager->Instruments.IsValidIndex(i);
		if (Tabs[i])
		{
			Tabs[i]->SetVisibility(bHave ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			Tabs[i]->SetBackgroundColor(bHave && i == Manager->ActiveInstrument ? MCTransitUI::BtnOn() : MCTransitUI::BtnOff());
		}
		if (TabLabels[i] && bHave)
		{
			const FMCTransitInstrument& I = Manager->Instruments[i];
			TabLabels[i]->SetText(FText::FromString(I.Name + (I.bLocked ? (I.bCalibrated ? TEXT(" (OK)") : TEXT(" (bad)")) : TEXT(""))));
		}
	}
	const FMCTransitInstrument* I = Manager->Instruments.IsValidIndex(Manager->ActiveInstrument) ? &Manager->Instruments[Manager->ActiveInstrument] : nullptr;
	const float T = Manager->CalTarget, Tol = Manager->CalTolerance;
	const bool bIn = I && FMath::Abs(I->Reading - T) <= Tol;
	if (ReadingText)
	{
		ReadingText->SetText(FText::FromString(I
			? FString::Printf(TEXT("%s reading %.1f  (offset %+.0f)  target %.0f +-%.0f"), *I->Name, I->Reading, I->Offset, T, Tol)
			: FString(TEXT("No instruments to calibrate."))));
		ReadingText->SetColorAndOpacity(FSlateColor(bIn ? MCTransitUI::Green() : MCTransitUI::Cyan()));
	}
	if (ReadingBar)
	{
		ReadingBar->SetPercent(I ? FMath::Clamp(I->Reading / BarMax, 0.f, 1.f) : 0.f);
		ReadingBar->SetFillColorAndOpacity(bIn ? MCTransitUI::Green() : MCTransitUI::Amber());
	}
	const bool bEdit = I && !I->bLocked;
	for (UButton* B : { OffsetMinus5.Get(), OffsetMinus1.Get(), OffsetPlus1.Get(), OffsetPlus5.Get(), LockButton.Get() })
		if (B) B->SetIsEnabled(bEdit);
	if (StatusText)
	{
		FString S;
		if (I) S = I->bLocked ? (I->bCalibrated ? TEXT("LOCKED - calibrated") : TEXT("LOCKED - out of tolerance")) : TEXT("Needs INSTRUMENT RAIL ON (ask Systems). Set the offset, then LOCK.");
		StatusText->SetText(FText::FromString(S));
	}
}
