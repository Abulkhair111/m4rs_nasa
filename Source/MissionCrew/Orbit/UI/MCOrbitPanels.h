// S3 role panels, built entirely in code (pattern of the S2 panels). Each client creates ONLY the panel of its own role.
// Base class: finds the manager, refreshes a text block from Manager->PanelText[Role]; buttons send action codes through the relay.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MCOrbitPanels.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class AMCOrbitManager;

namespace MCOrbitUI
{
	inline FLinearColor Navy() { return FLinearColor(0.02f, 0.05f, 0.10f, 0.88f); }
	inline FLinearColor Cyan() { return FLinearColor(0.25f, 0.88f, 1.f, 1.f); }
	inline FLinearColor Amber() { return FLinearColor(1.f, 0.75f, 0.2f, 1.f); }
	inline FLinearColor Dim() { return FLinearColor(0.55f, 0.65f, 0.75f, 1.f); }
}

UCLASS()
class MISSIONCREW_API UMCOrbitClickRelay : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<UUserWidget> Owner;
	int32 Code = 0;
	/** Mode 1 = HUD role tab (Code = role) instead of an action. */
	int32 Mode = 0;
	UFUNCTION() void Fire();
};

UCLASS(Abstract)
class MISSIONCREW_API UMCOrbitPanelBase : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Role 1..4 this panel serves (set by the HUD). */
	int32 Role = 0;
	void SendAction(int32 Code);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit") float RefreshHz = 10.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& G, float Dt) override;
	virtual const TCHAR* Title() const { return TEXT("PANEL"); }
	virtual void BuildButtons() {}
	UButton* AddButton(UHorizontalBox* Row, const FString& Label, int32 Code, float MinWidth = 90.f);
	UHorizontalBox* AddRow();

	UPROPERTY(Transient) TObjectPtr<AMCOrbitManager> Manager;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> InfoText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Box;
	UPROPERTY(Transient) TArray<TObjectPtr<UMCOrbitClickRelay>> Relays;
	float FindTimer = 0.f, RefreshTimer = 0.f;
};

UCLASS() class MISSIONCREW_API UMCOrbitSystemsPanel : public UMCOrbitPanelBase
{
	GENERATED_BODY()
	virtual const TCHAR* Title() const override { return TEXT("SYSTEMS OPERATOR"); }
	virtual void BuildButtons() override;
};
UCLASS() class MISSIONCREW_API UMCOrbitSafetyPanel : public UMCOrbitPanelBase
{
	GENERATED_BODY()
	virtual const TCHAR* Title() const override { return TEXT("SAFETY OFFICER"); }
	virtual void BuildButtons() override;
};
UCLASS() class MISSIONCREW_API UMCOrbitPilotPanel : public UMCOrbitPanelBase
{
	GENERATED_BODY()
	virtual const TCHAR* Title() const override { return TEXT("PILOT"); }
	virtual void BuildButtons() override;
};
UCLASS() class MISSIONCREW_API UMCOrbitAnalystPanel : public UMCOrbitPanelBase
{
	GENERATED_BODY()
	virtual const TCHAR* Title() const override { return TEXT("LANDING ANALYST"); }
	virtual void BuildButtons() override;
};
