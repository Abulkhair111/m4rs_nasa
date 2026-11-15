#include "Samples/MCSamplesManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Samples/MCSamplesRelay.h"
#include "Samples/UI/MCSamplesPanel.h"
#include "Core/MCMissionSubsystem.h"
#include "Transit/MCTransitManager.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCSamples, Log, All);

namespace
{
	UMCSamplesConfig* MakeDefaultConfig(UObject* Outer)
	{
		UMCSamplesConfig* C = NewObject<UMCSamplesConfig>(Outer);
		auto Add = [&](const TCHAR* N, float Mass, float Pwr, bool bAct, bool bFrozen, float Leak, float Eff)
		{
			FMCSamplesContainer& X = C->Containers.AddDefaulted_GetRef();
			X.Name = N; X.MassKg = Mass; X.BasePowerW = Pwr; X.bActive = bAct; X.bSupportsFrozen = bFrozen; X.LeakPerSec = Leak; X.CoolEff = Eff;
		};
		Add(TEXT("PASSIVE COLD BOX"), 4.f, 0.f, false, false, 0.002f, 0.f);
		Add(TEXT("ACTIVE COOLER"), 12.f, 25.f, true, true, 0.010f, 1.f);
		Add(TEXT("HYBRID CASE"), 8.f, 12.f, true, true, 0.006f, 0.6f);
		auto Mode = [&](const TCHAR* N, float R, float P) { FMCCoolingMode& M = C->Modes.AddDefaulted_GetRef(); M.Name = N; M.CoolRate = R; M.PowerW = P; };
		Mode(TEXT("OFF"), 0.f, 0.f);
		Mode(TEXT("ECO"), 0.3f, 20.f);
		Mode(TEXT("NOMINAL"), 0.8f, 50.f);
		Mode(TEXT("BOOST"), 1.6f, 90.f);
		Mode(TEXT("BACKUP"), 0.6f, 35.f);
		return C;
	}
}

AMCSamplesManager::AMCSamplesManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	USceneComponent* R = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(R);
}

void AMCSamplesManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCSamplesManager, bFreeRoles);
	DOREPLIFETIME(AMCSamplesManager, bStarted);
	DOREPLIFETIME(AMCSamplesManager, bDone);
	DOREPLIFETIME(AMCSamplesManager, Seed);
	DOREPLIFETIME(AMCSamplesManager, ElapsedSec);
	DOREPLIFETIME(AMCSamplesManager, Samples);
	DOREPLIFETIME(AMCSamplesManager, PendType);
	DOREPLIFETIME(AMCSamplesManager, PendCond);
	DOREPLIFETIME(AMCSamplesManager, WaitSci);
	DOREPLIFETIME(AMCSamplesManager, WaitTech);
	DOREPLIFETIME(AMCSamplesManager, WaitCargo);
	DOREPLIFETIME(AMCSamplesManager, RoutineReady);
	DOREPLIFETIME(AMCSamplesManager, ContainerIdx);
	DOREPLIFETIME(AMCSamplesManager, ContainerNames);
	DOREPLIFETIME(AMCSamplesManager, ModeNames);
	DOREPLIFETIME(AMCSamplesManager, CoolMode);
	DOREPLIFETIME(AMCSamplesManager, TempC);
	DOREPLIFETIME(AMCSamplesManager, LimitC);
	DOREPLIFETIME(AMCSamplesManager, HoldLeftSec);
	DOREPLIFETIME(AMCSamplesManager, MassKg);
	DOREPLIFETIME(AMCSamplesManager, PowerW);
	DOREPLIFETIME(AMCSamplesManager, PowerBudgetW);
	DOREPLIFETIME(AMCSamplesManager, InRangeFrac);
	DOREPLIFETIME(AMCSamplesManager, EventKind);
	DOREPLIFETIME(AMCSamplesManager, bEventActive);
	DOREPLIFETIME(AMCSamplesManager, bEventFixed);
	DOREPLIFETIME(AMCSamplesManager, bStorageUpdated);
	DOREPLIFETIME(AMCSamplesManager, Message);
	DOREPLIFETIME(AMCSamplesManager, Score);
	DOREPLIFETIME(AMCSamplesManager, SamplesOk);
	DOREPLIFETIME(AMCSamplesManager, RoleGood);
	DOREPLIFETIME(AMCSamplesManager, RoleBad);
	DOREPLIFETIME(AMCSamplesManager, ReportText);
}

const UMCSamplesConfig& AMCSamplesManager::Cfg() const
{
	if (Config) return *Config;
	AMCSamplesManager* Me = const_cast<AMCSamplesManager*>(this);
	if (!Me->DefaultConfig) Me->DefaultConfig = MakeDefaultConfig(Me);
	return *Me->DefaultConfig;
}

void AMCSamplesManager::BeginPlay()
{
	Super::BeginPlay();
	RoutineReady.Init(false, 5);
	RoleGood.Init(0, 5);
	RoleBad.Init(0, 5);
	if (HasAuthority() && bAutoStart)
	{
		FTimerHandle H;
		GetWorldTimerManager().SetTimer(H, this, &AMCSamplesManager::BeginSamples, 1.f, false);
	}
}

void AMCSamplesManager::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Panel) Panel->RemoveFromParent();
	Panel = nullptr;
	Super::EndPlay(Reason);
}

FString AMCSamplesManager::StorageName(int32 C)
{
	switch (C) { case 0: return TEXT("FROZEN"); case 1: return TEXT("COOL"); case 2: return TEXT("AMBIENT"); }
	return TEXT("-");
}
FString AMCSamplesManager::TypeName(int32 T)
{
	switch (T) { case 0: return TEXT("ICE"); case 1: return TEXT("REGOLITH"); case 2: return TEXT("ROCK"); }
	return TEXT("-");
}

void AMCSamplesManager::DebugLog(const FString& T) const { UE_LOG(LogMCSamples, Log, TEXT("DEBUGTEST %s"), *T); }

void AMCSamplesManager::Say(const FString& T)
{
	Message = T;
	OnRep_Message();
}
void AMCSamplesManager::OnRep_Message() { UE_LOG(LogMCSamples, Verbose, TEXT("S5: %s"), *Message); }

void AMCSamplesManager::Award(int32 Rl, bool bGood)
{
	if (Rl < 1 || Rl > 4) return;
	(bGood ? RoleGood : RoleBad)[Rl]++;
	UMCMissionSubsystem::LogAccuracy(this, 5, Rl, bGood, Message);
}

FMCSample* AMCSamplesManager::FirstAt(EMCSampleStage S)
{
	for (FMCSample& X : Samples) if (X.Stage == S) return &X;
	return nullptr;
}

int32 AMCSamplesManager::CountSecured() const
{
	int32 N = 0;
	for (const FMCSample& X : Samples) if (X.Stage == EMCSampleStage::Secured) ++N;
	return N;
}

const FMCSamplesContainer* AMCSamplesManager::Cont() const
{
	const UMCSamplesConfig& C = Cfg();
	return C.Containers.IsValidIndex(ContainerIdx) ? &C.Containers[ContainerIdx] : nullptr;
}

// ---------------------------------------------------------------- start
void AMCSamplesManager::BeginSamples()
{
	if (!HasAuthority() || bStarted) return;
	const UMCSamplesConfig& C = Cfg();
	Seed = SeedOverride != 0 ? SeedOverride : FMath::Max(1, (int32)(FPlatformTime::Cycles() & 0x7fffffff));
	Stream.Initialize(Seed);

	// sample list from S4 (S4Samples / S4SampleTypes), default 3
	TArray<int32> Types;
	int32 N = 0;
	if (UMCMissionSubsystem* Sub = UMCMissionSubsystem::Get(this))
	{
		N = Sub->S4Samples;
		for (const FString& S : Sub->S4SampleTypes) Types.Add(S.Contains(TEXT("ICE")) ? 0 : S.Contains(TEXT("REGO")) ? 1 : 2);
		if (N <= 0) N = Types.Num();
		Sub->SetMissionSeed(Seed);
	}
	if (SampleCountOverride > 0) N = SampleCountOverride;
	if (N <= 0) { N = C.DefaultSampleCount; Types.Reset(); }
	N = FMath::Clamp(N, 1, 8);

	Samples.Reset();
	TrueType.Reset();
	LeakRoll.Reset();
	for (int32 i = 0; i < N; ++i)
	{
		const int32 T = Types.IsValidIndex(i) ? Types[i] : (i < 3 && Types.Num() == 0 ? i : Stream.RandRange(0, 2));
		FMCSample& S = Samples.AddDefaulted_GetRef();
		S.Id = i + 1;
		switch (T)
		{
		case 0: S.Hint = FString::Printf(TEXT("H2O band %d%%  density %.2f  frost on grains"), Stream.RandRange(70, 94), 0.85f + Stream.FRand() * 0.15f); break;
		case 1: S.Hint = FString::Printf(TEXT("H2O band %d%%  density %.2f  fine grey dust"), Stream.RandRange(1, 7), 1.4f + Stream.FRand() * 0.3f); break;
		default: S.Hint = FString::Printf(TEXT("H2O band 0%%  density %.2f  solid basalt chunk"), 2.8f + Stream.FRand() * 0.3f); break;
		}
		TrueType.Add((uint8)T);
		LeakRoll.Add(Stream.FRand() < C.SealLeakChance);
	}
	EventAt = Stream.FRandRange(C.EventMinSec, C.EventMaxSec);

	ContainerNames.Reset();
	for (const FMCSamplesContainer& X : C.Containers) ContainerNames.Add(X.Name);
	ModeNames.Reset();
	for (const FMCCoolingMode& M : C.Modes) ModeNames.Add(M.Name);
	ContainerIdx = -1;
	CoolMode = 2;
	TempC = C.StartTempC;
	PowerBudgetW = C.PowerBudgetW;
	PendType = PendCond = -1;
	bStarted = true;
	for (float& F : RoutineCD) F = C.RoutineCooldownSec * 0.5f;
	UpdateLimit();
	UpdateBoard();
	Say(FString::Printf(TEXT("%d samples to preserve. Scientist: identify, label, choose storage."), N));
	DebugLog(FString::Printf(TEXT("start seed=%d samples=%d eventAt=%.1f"), Seed, N, EventAt));
	if (bDebugAutoTest) RunDebugTest();
}

void AMCSamplesManager::RunDebugTest()
{
	if (!HasAuthority()) return;
	bFreeRoles = true;
	bDebugBot = true;
	bTravelOnComplete = FParse::Param(FCommandLine::Get(), TEXT("MCTravelTest"));
	if (!bStarted) BeginSamples();
	DebugLog(TEXT("bot started"));
}

// ---------------------------------------------------------------- work / idle
bool AMCSamplesManager::HasWork(int32 Rl) const
{
	auto Has = [&](EMCSampleStage S) { for (const FMCSample& X : Samples) if (X.Stage == S) return true; return false; };
	switch (Rl)
	{
	case MCSamples::Systems: return true;
	case MCSamples::Scientist: return Has(EMCSampleStage::Raw) || (bEventActive && !bStorageUpdated && EventKind == EMCSamplesEvent::CoolerFault);
	case MCSamples::Technician: return Has(EMCSampleStage::Labeled) || Has(EMCSampleStage::Placed) || Has(EMCSampleStage::Sealing) || Has(EMCSampleStage::Sealed);
	case MCSamples::Cargo: return Has(EMCSampleStage::Checked) || Has(EMCSampleStage::Loaded);
	}
	return false;
}

// ---------------------------------------------------------------- actions
bool AMCSamplesManager::HandleAction(int32 Code, int32 Who)
{
	if (!HasAuthority() || !bStarted || bDone) return false;
	const int32 Rl = Code / 100, Act = Code % 100;
	if (Rl < 1 || Rl > 4) return false;
	if (!bFreeRoles && Who != Rl) return false;

	if (Act == MCSamples::ActRoutine)
	{
		if (!RoutineReady[Rl]) return false;
		RoutineReady[Rl] = false;
		RoutineCD[Rl] = Cfg().RoutineCooldownSec;
		static const TCHAR* Names[] = { TEXT(""), TEXT("Telemetry logged."), TEXT("Spectrometer calibrated."), TEXT("Sealer primed."), TEXT("Strap tension inspected.") };
		Say(Names[Rl]);
		Award(Rl, true);
		return true;
	}
	switch (Rl)
	{
	case MCSamples::Systems: return DoSystems(Act);
	case MCSamples::Scientist: return DoScientist(Act);
	case MCSamples::Technician: return DoTech(Act);
	default: return DoCargo(Act);
	}
}

bool AMCSamplesManager::DoSystems(int32 A)
{
	const UMCSamplesConfig& C = Cfg();
	if (A >= 0 && A <= 4)
	{
		const FMCSamplesContainer* K = Cont();
		if (!K || !K->bActive) { Say(TEXT("No active cooler in this container.")); return true; }
		CoolMode = A;
		Say(FString::Printf(TEXT("Cooling mode %s."), *C.Modes[A].Name));
		if (bEventActive && !bEventFixed && EventKind == EMCSamplesEvent::CoolerFault && A == 4)
		{
			bEventFixed = true;
			const bool bFast = ElapsedSec - EventStart <= C.EventResponseSec;
			Say(bFast ? TEXT("Backup cooling online.") : TEXT("Backup cooling online (late)."));
			Award(MCSamples::Systems, bFast);
			bEventResponded = true;
		}
		UpdateBoard();
		return true;
	}
	if (A == 5)
	{
		if (bEventActive && !bEventFixed && EventKind == EMCSamplesEvent::HoldWarning)
		{
			bEventFixed = true;
			bInsulated = true;
			const bool bFast = ElapsedSec - EventStart <= C.EventResponseSec;
			Say(TEXT("Insulation cover deployed."));
			Award(MCSamples::Systems, bFast);
			bEventResponded = true;
		}
		else { Say(TEXT("Nothing to cover.")); Award(MCSamples::Systems, false); }
		return true;
	}
	return false;
}

bool AMCSamplesManager::DoScientist(int32 A)
{
	if (A >= 0 && A <= 2) { PendType = A; return true; }
	if (A >= 3 && A <= 5) { PendCond = A - 3; return true; }
	if (A == 6)
	{
		FMCSample* S = FirstAt(EMCSampleStage::Raw);
		if (!S) { Say(TEXT("No sample waiting.")); return true; }
		if (PendType < 0 || PendCond < 0) { Say(TEXT("Pick a type AND a storage condition first.")); return true; }
		const int32 Idx = S->Id - 1;
		const int32 TT = TrueType.IsValidIndex(Idx) ? TrueType[Idx] : 2;
		const int32 Need = TT; // Ice -> Frozen(0), Regolith -> Cool(1), Rock -> Ambient(2)
		S->ChosenType = PendType;
		S->ChosenCond = PendCond;
		S->Stage = EMCSampleStage::Labeled;
		Say(FString::Printf(TEXT("Sample %d labeled %s / %s."), S->Id, *TypeName(PendType), *StorageName(PendCond)));
		Award(MCSamples::Scientist, PendType == TT);
		Award(MCSamples::Scientist, PendCond == Need);
		PendType = PendCond = -1;
		UpdateLimit();
		UpdateBoard();
		return true;
	}
	if (A == 7)
	{
		if (bEventActive && EventKind == EMCSamplesEvent::CoolerFault && !bStorageUpdated)
		{
			bStorageUpdated = true;
			Say(TEXT("Storage conditions revised for backup cooling."));
			Award(MCSamples::Scientist, true);
			UpdateLimit();
		}
		else { Say(TEXT("No storage update needed.")); Award(MCSamples::Scientist, false); }
		return true;
	}
	return false;
}

bool AMCSamplesManager::DoTech(int32 A)
{
	const UMCSamplesConfig& C = Cfg();
	if (A >= 0 && A <= 2)
	{
		if (!C.Containers.IsValidIndex(A) || A == ContainerIdx) return true;
		bool bRepack = false;
		for (FMCSample& S : Samples)
			if (S.Stage == EMCSampleStage::Placed || S.Stage == EMCSampleStage::Sealing || S.Stage == EMCSampleStage::Sealed || S.Stage == EMCSampleStage::Checked)
			{ S.Stage = EMCSampleStage::Labeled; S.SealProgress = 0.f; bRepack = true; }
		ContainerIdx = A;
		if (!C.Containers[A].bActive) CoolMode = 0;
		else if (CoolMode == 0) CoolMode = 2;
		Say(bRepack ? TEXT("Container swapped: samples must be repacked.") : FString::Printf(TEXT("Container: %s."), *C.Containers[A].Name));
		if (bRepack) Award(MCSamples::Technician, false);
		UpdateBoard();
		return true;
	}
	if (A == 3)
	{
		FMCSample* S = FirstAt(EMCSampleStage::Labeled);
		const FMCSamplesContainer* K = Cont();
		if (!S) { Say(TEXT("No labeled sample waiting.")); return true; }
		if (!K) { Say(TEXT("Choose a container first.")); return true; }
		if (S->ChosenCond == 0 && !K->bSupportsFrozen) { Say(TEXT("This container cannot hold FROZEN samples.")); Award(MCSamples::Technician, false); return true; }
		S->Stage = EMCSampleStage::Placed;
		Say(FString::Printf(TEXT("Sample %d placed."), S->Id));
		if (!bContainerScored) { bContainerScored = true; Award(MCSamples::Technician, true); }
		UpdateBoard();
		return true;
	}
	if (A == 4)
	{
		FMCSample* S = FirstAt(EMCSampleStage::Placed);
		if (!S) { Say(TEXT("No placed sample to seal.")); return true; }
		S->Stage = EMCSampleStage::Sealing;
		S->SealProgress = 0.f;
		Say(FString::Printf(TEXT("Sealing sample %d..."), S->Id));
		return true;
	}
	if (A == 5)
	{
		FMCSample* S = FirstAt(EMCSampleStage::Sealed);
		if (!S) { Say(TEXT("No sealed sample to check yet.")); return true; }
		const int32 Idx = S->Id - 1;
		if (LeakRoll.IsValidIndex(Idx) && LeakRoll[Idx])
		{
			LeakRoll[Idx] = false; // re-seal always succeeds
			S->Stage = EMCSampleStage::Placed;
			S->SealProgress = 0.f;
			Say(FString::Printf(TEXT("LEAK on sample %d! Seal again."), S->Id));
			Award(MCSamples::Technician, false); // bad seal
			Award(MCSamples::Technician, true);   // caught by the check
		}
		else
		{
			S->Stage = EMCSampleStage::Checked;
			Say(FString::Printf(TEXT("Sample %d packaging OK."), S->Id));
			Award(MCSamples::Technician, true);
		}
		UpdateBoard();
		return true;
	}
	return false;
}

bool AMCSamplesManager::DoCargo(int32 A)
{
	if (A == 0)
	{
		FMCSample* S = FirstAt(EMCSampleStage::Checked);
		if (!S) { Say(TEXT("Nothing checked to load.")); return true; }
		S->Stage = EMCSampleStage::Loaded;
		Say(FString::Printf(TEXT("Sample %d loaded."), S->Id));
		Award(MCSamples::Cargo, true);
		UpdateBoard();
		return true;
	}
	if (A == 1)
	{
		FMCSample* S = FirstAt(EMCSampleStage::Loaded);
		if (!S) { Say(TEXT("Nothing loaded to secure.")); return true; }
		S->Stage = EMCSampleStage::Secured;
		Say(FString::Printf(TEXT("Sample %d secured."), S->Id));
		Award(MCSamples::Cargo, true);
		UpdateBoard();
		return true;
	}
	return false;
}

// ---------------------------------------------------------------- simulation
void AMCSamplesManager::UpdateLimit()
{
	const UMCSamplesConfig& C = Cfg();
	float L = C.CondLimitC.IsValidIndex(1) ? C.CondLimitC[1] : 4.f;
	bool bAny = false;
	for (const FMCSample& S : Samples)
		if (S.ChosenCond >= 0 && C.CondLimitC.IsValidIndex(S.ChosenCond))
		{
			L = bAny ? FMath::Min(L, C.CondLimitC[S.ChosenCond]) : C.CondLimitC[S.ChosenCond];
			bAny = true;
		}
	if (bStorageUpdated) L += C.FaultRelaxC;
	LimitC = L;
}

void AMCSamplesManager::UpdateBoard()
{
	const UMCSamplesConfig& C = Cfg();
	const FMCSamplesContainer* K = Cont();
	float Mass = 0.f, Pwr = 0.f;
	if (K)
	{
		Mass = K->MassKg + C.SampleMassKg * Samples.Num();
		Pwr = K->BasePowerW + (K->bActive && C.Modes.IsValidIndex(CoolMode) ? C.Modes[CoolMode].PowerW : 0.f);
	}
	MassKg = Mass; PowerW = Pwr;
	int32 A = 0, B = 0, D = 0;
	for (const FMCSample& S : Samples)
	{
		if (S.Stage == EMCSampleStage::Raw) ++A;
		else if (S.Stage == EMCSampleStage::Labeled || S.Stage == EMCSampleStage::Placed || S.Stage == EMCSampleStage::Sealing || S.Stage == EMCSampleStage::Sealed) ++B;
		else if (S.Stage == EMCSampleStage::Checked || S.Stage == EMCSampleStage::Loaded) ++D;
	}
	WaitSci = A; WaitTech = B; WaitCargo = D;
}

float AMCSamplesManager::LeakNow() const
{
	const FMCSamplesContainer* K = Cont();
	if (!K) return 0.f;
	const UMCSamplesConfig& C = Cfg();
	float L = K->LeakPerSec;
	if (bEventActive && !bEventFixed && EventKind == EMCSamplesEvent::HoldWarning) L *= C.HoldWarnLeakMul;
	if (bInsulated) L *= C.InsulationLeakMul;
	return L;
}

float AMCSamplesManager::CoolRateNow() const
{
	const FMCSamplesContainer* K = Cont();
	const UMCSamplesConfig& C = Cfg();
	if (!K || !K->bActive || !C.Modes.IsValidIndex(CoolMode)) return 0.f;
	if (bEventActive && !bEventFixed && EventKind == EMCSamplesEvent::CoolerFault && CoolMode != 4) return 0.f;
	return C.Modes[CoolMode].CoolRate * K->CoolEff;
}

void AMCSamplesManager::SimTick(float Dt)
{
	const UMCSamplesConfig& C = Cfg();
	if (Cont())
	{
		const float Sat = FMath::Clamp((TempC + 60.f) / 15.f, 0.f, 1.f);
		TempC += LeakNow() * (C.AmbientC - TempC) * Dt - CoolRateNow() * Sat * Dt;
		TempC = FMath::Clamp(TempC, -60.f, C.AmbientC);
	}
	// hold time: time until LimitC is crossed (analytic with the current net rate)
	const float Leak = LeakNow(), Cool = CoolRateNow();
	float Hold = 999.f;
	if (TempC >= LimitC) Hold = 0.f;
	else if (Leak > KINDA_SMALL_NUMBER)
	{
		const float Eq = C.AmbientC - Cool / Leak; // asymptote
		if (Eq > LimitC)
		{
			const float Num = TempC - Eq, Den = LimitC - Eq;
			Hold = (Den != 0.f && Num / Den > 0.f) ? FMath::Min(999.f, FMath::Loge(Num / Den) * -1.f / Leak) : 999.f;
			Hold = FMath::Max(0.f, Hold);
		}
	}
	HoldLeftSec = FMath::RoundToFloat(Hold);
	TempC = FMath::RoundToFloat(TempC * 10.f) / 10.f;

	// seal progress
	for (FMCSample& S : Samples)
		if (S.Stage == EMCSampleStage::Sealing)
		{
			S.SealProgress = FMath::Min(1.f, S.SealProgress + Dt / FMath::Max(0.1f, C.SealSec));
			if (S.SealProgress >= 1.f) { S.Stage = EMCSampleStage::Sealed; UpdateBoard(); }
		}
}

void AMCSamplesManager::EventTick()
{
	if (!bEnableEvent || bEventFired || ElapsedSec < EventAt || ContainerIdx < 0) return;
	const FMCSamplesContainer* K = Cont();
	bEventFired = true;
	bEventActive = true;
	EventStart = ElapsedSec;
	EventKind = K && K->bActive ? EMCSamplesEvent::CoolerFault : EMCSamplesEvent::HoldWarning;
	Say(EventKind == EMCSamplesEvent::CoolerFault
		? TEXT("ALARM: COOLER FAULT. Systems: switch to BACKUP. Scientist: update storage.")
		: TEXT("WARNING: container hold time dropping. Systems: deploy insulation."));
	DebugLog(FString::Printf(TEXT("event kind=%d at %.1f"), (int32)EventKind, ElapsedSec));
}

// ---------------------------------------------------------------- relays, roles, panel
void AMCSamplesManager::SpawnRelays()
{
	UWorld* W = GetWorld();
	for (auto It = Relays.CreateIterator(); It; ++It) if (!It.Key() || !It.Value()) It.RemoveCurrent();
	for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || Relays.Contains(PC)) continue;
		FActorSpawnParameters P;
		P.Owner = PC;
		if (AMCSamplesRelay* R = W->SpawnActor<AMCSamplesRelay>(AMCSamplesRelay::StaticClass(), FTransform::Identity, P)) Relays.Add(PC, R);
	}
}

void AMCSamplesManager::RoleApplyTick()
{
	if (RoleApplyElapsed > 30.f) return;
	RoleApplyElapsed += 1.f;
	if (UMCMissionSubsystem* Sub = UMCMissionSubsystem::Get(this)) Sub->ApplyRoles(GetWorld());
}

void AMCSamplesManager::ClientPanelTick()
{
	if (!bCreatePanel || GetNetMode() == NM_DedicatedServer) return;
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController()) return;
	if (Panel) return;
	TSubclassOf<UUserWidget> Cls = PanelClass;
	if (!Cls) Cls = UMCSamplesPanel::StaticClass();
	Panel = CreateWidget<UUserWidget>(PC, Cls);
	if (UMCSamplesPanel* P = Cast<UMCSamplesPanel>(Panel)) P->Manager = this;
	if (Panel)
	{
		Panel->AddToViewport(20);
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
}

void AMCSamplesManager::Tick(float Dt)
{
	Super::Tick(Dt);
	ClientPanelTick();
	if (!HasAuthority()) return;

	RelayTimer -= Dt;
	if (RelayTimer <= 0.f) { RelayTimer = 1.f; SpawnRelays(); if (bStarted) RoleApplyTick(); }
	if (!bStarted) return;

	if (bDone)
	{
		if (!bTravelled && bTravelOnComplete && ElapsedSec - DoneTime >= Cfg().TravelDelaySec)
		{
			bTravelled = true;
			if (!ReturnMapPath.IsEmpty() && FPackageName::DoesPackageExist(ReturnMapPath))
				GetWorld()->ServerTravel(ReturnMapPath + TEXT("?listen"));
			else UE_LOG(LogMCSamples, Warning, TEXT("S5 done; map '%s' missing, staying."), *ReturnMapPath);
		}
		ClockSec += Dt; ElapsedSec = FMath::RoundToFloat(ClockSec * 5.f) / 5.f;
		return;
	}

	ClockSec += Dt;
	ElapsedSec = FMath::RoundToFloat(ClockSec * 5.f) / 5.f;
	SimTick(Dt);
	EventTick();

	// accuracy sampling every 5 s: Systems keeps temperature in range and power in budget
	TotalSec += Dt;
	if (Cont() && TempC <= LimitC) InRangeSec += Dt;
	if (!Cont()) InRangeSec += Dt;
	InRangeFrac = TotalSec > 0.f ? InRangeSec / TotalSec : 1.f;
	SampleClock += Dt;
	if (SampleClock >= 5.f && Cont())
	{
		SampleClock = 0.f;
		const bool bGood = TempC <= LimitC && PowerW <= PowerBudgetW;
		Say(bGood ? Message : (TempC > LimitC ? TEXT("TEMPERATURE ABOVE LIMIT!") : TEXT("POWER OVER BUDGET!")));
		Award(MCSamples::Systems, bGood);
	}

	// no-wait: routine check becomes available when cooling down finished; track idle gaps
	const UMCSamplesConfig& C = Cfg();
	for (int32 R = 1; R <= 4; ++R)
	{
		if (!RoutineReady[R])
		{
			RoutineCD[R] -= Dt;
			if (RoutineCD[R] <= 0.f) RoutineReady[R] = true;
		}
		if (HasWork(R) || RoutineReady[R]) IdleClock[R] = 0.f;
		else
		{
			IdleClock[R] += Dt;
			if (IdleClock[R] > IdleMax[R]) IdleMax[R] = IdleClock[R];
			if (IdleClock[R] > 10.f) UE_LOG(LogMCSamples, Warning, TEXT("S5 role %d idle %.1fs"), R, IdleClock[R]);
		}
	}
	(void)C;

	if (bDebugBot) DebugBot();

	if (CountSecured() >= Samples.Num()) Complete(false);
	else if (ElapsedSec >= C.TimeLimitSec) Complete(true);
}

// ---------------------------------------------------------------- end
void AMCSamplesManager::Complete(bool bTimedOut)
{
	if (bDone) return;
	bDone = true;
	DoneTime = ElapsedSec;
	int32 Ok = 0;
	const FMCSamplesContainer* K = Cont();
	for (const FMCSample& S : Samples)
	{
		const int32 Idx = S.Id - 1;
		const int32 TT = TrueType.IsValidIndex(Idx) ? TrueType[Idx] : 2;
		const bool bSecured = S.Stage == EMCSampleStage::Secured;
		const bool bContainerOk = K && !(S.ChosenCond == 0 && !K->bSupportsFrozen);
		if (bSecured && S.ChosenType == TT && S.ChosenCond == TT && bContainerOk) ++Ok;
	}
	SamplesOk = Ok;
	int32 G = 0, B = 0;
	for (int32 R = 1; R <= 4; ++R) { G += RoleGood[R]; B += RoleBad[R]; }
	const float Acc = (G + B) > 0 ? (float)G / (G + B) : 1.f;
	const float EventPart = (!bEventFired || bEventResponded) ? 1.f : 0.f;
	const float N = FMath::Max(1, Samples.Num());
	Score = FMath::RoundToInt(40.f * Ok / N + 30.f * InRangeFrac + 20.f * Acc + 10.f * EventPart);
	if (bTimedOut) Score = FMath::Min(Score, 60);

	ReportText = FString::Printf(TEXT("S5 %s: %d/%d samples stored correctly, %.0f%% of time in range, container %s, mass %.1f kg, power %.0f W. Score %d."),
		bTimedOut ? TEXT("TIME UP") : TEXT("COMPLETE"), Ok, Samples.Num(), InRangeFrac * 100.f, K ? *K->Name : TEXT("none"), MassKg, PowerW, Score);
	Say(ReportText);
	DebugLog(ReportText);
	for (int32 R = 1; R <= 4; ++R) DebugLog(FString::Printf(TEXT("role %d good=%d bad=%d maxIdle=%.1fs"), R, RoleGood[R], RoleBad[R], IdleMax[R]));

	if (UMCMissionSubsystem* Sub = UMCMissionSubsystem::Get(this))
	{
		Sub->bHasS5 = true;
		Sub->S5Score = Score;
		Sub->S5SamplesOk = Ok;
		Sub->S5ContainerName = K ? K->Name : FString();
		Sub->S5MassKg = MassKg;
		Sub->S5PowerW = PowerW;
		Sub->S5RoleGood = RoleGood;
		Sub->S5RoleBad = RoleBad;
		Sub->SetStageResult(5, Score, ElapsedSec, TEXT("Sealing, labels and cooling decide whether samples survive the trip home."));
	}
	OnComplete.Broadcast();
}

// ---------------------------------------------------------------- debug bot (uses server truth)
void AMCSamplesManager::DebugBot()
{
	BotTimer -= GetWorld()->GetDeltaSeconds();
	if (BotTimer > 0.f) return;
	BotTimer = 0.4f;
	if (ContainerIdx < 0) { HandleAction(MCSamples::MakeCode(3, 1), 3); return; } // ACTIVE COOLER
	if (bEventActive && !bEventFixed)
	{
		if (EventKind == EMCSamplesEvent::CoolerFault) HandleAction(104, 1); else HandleAction(105, 1);
		return;
	}
	if (bEventActive && EventKind == EMCSamplesEvent::CoolerFault && !bStorageUpdated) { HandleAction(207, 2); return; }
	if (FMCSample* S = FirstAt(EMCSampleStage::Raw))
	{
		const int32 TT = TrueType.IsValidIndex(S->Id - 1) ? TrueType[S->Id - 1] : 2;
		if (PendType < 0) { HandleAction(200 + TT, 2); return; }
		if (PendCond < 0) { HandleAction(203 + TT, 2); return; }
		HandleAction(206, 2);
		return;
	}
	if (FirstAt(EMCSampleStage::Labeled)) { HandleAction(303, 3); return; }
	if (FirstAt(EMCSampleStage::Placed)) { HandleAction(304, 3); return; }
	if (FirstAt(EMCSampleStage::Sealed)) { HandleAction(305, 3); return; }
	if (FirstAt(EMCSampleStage::Checked)) { HandleAction(400, 4); return; }
	if (FirstAt(EMCSampleStage::Loaded)) { HandleAction(401, 4); return; }
}
