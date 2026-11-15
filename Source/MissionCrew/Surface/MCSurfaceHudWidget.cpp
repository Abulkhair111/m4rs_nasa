#include "Surface/MCSurfaceHudWidget.h"
#include "Surface/MCSurfaceLink.h"
#include "Surface/MCSurfaceManager.h"
#include "Transit/MCTransitManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"

void UMCSurfaceClickRelay::Fire() { if (AMCSurfaceLink* L = Link.Get()) { L->ServerAction(Code); } }

UTextBlock* UMCSurfaceHudWidget::AddText(UVerticalBox* Box, int32 Size, FLinearColor Col)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo F = T->GetFont(); F.Size = Size; T->SetFont(F);
	T->SetColorAndOpacity(FSlateColor(Col));
	T->SetAutoWrapText(true);
	Box->AddChildToVerticalBox(T);
	return T;
}

void UMCSurfaceHudWidget::AddButtons(UVerticalBox* Box, const TArray<TPair<FString, int32>>& Defs)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	Box->AddChildToVerticalBox(Row);
	for (const TPair<FString, int32>& D : Defs)
	{
		UButton* B = WidgetTree->ConstructWidget<UButton>();
		UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>();
		L->SetText(FText::FromString(D.Key));
		B->SetContent(L);
		UMCSurfaceClickRelay* R = NewObject<UMCSurfaceClickRelay>(this);
		R->Link = Link; R->Code = D.Value;
		Relays.Add(R);
		B->OnClicked.AddDynamic(R, &UMCSurfaceClickRelay::Fire);
		Row->AddChildToHorizontalBox(B);
	}
}

UVerticalBox* UMCSurfaceHudWidget::AddPanel(UVerticalBox* Root, const TCHAR* Title)
{
	UBorder* Bg = WidgetTree->ConstructWidget<UBorder>();
	Bg->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.10f, 0.85f));
	Bg->SetPadding(FMargin(12.f));
	UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>();
	Bg->SetContent(VB);
	Root->AddChildToVerticalBox(Bg);
	AddText(VB, 20, FLinearColor(0.25f, 0.88f, 1.f))->SetText(FText::FromString(Title));
	return VB;
}

void UMCSurfaceHudWidget::Build()
{
	const int32 Role = Link ? AMCTransitManager::GetCrewRoleOfController(Cast<AController>(Link->GetOwner())) : 0;
	BuiltRole = Role;
	Relays.Reset();
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	UVerticalBox* Top = WidgetTree->ConstructWidget<UVerticalBox>();
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Top)) { S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f)); S->SetOffsets(FMargin(10.f, 10.f, 10.f, 0.f)); S->SetAutoSize(true); }
	TopText = AddText(Top, 18, FLinearColor::White);
	MsgText = AddText(Top, 16, FLinearColor(1.f, 0.75f, 0.2f));
	UVerticalBox* Side = WidgetTree->ConstructWidget<UVerticalBox>();
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Side)) { S->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f)); S->SetAlignment(FVector2D(1.f, 1.f)); S->SetPosition(FVector2D(-10.f, -10.f)); S->SetAutoSize(true); }
	const auto C = [](int32 R, int32 A) { return R * 100 + A; };
	PowerText = SciText = DriveText = DrillText = nullptr;
	if (Role == 0 || Role == 1)
	{
		UVerticalBox* P = AddPanel(Side, TEXT("POWER OPERATOR"));
		PowerText = AddText(P, 15, FLinearColor::White);
		AddButtons(P, { {TEXT("Comms/Lights"), C(1,1)}, {TEXT("Instruments"), C(1,2)}, {TEXT("Drill +"), C(1,3)}, {TEXT("Drill -"), C(1,4)} });
	}
	if (Role == 0 || Role == 2)
	{
		UVerticalBox* P = AddPanel(Side, TEXT("SCIENCE ANALYST"));
		SciText = AddText(P, 15, FLinearColor::White);
		AddButtons(P, { {TEXT("Site A"), C(2,1)}, {TEXT("Site B"), C(2,2)}, {TEXT("Site C"), C(2,3)}, {TEXT("Site D"), C(2,4)}, {TEXT("Rescan"), C(2,5)} });
	}
	if (Role == 0 || Role == 3)
	{
		UVerticalBox* P = AddPanel(Side, TEXT("ROVER DRIVER (W/S throttle, A/D steer)"));
		DriveText = AddText(P, 15, FLinearColor::White);
		AddButtons(P, { {TEXT("STOP"), C(3,5)} });
	}
	if (Role == 0 || Role == 4)
	{
		UVerticalBox* P = AddPanel(Side, TEXT("DRILL OPERATOR"));
		DrillText = AddText(P, 15, FLinearColor::White);
		AddButtons(P, { {TEXT("Depth 1"), C(4,1)}, {TEXT("Depth 2"), C(4,2)}, {TEXT("Depth 3"), C(4,3)}, {TEXT("START"), C(4,4)}, {TEXT("EXTRACT"), C(4,5)} });
	}
}

void UMCSurfaceHudWidget::NativeTick(const FGeometry& G, float Dt)
{
	Super::NativeTick(G, Dt);
	AMCSurfaceManager* M = Link ? Link->GetManager() : nullptr;
	if (!M || !Link) { return; }
	APlayerController* PC = GetOwningPlayer();
	const int32 Role = AMCTransitManager::GetCrewRoleOfController(PC);
	if (Role != BuiltRole) { Build(); }

	if ((Role == 3 || (Role == 0 && M->bFreeRoles)) && PC)
	{
		const float T = (PC->IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (PC->IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
		const float S = (PC->IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (PC->IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
		if (T != LastT || S != LastS) { LastT = T; LastS = S; Link->ServerDrive(T, S); }
	}

	Timer -= Dt;
	if (Timer > 0.f) { return; }
	Timer = 0.1f;
	const auto Set = [](UTextBlock* T, const FString& S) { if (T) { T->SetText(FText::FromString(S)); } };
	Set(TopText, FString::Printf(TEXT("SURFACE | T+ %.0f / %.0f s | SEED %d | SAMPLES %d | SCORE %d%s"), M->ElapsedSec, M->TimeoutSec, M->Seed, M->Samples.Num(), M->Score, M->bDone ? TEXT(" | DONE") : TEXT("")));
	Set(MsgText, M->Message);
	FString Pw = FString::Printf(TEXT("Battery %.0f / %.0f Wh   Supply %.0f W   Demand %.0f W\nSolar %.0f W  Used %.1f Wh\nComms %s  Instruments %s  Drill level %d"),
		M->BatteryWh, M->CapacityWh, M->SupplyW, M->DemandW, M->SolarNowW, M->PowerUsedWh, M->bCommsOn ? TEXT("ON") : TEXT("off"), M->bInstrumentsOn ? TEXT("ON") : TEXT("off"), M->DrillPowerLevel);
	if (M->bPowerEvent) { Pw += FString::Printf(TEXT("\nPOWER DROP %.0f s: cut non-essential loads!"), M->EventLeftSec); }
	if (M->bBrownout) { Pw += TEXT("\nBROWNOUT"); }
	Set(PowerText, Pw);
	FString Sci = FString::Printf(TEXT("Chosen site: %s\n"), M->Sites.IsValidIndex(M->ChosenSite) ? *FString::Chr((TCHAR)(TEXT('A') + M->ChosenSite)) : TEXT("none"));
	for (int32 i = 0; i < M->Sites.Num(); ++i)
	{
		Sci += FString::Printf(TEXT("%c: hydrogen %.0f  temp %.0f K  scans %d\n"), (TCHAR)(TEXT('A') + i), M->Sites[i].Hydrogen, M->Sites[i].TempK, M->Sites[i].Scans);
	}
	Set(SciText, Sci);
	FString At = TEXT("no");
	if (M->Sites.IsValidIndex(M->RoverAtSite)) { At = FString::Chr((TCHAR)(TEXT('A') + M->RoverAtSite)); }
	Set(DriveText, FString::Printf(TEXT("Pos (%.0f, %.0f) cm  heading %.0f  rock hits %d  at site: %s"), M->RoverPos.X, M->RoverPos.Y, M->RoverYaw, M->RockHits, *At));
	Set(DrillText, FString::Printf(TEXT("Depth level %d  %s  progress %.0f%%%s%s"), M->DepthLevel, M->bDrilling ? TEXT("DRILLING") : TEXT("idle"), M->DrillProgress * 100.f,
		M->bDrillPaused ? TEXT("  PAUSED: no power") : TEXT(""), M->bSampleReady ? TEXT("  SAMPLE READY") : TEXT("")));
}
