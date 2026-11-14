#include "Transit/UI/MCEquipPanel.h"
#include "Transit/MCTransitManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UMCEquipPanel::BuildFallbackLayout()
{
	UVerticalBox* VB = MakeRootBox(TEXT("EQUIPMENT BAY"), MCTransitUI::Cyan());

	UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TabRow"));
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(TabRow)) S->SetPadding(FMargin(0.f, 4.f));
	for (int32 i = 0; i < MCTransit::MaxDevices; ++i)
	{
		UTextBlock* L = nullptr;
		UButton* B = MakeButton(*FString::Printf(TEXT("DeviceTab%d"), i), TEXT("-"), L, 100.f);
		L->Rename(*FString::Printf(TEXT("DeviceTabLabel%d"), i));
		if (UHorizontalBoxSlot* S = TabRow->AddChildToHorizontalBox(B)) S->SetPadding(FMargin(2.f, 0.f));
		switch (i)
		{
		case 0: DeviceTab0 = B; DeviceTabLabel0 = L; break;
		case 1: DeviceTab1 = B; DeviceTabLabel1 = L; break;
		case 2: DeviceTab2 = B; DeviceTabLabel2 = L; break;
		default: DeviceTab3 = B; DeviceTabLabel3 = L; break;
		}
	}

	ProcedureText = MakeText(TEXT("ProcedureText"), 14, MCTransitUI::Dim());
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(ProcedureText)) S->SetPadding(FMargin(0.f, 8.f));

	UHorizontalBox* StepRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("StepRow"));
	VB->AddChildToVerticalBox(StepRow);
	for (int32 i = 0; i < 3; ++i)
	{
		UTextBlock* L = nullptr;
		UButton* B = MakeButton(*FString::Printf(TEXT("StepButton%d"), i), TEXT("-"), L, 130.f);
		L->Rename(*FString::Printf(TEXT("StepLabel%d"), i));
		if (UHorizontalBoxSlot* S = StepRow->AddChildToHorizontalBox(B)) S->SetPadding(FMargin(3.f, 0.f));
		switch (i)
		{
		case 0: StepButton0 = B; StepLabel0 = L; break;
		case 1: StepButton1 = B; StepLabel1 = L; break;
		default: StepButton2 = B; StepLabel2 = L; break;
		}
	}
	StatusText = MakeText(TEXT("StatusText"), 16, MCTransitUI::Amber());
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(StatusText)) S->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
}

void UMCEquipPanel::BindButtons()
{
	Tabs = { DeviceTab0, DeviceTab1, DeviceTab2, DeviceTab3 };
	TabLabels = { DeviceTabLabel0, DeviceTabLabel1, DeviceTabLabel2, DeviceTabLabel3 };
	Steps = { StepButton0, StepButton1, StepButton2 };
	StepLabels = { StepLabel0, StepLabel1, StepLabel2 };
	for (int32 i = 0; i < Tabs.Num(); ++i) BindButton(Tabs[i], MCTransit::MakeCode(EMCTransitRole::Equipment, (int32)EMCEquipAction::SelectDevice0 + i));
	for (int32 i = 0; i < Steps.Num(); ++i) BindButton(Steps[i], MCTransit::MakeCode(EMCTransitRole::Equipment, i));
}

void UMCEquipPanel::RefreshPanel()
{
	static const TCHAR* StateName[] = { TEXT(""), TEXT(" (testing)"), TEXT(" (PASS)") };
	for (int32 i = 0; i < Tabs.Num(); ++i)
	{
		const bool bHave = Manager->Devices.IsValidIndex(i);
		if (Tabs[i])
		{
			Tabs[i]->SetVisibility(bHave ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			Tabs[i]->SetBackgroundColor(bHave && i == Manager->ActiveDevice ? MCTransitUI::BtnOn() : MCTransitUI::BtnOff());
		}
		if (TabLabels[i] && bHave)
		{
			const FMCTransitDevice& D = Manager->Devices[i];
			TabLabels[i]->SetText(FText::FromString(D.Name + StateName[FMath::Clamp(D.State, 0, 2)]));
		}
	}

	const FMCTransitDevice* D = Manager->Devices.IsValidIndex(Manager->ActiveDevice) ? &Manager->Devices[Manager->ActiveDevice] : nullptr;
	if (ProcedureText)
	{
		FString S;
		if (D)
		{
			S = FString::Printf(TEXT("%s - procedure:\n"), *D->Name);
			for (int32 k = 0; k < D->Steps.Num(); ++k)
				S += FString::Printf(TEXT("%s %d. %s\n"), k < D->Progress ? TEXT("[x]") : TEXT("[ ]"), k + 1, *D->Steps[k]);
		}
		else S = TEXT("No devices to test.");
		ProcedureText->SetText(FText::FromString(S));
	}
	for (int32 i = 0; i < Steps.Num(); ++i)
	{
		const bool bHave = D && D->ButtonLabels.IsValidIndex(i);
		if (StepLabels[i] && bHave) StepLabels[i]->SetText(FText::FromString(D->ButtonLabels[i]));
		if (Steps[i]) Steps[i]->SetIsEnabled(bHave && D->State == 0);
	}
	if (StatusText)
	{
		FString S;
		if (D) S = D->State == 1 ? TEXT("Testing...") : D->State == 2 ? TEXT("PASS") : TEXT("Needs MECHANISM RAIL ON (ask Systems).");
		StatusText->SetText(FText::FromString(S));
		StatusText->SetColorAndOpacity(FSlateColor(D && D->State == 2 ? MCTransitUI::Green() : MCTransitUI::Amber()));
	}
}
