#include "Bay/UI/MCBayMenuWidget.h"
#include "Bay/MCBayCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"

FText UMCBayMenuWidget::GetControlsText()
{
	return FText::FromString(TEXT(
		"MOVEMENT\n"
		"  W A S D       walk\n"
		"  Mouse         look around\n"
		"  Space         jump (low gravity)\n"
		"\n"
		"BUILDING\n"
		"  E             interact: aim at a socket or a rocket part to choose a part, swap it, or bolt / wire it with a tool\n"
		"  E on a launcher core stage: next launcher\n"
		"  R             remove the aimed part (only while it is not bolted)\n"
		"  Tab           cursor mode (mouse cursor on, look paused)\n"
		"  Esc or P      this menu\n"
		"\n"
		"ROLE TIPS\n"
		"  MCC Engineer (power and comms): panels, batteries, comms dish. Keep generation 10% above use and a battery installed.\n"
		"  Flight Director (rocket and budget): choose the launcher, keep mass under the payload and cost under the budget.\n"
		"  Pilot (engines and fuel): fuel must cover 90% of dry mass; take-off T/W needs 1.3 (green from 2.0 with a lidar).\n"
		"  Scientist (instruments): at least two ice detectors and enough science value.\n"
		"  Everyone: bolt every part (wrench) and wire it (cable); the wall boards show what is still red."));
}

void UMCBayMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
	if (WidgetTree && !WidgetTree->RootWidget && !ResumeButton) BuildFallbackLayout();
	if (ResumeButton) ResumeButton->OnClicked.AddDynamic(this, &UMCBayMenuWidget::HandleResume);
	if (ControlsButton) ControlsButton->OnClicked.AddDynamic(this, &UMCBayMenuWidget::HandleControls);
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &UMCBayMenuWidget::HandleBack);
	if (MainMenuButton) MainMenuButton->OnClicked.AddDynamic(this, &UMCBayMenuWidget::HandleMainMenu);
	if (QuitButton) QuitButton->OnClicked.AddDynamic(this, &UMCBayMenuWidget::HandleQuit);
	if (HelpText) HelpText->SetText(GetControlsText());
	ShowHelp(false);
}

void UMCBayMenuWidget::BuildFallbackLayout()
{
	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuDim"));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Dim->SetHorizontalAlignment(HAlign_Center);
	Dim->SetVerticalAlignment(VAlign_Center);
	WidgetTree->RootWidget = Dim;

	UBorder* Card = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuCard"));
	Card->SetBrushColor(FLinearColor(0.03f, 0.05f, 0.09f, 0.97f));
	Card->SetPadding(FMargin(48.f, 36.f));
	Dim->SetContent(Card);

	UVerticalBox* Pages = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuPages"));
	Card->SetContent(Pages);

	auto MakeText = [&](const TCHAR* Name, int32 Size, const FLinearColor& Col, const FString& Str, bool bWrap = false) {
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		T->SetFont(F);
		T->SetColorAndOpacity(FSlateColor(Col));
		T->SetText(FText::FromString(Str));
		T->SetAutoWrapText(bWrap);
		return T;
	};
	auto MakeButton = [&](TObjectPtr<UButton>& Out, const TCHAR* Name, const TCHAR* Label, UVerticalBox* Into) {
		Out = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Out->SetBackgroundColor(FLinearColor(0.10f, 0.22f, 0.45f, 1.f));
		Out->AddChild(MakeText(*FString::Printf(TEXT("%sLabel"), Name), 26, FLinearColor::White, Label));
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetMinDesiredWidth(380.f);
		Box->AddChild(Out);
		if (UVerticalBoxSlot* S = Into->AddChildToVerticalBox(Box)) S->SetPadding(FMargin(0, 8));
	};

	// Page 1: menu
	UVerticalBox* Menu = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuPanel"));
	MenuPanel = Menu;
	Pages->AddChildToVerticalBox(Menu);
	UTextBlock* Title = MakeText(TEXT("MenuTitle"), 40, FLinearColor(0.6f, 0.8f, 1.f), TEXT("PAUSE"));
	if (UVerticalBoxSlot* S = Menu->AddChildToVerticalBox(Title)) { S->SetPadding(FMargin(0, 0, 0, 16)); S->SetHorizontalAlignment(HAlign_Center); }
	MakeButton(ResumeButton, TEXT("ResumeButton"), TEXT("Resume"), Menu);
	MakeButton(ControlsButton, TEXT("ControlsButton"), TEXT("Controls / Help"), Menu);
	MakeButton(MainMenuButton, TEXT("MainMenuButton"), TEXT("Back to Main Menu"), Menu);
	MakeButton(QuitButton, TEXT("QuitButton"), TEXT("Quit"), Menu);

	// Page 2: help
	UVerticalBox* Help = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HelpPanel"));
	HelpPanel = Help;
	Pages->AddChildToVerticalBox(Help);
	UTextBlock* HTitle = MakeText(TEXT("HelpTitle"), 40, FLinearColor(0.6f, 0.8f, 1.f), TEXT("CONTROLS"));
	if (UVerticalBoxSlot* S = Help->AddChildToVerticalBox(HTitle)) S->SetPadding(FMargin(0, 0, 0, 12));
	USizeBox* TextBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	TextBox->SetWidthOverride(900.f);
	HelpText = MakeText(TEXT("HelpText"), 20, FLinearColor::White, FString(), true);
	TextBox->AddChild(HelpText);
	Help->AddChildToVerticalBox(TextBox);
	MakeButton(BackButton, TEXT("BackButton"), TEXT("Back"), Help);
}

void UMCBayMenuWidget::ShowHelp(bool bHelp)
{
	if (MenuPanel) MenuPanel->SetVisibility(bHelp ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (HelpPanel) HelpPanel->SetVisibility(bHelp ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UMCBayMenuWidget::SetCharacterCursor(bool bOn)
{
	if (AMCBayCharacter* C = Cast<AMCBayCharacter>(GetOwningPlayerPawn()))
	{
		C->SetCursorMode(bOn); // pauses look / move, shows the cursor
	}
}

void UMCBayMenuWidget::Open()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC || IsInViewport()) return;
	ShowHelp(false);
	AddToViewport(100);
	SetCharacterCursor(true);
	PC->SetShowMouseCursor(true);
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
}

void UMCBayMenuWidget::Close()
{
	if (!IsInViewport()) return;
	RemoveFromParent();
	SetCharacterCursor(false);
	if (APlayerController* PC = GetOwningPlayer())
	{
		// SetCursorMode(false) already restored game-only input when the pawn is an AMCBayCharacter; cover other pawns.
		if (!Cast<AMCBayCharacter>(GetOwningPlayerPawn()))
		{
			PC->SetShowMouseCursor(false);
			PC->SetInputMode(FInputModeGameOnly());
		}
	}
}

void UMCBayMenuWidget::Toggle()
{
	if (IsInViewport()) Close(); else Open();
}

FReply UMCBayMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Escape || K == EKeys::P)
	{
		// From the help page Esc goes back first.
		if (HelpPanel && HelpPanel->GetVisibility() == ESlateVisibility::Visible) ShowHelp(false);
		else Close();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UMCBayMenuWidget::HandleResume() { Close(); }
void UMCBayMenuWidget::HandleControls() { ShowHelp(true); }
void UMCBayMenuWidget::HandleBack() { ShowHelp(false); }

void UMCBayMenuWidget::HandleMainMenu()
{
	Close();
	UGameplayStatics::OpenLevel(this, MainMenuLevel);
}

void UMCBayMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
