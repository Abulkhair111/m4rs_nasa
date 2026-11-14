// Part delivery animation (agent F).
//
// AMCPartMover: replicated actor. Server spawns it when AMCConstructionManager approves a placement
// (OnPlacementApproved). Every machine moves it locally from the server world time:
//   a = clamp((now - StartTime) / Duration, 0, 1); s = a*a*(3-2a)
//   pos = Lerp(StartLoc, EndLoc, s) + (0,0, ArcHeight * sin(pi*a))
// Light parts: arc flight from the department shelf. Heavy parts: lowered straight down by a crane cable and hook
// (debug-drawn, no meshes are created; a user-supplied crane model can replace the debug draw later).
// On arrival the server spawns the AMCPartActor visual, attaches it to the socket, calls
// AMCConstructionManager::FinishPlacement and destroys the mover.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Construction/MCConstructionTypes.h"
#include "MCPartMover.generated.h"

class AMCPartSocket;
class AMCConstructionManager;
class UStaticMeshComponent;

/** The visible installed part (mesh from the part definition). Replaces BP_Part for C++ installs. */
UCLASS()
class MISSIONCREW_API AMCPartActor : public AActor
{
	GENERATED_BODY()
public:
	AMCPartActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** Server: set before/after spawn; clients apply it in OnRep_Def. */
	void SetDef(UMCPartDef* NewDef);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> MeshComp;
	UPROPERTY(ReplicatedUsing = OnRep_Def, BlueprintReadOnly) TObjectPtr<UMCPartDef> Def;

	/** Z offset from the mesh pivot to its bottom, so the part rests on the socket. */
	static float BottomOffset(const UMCPartDef* D);

protected:
	UFUNCTION() void OnRep_Def();
	void ApplyDef();
};

UCLASS()
class MISSIONCREW_API AMCPartMover : public AActor
{
	GENERATED_BODY()
public:
	AMCPartMover();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	/** Idempotent. Hooks OnPlacementApproved so approved placements spawn movers. Call on the server. */
	static void BindToManager(AMCConstructionManager* Manager);
	static AMCPartMover* Launch(AMCPartSocket* Socket, UMCPartDef* Def, EMCCrewRole InRole, bool bHeavy);

	UPROPERTY(Replicated, BlueprintReadOnly) TObjectPtr<UMCPartDef> Def;
	UPROPERTY(Replicated, BlueprintReadOnly) TObjectPtr<AMCPartSocket> Socket;
	UPROPERTY(Replicated, BlueprintReadOnly) FVector StartLoc = FVector::ZeroVector;
	UPROPERTY(Replicated, BlueprintReadOnly) FVector EndLoc = FVector::ZeroVector;
	UPROPERTY(Replicated, BlueprintReadOnly) double StartTime = 0.0;
	UPROPERTY(Replicated, BlueprintReadOnly) float Duration = 1.4f;
	UPROPERTY(Replicated, BlueprintReadOnly) float ArcHeight = 150.f;
	UPROPERTY(Replicated, BlueprintReadOnly) bool bHeavy = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> MeshComp;

	/** Crane anchor height above the end point (heavy parts). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mover") float CraneHeight = 1800.f;

private:
	static void HandlePlacementApproved(AMCPartSocket* Socket, UMCPartDef* Def, EMCCrewRole InRole, bool bHeavy);
	void ApplyMesh();
	void UpdatePose(float Alpha);
	void DrawCrane(float Alpha) const;
	void Finish();

	EMCCrewRole PlacedRole = EMCCrewRole::None;
	bool bDone = false;
	bool bMeshApplied = false;
};
