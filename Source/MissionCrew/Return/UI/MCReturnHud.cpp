#include "Return/UI/MCReturnHud.h"
#include "Return/MCReturnManager.h"
#include "Return/MCReturnTypes.h"
#include "Return/MCMissionSummary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void UMCReturnClickRelay::Fire() { if (UMCReturnHudWidget* H = Hud.Get()) H->Click(Code); }

void UMCReturnHudWidget::Click(int32 Code)
{
	APlayerController* PC = GetOwningPlayer();
	if (Code == -1) { UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/MissionCrew/Maps/L_MainMenu"))); return; }
	if (!PC || !Mgr) return;
	const int32 Who = AMCReturnManager::GetCrewRoleOf(PC);
	if (Mgr->HasAuthority()) Mgr->HandleAction(Code, Who);
	else if (AMCReturnLink* L = AMCReturnLink::FindFor(GetWorld(), PC)) L->ServerAction(Code);
}

UTextBlock* UMCReturnHudWidget::MakeText(int32 Size, FLinearColor C)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo F = T->GetFont();
	F.Size = Size;
	T->SetFont(F);
	T->SetColorAndOpacity(FSlateColor(C));
	return T;
}

UButton* UMCReturnHudWidget::AddButton(UVerticalBox* VB, const FString& Label, int32 Code)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* L = MakeText(16, FLinearColor::White);
	L->SetText(FText::FromString(Label));
	L->SetMinDesiredWidth(300.f);
	L->SetJustification(ETextJustify::Center);
	B->AddChild(L);
	UMCReturnClickRelay* R = NewObject<UMCReturnClickRelay>(this);
	R->Hud = this;
	R->Code = Code;
	Relays.Add(R);
	B->OnClicked.AddDynamic(R, &UMCReturnClickRelay::Fire);
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(B)) S->SetPadding(FMargin(0.f, 3.f));
	return B;
}

void UMCReturnHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree) return;
	const FLinearColor Cyan(0.25f, 0.88f, 1.f), Amber(1.f, 0.75f, 0.2f), Dim(0.6f, 0.7f, 0.8f);
	const FLinearColor Navy(0.02f, 0.05f, 0.1f, 0.88f);
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UBorder* Top = WidgetTree->ConstructWidget<UBorder>();
	Top->SetBrushColor(Navy);
	Top->SetPadding(FMargin(14.f, 8.f));
	UVerticalBox* TV = WidgetTree->ConstructWidget<UVerticalBox>();
	Top->SetContent(TV);
	TopText = MakeText(20, Cyan);
	TV->AddChildToVerticalBox(TopText);
	MsgText = MakeText(16, Amber);
	TV->AddChildToVerticalBox(MsgText);
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Top)) { S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f)); S->SetOffsets(FMargin(0.f, 0.f, 0.f, 70.f)); S->SetAutoSize(true); }

	UBorder* Board = WidgetTree->ConstructWidget<UBorder>();
	Board->SetBrushColor(Navy);
	Board->SetPadding(FMargin(12.f));
	BoardText = MakeText(14, Dim);
	Board->SetContent(BoardText);
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Board)) { S->SetAnchors(FAnchors(1.f, 0.f)); S->SetAlignment(FVector2D(1.f, 0.f)); S->SetPosition(FVector2D(-16.f, 90.f)); S->SetAutoSize(true); }

	UBorder* PBg = WidgetTree->ConstructWidget<UBorder>();
	PBg->SetBrushColor(Navy);
	PBg->SetPadding(FMargin(12.f));
	PanelBox = WidgetTree->ConstructWidget<UVerticalBox>();
	PBg->SetContent(PanelBox);
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(PBg)) { S->SetAnchors(FAnchors(0.f, 1.f)); S->SetAlignment(FVector2D(0.f, 1.f)); S->SetPosition(FVector2D(16.f, -16.f)); S->SetAutoSize(true); }

	UBorder* SBg = WidgetTree->ConstructWidget<UBorder>();
	SBg->SetBrushColor(FLinearColor(0.01f, 0.03f, 0.07f, 0.96f));
	SBg->SetPadding(FMargin(30.f));
	SummaryBox = WidgetTree->ConstructWidget<UVerticalBox>();
	SBg->SetContent(SummaryBox);
	SummaryText = MakeText(18, FLinearColor::White);
	SummaryBox->AddChildToVerticalBox(SummaryText);
	AddButton(SummaryBox, TEXT("BACK TO MAIN MENU"), -1);
	SBg->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(SBg)) { S->SetAnchors(FAnchors(0.5f, 0.5f)); S->SetAlignment(FVector2D(0.5f, 0.5f)); S->SetAutoSize(true); }
}

void UMCReturnHudWidget::BuildPanel(int32 Role)
{
	PanelBox->ClearChildren();
	Shown = Role;
	static const TCHAR* Titles[] = { TEXT(""), TEXT("SYSTEMS OPERATOR"), TEXT("DATA SPECIALIST"), TEXT("PILOT"), TEXT("RECOVERY SPECIALIST") };
	UTextBlock* T = MakeText(22, FLinearColor(0.25f, 0.88f, 1.f));
	T->SetText(FText::FromString(Titles[FMath::Clamp(Role, 0, 4)]));
	PanelBox->AddChildToVerticalBox(T);
	PanelBody = MakeText(14, FLinearColor(0.8f, 0.9f, 1.f));
	PanelBox->AddChildToVerticalBox(PanelBody);
	auto A = [&](const TCHAR* L, int32 Act) { AddButton(PanelBox, L, MCReturn::MakeCode(Role, Act)); };
	switch (Role)
	{
	case 1: A(TEXT("COOLANT LOOP"), 0); A(TEXT("POWER BUS B"), 1); A(TEXT("THRUSTER VALVE"), 2); A(TEXT("ANTENNA HEATER"), 3); break;
	case 2: A(TEXT("CHECK DATA"), MCReturn::DataCheck); A(TEXT("RE-FETCH SET 1"), 1); A(TEXT("RE-FETCH SET 2"), 2); A(TEXT("RE-FETCH SET 3"), 3);
		A(TEXT("QUEUE REPORT"), MCReturn::DataQueue); A(TEXT("SEND REPORT"), MCReturn::DataSend); break;
	case 3: A(TEXT("STEEPER (+angle)"), MCReturn::PilotSteeper); A(TEXT("SHALLOWER (-angle)"), MCReturn::PilotShallower);
		A(TEXT("LEFT"), MCReturn::PilotLeft); A(TEXT("RIGHT"), MCReturn::PilotRight); A(TEXT("DEPLOY CHUTES"), MCReturn::PilotChutes); break;
	case 4: A(TEXT("ACTIVATE BEACON"), MCReturn::RecBeacon); A(TEXT("CHECK HEAT SHIELD LATCH"), 1); A(TEXT("CHECK PARACHUTE PACK"), 2);
		A(TEXT("CHECK LOCATOR RADIO"), 3); A(TEXT("SECURE FAULT"), MCReturn::RecSecure); A(TEXT("PREPARE SAMPLES"), MCReturn::RecPrepare);
		A(TEXT("HAND OVER SAMPLES"), MCReturn::RecHandover); break;
	default: break;
	}
}

void UMCReturnHudWidget::NativeTick(const FGeometry& G, float Dt)
{
	Super::NativeTick(G, Dt);
	if (!Mgr)
	{
		FindTimer -= Dt;
		if (FindTimer <= 0.f) { FindTimer = 0.5f; Mgr = AMCReturnManager::Find(this); }
		if (!Mgr) return;
	}
	const int32 Real = AMCReturnManager::GetCrewRoleOf(GetOwningPlayer());
	const int32 Want = Real != 0 ? Real : 1;
	if (Want != Shown && PanelBox) BuildPanel(Want);
	TopText->SetText(FText::FromString(Mgr->TopText));
	MsgText->SetText(FText::FromString(Mgr->Message));
	BoardText->SetText(FText::FromString(Mgr->BoardText));
	if (PanelBody && Mgr->PanelText.IsValidIndex(Want)) PanelBody->SetText(FText::FromString(Mgr->PanelText[Want]));
	if (Mgr->bDone && !bSummaryShown && Mgr->Summary.bValid)
	{
		bSummaryShown = true;
		const FMCSummaryData& D = Mgr->Summary;
		FString S = FString::Printf(TEXT("MISSION COMPLETE\nFINAL SCORE  %d     TOTAL TIME  %s     SEED  %d\n\n"), D.TotalScore, *UMCMissionSummary::FormatTime(D.TotalSec), D.Seed);
		for (const FMCSummaryStage& St : D.Stages)
		{
			const FString Tm = St.bPlayed ? UMCMissionSummary::FormatTime(St.DurationSec) : FString(TEXT("--"));
			const FString Sc = St.bPlayed ? FString::FromInt(St.Score) : FString(TEXT("--"));
			S += FString::Printf(TEXT("%s   %s   %s\n   Lesson: %s\n"), *St.Name, *Tm, *Sc, *St.Lesson);
		}
		S += TEXT("\nPLAYER ACCURACY\n");
		for (const FMCSummaryPlayer& P : D.Players)
		{
			const FString Acc = P.AccuracyPct >= 0.f ? FString::Printf(TEXT("%.0f%%  (%d/%d)"), P.AccuracyPct, P.Good, P.Total) : FString(TEXT("no data"));
			S += FString::Printf(TEXT("%s (%s)   %s\n"), *P.Name, *UMCMissionSummary::RoleLabel(P.Role), *Acc);
		}
		SummaryText->SetText(FText::FromString(S));
		if (UWidget* P = SummaryBox->GetParent()) P->SetVisibility(ESlateVisibility::Visible);
		if (UWidget* P = PanelBox->GetParent()) P->SetVisibility(ESlateVisibility::Collapsed);
	}
}
