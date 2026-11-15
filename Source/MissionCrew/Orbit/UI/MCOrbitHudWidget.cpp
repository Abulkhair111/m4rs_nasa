#include "Orbit/UI/MCOrbitHudWidget.h"
#include "Orbit/UI/MCOrbitPanels.h"
#include "Orbit/MCOrbitManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UMCOrbitHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!SystemsPanelClass) SystemsPanelClass = UMCOrbitSystemsPanel::StaticClass();
	if (!SafetyPanelClass) SafetyPanelClass = UMCOrbitSafetyPanel::StaticClass();
	if (!PilotPanelClass) PilotPanelClass = UMCOrbitPilotPanel::StaticClass();
	if (!AnalystPanelClass) AnalystPanelClass = UMCOrbitAnalystPanel::StaticClass();
	if (WidgetTree && !WidgetTree->RootWidget) BuildLayout();
}

void UMCOrbitHudWidget::BuildLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HudCanvas"));
	WidgetTree->RootWidget = Canvas;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto MakeText = [&](const TCHAR* Name, int32 Size, FLinearColor C) {
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo F = T->GetFont(); F.Size = Size; T->SetFont(F); T->SetColorAndOpacity(FSlateColor(C));
		return T;
	};
	UBorder* Top = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TopBar"));
	Top->SetBrushColor(MCOrbitUI::Navy()); Top->SetPadding(FMargin(14.f, 8.f));
	UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Top->SetContent(VB);
	TopText = MakeText(TEXT("TopText"), 20, MCOrbitUI::Cyan()); VB->AddChildToVerticalBox(TopText);
	MessageText = MakeText(TEXT("MessageText"), 16, MCOrbitUI::Amber()); VB->AddChildToVerticalBox(MessageText);
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Top)) { S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f)); S->SetOffsets(FMargin(0.f, 0.f, 0.f, 70.f)); S->SetAutoSize(true); }

	UBorder* Board = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BoardBg"));
	Board->SetBrushColor(MCOrbitUI::Navy()); Board->SetPadding(FMargin(12.f));
	BoardText = MakeText(TEXT("BoardText"), 14, MCOrbitUI::Dim()); BoardText->SetAutoWrapText(true); BoardText->SetWrapTextAt(340.f);
	Board->SetContent(BoardText);
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Board)) { S->SetAnchors(FAnchors(1.f, 0.f)); S->SetAlignment(FVector2D(1.f, 0.f)); S->SetPosition(FVector2D(-16.f, 90.f)); S->SetAutoSize(true); }

	UVerticalBox* Left = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftVB"));
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Left)) { S->SetAnchors(FAnchors(0.f, 1.f)); S->SetAlignment(FVector2D(0.f, 1.f)); S->SetPosition(FVector2D(16.f, -16.f)); S->SetAutoSize(true); }
	TabBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RoleTabs"));
	TabBox->SetVisibility(ESlateVisibility::Collapsed);
	Left->AddChildToVerticalBox(TabBox);
	static const TCHAR* Names[] = { TEXT("SYSTEMS"), TEXT("SAFETY"), TEXT("PILOT"), TEXT("ANALYST") };
	for (int32 i = 0; i < 4; ++i)
	{
		UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		B->SetBackgroundColor(FLinearColor(0.10f, 0.45f, 0.55f, 1.f));
		UTextBlock* L = MakeText(*FString::Printf(TEXT("Tab%d"), i), 16, FLinearColor::White);
		L->SetText(FText::FromString(Names[i])); L->SetMinDesiredWidth(90.f); L->SetJustification(ETextJustify::Center);
		B->AddChild(L);
		UMCOrbitClickRelay* R = NewObject<UMCOrbitClickRelay>(this);
		R->Owner = this; R->Mode = 1; R->Code = i + 1; Relays.Add(R);
		B->OnClicked.AddDynamic(R, &UMCOrbitClickRelay::Fire);
		if (UHorizontalBoxSlot* S = TabBox->AddChildToHorizontalBox(B)) S->SetPadding(FMargin(2.f));
	}
	UVerticalBox* Host = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelHost"));
	PanelHost = Host;
	Left->AddChildToVerticalBox(Host);
}

void UMCOrbitHudWidget::BuildPanel(int32 Role)
{
	if (Panel) { Panel->RemoveFromParent(); Panel = nullptr; }
	ShownRole = Role;
	TSubclassOf<UMCOrbitPanelBase> Cls;
	switch (Role) { case 1: Cls = SystemsPanelClass; break; case 2: Cls = SafetyPanelClass; break; case 3: Cls = PilotPanelClass; break; case 4: Cls = AnalystPanelClass; break; default: break; }
	APlayerController* PC = GetOwningPlayer();
	if (!Cls || !PC || !PanelHost) return;
	Panel = CreateWidget<UMCOrbitPanelBase>(PC, Cls);
	if (Panel) { Panel->Role = Role; PanelHost->AddChild(Panel); }
}

void UMCOrbitHudWidget::NativeTick(const FGeometry& G, float Dt)
{
	Super::NativeTick(G, Dt);
	if (!Manager)
	{
		FindTimer -= Dt;
		if (FindTimer <= 0.f && GetWorld())
		{
			FindTimer = 0.5f;
			for (TActorIterator<AMCOrbitManager> It(GetWorld()); It; ++It) { Manager = *It; break; }
		}
		if (!Manager) return;
	}
	const int32 Real = AMCOrbitManager::GetCrewRoleOfController(GetOwningPlayer());
	const int32 Want = Real != 0 ? Real : FMath::Max(1, TestRole);
	if (TabBox) TabBox->SetVisibility(Real == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (Want != ShownRole) BuildPanel(Want);
	if (TopText) TopText->SetText(FText::FromString(Manager->TopText));
	if (BoardText) BoardText->SetText(FText::FromString(Manager->BoardText));
	if (MessageText)
	{
		if (Manager->Message != LastMessage) { LastMessage = Manager->Message; MessageTime = FPlatformTime::Seconds(); }
		const bool bShow = FPlatformTime::Seconds() - MessageTime < MessageSeconds;
		MessageText->SetText(FText::FromString(bShow ? LastMessage : FString()));
	}
}
