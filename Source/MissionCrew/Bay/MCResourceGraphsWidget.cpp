#include "Bay/MCResourceGraphsWidget.h"
#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

namespace
{
	const TCHAR* CheckNames[MCConst::NumChecks] = {
		TEXT("Launcher"), TEXT("Mass"), TEXT("Budget"), TEXT("Power"), TEXT("Comms"),
		TEXT("Fuel"), TEXT("Thrust (T/W)"), TEXT("Ice science"), TEXT("Wiring"), TEXT("Bolts") };
}

void UMCResourceGraphsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!Bar_Mass && !Bar_TWR && !TWRGauge && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
	}
}

void UMCResourceGraphsWidget::BuildFallbackLayout()
{
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GraphsRoot"));
	Root->SetBrushColor(FLinearColor(0.02f, 0.04f, 0.09f, 0.85f));
	Root->SetPadding(FMargin(14.f));
	WidgetTree->RootWidget = Root;
	UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GraphsVB"));
	Root->SetContent(VB);

	auto Text = [&](const TCHAR* Name, int32 Size) {
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		T->SetFont(F);
		return T;
	};

	// Big T/W gauge with markers.
	TWRGaugeText = Text(TEXT("TWRGaugeText"), 28);
	VB->AddChildToVerticalBox(TWRGaugeText);
	UOverlay* GaugeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("GaugeOverlay"));
	USizeBox* GaugeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("GaugeBox"));
	GaugeBox->SetHeightOverride(34.f);
	GaugeBox->AddChild(GaugeOverlay);
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(GaugeBox)) S->SetPadding(FMargin(0, 4, 0, 10));
	TWRGauge = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("TWRGauge"));
	GaugeOverlay->AddChildToOverlay(TWRGauge);
	auto Marker = [&](const TCHAR* Name) {
		USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		SB->SetWidthOverride(3.f);
		UBorder* B = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		B->SetBrushColor(FLinearColor::White);
		SB->AddChild(B);
		if (UOverlaySlot* S = GaugeOverlay->AddChildToOverlay(SB)) { S->SetHorizontalAlignment(HAlign_Left); S->SetVerticalAlignment(VAlign_Fill); }
		return SB;
	};
	TWRMarkerLow = Marker(TEXT("TWRMarkerLow"));
	TWRMarkerHigh = Marker(TEXT("TWRMarkerHigh"));

	struct FRowDef { const TCHAR* Label; TObjectPtr<UProgressBar>* Bar; TObjectPtr<UTextBlock>* Txt; EMCStackGroup Group; };
	// Bar == nullptr -> a section header (with a totals text for LANDER / ASCENT).
	const FRowDef Rows[] = {
		{ TEXT("Mass"),      &Bar_Mass,     &Txt_Mass,     EMCStackGroup::Lander },
		{ TEXT("Budget"),    &Bar_Budget,   &Txt_Budget,   EMCStackGroup::Lander },
		{ TEXT("LANDER"),    nullptr,       &Txt_LanderTotals, EMCStackGroup::Lander },
		{ TEXT("Power"),     &Bar_Power,    &Txt_Power,    EMCStackGroup::Lander },
		{ TEXT("Science"),   &Bar_Science,  &Txt_Science,  EMCStackGroup::Lander },
		{ TEXT("Data link"), &Bar_DataLink, &Txt_DataLink, EMCStackGroup::Lander },
		{ TEXT("ASCENT"),    nullptr,       &Txt_AscentTotals, EMCStackGroup::Ascent },
		{ TEXT("Fuel"),      &Bar_Fuel,     &Txt_Fuel,     EMCStackGroup::Ascent },
		{ TEXT("T/W"),       &Bar_TWR,      &Txt_TWR,      EMCStackGroup::Ascent },
		{ TEXT("Wired"),     &Bar_Wired,    &Txt_Wired,    EMCStackGroup::Lander },
		{ TEXT("Bolted"),    &Bar_Bolted,   &Txt_Bolted,   EMCStackGroup::Lander },
	};
	for (const FRowDef& R : Rows)
	{
		if (!R.Bar)
		{
			UTextBlock* Head = Text(*FString::Printf(TEXT("Hdr_%s"), R.Label), 18);
			Head->SetText(FText::FromString(R.Label));
			Head->SetColorAndOpacity(FSlateColor(MCBayUI::GroupColor(R.Group)));
			if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(Head)) S->SetPadding(FMargin(0, 10, 0, 0));
			*R.Txt = Text(*FString::Printf(TEXT("RowTxt_%s"), R.Label), 12);
			(*R.Txt)->SetAutoWrapText(true);
			if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(*R.Txt)) S->SetPadding(FMargin(0, 1));
			continue;
		}
		UTextBlock* Label = Text(*FString::Printf(TEXT("Lbl_%s"), R.Label), 14);
		Label->SetText(FText::FromString(R.Label));
		*R.Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
		*R.Txt = Text(*FString::Printf(TEXT("RowTxt_%s"), R.Label), 12);

		UHorizontalBox* HB = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LabelBox->SetWidthOverride(90.f);
		LabelBox->AddChild(Label);
		HB->AddChildToHorizontalBox(LabelBox);
		USizeBox* BarBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		BarBox->SetHeightOverride(16.f);
		BarBox->AddChild(*R.Bar);
		if (UHorizontalBoxSlot* S = HB->AddChildToHorizontalBox(BarBox)) { S->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); S->SetVerticalAlignment(VAlign_Center); }
		if (UHorizontalBoxSlot* S = HB->AddChildToHorizontalBox(*R.Txt)) { S->SetPadding(FMargin(10, 0, 0, 0)); S->SetVerticalAlignment(VAlign_Center); }
		if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(HB)) S->SetPadding(FMargin(0, 3));
	}

	CriticalText = Text(TEXT("CriticalText"), 14);
	CriticalText->SetAutoWrapText(true);
	CriticalText->SetColorAndOpacity(FSlateColor(MCBayUI::CheckColor(EMCCheckState::Red)));
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(CriticalText)) S->SetPadding(FMargin(0, 10, 0, 0));
}

void UMCResourceGraphsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Timer -= InDeltaTime;
	if (Timer > 0.f) { PlaceMarkers(); return; }
	Timer = RefreshInterval;
	if (!Manager) Manager = MCBayUI::FindManager(GetWorld());
	Refresh();
	PlaceMarkers();
}

void UMCResourceGraphsWidget::PlaceMarkers()
{
	if (!TWRGauge) return;
	const float W = TWRGauge->GetCachedGeometry().GetLocalSize().X;
	if (W <= 1.f || GaugeMax <= 0.f) return;
	if (TWRMarkerLow)  TWRMarkerLow->SetRenderTranslation(FVector2D(W * MCConst::TWRRed / GaugeMax, 0.f));
	if (TWRMarkerHigh) TWRMarkerHigh->SetRenderTranslation(FVector2D(W * MCConst::TWRGreen / GaugeMax, 0.f));
}

void UMCResourceGraphsWidget::SetRow(UProgressBar* Bar, UTextBlock* Txt, float Fraction, const FString& Text, const FLinearColor& Colour)
{
	if (Bar)
	{
		Bar->SetPercent(FMath::Clamp(Fraction, 0.f, 1.f));
		Bar->SetFillColorAndOpacity(Colour);
	}
	if (Txt)
	{
		Txt->SetText(FText::FromString(Text));
		Txt->SetColorAndOpacity(FSlateColor(Colour));
	}
}

void UMCResourceGraphsWidget::Refresh()
{
	const AMCConstructionManager* M = Manager;
	if (!M) return;

	auto Col = [M](EMCCheck C) { return MCBayUI::CheckColor(M->GetCheck(C)); };
	auto StateFrac = [M](EMCCheck C) {
		switch (M->GetCheck(C)) { case EMCCheckState::Green: return 1.f; case EMCCheckState::Yellow: return 0.5f; default: return 0.15f; }
	};

	const float Payload = M->RocketPayloadKg;
	SetRow(Bar_Mass, Txt_Mass, Payload > 0.f ? M->TotalMassKg / Payload : 0.f,
		Payload > 0.f ? FString::Printf(TEXT("%.0f / %.0f kg"), M->TotalMassKg, Payload) : FString::Printf(TEXT("%.0f kg (no rocket)"), M->TotalMassKg),
		Col(EMCCheck::Mass));

	SetRow(Bar_Budget, Txt_Budget, M->BudgetM > 0.f ? M->TotalCostM / M->BudgetM : 0.f,
		FString::Printf(TEXT("$%.0fM / $%.0fM"), M->TotalCostM, M->BudgetM), Col(EMCCheck::Budget));

	// Power: fills at 100% when generation covers 110% of use.
	const float PowerNeed = FMath::Max(M->PowerUseW * 1.1f, 1.f);
	SetRow(Bar_Power, Txt_Power, M->PowerGenW / PowerNeed,
		FString::Printf(TEXT("%.0f W gen / %.0f W use"), M->PowerGenW, M->PowerUseW), Col(EMCCheck::Power));

	SetRow(Bar_Fuel, Txt_Fuel, M->FuelRequiredKg > 0.f ? M->FuelKg / M->FuelRequiredKg : 0.f,
		FString::Printf(TEXT("%.0f / %.0f kg (need %.0f)"), M->FuelKg, M->FuelMaxKg, M->FuelRequiredKg), Col(EMCCheck::Fuel));

	const float Twr = M->TWR;
	SetRow(Bar_TWR, Txt_TWR, Twr / FMath::Max(GaugeMax, 0.1f), FString::Printf(TEXT("%.2f"), Twr), Col(EMCCheck::Thrust));
	SetRow(TWRGauge, TWRGaugeText, Twr / FMath::Max(GaugeMax, 0.1f), FString::Printf(TEXT("T/W %.2f"), Twr), Col(EMCCheck::Thrust));

	SetRow(Bar_Science, Txt_Science, ScienceTarget > 0.f ? M->ScienceValue / ScienceTarget : 0.f,
		FString::Printf(TEXT("%.0f pts, ice x%d"), M->ScienceValue, M->IceCount), Col(EMCCheck::IceScience));

	SetRow(Bar_DataLink, Txt_DataLink, M->DataMbps > 0.f ? M->BandwidthMbps / M->DataMbps : (M->BandwidthMbps > 0.f ? 1.f : 0.f),
		FString::Printf(TEXT("%.1f data / %.1f link Mbps"), M->DataMbps, M->BandwidthMbps), Col(EMCCheck::Comms));

	SetRow(Bar_Wired, Txt_Wired, StateFrac(EMCCheck::Grid),
		M->OffGridCount == 0 ? FString(TEXT("all wired")) : FString::Printf(TEXT("%d not wired (lander %d, ascent %d)"), M->OffGridCount, M->LanderTotals.OffGridCount, M->AscentTotals.OffGridCount), Col(EMCCheck::Grid));

	SetRow(Bar_Bolted, Txt_Bolted, StateFrac(EMCCheck::Structure),
		M->UnboltedCount == 0 ? FString(TEXT("all bolted")) : FString::Printf(TEXT("%d loose (lander %d, ascent %d)"), M->UnboltedCount, M->LanderTotals.UnboltedCount, M->AscentTotals.UnboltedCount), Col(EMCCheck::Structure));

	const FMCGroupTotals& L = M->LanderTotals;
	const FMCGroupTotals& A = M->AscentTotals;
	if (Txt_LanderTotals)
	{
		Txt_LanderTotals->SetText(FText::FromString(FString::Printf(TEXT("%.0f kg | $%.0fM | %d parts | science %.0f, ice x%d | power %.0f / %.0f W"),
			L.MassKg, L.CostM, L.PartCount, L.ScienceValue, L.IceCount, L.PowerGenW, L.PowerUseW)));
		Txt_LanderTotals->SetColorAndOpacity(FSlateColor(MCBayUI::GroupColor(EMCStackGroup::Lander)));
	}
	if (Txt_AscentTotals)
	{
		Txt_AscentTotals->SetText(FText::FromString(FString::Printf(TEXT("%.0f kg | $%.0fM | %d parts | thrust %.1f kN | tanks %.0f kg | boosters %d | take-off %.0f kg"),
			A.MassKg, A.CostM, A.PartCount, A.ThrustN / 1000.f, A.FuelMaxKg, A.BoosterCount, M->TotalMassKg)));
		Txt_AscentTotals->SetColorAndOpacity(FSlateColor(MCBayUI::GroupColor(EMCStackGroup::Ascent)));
	}

	if (CriticalText)
	{
		FString Crit;
		for (int32 i = 0; i < MCConst::NumChecks && i < M->Checks.Num(); ++i)
		{
			if (M->Checks[i] == EMCCheckState::Red)
			{
				Crit = FString::Printf(TEXT("%s: red"), CheckNames[i]);
				break;
			}
		}
		CriticalText->SetText(FText::FromString(Crit));
	}
}
