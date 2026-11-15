#include "Bay/MCBodyAnimComponent.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Bay/MCBayCharacter.h"

UMCBodyAnimComponent::UMCBodyAnimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(false);
}

void UMCBodyAnimComponent::BeginPlay()
{
	Super::BeginPlay();
	FindBody();
}

void UMCBodyAnimComponent::FindBody()
{
	BodyComp = nullptr;
	BodySkel = nullptr;
	bHaveBase = false;
	bPlayingAsset = false;
	bAnimTried = false;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// An empty BodyRef resolves to the owner's ROOT component (the capsule). Treat that as "unset": driving the
	// procedural bob/sway on the capsule fights character movement and snaps the pawn back (rubber-banding).
	if (USceneComponent* Ref = Cast<USceneComponent>(BodyRef.GetComponent(Owner)))
	{
		if (Ref != Owner->GetRootComponent()) BodyComp = Ref;
	}
	TArray<USkeletalMeshComponent*> Skels;
	Owner->GetComponents<USkeletalMeshComponent>(Skels);
	if (!BodyComp)
	{
		for (USkeletalMeshComponent* S : Skels)
			if (S && S->GetFName() == BodyName) { BodyComp = S; break; }
	}
	if (!BodyComp)
	{
		TArray<UActorComponent*> Tagged = Owner->GetComponentsByTag(USceneComponent::StaticClass(), TEXT("MCBody"));
		if (Tagged.Num() > 0) BodyComp = Cast<USceneComponent>(Tagged[0]);
	}
	if (!BodyComp)
	{
		// A character's own Mesh (ACharacter::Mesh) is a skeletal mesh and counts; only the capsule is excluded.
		for (USkeletalMeshComponent* S : Skels)
			if (S) { BodyComp = S; break; }
	}
	if (!BodyComp)
	{
		// Static-mesh body (e.g. current BP_MC_BayPlayer 'BodyMesh'): procedural fallback only.
		TArray<UStaticMeshComponent*> Statics;
		Owner->GetComponents<UStaticMeshComponent>(Statics);
		for (UStaticMeshComponent* M : Statics)
			if (M && M->GetFName() == TEXT("BodyMesh")) { BodyComp = M; break; }
	}
	BodySkel = Cast<USkeletalMeshComponent>(BodyComp);
	if (BodyComp)
	{
		BaseLoc = BodyComp->GetRelativeLocation();
		BaseRot = BodyComp->GetRelativeRotation();
		bHaveBase = true;
	}
}

UAnimSequence* UMCBodyAnimComponent::GetAnimFor(EMCBodyAnimState S) const
{
	const TSoftObjectPtr<UAnimSequence>* P = nullptr;
	switch (S)
	{
	case EMCBodyAnimState::Idle: P = &IdleAnim; break;
	case EMCBodyAnimState::Walk: P = &WalkAnim; break;
	case EMCBodyAnimState::Run:  P = &RunAnim; break;
	case EMCBodyAnimState::Jump: P = &JumpAnim; break;
	case EMCBodyAnimState::Fall: P = &FallAnim; break;
	case EMCBodyAnimState::Sit:  P = &SitAnim; break;
	case EMCBodyAnimState::Work: P = &WorkAnim; break;
	}
	UAnimSequence* Seq = (P && !P->IsNull()) ? P->LoadSynchronous() : nullptr;
	// Fallbacks so a partly filled set still works.
	if (!Seq && (S == EMCBodyAnimState::Run)) Seq = WalkAnim.IsNull() ? nullptr : WalkAnim.LoadSynchronous();
	if (!Seq && (S == EMCBodyAnimState::Jump)) Seq = FallAnim.IsNull() ? nullptr : FallAnim.LoadSynchronous();
	if (!Seq && (S == EMCBodyAnimState::Fall)) Seq = JumpAnim.IsNull() ? nullptr : JumpAnim.LoadSynchronous();
	if (!Seq && (S == EMCBodyAnimState::Work)) Seq = SitAnim.IsNull() ? nullptr : SitAnim.LoadSynchronous();
	if (!Seq && S != EMCBodyAnimState::Idle) Seq = IdleAnim.IsNull() ? nullptr : IdleAnim.LoadSynchronous();
	return Seq;
}

bool UMCBodyAnimComponent::ShouldLoop(EMCBodyAnimState S) const
{
	if (S == EMCBodyAnimState::Jump) return bLoopJump;
	if (S == EMCBodyAnimState::Fall) return bLoopFall;
	return true;
}

EMCBodyAnimState UMCBodyAnimComponent::ComputeState(float GroundSpeed, bool bFalling, float VelZ) const
{
	if (const AMCBayCharacter* SC = Cast<AMCBayCharacter>(GetOwner()))
	{
		if (SC->SeatedStation) return bWorking_ ? EMCBodyAnimState::Work : EMCBodyAnimState::Sit;
	}
	if (bFalling) return VelZ > 50.f ? EMCBodyAnimState::Jump : EMCBodyAnimState::Fall;
	if (GroundSpeed >= RunSpeed) return EMCBodyAnimState::Run;
	if (GroundSpeed >= WalkSpeed) return EMCBodyAnimState::Walk;
	return EMCBodyAnimState::Idle;
}

void UMCBodyAnimComponent::EnterState(EMCBodyAnimState NewState)
{
	CurrentState = NewState;
	if (!BodySkel) return;
	UAnimSequence* Seq = GetAnimFor(NewState);
	if (!Seq)
	{
		bPlayingAsset = false;
		return;
	}
	if (BodySkel->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
		BodySkel->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	BodySkel->SetAnimation(Seq);
	BodySkel->SetPlayRate(1.f);
	BodySkel->PlayAnimation(Seq, ShouldLoop(NewState));
	bPlayingAsset = true;
}

void UMCBodyAnimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner || Owner->GetNetMode() == NM_DedicatedServer) return;
	if (!BodyComp) { FindBody(); if (!BodyComp) return; }

	float GroundSpeed = 0.f, VelZ = 0.f;
	bool bFalling = false;
	if (const ACharacter* C = Cast<ACharacter>(Owner))
	{
		const FVector V = C->GetVelocity();
		GroundSpeed = V.Size2D();
		VelZ = V.Z;
		if (const UCharacterMovementComponent* M = C->GetCharacterMovement()) bFalling = M->IsFalling();
	}
	else
	{
		const FVector V = Owner->GetVelocity();
		GroundSpeed = V.Size2D();
		VelZ = V.Z;
	}

	const EMCBodyAnimState NewState = ComputeState(GroundSpeed, bFalling, VelZ);
	if (!bAnimTried || NewState != CurrentState)
	{
		bAnimTried = true;
		EnterState(NewState);
	}

	// Walk/Run play rate follows speed.
	if (bPlayingAsset && BodySkel && (CurrentState == EMCBodyAnimState::Walk || CurrentState == EMCBodyAnimState::Run))
	{
		const bool bRunClip = CurrentState == EMCBodyAnimState::Run && !RunAnim.IsNull();
		const float Ref = FMath::Max(1.f, bRunClip ? RunRefSpeed : WalkRefSpeed);
		BodySkel->SetPlayRate(FMath::Clamp(GroundSpeed / Ref, MinPlayRate, MaxPlayRate));
	}

	if (bProceduralFallback && !bPlayingAsset)
		ApplyProcedural(DeltaTime, GroundSpeed, CurrentState);
	else if (bHaveBase && BodyComp)
	{
		// Asset playing: make sure a previous procedural offset is cleared.
		if (!BodyComp->GetRelativeLocation().Equals(BaseLoc, 0.01f)) BodyComp->SetRelativeLocation(BaseLoc);
		if (!BodyComp->GetRelativeRotation().Equals(BaseRot, 0.01f)) BodyComp->SetRelativeRotation(BaseRot);
	}
}

void UMCBodyAnimComponent::ApplyProcedural(float DeltaTime, float GroundSpeed, EMCBodyAnimState S)
{
	if (!BodyComp || !bHaveBase) return;
	FVector Off = FVector::ZeroVector;
	FRotator Rot = FRotator::ZeroRotator;

	if (S == EMCBodyAnimState::Walk || S == EMCBodyAnimState::Run)
	{
		const float Rate = FMath::Clamp(GroundSpeed / 150.f, 0.6f, 2.5f);
		PhaseTime += DeltaTime * Rate * 2.f * PI * 1.6f; // ~1.6 steps/s at ref speed
		Off.Z = FMath::Abs(FMath::Sin(PhaseTime)) * WalkBobAmplitude;
		Rot.Roll = FMath::Sin(PhaseTime) * WalkSwayDegrees;
		Rot.Pitch = WalkLeanDegrees * FMath::Clamp(GroundSpeed / 450.f, 0.f, 1.f);
	}
	else if (S == EMCBodyAnimState::Idle || S == EMCBodyAnimState::Sit || S == EMCBodyAnimState::Work)
	{
		PhaseTime += DeltaTime * IdleBobSpeed;
		Off.Z = FMath::Sin(PhaseTime) * IdleBobAmplitude;
		Rot.Roll = FMath::Sin(PhaseTime * 0.5f) * 0.8f;
	}
	// Jump / Fall: no offset.

	// Smooth towards the target pose so state changes do not pop.
	const FVector CurOff = BodyComp->GetRelativeLocation() - BaseLoc;
	const FRotator CurRot = (BodyComp->GetRelativeRotation() - BaseRot).GetNormalized();
	const float A = FMath::Clamp(DeltaTime * 12.f, 0.f, 1.f);
	BodyComp->SetRelativeLocation(BaseLoc + FMath::Lerp(CurOff, Off, A));
	BodyComp->SetRelativeRotation(BaseRot + FMath::Lerp(CurRot, Rot, A));
}
