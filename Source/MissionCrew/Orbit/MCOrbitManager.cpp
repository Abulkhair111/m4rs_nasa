#include "Orbit/MCOrbitManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Orbit/MCOrbitRelay.h"
#include "Orbit/UI/MCOrbitHudWidget.h"
#include "Core/MCMissionSubsystem.h"
#include "MissionCrew.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* RoleNames[] = { TEXT("-"), TEXT("SYSTEMS OPERATOR"), TEXT("SAFETY OFFICER"), TEXT("PILOT"), TEXT("LANDING ANALYST") };
	const TCHAR* IssueTexts[] = { TEXT(""), TEXT("RADAR RAIL OFF - altitude readout would be invalid"), TEXT("hazard scan data stale"),
		TEXT("descent plan not loaded"), TEXT("ice layer stale") };
	const TCHAR* RailNames[] = { TEXT("RADAR / LIDAR"), TEXT("ENGINE PRESSURE"), TEXT("LEGS UNLOCK"), TEXT("THRUSTERS") };
	const TCHAR* ZoneNames[] = { TEXT("N"), TEXT("E"), TEXT("S"), TEXT("W") };
	FVector2D ZoneDir(int32 Z) { static const FVector2D D[4] = { {0, 1}, {1, 0}, {0, -1}, {-1, 0} }; return D[FMath::Clamp(Z, 0, 3)]; }
}

AMCOrbitManager::AMCOrbitManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(10.f);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AMCOrbitManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCOrbitManager, bFreeRoles);
	DOREPLIFETIME(AMCOrbitManager, Stage); DOREPLIFETIME(AMCOrbitManager, bStarted); DOREPLIFETIME(AMCOrbitManager, bDone);
	DOREPLIFETIME(AMCOrbitManager, Seed); DOREPLIFETIME(AMCOrbitManager, StageEndTime); DOREPLIFETIME(AMCOrbitManager, ElapsedSec);
	DOREPLIFETIME(AMCOrbitManager, GoFlags); DOREPLIFETIME(AMCOrbitManager, NoGoFlags);
	DOREPLIFETIME(AMCOrbitManager, Health); DOREPLIFETIME(AMCOrbitManager, FuelKg); DOREPLIFETIME(AMCOrbitManager, FuelStartKg);
	DOREPLIFETIME(AMCOrbitManager, Sites); DOREPLIFETIME(AMCOrbitManager, Hazards);
	DOREPLIFETIME(AMCOrbitManager, ProposedSite); DOREPLIFETIME(AMCOrbitManager, ChosenSite); DOREPLIFETIME(AMCOrbitManager, SafetyViewSite);
	DOREPLIFETIME(AMCOrbitManager, bSiteConfirmed); DOREPLIFETIME(AMCOrbitManager, bSiteRejected);
	DOREPLIFETIME(AMCOrbitManager, Rails); DOREPLIFETIME(AMCOrbitManager, bLimitsReported);
	DOREPLIFETIME(AMCOrbitManager, AltM); DOREPLIFETIME(AMCOrbitManager, VertSpeed); DOREPLIFETIME(AMCOrbitManager, PosX); DOREPLIFETIME(AMCOrbitManager, PosY);
	DOREPLIFETIME(AMCOrbitManager, VelX); DOREPLIFETIME(AMCOrbitManager, VelY); DOREPLIFETIME(AMCOrbitManager, Throttle);
	DOREPLIFETIME(AMCOrbitManager, TargetX); DOREPLIFETIME(AMCOrbitManager, TargetY); DOREPLIFETIME(AMCOrbitManager, SafeRadiusM);
	DOREPLIFETIME(AMCOrbitManager, bAltValid); DOREPLIFETIME(AMCOrbitManager, bLegsDeployed); DOREPLIFETIME(AMCOrbitManager, bBoulderField);
	DOREPLIFETIME(AMCOrbitManager, BoulderRadiusM); DOREPLIFETIME(AMCOrbitManager, SafetyZoneClear); DOREPLIFETIME(AMCOrbitManager, DivertZone);
	DOREPLIFETIME(AMCOrbitManager, bTouchedDown);
	DOREPLIFETIME(AMCOrbitManager, TopText); DOREPLIFETIME(AMCOrbitManager, BoardText); DOREPLIFETIME(AMCOrbitManager, PanelText); DOREPLIFETIME(AMCOrbitManager, Message);
	DOREPLIFETIME(AMCOrbitManager, OrbitScore); DOREPLIFETIME(AMCOrbitManager, Result); DOREPLIFETIME(AMCOrbitManager, RoleGood); DOREPLIFETIME(AMCOrbitManager, RoleBad);
	DOREPLIFETIME(AMCOrbitManager, ReportText);
}

const UMCOrbitConfig& AMCOrbitManager::Cfg() const
{
	if (Config) return *Config;
	return *DefaultConfig;
}

const TCHAR* AMCOrbitManager::StageName(EMCOrbitStage S)
{
	switch (S)
	{
	case EMCOrbitStage::GoPoll: return TEXT("GO / NO-GO");
	case EMCOrbitStage::Insertion: return TEXT("ORBIT INSERTION");
	case EMCOrbitStage::Survey: return TEXT("SITE SURVEY");
	
	case EMCOrbitStage::SiteLock: return TEXT("SITE LOCK");
	case EMCOrbitStage::Descent: return TEXT("POWERED DESCENT");
	case EMCOrbitStage::Report: return TEXT("TOUCHDOWN REPORT");
	}
	return TEXT("?");
}

int32 AMCOrbitManager::GetCrewRoleOfController(const AController* Controller)
{
	const APlayerState* PS = Controller ? Controller->PlayerState.Get() : nullptr;
	if (!PS) return 0;
	return UMCMissionSubsystem::ReadCrewRole(PS);
}

AMCOrbitRelay* AMCOrbitManager::FindRelayFor(UWorld* World, const AController* Controller)
{
	if (!World || !Controller) return nullptr;
	for (TActorIterator<AMCOrbitRelay> It(World); It; ++It) { if (It->GetOwner() == Controller) return *It; }
	return nullptr;
}

void AMCOrbitManager::SendAction(APlayerController* PC, int32 Code)
{
	if (!PC) return;
	if (AMCOrbitRelay* R = FindRelayFor(PC->GetWorld(), PC)) { R->ServerAction(Code); return; }
	if (PC->HasAuthority())
		for (TActorIterator<AMCOrbitManager> It(PC->GetWorld()); It; ++It) { It->HandleAction(Code, GetCrewRoleOfController(PC)); break; }
}

float AMCOrbitManager::GetStageTimeLeft() const
{
	if (StageEndTime <= 0.0 || !GetWorld()) return 0.f;
	return FMath::Max(0.f, (float)(StageEndTime - NowSec()));
}

void AMCOrbitManager::OnRep_Message() { BP_OnMessage(Message); }

void AMCOrbitManager::DebugLog(const FString& T) const
{
	if (bDebugBot) UE_LOG(LogMissionCrew, Log, TEXT("DEBUGTEST [S3] %s"), *T);
}

// ------------------------------------------------------------------ start

void AMCOrbitManager::BeginPlay()
{
	Super::BeginPlay();
	DefaultConfig = NewObject<UMCOrbitConfig>(this);
	if (!HasAuthority()) return;
	if (bDebugAutoTest) bFreeRoles = true;
	if (bAutoStart) BeginOrbit();
	if (bDebugAutoTest) RunDebugTest();
}

void AMCOrbitManager::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Hud) { Hud->RemoveFromParent(); Hud = nullptr; }
	Super::EndPlay(Reason);
}

void AMCOrbitManager::BeginOrbit()
{
	if (!HasAuthority() || bBegun) return;
	bBegun = true;
	if (!DefaultConfig) DefaultConfig = NewObject<UMCOrbitConfig>(this);
	const UMCOrbitConfig& C = Cfg();
	Seed = SeedOverride != 0 ? SeedOverride : FMath::RandRange(1, 99999);
	Stream.Initialize(Seed);

	// ---- carry-over from S1/S2 (UMCMissionSubsystem; test defaults when absent) ----
	FuelKg = C.DefaultFuelKg; Health = C.DefaultHealth;
	bHasIceInst = true; CalibFrac = 0.5f; bLegsOkS2 = true; MissKmS2 = 0.f; bInCorridorS2 = true;
	if (const UMCMissionSubsystem* M = UMCMissionSubsystem::Get(this))
	{
		if (M->bHasTransit)
		{
			FuelKg = M->Transit.FuelLeftKg; Health = M->Transit.Health;
			CalibFrac = M->Transit.InstrumentsTotal > 0 ? (float)M->Transit.InstrumentsCalibrated / M->Transit.InstrumentsTotal : 0.f;
			bLegsOkS2 = M->Transit.DevicesTotal == 0 || M->Transit.DevicesPassed >= M->Transit.DevicesTotal;
			MissKmS2 = M->Transit.MissKm; bInCorridorS2 = M->Transit.bInCorridor;
		}
		if (M->bHasLoadout) bHasIceInst = M->bHasIceInstrument;
	}
	FuelKg = FMath::Max(0.f, FuelKg - (bInCorridorS2 ? 0.f : MissKmS2 * C.InsertionFuelPerKmOff));
	FuelStartKg = FuelKg; InitialFuel = FuelKg;
	LimitVert = C.MaxVertSpeed; LimitHoriz = C.MaxHorizSpeed;

	// ---- sites ----
	TArray<FMCOrbitSiteDef> Defs = C.Sites;
	if (Defs.Num() > MCOrbit::MaxSites) Defs.SetNum(MCOrbit::MaxSites);
	if (Defs.Num() == 0) { Defs.Add(FMCOrbitSiteDef()); }
	const float Noise = CalibFrac > 0.5f ? C.IceNoiseCalibrated : C.IceNoiseUncalibrated;
	Sites.Reset(); Hazards.Reset(); IceTrue.Reset(); HazardReal.Reset();
	for (int32 s = 0; s < Defs.Num(); ++s)
	{
		const FMCOrbitSiteDef& D = Defs[s];
		const float Ice = FMath::Clamp(D.IceTrue + Stream.FRandRange(-0.03f, 0.03f), 0.f, 1.f);
		IceTrue.Add(Ice);
		FMCOrbitSite S; S.Name = D.Name; S.MapX = D.MapX; S.MapY = D.MapY;
		S.DisplayIce = bHasIceInst ? FMath::Clamp(Ice + Stream.FRandRange(-Noise, Noise), 0.f, 1.f) : -1.f;
		Sites.Add(S);
		const int32 Real = FMath::Clamp(D.RealHazards >= 0 ? D.RealHazards : FMath::RoundToInt(Ice * 4.f) + 1, 0, MCOrbit::HazardsPerSite);
		TArray<int32> Slots; for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i) Slots.Add(i);
		for (int32 i = Slots.Num() - 1; i > 0; --i) Slots.Swap(i, Stream.RandRange(0, i));
		for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i)
		{
			FMCOrbitHazard H; H.Site = s; H.Type = Stream.RandRange(0, 2);
			const float Ang = Stream.FRandRange(0.f, 2.f * PI), Rad = Stream.FRandRange(5.f, C.SafeRadiusM * 1.3f);
			H.X = FMath::Cos(Ang) * Rad; H.Y = FMath::Sin(Ang) * Rad; H.Radius = Stream.FRandRange(6.f, 14.f);
			Hazards.Add(H);
			HazardReal.Add(Slots.IndexOfByKey(i) < Real);   // slot position < Real = real
		}
	}
	TrueRank.Init(0, Defs.Num());
	{
		TArray<int32> Order; for (int32 i = 0; i < Defs.Num(); ++i) Order.Add(i);
		Order.Sort([this](int32 A, int32 B) { return IceTrue[A] > IceTrue[B]; });
		for (int32 r = 0; r < Order.Num(); ++r) TrueRank[Order[r]] = r + 1;
	}

	// ---- rails, GO-poll issues, event times, zones ----
	Rails.Reset();
	for (int32 i = 0; i < MCOrbit::NumRails; ++i) { FMCOrbitRail R; R.Name = RailNames[i]; Rails.Add(R); }
	IssueRole.Init(0, MCOrbit::NumRoles); IssueText.Init(FString(), MCOrbit::NumRoles); IssueCleared.Init(false, MCOrbit::NumRoles);
	int32 Count = 0;
	for (int32 r = 1; r <= 4; ++r) if (Stream.FRand() < 0.4f) { IssueRole[r] = 1; ++Count; }
	if (Count == 0) IssueRole[Stream.RandRange(1, 4)] = 1;
	for (int32 r = 1; r <= 4; ++r) if (IssueRole[r]) IssueText[r] = IssueTexts[r];
	if (IssueRole[1]) Rails[0].bOn = false;
	EventTime = Stream.FRandRange(C.EventDelayMinSec, C.EventDelayMaxSec);
	RailTripTime = Stream.FRandRange(C.RailTripMinSec, C.RailTripMaxSec);
	TrippedRail = Stream.RandRange(0, 3);
	ZoneDensity.Init(0.f, 4); ZoneIce.Init(0.f, 4);
	for (int32 z = 0; z < 4; ++z) ZoneDensity[z] = Stream.FRandRange(15.f, 60.f);
	SafeZone = 0; for (int32 z = 1; z < 4; ++z) if (ZoneDensity[z] < ZoneDensity[SafeZone]) SafeZone = z;
	ZoneDensity[SafeZone] = FMath::Min(ZoneDensity[SafeZone], 6.f);

	GoFlags.Init(false, 5); NoGoFlags.Init(false, 5); RoleGood.Init(0, 5); RoleBad.Init(0, 5); PanelText.Init(FString(), 5);
	bStarted = true;
	SetStage(EMCOrbitStage::GoPoll, 0.f);
	PollHoldUntil = 0.f;
	Say(TEXT("Orbit & Landing: every role runs its pre-checks, then GO or NO-GO."));
	RebuildStrings();
	UE_LOG(LogMissionCrew, Log, TEXT("Orbit: begin, seed %d, fuel %.0f kg, %d sites"), Seed, FuelKg, Sites.Num());
}

// ------------------------------------------------------------------ helpers

void AMCOrbitManager::SetStage(EMCOrbitStage S, float Dur)
{
	Stage = S; StageClock = 0.f;
	StageEndTime = Dur > 0.f && GetWorld() ? NowSec() + Dur : 0.0;
}

void AMCOrbitManager::Say(const FString& T) { Message = T; BP_OnMessage(T); if (PanelText.Num() > 0) PanelText[0] = T; }

void AMCOrbitManager::Score(int32 Rl, bool bGood)
{
	if (Rl < 1 || Rl > 4 || RoleGood.Num() < 5) return;
	(bGood ? RoleGood : RoleBad)[Rl]++;
}

bool AMCOrbitManager::RailOn(int32 I) const { return Rails.IsValidIndex(I) ? Rails[I].bOn : true; }
float AMCOrbitManager::VertLimit() const { return (!bLegsOkS2 || !RailOn(2)) ? Cfg().LegsFailMaxVert : LimitVert; }
float AMCOrbitManager::HorizLimit() const { return LimitHoriz; }

FString AMCOrbitManager::CompassOf(float Dx, float Dy)
{
	const float D = FMath::Sqrt(Dx * Dx + Dy * Dy);
	if (D < 1.f) return TEXT("on target");
	static const TCHAR* P[] = { TEXT("E"), TEXT("NE"), TEXT("N"), TEXT("NW"), TEXT("W"), TEXT("SW"), TEXT("S"), TEXT("SE") };
	const int32 Idx = (FMath::RoundToInt(FMath::Atan2(Dy, Dx) / (PI / 4.f)) + 8) % 8;
	return FString::Printf(TEXT("%.0f m %s"), D, P[Idx]);
}

// ------------------------------------------------------------------ actions

bool AMCOrbitManager::HandleAction(int32 Code, int32 Who)
{
	if (!HasAuthority() || !bBegun || bDone) return false;
	const int32 Rl = Code / 100, Action = Code % 100;
	if (Code == MCOrbit::GoCode || Code == MCOrbit::NoGoCode)
	{
		if (!bFreeRoles && (Who < 1 || Who > 4)) return false;
		return DoGo(Who, Code == MCOrbit::GoCode);
	}
	if (Rl < 1 || Rl > 4) return false;
	if (!bFreeRoles && Who != Rl) return false;
	switch (Rl)
	{
	case 1: return DoSystems(Action);
	case 2: return DoSafety(Action);
	case 3: return DoPilot(Action);
	default: return DoAnalyst(Action);
	}
}

bool AMCOrbitManager::DoGo(int32 Who, bool bGo)
{
	if (Stage != EMCOrbitStage::GoPoll) return false;
	const int32 R0 = (Who >= 1 && Who <= 4) ? Who : 1, R1 = (Who >= 1 && Who <= 4) ? Who : 4;
	for (int32 r = R0; r <= R1; ++r)
	{
		if (GoFlags[r]) continue;
		const bool bIssue = IssueRole[r] && !IssueCleared[r];
		if (bGo)
		{
			if (bIssue) { Score(r, false); Health = FMath::Max(0.f, Health - 3.f); Say(FString::Printf(TEXT("%s said GO with an open issue (%s). It stays unresolved."), RoleNames[r], *IssueText[r])); }
			else Score(r, true);
			GoFlags[r] = true;
		}
		else
		{
			if (bIssue)
			{
				Score(r, true); IssueCleared[r] = true; NoGoFlags[r] = true;
				if (r == 1) Rails[0].bOn = true;
				PollHoldUntil = FMath::Max(PollHoldUntil, RunClock + Cfg().NoGoHoldSec);
				Say(FString::Printf(TEXT("%s NO-GO: %s. Holding, issue cleared."), RoleNames[r], *IssueText[r]));
			}
			else { Score(r, false); Say(FString::Printf(TEXT("%s NO-GO without cause: delay."), RoleNames[r])); PollHoldUntil = FMath::Max(PollHoldUntil, RunClock + 2.f); }
		}
	}
	return true;
}

bool AMCOrbitManager::DoSystems(int32 A)
{
	if (A >= 0 && A < MCOrbit::NumRails)
	{
		FMCOrbitRail& R = Rails[A];
		R.bOn = !R.bOn;
		if (R.bOn && R.bTripped) { R.bTripped = false; bTripFixed = true; Score(1, true); Say(FString::Printf(TEXT("Systems restored %s."), RailNames[A])); }
		else if (!R.bOn) { Score(1, false); Say(FString::Printf(TEXT("%s switched OFF."), RailNames[A])); }
		else Score(1, true);
		return true;
	}
	if (A == MCOrbit::SysReportLimits)
	{
		if (bLimitsReported) return true;
		bLimitsReported = true; Score(1, true);
		Say(FString::Printf(TEXT("Systems reports limits: vertical %.1f m/s, horizontal %.1f m/s."), VertLimit(), HorizLimit()));
		return true;
	}
	return false;
}

bool AMCOrbitManager::DoSafety(int32 A)
{
	const int32 NS = Sites.Num();
	if (A >= MCOrbit::SafeViewBase && A < MCOrbit::SafeViewBase + 4)
	{
		if (A - MCOrbit::SafeViewBase < NS) SafetyViewSite = A - MCOrbit::SafeViewBase;
		return true;
	}
	if (A >= MCOrbit::SafeMarkBase && A < MCOrbit::HazardsPerSite)
	{
		if (Stage < EMCOrbitStage::Insertion || Stage > EMCOrbitStage::Descent) return false;
		const int32 I = HazardIndex(SafetyViewSite, A);
		if (!Hazards.IsValidIndex(I)) return false;
		Hazards[I].bMarked = !Hazards[I].bMarked;
		if (Hazards[I].bMarked && !(MarkScored & (1 << I))) { MarkScored |= (1 << I); Score(2, HazardReal[I]); }
		return true;
	}
	if (A >= MCOrbit::SafeZoneBase && A < MCOrbit::SafeZoneBase + 4)
	{
		if (!bBoulderField) { Say(TEXT("Safety: no boulder field reported yet.")); return true; }
		const int32 Z = A - MCOrbit::SafeZoneBase;
		if (Z == SafeZone) { SafetyZoneClear = Z; Score(2, true); Say(FString::Printf(TEXT("Safety: zone %s is CLEAR."), ZoneNames[Z])); }
		else { Score(2, false); Say(FString::Printf(TEXT("Safety: zone %s still shows rocks."), ZoneNames[Z])); }
		return true;
	}
	if (A == MCOrbit::SafeConfirm || A == MCOrbit::SafeReject)
	{
		if (Stage != EMCOrbitStage::SiteLock || ProposedSite < 0 || bSiteConfirmed || bSiteRejected) return false;
		int32 Real = 0; for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i) Real += HazardReal[HazardIndex(ProposedSite, i)] ? 1 : 0;
		const bool bUnsafe = Real >= 4;
		if (A == MCOrbit::SafeConfirm) { bSiteConfirmed = true; Score(2, !bUnsafe); }
		else { bSiteRejected = true; Score(2, bUnsafe); }
		Say(A == MCOrbit::SafeConfirm ? TEXT("Safety confirms the proposed site.") : TEXT("Safety rejects the proposed site: backup site."));
		LockSite();
		return true;
	}
	return false;
}

bool AMCOrbitManager::DoPilot(int32 A)
{
	if (A == MCOrbit::PilInsertion)
	{
		if (Stage != EMCOrbitStage::Insertion || bInsertionDone) return false;
		bInsertionDone = true; FuelKg = FMath::Max(0.f, FuelKg - Cfg().InsertionFuelKg); Score(3, true);
		Say(TEXT("Orbit insertion burn complete. Survey begins."));
		SetStage(EMCOrbitStage::Survey, Cfg().SurveySec);
		return true;
	}
	if (A == MCOrbit::PilSteep || A == MCOrbit::PilShallow)
	{
		if (Stage >= EMCOrbitStage::Descent) return false;
		Approach = A == MCOrbit::PilSteep ? 1 : 2;
		Say(Approach == 1 ? TEXT("Pilot plans a STEEP approach: faster, harder to arrest.") : TEXT("Pilot plans a SHALLOW approach: slower, costs fuel."));
		return true;
	}
	if (Stage != EMCOrbitStage::Descent || bTouchedDown) return false;
	const UMCOrbitConfig& C = Cfg();
	switch (A)
	{
	case MCOrbit::PilThrUpBig: SetThrottle(Throttle + C.ThrottleStepBig); return true;
	case MCOrbit::PilThrDnBig: SetThrottle(Throttle - C.ThrottleStepBig); return true;
	case MCOrbit::PilThrUpSm: SetThrottle(Throttle + C.ThrottleStepSmall); return true;
	case MCOrbit::PilThrDnSm: SetThrottle(Throttle - C.ThrottleStepSmall); return true;
	case MCOrbit::PilHover: SetThrottle(C.MoonG / C.MaxAccel); return true;
	case MCOrbit::PilCut: SetThrottle(0.f); return true;
	case MCOrbit::PilWest: Nudge(-1.f, 0.f); return true;
	case MCOrbit::PilEast: Nudge(1.f, 0.f); return true;
	case MCOrbit::PilNorth: Nudge(0.f, 1.f); return true;
	case MCOrbit::PilSouth: Nudge(0.f, -1.f); return true;
	case MCOrbit::PilStop:
	{
		if (!RailOn(3) || FuelKg <= 0.f) { Say(TEXT("Thrusters unavailable.")); return true; }
		FuelKg = FMath::Max(0.f, FuelKg - C.StopFuelKgPerMps * FMath::Sqrt(VelX * VelX + VelY * VelY));
		VelX = VelY = 0.f; return true;
	}
	}
	return false;
}

void AMCOrbitManager::SetThrottle(float T) { Throttle = FuelKg > 0.f ? FMath::Clamp(T, 0.f, 1.f) : 0.f; }

void AMCOrbitManager::Nudge(float Dx, float Dy)
{
	if (!RailOn(3) || FuelKg <= 0.f) { Say(TEXT("Thrusters unavailable.")); return; }
	VelX += Dx * Cfg().NudgeMps; VelY += Dy * Cfg().NudgeMps;
	FuelKg = FMath::Max(0.f, FuelKg - Cfg().NudgeFuelKg);
}

bool AMCOrbitManager::DoAnalyst(int32 A)
{
	const int32 NS = Sites.Num();
	if (A >= MCOrbit::AnaRankBase && A < MCOrbit::AnaRankBase + 4)
	{
		if (ChosenSite >= 0 || A >= NS || Stage < EMCOrbitStage::Insertion || Stage > EMCOrbitStage::SiteLock || Sites[A].Rank > 0) return false;
		int32 Max = 0; for (const FMCOrbitSite& S : Sites) Max = FMath::Max(Max, S.Rank);
		Sites[A].Rank = Max + 1; return true;
	}
	if (A == MCOrbit::AnaReset) { for (FMCOrbitSite& S : Sites) S.Rank = 0; ProposedSite = -1; return true; }
	if (A == MCOrbit::AnaPropose)
	{
		if (ChosenSite >= 0 || Stage < EMCOrbitStage::Survey || Stage > EMCOrbitStage::SiteLock) return false;
		int32 Best = -1; for (int32 i = 0; i < NS; ++i) if (Sites[i].Rank == 1) Best = i;
		if (Best < 0) { Say(TEXT("Analyst: rank the sites first.")); return true; }
		ProposedSite = Best; Score(4, TrueRank[Best] <= 2);
		Say(FString::Printf(TEXT("Analyst proposes %s."), *Sites[Best].Name));
		if (Stage == EMCOrbitStage::Survey) SetStage(EMCOrbitStage::SiteLock, Cfg().SiteLockSec);
		return true;
	}
	if (A >= MCOrbit::AnaZoneBase && A < MCOrbit::AnaZoneBase + 4)
	{
		if (!bBoulderField || bEventResolved) return false;
		const int32 Z = A - MCOrbit::AnaZoneBase;
		if (SafetyZoneClear < 0) { Say(TEXT("Analyst: wait for Safety to clear a zone.")); return true; }
		if (Z != SafetyZoneClear) { Score(4, false); Say(TEXT("Analyst: that zone is not cleared by Safety.")); return true; }
		DivertZone = Z; bEventResolved = true; Score(4, true);
		const FVector2D D = ZoneDir(Z);
		TargetX = D.X * Cfg().ZoneOffsetM; TargetY = D.Y * Cfg().ZoneOffsetM;
		Say(FString::Printf(TEXT("New landing zone %s. Pilot: divert."), ZoneNames[Z]));
		return true;
	}
	return false;
}

void AMCOrbitManager::LockSite()
{
	if (ChosenSite >= 0) return;
	const int32 NS = Sites.Num();
	int32 Pick = ProposedSite;
	if (Pick < 0)
	{
		Pick = 0; for (int32 i = 1; i < NS; ++i) if (TrueRank[i] > TrueRank[Pick]) Pick = i;   // no proposal: worst default
		Say(TEXT("No proposal: default site assigned."));
	}
	else if (bSiteRejected)
	{
		int32 Backup = -1; for (int32 i = 0; i < NS; ++i) if (i != ProposedSite && Sites[i].Rank == 2) Backup = i;
		if (Backup < 0) { float Best = -1.f; for (int32 i = 0; i < NS; ++i) if (i != ProposedSite && Sites[i].DisplayIce >= Best) { Best = Sites[i].DisplayIce; Backup = i; } }
		Pick = Backup >= 0 ? Backup : Pick;
	}
	ChosenSite = Pick; SafetyViewSite = Pick;
	StageClock = 0.f;
}

void AMCOrbitManager::StartDescent()
{
	const UMCOrbitConfig& C = Cfg();
	for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i)   // keep only the chosen site's marks relevant (all stay replicated)
		(void)i;
	const float Ang = Stream.FRandRange(0.f, 2.f * PI);
	PosX = FMath::Cos(Ang) * C.StartOffsetM; PosY = FMath::Sin(Ang) * C.StartOffsetM; VelX = VelY = 0.f;
	AltM = C.StartAltM;
	VertSpeed = Approach == 1 ? C.SteepVz : Approach == 2 ? C.ShallowVz : C.StartVz;
	if (Approach == 2) FuelKg = FMath::Max(0.f, FuelKg - C.ShallowFuelKg);
	TargetX = TargetY = 0.f; SafeRadiusM = C.SafeRadiusM; BoulderRadiusM = C.BoulderRadiusM;
	Throttle = 0.f; DescentClock = 0.f;
	SetStage(EMCOrbitStage::Descent, C.DescentMaxSec);
	Say(FString::Printf(TEXT("Powered descent to %s. Pilot has control."), *Sites[ChosenSite].Name));
}

void AMCOrbitManager::DescentTick(float Dt)
{
	const UMCOrbitConfig& C = Cfg();
	if (bTouchedDown) return;
	DescentClock += Dt;
	if (!bEventFired && DescentClock >= EventTime)
	{
		bEventFired = true; bBoulderField = true;
		Say(TEXT("HAZARD ALERT: boulder field at the target. Safety: clear a zone. Analyst: divert."));
	}
	if (!bRailTripFired && DescentClock >= RailTripTime)
	{
		bRailTripFired = true;
		if (Rails[TrippedRail].bOn) { Rails[TrippedRail].bOn = false; Rails[TrippedRail].bTripped = true; Say(FString::Printf(TEXT("ALARM: %s rail tripped. Systems: restore it."), RailNames[TrippedRail])); }
	}
	bAltValid = RailOn(0);
	bLegsDeployed = RailOn(2) && AltM < 40.f;
	const float Frac = RailOn(1) ? 1.f : C.LowPressureThrustFrac;
	if (FuelKg <= 0.f) Throttle = 0.f;
	const float Acc = -C.MoonG + Throttle * C.MaxAccel * Frac;
	VertSpeed += Acc * Dt; AltM += VertSpeed * Dt;
	PosX += VelX * Dt; PosY += VelY * Dt;
	FuelKg = FMath::Max(0.f, FuelKg - Throttle * C.BurnKgPerSec * Dt);
	if (AltM <= 0.f) { AltM = 0.f; Touchdown(false); }
	else if (StageEndTime > 0.0 && GetStageTimeLeft() <= 0.f) { AltM = 0.f; Touchdown(true); }
}

void AMCOrbitManager::Touchdown(bool bTimeout)
{
	const UMCOrbitConfig& C = Cfg();
	bTouchedDown = true;
	const float Vert = FMath::Abs(VertSpeed), Horiz = FMath::Sqrt(VelX * VelX + VelY * VelY);
	const float Dist = FMath::Sqrt(FMath::Square(PosX - TargetX) + FMath::Square(PosY - TargetY));
	const float VL = VertLimit(), HL = HorizLimit();
	FMCOrbitReport& R = Result;
	R.bCrashed = Vert > VL * C.CrashFactor || Horiz > HL * C.CrashFactor || bTimeout;
	R.bLanded = !R.bCrashed;
	R.TouchdownVertMps = Vert; R.TouchdownHorizMps = Horiz; R.TargetDistM = Dist; R.FuelLeftKg = FuelKg;
	bool bHazardHit = false;
	for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i)
	{
		const int32 I = HazardIndex(ChosenSite, i);
		if (HazardReal[I] && FMath::Sqrt(FMath::Square(PosX - Hazards[I].X) + FMath::Square(PosY - Hazards[I].Y)) < Hazards[I].Radius + C.LegRadiusM) bHazardHit = true;
	}
	const bool bInBoulders = bBoulderField && FMath::Sqrt(PosX * PosX + PosY * PosY) < BoulderRadiusM;
	if (bHazardHit) Health = FMath::Max(0.f, Health - 35.f);
	if (bInBoulders) { Health = FMath::Max(0.f, Health - 50.f); R.bCrashed = true; R.bLanded = false; }
	if (R.bCrashed) Health = 0.f;
	else Health = FMath::Max(0.f, Health - FMath::Max(0.f, Vert - 1.f) * C.TouchdownHealthLossPerMps);
	bLegsDeployed = RailOn(2);
	Throttle = 0.f; VertSpeed = 0.f; VelX = VelY = 0.f;
	R.bEventHandled = !bBoulderField || (R.bLanded && !bInBoulders);
	R.bLegsOk = bLegsOkS2 && RailOn(2);
	DebugLog(FString::Printf(TEXT("TOUCHDOWN vert=%.2f horiz=%.2f dist=%.1f crashed=%d hazardHit=%d boulders=%d"), Vert, Horiz, Dist, R.bCrashed, bHazardHit, bInBoulders));
	if (!R.bCrashed)
	{
		const bool bOkLimits = Vert <= VL && Horiz <= HL && !bHazardHit;
		Score(3, Dist <= C.SafeRadiusM && bOkLimits);
	}
	else Score(3, false);
	R.MarksCorrect = 0; R.MarksWrong = 0; R.HazardsMissed = 0;
	for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i)
	{
		const int32 I = HazardIndex(ChosenSite, i);
		if (Hazards[I].bMarked) { (HazardReal[I] ? R.MarksCorrect : R.MarksWrong)++; }
		else if (HazardReal[I]) R.HazardsMissed++;
	}
	FinishReport();
}

void AMCOrbitManager::FinishReport()
{
	const UMCOrbitConfig& C = Cfg();
	FMCOrbitReport& R = Result;
	R.Seed = Seed; R.ChosenSite = ChosenSite; R.ChosenSiteName = Sites.IsValidIndex(ChosenSite) ? Sites[ChosenSite].Name : FString();
	R.IceLikelihood = IceTrue.IsValidIndex(ChosenSite) ? IceTrue[ChosenSite] : 0.f;
	R.Health = Health; R.FuelLeftKg = FuelKg; R.bLimitsReported = bLimitsReported;
	// ranking match
	const int32 N = Sites.Num();
	float Diff = 0.f, MaxDiff = 0.f;
	for (int32 i = 0; i < N; ++i) { const int32 Rk = Sites[i].Rank > 0 ? Sites[i].Rank : N; Diff += FMath::Abs(Rk - TrueRank[i]); MaxDiff += N - 1; }
	R.RankingMatch = MaxDiff > 0.f ? 1.f - Diff / MaxDiff : 1.f;
	const int32 RealAtSite = [&]() { int32 c = 0; for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i) c += HazardReal[HazardIndex(ChosenSite, i)] ? 1 : 0; return c; }();
	const float HazFrac = RealAtSite > 0 ? FMath::Clamp((float)(R.MarksCorrect - R.MarksWrong) / RealAtSite, 0.f, 1.f) : (R.MarksWrong == 0 ? 1.f : 0.5f);
	bool bRailsOk = true; for (const FMCOrbitRail& Rl : Rails) bRailsOk &= Rl.bOn;
	const float SysFrac = (bLimitsReported ? 0.5f : 0.f) + (bRailsOk ? 0.35f : 0.f) + (R.bEventHandled || !bBoulderField ? 0.15f : 0.f);
	float LandFrac = 0.f;
	if (R.bLanded) LandFrac = 0.4f + (R.TargetDistM <= C.SafeRadiusM ? 0.3f : 0.f) + (R.TouchdownVertMps <= VertLimit() && R.TouchdownHorizMps <= HorizLimit() ? 0.3f : 0.f);
	const float FH = 0.5f * (InitialFuel > 0.f ? FMath::Clamp(FuelKg / InitialFuel, 0.f, 1.f) : 0.f) + 0.5f * Health / 100.f;
	const float Sc = C.WeightRanking * R.RankingMatch + C.WeightHazards * HazFrac + C.WeightSystems * SysFrac + C.WeightLanding * LandFrac + C.WeightFuelHealth * FH;
	R.Score = FMath::Clamp(FMath::RoundToInt(Sc), 0, 100);
	OrbitScore = R.Score; R.RoleGood = RoleGood; R.RoleBad = RoleBad;
	FString T = TEXT("S3 ORBIT & LANDING REPORT\n");
	T += FString::Printf(TEXT("Seed %d  Site %s  (true ice %.0f%%)\n"), Seed, *R.ChosenSiteName, R.IceLikelihood * 100.f);
	T += FString::Printf(TEXT("Ranking match %.0f%%  (%.0f/%.0f)\n"), R.RankingMatch * 100.f, C.WeightRanking * R.RankingMatch, C.WeightRanking);
	T += FString::Printf(TEXT("Hazards: %d marked right, %d wrong, %d missed  (%.0f/%.0f)\n"), R.MarksCorrect, R.MarksWrong, R.HazardsMissed, C.WeightHazards * HazFrac, C.WeightHazards);
	T += FString::Printf(TEXT("Systems: limits %s, rails %s, event %s  (%.0f/%.0f)\n"), bLimitsReported ? TEXT("reported") : TEXT("NOT reported"), bRailsOk ? TEXT("ok") : TEXT("FAULT"), R.bEventHandled ? TEXT("handled") : TEXT("failed"), C.WeightSystems * SysFrac, C.WeightSystems);
	T += FString::Printf(TEXT("Landing: %s, vertical %.1f m/s, horizontal %.1f m/s, %.0f m from target  (%.0f/%.0f)\n"), R.bCrashed ? TEXT("CRASHED") : TEXT("LANDED"), R.TouchdownVertMps, R.TouchdownHorizMps, R.TargetDistM, C.WeightLanding * LandFrac, C.WeightLanding);
	T += FString::Printf(TEXT("Fuel %.0f kg, health %.0f%%  (%.0f/%.0f)\n"), FuelKg, Health, C.WeightFuelHealth * FH, C.WeightFuelHealth);
	T += FString::Printf(TEXT("TOTAL SCORE: %d / 100\nAccuracy (good/bad):\n"), R.Score);
	for (int32 r = 1; r <= 4; ++r)
	{
		const int32 G = RoleGood[r], B = RoleBad[r];
		T += FString::Printf(TEXT("  %s: %d / %d  (%d%%)\n"), RoleNames[r], G, B, (G + B) > 0 ? FMath::RoundToInt(100.f * G / (G + B)) : 100);
	}
	ReportText = T; R.RoleGood = RoleGood; R.RoleBad = RoleBad;
	SetStage(EMCOrbitStage::Report, C.ReportHoldSec);
	Say(FString::Printf(TEXT("%s. Score %d / 100."), R.bCrashed ? TEXT("Lander lost") : TEXT("Touchdown"), R.Score));
	UE_LOG(LogMissionCrew, Log, TEXT("%s"), *T);
	DebugLog(FString::Printf(TEXT("DONE score=%d landed=%d dist=%.1f site=%s"), R.Score, R.bLanded, R.TargetDistM, *R.ChosenSiteName));
	RebuildStrings();
}

// ------------------------------------------------------------------ tick

void AMCOrbitManager::Tick(float Dt)
{
	Super::Tick(Dt);
	// local HUD (every machine)
	if (bCreateHud && !Hud && GetNetMode() != NM_DedicatedServer)
	{
		HudTimer -= Dt;
		if (HudTimer <= 0.f) { HudTimer = 0.5f; EnsureHud(); }
	}
	if (!HasAuthority() || !bBegun) return;
	RunClock += Dt; StageClock += Dt; StringTimer -= Dt; RelayTimer -= Dt;
	if (RelayTimer <= 0.f) { RelayTimer = 1.f; EnsureRelays(); RoleApplyTick(); }
	if (bDone) return;
	ElapsedSec = RunClock;
	StageTick(Dt);
	if (bDebugBot) DebugBotTick(Dt);
	if (StringTimer <= 0.f) { StringTimer = 0.2f; RebuildStrings(); }
}

void AMCOrbitManager::EnsureHud()
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->IsLocalController()) return;
	TSubclassOf<UMCOrbitHudWidget> Cls = HudClass ? HudClass : TSubclassOf<UMCOrbitHudWidget>(UMCOrbitHudWidget::StaticClass());
	Hud = CreateWidget<UMCOrbitHudWidget>(PC, Cls);
	if (Hud) { Hud->AddToViewport(10); PC->SetShowMouseCursor(true); PC->SetInputMode(FInputModeGameAndUI()); }
}

void AMCOrbitManager::EnsureRelays()
{
	UWorld* W = GetWorld();
	for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || FindRelayFor(W, PC)) continue;
		FActorSpawnParameters P; P.Owner = PC; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		W->SpawnActor<AMCOrbitRelay>(AMCOrbitRelay::StaticClass(), FTransform::Identity, P);
	}
}

void AMCOrbitManager::RoleApplyTick()
{
	RoleApplyElapsed += 1.f;
	if (RoleApplyElapsed > RoleApplySec) return;
	if (UMCMissionSubsystem* M = UMCMissionSubsystem::Get(this)) M->ApplyRoles(GetWorld());
}

void AMCOrbitManager::StageTick(float Dt)
{
	const UMCOrbitConfig& C = Cfg();
	const bool bTimeUp = StageEndTime > 0.0 && GetStageTimeLeft() <= 0.f;
	switch (Stage)
	{
	case EMCOrbitStage::GoPoll:
	{
		bool bAll = true; for (int32 r = 1; r <= 4; ++r) bAll &= GoFlags[r];
		if (!bAll && StageClock > C.GoPollTimeoutSec)
		{
			for (int32 r = 1; r <= 4; ++r) if (!GoFlags[r]) { Score(r, false); GoFlags[r] = true; }
			bAll = true; Say(TEXT("GO poll timed out: open roles marked."));
		}
		if (bAll && RunClock >= PollHoldUntil)
		{
			Say(TEXT("All GO. Pilot: orbit insertion burn."));
			SetStage(EMCOrbitStage::Insertion, C.InsertionSec);
		}
		break;
	}
	case EMCOrbitStage::Insertion:
		if (bTimeUp)
		{
			bInsertionDone = true; FuelKg = FMath::Max(0.f, FuelKg - C.InsertionFuelKg * 1.5f); Score(3, false);
			Say(TEXT("Insertion burn was late (extra fuel). Survey begins."));
			SetStage(EMCOrbitStage::Survey, C.SurveySec);
		}
		break;
	case EMCOrbitStage::Survey:
		if (bTimeUp) SetStage(EMCOrbitStage::SiteLock, C.SiteLockSec);
		break;
	case EMCOrbitStage::SiteLock:
		if (ChosenSite >= 0) StartDescent();
		else if (bTimeUp) { LockSite(); }
		break;
	case EMCOrbitStage::Descent:
		DescentTick(Dt);
		break;
	case EMCOrbitStage::Report:
		if (bTimeUp && !bDone)
		{
			bDone = true;
			if (UMCMissionSubsystem* M = UMCMissionSubsystem::Get(this)) M->RecordLanding(Result);
			OnComplete.Broadcast(Result);
			if (bTravelOnComplete && (!bDebugBot || FParse::Param(FCommandLine::Get(), TEXT("MCTravelTest"))) && !NextMapName.IsEmpty() && !bTraveled)
			{
				bTraveled = true;
				FString URL = NextMapName;
				if (GetNetMode() == NM_ListenServer && !URL.Contains(TEXT("listen"))) URL += TEXT("?listen");
				UE_LOG(LogMissionCrew, Log, TEXT("Orbit: ServerTravel to %s"), *URL);
				GetWorld()->ServerTravel(URL);
			}
		}
		break;
	}
}

// ------------------------------------------------------------------ strings

FString AMCOrbitManager::RailListText() const
{
	FString S;
	for (const FMCOrbitRail& R : Rails) S += FString::Printf(TEXT("  [%s] %s%s\n"), R.bOn ? TEXT("ON ") : TEXT("OFF"), *R.Name, R.bTripped ? TEXT("  << TRIPPED") : TEXT(""));
	return S;
}

void AMCOrbitManager::RebuildStrings()
{
	const UMCOrbitConfig& C = Cfg();
	if (PanelText.Num() < MCOrbit::NumRoles) PanelText.Init(FString(), MCOrbit::NumRoles);
	const FString Shared = PanelText[0];
	TopText = FString::Printf(TEXT("%s | %s %.0f s | SEED %d | HEALTH %.0f | FUEL %.0f kg"), TEXT("ORBIT & LANDING"), StageName(Stage), GetStageTimeLeft(), Seed, Health, FuelKg);
	if (bDone || Stage == EMCOrbitStage::Report) BoardText = ReportText;
	else
	{
		BoardText = FString::Printf(TEXT("STAGE: %s\n"), StageName(Stage));
		for (int32 r = 1; r <= 4; ++r) BoardText += FString::Printf(TEXT("%s  GO %s\n"), RoleNames[r], GoFlags[r] ? TEXT("YES") : TEXT("-"));
		if (ChosenSite >= 0) BoardText += FString::Printf(TEXT("Landing site: %s\n"), *Sites[ChosenSite].Name);
	}
	const bool bDescent = Stage == EMCOrbitStage::Descent;
	// Systems
	FString P = FString::Printf(TEXT("Lander rails:\n%s"), *RailListText());
	P += FString::Printf(TEXT("Limits: vertical %.1f m/s, horizontal %.1f m/s%s\n"), VertLimit(), HorizLimit(), bLimitsReported ? TEXT("  (reported)") : TEXT("  (REPORT to the Pilot)"));
	if (Stage == EMCOrbitStage::GoPoll && IssueRole[1] && !IssueCleared[1]) P += TEXT("CHECK: radar rail is OFF!\n");
	PanelText[1] = P;
	// Safety
	P = FString::Printf(TEXT("Viewing %s. Candidates (X,Y m): mark real hazards only.\n"), Sites.IsValidIndex(SafetyViewSite) ? *Sites[SafetyViewSite].Name : TEXT("-"));
	if (Sites.IsValidIndex(SafetyViewSite))
		for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i)
		{
			const FMCOrbitHazard& H = Hazards[HazardIndex(SafetyViewSite, i)];
			static const TCHAR* TN[] = { TEXT("rock"), TEXT("steep slope"), TEXT("crater") };
			P += FString::Printf(TEXT("  #%d %s (%.0f,%.0f) r%.0f %s\n"), i + 1, TN[H.Type], H.X, H.Y, H.Radius, H.bMarked ? TEXT("MARKED") : TEXT(""));
		}
	if (bBoulderField)
	{
		P += TEXT("Zone scan (rocks per area):");
		for (int32 z = 0; z < 4; ++z) P += FString::Printf(TEXT("  %s %.0f"), ZoneNames[z], ZoneDensity[z]);
		P += FString::Printf(TEXT("\nCleared zone: %s\n"), SafetyZoneClear >= 0 ? ZoneNames[SafetyZoneClear] : TEXT("none"));
	}
	if (Stage == EMCOrbitStage::SiteLock && ProposedSite >= 0) P += FString::Printf(TEXT("PROPOSED: %s - confirm or reject.\n"), *Sites[ProposedSite].Name);
	PanelText[2] = P;
	// Pilot
	P.Empty();
	if (bDescent)
	{
		const float Dx = TargetX - PosX, Dy = TargetY - PosY, H = FMath::Sqrt(VelX * VelX + VelY * VelY);
		if (bAltValid) P += FString::Printf(TEXT("ALT %.0f m   V-SPEED %.1f m/s   H-SPEED %.1f m/s\n"), AltM, VertSpeed, H);
		else P += TEXT("ALT / SPEED: NO RADAR (Systems: restore the rail)\n");
		P += FString::Printf(TEXT("Target: %s%s\nThrottle %.0f%%  (hover %.0f%%)   Fuel %.0f kg\n"), *CompassOf(Dx, Dy), bBoulderField ? TEXT("  BOULDERS at site centre") : TEXT(""), Throttle * 100.f, C.MoonG / C.MaxAccel * 100.f, FuelKg);
	}
	else P += FString::Printf(TEXT("Approach plan: %s. %s\n"), Approach == 1 ? TEXT("STEEP") : Approach == 2 ? TEXT("SHALLOW") : TEXT("NOMINAL"), Stage == EMCOrbitStage::Insertion ? TEXT("Fire the insertion burn!") : TEXT(""));
	P += bLimitsReported ? FString::Printf(TEXT("Limits: %.1f m/s down, %.1f m/s sideways\n"), VertLimit(), HorizLimit()) : FString(TEXT("Limits unknown (ask Systems)\n"));
	if (Stage == EMCOrbitStage::GoPoll && IssueRole[3] && !IssueCleared[3]) P += TEXT("CHECK: descent plan not loaded!\n");
	PanelText[3] = P;
	// Analyst
	P = TEXT("Orbital ice layer (displayed likelihood):\n");
	for (int32 i = 0; i < Sites.Num(); ++i)
		P += FString::Printf(TEXT("  %d. %s  ice %s  rank %s\n"), i + 1, *Sites[i].Name, Sites[i].DisplayIce >= 0.f ? *FString::Printf(TEXT("%.0f%%"), Sites[i].DisplayIce * 100.f) : TEXT("no data"), Sites[i].Rank > 0 ? *FString::FromInt(Sites[i].Rank) : TEXT("-"));
	if (ProposedSite >= 0) P += FString::Printf(TEXT("Proposed: %s\n"), *Sites[ProposedSite].Name);
	if (bBoulderField) P += FString::Printf(TEXT("DIVERT: Safety cleared %s. Pick it.\n"), SafetyZoneClear >= 0 ? ZoneNames[SafetyZoneClear] : TEXT("(waiting)"));
	if (Stage == EMCOrbitStage::GoPoll && IssueRole[4] && !IssueCleared[4]) P += TEXT("CHECK: ice layer stale!\n");
	PanelText[4] = P;
	PanelText[0] = Shared;
}

// ------------------------------------------------------------------ debug bot (server truth, scripted)

void AMCOrbitManager::RunDebugTest()
{
	if (!HasAuthority()) return;
	bFreeRoles = true; bDebugBot = true; BotTimer = 0.5f; BotStep = 0;
	if (!bBegun) BeginOrbit();
	DebugLog(TEXT("START"));
}

void AMCOrbitManager::DebugBotTick(float Dt)
{
	BotTimer -= Dt;
	if (BotTimer > 0.f) return;
	BotTimer = 0.4f;
	auto Do = [this](int32 Code) { HandleAction(Code, 0); };
	switch (Stage)
	{
	case EMCOrbitStage::GoPoll:
		if (BotStep == 0) { Do(MCOrbit::NoGoCode); BotStep = 1; } else Do(MCOrbit::GoCode);
		break;
	case EMCOrbitStage::Insertion: Do(330); BotStep = 10; break;
	case EMCOrbitStage::Survey:
	{
		if (BotStep < 20)
		{
			Do(400 + 4);
			TArray<int32> O; for (int32 i = 0; i < Sites.Num(); ++i) O.Add(i);
			O.Sort([this](int32 A, int32 B) { return Sites[A].DisplayIce > Sites[B].DisplayIce; });
			for (int32 i : O) Do(400 + i);
			// mark real hazards on the best-ice site
			Do(220 + O[0]);
			for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i) if (HazardReal[HazardIndex(O[0], i)] && !Hazards[HazardIndex(O[0], i)].bMarked) Do(200 + i);
			Do(104); Do(405); BotStep = 20;
		}
		break;
	}
	case EMCOrbitStage::SiteLock:
		if (ChosenSite < 0 && BotStep < 30)
		{
			int32 Real = 0; for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i) Real += HazardReal[HazardIndex(ProposedSite, i)] ? 1 : 0;
			Do(Real >= 4 ? 231 : 230); BotStep = 30;
			if (ChosenSite >= 0)
			{
				Do(220 + ChosenSite);
				for (int32 i = 0; i < MCOrbit::HazardsPerSite; ++i) if (HazardReal[HazardIndex(ChosenSite, i)] && !Hazards[HazardIndex(ChosenSite, i)].bMarked) Do(200 + i);
			}
		}
		break;
	case EMCOrbitStage::Descent:
	{
		const UMCOrbitConfig& C = Cfg();
		if (bBoulderField && SafetyZoneClear < 0) Do(210 + SafeZone);
		if (bBoulderField && SafetyZoneClear >= 0 && !bEventResolved) Do(410 + SafeZone);
		if (TrippedRail >= 0 && Rails[TrippedRail].bTripped) Do(100 + TrippedRail);
		const float VzT = -FMath::Max(1.0f, AltM * 0.08f);
		SetThrottle((C.MoonG + (VzT - VertSpeed) * 1.2f) / C.MaxAccel / (RailOn(1) ? 1.f : C.LowPressureThrustFrac));
		const float Dx = TargetX - PosX, Dy = TargetY - PosY;
		const float WantX = FMath::Clamp(Dx * 0.05f, -3.f, 3.f), WantY = FMath::Clamp(Dy * 0.05f, -3.f, 3.f);
		const float EX = WantX - VelX, EY = WantY - VelY;
		if (FMath::Abs(EX) > 0.8f && FMath::Abs(EX) >= FMath::Abs(EY)) Do(EX > 0 ? 305 : 304);
		else if (FMath::Abs(EY) > 0.8f) Do(EY > 0 ? 306 : 307);
		BotTimer = 0.1f;
		break;
	}
	default: break;
	}
}
