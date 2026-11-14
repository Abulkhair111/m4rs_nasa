#include "Bay/MCPartMover.h"
#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"
#include "Construction/MCPartSocket.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"

// ------------------------------------------------------------------ AMCPartActor

AMCPartActor::AMCPartActor()
{
	bReplicates = true;
	SetReplicatingMovement(true);
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(Root);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetMobility(EComponentMobility::Movable);
}

void AMCPartActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCPartActor, Def);
}

void AMCPartActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyDef();
}

void AMCPartActor::SetDef(UMCPartDef* NewDef)
{
	Def = NewDef;
	ApplyDef();
}

void AMCPartActor::OnRep_Def() { ApplyDef(); }

float AMCPartActor::BottomOffset(const UMCPartDef* D)
{
	if (!D || !D->Mesh) return 0.f;
	const FBoxSphereBounds B = D->Mesh->GetBounds();
	return (B.BoxExtent.Z - B.Origin.Z) * D->MeshScale.Z;
}

void AMCPartActor::ApplyDef()
{
	if (!Def || !MeshComp) return;
	MeshComp->SetStaticMesh(Def->Mesh);
	MeshComp->SetRelativeScale3D(Def->MeshScale);
	if (Def->Color.A > 0.f && MeshComp->GetNumMaterials() > 0)
	{
		if (UMaterialInstanceDynamic* MID = MeshComp->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Def->Color);
		}
	}
}

// ------------------------------------------------------------------ AMCPartMover

namespace
{
	double ServerNow(const UWorld* W)
	{
		if (const AGameStateBase* GS = W ? W->GetGameState() : nullptr)
		{
			return GS->GetServerWorldTimeSeconds();
		}
		return W ? W->GetTimeSeconds() : 0.0;
	}

	TSet<TWeakObjectPtr<AMCConstructionManager>>& BoundManagers()
	{
		static TSet<TWeakObjectPtr<AMCConstructionManager>> Set;
		return Set;
	}
}

AMCPartMover::AMCPartMover()
{
	bReplicates = true;
	SetReplicatingMovement(false); // every machine computes the pose from server time
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComp->SetupAttachment(Root);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetMobility(EComponentMobility::Movable);
}

void AMCPartMover::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCPartMover, Def);
	DOREPLIFETIME(AMCPartMover, Socket);
	DOREPLIFETIME(AMCPartMover, StartLoc);
	DOREPLIFETIME(AMCPartMover, EndLoc);
	DOREPLIFETIME(AMCPartMover, StartTime);
	DOREPLIFETIME(AMCPartMover, Duration);
	DOREPLIFETIME(AMCPartMover, ArcHeight);
	DOREPLIFETIME(AMCPartMover, bHeavy);
}

void AMCPartMover::BeginPlay()
{
	Super::BeginPlay();
	ApplyMesh();
}

void AMCPartMover::ApplyMesh()
{
	if (bMeshApplied || !Def || !Def->Mesh) return;
	MeshComp->SetStaticMesh(Def->Mesh);
	MeshComp->SetRelativeScale3D(Def->MeshScale);
	bMeshApplied = true;
}

void AMCPartMover::BindToManager(AMCConstructionManager* Manager)
{
	if (!Manager || !Manager->HasAuthority()) return;
	TWeakObjectPtr<AMCConstructionManager> Key(Manager);
	if (BoundManagers().Contains(Key)) return;
	BoundManagers().Add(Key);
	Manager->OnPlacementApproved.AddStatic(&AMCPartMover::HandlePlacementApproved);
}

void AMCPartMover::HandlePlacementApproved(AMCPartSocket* Socket, UMCPartDef* Def, EMCCrewRole InRole, bool bHeavy)
{
	Launch(Socket, Def, InRole, bHeavy);
}

AMCPartMover* AMCPartMover::Launch(AMCPartSocket* Socket, UMCPartDef* Def, EMCCrewRole InRole, bool bHeavy)
{
	if (!Socket || !Def) return nullptr;
	UWorld* World = Socket->GetWorld();
	if (!World) return nullptr;

	const FVector End = Socket->GetActorLocation() + FVector(0, 0, AMCPartActor::BottomOffset(Def));
	FVector Start = End + FVector(0, 0, 500.f);
	float Arc = 0.f;
	float Dur = 2.6f;

	if (!bHeavy)
	{
		Dur = 1.4f;
		Arc = 155.f;
		// Matching-department shelf (levels name them Shelf_PowerComms / Shelf_Propulsion / Shelf_Science).
		const TCHAR* Needle = nullptr;
		switch (Def->Department)
		{
		case 1: Needle = TEXT("Shelf_PowerComms"); break;
		case 3: Needle = TEXT("Shelf_Propulsion"); break;
		case 4: Needle = TEXT("Shelf_Science"); break;
		default: break;
		}
		bool bFound = false;
		if (Needle)
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->GetName().Contains(Needle))
				{
					Start = It->GetActorLocation() + FVector(0, 0, 120.f);
					bFound = true;
					break;
				}
			}
		}
		if (!bFound)
		{
			Start = End + FVector(400.f, 0, 120.f); // no shelf found: fly in from the side
		}
	}

	AMCPartMover* M = World->SpawnActorDeferred<AMCPartMover>(AMCPartMover::StaticClass(), FTransform(Socket->GetActorRotation(), Start), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!M) return nullptr;
	M->Def = Def;
	M->Socket = Socket;
	M->StartLoc = Start;
	M->EndLoc = End;
	M->StartTime = ServerNow(World);
	M->Duration = Dur;
	M->ArcHeight = Arc;
	M->bHeavy = bHeavy;
	M->PlacedRole = InRole;
	M->FinishSpawning(FTransform(Socket->GetActorRotation(), Start));
	return M;
}

void AMCPartMover::UpdatePose(float Alpha)
{
	const float S = Alpha * Alpha * (3.f - 2.f * Alpha);
	FVector Pos = FMath::Lerp(StartLoc, EndLoc, (double)S);
	Pos.Z += ArcHeight * FMath::Sin(PI * Alpha);
	SetActorLocation(Pos);
}

void AMCPartMover::DrawCrane(float Alpha) const
{
#if ENABLE_DRAW_DEBUG
	UWorld* W = GetWorld();
	if (!W) return;
	const FVector Anchor = EndLoc + FVector(0, 0, CraneHeight);
	const FVector Hook = GetActorLocation() + FVector(0, 0, 90.f);
	// Trolley rail, cable, hook, slings.
	DrawDebugLine(W, Anchor + FVector(-250, 0, 0), Anchor + FVector(250, 0, 0), FColor(255, 200, 0), false, 0.f, 0, 6.f);
	DrawDebugLine(W, Anchor, Hook + FVector(0, 0, 40), FColor(220, 220, 220), false, 0.f, 0, 3.f);
	DrawDebugSphere(W, Hook + FVector(0, 0, 40), 14.f, 8, FColor(255, 200, 0), false, 0.f, 0, 3.f);
	DrawDebugLine(W, Hook + FVector(0, 0, 40), Hook + FVector(-45, 0, -20), FColor(255, 200, 0), false, 0.f, 0, 2.f);
	DrawDebugLine(W, Hook + FVector(0, 0, 40), Hook + FVector(45, 0, -20), FColor(255, 200, 0), false, 0.f, 0, 2.f);
#endif
}

void AMCPartMover::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDone) return;
	if (!bMeshApplied) ApplyMesh();
	if (Duration <= KINDA_SMALL_NUMBER) return;

	const double Now = ServerNow(GetWorld());
	const float Alpha = (float)FMath::Clamp((Now - StartTime) / Duration, 0.0, 1.0);
	UpdatePose(Alpha);
	if (bHeavy && GetNetMode() != NM_DedicatedServer) DrawCrane(Alpha);

	if (HasAuthority() && Alpha >= 1.f)
	{
		bDone = true;
		Finish();
	}
}

void AMCPartMover::Finish()
{
	AMCConstructionManager* Mgr = MCBayUI::FindManager(GetWorld());
	if (Socket && Def && Mgr)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AMCPartActor* Part = GetWorld()->SpawnActor<AMCPartActor>(AMCPartActor::StaticClass(), EndLoc, Socket->GetActorRotation(), P);
		if (Part)
		{
			Part->SetDef(Def);
			Part->AttachToActor(Socket, FAttachmentTransformRules::KeepWorldTransform);
		}
		Mgr->FinishPlacement(Socket, Def, Part, PlacedRole);
	}
	else if (Socket)
	{
		Socket->SetReserved(false); // do not leave the socket locked
	}
	Destroy();
}
