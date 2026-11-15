#include "Return/MCReturnManager.h"
#include "Return/MCReturnView.h"
#include "Return/UI/MCReturnHud.h"
#include "Core/MCMissionSubsystem.h"
#include "MissionCrew.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/UserWidget.h"

namespace
{
	const TCHAR* AlarmTextFor(int32 Switch)
	{
		static const TCHAR* T[4] = { TEXT("COOLANT LOOP PRESSURE LOW"), TEXT("POWER BUS B UNDERVOLTAGE"), TEXT("THRUSTER VALVE OVERHEAT"), TEXT("ANTENNA HEATER FAULT") };
		return T[FMath::Clamp(Switch, 0, 3)];
	}
	const TCHAR* ItemName(int32 I)
	{
		static const TCHAR* T[3] = { TEXT("HEAT SHIELD LATCH"), TEXT("PARACHUTE PACK"), TEXT("LOCATOR RADIO") };
		return T[FMath::Clamp(I, 0, 2)];
	}
}

// ------------------------------------------------------------------ link
AMCReturnLink::AMCReturnLink()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	SetReplicateMovement(false);
	PrimaryActorTick.bCanEverTick = false;
}

void AMCReturnLink::ServerAction_Implementation(int32 Code)
{
	AMCReturnManager* M = AMCReturnManager::Find(this);
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (M && PC) M->HandleAction(Code, AMCReturnManager::GetCrewRoleOf(PC));
}

AMCReturnLink* AMCReturnLink::FindFor(UWorld* World, const APlayerController* PC)
{
	if (!World || !PC) return nullptr;
	for (TActorIterator<AMCReturnLink> It(World); It; ++It) if (It->GetOwner() == PC) return *It;
	return nullptr;
}

// ------------------------------------------------------------------ manager
AMCReturnManager::AMCReturnManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(10.f);
	RoleGood.Init(0, 5);
	RoleBad.Init(0, 5);
	PanelText.Init(FString(), 5);
	SwitchFault.Init(false, MCReturn::NumSwitches);
}

void AMCReturnManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCReturnManager, bFreeRoles);
	DOREPLIFETIME(AMCReturnManager, Stage);
	DOREPLIFETIME(AMCReturnManager, bStarted);
	DOREPLIFETIME(AMCReturnManager, bDone);
	DOREPLIFETIME(AMCReturnManager, bBlackout);
	DOREPLIFETIME(AMCReturnManager, Seed);
	DOREPLIFETIME(AMCReturnManager, ElapsedSec);
	DOREPLIFETIME(AMCReturnManager, Progress);
	DOREPLIFETIME(AMCReturnManager, Score);
	DOREPLIFETIME(AMCReturnManager, RoleGood);
	DOREPLIFETIME(AMCReturnManager, RoleBad);
	DOREPLIFETIME(AMCReturnManager, ReportState);
	DOREPLIFETIME(AMCReturnManager, bBeaconOn);
	DOREPLIFETIME(AMCReturnManager, bChuteOpen);
	DOREPLIFETIME(AMCReturnManager, bLanded);
	DOREPLIFETIME(AMCReturnManager, Health);
	DOREPLIFETIME(AMCReturnManager, ShipAngleErr);
	DOREPLIFETIME(AMCReturnManager, ShipCrossKm);
	DOREPLIFETIME(AMCReturnManager, LandMissKm);
	DOREPLIFETIME(AMCReturnManager, PulseSeq);
	DOREPLIFETIME(AMCReturnManager, SwitchFault);
	DOREPLIFETIME(AMCReturnManager, TopText);
	DOREPLIFETIME(AMCReturnManager, BoardText);
	DOREPLIFETIME(AMCReturnManager, PanelText);
	DOREPLIFETIME(AMCReturnManager, Message);
	DOREPLIFETIME(AMCReturnManager, LessonText);
	DOREPLIFETIME(AMCReturnManager, Summary);
}

const UMCReturnConfig& AMCReturnManager::Cfg() const
{
	if (Config) return *Config;
	AMCReturnManager* Self = const_cast<AMCReturnManager*>(this);
	if (!Self->DefaultConfig) Self->DefaultConfig = NewObject<UMCReturnConfig>(Self, NAME_None, RF_Transient);
	return *Self->DefaultConfig;
}

AMCReturnManager* AMCReturnManager::Find(const UObject* WorldContext)
{
	UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!W) return nullptr;
	for (TActorIterator<AMCReturnManager> It(W); It; ++It) return *It;
	return nullptr;
}

int32 AMCReturnManager::GetCrewRoleOf(const APlayerController* PC)
{
	return PC ? UMCMissionSubsystem::ReadCrewRole(PC->PlayerState) : 0;
}

void AMCReturnManager::DebugLog(const FString& Text) const
{
	if (bBot) { UE_LOG(LogMissionCrew, Log, TEXT("DEBUGTEST %s"), *Text); }
}

void AMCReturnManager::BeginPlay()
{
	Super::BeginPlay();
	StartTimer = StartDelaySec;
	if (HasAuthority() && bDebugAutoTest) bFreeRoles = true;
	if (bDebugAutoTest && HasAuthority()) RunDebugTest();
}

void AMCReturnManager::Say(const FString& Text)
{
	Message = Text;
	DebugLog(Text);
}

void AMCReturnManager::Mark(int32 CrewR, bool bGood, const FString& Note)
{
	if (CrewR >= 1 && CrewR <= 4)
	{
		if (bGood) ++RoleGood[CrewR]; else ++RoleBad[CrewR];
	}
	Score += bGood ? Cfg().PointsGood : -Cfg().PointsBad;
	if (UMCMissionSubsystem* Sub = UMCMissionSubsystem::Get(this)) Sub->AppendAccuracy(MCReturn::StageIndex, CrewR, bGood, Note);
	DebugLog(FString::Printf(TEXT("%s role %d: %s"), bGood ? TEXT("GOOD") : TEXT("BAD "), CrewR, *Note));
}

// ------------------------------------------------------------------ start
void AMCReturnManager::BeginReturn()
{
	if (!HasAuthority() || bStarted) return;
	UMCMissionSubsystem* Sub = UMCMissionSubsystem::Get(this);
	Seed = SeedOverride != 0 ? SeedOverride : (Sub && Sub->MissionSeed != 0 ? Sub->MissionSeed : FMath::RandRange(1000, 999999));
	if (Sub) Sub->SetMissionSeed(Seed);
	Stream.Initialize(Seed ^ 0x5106);
	const UMCReturnConfig& C = Cfg();

	BadSet = Stream.RandRange(0, 2);
	FaultItem = Stream.RandRange(0, 2);
	BiasAngle = Stream.FRand() < 0.5f ? -1.f : 1.f;
	BiasCross = Stream.FRand() < 0.5f ? -1.f : 1.f;
	PhaseA = Stream.FRand() * 6.28f;
	PhaseB = Stream.FRand() * 6.28f;
	AngleErr = (Stream.FRand() < 0.5f ? -1.f : 1.f) * FMath::Lerp(1.3f, 2.3f, Stream.FRand());
	CrossKm = (Stream.FRand() < 0.5f ? -1.f : 1.f) * FMath::Lerp(4.f, 9.f, Stream.FRand());
	StaleAngle = AngleErr; StaleCross = CrossKm;

	// warnings: distinct switches in seeded order, spread over cruise + reentry
	const int32 N = FMath::Clamp(C.WarningCount, 0, MCReturn::NumSwitches);
	TArray<int32> Perm = { 0, 1, 2, 3 };
	for (int32 i = 3; i > 0; --i) Perm.Swap(i, Stream.RandRange(0, i));
	const float Span = FMath::Max(10.f, C.CruiseSec + C.ReentrySec - C.WarningWindowSec - 10.f);
	AlarmAt.Reset(); AlarmSwitchIdx.Reset(); AlarmState.Reset(); AlarmOpenedAt.Reset();
	for (int32 i = 0; i < N; ++i)
	{
		AlarmAt.Add(10.f + (i + Stream.FRand() * 0.6f) * Span / N);
		AlarmSwitchIdx.Add(Perm[i]);
		AlarmState.Add(0);
		AlarmOpenedAt.Add(0.f);
	}
	bStarted = true;
	Stage = EMCReturnStage::Cruise;
	RunClock = StageClock = 0.f;
	Say(TEXT("Return trajectory set. Data: queue the report before the reentry blackout."));
	DebugLog(FString::Printf(TEXT("START seed %d badSet %d faultItem %d"), Seed, BadSet, FaultItem));
	RebuildStrings();
}

void AMCReturnManager::RunDebugTest()
{
	if (!HasAuthority()) return;
	bFreeRoles = true;
	bBot = true;
	TimeScale = FMath::Max(TimeScale, 5.f);
	BeginReturn();
	DebugLog(TEXT("bot running"));
}

// ------------------------------------------------------------------ tick
void AMCReturnManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCreateLocalUI && GetNetMode() != NM_DedicatedServer) SetupLocalUI();
	if (!HasAuthority()) return;

	ServerHousekeeping(DeltaSeconds);
	if (!bStarted)
	{
		if (bAutoStart && (StartTimer -= DeltaSeconds) <= 0.f) BeginReturn();
		return;
	}
	if (bDone) return;

	const float Dt = DeltaSeconds * FMath::Max(0.1f, TimeScale);
	const UMCReturnConfig& C = Cfg();
	RunClock += Dt;
	StageClock += Dt;
	ElapsedSec = RunClock;
	Progress = FMath::Clamp(RunClock / FMath::Max(1.f, C.CruiseSec + C.ReentrySec + C.LandingSec), 0.f, 1.f);

	SystemsTick();
	PilotTick(Dt);

	if (Stage == EMCReturnStage::Cruise && StageClock >= C.CruiseSec) EnterStage(EMCReturnStage::Reentry);
	else if (Stage == EMCReturnStage::Reentry)
	{
		if (bBlackout && StageClock >= C.BlackoutSec)
		{
			bBlackout = false;
			if (ReportState == 1) { ReportState = 2; Mark(2, true, TEXT("Queued report transmitted after the blackout")); }
			Say(ReportState == 2 ? TEXT("Radio contact restored. Report transmitted.") : TEXT("Radio contact restored."));
		}
		if (StageClock >= C.ReentrySec) EnterStage(EMCReturnStage::Landing);
	}
	else if (Stage == EMCReturnStage::Landing)
	{
		if (!bBeaconWindowClosed && StageClock >= C.BeaconWindowSec)
		{
			bBeaconWindowClosed = true;
			if (!bBeaconOn) Mark(4, false, TEXT("Beacon not activated in time"));
		}
		if (!bLanded && StageClock >= C.LandingSec) Touchdown();
		if (bLanded && (bHandedOver || StageClock >= C.LandingSec + C.HandoverGraceSec)) Finish();
	}

	if (bBot) { BotTimer += Dt; if (BotTimer >= 0.25f) { BotTimer = 0.f; BotTick(); } }
	StringTimer -= DeltaSeconds;
	if (StringTimer <= 0.f) { StringTimer = 0.2f; RebuildStrings(); }
}

void AMCReturnManager::ServerHousekeeping(float Dt)
{
	UWorld* W = GetWorld();
	LinkTimer -= Dt;
	if (LinkTimer <= 0.f)
	{
		LinkTimer = 0.5f;
		for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (PC && !AMCReturnLink::FindFor(W, PC))
			{
				FActorSpawnParameters P;
				P.Owner = PC;
				W->SpawnActor<AMCReturnLink>(AMCReturnLink::StaticClass(), FTransform::Identity, P);
			}
		}
	}
	if (!bRolesDone)
	{
		RoleElapsed += Dt;
		RoleTimer -= Dt;
		if (RoleTimer <= 0.f)
		{
			RoleTimer = 1.f;
			if (UMCMissionSubsystem* Sub = UMCMissionSubsystem::Get(this)) Sub->ApplyRoles(W);
		}
		if (RoleElapsed > RoleRetrySec) bRolesDone = true;
	}
}

void AMCReturnManager::SetupLocalUI()
{
	if (Hud && View) return;
	UWorld* W = GetWorld();
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->IsLocalController()) return;
	if (!View)
	{
		for (TActorIterator<AMCReturnView> It(W); It; ++It) { View = *It; break; }
		if (!View)
		{
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			View = W->SpawnActor<AMCReturnView>(AMCReturnView::StaticClass(), GetActorTransform(), P);
		}
		if (View) PC->SetViewTargetWithBlend(View, 0.5f);
	}
	if (!Hud)
	{
		Hud = CreateWidget<UMCReturnHudWidget>(PC, UMCReturnHudWidget::StaticClass());
		if (Hud)
		{
			Hud->AddToViewport(30);
			PC->SetShowMouseCursor(true);
			PC->SetInputMode(FInputModeGameAndUI());
		}
	}
}

// ------------------------------------------------------------------ stages
void AMCReturnManager::EnterStage(EMCReturnStage S)
{
	Stage = S;
	StageClock = 0.f;
	const UMCReturnConfig& C = Cfg();
	if (S == EMCReturnStage::Reentry)
	{
		bBlackout = true;
		StaleAngle = AngleErr; StaleCross = CrossKm;
		Mark(3, FMath::Abs(AngleErr) <= C.CorridorDeg, FString::Printf(TEXT("Entry angle error %.1f deg at interface"), AngleErr));
		if (ReportState == 0)
		{
			ReportState = 3;
			Mark(2, false, TEXT("Report not queued before the blackout: lost"));
			Say(TEXT("REENTRY: RADIO BLACKOUT. The report was not queued and is LOST."));
		}
		else Say(TEXT("REENTRY: RADIO BLACKOUT. A queued report goes out when contact returns."));
	}
	else if (S == EMCReturnStage::Landing)
	{
		const float Frac = ReentryTime > 0.f ? CorridorTime / ReentryTime : 0.f;
		Mark(3, Frac >= 0.7f, FString::Printf(TEXT("Reentry corridor held %.0f%% of the time"), Frac * 100.f));
		Say(TEXT("Landing phase. Deploy chutes (Pilot), beacon on (Recovery)."));
	}
}

void AMCReturnManager::Touchdown()
{
	const UMCReturnConfig& C = Cfg();
	float Miss = FMath::Sqrt(FMath::Square(AngleErr * C.KmPerDeg) + FMath::Square(CrossKm));
	if (!bChuteOpen)
	{
		Miss *= 1.8f;
		Mark(3, false, TEXT("Touchdown without parachutes"));
	}
	bInZone = Miss <= C.ZoneKm;
	LandMissKm = Miss;
	bLanded = true;
	bPilotLanded = true;
	LandedAt = StageClock;
	Mark(3, bInZone, FString::Printf(TEXT("Landed %.1f km from the target (zone %.0f km)"), Miss, C.ZoneKm));
	Say(bInZone ? TEXT("TOUCHDOWN inside the target zone. Hand over the samples.") : TEXT("TOUCHDOWN outside the target zone. Hand over the samples."));
}

void AMCReturnManager::Finish()
{
	const UMCReturnConfig& C = Cfg();
	if (!bHandedOver) Mark(4, false, TEXT("Samples not handed over"));
	if (!bEquipPassed) Mark(4, false, TEXT("Capsule equipment checks not completed"));
	for (int32 i = 0; i < AlarmState.Num(); ++i)
		if (AlarmState[i] == 1 || AlarmState[i] == 0) { AlarmState[i] = 3; ++WarningsMissed; Mark(1, false, FString::Printf(TEXT("Warning never resolved: %s"), AlarmTextFor(AlarmSwitchIdx[i]))); }
	SwitchFault.Init(false, MCReturn::NumSwitches);

	const float Frac = ReentryTime > 0.f ? CorridorTime / ReentryTime : 0.f;
	float LandFrac = 0.f;
	if (bChuteOpen || bInZone)
		LandFrac = bInZone ? 1.f - 0.5f * LandMissKm / FMath::Max(0.1f, C.ZoneKm) : 0.25f * FMath::Clamp(1.f - (LandMissKm - C.ZoneKm) / (2.f * C.ZoneKm), 0.f, 1.f);
	const float RepFrac = ReportState == 2 ? (bReportComplete ? 1.f : 0.5f) : 0.f;
	int32 Total = Score + FMath::RoundToInt(C.CorridorBonus * Frac + C.LandingBonus * LandFrac + C.ReportBonus * RepFrac);
	Score = FMath::Clamp(FMath::RoundToInt(Total * 100.f / 1700.f), 0, 100); // normalise to 0..100 like the other stages (1700 = a clean run)

	if (ReportState == 3) LessonText = TEXT("Queue the report before the blackout: a message not queued when the radio goes dark is lost.");
	else if (Frac < 0.7f) LessonText = TEXT("Hold the reentry corridor: trim the flight-path angle early; telemetry is gone in the blackout.");
	else if (!bInZone) LessonText = TEXT("Trim cross-range before the chutes open: the landing zone is small.");
	else if (!bBeaconOn || !bHandedOver) LessonText = TEXT("Recovery needs the beacon on inside its window and the samples ready at touchdown.");
	else if (WarningsMissed > 0) LessonText = TEXT("Resolve each warning with its own switch before it times out.");
	else LessonText = TEXT("Parallel roles worked: report sent, corridor held, beacon on, samples handed over.");

	bDone = true;
	Stage = EMCReturnStage::Complete;
	Progress = 1.f;
	UMCMissionSubsystem* Sub = UMCMissionSubsystem::Get(this);
	if (Sub) Sub->SetStageResult(MCReturn::StageIndex, Score, RunClock, LessonText);
	Summary = UMCMissionSummary::Build(Sub, Seed);
	Say(FString::Printf(TEXT("RETURN COMPLETE. Score %d."), Score));
	RebuildStrings();
	if (bBot) DebugLog(FString::Printf(TEXT("DONE score %d\n%s"), Score, *UMCMissionSummary::ToText(Summary)));
	OnComplete.Broadcast(Score);
}

// ------------------------------------------------------------------ systems / pilot
void AMCReturnManager::SystemsTick()
{
	const UMCReturnConfig& C = Cfg();
	for (int32 i = 0; i < AlarmState.Num(); ++i)
	{
		if (AlarmState[i] == 0 && RunClock >= AlarmAt[i])
		{
			AlarmState[i] = 1;
			AlarmOpenedAt[i] = RunClock;
			SwitchFault[AlarmSwitchIdx[i]] = true;
			Say(FString::Printf(TEXT("WARNING: %s"), AlarmTextFor(AlarmSwitchIdx[i])));
		}
		else if (AlarmState[i] == 1 && RunClock - AlarmOpenedAt[i] > C.WarningWindowSec)
		{
			AlarmState[i] = 3;
			++WarningsMissed;
			SwitchFault[AlarmSwitchIdx[i]] = false;
			Health = FMath::Max(0.f, Health - 8.f);
			Mark(1, false, FString::Printf(TEXT("Warning timed out: %s"), AlarmTextFor(AlarmSwitchIdx[i])));
		}
	}
}

void AMCReturnManager::PilotTick(float Dt)
{
	if (bLanded) return;
	const UMCReturnConfig& C = Cfg();
	float Mult = 0.5f;
	if (Stage == EMCReturnStage::Reentry) Mult = bBlackout ? 0.9f : 1.3f;
	else if (Stage == EMCReturnStage::Landing) Mult = bChuteOpen ? 0.1f : 0.5f;
	AngleErr += (BiasAngle * C.DriftDegPerSec + 0.5f * C.DriftDegPerSec * FMath::Sin(RunClock * 0.8f + PhaseA)) * Mult * Dt;
	CrossKm += (BiasCross * C.DriftKmPerSec + 0.5f * C.DriftKmPerSec * FMath::Sin(RunClock * 0.6f + PhaseB)) * Mult * Dt;
	if (Stage == EMCReturnStage::Reentry)
	{
		ReentryTime += Dt;
		if (FMath::Abs(AngleErr) <= C.CorridorDeg) CorridorTime += Dt;
		else Health = FMath::Max(0.f, Health - 1.5f * Dt);
	}
	if (!bBlackout) { StaleAngle = AngleErr; StaleCross = CrossKm; }
	ShipAngleErr = AngleErr;
	ShipCrossKm = CrossKm;
}

// ------------------------------------------------------------------ actions
bool AMCReturnManager::HandleAction(int32 Code, int32 Who)
{
	if (!HasAuthority() || !bStarted || bDone) return false;
	const int32 CrewR = Code / 100, Action = Code % 100;
	if (CrewR < 1 || CrewR > 4) return false;
	if (!bFreeRoles && Who != CrewR) return false;
	switch (CrewR)
	{
	case 1: return DoSystems(Action);
	case 2: return DoData(Action);
	case 3: return DoPilot(Action);
	default: return DoRecovery(Action);
	}
}

bool AMCReturnManager::DoSystems(int32 Action)
{
	if (Action < 0 || Action >= MCReturn::NumSwitches) return false;
	for (int32 i = 0; i < AlarmState.Num(); ++i)
	{
		if (AlarmState[i] == 1 && AlarmSwitchIdx[i] == Action)
		{
			AlarmState[i] = 2;
			++WarningsFixed;
			SwitchFault[Action] = false;
			Mark(1, true, FString::Printf(TEXT("Warning resolved: %s"), AlarmTextFor(Action)));
			return true;
		}
	}
	Mark(1, false, FString::Printf(TEXT("Switched %s with no warning open"), AlarmTextFor(Action)));
	return true;
}

bool AMCReturnManager::DoData(int32 Action)
{
	if (Action == MCReturn::DataCheck)
	{
		if (!bDataChecked)
		{
			bDataChecked = true;
			Mark(2, true, TEXT("Science data checked"));
			Say(FString::Printf(TEXT("Data check: set %d is corrupt. Re-fetch it."), BadSet + 1));
		}
		return true;
	}
	if (Action >= 1 && Action <= 3)
	{
		const int32 I = Action - 1;
		if (ReportState != 0 || bSetFixed[I]) return false;
		if (I == BadSet) { bSetFixed[I] = true; Mark(2, true, TEXT("Corrupt data set re-fetched")); }
		else Mark(2, false, TEXT("Re-fetched a healthy data set"));
		return true;
	}
	if (Action == MCReturn::DataQueue)
	{
		if (ReportState != 0) return false;
		bReportComplete = bSetFixed[BadSet];
		ReportState = 1;
		Mark(2, bReportComplete, bReportComplete ? TEXT("Complete report queued before the blackout") : TEXT("Report queued with a corrupt data set"));
		Say(TEXT("Report queued."));
		return true;
	}
	if (Action == MCReturn::DataSend)
	{
		if (ReportState == 1 && !bBlackout)
		{
			ReportState = 2;
			Mark(2, true, TEXT("Report sent over the open link"));
			Say(TEXT("Report sent."));
			return true;
		}
		Say(ReportState == 0 ? TEXT("Nothing queued to send.") : TEXT("No link: waiting for contact."));
		return false;
	}
	return false;
}

bool AMCReturnManager::DoPilot(int32 Action)
{
	if (bLanded) return false;
	const UMCReturnConfig& C = Cfg();
	switch (Action)
	{
	case MCReturn::PilotSteeper:   AngleErr += C.PulseDeg; break;
	case MCReturn::PilotShallower: AngleErr -= C.PulseDeg; break;
	case MCReturn::PilotLeft:      CrossKm -= C.PulseKm; break;
	case MCReturn::PilotRight:     CrossKm += C.PulseKm; break;
	case MCReturn::PilotChutes:
		if (bChuteOpen) return false;
		if (Stage != EMCReturnStage::Landing) { Mark(3, false, TEXT("Chutes commanded too early")); return true; }
		bChuteOpen = true;
		Mark(3, true, TEXT("Parachutes deployed"));
		Say(TEXT("Parachutes open."));
		return true;
	default: return false;
	}
	++PulseSeq;
	if (!bBlackout) { StaleAngle = AngleErr; StaleCross = CrossKm; }
	ShipAngleErr = AngleErr;
	ShipCrossKm = CrossKm;
	return true;
}

bool AMCReturnManager::DoRecovery(int32 Action)
{
	auto CheckPass = [this]()
	{
		if (!bEquipPassed && bItemChecked[0] && bItemChecked[1] && bItemChecked[2] && bFaultSecured)
		{
			bEquipPassed = true;
			Mark(4, true, TEXT("Capsule equipment checks passed"));
		}
	};
	if (Action == MCReturn::RecBeacon)
	{
		if (bBeaconOn) return false;
		if (Stage == EMCReturnStage::Cruise || Stage == EMCReturnStage::Reentry) { Mark(4, false, TEXT("Beacon switched on too early")); return true; }
		bBeaconOn = true;
		if (!bBeaconWindowClosed) Mark(4, true, TEXT("Beacon on in time"));
		Say(TEXT("Beacon active."));
		return true;
	}
	if (Action >= 1 && Action <= 3)
	{
		const int32 I = Action - 1;
		if (bItemChecked[I]) return false;
		bItemChecked[I] = true;
		Say(I == FaultItem ? FString::Printf(TEXT("%s: FAULT found. Secure it."), ItemName(I)) : FString::Printf(TEXT("%s: OK."), ItemName(I)));
		CheckPass();
		return true;
	}
	if (Action == MCReturn::RecSecure)
	{
		if (bItemChecked[FaultItem] && !bFaultSecured) { bFaultSecured = true; Mark(4, true, TEXT("Capsule fault secured")); CheckPass(); }
		else Mark(4, false, TEXT("Secured with no known fault"));
		return true;
	}
	if (Action == MCReturn::RecPrepare)
	{
		if (bPrepared) return false;
		if (!bEquipPassed) { Mark(4, false, TEXT("Samples prepared before the capsule checks passed")); return true; }
		bPrepared = true;
		Mark(4, true, TEXT("Samples sealed and ready for handover"));
		return true;
	}
	if (Action == MCReturn::RecHandover)
	{
		if (bHandedOver) return false;
		if (!bLanded || !bPrepared) { Mark(4, false, bLanded ? TEXT("Handover with unprepared samples") : TEXT("Handover before touchdown")); return true; }
		bHandedOver = true;
		Mark(4, true, TEXT("Samples handed over"));
		Say(TEXT("Samples handed over."));
		return true;
	}
	return false;
}

// ------------------------------------------------------------------ strings
void AMCReturnManager::RebuildStrings()
{
	const UMCReturnConfig& C = Cfg();
	const TCHAR* StageNames[] = { TEXT("RETURN CRUISE"), TEXT("REENTRY"), TEXT("LANDING"), TEXT("COMPLETE") };
	FString Top = bStarted ? StageNames[(int32)Stage] : TEXT("WAITING FOR RETURN BURN");
	if (bStarted && !bDone) Top += FString::Printf(TEXT("  |  T+ %.0f s"), ElapsedSec);
	if (bBlackout) Top += TEXT("  |  RADIO BLACKOUT");
	TopText = FString::Printf(TEXT("%s  |  SEED %d  |  HEALTH %.0f  |  SCORE %d"), *Top, Seed, Health, Score);

	const TCHAR* Rep[] = { TEXT("DRAFT"), TEXT("QUEUED"), TEXT("SENT"), TEXT("LOST") };
	BoardText = FString::Printf(TEXT("REPORT  %s\nBEACON  %s\nCHUTES  %s\nLANDING  %s\nWARNINGS  %d fixed / %d missed"),
		Rep[FMath::Clamp(ReportState, 0, 3)], bBeaconOn ? TEXT("ON") : TEXT("OFF"), bChuteOpen ? TEXT("OPEN") : TEXT("STOWED"),
		bLanded ? *FString::Printf(TEXT("%.1f km from target (%s)"), LandMissKm, bInZone ? TEXT("IN ZONE") : TEXT("OUT")) : TEXT("-"),
		WarningsFixed, WarningsMissed);

	// 1 Systems
	FString S1 = TEXT("Press the switch of the warning that is open (red).\n");
	for (int32 i = 0; i < AlarmState.Num(); ++i)
	{
		if (AlarmState[i] == 1) S1 += FString::Printf(TEXT("! %s  (%.0f s)\n"), AlarmTextFor(AlarmSwitchIdx[i]), FMath::Max(0.f, C.WarningWindowSec - (RunClock - AlarmOpenedAt[i])));
		else if (AlarmState[i] == 2) S1 += FString::Printf(TEXT("ok %s\n"), AlarmTextFor(AlarmSwitchIdx[i]));
		else if (AlarmState[i] == 3) S1 += FString::Printf(TEXT("MISSED %s\n"), AlarmTextFor(AlarmSwitchIdx[i]));
	}
	PanelText[1] = S1;

	// 2 Data
	FString S2;
	static const TCHAR* SetNames[3] = { TEXT("TELEMETRY"), TEXT("SPECTRA"), TEXT("IMAGERY") };
	for (int32 i = 0; i < 3; ++i)
	{
		const TCHAR* St = TEXT("?");
		if (bSetFixed[i]) St = TEXT("FIXED");
		else if (bDataChecked) St = (i == BadSet) ? TEXT("CORRUPT") : TEXT("OK");
		S2 += FString::Printf(TEXT("SET %d  %s  [%s]\n"), i + 1, SetNames[i], St);
	}
	S2 += FString::Printf(TEXT("\nREPORT  %s\n"), Rep[FMath::Clamp(ReportState, 0, 3)]);
	if (Stage == EMCReturnStage::Cruise) S2 += FString::Printf(TEXT("BLACKOUT IN %.0f s: queue the report before it!"), FMath::Max(0.f, C.CruiseSec - StageClock));
	else S2 += bBlackout ? TEXT("LINK DOWN") : TEXT("LINK UP");
	PanelText[2] = S2;

	// 3 Pilot (the readout freezes in the blackout)
	FString S3;
	S3 += FString::Printf(TEXT("PATH ANGLE ERR  %+.2f deg   (corridor +-%.1f)\nCROSS-RANGE      %+.1f km\n"), StaleAngle, C.CorridorDeg, StaleCross);
	S3 += FString::Printf(TEXT("PREDICTED MISS  %.1f km   (zone %.0f km)\n"), FMath::Sqrt(FMath::Square(StaleAngle * C.KmPerDeg) + FMath::Square(StaleCross)), C.ZoneKm);
	if (bBlackout) S3 += TEXT("NO TELEMETRY: readout frozen\n");
	S3 += FString::Printf(TEXT("CHUTES  %s"), bChuteOpen ? TEXT("OPEN") : TEXT("STOWED"));
	PanelText[3] = S3;

	// 4 Recovery
	FString S4;
	for (int32 i = 0; i < 3; ++i)
	{
		const TCHAR* St = TEXT("?");
		if (bItemChecked[i]) St = (i == FaultItem) ? (bFaultSecured ? TEXT("SECURED") : TEXT("FAULT")) : TEXT("OK");
		S4 += FString::Printf(TEXT("%d %s  [%s]\n"), i + 1, ItemName(i), St);
	}
	S4 += FString::Printf(TEXT("\nBEACON  %s\nSAMPLES  %s"), bBeaconOn ? TEXT("ON") : (Stage == EMCReturnStage::Landing ? TEXT("OFF (window open!)") : TEXT("OFF (wait for landing)")),
		bHandedOver ? TEXT("HANDED OVER") : (bPrepared ? TEXT("READY") : TEXT("not prepared")));
	PanelText[4] = S4;
}

// ------------------------------------------------------------------ bot
void AMCReturnManager::BotTick()
{
	auto Act = [this](int32 R, int32 Action) { HandleAction(MCReturn::MakeCode(R, Action), R); };
	const UMCReturnConfig& C = Cfg();
	// systems
	for (int32 i = 0; i < AlarmState.Num(); ++i) if (AlarmState[i] == 1) Act(1, AlarmSwitchIdx[i]);
	// data
	if (RunClock > 3.f && !bDataChecked) Act(2, MCReturn::DataCheck);
	else if (bDataChecked && ReportState == 0 && !bSetFixed[BadSet]) Act(2, MCReturn::DataRefetch0 + BadSet);
	else if (RunClock > 12.f && ReportState == 0 && Stage == EMCReturnStage::Cruise) Act(2, MCReturn::DataQueue);
	else if (ReportState == 1 && !bBlackout) Act(2, MCReturn::DataSend);
	// recovery
	if (RunClock > 6.f)
	{
		for (int32 i = 0; i < 3; ++i) if (!bItemChecked[i]) { Act(4, MCReturn::RecCheck0 + i); break; }
		if (bItemChecked[FaultItem] && !bFaultSecured) Act(4, MCReturn::RecSecure);
		if (bEquipPassed && !bPrepared) Act(4, MCReturn::RecPrepare);
	}
	if (Stage == EMCReturnStage::Landing && !bBeaconOn && StageClock > 2.f) Act(4, MCReturn::RecBeacon);
	if (bLanded && bPrepared && !bHandedOver) Act(4, MCReturn::RecHandover);
	// pilot
	if (AngleErr > 0.25f) Act(3, MCReturn::PilotShallower);
	else if (AngleErr < -0.25f) Act(3, MCReturn::PilotSteeper);
	else if (CrossKm > 1.f) Act(3, MCReturn::PilotLeft);
	else if (CrossKm < -1.f) Act(3, MCReturn::PilotRight);
	if (Stage == EMCReturnStage::Landing && !bChuteOpen && StageClock > 3.f && StageClock < C.LandingSec - 3.f) Act(3, MCReturn::PilotChutes);
}
