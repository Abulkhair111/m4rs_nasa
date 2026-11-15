#include "Bay/MCBayHudWidget.h"
#include "Bay/MCBayCharacter.h"
#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

void UMCBayHudWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!TWRText && !FuelText && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
	}
}

void UMCBayHudWidget::BuildFallbackLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HudCanvas"));
	WidgetTree->RootWidget = Canvas;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HudVB"));
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(VB))
	{
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(24.f, 24.f));
		S->SetAutoSize(true);
	}
	auto AddText = [&](TObjectPtr<UTextBlock>& Out, const TCHAR* Name, int32 Size) {
		Out = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo F = Out->GetFont();
		F.Size = Size;
		Out->SetFont(F);
		if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(Out)) S->SetPadding(FMargin(0, 2));
	};
	AddText(TWRText, TEXT("TWRText"), 26);
	TWRBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("TWRBar"));
	VB->AddChildToVerticalBox(TWRBar);
	AddText(FuelText, TEXT("FuelText"), 16);
	AddText(MessageText, TEXT("MessageText"), 16);

	PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptText"));
	FSlateFontInfo F = PromptText->GetFont();
	F.Size = 24;
	PromptText->SetFont(F);
	PromptText->SetJustification(ETextJustify::Center);
	PromptText->SetAutoWrapText(true);
	PromptText->SetShadowOffset(FVector2D(2.f, 2.f));
	PromptText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(PromptText))
	{
		S->SetAnchors(FAnchors(0.5f, 0.62f));
		S->SetAlignment(FVector2D(0.5f, 0.f));
		S->SetPosition(FVector2D::ZeroVector);
		S->SetAutoSize(false);
		S->SetSize(FVector2D(1000.f, 320.f));
	}

	HintText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HintText"));
	FSlateFontInfo HF = HintText->GetFont();
	HF.Size = 13;
	HintText->SetFont(HF);
	HintText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f, 0.8f)));
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(HintText))
	{
		S->SetAnchors(FAnchors(0.f, 1.f));
		S->SetAlignment(FVector2D(0.f, 1.f));
		S->SetPosition(FVector2D(24.f, -20.f));
		S->SetAutoSize(true);
	}
}

void UMCBayHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Manager)
	{
		FindTimer -= InDeltaTime;
		if (FindTimer <= 0.f)
		{
			FindTimer = 0.5f;
			Manager = MCBayUI::FindManager(GetWorld());
		}
	}
	Refresh();
}

void UMCBayHudWidget::Refresh()
{
	if (HintText) HintText->SetText(HintString);
	if (PromptText)
	{
		FText Prompt;
		if (const AMCBayCharacter* C = Cast<AMCBayCharacter>(GetOwningPlayerPawn())) Prompt = C->AimPrompt;
		PromptText->SetText(Prompt);
	}
	if (!Manager) return;

	const float Twr = Manager->TWR;
	const FLinearColor Col = MCBayUI::TWRColor((Manager->bHasEngine || Manager->BoosterCount > 0) ? Twr : 0.f);
	if (TWRText)
	{
		TWRText->SetText(FText::FromString(FString::Printf(TEXT("T/W %.2f"), Twr)));
		TWRText->SetColorAndOpacity(FSlateColor(Col));
	}
	if (TWRBar)
	{
		TWRBar->SetPercent(FMath::Clamp(Twr / FMath::Max(TWRBarMax, 0.1f), 0.f, 1.f));
		TWRBar->SetFillColorAndOpacity(Col);
	}
	if (FuelText)
	{
		FuelText->SetText(FText::FromString(FString::Printf(TEXT("Fuel %.0f / %.0f kg (need %.0f)"), Manager->FuelKg, Manager->FuelMaxKg, Manager->FuelRequiredKg)));
		const bool bFuelOk = Manager->FuelKg >= Manager->FuelRequiredKg && Manager->FuelRequiredKg > 0.f;
		FuelText->SetColorAndOpacity(FSlateColor(MCBayUI::CheckColor(bFuelOk ? EMCCheckState::Green : EMCCheckState::Red)));
	}
	if (MessageText)
	{
		const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
		const double Now = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
		const bool bShow = !Manager->Message.IsEmpty() && (Now - Manager->MessageTime) < MessageSeconds;
		MessageText->SetText(bShow ? FText::FromString(Manager->Message) : FText::GetEmpty());
	}
}
