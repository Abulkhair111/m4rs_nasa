#include "Transit/MCTransitManager.h"
#include "Construction/MCConstructionManager.h"
#include "Core/MCMissionSubsystem.h"
#include "Bay/MCBayCharacter.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "MissionCrew.h"

namespace
{
	const TCHAR* StageName(EMCTransitStage S)
	{
		switch (S)
		{
		case EMCTransitStage::GoPoll:    return TEXT("GO/NO-GO POLL");
		case EMCTransitStage::Countdown: return TEXT("COUNTDOWN");
		case EMCTransitStage::Ascent:    return TEXT("ASCENT");
		case EMCTransitStage::Coast:     return TEXT("COAST TO MOON");
		default:                         return TEXT("ARRIVAL");
		}
	}

	const TCHAR* SwitchNames[MCTransit::NumSwitches] = {
		TEXT("HEATERS"), TEXT("NAV SENSOR"), TEXT("TRANSPONDER"), TEXT("WHEEL DESAT"), TEXT("INSTRUMENT RAIL"), TEXT("MECHANISM RAIL")
	};

	const TCHAR* RoleNames[MCTransit::NumRoles] = { TEXT("Solo"), TEXT("Systems"), TEXT("Equipment"), TEXT("Pilot"), TEXT("Science") };

	// Device procedures (3 steps each): 0 legs, 1 rover, 2 drill, 3 lidar.
	const TCHAR* DeviceName[MCTransit::MaxDevices] = { TEXT("LANDING LEGS"), TEXT("ROVER"), TEXT("DRILL"), TEXT("LIDAR") };
	const TCHAR* DeviceSteps[MCTransit::MaxDevices][3] = {
		{ TEXT("UNLATCH CLAMPS"), TEXT("DEPLOY STRUTS"), TEXT("LOCK FOOTPADS") },
		{ TEXT("RELEASE TIE-DOWNS"), TEXT("POWER DRIVE"), TEXT("TEST DRIVE") },
		{ TEXT("UNSTOW MAST"), TEXT("SPIN UP"), TEXT("TEST BITE") },
		{ TEXT("COVER OFF"), TEXT("WARM UP"), TEXT("TEST SCAN") }
	};

	// Ordinary alarms: text, switch, wanted state.
	struct FAlarmDef { const TCHAR* Text; int32 Switch; bool bWantOn; };
	const FAlarmDef AlarmDefs[MCTransit::NumAlarms] = {
		{ TEXT("CABIN TEMP LOW"),             (int32)EMCSystemsAction::Heaters,     true },
		{ TEXT("XPNDR A: NO CARRIER"),        (int32)EMCSystemsAction::Transponder, true },
		{ TEXT("REACTION WHEEL RPM HIGH"),    (int32)EMCSystemsAction::WheelDesat,  true },
		{ TEXT("NAV SENSOR DISAGREE"),        (int32)EMCSystemsAction::NavSensor,   true }   // sensor fault alarm
	};

	double ServerNow(const UWorld* World)
	{
		const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
		return GS ? GS->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
	}
}

AMCTransitManager::AMCTransitManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(10.f);

	GoFlags.Init(false, MCTransit::NumRoles);
	RoleGood.Init(0, MCTransit::NumRoles);
	RoleBad.Init(0, MCTransit::NumRoles);
	PanelText.Init(FString(), MCTransit::NumRoles);
}

void AMCTransitManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCTransitManager, bFreeRoles);
	DOREPLIFETIME(AMCTransitManager, Stage);
	DOREPLIFETIME(AMCTransitManager, bStarted);
	DOREPLIFETIME(AMCTransitManager, bDone);
	DOREPLIFETIME(AMCTransitManager, Seed);
	DOREPLIFETIME(AMCTransitManager, StageEndTime);
	DOREPLIFETIME(AMCTransitManager, ElapsedSec);
	DOREPLIFETIME(AMCTransitManager, TransitDurationSec);
	DOREPLIFETIME(AMCTransitManager, GoFlags);
	DOREPLIFETIME(AMCTransitManager, Health);
	DOREPLIFETIME(AMCTransitManager, FuelKg);
	DOREPLIFETIME(AMCTransitManager, FuelStartKg);
	DOREPLIFETIME(AMCTransitManager, LaunchReliability);
	DOREPLIFETIME(AMCTransitManager, DispX);
	DOREPLIFETIME(AMCTransitManager, DispY);
	DOREPLIFETIME(AMCTransitManager, CorridorKm);
	DOREPLIFETIME(AMCTransitManager, Windows);
	DOREPLIFETIME(AMCTransitManager, Switches);
	DOREPLIFETIME(AMCTransitManager, Alarms);
	DOREPLIFETIME(AMCTransitManager, Devices);
	DOREPLIFETIME(AMCTransitManager, ActiveDevice);
	DOREPLIFETIME(AMCTransitManager, Instruments);
	DOREPLIFETIME(AMCTransitManager, ActiveInstrument);
	DOREPLIFETIME(AMCTransitManager, CalTarget);
	DOREPLIFETIME(AMCTransitManager, CalTolerance);
	DOREPLIFETIME(AMCTransitManager, TopText);
	DOREPLIFETIME(AMCTransitManager, BoardText);
	DOREPLIFETIME(AMCTransitManager, PanelText);
	DOREPLIFETIME(AMCTransitManager, Message);
	DOREPLIFETIME(AMCTransitManager, TransitScore);
	DOREPLIFETIME(AMCTransitManager, Result);
	DOREPLIFETIME(AMCTransitManager, RoleGood);
	DOREPLIFETIME(AMCTransitManager, RoleBad);
	DOREPLIFETIME(AMCTransitManager, ReportText);
	DOREPLIFETIME(AMCTransitManager, bDisplayStale);
	DOREPLIFETIME(AMCTransitManager, ShipErrX);
	DOREPLIFETIME(AMCTransitManager, ShipErrY);
	DOREPLIFETIME(AMCTransitManager, PulseSeq);
	DOREPLIFETIME(AMCTransitManager, LastPulseDir);
}

const UMCTransitConfig& AMCTransitManager::Cfg() const
{
	return Config ? *Config : *GetDefault<UMCTransitConfig>();
}

int32 AMCTransitManager::GetCrewRoleOfController(const AController* Controller)
{
	const APlayerState* PS = Controller ? Controller->PlayerState.Get() : nullptr;
	if (!PS) return 0;
	if (const FProperty* P = PS->GetClass()->FindPropertyByName(TEXT("CrewRole")))
	{
		const void* Addr = P->ContainerPtrToValuePtr<void>(PS);
		if (const FIntProperty* IP = CastField<FIntProperty>(P)) return IP->GetPropertyValue(Addr);
		if (const FByteProperty* BP = CastField<FByteProperty>(P)) return BP->GetPropertyValue(Addr);
		if (const FEnumProperty* EP = CastField<FEnumProperty>(P)) return (int32)EP->GetUnderlyingProperty()->GetSignedIntPropertyValue(Addr);
	}
	return 0;
}

// ------------------------------------------------------------------ start

void AMCTransitManager::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) { return; }

	if (bDebugAutoTest) { bFreeRoles = true; }

	const FString MapName = GetWorld() ? GetWorld()->GetMapName() : FString();
	if (bStartOnBeginPlay || MapName.Contains(TEXT("L_Transit")))
	{
		// The S1 result comes from UMCMissionSubsystem when we travelled here from the bay; else the test loadout.
		const UMCMissionSubsystem* Mission = UMCMissionSubsystem::Get(this);
		if (!(Mission && Mission->bHasLoadout)) { bTestMode = true; }
		BeginTransit();
		return;
	}
	if (bAutoStart) { FindConstructionManager(); }
}

void AMCTransitManager::FindConstructionManager()
{
	if (!ConsMgr)
	{
		for (TActorIterator<AMCConstructionManager> It(GetWorld()); It; ++It) { ConsMgr = *It; break; }
	}
	if (ConsMgr && !bBound)
	{
		ConsMgr->OnLaunchComplete.AddDynamic(this, &AMCTransitManager::OnConstructionLaunched);
		bBound = true;
	}
}

void AMCTransitManager::OnConstructionLaunched(const FMCLoadout& InLoadout)
{
	if (!bStarted) { BeginTransit(); }
}

void AMCTransitManager::BeginTransit()
{
	if (!HasAuthority() || bStarted) { return; }
	if (bDebugAutoTest) { bFreeRoles = true; }
	if (!ConsMgr) { FindConstructionManager(); }

	const UMCMissionSubsystem* Mission = UMCMissionSubsystem::Get(this);
	const bool bHaveS1 = (Mission && Mission->bHasLoadout) || (ConsMgr && ConsMgr->bLaunched);
	LoadLoadout(bTestMode || !bHaveS1);
	InitData();
	bStarted = true;
	SetStage(EMCTransitStage::GoPoll, 0.f);
	Say(TEXT("All stations: press GO FOR LAUNCH."));
	UE_LOG(LogMissionCrew, Log, TEXT("Transit: begin, seed %d, %d devices, %d instruments, fuel %.0f kg"),
		Seed, Devices.Num(), Instruments.Num(), FuelKg);
	if (bDebugAutoTest && !bDebugBot) { RunDebugTest(); }

	if (bAutoSeat && GetWorld())
	{
		SeatElapsed = 0.f;
		AutoSeated.Reset();
		GetWorldTimerManager().SetTimer(SeatTimer, this, &AMCTransitManager::AutoSeatTick, 1.f, true, FMath::Max(0.1f, SeatDelaySec));
	}
}

void AMCTransitManager::AutoSeatTick()
{
	UWorld* W = GetWorld();
	if (!W) { return; }
	SeatElapsed += 1.f;

	// Roles first: non-seamless travel makes new PlayerStates, CrewRole is back at its default.
	if (UMCMissionSubsystem* Mission = UMCMissionSubsystem::Get(this)) { Mission->ApplyRoles(W); }

	int32 Players = 0, Seated = 0;
	for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (!PC) { continue; }
		++Players;
		AMCBayCharacter* Pawn = Cast<AMCBayCharacter>(PC->GetPawn());
		if (!Pawn) { continue; }
		const TWeakObjectPtr<APawn> Key(Pawn);
		if (AutoSeated.Contains(Key) || Pawn->IsSeated()) { ++Seated; continue; } // seat each pawn once; a player who stood up stays up
		if (Pawn->GetCrewRole() <= 0 && !bFreeRoles) { continue; }                // role not known yet (PlayerState still replicating)
		if (Pawn->SeatAtOwnStation()) { AutoSeated.Add(Key); ++Seated; }
	}
	if ((Players > 0 && Seated >= Players) || SeatElapsed >= SeatRetrySec)
	{
		GetWorldTimerManager().ClearTimer(SeatTimer);
		UE_LOG(LogMissionCrew, Log, TEXT("Transit: auto-seat finished, %d of %d players seated"), Seated, Players);
	}
}

void AMCTransitManager::LoadLoadout(bool bUseDefault)
{
	const UMCTransitConfig& C = Cfg();
	Loadout = FMCLoadout();
	bHaveLoadout = false;
	Devices.Reset();
	Instruments.Reset();

	bool bRover = false, bDrill = false, bLidar = false;
	TArray<FString> InstNames;

	// Source priority: 1) UMCMissionSubsystem (survives the map change), 2) a launched AMCConstructionManager in this world, 3) test loadout.
	const UMCMissionSubsystem* Mission = UMCMissionSubsystem::Get(this);
	const bool bFromSub = Mission && Mission->bHasLoadout;
	if (!bUseDefault && (bFromSub || ConsMgr))
	{
		if (bFromSub)
		{
			Loadout = Mission->Loadout;
			LaunchReliability = Mission->LaunchReliability;
			FuelReserveKg = C.LandingReserveFrac * Mission->FuelRequiredKg;
		}
		else
		{
			Loadout = ConsMgr->BuildLoadout();
			LaunchReliability = FMath::Clamp(ConsMgr->RocketReliability > 0.f ? ConsMgr->RocketReliability : 1.f, 0.f, 1.f);
			FuelReserveKg = C.LandingReserveFrac * ConsMgr->FuelRequiredKg;
		}
		bHaveLoadout = true;
		FuelStartKg = Loadout.FuelKg;
		for (const FMCLoadoutEntry& E : Loadout.Parts)
		{
			switch (E.SocketType)
			{
			case EMCSocketType::Rover:   bRover = true; break;
			case EMCSocketType::Drill:   bDrill = true; break;
			case EMCSocketType::Lidar:   bLidar = true; break;
			case EMCSocketType::Science:
				if (InstNames.Num() < MCTransit::MaxInstruments)
				{
					FString N = E.Def ? E.Def->DisplayName.ToString().ToUpper() : TEXT("INSTRUMENT");
					InstNames.Add(N);
				}
				break;
			default: break;
			}
		}
	}
	else
	{
		FuelStartKg = C.DefaultFuelKg;
		LaunchReliability = C.DefaultLaunchReliability;
		FuelReserveKg = C.DefaultReserveKg;
		bRover = bDrill = true;
		InstNames = { TEXT("NSS"), TEXT("NIRVSS"), TEXT("MSOLO") };
	}
	FuelKg = FuelStartKg;

	// Devices: legs always, then rover, drill, lidar if installed. Perms are drawn later in InitData.
	TArray<int32> Kinds = { 0 };
	if (bRover) { Kinds.Add(1); }
	if (bDrill) { Kinds.Add(2); }
	if (bLidar) { Kinds.Add(3); }
	for (int32 K : Kinds)
	{
		FMCTransitDevice D;
		D.Name = DeviceName[K];
		for (int32 s = 0; s < 3; ++s) { D.Steps.Add(DeviceSteps[K][s]); }
		D.ButtonLabels = D.Steps; // scrambled in InitData
		Devices.Add(D);
	}
	for (const FString& N : InstNames)
	{
		FMCTransitInstrument I;
		I.Name = N;
		Instruments.Add(I);
	}
	// Remember which kind each device is (for the perm table the order of Devices is enough).
}

void AMCTransitManager::InitData()
{
	const UMCTransitConfig& C = Cfg();
	Seed = SeedOverride != 0 ? SeedOverride : FMath::RandRange(1, 99999);
	Stream.Initialize(Seed);

	CorridorKm = C.CorridorKm;
	TransitDurationSec = C.TransitSec;
	CalTarget = C.CalTarget;
	CalTolerance = C.CalTolerance;
	Health = 100.f;
	bAnomaly = false;
	bFaultHappened = bFaultFixed = false;
	bDisplayStale = false;
	RunClock = StageClock = CoastClock = 0.f;

	// All random draws, in a fixed order, independent of the loadout contents.
	// 1) initial predicted miss
	ErrX = Stream.FRandRange(-C.InitialErrMaxKm, C.InitialErrMaxKm);
	ErrY = Stream.FRandRange(-C.InitialErrMaxKm, C.InitialErrMaxKm);
	InitErrX = ErrX; InitErrY = ErrY;
	// 2) ascent anomaly
	AnomalyRoll = Stream.FRand();
	const float AnomAng = Stream.FRandRange(0.f, 2.f * PI);
	AnomalyDirX = FMath::Cos(AnomAng); AnomalyDirY = FMath::Sin(AnomAng);
	// 3) drift per segment (5 segments between/after the 4 windows)
	DriftX.SetNum(5); DriftY.SetNum(5);
	for (int32 i = 0; i < 5; ++i)
	{
		DriftX[i] = Stream.FRandRange(-C.DriftMaxKm, C.DriftMaxKm);
		DriftY[i] = Stream.FRandRange(-C.DriftMaxKm, C.DriftMaxKm);
	}
	// 4) sensor fault
	FaultTime = Stream.FRandRange(C.FaultTimeMinSec, C.FaultTimeMaxSec);
	const float KickAng = Stream.FRandRange(0.f, 2.f * PI);
	const float KickMag = Stream.FRandRange(C.FaultKickMinKm, C.FaultKickMaxKm);
	KickX = FMath::Cos(KickAng) * KickMag; KickY = FMath::Sin(KickAng) * KickMag;
	// 5) alarms: 3 ordinary + the fault alarm at FaultTime
	AlarmTime.SetNum(MCTransit::NumAlarms);
	AlarmSwitch.SetNum(MCTransit::NumAlarms);
	AlarmWantOn.SetNum(MCTransit::NumAlarms);
	Alarms.SetNum(MCTransit::NumAlarms);
	for (int32 i = 0; i < MCTransit::NumAlarms; ++i)
	{
		const float T = Stream.FRandRange(C.AlarmMinSec, C.AlarmMaxSec);
		AlarmTime[i] = (i == MCTransit::NumAlarms - 1) ? FaultTime : T;
		AlarmSwitch[i] = AlarmDefs[i].Switch;
		AlarmWantOn[i] = AlarmDefs[i].bWantOn;
		Alarms[i] = FMCTransitAlarm();
		Alarms[i].Text = AlarmDefs[i].Text;
	}
	// 6) equipment button scrambles (4 devices drawn regardless of how many exist)
	TArray<TArray<int32>> AllPerm;
	for (int32 d = 0; d < MCTransit::MaxDevices; ++d)
	{
		TArray<int32> P = { 0, 1, 2 };
		for (int32 i = 2; i > 0; --i) { P.Swap(i, Stream.RandRange(0, i)); }
		AllPerm.Add(P);
	}
	DevicePerm.Reset();
	DeviceTestClock.Reset();
	for (int32 d = 0; d < Devices.Num(); ++d)
	{
		DevicePerm.Add(AllPerm[d]);
		DeviceTestClock.Add(0.f);
		FMCTransitDevice& D = Devices[d];
		D.ButtonLabels.SetNum(3);
		for (int32 p = 0; p < 3; ++p) { D.ButtonLabels[p] = D.Steps[AllPerm[d][p]]; }
		D.Progress = 0; D.State = 0;
	}
	ActiveDevice = 0;
	// 7) instruments (3 drawn regardless)
	InstWobblePhase.Reset(); InstWobbleFreq.Reset();
	TArray<float> StartOff;
	for (int32 i = 0; i < MCTransit::MaxInstruments; ++i)
	{
		const float Dist = FMath::RoundToFloat(Stream.FRandRange(12.f, 35.f));
		const float Sign = Stream.FRand() < 0.5f ? -1.f : 1.f;
		StartOff.Add(C.CalTarget + Sign * Dist);
		InstWobblePhase.Add(Stream.FRandRange(0.f, 2.f * PI));
		InstWobbleFreq.Add(Stream.FRandRange(1.5f, 3.0f));
	}
	for (int32 i = 0; i < Instruments.Num(); ++i)
	{
		Instruments[i].Offset = StartOff[i];
		Instruments[i].Reading = StartOff[i];
		Instruments[i].bLocked = Instruments[i].bCalibrated = false;
	}
	ActiveInstrument = 0;

	// Switches: everything off / sensor A / transponder A.
	Switches.SetNum(MCTransit::NumSwitches);
	for (int32 i = 0; i < MCTransit::NumSwitches; ++i) { Switches[i].bOn = false; }
	RefreshSwitchLabels();

	// Burn windows
	Windows.SetNum(MCTransit::NumWindows);
	SegmentStart.SetNum(6);
	SegmentStart[0] = 0.f;
	for (int32 i = 0; i < MCTransit::NumWindows; ++i)
	{
		FMCTransitWindow W;
		W.Label = (i == MCTransit::FaultWindowIndex) ? FString(TEXT("TCM-FAULT")) : FString::Printf(TEXT("TCM-%d"), i + 1);
		if (i < MCTransit::FaultWindowIndex && C.WindowOpenSec.IsValidIndex(i))
		{
			W.OpenSec = C.WindowOpenSec[i];
			W.CloseSec = W.OpenSec + C.WindowSec;
			SegmentStart[i + 1] = W.OpenSec;
		}
		Windows[i] = W;
	}
	SegmentStart[5] = C.TransitSec;

	RoleGood.Init(0, MCTransit::NumRoles);
	RoleBad.Init(0, MCTransit::NumRoles);
	GoFlags.Init(false, MCTransit::NumRoles);
	PanelText.Init(FString(), MCTransit::NumRoles);
	bDone = false;
	TransitScore = 0;
	Result = FMCTransitReport();
	ReportText.Empty();

	StageEndTime = 0.0;
	RefreshDisplay();
	RebuildStrings();
}

// ------------------------------------------------------------------ helpers

void AMCTransitManager::SetStage(EMCTransitStage NewStage, float DurationSec)
{
	Stage = NewStage;
	StageClock = 0.f;
	StageEndTime = DurationSec > 0.f ? ServerNow(GetWorld()) + DurationSec : 0.0;
	DebugLog(FString::Printf(TEXT("stage -> %s (run %.1f s)"), StageName(NewStage), RunClock));
}

float AMCTransitManager::GetStageTimeLeft() const
{
	if (StageEndTime <= 0.0) { return 0.f; }
	return FMath::Max(0.f, static_cast<float>(StageEndTime - ServerNow(GetWorld())));
}

void AMCTransitManager::Say(const FString& Text)
{
	Message = Text;
	BP_OnMessage(Text);
}

void AMCTransitManager::OnRep_Message()
{
	BP_OnMessage(Message);
}

void AMCTransitManager::Score(int32 CrewR, bool bGood)
{
	if (CrewR < 1 || CrewR >= MCTransit::NumRoles) { return; }
	if (bGood) { RoleGood[CrewR]++; } else { RoleBad[CrewR]++; }
}

void AMCTransitManager::DebugLog(const FString& Text) const
{
	if (bDebugBot) { UE_LOG(LogMissionCrew, Log, TEXT("DEBUGTEST %s"), *Text); }
}

float AMCTransitManager::GetTrueMiss() const
{
	return FMath::Sqrt(ErrX * ErrX + ErrY * ErrY);
}

void AMCTransitManager::RefreshSwitchLabels()
{
	for (int32 i = 0; i < Switches.Num() && i < MCTransit::NumSwitches; ++i)
	{
		const bool bOn = Switches[i].bOn;
		FString State = bOn ? TEXT("ON") : TEXT("OFF");
		if (i == (int32)EMCSystemsAction::NavSensor || i == (int32)EMCSystemsAction::Transponder) { State = bOn ? TEXT("B") : TEXT("A"); }
		Switches[i].Label = FString::Printf(TEXT("%s  [%s]"), SwitchNames[i], *State);
	}
}

void AMCTransitManager::RefreshDisplay()
{
	const bool bNavB = Switches.IsValidIndex(1) && Switches[1].bOn;
	const bool bStale = bFaultHappened && !bNavB;
	bDisplayStale = bStale;
	ShipErrX = ErrX; ShipErrY = ErrY; // flight view: the true miss drives the 3D ship
	if (!bStale)
	{
		StaleX = ErrX; StaleY = ErrY;
	}
	DispX = StaleX;
	DispY = StaleY;
}

int32 AMCTransitManager::FirstOpenWindow() const
{
	for (int32 i = 0; i < Windows.Num(); ++i) { if (Windows[i].bOpen) { return i; } }
	return INDEX_NONE;
}

void AMCTransitManager::OpenWindow(int32 Index)
{
	if (!Windows.IsValidIndex(Index)) { return; }
	Windows[Index].bOpen = true;
	Say(FString::Printf(TEXT("%s window OPEN: Pilot, burn now."), *Windows[Index].Label));
	DebugLog(FString::Printf(TEXT("%s open at coast %.1f"), *Windows[Index].Label, CoastClock));
}

void AMCTransitManager::CloseWindow(int32 Index)
{
	if (!Windows.IsValidIndex(Index)) { return; }
	Windows[Index].bOpen = false;
	Windows[Index].bDone = true;
	DebugLog(FString::Printf(TEXT("%s closed, %d pulses"), *Windows[Index].Label, Windows[Index].Pulses));
}

void AMCTransitManager::RaiseAlarm(int32 Index)
{
	if (!Alarms.IsValidIndex(Index) || Alarms[Index].bActive || Alarms[Index].bFixed) { return; }
	Alarms[Index].bActive = true;
	Say(FString::Printf(TEXT("ALARM: %s"), *Alarms[Index].Text));
	DebugLog(FString::Printf(TEXT("alarm %d '%s' at coast %.1f"), Index, *Alarms[Index].Text, CoastClock));
}

// ------------------------------------------------------------------ actions

bool AMCTransitManager::HandleAction(int32 Code, int32 Who)
{
	if (!HasAuthority() || !bStarted || bDone) { return false; }

	if (Code == MCTransit::GoCode) { return DoGo(Who); }

	const int32 CrewR = MCTransit::RoleOfCode(Code);
	const int32 Act = MCTransit::ActionOfCode(Code);
	if (CrewR < 1 || CrewR > 4) { return false; }
	if (!bFreeRoles && Who != CrewR)
	{
		Say(FString::Printf(TEXT("That belongs to the %s station."), RoleNames[CrewR]));
		return false;
	}
	if (Stage == EMCTransitStage::GoPoll)
	{
		Say(TEXT("Waiting for GO from all stations."));
		return false;
	}

	switch (CrewR)
	{
	case 1: return DoSystems(Act, Who);
	case 2: return DoEquip(Act);
	case 3: return DoPilot(Act);
	default: return DoScience(Act);
	}
}

bool AMCTransitManager::DoGo(int32 Who)
{
	if (Stage != EMCTransitStage::GoPoll) { return false; }
	if (bFreeRoles)
	{
		for (int32 r = 1; r <= 4; ++r) { GoFlags[r] = true; }
		return true;
	}
	if (Who < 1 || Who > 4) { return false; }
	GoFlags[Who] = true;
	Say(FString::Printf(TEXT("%s: GO."), RoleNames[Who]));
	return true;
}

bool AMCTransitManager::DoSystems(int32 Act, int32 Who)
{
	if (Act < 0 || Act >= MCTransit::NumSwitches || !Switches.IsValidIndex(Act)) { return false; }
	Switches[Act].bOn = !Switches[Act].bOn;
	const bool bNow = Switches[Act].bOn;
	RefreshSwitchLabels();

	bool bGood = false;
	for (int32 i = 0; i < Alarms.Num(); ++i)
	{
		if (Alarms[i].bActive && !Alarms[i].bFixed && AlarmSwitch[i] == Act && AlarmWantOn[i] == bNow)
		{
			Alarms[i].bFixed = true;
			Alarms[i].bActive = false;
			bGood = true;
			Say(FString::Printf(TEXT("Alarm cleared: %s"), *Alarms[i].Text));
			if (i == MCTransit::NumAlarms - 1)
			{
				bFaultFixed = true;
				// display now shows the truth; open the extra window
				Windows[MCTransit::FaultWindowIndex].OpenSec = CoastClock;
				Windows[MCTransit::FaultWindowIndex].CloseSec = CoastClock + Cfg().WindowSec;
				Say(TEXT("NAV SENSOR B online. TCM-FAULT window open: Pilot, re-aim."));
			}
		}
	}
	if (!bGood && bNow && (Act == (int32)EMCSystemsAction::InstrumentRail || Act == (int32)EMCSystemsAction::MechanismRail))
	{
		bGood = true;
	}
	Score(1, bGood);
	RefreshDisplay();
	return true;
}

bool AMCTransitManager::DoEquip(int32 Act)
{
	if (!Switches.IsValidIndex((int32)EMCSystemsAction::MechanismRail) || !Switches[(int32)EMCSystemsAction::MechanismRail].bOn)
	{
		Say(TEXT("NO POWER: MECHANISM RAIL is OFF. Ask Systems."));
		return false;
	}
	if (Act >= 3 && Act <= 6)
	{
		const int32 Idx = Act - 3;
		if (Devices.IsValidIndex(Idx) && Devices[Idx].State == 0) { ActiveDevice = Idx; return true; }
		return false;
	}
	if (Act < 0 || Act > 2 || !Devices.IsValidIndex(ActiveDevice)) { return false; }
	FMCTransitDevice& D = Devices[ActiveDevice];
	if (D.State != 0) { return false; }
	const int32 Step = DevicePerm[ActiveDevice][Act];
	if (Step == D.Progress)
	{
		D.Progress++;
		Score(2, true);
		if (D.Progress >= 3)
		{
			D.State = 1;
			DeviceTestClock[ActiveDevice] = 0.f;
			Say(FString::Printf(TEXT("%s: running test..."), *D.Name));
		}
	}
	else
	{
		D.Progress = 0;
		Score(2, false);
		Say(FString::Printf(TEXT("%s: wrong order, procedure reset."), *D.Name));
	}
	return true;
}

bool AMCTransitManager::DoPilot(int32 Act)
{
	if (Stage != EMCTransitStage::Coast) { Say(TEXT("No burn possible now.")); return false; }
	const UMCTransitConfig& C = Cfg();
	const int32 W = FirstOpenWindow();
	if (W == INDEX_NONE)
	{
		Say(TEXT("No burn window open."));
		Score(3, false);
		return true;
	}
	const float Progress = FMath::Clamp(CoastClock / FMath::Max(1.f, C.TransitSec), 0.f, 1.f);
	const float Cost = C.NudgeFuelKg * (1.f + 2.f * Progress);
	if (FuelKg < Cost)
	{
		Say(TEXT("Not enough fuel for a pulse."));
		Score(3, false);
		return true;
	}
	const float Before = GetTrueMiss();
	switch (Act)
	{
	case 0: ErrX -= C.NudgeKm; break;
	case 1: ErrX += C.NudgeKm; break;
	case 2: ErrY += C.NudgeKm; break;
	case 3: ErrY -= C.NudgeKm; break;
	default: return false;
	}
	FuelKg -= Cost;
	Windows[W].Pulses++;
	LastPulseDir = Act; ++PulseSeq; // flight view: flare + RCS puff
	Score(3, GetTrueMiss() < Before);
	RefreshDisplay();
	return true;
}

bool AMCTransitManager::DoScience(int32 Act)
{
	if (!Switches.IsValidIndex((int32)EMCSystemsAction::InstrumentRail) || !Switches[(int32)EMCSystemsAction::InstrumentRail].bOn)
	{
		Say(TEXT("NO POWER: INSTRUMENT RAIL is OFF. Ask Systems."));
		return false;
	}
	if (Act >= 5 && Act <= 7)
	{
		const int32 Idx = Act - 5;
		if (Instruments.IsValidIndex(Idx) && !Instruments[Idx].bLocked) { ActiveInstrument = Idx; return true; }
		return false;
	}
	if (!Instruments.IsValidIndex(ActiveInstrument) || Act < 0 || Act > 4) { return false; }
	FMCTransitInstrument& I = Instruments[ActiveInstrument];
	if (I.bLocked) { return false; }
	static const float Delta[4] = { -5.f, -1.f, 1.f, 5.f };
	if (Act < 4)
	{
		I.Offset = FMath::Clamp(I.Offset + Delta[Act], 0.f, 100.f);
		return true;
	}
	// LOCK
	const UMCTransitConfig& C = Cfg();
	const float Wobble = C.WobbleAmp * FMath::Sin(RunClock * InstWobbleFreq[ActiveInstrument] + InstWobblePhase[ActiveInstrument]);
	const float Reading = I.Offset + Wobble;
	if (FMath::Abs(Reading - C.CalTarget) <= C.CalTolerance)
	{
		I.bLocked = true;
		I.bCalibrated = true;
		Score(4, true);
		Say(FString::Printf(TEXT("%s calibrated."), *I.Name));
		for (int32 k = 0; k < Instruments.Num(); ++k) { if (!Instruments[k].bLocked) { ActiveInstrument = k; break; } }
	}
	else
	{
		Score(4, false);
		Say(FString::Printf(TEXT("%s out of tolerance (%.1f). Adjust and lock again."), *I.Name, Reading));
	}
	return true;
}

// ------------------------------------------------------------------ tick

void AMCTransitManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority()) { return; }
	const float Dt = FMath::Min(DeltaSeconds, 0.5f);

	if (!bStarted)
	{
		if (bAutoStart)
		{
			PollTimer += Dt;
			if (PollTimer >= 1.f)
			{
				PollTimer = 0.f;
				FindConstructionManager();
				if (ConsMgr && ConsMgr->bLaunched) { BeginTransit(); }
			}
		}
		return;
	}

	if (!bDone)
	{
		RunClock += Dt;
		StageClock += Dt;
		StageTick(Dt);
		if (bDebugBot) { DebugBotTick(); }
	}

	StringTimer += Dt;
	if (StringTimer >= 0.2f)
	{
		StringTimer = 0.f;
		RebuildStrings();
	}
}

void AMCTransitManager::StageTick(float Dt)
{
	const UMCTransitConfig& C = Cfg();
	switch (Stage)
	{
	case EMCTransitStage::GoPoll:
		if (GoFlags[1] && GoFlags[2] && GoFlags[3] && GoFlags[4])
		{
			SetStage(EMCTransitStage::Countdown, C.CountdownSec);
			Say(TEXT("All stations GO. Countdown started."));
		}
		break;
	case EMCTransitStage::Countdown:
		if (StageClock >= C.CountdownSec)
		{
			SetStage(EMCTransitStage::Ascent, C.AscentSec);
			Say(TEXT("LIFT-OFF!"));
		}
		break;
	case EMCTransitStage::Ascent:
		if (StageClock >= C.AscentSec)
		{
			if (AnomalyRoll > LaunchReliability)
			{
				bAnomaly = true;
				ErrX += AnomalyDirX * C.AnomalyErrKm;
				ErrY += AnomalyDirY * C.AnomalyErrKm;
				FuelKg = FMath::Max(0.f, FuelKg - FuelStartKg * C.AnomalyFuelFrac);
				Say(TEXT("Ascent anomaly: course error and fuel loss."));
			}
			RefreshDisplay();
			DebugLog(FString::Printf(TEXT("ascent done anomaly=%d"), bAnomaly ? 1 : 0));
			CoastClock = 0.f;
			SetStage(EMCTransitStage::Coast, C.TransitSec);
			Say(TEXT("Orbit insertion burn complete. Coasting to the Moon."));
		}
		break;
	case EMCTransitStage::Coast:
		CoastTick(Dt);
		break;
	default: break;
	}

	// Equipment test timers run in every started stage.
	for (int32 d = 0; d < Devices.Num(); ++d)
	{
		if (Devices[d].State == 1)
		{
			DeviceTestClock[d] += Dt;
			if (DeviceTestClock[d] >= C.DeviceTestSec)
			{
				Devices[d].State = 2;
				Say(FString::Printf(TEXT("%s: TEST PASSED."), *Devices[d].Name));
				if (ActiveDevice == d)
				{
					for (int32 k = 0; k < Devices.Num(); ++k) { if (Devices[k].State == 0) { ActiveDevice = k; break; } }
				}
			}
		}
	}
	// Live instrument readings.
	for (int32 i = 0; i < Instruments.Num(); ++i)
	{
		Instruments[i].Reading = Instruments[i].Offset + C.WobbleAmp * FMath::Sin(RunClock * InstWobbleFreq[i] + InstWobblePhase[i]);
	}
}

void AMCTransitManager::CoastTick(float Dt)
{
	const UMCTransitConfig& C = Cfg();
	CoastClock += Dt;
	ElapsedSec = CoastClock;

	// Course drift of the current segment.
	int32 Seg = 0;
	for (int32 i = 0; i < MCTransit::FaultWindowIndex; ++i) { if (CoastClock >= Windows[i].OpenSec && Windows[i].OpenSec >= 0.f) { Seg = i + 1; } }
	const float SegLen = FMath::Max(1.f, SegmentStart[Seg + 1] - SegmentStart[Seg]);
	ErrX += DriftX[Seg] / SegLen * Dt;
	ErrY += DriftY[Seg] / SegLen * Dt;

	// Windows.
	for (int32 i = 0; i < Windows.Num(); ++i)
	{
		FMCTransitWindow& W = Windows[i];
		if (W.OpenSec < 0.f || W.bDone) { continue; }
		if (!W.bOpen && CoastClock >= W.OpenSec) { OpenWindow(i); }
		else if (W.bOpen && CoastClock >= W.CloseSec) { CloseWindow(i); }
	}

	// Alarms (index 3 = sensor fault, handled below).
	for (int32 i = 0; i < MCTransit::NumAlarms - 1; ++i)
	{
		if (CoastClock >= AlarmTime[i]) { RaiseAlarm(i); }
	}

	// Sensor fault: the true course jumps, the display keeps the old value.
	if (!bFaultHappened && CoastClock >= FaultTime)
	{
		RefreshDisplay(); // freeze the display at the pre-kick value
		ErrX += KickX; ErrY += KickY;
		bFaultHappened = true;
		RaiseAlarm(MCTransit::NumAlarms - 1);
		DebugLog(FString::Printf(TEXT("SENSOR FAULT at coast %.1f kick (%.1f, %.1f)"), CoastClock, KickX, KickY));
		if (Switches[(int32)EMCSystemsAction::NavSensor].bOn)
		{
			// Sensor B was already selected: the fault is harmless, still open the extra window.
			Alarms[MCTransit::NumAlarms - 1].bFixed = true;
			Alarms[MCTransit::NumAlarms - 1].bActive = false;
			bFaultFixed = true;
			Windows[MCTransit::FaultWindowIndex].OpenSec = CoastClock;
			Windows[MCTransit::FaultWindowIndex].CloseSec = CoastClock + C.WindowSec;
		}
		RefreshDisplay();
	}

	// Health drain per open alarm.
	int32 Open = 0;
	for (const FMCTransitAlarm& A : Alarms) { if (A.bActive && !A.bFixed) { ++Open; } }
	Health = FMath::Max(0.f, Health - Open * C.HealthDrainPerAlarmSec * Dt);

	RefreshDisplay();

	if (CoastClock >= C.TransitSec) { Arrive(); }
}

void AMCTransitManager::Arrive()
{
	const UMCTransitConfig& C = Cfg();
	const float Miss = GetTrueMiss();
	const bool bIn = Miss <= C.CorridorKm;
	if (!bIn)
	{
		FuelKg = FMath::Max(0.f, FuelKg - (Miss - C.CorridorKm) * C.OffCorridorFuelKgPerKm);
	}

	FMCTransitReport R;
	R.MissKm = Miss;
	R.bInCorridor = bIn;
	R.FuelLeftKg = FuelKg;
	R.Health = Health;
	R.bAnomaly = bAnomaly;
	R.InstrumentsTotal = Instruments.Num();
	for (const FMCTransitInstrument& I : Instruments) { if (I.bCalibrated) { R.InstrumentsCalibrated++; } }
	R.DevicesTotal = Devices.Num();
	for (const FMCTransitDevice& D : Devices) { if (D.State == 2) { R.DevicesPassed++; } }

	R.CourseScore = bIn ? C.WeightCourse : C.WeightCourse * FMath::Max(0.f, 1.f - (Miss - C.CorridorKm) / FMath::Max(1.f, C.CourseFalloffKm));
	R.InstrumentScore = R.InstrumentsTotal == 0 ? C.WeightInstruments : C.WeightInstruments * R.InstrumentsCalibrated / R.InstrumentsTotal;
	R.EquipmentScore = R.DevicesTotal == 0 ? C.WeightEquipment : C.WeightEquipment * R.DevicesPassed / R.DevicesTotal;
	R.HealthScore = C.WeightHealth * FMath::Clamp(Health / 100.f, 0.f, 1.f);
	R.FuelScore = (FuelReserveKg <= 0.f || FuelKg >= FuelReserveKg) ? C.WeightFuel : C.WeightFuel * FuelKg / FuelReserveKg;
	R.Score = FMath::RoundToInt(R.CourseScore + R.InstrumentScore + R.EquipmentScore + R.HealthScore + R.FuelScore);

	Result = R;
	TransitScore = R.Score;

	FString T = TEXT("S2 LAUNCH & TRANSIT REPORT\n");
	T += FString::Printf(TEXT("Seed %d%s\n"), Seed, bAnomaly ? TEXT("  (ascent anomaly)") : TEXT(""));
	T += FString::Printf(TEXT("Course: miss %.1f km, corridor %.0f km -> %s  (%.0f/%.0f)\n"), Miss, C.CorridorKm, bIn ? TEXT("ON COURSE") : TEXT("OFF COURSE"), R.CourseScore, C.WeightCourse);
	T += FString::Printf(TEXT("Instruments: %d/%d calibrated  (%.0f/%.0f)\n"), R.InstrumentsCalibrated, R.InstrumentsTotal, R.InstrumentScore, C.WeightInstruments);
	T += FString::Printf(TEXT("Equipment: %d/%d tests passed  (%.0f/%.0f)\n"), R.DevicesPassed, R.DevicesTotal, R.EquipmentScore, C.WeightEquipment);
	T += FString::Printf(TEXT("Ship health: %.0f%%  (%.0f/%.0f)\n"), Health, R.HealthScore, C.WeightHealth);
	T += FString::Printf(TEXT("Fuel left: %.0f kg, landing reserve %.0f kg  (%.0f/%.0f)\n"), FuelKg, FuelReserveKg, R.FuelScore, C.WeightFuel);
	T += FString::Printf(TEXT("TOTAL SCORE: %d / 100\n"), R.Score);
	T += TEXT("Accuracy (good/bad actions):\n");
	for (int32 r = 1; r <= 4; ++r)
	{
		const int32 G = RoleGood[r], B = RoleBad[r];
		T += FString::Printf(TEXT("  %s: %d / %d  (%d%%)\n"), RoleNames[r], G, B, (G + B) > 0 ? FMath::RoundToInt(100.f * G / (G + B)) : 100);
	}
	ReportText = T;

	SetStage(EMCTransitStage::Arrival, 0.f);
	bDone = true;
	Say(FString::Printf(TEXT("Arrived at the Moon. Score %d / 100."), R.Score));
	UE_LOG(LogMissionCrew, Log, TEXT("%s"), *T);
	DebugLog(FString::Printf(TEXT("DONE score=%d miss=%.1f inCorridor=%d fuel=%.0f health=%.0f instr=%d/%d dev=%d/%d"),
		R.Score, Miss, bIn ? 1 : 0, FuelKg, Health, R.InstrumentsCalibrated, R.InstrumentsTotal, R.DevicesPassed, R.DevicesTotal));
	RebuildStrings();
	if (UMCMissionSubsystem* Mis = UMCMissionSubsystem::Get(this)) { Mis->RecordTransit(Result); }
	OnTransitComplete.Broadcast(Result);
	if (HasAuthority() && !NextMapName.IsEmpty() && (!bDebugAutoTest || FParse::Param(FCommandLine::Get(), TEXT("MCTravelTest"))) && GetWorld())
	{
		FTimerHandle H;
		GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (UWorld* W = GetWorld())
			{
				FString URL = NextMapName;
				if (GetNetMode() == NM_ListenServer && !URL.Contains(TEXT("listen"))) URL += TEXT("?listen");
				UE_LOG(LogMissionCrew, Log, TEXT("Transit: ServerTravel to %s"), *URL);
				W->ServerTravel(URL);
			}
		}), FMath::Max(1.f, TravelDelaySec), false);
	}
}

// ------------------------------------------------------------------ strings

void AMCTransitManager::RebuildStrings()
{
	const UMCTransitConfig& C = Cfg();
	if (PanelText.Num() < MCTransit::NumRoles) { PanelText.Init(FString(), MCTransit::NumRoles); }

	FString Clock;
	if (Stage == EMCTransitStage::GoPoll) { Clock = TEXT("waiting for GO"); }
	else if (Stage == EMCTransitStage::Coast) { Clock = FString::Printf(TEXT("T+%.0f / %.0f s"), CoastClock, C.TransitSec); }
	else if (Stage == EMCTransitStage::Arrival) { Clock = TEXT("done"); }
	else { Clock = FString::Printf(TEXT("%.0f s"), GetStageTimeLeft()); }
	TopText = FString::Printf(TEXT("%s  |  %s  |  HEALTH %.0f%%  |  FUEL %.0f kg"), StageName(Stage), *Clock, Health, FuelKg);

	// Board: shared status of the four jobs (only what every station may see).
	int32 DevPass = 0, InstOk = 0, OpenAlarms = 0;
	for (const FMCTransitDevice& D : Devices) { if (D.State == 2) { ++DevPass; } }
	for (const FMCTransitInstrument& I : Instruments) { if (I.bCalibrated) { ++InstOk; } }
	for (const FMCTransitAlarm& A : Alarms) { if (A.bActive && !A.bFixed) { ++OpenAlarms; } }
	const float DispMiss = FMath::Sqrt(DispX * DispX + DispY * DispY);
	FString Go;
	for (int32 r = 1; r <= 4; ++r) { Go += FString::Printf(TEXT("%s%s"), RoleNames[r], GoFlags[r] ? TEXT(" GO  ") : TEXT(" --  ")); }
	BoardText = (Stage == EMCTransitStage::GoPoll)
		? FString::Printf(TEXT("GO/NO-GO: %s"), *Go)
		: FString::Printf(TEXT("SYSTEMS alarms open %d\nEQUIPMENT tests %d/%d\nPILOT miss %.1f km (corridor %.0f)\nSCIENCE calibrated %d/%d"),
			OpenAlarms, DevPass, Devices.Num(), DispMiss, C.CorridorKm, InstOk, Instruments.Num());

	// Systems
	{
		FString S;
		if (Stage == EMCTransitStage::GoPoll) { S = TEXT("Press GO FOR LAUNCH.\nBoth rails are OFF: switch them ON for Equipment and Science."); }
		else
		{
			S = TEXT("ALARMS\n");
			bool bAny = false;
			for (const FMCTransitAlarm& A : Alarms)
			{
				if (A.bActive && !A.bFixed) { S += FString::Printf(TEXT("  ! %s\n"), *A.Text); bAny = true; }
			}
			if (!bAny) { S += TEXT("  none\n"); }
			S += TEXT("Flip the switch that cures the alarm. Wrong flips count against you.");
		}
		PanelText[1] = S;
	}
	// Equipment
	{
		FString S;
		if (!Switches.IsValidIndex(5) || !Switches[5].bOn) { S = TEXT("MECHANISM RAIL OFF. Ask Systems to switch it ON.\n"); }
		for (int32 d = 0; d < Devices.Num(); ++d)
		{
			const FMCTransitDevice& D = Devices[d];
			S += FString::Printf(TEXT("%s%s: %s\n"), d == ActiveDevice ? TEXT("> ") : TEXT("  "), *D.Name,
				D.State == 2 ? TEXT("PASS") : (D.State == 1 ? TEXT("TESTING...") : *FString::Printf(TEXT("step %d/3"), D.Progress)));
		}
		if (Devices.IsValidIndex(ActiveDevice))
		{
			const FMCTransitDevice& D = Devices[ActiveDevice];
			S += FString::Printf(TEXT("Procedure: 1 %s, 2 %s, 3 %s"), *D.Steps[0], *D.Steps[1], *D.Steps[2]);
		}
		PanelText[2] = S;
	}
	// Pilot
	{
		FString S = FString::Printf(TEXT("PREDICTED MISS  X %+.1f  Y %+.1f km  (|d| %.1f, corridor %.0f)\nFUEL %.0f kg\n"), DispX, DispY, DispMiss, C.CorridorKm, FuelKg);
		const int32 W = FirstOpenWindow();
		if (Stage != EMCTransitStage::Coast) { S += TEXT("Burns open after orbit insertion."); }
		else if (W != INDEX_NONE) { S += FString::Printf(TEXT("%s OPEN  %.0f s left"), *Windows[W].Label, Windows[W].CloseSec - CoastClock); }
		else
		{
			float Next = -1.f; FString Lab;
			for (const FMCTransitWindow& Wi : Windows) { if (!Wi.bDone && Wi.OpenSec >= CoastClock && (Next < 0.f || Wi.OpenSec < Next)) { Next = Wi.OpenSec; Lab = Wi.Label; } }
			S += Next >= 0.f ? FString::Printf(TEXT("Next window %s in %.0f s"), *Lab, Next - CoastClock) : FString(TEXT("No more windows."));
		}
		PanelText[3] = S;
	}
	// Science
	{
		FString S;
		if (!Switches.IsValidIndex(4) || !Switches[4].bOn) { S = TEXT("INSTRUMENT RAIL OFF. Ask Systems to switch it ON.\n"); }
		if (Instruments.Num() == 0) { S += TEXT("No instruments aboard: nothing to calibrate."); }
		for (int32 i = 0; i < Instruments.Num(); ++i)
		{
			const FMCTransitInstrument& I = Instruments[i];
			S += FString::Printf(TEXT("%s%s: %s\n"), i == ActiveInstrument ? TEXT("> ") : TEXT("  "), *I.Name,
				I.bLocked ? TEXT("LOCKED OK") : *FString::Printf(TEXT("%.1f  (goal %.0f +-%.0f)"), I.Reading, C.CalTarget, C.CalTolerance));
		}
		PanelText[4] = S;
	}
	PanelText[0] = Message;
}

// ------------------------------------------------------------------ debug bot

void AMCTransitManager::RunDebugTest()
{
	if (!HasAuthority()) { return; }
	bFreeRoles = true;
	bDebugBot = true;
	BotTimer = 0.f;
	DebugLog(TEXT("run requested"));
	if (!bStarted)
	{
		bTestMode = true;
		BeginTransit();
	}
	DebugLog(FString::Printf(TEXT("SEEDCHECK seed=%d init=(%.2f,%.2f) fault=%.2f kick=(%.2f,%.2f) anomalyRoll=%.3f reliability=%.2f"),
		Seed, InitErrX, InitErrY, FaultTime, KickX, KickY, AnomalyRoll, LaunchReliability));
}

void AMCTransitManager::DebugBotTick()
{
	BotTimer += 0.1f;
	if (BotTimer < 0.25f) { return; }
	BotTimer = 0.f;
	const UMCTransitConfig& C = Cfg();

	if (Stage == EMCTransitStage::GoPoll) { HandleAction(MCTransit::GoCode, 0); return; }

	const auto Press = [this](EMCTransitRole R, int32 A) { HandleAction(MCTransit::MakeCode(R, A), 0); };

	// Systems: rails on, cure active alarms.
	for (int32 rail : { (int32)EMCSystemsAction::InstrumentRail, (int32)EMCSystemsAction::MechanismRail })
	{
		if (!Switches[rail].bOn) { Press(EMCTransitRole::Systems, rail); return; }
	}
	for (int32 i = 0; i < Alarms.Num(); ++i)
	{
		if (Alarms[i].bActive && !Alarms[i].bFixed && Switches[AlarmSwitch[i]].bOn != AlarmWantOn[i])
		{
			Press(EMCTransitRole::Systems, AlarmSwitch[i]);
			return;
		}
	}
	// Equipment
	if (Devices.IsValidIndex(ActiveDevice))
	{
		const FMCTransitDevice& D = Devices[ActiveDevice];
		if (D.State == 0)
		{
			for (int32 p = 0; p < 3; ++p)
			{
				if (DevicePerm[ActiveDevice][p] == D.Progress) { Press(EMCTransitRole::Equipment, p); break; }
			}
		}
	}
	// Science
	if (Instruments.IsValidIndex(ActiveInstrument) && !Instruments[ActiveInstrument].bLocked)
	{
		const float D = C.CalTarget - Instruments[ActiveInstrument].Offset;
		if (D >= 5.f)        { Press(EMCTransitRole::Science, (int32)EMCScienceAction::Plus5); }
		else if (D <= -5.f)  { Press(EMCTransitRole::Science, (int32)EMCScienceAction::Minus5); }
		else if (D >= 1.f)   { Press(EMCTransitRole::Science, (int32)EMCScienceAction::Plus1); }
		else if (D <= -1.f)  { Press(EMCTransitRole::Science, (int32)EMCScienceAction::Minus1); }
		else                 { Press(EMCTransitRole::Science, (int32)EMCScienceAction::Lock); }
	}
	// Pilot: pulse on the largest true axis while a pulse still helps (needs a window; sensor B fixes the display, not the truth).
	if (Stage == EMCTransitStage::Coast && FirstOpenWindow() != INDEX_NONE)
	{
		const float Half = C.NudgeKm * 0.5f;
		if (FMath::Abs(ErrX) >= FMath::Abs(ErrY) && FMath::Abs(ErrX) > Half)
		{
			Press(EMCTransitRole::Pilot, ErrX > 0.f ? (int32)EMCPilotAction::Left : (int32)EMCPilotAction::Right);
		}
		else if (FMath::Abs(ErrY) > Half)
		{
			Press(EMCTransitRole::Pilot, ErrY > 0.f ? (int32)EMCPilotAction::Down : (int32)EMCPilotAction::Up);
		}
	}
}
