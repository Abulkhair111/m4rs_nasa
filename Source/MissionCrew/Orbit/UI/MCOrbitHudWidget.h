// S3 HUD: top bar, message line, board, and the ONE panel of the local role (role 0 in free/solo mode gets four tabs).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MCOrbitHudWidget.generated.h"

class UTextBlock;
class UPanelWidget;
class UHorizontalBox;
class AMCOrbitManager;
class UMCOrbitPanelBase;
class UMCOrbitClickRelay;

UCLASS(Blueprintable)
class MISSIONCREW_API UMCOrbitHudWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Optional Blueprint panel classes (null = the in-code panels). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit") TSubclassOf<UMCOrbitPanelBase> SystemsPanelClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit") TSubclassOf<UMCOrbitPanelBase> SafetyPanelClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit") TSubclassOf<UMCOrbitPanelBase> PilotPanelClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit") TSubclassOf<UMCOrbitPanelBase> AnalystPanelClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit") float MessageSeconds = 8.f;
	UFUNCTION(BlueprintCallable, Category = "Orbit") void SetTestRole(int32 Role) { TestRole = FMath::Clamp(Role, 0, 4); }
	UFUNCTION(BlueprintPure, Category = "Orbit") int32 GetShownRole() const { return ShownRole; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& G, float Dt) override;
	void BuildLayout();
	void BuildPanel(int32 Role);

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TopText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BoardText;
	UPROPERTY(Transient) TObjectPtr<UPanelWidget> PanelHost;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> TabBox;
	UPROPERTY(Transient) TObjectPtr<AMCOrbitManager> Manager;
	UPROPERTY(Transient) TObjectPtr<UMCOrbitPanelBase> Panel;
	UPROPERTY(Transient) TArray<TObjectPtr<UMCOrbitClickRelay>> Relays;
	int32 ShownRole = -1, TestRole = 0;
	float FindTimer = 0.f;
	FString LastMessage;
	double MessageTime = -1000.0;
};
