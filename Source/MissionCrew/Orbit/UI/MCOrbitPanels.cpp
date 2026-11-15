#include "Orbit/UI/MCOrbitPanels.h"
#include "Orbit/UI/MCOrbitHudWidget.h"
#include "Orbit/MCOrbitManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UMCOrbitClickRelay::Fire()
{
	if (Mode == 1) { if (UMCOrbitHudWidget* H = Cast<UMCOrbitHudWidget>(Owner.Get())) H->SetTestRole(Code); return; }
	if (UMCOrbitPanelBase* P = Cast<UMCOrbitPanelBase>(Owner.Get())) P->SendAction(Code);
}

static UTextBlock* NewText(UWidgetTree* T, const TCHAR* Name, int32 Size, FLinearColor C)
{
	UTextBlock* X = T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	FSlateFontInfo F = X->GetFont(); F.Size = Size; X->SetFont(F);
	X->SetColorAndOpacity(FSlateColor(C));
	return X;
}

void UMCOrbitPanelBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget) return;
	UBorder* Bg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBg"));
	Bg->SetBrushColor(MCOrbitUI::Navy());
	Bg->SetPadding(FMargin(14.f));
	WidgetTree->RootWidget = Bg;
	Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelVB"));
	Bg->SetContent(Box);
	UTextBlock* T = NewText(WidgetTree, TEXT("TitleText"), 22, MCOrbitUI::Cyan());
	T->SetText(FText::FromString(Title()));
	Box->AddChildToVerticalBox(T);
	InfoText = NewText(WidgetTree, TEXT("InfoText"), 14, MCOrbitUI::Dim());
	InfoText->SetMinDesiredWidth(430.f);
	Box->AddChildToVerticalBox(InfoText);
	BuildButtons();
}

UHorizontalBox* UMCOrbitPanelBase::AddRow()
{
	UHorizontalBox* R = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(R)) S->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	return R;
}

UButton* UMCOrbitPanelBase::AddButton(UHorizontalBox* Row, const FString& Label, int32 Code, float MinWidth)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	B->SetBackgroundColor(FLinearColor(0.10f, 0.45f, 0.55f, 1.f));
	UTextBlock* L = NewText(WidgetTree, *FString::Printf(TEXT("Btn%d_Label"), Code), 14, FLinearColor::White);
	L->SetText(FText::FromString(Label)); L->SetMinDesiredWidth(MinWidth); L->SetJustification(ETextJustify::Center);
	B->AddChild(L);
	UMCOrbitClickRelay* R = NewObject<UMCOrbitClickRelay>(this);
	R->Owner = this; R->Code = Code;
	Relays.Add(R);
	B->OnClicked.AddDynamic(R, &UMCOrbitClickRelay::Fire);
	if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(B)) S->SetPadding(FMargin(2.f));
	return B;
}

void UMCOrbitPanelBase::SendAction(int32 Code)
{
	AMCOrbitManager::SendAction(GetOwningPlayer(), Code);
}

void UMCOrbitPanelBase::NativeTick(const FGeometry& G, float Dt)
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
		return;
	}
	RefreshTimer -= Dt;
	if (RefreshTimer > 0.f) return;
	RefreshTimer = RefreshHz > 0.f ? 1.f / RefreshHz : 0.1f;
	if (InfoText && Manager->PanelText.IsValidIndex(Role)) InfoText->SetText(FText::FromString(Manager->PanelText[Role]));
}

// ---- role button sets (labels are static; the info text carries the live names) ----

void UMCOrbitSystemsPanel::BuildButtons()
{
	UHorizontalBox* R = AddRow();
	AddButton(R, TEXT("RADAR"), 100); AddButton(R, TEXT("PRESSURE"), 101); AddButton(R, TEXT("LEGS"), 102); AddButton(R, TEXT("THRUSTERS"), 103);
	R = AddRow();
	AddButton(R, TEXT("REPORT LIMITS TO PILOT"), 104, 260.f);
	R = AddRow();
	AddButton(R, TEXT("GO"), 90, 120.f); AddButton(R, TEXT("NO-GO"), 91, 120.f);
}

void UMCOrbitSafetyPanel::BuildButtons()
{
	UHorizontalBox* R = AddRow();
	for (int32 i = 0; i < 6; ++i) AddButton(R, FString::Printf(TEXT("MARK #%d"), i + 1), 200 + i, 62.f);
	R = AddRow();
	for (int32 i = 0; i < 4; ++i) AddButton(R, FString::Printf(TEXT("VIEW SITE %d"), i + 1), 220 + i, 80.f);
	R = AddRow();
	static const TCHAR* Z[] = { TEXT("CLEAR N"), TEXT("CLEAR E"), TEXT("CLEAR S"), TEXT("CLEAR W") };
	for (int32 i = 0; i < 4; ++i) AddButton(R, Z[i], 210 + i, 80.f);
	R = AddRow();
	AddButton(R, TEXT("CONFIRM SITE"), 230, 130.f); AddButton(R, TEXT("REJECT SITE"), 231, 130.f);
	R = AddRow();
	AddButton(R, TEXT("GO"), 90, 120.f); AddButton(R, TEXT("NO-GO"), 91, 120.f);
}

void UMCOrbitPilotPanel::BuildButtons()
{
	UHorizontalBox* R = AddRow();
	AddButton(R, TEXT("BURN INSERTION"), 330, 170.f); AddButton(R, TEXT("STEEP"), 320, 80.f); AddButton(R, TEXT("SHALLOW"), 321, 80.f);
	R = AddRow();
	AddButton(R, TEXT("THR +10"), 300, 80.f); AddButton(R, TEXT("THR -10"), 301, 80.f); AddButton(R, TEXT("+2"), 302, 50.f); AddButton(R, TEXT("-2"), 303, 50.f);
	R = AddRow();
	AddButton(R, TEXT("HOVER"), 308, 80.f); AddButton(R, TEXT("CUT"), 309, 80.f); AddButton(R, TEXT("STOP DRIFT"), 310, 110.f);
	R = AddRow();
	AddButton(R, TEXT("W"), 304, 50.f); AddButton(R, TEXT("N"), 306, 50.f); AddButton(R, TEXT("S"), 307, 50.f); AddButton(R, TEXT("E"), 305, 50.f);
	R = AddRow();
	AddButton(R, TEXT("GO"), 90, 120.f); AddButton(R, TEXT("NO-GO"), 91, 120.f);
}

void UMCOrbitAnalystPanel::BuildButtons()
{
	UHorizontalBox* R = AddRow();
	for (int32 i = 0; i < 4; ++i) AddButton(R, FString::Printf(TEXT("RANK %d"), i + 1), 400 + i, 70.f);
	AddButton(R, TEXT("RESET"), 404, 70.f);
	R = AddRow();
	AddButton(R, TEXT("PROPOSE TOP SITE"), 405, 220.f);
	R = AddRow();
	static const TCHAR* Z[] = { TEXT("DIVERT N"), TEXT("DIVERT E"), TEXT("DIVERT S"), TEXT("DIVERT W") };
	for (int32 i = 0; i < 4; ++i) AddButton(R, Z[i], 410 + i, 80.f);
	R = AddRow();
	AddButton(R, TEXT("GO"), 90, 120.f); AddButton(R, TEXT("NO-GO"), 91, 120.f);
}
