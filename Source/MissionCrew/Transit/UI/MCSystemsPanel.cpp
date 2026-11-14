#include "Transit/UI/MCSystemsPanel.h"
#include "Transit/MCTransitManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UMCSystemsPanel::BuildFallbackLayout()
{
	UVerticalBox* VB = MakeRootBox(TEXT("SYSTEMS CONSOLE"), MCTransitUI::Cyan());
	for (int32 i = 0; i < MCTransit::NumSwitches; ++i)
	{
		UTextBlock* L = nullptr;
		UButton* B = MakeButton(*FString::Printf(TEXT("SwitchButton%d"), i), TEXT("-"), L, 320.f);
		L->Rename(*FString::Printf(TEXT("SwitchLabel%d"), i));
		if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(B)) S->SetPadding(FMargin(0.f, 3.f));
		switch (i)
		{
		case 0: SwitchButton0 = B; SwitchLabel0 = L; break;
		case 1: SwitchButton1 = B; SwitchLabel1 = L; break;
		case 2: SwitchButton2 = B; SwitchLabel2 = L; break;
		case 3: SwitchButton3 = B; SwitchLabel3 = L; break;
		case 4: SwitchButton4 = B; SwitchLabel4 = L; break;
		default: SwitchButton5 = B; SwitchLabel5 = L; break;
		}
	}
	UTextBlock* H = MakeText(TEXT("AlarmHeader"), 16, MCTransitUI::Amber());
	H->SetText(FText::FromString(TEXT("ALARMS")));
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(H)) S->SetPadding(FMargin(0.f, 10.f, 0.f, 2.f));
	AlarmText = MakeText(TEXT("AlarmText"), 14, MCTransitUI::Amber());
	AlarmText->SetAutoWrapText(true);
	AlarmText->SetWrapTextAt(320.f);
	VB->AddChildToVerticalBox(AlarmText);
}

void UMCSystemsPanel::BindButtons()
{
	Buttons = { SwitchButton0, SwitchButton1, SwitchButton2, SwitchButton3, SwitchButton4, SwitchButton5 };
	Labels = { SwitchLabel0, SwitchLabel1, SwitchLabel2, SwitchLabel3, SwitchLabel4, SwitchLabel5 };
	for (int32 i = 0; i < Buttons.Num(); ++i) BindButton(Buttons[i], MCTransit::MakeCode(EMCTransitRole::Systems, i));
}

void UMCSystemsPanel::RefreshPanel()
{
	for (int32 i = 0; i < Buttons.Num(); ++i)
	{
		const bool bHave = Manager->Switches.IsValidIndex(i);
		if (Labels[i] && bHave) Labels[i]->SetText(FText::FromString(Manager->Switches[i].Label));
		if (Buttons[i])
		{
			Buttons[i]->SetBackgroundColor(bHave && Manager->Switches[i].bOn ? MCTransitUI::BtnOn() : MCTransitUI::BtnOff());
			Buttons[i]->SetIsEnabled(bHave);
		}
	}
	if (AlarmText)
	{
		FString S;
		for (const FMCTransitAlarm& A : Manager->Alarms)
		{
			if (A.bFixed) S += FString::Printf(TEXT("ok  %s\n"), *A.Text);
			else if (A.bActive) S += FString::Printf(TEXT("!  %s\n"), *A.Text);
		}
		if (S.IsEmpty()) S = TEXT("No alarms.");
		AlarmText->SetText(FText::FromString(S));
	}
}
