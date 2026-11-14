#include "Transit/UI/MCPilotPanel.h"
#include "Transit/MCTransitManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

static FSlateBrush MakeRoundBrush(const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth, float Radius)
{
	FSlateBrush B;
	B.DrawAs = ESlateBrushDrawType::RoundedBox;
	B.TintColor = FSlateColor(Fill);
	B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	B.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
	B.OutlineSettings.Color = FSlateColor(Outline);
	B.OutlineSettings.Width = OutlineWidth;
	return B;
}

static void PlaceOnCanvas(UWidget* W, const FVector2D& Pos, const FVector2D& Size)
{
	if (!W) return;
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(W->Slot))
	{
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetPosition(Pos);
		S->SetSize(Size);
	}
}

void UMCPilotPanel::BuildFallbackLayout()
{
	UVerticalBox* VB = MakeRootBox(TEXT("FLIGHT CONSOLE"), MCTransitUI::Cyan());

	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ScopeBox"));
	Box->SetWidthOverride(ScopeSize);
	Box->SetHeightOverride(ScopeSize);
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(Box)) { S->SetPadding(FMargin(0.f, 6.f)); S->SetHorizontalAlignment(HAlign_Center); }
	ScopeCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ScopeCanvas"));
	Box->AddChild(ScopeCanvas);

	auto AddImage = [&](const TCHAR* Name, const FSlateBrush& Brush) {
		UImage* I = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		I->SetBrush(Brush);
		ScopeCanvas->AddChildToCanvas(I);
		return I;
	};
	UImage* Frame = AddImage(TEXT("ScopeFrame"), MakeRoundBrush(FLinearColor(0.f, 0.05f, 0.08f, 0.9f), MCTransitUI::Dim(), 1.5f, 4.f));
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Frame->Slot)) { S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f)); S->SetOffsets(FMargin(0.f)); }
	UImage* HLine = AddImage(TEXT("CrossH"), MakeRoundBrush(MCTransitUI::Dim(), MCTransitUI::Dim(), 0.f, 0.f));
	PlaceOnCanvas(HLine, FVector2D::ZeroVector, FVector2D(ScopeSize, 1.f));
	UImage* VLine = AddImage(TEXT("CrossV"), MakeRoundBrush(MCTransitUI::Dim(), MCTransitUI::Dim(), 0.f, 0.f));
	PlaceOnCanvas(VLine, FVector2D::ZeroVector, FVector2D(1.f, ScopeSize));
	CorridorRing = AddImage(TEXT("CorridorRing"), MakeRoundBrush(FLinearColor(0.f, 0.f, 0.f, 0.f), MCTransitUI::Green(), 2.f, 500.f));
	MissDot = AddImage(TEXT("MissDot"), MakeRoundBrush(MCTransitUI::Amber(), MCTransitUI::Amber(), 0.f, 6.f));

	MissText = MakeText(TEXT("MissText"), 15, MCTransitUI::Cyan());
	VB->AddChildToVerticalBox(MissText);
	WindowText = MakeText(TEXT("WindowText"), 16, MCTransitUI::Amber());
	VB->AddChildToVerticalBox(WindowText);
	FuelText = MakeText(TEXT("FuelText"), 14, MCTransitUI::Dim());
	VB->AddChildToVerticalBox(FuelText);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BurnRow"));
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(Row)) S->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	auto AddBurn = [&](TObjectPtr<UButton>& Out, const TCHAR* Name, const TCHAR* Label) {
		UTextBlock* L = nullptr;
		Out = MakeButton(Name, Label, L, 56.f);
		if (UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Out)) S->SetPadding(FMargin(3.f, 0.f));
	};
	AddBurn(BurnLeft, TEXT("BurnLeft"), TEXT("LEFT"));
	AddBurn(BurnRight, TEXT("BurnRight"), TEXT("RIGHT"));
	AddBurn(BurnUp, TEXT("BurnUp"), TEXT("UP"));
	AddBurn(BurnDown, TEXT("BurnDown"), TEXT("DOWN"));
}

void UMCPilotPanel::BindButtons()
{
	BindButton(BurnLeft, MCTransit::MakeCode(EMCTransitRole::Pilot, (int32)EMCPilotAction::Left));
	BindButton(BurnRight, MCTransit::MakeCode(EMCTransitRole::Pilot, (int32)EMCPilotAction::Right));
	BindButton(BurnUp, MCTransit::MakeCode(EMCTransitRole::Pilot, (int32)EMCPilotAction::Up));
	BindButton(BurnDown, MCTransit::MakeCode(EMCTransitRole::Pilot, (int32)EMCPilotAction::Down));
}

void UMCPilotPanel::RefreshPanel()
{
	const float X = Manager->DispX, Y = Manager->DispY;
	const float Dist = FMath::Sqrt(X * X + Y * Y);
	const bool bInside = Dist <= Manager->CorridorKm;

	const float Half = ScopeSize * 0.5f - 6.f;
	FVector2D P(X * PxPerKm, -Y * PxPerKm);
	P.X = FMath::Clamp(P.X, -Half, Half);
	P.Y = FMath::Clamp(P.Y, -Half, Half);
	PlaceOnCanvas(MissDot, P, FVector2D(12.f, 12.f));
	const float Diam = 2.f * Manager->CorridorKm * PxPerKm;
	PlaceOnCanvas(CorridorRing, FVector2D::ZeroVector, FVector2D(Diam, Diam));
	if (MissDot) MissDot->SetColorAndOpacity(bInside ? MCTransitUI::Green() : MCTransitUI::Amber());

	if (MissText)
	{
		MissText->SetText(FText::FromString(FString::Printf(TEXT("Predicted miss %.1f km (x %+.1f, y %+.1f)  %s  corridor +-%.0f km"),
			Dist, X, Y, bInside ? TEXT("INSIDE") : TEXT("OUTSIDE"), Manager->CorridorKm)));
		MissText->SetColorAndOpacity(FSlateColor(bInside ? MCTransitUI::Green() : MCTransitUI::Amber()));
	}

	// windows: open one, else the next scheduled
	const FMCTransitWindow* Open = nullptr;
	const FMCTransitWindow* Next = nullptr;
	for (const FMCTransitWindow& W : Manager->Windows)
	{
		if (W.bOpen) { Open = &W; break; }
		if (!W.bDone && W.OpenSec >= 0.f && W.OpenSec > Manager->ElapsedSec && (!Next || W.OpenSec < Next->OpenSec)) Next = &W;
	}
	const bool bCoast = Manager->Stage == EMCTransitStage::Coast;
	if (WindowText)
	{
		FString S;
		if (!bCoast) S = TEXT("Burn windows open during the coast.");
		else if (Open) S = FString::Printf(TEXT("%s OPEN - %.0f s left"), *Open->Label, FMath::Max(0.f, Open->CloseSec - Manager->ElapsedSec));
		else if (Next) S = FString::Printf(TEXT("Next: %s in %.0f s"), *Next->Label, FMath::Max(0.f, Next->OpenSec - Manager->ElapsedSec));
		else S = TEXT("No more windows.");
		WindowText->SetText(FText::FromString(S));
		WindowText->SetColorAndOpacity(FSlateColor(Open ? MCTransitUI::Green() : MCTransitUI::Amber()));
	}
	if (FuelText)
		FuelText->SetText(FText::FromString(FString::Printf(TEXT("Fuel %.0f / %.0f kg"), Manager->FuelKg, Manager->FuelStartKg)));

	const bool bCanBurn = bCoast && Open != nullptr;
	for (UButton* B : { BurnLeft.Get(), BurnRight.Get(), BurnUp.Get(), BurnDown.Get() })
		if (B) B->SetIsEnabled(bCanBurn);
}
