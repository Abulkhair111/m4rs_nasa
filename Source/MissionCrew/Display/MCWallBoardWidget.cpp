#include "Display/MCWallBoardWidget.h"
#include "Display/MCDisplayHelpers.h"
#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

namespace
{
	const TCHAR* GBoardCheckLabels[MCConst::NumChecks] = {
		TEXT("Launcher chosen"), TEXT("Mass vs payload"), TEXT("Budget"), TEXT("Power balance"), TEXT("Comms link"),
		TEXT("Fuel"), TEXT("Thrust (T/W)"), TEXT("Ice science"), TEXT("Power grid"), TEXT("Structure (bolts)")
	};
	const TCHAR* GBoardRoleLabels[MCConst::NumRoles] = { TEXT("Solo"), TEXT("Power"), TEXT("Construction"), TEXT("Propulsion"), TEXT("Science") };
}

void UMCWallBoardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree) return;
	if (!WidgetTree->RootWidget && !HeaderText)
	{
		BuildFallbackLayout();
	}
	else
	{
		for (int32 i = 0; i < MCConst::NumChecks; ++i)
		{
			UTextBlock* T = Cast<UTextBlock>(WidgetTree->FindWidget(*FString::Printf(TEXT("CheckText_%d"), i)));
			UBorder* B = Cast<UBorder>(WidgetTree->FindWidget(*FString::Printf(TEXT("CheckSwatch_%d"), i)));
			CheckTexts.Add(T);
			CheckSwatches.Add(B);
		}
	}
}

void UMCWallBoardWidget::BuildFallbackLayout()
{
	UBorder* Panel = MCDisplay::MakePanel(WidgetTree, TEXT("BoardPanel"));
	WidgetTree->RootWidget = Panel;

	UHorizontalBox* Cols = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Cols"));
	Panel->SetContent(Cols);

	// Left: the 10 checks.
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftCol"));
	if (UHorizontalBoxSlot* S = Cols->AddChildToHorizontalBox(Left)) { S->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); S->SetPadding(FMargin(0, 0, 40, 0)); }
	UTextBlock* Title = MCDisplay::MakeText(WidgetTree, TEXT("Title"), 54, FLinearColor(0.6f, 0.8f, 1.f));
	Title->SetText(FText::FromString(TEXT("PREFLIGHT CHECKS")));
	Left->AddChildToVerticalBox(Title);
	for (int32 i = 0; i < MCConst::NumChecks; ++i)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UBorder* Sw = nullptr;
		USizeBox* SwBox = MCDisplay::MakeSwatch(WidgetTree, *FString::Printf(TEXT("CheckSwatch_%d"), i), 58.f, Sw);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(SwBox)) { S->SetVerticalAlignment(VAlign_Center); S->SetPadding(FMargin(0, 0, 24, 0)); }
		UTextBlock* T = MCDisplay::MakeText(WidgetTree, *FString::Printf(TEXT("CheckText_%d"), i), 50);
		T->SetText(FText::FromString(GBoardCheckLabels[i]));
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(T)) S->SetVerticalAlignment(VAlign_Center);
		if (UVerticalBoxSlot* S = Left->AddChildToVerticalBox(Row)) S->SetPadding(FMargin(0, 6));
		CheckTexts.Add(T);
		CheckSwatches.Add(Sw);
	}

	// Right: header, failing, accuracy, message.
	UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightCol"));
	if (UHorizontalBoxSlot* S = Cols->AddChildToHorizontalBox(Right)) S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	HeaderText = MCDisplay::MakeText(WidgetTree, TEXT("HeaderText"), 54);
	if (UVerticalBoxSlot* S = Right->AddChildToVerticalBox(HeaderText)) S->SetPadding(FMargin(0, 0, 0, 20));
	FailText = MCDisplay::MakeText(WidgetTree, TEXT("FailText"), 44, FLinearColor(1.f, 0.4f, 0.4f), true);
	if (UVerticalBoxSlot* S = Right->AddChildToVerticalBox(FailText)) S->SetPadding(FMargin(0, 0, 0, 20));
	AccuracyText = MCDisplay::MakeText(WidgetTree, TEXT("AccuracyText"), 44, FLinearColor(0.85f, 0.85f, 0.85f));
	if (UVerticalBoxSlot* S = Right->AddChildToVerticalBox(AccuracyText)) S->SetPadding(FMargin(0, 0, 0, 20));
	MessageText = MCDisplay::MakeText(WidgetTree, TEXT("MessageText"), 44, FLinearColor(1.f, 0.9f, 0.5f), true);
	Right->AddChildToVerticalBox(MessageText);
}

void UMCWallBoardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Timer -= InDeltaTime;
	if (Timer > 0.f) return;
	Timer = RefreshInterval;
	if (!Manager) Manager = MCBayUI::FindManager(GetWorld());
	Refresh();
}

void UMCWallBoardWidget::Refresh()
{
	const AMCConstructionManager* M = Manager;
	if (!M)
	{
		if (HeaderText) HeaderText->SetText(FText::FromString(TEXT("No construction manager")));
		return;
	}
	for (int32 i = 0; i < CheckTexts.Num() && i < MCConst::NumChecks; ++i)
	{
		const EMCCheckState S = M->Checks.IsValidIndex(i) ? M->Checks[i] : EMCCheckState::Red;
		const FLinearColor C = MCBayUI::CheckColor(S);
		if (CheckTexts[i])
		{
			CheckTexts[i]->SetText(FText::FromString(FString::Printf(TEXT("%s  %s"), GBoardCheckLabels[i], *MCDisplay::CheckStateTag(S))));
			CheckTexts[i]->SetColorAndOpacity(FSlateColor(C));
		}
		if (CheckSwatches.IsValidIndex(i) && CheckSwatches[i]) CheckSwatches[i]->SetBrushColor(C);
	}
	if (HeaderText)
	{
		HeaderText->SetText(FText::FromString(FString::Printf(TEXT("Thrust %.1f kN\nT/W %.2f\nTanks %.0f / %.0f kg\nIce detectors %d"),
			M->ThrustN / 1000.f, M->TWR, M->FuelKg, M->FuelMaxKg, M->IceCount)));
		HeaderText->SetColorAndOpacity(FSlateColor(MCBayUI::TWRColor((M->bHasEngine || M->BoosterCount > 0) ? M->TWR : 0.f)));
	}
	if (FailText)
	{
		FString F;
		for (int32 i = 0; i < MCConst::NumChecks && M->Checks.IsValidIndex(i); ++i)
		{
			if (M->Checks[i] == EMCCheckState::Red) F += FString::Printf(TEXT("%s%s"), F.IsEmpty() ? TEXT("") : TEXT(", "), GBoardCheckLabels[i]);
		}
		FailText->SetText(FText::FromString(F.IsEmpty() ? FString(TEXT("No failing checks")) : FString(TEXT("FAILING: ")) + F));
		FailText->SetColorAndOpacity(FSlateColor(MCBayUI::CheckColor(F.IsEmpty() ? EMCCheckState::Green : EMCCheckState::Red)));
	}
	if (AccuracyText)
	{
		FString A = TEXT("ACCURACY\n");
		for (int32 r = 1; r < MCConst::NumRoles; ++r)
		{
			const int32 G = M->RoleGood.IsValidIndex(r) ? M->RoleGood[r] : 0;
			const int32 B = M->RoleBad.IsValidIndex(r) ? M->RoleBad[r] : 0;
			A += FString::Printf(TEXT("%s  %d / %d\n"), GBoardRoleLabels[r], G, G + B);
		}
		AccuracyText->SetText(FText::FromString(A.TrimEnd()));
	}
	if (MessageText)
	{
		const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
		const double Now = GS ? GS->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);
		const bool bShow = !M->Message.IsEmpty() && (Now - M->MessageTime) < MessageSeconds;
		MessageText->SetText(FText::FromString(bShow ? M->Message : FString()));
	}
}
