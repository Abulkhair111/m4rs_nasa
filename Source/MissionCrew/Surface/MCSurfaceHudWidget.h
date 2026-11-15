// S4 HUD: shows only the panel of the local player's role (role 0 = all four, solo test). Built fully in code.
// Buttons call AMCSurfaceLink::ServerAction; the Rover Driver drives with W/S/A/D (ServerDrive).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MCSurfaceHudWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class AMCSurfaceLink;
class AMCSurfaceManager;

UCLASS()
class MISSIONCREW_API UMCSurfaceClickRelay : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY() TWeakObjectPtr<AMCSurfaceLink> Link;
	int32 Code = 0;
	UFUNCTION() void Fire();
};

UCLASS()
class MISSIONCREW_API UMCSurfaceHudWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient) TObjectPtr<AMCSurfaceLink> Link;
protected:
	virtual void NativeTick(const FGeometry& G, float Dt) override;
	void Build();
	UVerticalBox* AddPanel(UVerticalBox* Root, const TCHAR* Title);
	UTextBlock* AddText(UVerticalBox* Box, int32 Size, FLinearColor Col);
	void AddButtons(UVerticalBox* Box, const TArray<TPair<FString, int32>>& Defs);

	UPROPERTY(Transient) TArray<TObjectPtr<UMCSurfaceClickRelay>> Relays;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TopText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MsgText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PowerText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SciText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DriveText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DrillText;
	int32 BuiltRole = -1;
	float LastT = 0.f, LastS = 0.f;
	float Timer = 0.f;
};
