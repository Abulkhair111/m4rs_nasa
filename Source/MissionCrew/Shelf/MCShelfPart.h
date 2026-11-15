// C++ shelf items (Agent R, 2026-11-15). Replace the leftover Blueprint BP_Part decor on the shelves/plinths.
//
// AMCShelfPart: one part standing on a shelf. It is an AMCInteractable, so the pawn's E/aim trace already hits it
// (TraceBox, Visibility) and the HUD prompt (AimPrompt) shows its rich GetPromptFor text. E does NOT go through the
// server: the pawn stores the part as SelectedPart (local), the next E on a compatible empty socket calls the server
// RPC ServerPlacePartFrom, and the part flies in from this shelf position. Shelves are infinite supply: the shelf
// part stays where it is and only plays a short pickup bob.
//
// Visual: Def->Mesh with Def->MeshScale and (for parts with no StackMesh) Def->StackMeshRotation, fitted by the mesh
// bounds so the bottom sits on the actor origin and the centre of the footprint on the actor X/Y.
#pragma once

#include "CoreMinimal.h"
#include "Interact/MCInteractable.h"
#include "MCShelfPart.generated.h"

class AMCPartSocket;
class UMCPartDef;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCShelfPart : public AMCInteractable
{
	GENERATED_BODY()

public:
	AMCShelfPart();

	virtual void Tick(float DeltaSeconds) override;

	/** The part this shelf item represents. Set it before spawning, or per instance in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf") TObjectPtr<UMCPartDef> Def;
	/** > 0: scale the part down uniformly so its largest dimension is at most this many cm (0 = real size). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf") float FitMaxCm = 0.f;
	/** Show the part name as a small world label above the part. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shelf") bool bShowNameLabel = true;

	/** Assign a definition at runtime and rebuild the visual. */
	UFUNCTION(BlueprintCallable, Category = "Shelf") void SetPartDef(UMCPartDef* NewDef);
	/** (Re)builds mesh, fit, trace box and label from Def. */
	UFUNCTION(BlueprintCallable, Category = "Shelf") void ApplyDef();
	/** Centre of the part in the world: the start point of the flight to the socket. */
	UFUNCTION(BlueprintPure, Category = "Shelf") FVector GetPickupLocation() const;
	/** Short local bob (any machine). */
	UFUNCTION(BlueprintCallable, Category = "Shelf") void PlayPickup();

	virtual FText GetPromptFor(const AMCBayCharacter* Who) const override;
	/** Selection is local to the pawn, so the server Use is a no-op (keeps BP_OnUsed). */
	virtual void Use(AMCBayCharacter* Who) override;

	/** Empty string when Def can be installed on Socket by this role right now, otherwise the reason (short sentence). */
	static FString WhyNotPlaceable(const UMCPartDef* PartDef, const AMCPartSocket* Socket, const AMCConstructionManager* Manager, int32 CrewRole);
	/** "Power Engineer" ... for a department number. */
	static FString DepartmentTitle(int32 Dept);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	FVector PartSize = FVector(30.f);
	FVector MeshBaseOffset = FVector::ZeroVector;
	float PickupTime = -1.f;
	static constexpr float PickupDuration = 0.5f;
};
