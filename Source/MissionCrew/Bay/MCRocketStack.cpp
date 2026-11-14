#include "Bay/MCRocketStack.h"
#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"
#include "Construction/MCPartSocket.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Algo/StableSort.h"

// ------------------------------------------------------------------ slot library

int32 UMCRocketStackLibrary::CoreRank(EMCStackKind Kind)
{
	switch (Kind)
	{
	case EMCStackKind::Engine:    return 0;
	case EMCStackKind::Tank:      return 1;
	case EMCStackKind::Decoupler: return 2;
	case EMCStackKind::Other:     return 3;
	case EMCStackKind::Capsule:   return 4;
	case EMCStackKind::Fairing:   return 5;
	default:                      return 3;
	}
}

TArray<FMCStackSlot> UMCRocketStackLibrary::GetAscentSlots(const FMCStackLayoutParams& P)
{
	TArray<FMCStackSlot> Out;
	int32 Core = 0;
	auto AddCore = [&](const TCHAR* Kind, int32 KindIdx, EMCSocketType Type)
	{
		FMCStackSlot S;
		S.Name = FName(*FString::Printf(TEXT("Ascent_%s_%d"), Kind, KindIdx));
		S.Index = Core;
		S.bRadial = false;
		S.SocketType = Type;
		S.LocalOffset = P.CoreColumnOffset + FVector(0, 0, P.CoreSpacingCm * Core);
		Out.Add(S);
		++Core;
	};
	for (int32 i = 0; i < P.NumEngineSlots; ++i)    AddCore(TEXT("Engine"), i, EMCSocketType::Engine);
	for (int32 i = 0; i < P.NumTankSlots; ++i)      AddCore(TEXT("Tank"), i, EMCSocketType::Tank);
	for (int32 i = 0; i < P.NumStructureSlots; ++i) AddCore(TEXT("Structure"), i, EMCSocketType::Structure);

	for (int32 i = 0; i < P.NumRadialSlots; ++i)
	{
		const float Yaw = 360.f * i / FMath::Max(1, P.NumRadialSlots);
		const float Rad = FMath::DegreesToRadians(Yaw);
		FMCStackSlot S;
		S.Name = FName(*FString::Printf(TEXT("Ascent_Booster_%d"), i));
		S.Index = i;
		S.bRadial = true;
		S.SocketType = EMCSocketType::Booster;
		S.YawDeg = Yaw;
		S.LocalOffset = FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.f) * P.RadialRingRadiusCm + FVector(0, 0, P.RadialHeightCm);
		Out.Add(S);
	}
	return Out;
}

TArray<AMCPartSocket*> UMCRocketStackLibrary::SpawnAscentSockets(UObject* WorldContextObject, const FTransform& Origin, const FMCStackLayoutParams& Params, TSubclassOf<AMCPartSocket> SocketClass)
{
	TArray<AMCPartSocket*> Out;
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World) return Out;
	if (World->GetNetMode() == NM_Client) return Out; // sockets are server-owned, replicated
	UClass* Cls = SocketClass ? SocketClass.Get() : AMCPartSocket::StaticClass();

	for (const FMCStackSlot& Slot : GetAscentSlots(Params))
	{
		const FTransform Rel(FRotator(0.f, Slot.YawDeg, 0.f), Slot.LocalOffset);
		const FTransform T = Rel * Origin;
		AMCPartSocket* S = World->SpawnActorDeferred<AMCPartSocket>(Cls, T, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!S) continue;
		S->SocketType = Slot.SocketType;
		S->SocketGroup = EMCStackGroup::Ascent;
		S->Tags.AddUnique(TEXT("AscentSlot"));
		S->Tags.AddUnique(Slot.Name);
		S->FinishSpawning(T);
#if WITH_EDITOR
		S->SetActorLabel(Slot.Name.ToString());
#endif
		Out.Add(S);
	}
	return Out;
}

// ------------------------------------------------------------------ AMCRocketStack

AMCRocketStack::AMCRocketStack()
{
	bReplicates = true;     // state is derived from replicated sockets; nothing of its own to replicate
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = true;
	StackRoot = CreateDefaultSubobject<USceneComponent>(TEXT("StackRoot"));
	SetRootComponent(StackRoot);
	StackRoot->SetMobility(EComponentMobility::Movable);
	GhostMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent")));
}

AMCRocketStack* AMCRocketStack::Find(const UWorld* World)
{
	if (!World) return nullptr;
	for (TActorIterator<AMCRocketStack> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (!It->bDisplayMode) return *It;
	}
	return nullptr;
}

AMCRocketStack* AMCRocketStack::FindDisplay(const UWorld* World)
{
	if (!World) return nullptr;
	for (TActorIterator<AMCRocketStack> It(const_cast<UWorld*>(World)); It; ++It)
	{
		if (It->bDisplayMode) return *It;
	}
	return nullptr;
}

const UMCPartDef* AMCRocketStack::EffGhost() const
{
	if (bDisplayMode) { const AMCRocketStack* S = Source.Get(); return S ? S->Ghost.Get() : nullptr; }
	return Ghost.Get();
}

AMCPartSocket* AMCRocketStack::EffGhostReplaces() const
{
	if (bDisplayMode) { const AMCRocketStack* S = Source.Get(); return S ? S->GhostReplaces.Get() : nullptr; }
	return GhostReplaces.Get();
}

const UMCRocketPreset* AMCRocketStack::EffPreset() const
{
	const AMCConstructionManager* M = Mgr.Get();
	return M ? M->GetSelectedPreset() : nullptr;
}

void AMCRocketStack::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoSpawnSockets && HasAuthority())
	{
		bool bHave = false;
		for (TActorIterator<AMCPartSocket> It(GetWorld()); It; ++It)
		{
			if (It->GetGroup() == EMCStackGroup::Ascent) { bHave = true; break; }
		}
		if (!bHave)
		{
			AMCConstructionManager* Manager = MCBayUI::FindManager(GetWorld());
			for (AMCPartSocket* S : UMCRocketStackLibrary::SpawnAscentSockets(this, GetActorTransform(), SocketLayout, SocketClass))
			{
				if (Manager) Manager->RegisterSocket(S);
			}
		}
	}
	RefreshSocketCache();
}

void AMCRocketStack::RefreshSocketCache()
{
	CachedSockets.Reset();
	if (!GetWorld()) return;
	Mgr = MCBayUI::FindManager(GetWorld());
	if (bDisplayMode)
	{
		Source = Find(GetWorld());
		if (const AMCRocketStack* S = Source.Get())
		{
			// The display shows the same lander as the real stack unless it has its own.
			if (LanderMesh.IsNull() && !S->LanderMesh.IsNull())
			{
				LanderMesh = S->LanderMesh;
				LanderScale = S->LanderScale;
				LanderRotation = S->LanderRotation;
				LanderHeightCm = S->LanderHeightCm;
			}
		}
	}
	TArray<AMCPartSocket*> List;
	for (TActorIterator<AMCPartSocket> It(GetWorld()); It; ++It)
	{
		if (It->GetGroup() == EMCStackGroup::Ascent) List.Add(*It);
	}
	// Same order on every machine: by position, then name.
	List.Sort([](const AMCPartSocket& A, const AMCPartSocket& B)
	{
		const FVector LA = A.GetActorLocation(), LB = B.GetActorLocation();
		const int32 ZA = FMath::RoundToInt(LA.Z), ZB = FMath::RoundToInt(LB.Z);
		if (ZA != ZB) return ZA < ZB;
		const int32 YA = FMath::RoundToInt(LA.Y), YB = FMath::RoundToInt(LB.Y);
		if (YA != YB) return YA < YB;
		const int32 XA = FMath::RoundToInt(LA.X), XB = FMath::RoundToInt(LB.X);
		if (XA != XB) return XA < XB;
		return A.GetName() < B.GetName();
	});
	for (AMCPartSocket* S : List) CachedSockets.Add(S);
}

bool AMCRocketStack::CollectSignature(TArray<const void*>& Out) const
{
	for (const TWeakObjectPtr<AMCPartSocket>& W : CachedSockets)
	{
		const AMCPartSocket* S = W.Get();
		if (!S) continue;
		Out.Add(S);
		Out.Add(S->Installed.Get());
		Out.Add(S->InstalledActor.Get());
	}
	Out.Add(EffGhost());
	Out.Add(EffGhostReplaces());
	Out.Add(EffPreset());
	Out.Add(bShowLander ? LanderMesh.Get() : nullptr);
	return true;
}

// ---- geometry ----

AMCRocketStack::FGeom AMCRocketStack::ComputeGeom(UStaticMesh* Mesh, const FVector& InScale, const FRotator& Rot, float FitHeight, float FitRadius, float EmptyH, float EmptyR)
{
	FGeom G;
	G.Mesh = Mesh;
	G.Rot = Rot;
	G.Scale = InScale;
	if (!Mesh)
	{
		G.Height = FitHeight > 0.f ? FitHeight : EmptyH;
		G.Radius = FitRadius > 0.f ? FitRadius : EmptyR;
		return G;
	}
	const FBoxSphereBounds Bnd = Mesh->GetBounds();
	const FBox Local(Bnd.Origin - Bnd.BoxExtent, Bnd.Origin + Bnd.BoxExtent);
	FBox B = Local.TransformBy(FTransform(Rot, FVector::ZeroVector, G.Scale));
	const float H = B.Max.Z - B.Min.Z;
	if (FitHeight > 0.f && H > KINDA_SMALL_NUMBER)
	{
		G.Scale *= FitHeight / H; // uniform fit, independent of the rotation
		B = Local.TransformBy(FTransform(Rot, FVector::ZeroVector, G.Scale));
	}
	G.Height = B.Max.Z - B.Min.Z;
	G.Radius = FitRadius > 0.f ? FitRadius : 0.5f * FMath::Max(B.Max.X - B.Min.X, B.Max.Y - B.Min.Y);
	const FVector C = B.GetCenter();
	G.Pivot = FVector(-C.X, -C.Y, -B.Min.Z);
	return G;
}

AMCRocketStack::FGeom AMCRocketStack::GeomForDef(const UMCPartDef* Def) const
{
	if (!Def) return FGeom();
	UStaticMesh* M = nullptr;
	FVector Scale = Def->MeshScale;
	if (!Def->StackMesh.IsNull())
	{
		M = Def->StackMesh.LoadSynchronous();
		Scale = Def->StackMeshScale;
	}
	if (!M)
	{
		M = Def->Mesh;
		Scale = Def->MeshScale;
	}
	return ComputeGeom(M, Scale, Def->StackMeshRotation, Def->StackHeightCm, Def->StackRadiusCm, EmptyPartHeightCm, EmptyPartRadiusCm);
}

AMCRocketStack::FGeom AMCRocketStack::GeomForLander() const
{
	UStaticMesh* M = LanderMesh.IsNull() ? nullptr : LanderMesh.LoadSynchronous();
	return ComputeGeom(M, LanderScale, LanderRotation, LanderHeightCm, 0.f, 0.f, 0.f);
}

// ---- rebuild ----

void AMCRocketStack::Rebuild()
{
	struct FDesired
	{
		FEntry E;
		int32 Copy = 0;
		bool bLander = false;
		FGeom G;
		FVector Loc = FVector::ZeroVector;
		bool bRadial = false;
		FVector Outward = FVector::ZeroVector;
	};

	TArray<FEntry> Core, Radial;
	auto AddEntry = [&](const FEntry& E)
	{
		if (E.Def->IsRadialInStack()) Radial.Add(E); else Core.Add(E);
	};
	const UMCPartDef* GhostDef = EffGhost();
	AMCPartSocket* GhostRepl = GhostDef ? EffGhostReplaces() : nullptr;
	// Launcher preset hardware first (no socket): the core stage under the players' parts, and the main side boosters.
	if (const UMCRocketPreset* Preset = EffPreset())
	{
		int32 Ord = 0;
		for (const UMCPartDef* D : Preset->CoreParts)
		{
			if (!D) continue;
			FEntry E; E.Def = D; E.bPreset = true; E.Order = Ord++;
			Core.Add(E);
		}
		if (Preset->RadialPart && Preset->RadialCount > 0)
		{
			FEntry E; E.Def = Preset->RadialPart; E.bPreset = true; E.Count = Preset->RadialCount;
			Radial.Add(E);
		}
	}
	for (const TWeakObjectPtr<AMCPartSocket>& W : CachedSockets)
	{
		AMCPartSocket* S = W.Get();
		if (!S || !S->Installed || S->Installed->GetStackGroup() != EMCStackGroup::Ascent) continue;
		if (S == GhostRepl) continue; // swap preview: the ghost takes its place
		FEntry E; E.Def = S->Installed; E.Socket = S;
		AddEntry(E);
	}
	if (GhostDef)
	{
		FEntry E; E.Def = GhostDef; E.bGhost = true;
		AddEntry(E);
	}
	Algo::StableSortBy(Core, [](const FEntry& E) { return E.bPreset ? -1000 + E.Order : UMCRocketStackLibrary::CoreRank(E.Def->GetEffectiveStackKind()); });

	TArray<FDesired> Want;
	float Z = 0.f, CoreRadius = 0.f, RealZ = 0.f;
	bool bGhostInCore = false;
	for (const FEntry& E : Core)
	{
		FDesired D; D.E = E; D.G = GeomForDef(E.Def);
		D.Loc = FVector(D.G.Pivot.X, D.G.Pivot.Y, Z + D.G.Pivot.Z);
		Z += D.G.Height;
		if (!E.bGhost) { RealZ += D.G.Height; CoreRadius = FMath::Max(CoreRadius, D.G.Radius); }
		else bGhostInCore = true;
		Want.Add(D);
	}
	if (CoreRadius <= 0.f) CoreRadius = DefaultCoreRadiusCm;
	if (bShowLander)
	{
		FDesired D; D.bLander = true; D.G = GeomForLander();
		if (D.G.Mesh)
		{
			D.Loc = FVector(D.G.Pivot.X, D.G.Pivot.Y, Z + D.G.Pivot.Z);
			Z += D.G.Height; RealZ += D.G.Height;
			Want.Add(D);
		}
	}
	// Note: the lander shifts by the ghost height too (Z includes it); RealZ does not.
	(void)bGhostInCore;

	// Radial copies, spread evenly around the core.
	auto CopiesOf = [](const FEntry& E) { return E.Count > 0 ? E.Count : FMath::Max(1, E.Def->RadialCount); };
	int32 TotalCopies = 0;
	for (const FEntry& E : Radial) TotalCopies += CopiesOf(E);
	int32 Running = 0;
	float RadialTop = 0.f;
	for (const FEntry& E : Radial)
	{
		const FGeom G = GeomForDef(E.Def);
		const int32 N = CopiesOf(E);
		for (int32 c = 0; c < N; ++c, ++Running)
		{
			const float Ang = FMath::DegreesToRadians(RadialStartYawDeg + 360.f * Running / FMath::Max(1, TotalCopies));
			FDesired D; D.E = E; D.Copy = c; D.G = G; D.bRadial = true;
			D.Outward = FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.f);
			D.Loc = D.Outward * (CoreRadius + G.Radius) + FVector(G.Pivot.X, G.Pivot.Y, G.Pivot.Z);
			Want.Add(D);
		}
		RadialTop = FMath::Max(RadialTop, G.Height);
	}
	TotalHeightCm = FMath::Max(Z, RadialTop);
	RealHeightCm = RealZ;

	// Match existing visuals so unchanged parts keep their position (and animate when only the target moved).
	TArray<bool> Used; Used.Init(false, Visuals.Num());
	TArray<FMCStackVisual> NewVis;
	for (FDesired& D : Want)
	{
		int32 Found = INDEX_NONE;
		for (int32 i = 0; i < Visuals.Num(); ++i)
		{
			const FMCStackVisual& V = Visuals[i];
			if (!Used[i] && V.Comp && V.bGhost == D.E.bGhost && V.bLander == D.bLander && V.bPreset == D.E.bPreset && V.Copy == D.Copy
				&& V.Socket.Get() == D.E.Socket && V.Def.Get() == D.E.Def)
			{
				Found = i; break;
			}
		}
		FMCStackVisual V;
		if (Found != INDEX_NONE)
		{
			Used[Found] = true;
			V = Visuals[Found];
		}
		else
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
			C->SetMobility(EComponentMobility::Movable);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetupAttachment(StackRoot);
			C->RegisterComponent();
			V.Comp = C;
			V.Socket = D.E.Socket;
			V.Def = D.E.Def;
			V.Copy = D.Copy;
			V.bGhost = D.E.bGhost;
			V.bLander = D.bLander;
			V.bPreset = D.E.bPreset;
			const FVector Drop = D.E.bGhost ? FVector::ZeroVector
				: (D.bRadial ? D.Outward * 500.f + FVector(0, 0, DropHeightCm * 0.4f) : FVector(0, 0, DropHeightCm));
			V.Cur = D.Loc + Drop;
			V.Comp->SetRelativeLocation(V.Cur);
		}
		V.Target = D.Loc;
		ApplyVisual(V, D.G.Mesh, D.G.Scale, D.G.Rot);
		NewVis.Add(V);
	}
	for (int32 i = 0; i < Visuals.Num(); ++i)
	{
		if (Used[i] || !Visuals[i].Comp) continue;
		const FMCStackVisual& Old = Visuals[i];
		if (bAnimateSwap && !Old.bGhost && Old.Comp->GetStaticMesh() && LiftOutTimeSec > 0.f)
		{
			// Swap animation: the old piece lifts out (and is destroyed in Tick) while the new one lowers in.
			FMCLeavingVisual L;
			L.Comp = Old.Comp;
			L.Cur = Old.Cur;
			L.Target = Old.Cur + FVector(0.f, 0.f, LiftOutHeightCm);
			L.TimeLeft = LiftOutTimeSec;
			Leaving.Add(L);
		}
		else
		{
			Old.Comp->DestroyComponent();
		}
	}
	Visuals = MoveTemp(NewVis);
	HideSocketActors();
}

void AMCRocketStack::ApplyVisual(FMCStackVisual& V, UStaticMesh* Mesh, const FVector& Scale, const FRotator& Rot)
{
	if (!V.Comp) return;
	if (V.Comp->GetStaticMesh() != Mesh) V.Comp->SetStaticMesh(Mesh);
	V.Comp->SetRelativeScale3D(Scale);
	V.Comp->SetRelativeRotation(Rot);
	V.Comp->SetVisibility(Mesh != nullptr);
	if (V.bGhost && Mesh)
	{
		if (UMaterialInterface* GM = GhostMaterial.IsNull() ? nullptr : GhostMaterial.LoadSynchronous())
		{
			for (int32 i = 0; i < V.Comp->GetNumMaterials(); ++i) V.Comp->SetMaterial(i, GM);
		}
		V.Comp->SetCastShadow(false);
	}
}

void AMCRocketStack::HideSocketActors()
{
	if (!bHideSocketPartActors) return;
	for (const TWeakObjectPtr<AMCPartSocket>& W : CachedSockets)
	{
		const AMCPartSocket* S = W.Get();
		if (S && S->InstalledActor) S->InstalledActor->SetActorHiddenInGame(true);
	}
}

void AMCRocketStack::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetNetMode() == NM_DedicatedServer) return;

	SocketCacheTimer -= DeltaSeconds;
	if (SocketCacheTimer <= 0.f)
	{
		SocketCacheTimer = 1.f;
		RefreshSocketCache();
	}

	TArray<const void*> Sig;
	CollectSignature(Sig);
	if (bDirty || Sig != LastSignature)
	{
		LastSignature = MoveTemp(Sig);
		bDirty = false;
		Rebuild();
	}

	float TargetScale = DisplayScale;
	if (bAutoFitHeight && TotalHeightCm > 1.f)
	{
		TargetScale = FMath::Min(TargetScale, MaxStackHeightCm / TotalHeightCm);
	}
	if (!bScaleInit) { CurScale = TargetScale; bScaleInit = true; }
	else CurScale = FMath::FInterpTo(CurScale, TargetScale, DeltaSeconds, ScaleInterpSpeed);
	StackRoot->SetRelativeScale3D(FVector(CurScale));

	for (int32 i = Leaving.Num() - 1; i >= 0; --i)
	{
		FMCLeavingVisual& L = Leaving[i];
		L.TimeLeft -= DeltaSeconds;
		if (!L.Comp || L.TimeLeft <= 0.f)
		{
			if (L.Comp) L.Comp->DestroyComponent();
			Leaving.RemoveAtSwap(i);
			continue;
		}
		L.Cur = FMath::VInterpTo(L.Cur, L.Target, DeltaSeconds, PartInterpSpeed);
		L.Comp->SetRelativeLocation(L.Cur);
	}

	const double T = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	for (FMCStackVisual& V : Visuals)
	{
		if (!V.Comp) continue;
		V.Cur = FMath::VInterpTo(V.Cur, V.Target, DeltaSeconds, PartInterpSpeed);
		V.Comp->SetRelativeLocation(V.Cur);
		if (V.bGhost && V.Comp->GetStaticMesh())
		{
			// Without a translucent material the ghost pulses so it reads as a preview.
			if (GhostMaterial.IsNull() || !GhostMaterial.IsValid())
			{
				V.Comp->SetVisibility(FMath::Sin(T * 7.0) > -0.4);
			}
		}
	}
}

void AMCRocketStack::ShowGhost(const UMCPartDef* Def)
{
	if (Def && (Def->IsTool() || Def->GetStackGroup() != EMCStackGroup::Ascent)) Def = nullptr;
	if (Ghost.Get() == Def) return;
	Ghost = Def;
	bDirty = true;
}

void AMCRocketStack::SetGhostReplaces(AMCPartSocket* Socket)
{
	if (GhostReplaces.Get() == Socket) return;
	GhostReplaces = Socket;
	bDirty = true;
}

void AMCRocketStack::ClearGhost()
{
	if (!Ghost.IsValid() && !GhostReplaces.IsValid()) return;
	Ghost = nullptr;
	GhostReplaces = nullptr;
	bDirty = true;
}

bool AMCRocketStack::TracePart(const FVector& Eye, const FVector& Dir, float MaxDist, FMCStackHit& Out) const
{
	const float Reach = FMath::Min(MaxDist, AimRangeCm);
	const FVector End = Eye + Dir * Reach;
	bool bFound = false;
	float Best = Reach;
	for (const FMCStackVisual& V : Visuals)
	{
		if (!V.Comp || V.bGhost || V.bLander || !V.Comp->GetStaticMesh()) continue;
		if (!V.bPreset && !V.Socket.IsValid()) continue;
		const FBox Box = V.Comp->Bounds.GetBox().ExpandBy(8.0);
		FVector HitLoc, HitNormal; float HitTime = 0.f;
		if (!FMath::LineExtentBoxIntersection(Box, Eye, End, FVector::ZeroVector, HitLoc, HitNormal, HitTime)) continue;
		const float D = FVector::Dist(Eye, HitLoc);
		if (D < Best || !bFound)
		{
			if (D > Reach) continue;
			Best = D;
			bFound = true;
			Out.Socket = V.Socket.Get();
			Out.Def = V.Def.Get();
			Out.bPreset = V.bPreset;
			Out.Dist = D;
		}
	}
	return bFound;
}
