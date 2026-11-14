// Shared base of the S2 console panels (agent S2b): manager lookup, local role, button -> action wiring, fallback-layout helpers.
//
// Button clicks go to the LOCAL PAWN's  UFUNCTION(Server, Reliable) void ServerTransitAction(int32 Code)
// (AMCStationCharacter, agent S2c). It is found by name through reflection, so this code has no include dependency on the pawn.
// If neither pawn nor controller has the function and we are the server, AMCTransitManager::HandleAction(Code, CrewRole) is called directly.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Transit/MCTransitTypes.h"
#include "MCTransitPanelBase.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class AMCTransitManager;
class APlayerController;
class UMCTransitPanelBase;

namespace MCTransitUI
{
	inline FLinearColor Navy()  { return FLinearColor(0.02f, 0.05f, 0.10f, 0.88f); }
	inline FLinearColor Cyan()  { return FLinearColor(0.25f, 0.88f, 1.00f, 1.f); }
	inline FLinearColor Green() { return FLinearColor(0.30f, 0.90f, 0.45f, 1.f); }
	inline FLinearColor Amber() { return FLinearColor(1.00f, 0.75f, 0.20f, 1.f); }
	inline FLinearColor Red()   { return FLinearColor(1.00f, 0.30f, 0.30f, 1.f); }
	inline FLinearColor Dim()   { return FLinearColor(0.55f, 0.65f, 0.75f, 1.f); }
	inline FLinearColor BtnOff(){ return FLinearColor(0.08f, 0.16f, 0.26f, 1.f); }
	inline FLinearColor BtnOn() { return FLinearColor(0.10f, 0.45f, 0.55f, 1.f); }
	/** "COAST TO MOON", "COUNTDOWN"... */
	MISSIONCREW_API const TCHAR* StageName(EMCTransitStage Stage);
	/** Calls ServerTransitAction(Code) on the player's pawn (then controller) by reflection; falls back to Manager->HandleAction on the server. */
	MISSIONCREW_API void SendToPawn(APlayerController* PC, int32 Code, AMCTransitManager* Manager);
}

/** Forwards a UButton click to its panel with a fixed action code (UButton::OnClicked has no parameters). */
UCLASS()
class MISSIONCREW_API UMCTransitClickRelay : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<UMCTransitPanelBase> Owner;
	int32 Code = 0;
	UFUNCTION() void Fire();
};

UCLASS(Abstract)
class MISSIONCREW_API UMCTransitPanelBase : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Send an action code (role*100+action, 90 = GO) to the server through the local pawn. */
	UFUNCTION(BlueprintCallable, Category = "Transit") void SendAction(int32 Code);
	/** CrewRole of the local player (0 = unassigned). */
	UFUNCTION(BlueprintPure, Category = "Transit") int32 GetLocalRole() const;
	UFUNCTION(BlueprintPure, Category = "Transit") AMCTransitManager* GetManager() const { return Manager; }
	/** Panel refresh rate (Hz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transit") float RefreshHz = 10.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	/** Fill the widgets from the manager. Called RefreshHz times per second with a valid manager. */
	virtual void RefreshPanel() {}
	/** Called once after the layout exists (designer or fallback): call BindButton here. */
	virtual void BindButtons() {}
	/** Builds the whole layout in code when the Blueprint has no root widget. */
	virtual void BuildFallbackLayout() {}

	/** Click on Button sends Code. Null buttons are ignored. */
	void BindButton(UButton* Button, int32 Code);

	// Fallback layout helpers
	UVerticalBox* MakeRootBox(const TCHAR* Title, FLinearColor TitleColor);
	UTextBlock* MakeText(const TCHAR* Name, int32 Size, FLinearColor Color = FLinearColor(0.25f, 0.88f, 1.f, 1.f));
	UButton* MakeButton(const TCHAR* Name, const FString& Label, UTextBlock*& OutLabel, float MinWidth = 120.f);

	UPROPERTY(Transient) TObjectPtr<AMCTransitManager> Manager;
	UPROPERTY(Transient) TArray<TObjectPtr<UMCTransitClickRelay>> Relays;
	float FindTimer = 0.f;
	float RefreshTimer = 0.f;
};
