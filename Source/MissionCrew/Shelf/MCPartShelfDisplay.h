// One actor per shelf (Agent R). At BeginPlay (every machine, deterministic, no replication) it spawns one AMCShelfPart
// per part definition in a row (or several rows) along its local Y axis. The actor origin is the TABLETOP (parts stand
// on it, bottoms at PartZ above the origin); local +X is the front of the shelf (the side the player stands on).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Construction/MCConstructionTypes.h"
#include "MCPartShelfDisplay.generated.h"

class AMCShelfPart;
class UTextRenderComponent;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCPartShelfDisplay : public AActor
{
	GENERATED_BODY()

public:
	AMCPartShelfDisplay();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shelf") TObjectPtr<USceneComponent> Root;
	/** Department sign above the row (engine text). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shelf") TObjectPtr<UTextRenderComponent> TitleText;

	/** Sign text, e.g. "POWER & COMMS (LANDER)". Empty = no sign. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf") FString DepartmentLabel;
	/** Department number 1..4 used by the catalog fill (0 = every department). Also tints nothing: info only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf") int32 Department = 0;
	/** The parts on this shelf, in order (left to right along local +Y). Tools are skipped (the tool stand handles them). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf") TArray<TSoftObjectPtr<UMCPartDef>> Parts;

	/** Cheat: ignore Parts and take every non-tool def of the manager's Catalog whose Department equals Department (0 = all). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf|Cheat") bool bFillFromCatalog = false;

	/** Distance between neighbouring parts in a row, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf|Layout") float Spacing = 100.f;
	/** Number of rows (parts are split evenly; extra rows go behind, along local -X). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf|Layout", meta = (ClampMin = "1")) int32 Rows = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf|Layout") float RowSpacing = 100.f;
	/** Centre each row on the actor origin (otherwise the row starts at the origin and runs along +Y). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf|Layout") bool bCenterRow = true;
	/** Extra yaw of every part (and its name label) relative to the actor. 0 = labels face local +X. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf|Layout") float FacingYaw = 0.f;
	/** Height of the part bottoms above the actor origin, cm (e.g. plinth thickness). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf|Layout") float PartZ = 0.f;
	/** > 0: shrink parts so none is larger than this (cm) in any direction. 0 = real size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf|Layout") float MaxPartSizeCm = 0.f;
	/** Show each part's name above it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf|Layout") bool bShowNameLabels = true;
	/** Sign height above the actor origin, cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf|Layout") float TitleHeight = 240.f;

	/** Destroys and respawns the shelf items (call after changing Parts at runtime). */
	UFUNCTION(BlueprintCallable, Category = "Shelf") void Rebuild();
	const TArray<TObjectPtr<AMCShelfPart>>& GetShelfParts() const { return Spawned; }

protected:
	UPROPERTY() TArray<TObjectPtr<AMCShelfPart>> Spawned;
	void Clear();
};
