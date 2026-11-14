// Choose-to-place picker (agent F).
//
// Lists the parts that fit the aimed socket and the player's department, shows a detail text and the predicted
// T/W, fuel and power if the highlighted part were installed (manager Preview, no state written), and confirms
// through AMCBayCharacter::ServerPlacePart.
//
// Reparent a WBP to UMCPartPickerWidget and add widgets with EXACTLY these names (all optional):
//   PartList      UPanelWidget (ScrollBox or VerticalBox)  rows are added here at runtime (one Button+TextBlock each)
//   TitleText     TextBlock   "Choose a power part"
//   DetailText    TextBlock   selected part: name, analog, description, numbers
//   TWRText       TextBlock   predicted T/W, coloured red < 1.3, yellow < 2.0, green >= 2.0
//   FuelText      TextBlock   predicted fuel margin
//   PowerText     TextBlock   predicted power balance
//   CostText      TextBlock   predicted mass / cost
//   ConfirmButton Button
//   CancelButton  Button
// If the Blueprint has no layout at all, a plain fallback layout is built in code, so the C++ class works alone.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Construction/MCConstructionTypes.h"
#include "MCPartPickerWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class UButton;
class UMCPartPickerWidget;
class AMCPartSocket;
class AMCBayCharacter;
class AMCConstructionManager;

/** Routes a row button's OnClicked to the picker with its index. */
UCLASS()
class MISSIONCREW_API UMCPickerRowProxy : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<UMCPartPickerWidget> Owner;
	int32 Index = 0;
	UFUNCTION() void HandleClicked();
};

UCLASS(Blueprintable)
class MISSIONCREW_API UMCPartPickerWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Fill the list for this socket and role, switch the player to cursor mode, show the widget. */
	UFUNCTION(BlueprintCallable, Category = "Picker")
	void Open(AMCPartSocket* InSocket, AMCBayCharacter* InCharacter);

	UFUNCTION(BlueprintCallable, Category = "Picker") void SelectIndex(int32 Index);
	UFUNCTION(BlueprintCallable, Category = "Picker") void Confirm();
	UFUNCTION(BlueprintCallable, Category = "Picker") void Cancel();
	UFUNCTION(BlueprintCallable, Category = "Picker") void Close();

	UFUNCTION(BlueprintPure, Category = "Picker") UMCPartDef* GetSelectedDef() const;

	/** Fires after the list is built (for Blueprint styling). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Picker") void BP_OnOpened(int32 NumCandidates);

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> PartList;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TWRText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> FuelText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PowerText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> CostText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> CancelButton;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	void BuildFallbackLayout();
	void PopulateRows();
	void ClearRows();
	void UpdatePreview();

	UFUNCTION() void HandleConfirm();
	UFUNCTION() void HandleCancel();

	UPROPERTY() TObjectPtr<AMCPartSocket> Socket;
	UPROPERTY() TObjectPtr<AMCBayCharacter> Character;
	UPROPERTY() TObjectPtr<AMCConstructionManager> Manager;
	UPROPERTY() TArray<TObjectPtr<UMCPartDef>> Candidates;
	UPROPERTY() TArray<TObjectPtr<UButton>> RowButtons;
	UPROPERTY() TArray<TObjectPtr<UMCPickerRowProxy>> RowProxies;
	int32 SelectedIndex = INDEX_NONE;
};
