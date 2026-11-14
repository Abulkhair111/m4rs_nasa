// In-game bay menu (agent I2): Esc pause menu (Resume, Controls / Help, Back to Main Menu, Quit) and the Controls panel.
// Code-built layout, no editor assets needed. Optional Blueprint child: set BP names ResumeButton, ControlsButton,
// MainMenuButton, QuitButton, BackButton (Buttons), MenuPanel, HelpPanel (any widget), HelpText (TextBlock).
//
// HOOK FOR THE CHARACTER (agent I1, owner of AMCBayCharacter):
//   1. Header:  #include "Bay/UI/MCBayMenuWidget.h"   member:  UPROPERTY() TObjectPtr<UMCBayMenuWidget> Menu;
//               optional UPROPERTY(EditAnywhere) TSubclassOf<UMCBayMenuWidget> MenuClass;
//   2. Input:   map EKeys::Escape (and EKeys::P, because in the editor PIE Esc stops the session) to an action, Started ->
//         void AMCBayCharacter::OnMenuPressed()
//         {
//             if (Picker && Picker->IsInViewport()) { Picker->Cancel(); return; }   // Esc closes the picker first
//             ToggleMenu();
//         }
//         void AMCBayCharacter::ToggleMenu()
//         {
//             APlayerController* PC = Cast<APlayerController>(GetController());
//             if (!PC || !PC->IsLocalController()) return;
//             if (!Menu) Menu = CreateWidget<UMCBayMenuWidget>(PC, MenuClass ? MenuClass : TSubclassOf<UMCBayMenuWidget>(UMCBayMenuWidget::StaticClass()));
//             if (Menu) Menu->Toggle();
//         }
//   3. UnPossessed(): if (Menu) { Menu->Close(); Menu = nullptr; }
//   The widget itself switches the cursor on/off through AMCBayCharacter::SetCursorMode (look/move paused while open), shows
//   the mouse cursor, sets Game-and-UI input with itself focused, and handles Esc/P to close (so the key is not toggled twice).
//   Nothing else is needed. The menu does not pause the game (LAN listen server).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MCBayMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;

UCLASS(Blueprintable)
class MISSIONCREW_API UMCBayMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Shows the menu (adds to viewport, cursor on). */
	UFUNCTION(BlueprintCallable, Category = "Menu") void Open();
	/** Hides it and returns to first-person control. */
	UFUNCTION(BlueprintCallable, Category = "Menu") void Close();
	UFUNCTION(BlueprintCallable, Category = "Menu") void Toggle();
	UFUNCTION(BlueprintPure, Category = "Menu") bool IsOpen() const { return IsInViewport(); }
	/** Shows the Controls / Help page (true) or the main menu page (false). */
	UFUNCTION(BlueprintCallable, Category = "Menu") void ShowHelp(bool bHelp);

	/** Level opened by "Back to Main Menu". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu") FName MainMenuLevel = TEXT("L_MainMenu");

	/** The controls and role tips text (shown in the Help panel and reusable elsewhere). */
	UFUNCTION(BlueprintPure, Category = "Menu") static FText GetControlsText();

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ResumeButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ControlsButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> MainMenuButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> QuitButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BackButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> MenuPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> HelpPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HelpText;

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	void BuildFallbackLayout();
	void SetCharacterCursor(bool bOn);

	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleControls();
	UFUNCTION() void HandleBack();
	UFUNCTION() void HandleMainMenu();
	UFUNCTION() void HandleQuit();
};
