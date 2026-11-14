#include "Transit/UI/MCTransitHudWidget.h"
#include "Transit/UI/MCTransitPanelBase.h"
#include "Transit/UI/MCSystemsPanel.h"
#include "Transit/UI/MCEquipPanel.h"
#include "Transit/UI/MCPilotPanel.h"
#include "Transit/UI/MCSciencePanel.h"
#include "Transit/MCTransitManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UMCTransitHudRelay::Fire()
{
	UMCTransitHudWidget* H = Hud.Get();
	if (!H) return;
	if (Mode == 1) H->SetTestRole(Code);
	else MCTransitUI::SendToPawn(H->GetOwningPlayer(), Code, nullptr);
}

void UMCTransitHudWidget::BindClick(UButton* Button, int32 Mode, int32 Code)
{
	if (!Button) return;
	UMCTransitHudRelay* R = NewObject<UMCTransitHudRelay>(this);
	R->Hud = this;
	R->Mode = Mode;
	R->Code = Code;
	Relays.Add(R);
	Button->OnClicked.AddDynamic(R, &UMCTransitHudRelay::Fire);
}

void UMCTransitHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!SystemsPanelClass) SystemsPanelClass = UMCSystemsPanel::StaticClass();
	if (!EquipPanelClass) EquipPanelClass = UMCEquipPanel::StaticClass();
	if (!PilotPanelClass) PilotPanelClass = UMCPilotPanel::StaticClass();
	if (!SciencePanelClass) SciencePanelClass = UMCSciencePanel::StaticClass();
	if (WidgetTree && !WidgetTree->RootWidget) BuildFallbackLayout();
	BindClick(GoButton, 0, MCTransit::GoCode);
}

void UMCTransitHudWidget::BuildFallbackLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HudCanvas"));
	WidgetTree->RootWidget = Canvas;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	auto MakeText = [&](const TCHAR* Name, int32 Size, FLinearColor Color) {
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		T->SetFont(F);
		T->SetColorAndOpacity(FSlateColor(Color));
		return T;
	};
	auto MakeButton = [&](const TCHAR* Name, const TCHAR* Label, float W, UTextBlock*& OutLabel) {
		UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		B->SetBackgroundColor(MCTransitUI::BtnOn());
		OutLabel = MakeText(*(FString(Name) + TEXT("_Label")), 18, FLinearColor::White);
		OutLabel->SetText(FText::FromString(Label));
		OutLabel->SetMinDesiredWidth(W);
		OutLabel->SetJustification(ETextJustify::Center);
		B->AddChild(OutLabel);
		return B;
	};

	// top bar + message (stretched across the top)
	UBorder* Top = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TopBar"));
	Top->SetBrushColor(MCTransitUI::Navy());
	Top->SetPadding(FMargin(14.f, 8.f));
	UVerticalBox* TopVB = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TopVB"));
	Top->SetContent(TopVB);
	TopText = MakeText(TEXT("TopText"), 20, MCTransitUI::Cyan());
	TopVB->AddChildToVerticalBox(TopText);
	MessageText = MakeText(TEXT("MessageText"), 16, MCTransitUI::Amber());
	TopVB->AddChildToVerticalBox(MessageText);
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Top))
	{
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		S->SetOffsets(FMargin(0.f, 0.f, 0.f, 70.f));
		S->SetAutoSize(true);
	}

	// mission board (top right, under the bar)
	UBorder* Board = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BoardBg"));
	Board->SetBrushColor(MCTransitUI::Navy());
	Board->SetPadding(FMargin(12.f));
	BoardText = MakeText(TEXT("BoardText"), 14, MCTransitUI::Dim());
	BoardText->SetAutoWrapText(true);
	BoardText->SetWrapTextAt(320.f);
	Board->SetContent(BoardText);
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Board))
	{
		S->SetAnchors(FAnchors(1.f, 0.f));
		S->SetAlignment(FVector2D(1.f, 0.f));
		S->SetPosition(FVector2D(-16.f, 90.f));
		S->SetAutoSize(true);
	}

	// panel host (bottom left) + role tabs for role 0
	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftVB"));
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Left))
	{
		S->SetAnchors(FAnchors(0.f, 1.f));
		S->SetAlignment(FVector2D(0.f, 1.f));
		S->SetPosition(FVector2D(16.f, -16.f));
		S->SetAutoSize(true);
	}
	UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RoleTabs"));
	TabBox = Tabs;
	Tabs->SetVisibility(ESlateVisibility::Collapsed);
	Left->AddChildToVerticalBox(Tabs);
	static const TCHAR* RoleNames[] = { TEXT("SYSTEMS"), TEXT("EQUIPMENT"), TEXT("PILOT"), TEXT("SCIENCE") };
	for (int32 i = 0; i < 4; ++i)
	{
		UTextBlock* L = nullptr;
		UButton* B = MakeButton(*FString::Printf(TEXT("RoleTab%d"), i), RoleNames[i], 90.f, L);
		BindClick(B, 1, i + 1);
		if (UHorizontalBoxSlot* S = Tabs->AddChildToHorizontalBox(B)) S->SetPadding(FMargin(2.f));
	}
	UVerticalBox* Host = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelHost"));
	PanelHost = Host;
	Left->AddChildToVerticalBox(Host);

	// GO FOR LAUNCH (bottom centre)
	UTextBlock* GoL = nullptr;
	UButton* Go = MakeButton(TEXT("GoButton"), TEXT("GO FOR LAUNCH"), 260.f, GoL);
	GoLabel = GoL;
	GoButton = Go;
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Go))
	{
		S->SetAnchors(FAnchors(0.5f, 1.f));
		S->SetAlignment(FVector2D(0.5f, 1.f));
		S->SetPosition(FVector2D(0.f, -40.f));
		S->SetAutoSize(true);
	}
}

void UMCTransitHudWidget::SetTestRole(int32 Role)
{
	TestRole = FMath::Clamp(Role, 0, 4);
}

void UMCTransitHudWidget::BuildPanel(int32 Role)
{
	if (Panel)
	{
		Panel->RemoveFromParent();
		Panel = nullptr;
	}
	ShownRole = Role;
	TSubclassOf<UMCTransitPanelBase> Cls;
	switch (Role)
	{
	case 1: Cls = SystemsPanelClass; break;
	case 2: Cls = EquipPanelClass; break;
	case 3: Cls = PilotPanelClass; break;
	case 4: Cls = SciencePanelClass; break;
	default: break;
	}
	APlayerController* PC = GetOwningPlayer();
	if (!Cls || !PC || !PanelHost) return;
	Panel = CreateWidget<UMCTransitPanelBase>(PC, Cls);
	if (Panel) PanelHost->AddChild(Panel);
}

void UMCTransitHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Manager)
	{
		FindTimer -= InDeltaTime;
		if (FindTimer <= 0.f)
		{
			FindTimer = 0.5f;
			if (UWorld* W = GetWorld())
			{
				for (TActorIterator<AMCTransitManager> It(W); It; ++It) { Manager = *It; break; }
			}
		}
		if (!Manager) return;
	}
	Refresh();
}

void UMCTransitHudWidget::Refresh()
{
	const AMCTransitManager& M = *Manager;
	const int32 Real = AMCTransitManager::GetCrewRoleOfController(GetOwningPlayer());
	// Role 0: free/solo roles show the chosen tab (default Systems); an assigned role is fixed.
	const int32 Want = Real != 0 ? Real : (TestRole != 0 ? TestRole : 1);
	if (Want != ShownRole || (!Panel && PanelHost)) BuildPanel(Want);
	if (TabBox) TabBox->SetVisibility(Real == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	const bool bPoll = M.Stage == EMCTransitStage::GoPoll;
	if (TopText)
	{
		FString S = FString::Printf(TEXT("%s"), MCTransitUI::StageName(M.Stage));
		if (!M.bStarted) S = TEXT("WAITING FOR LAUNCH");
		else if (M.Stage == EMCTransitStage::Coast)
			S += FString::Printf(TEXT("  |  T+ %.0f / %.0f s"), M.ElapsedSec, M.TransitDurationSec);
		else if (M.Stage == EMCTransitStage::Countdown || M.Stage == EMCTransitStage::Ascent)
			S += FString::Printf(TEXT("  |  %.0f s"), FMath::Max(0.f, M.GetStageTimeLeft()));
		S += FString::Printf(TEXT("  |  SEED %d  |  HEALTH %.0f  |  FUEL %.0f kg"), M.Seed, M.Health, M.FuelKg);
		TopText->SetText(FText::FromString(S));
	}
	if (MessageText)
	{
		const double Now = FPlatformTime::Seconds();
		if (M.Message != LastMessage) { LastMessage = M.Message; MessageTime = Now; }
		MessageText->SetText(FText::FromString(Now - MessageTime < MessageSeconds ? LastMessage : FString()));
	}
	if (BoardText)
		BoardText->SetText(FText::FromString(M.bDone && !M.ReportText.IsEmpty() ? M.ReportText : M.BoardText));

	if (GoButton)
	{
		const bool bShow = bPoll && M.bStarted;
		GoButton->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bShow && GoLabel)
		{
			FString S = TEXT("GO FOR LAUNCH   ");
			for (int32 r = 1; r <= 4; ++r)
				S += FString::Printf(TEXT("[%d%s] "), r, M.GoFlags.IsValidIndex(r) && M.GoFlags[r] ? TEXT("+") : TEXT(" "));
			GoLabel->SetText(FText::FromString(S));
		}
	}
}
