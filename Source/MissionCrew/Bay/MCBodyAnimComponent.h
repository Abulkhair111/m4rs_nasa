// Body animation without an Animation Blueprint (agent W).
//
// Drives the character's body USkeletalMeshComponent with single-node animation (SetAnimation/PlayAnimation).
// State comes from the owner: ground speed, IsFalling, and for AMCStationCharacter the seated flag
// (SeatedStation != null -> SitAnim). Runs on every machine (clients read the replicated movement velocity),
// skipped on dedicated servers.
//
// Limits: single-node playback has no blending, so an animation switch is a hard cut (only change on state
// change to keep it cheap). Walk/Run play rate is scaled by speed. No upper/lower body layering.
//
// bProceduralFallback: when no animation is assigned/available (or the body is a plain static mesh), a gentle
// idle bob / walk sway is applied to the body component's relative location/rotation.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MCBodyAnimComponent.generated.h"

class UAnimSequence;
class USkeletalMeshComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EMCBodyAnimState : uint8 { Idle, Walk, Run, Jump, Fall, Sit, Work };

UCLASS(Blueprintable, ClassGroup = (MissionCrew), meta = (BlueprintSpawnableComponent))
class MISSIONCREW_API UMCBodyAnimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMCBodyAnimComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ---- Animation assets (assign in BP_MC_BayPlayer; all optional) ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") TSoftObjectPtr<UAnimSequence> IdleAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") TSoftObjectPtr<UAnimSequence> WalkAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") TSoftObjectPtr<UAnimSequence> RunAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") TSoftObjectPtr<UAnimSequence> JumpAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") TSoftObjectPtr<UAnimSequence> FallAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") TSoftObjectPtr<UAnimSequence> SitAnim;
	/** Optional: played when SetWorking(true) is called (e.g. at a station). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") TSoftObjectPtr<UAnimSequence> WorkAnim;

	// ---- Tuning ----
	/** Ground speed (cm/s) above which Walk plays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") float WalkSpeed = 20.f;
	/** Ground speed (cm/s) above which Run plays (if RunAnim set; else Walk is sped up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") float RunSpeed = 380.f;
	/** Speed at which the Walk clip plays at rate 1 (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") float WalkRefSpeed = 150.f;
	/** Speed at which the Run clip plays at rate 1 (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") float RunRefSpeed = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") float MinPlayRate = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") float MaxPlayRate = 2.f;
	/** Looping flags: Idle/Walk/Run/Sit/Work loop; Jump/Fall loop is configurable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") bool bLoopJump = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim") bool bLoopFall = true;

	// ---- Body lookup ----
	/** Optional explicit reference (set it in the BP if auto-search picks the wrong mesh). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim|Body") FComponentReference BodyRef;
	/** Name searched first; then tag 'MCBody'; then the first skeletal mesh that is not the capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim|Body") FName BodyName = TEXT("Body");

	// ---- Procedural fallback ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim|Procedural") bool bProceduralFallback = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim|Procedural") float IdleBobAmplitude = 0.8f;   // cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim|Procedural") float IdleBobSpeed = 1.6f;       // Hz-ish
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim|Procedural") float WalkBobAmplitude = 2.5f;   // cm
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim|Procedural") float WalkSwayDegrees = 4.f;    // roll
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BodyAnim|Procedural") float WalkLeanDegrees = 5.f;    // forward pitch

	// ---- State / API ----
	UPROPERTY(BlueprintReadOnly, Category = "BodyAnim") EMCBodyAnimState CurrentState = EMCBodyAnimState::Idle;
	UFUNCTION(BlueprintCallable, Category = "BodyAnim") void SetWorking(bool bWorking) { bWorking_ = bWorking; }
	UFUNCTION(BlueprintPure, Category = "BodyAnim") USkeletalMeshComponent* GetBodySkeletal() const { return BodySkel; }
	/** Re-run the body search (call after changing components at runtime). */
	UFUNCTION(BlueprintCallable, Category = "BodyAnim") void FindBody();

protected:
	EMCBodyAnimState ComputeState(float GroundSpeed, bool bFalling, float VelZ) const;
	void EnterState(EMCBodyAnimState NewState);
	UAnimSequence* GetAnimFor(EMCBodyAnimState S) const;
	bool ShouldLoop(EMCBodyAnimState S) const;
	void ApplyProcedural(float DeltaTime, float GroundSpeed, EMCBodyAnimState S);

	UPROPERTY(Transient) TObjectPtr<USceneComponent> BodyComp;
	UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> BodySkel;

	bool bWorking_ = false;
	bool bAnimTried = false;
	bool bHaveBase = false;
	bool bPlayingAsset = false;
	float PhaseTime = 0.f;
	FVector BaseLoc = FVector::ZeroVector;
	FRotator BaseRot = FRotator::ZeroRotator;
};
