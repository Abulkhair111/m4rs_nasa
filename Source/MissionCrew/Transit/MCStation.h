// Transit station (agent S2c). One actor per role placed in the level (Role 1..4).
// Components: Root, Trigger box (interaction area), SeatPoint (where the pawn stands/sits), Camera (looks at the
// station screen; the view blends to it while seated), TextRender label.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCStation.generated.h"

class UBoxComponent;
class UCameraComponent;
class UTextRenderComponent;
class USceneComponent;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCStation : public AActor
{
	GENERATED_BODY()

public:
	AMCStation();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** CrewRole 1 Systems, 2 Equipment, 3 Pilot, 4 Science. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station", meta = (ClampMin = "1", ClampMax = "4")) int32 CrewRole = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") FText StationName;
	/** Max distance (cm) from the eye at which the station can be used when aimed at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station") float UseRange = 400.f;
	/** Server-side: the pawn that sits here (replicated so clients show "occupied"). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Station") TObjectPtr<APawn> Occupant;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Station") TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Station") TObjectPtr<UBoxComponent> Trigger;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Station") TObjectPtr<USceneComponent> SeatPoint;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Station") TObjectPtr<UCameraComponent> StationCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Station") TObjectPtr<UTextRenderComponent> Label;

	UFUNCTION(BlueprintPure, Category = "Station") static FString RoleName(int32 InRole);
	/** True if a player of PlayerRole (0 = unassigned) may use this station. bFreeRoles lifts the restriction. */
	UFUNCTION(BlueprintPure, Category = "Station") bool CanBeUsedBy(int32 PlayerRole, bool bFreeRoles) const;
	UFUNCTION(BlueprintPure, Category = "Station") bool IsFree() const { return Occupant == nullptr; }
	UFUNCTION(BlueprintCallable, Category = "Station") static AMCStation* FindForRole(UObject* WorldContext, int32 InRole);
};
