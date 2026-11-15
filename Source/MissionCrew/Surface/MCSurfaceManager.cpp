#include "Surface/MCSurfaceManager.h"
#include "Surface/MCSurfaceRover.h"
#include "Surface/MCSurfaceLink.h"
#include "Transit/MCTransitManager.h"
#include "Core/MCMissionSubsystem.h"
#include "MissionCrew.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AMCSurfaceManager::AMCSurfaceManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(10.f);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AMCSurfaceManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCSurfaceManager, bFreeRoles);
	DOREPLIFETIME(AMCSurfaceManager, bStarted);
	DOREPLIFETIME(AMCSurfaceManager, bDone);
	DOREPLIFETIME(AMCSurfaceManager, Seed);
	DOREPLIFETIME(AMCSurfaceManager, ElapsedSec);
	DOREPLIFETIME(AMCSurfaceManager, TimeoutSec);
	DOREPLIFETIME(AMCSurfaceManager, Message);
	DOREPLIFETIME(AMCSurfaceManager, Score);
	DOREPLIFETIME(AMCSurfaceManager, Sites);
	DOREPLIFETIME(AMCSurfaceManager, RockPos);
	DOREPLIFETIME(AMCSurfaceManager, RockRadius);
	DOREPLIFETIME(AMCSurfaceManager, FieldHalfSize);
	DOREPLIFETIME(AMCSurfaceManager, SiteRadius);
	DOREPLIFETIME(AMCSurfaceManager, ChosenSite);
	DOREPLIFETIME(AMCSurfaceManager, Rover);
	DOREPLIFETIME(AMCSurfaceManager, RoverPos);
	DOREPLIFETIME(AMCSurfaceManager, RoverYaw);
	DOREPLIFETIME(AMCSurfaceManager, RockHits);
	DOREPLIFETIME(AMCSurfaceManager, RoverSpeedNow);
	DOREPLIFETIME(AMCSurfaceManager, RoverAtSite);
	DOREPLIFETIME(AMCSurfaceManager, BatteryWh);
	DOREPLIFETIME(AMCSurfaceManager, CapacityWh);
	DOREPLIFETIME(AMCSurfaceManager, SolarNowW);
	DOREPLIFETIME(AMCSurfaceManager, SupplyW);
	DOREPLIFETIME(AMCSurfaceManager, DemandW);
	DOREPLIFETIME(AMCSurfaceManager, bCommsOn);
	DOREPLIFETIME(AMCSurfaceManager, bInstrumentsOn);
	DOREPLIFETIME(AMCSurfaceManager, DrillPowerLevel);
	DOREPLIFETIME(AMCSurfaceManager, bBrownout);
	DOREPLIFETIME(AMCSurfaceManager, bPowerEvent);
	DOREPLIFETIME(AMCSurfaceManager, EventLeftSec);
	DOREPLIFETIME(AMCSurfaceManager, PowerUsedWh);
	DOREPLIFETIME(AMCSurfaceManager, DepthLevel);
	DOREPLIFETIME(AMCSurfaceManager, bDrilling);
	DOREPLIFETIME(AMCSurfaceManager, bDrillPaused);
	DOREPLIFETIME(AMCSurfaceManager, DrillProgress);
	DOREPLIFETIME(AMCSurfaceManager, bSampleReady);
	DOREPLIFETIME(AMCSurfaceManager, Samples);
	DOREPLIFETIME(AMCSurfaceManager, Result);
	DOREPLIFETIME(AMCSurfaceManager, RoleGood);
	DOREPLIFETIME(AMCSurfaceManager, RoleBad);
}

const UMCSurfaceConfig& AMCSurfaceManager::Cfg() const
{
	if (Config) { return *Config; }
	return *GetDefault<UMCSurfaceConfig>();
}

FVector AMCSurfaceManager::ToWorld(const FVector2D& P) const
{
	return GetActorLocation() + FVector(P.X, P.Y, 0.0);
}

FVector AMCSurfaceManager::SiteWorldLocation(int32 Index) const
{
	return Sites.IsValidIndex(Index) ? ToWorld(Sites[Index].Pos) : GetActorLocation();
}

void AMCSurfaceManager::DebugLog(const FString& Text) const
{
	UE_LOG(LogMissionCrew, Log, TEXT("DEBUGTEST Surface: %s"), *Text);
}

void AMCSurfaceManager::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) { return; }
	if (bDebugAutoTest) { bFreeRoles = true; }
	if (bAutoStart) { BeginSurface(); }
	if (bDebugAutoTest && !bDebugBot) { RunDebugTest(); }
	if (bApplyRoles && GetWorld())
	{
		RoleElapsed = 0.f;
		GetWorldTimerManager().SetTimer(RoleTimer, this, &AMCSurfaceManager::ApplyRolesTick, 1.f, true, 1.f);
	}
}

void AMCSurfaceManager::ApplyRolesTick()
{
	RoleElapsed += 1.f;
	UWorld* W = GetWorld();
	if (!W) { return; }
	if (UMCMissionSubsystem* Mission = UMCMissionSubsystem::Get(this)) { Mission->ApplyRoles(W); }
	if (RoleElapsed >= RoleRetrySec) { GetWorldTimerManager().ClearTimer(RoleTimer); }
}

void AMCSurfaceManager::Say(const FString& Text)
{
	Message = Text;
	UE_LOG(LogMissionCrew, Log, TEXT("Surface: %s"), *Text);
}

void AMCSurfaceManager::AddScore(int32 RoleIdx, bool bGood)
{
	if (!RoleGood.IsValidIndex(RoleIdx)) { return; }
	(bGood ? RoleGood : RoleBad)[RoleIdx]++;
}

void AMCSurfaceManager::Recalc()
{
	int32 Ice = 0, Chosen = 0, Other = 0;
	for (const FMCSurfSample& S : Samples)
	{
		if (S.Type == EMCSurfSampleType::Ice && S.bAtChosenSite) { ++Ice; }
		else if (S.Type == EMCSurfSampleType::Ice) { ++Other; }
		else { ++Chosen; }
	}
	Score = FMath::Max(0, Ice * 500 + Other * 250 + Chosen * 50 + (bEventHandled ? 150 : 0) - RockHits * 25 - FMath::RoundToInt(BrownoutSec * 2.f));
}

// ------------------------------------------------------------------ start

void AMCSurfaceManager::BeginSurface()
{
	if (!HasAuthority() || bStarted) { return; }
	const UMCSurfaceConfig& C = Cfg();
	Seed = SeedOverride != 0 ? SeedOverride : FMath::RandRange(1000, 99999);
	Stream.Initialize(Seed);
	TimeoutSec = C.PhaseTimeoutSec;
	RoleGood.Init(0, 5);
	RoleBad.Init(0, 5);
	InitField();

	CapacityWh = C.BatteryCapacityWh;
	BatteryWh = CapacityWh;
	EventAt = Stream.FRandRange(C.EventMinSec, C.EventMaxSec);

	if (UWorld* W = GetWorld())
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AMCSurfaceRover* R = W->SpawnActorDeferred<AMCSurfaceRover>(AMCSurfaceRover::StaticClass(), FTransform(ToWorld(RoverPos)), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (R)
		{
			R->MeshAsset = C.RoverMesh;
			R->MeshScale = C.RoverMeshScale;
			R->YawOffset = C.RoverYawOffset;
			R->FinishSpawning(FTransform(FRotator(0.f, RoverYaw, 0.f), ToWorld(RoverPos)));
			Rover = R;
		}
	}
	bStarted = true;
	Say(TEXT("Surface operations started. Analyst: pick a site. Driver: drive there. Drill: set depth."));
	UE_LOG(LogMissionCrew, Log, TEXT("Surface: begin seed=%d sites=%d rocks=%d eventAt=%.1f"), Seed, Sites.Num(), RockPos.Num(), EventAt);
}

void AMCSurfaceManager::InitField()
{
	const UMCSurfaceConfig& C = Cfg();
	FieldHalfSize = C.FieldHalfSizeCm;
	SiteRadius = C.SiteRadiusCm;
	Sites.Reset(); IceFrac.Reset(); IceDepth.Reset(); NoiseA.Reset(); NoiseB.Reset();
	RockPos.Reset(); RockRadius.Reset();
	RoverPos = FVector2D::ZeroVector;
	RoverYaw = 0.f;

	const int32 N = FMath::Clamp(C.SiteCount, 1, MCSurface::MaxSites);
	const int32 Best = Stream.RandRange(0, N - 1);
	const float Base = Stream.FRandRange(0.f, 360.f);
	for (int32 i = 0; i < N; ++i)
	{
		const float Ang = FMath::DegreesToRadians(Base + i * (360.f / N) + Stream.FRandRange(-20.f, 20.f));
		const float Dist = Stream.FRandRange(C.SiteMinDistCm, C.SiteMaxDistCm);
		FMCSurfSite S;
		S.Pos = FVector2D(FMath::Cos(Ang), FMath::Sin(Ang)) * Dist;
		// at least one site is clearly ice-likely (the Best one), another may be decoy-ish
		const float Frac = (i == Best) ? Stream.FRandRange(0.7f, 0.95f) : Stream.FRandRange(0.05f, FMath::Max(0.06f, C.IceLikelyThreshold - 0.1f));
		IceFrac.Add(Frac);
		IceDepth.Add(Stream.RandRange(1, 3));
		NoiseA.Add(Stream.FRandRange(-1.f, 1.f));
		NoiseB.Add(Stream.FRandRange(-1.f, 1.f));
		Sites.Add(S);
	}
	for (int32 i = 0; i < N; ++i) { RefreshReadings(i); }

	int32 Guard = 0;
	while (RockPos.Num() < C.RockCount && Guard++ < 500)
	{
		const FVector2D P(Stream.FRandRange(-FieldHalfSize, FieldHalfSize), Stream.FRandRange(-FieldHalfSize, FieldHalfSize));
		const float R = Stream.FRandRange(120.f, 230.f);
		bool bOk = P.Size() > 700.f;
		for (const FMCSurfSite& S : Sites) { if (FVector2D::Distance(S.Pos, P) < SiteRadius + R + 100.f) { bOk = false; } }
		for (int32 k = 0; k < RockPos.Num(); ++k) { if (FVector2D::Distance(RockPos[k], P) < RockRadius[k] + R + 150.f) { bOk = false; } }
		if (bOk) { RockPos.Add(P); RockRadius.Add(R); }
	}
}

void AMCSurfaceManager::RefreshReadings(int32 I)
{
	if (!Sites.IsValidIndex(I)) { return; }
	const UMCSurfaceConfig& C = Cfg();
	FMCSurfSite& S = Sites[I];
	const float Noise = C.ReadingNoise / (1.f + S.Scans * 1.5f);
	S.Hydrogen = FMath::Clamp(IceFrac[I] * 100.f + NoiseA[I] * Noise, 0.f, 100.f);
	S.TempK = FMath::Max(40.f, 260.f - IceFrac[I] * 160.f + NoiseB[I] * Noise * 0.8f);
}

// ------------------------------------------------------------------ actions

bool AMCSurfaceManager::HandleDrive(float InThrottle, float InSteer, int32 Who)
{
	if (!HasAuthority() || !bStarted || bDone) { return false; }
	if (!bFreeRoles && Who != (int32)EMCSurfRole::Driver) { return false; }
	Throttle = FMath::Clamp(InThrottle, -1.f, 1.f);
	Steer = FMath::Clamp(InSteer, -1.f, 1.f);
	return true;
}

bool AMCSurfaceManager::HandleAction(int32 Code, int32 Who)
{
	if (!HasAuthority() || !bStarted || bDone) { return false; }
	const int32 RoleIdx = Code / 100;
	const int32 Act = Code % 100;
	if (RoleIdx < 1 || RoleIdx > 4) { return false; }
	if (!bFreeRoles && Who != RoleIdx) { return false; }
	const UMCSurfaceConfig& C = Cfg();

	switch (RoleIdx)
	{
	case 1:
		if (Act == (int32)EMCSurfPowerAction::ToggleComms)
		{
			bCommsOn = !bCommsOn;
			if (!bCommsOn && bPowerEvent) { bEventHandled = true; AddScore(1, true); Say(TEXT("Non-essential loads cut: comms and lights off.")); }
			else { Say(bCommsOn ? TEXT("Comms and lights on.") : TEXT("Comms and lights off.")); }
		}
		else if (Act == (int32)EMCSurfPowerAction::ToggleInstruments)
		{
			bInstrumentsOn = !bInstrumentsOn;
			if (!bInstrumentsOn && bPowerEvent) { bEventHandled = true; AddScore(1, true); Say(TEXT("Non-essential loads cut: instruments off.")); }
			else { Say(bInstrumentsOn ? TEXT("Instruments on.") : TEXT("Instruments off.")); }
		}
		else if (Act == (int32)EMCSurfPowerAction::DrillPowerUp) { DrillPowerLevel = FMath::Min(3, DrillPowerLevel + 1); }
		else if (Act == (int32)EMCSurfPowerAction::DrillPowerDown) { DrillPowerLevel = FMath::Max(1, DrillPowerLevel - 1); }
		else { return false; }
		break;
	case 2:
		if (Act >= 1 && Act <= MCSurface::MaxSites)
		{
			const int32 Idx = Act - 1;
			if (!Sites.IsValidIndex(Idx)) { return false; }
			ChosenSite = Idx;
			const bool bGood = IceFrac[Idx] >= C.IceLikelyThreshold;
			AddScore(2, bGood);
			Say(FString::Printf(TEXT("Science rated site %c as ice-likely. Rover: drive there."), (TCHAR)(TEXT('A') + Idx)));
		}
		else if (Act == (int32)EMCSurfScienceAction::Scan)
		{
			if (ChosenSite < 0) { Say(TEXT("Pick a site first.")); return true; }
			if (!bInstrumentsOn || bBrownout) { Say(TEXT("Scan refused: instruments have no power.")); AddScore(2, false); return true; }
			if (BatteryWh < C.ScanEnergyWh) { Say(TEXT("Scan refused: battery too low.")); return true; }
			BatteryWh -= C.ScanEnergyWh;
			PowerUsedWh += C.ScanEnergyWh;
			Sites[ChosenSite].Scans++;
			RefreshReadings(ChosenSite);
			Say(FString::Printf(TEXT("Rescan of site %c done. Readings refined."), (TCHAR)(TEXT('A') + ChosenSite)));
		}
		else { return false; }
		break;
	case 3:
		if (Act == (int32)EMCSurfDriverAction::Stop) { Throttle = 0.f; Steer = 0.f; }
		else { return false; }
		break;
	case 4:
		if (Act >= 1 && Act <= 3)
		{
			if (bDrilling) { Say(TEXT("Cannot change depth while drilling.")); return true; }
			DepthLevel = Act;
		}
		else if (Act == (int32)EMCSurfDrillAction::Start)
		{
			if (bDrilling || bSampleReady) { return true; }
			if (Samples.Num() >= C.MaxSamples) { Say(TEXT("Sample bay full.")); return true; }
			if (RoverAtSite < 0) { Say(TEXT("Drill refused: the rover is not inside a site.")); AddScore(4, false); return true; }
			if (FMath::Abs(Throttle) > 0.01f) { Say(TEXT("Drill refused: the rover is moving.")); return true; }
			DrillSite = RoverAtSite;
			DrillProgress = 0.f;
			bDrilling = true;
			Say(FString::Printf(TEXT("Drilling at site %c, depth level %d."), (TCHAR)(TEXT('A') + DrillSite), DepthLevel));
		}
		else if (Act == (int32)EMCSurfDrillAction::Extract)
		{
			if (!bSampleReady || !Sites.IsValidIndex(DrillSite)) { Say(TEXT("Nothing to extract yet.")); return true; }
			FMCSurfSample S;
			S.Site = DrillSite;
			S.DepthLevel = DepthLevel;
			S.bAtChosenSite = (DrillSite == ChosenSite);
			const bool bIce = IceFrac[DrillSite] >= C.IceLikelyThreshold && DepthLevel >= IceDepth[DrillSite];
			S.Type = bIce ? EMCSurfSampleType::Ice : EMCSurfSampleType::Regolith;
			Samples.Add(S);
			bSampleReady = false;
			AddScore(4, bIce && S.bAtChosenSite);
			if (!bIce) { AddScore(4, false); }
			Say(FString::Printf(TEXT("Sample %d extracted: %s from site %c at level %d."), Samples.Num(), bIce ? TEXT("ICE") : TEXT("dry regolith"), (TCHAR)(TEXT('A') + DrillSite), DepthLevel));
			if (bIce && S.bAtChosenSite && FinishClock < 0.f) { FinishClock = 0.f; }
		}
		else { return false; }
		break;
	}
	Recalc();
	return true;
}

// ------------------------------------------------------------------ simulation

void AMCSurfaceManager::PowerTick(float Dt)
{
	const UMCSurfaceConfig& C = Cfg();

	// Power drop event
	if (!bEventDone && ElapsedSec >= EventAt)
	{
		bEventDone = true;
		bPowerEvent = true;
		EventClock = 0.f;
		CapacityWh = C.BatteryCapacityWh * (1.f - C.EventCapacityLoss);
		BatteryWh = FMath::Min(BatteryWh, CapacityWh);
		Say(TEXT("POWER DROP: battery capacity lost, solar output down. Power Operator: cut non-essential loads."));
	}
	if (bPowerEvent)
	{
		EventClock += Dt;
		EventLeftSec = FMath::Max(0.f, C.EventDurationSec - EventClock);
		if (EventClock >= C.EventDurationSec) { bPowerEvent = false; EventLeftSec = 0.f; Say(TEXT("Power drop over. Solar output recovering; battery capacity stays reduced.")); }
	}

	SolarNowW = C.SolarW * (bPowerEvent ? C.EventSolarFactor : 1.f);
	SupplyW = SolarNowW + (BatteryWh > 0.01f ? C.BatteryMaxDischargeW : 0.f);
	const bool bMoving = FMath::Abs(Throttle) > 0.01f;
	const float NonDrill = C.EssentialW + (bCommsOn ? C.CommsW : 0.f) + (bInstrumentsOn ? C.InstrumentsW : 0.f) + (bMoving ? C.RoverW : 0.f);
	const float DrillW = C.DrillWPerLevel * DrillPowerLevel;
	bBrownout = NonDrill > SupplyW;
	const bool bDrillRuns = bDrilling && !bBrownout && (NonDrill + DrillW <= SupplyW);
	bDrillPaused = bDrilling && !bDrillRuns;
	DemandW = NonDrill + (bDrillRuns ? DrillW : 0.f);

	const float DeltaWh = (SolarNowW - DemandW) * Dt * C.PowerTimeScale / 3600.f;
	BatteryWh = FMath::Clamp(BatteryWh + DeltaWh, 0.f, CapacityWh);
	PowerUsedWh += DemandW * Dt * C.PowerTimeScale / 3600.f;

	if (bBrownout) { BrownoutSec += Dt; }
	if (bBrownout && !bWasBrownout) { AddScore(1, false); Say(TEXT("BROWNOUT: demand exceeds supply. Cut loads.")); }
	bWasBrownout = bBrownout;
	// Drill progress is advanced in DrillTick using bDrilling && !bDrillPaused.
}

void AMCSurfaceManager::RoverTick(float Dt)
{
	const UMCSurfaceConfig& C = Cfg();
	HitCooldown = FMath::Max(0.f, HitCooldown - Dt);
	const float PowerFactor = bBrownout ? 0.4f : 1.f;
	if (bDrilling && FMath::Abs(Throttle) > 0.01f)
	{
		bDrilling = false; DrillProgress = 0.f;
		Say(TEXT("Drilling aborted: the rover moved."));
		AddScore(3, false);
	}
	RoverYaw = FMath::UnwindDegrees(RoverYaw + Steer * C.RoverTurnDegS * Dt * (FMath::Abs(Throttle) > 0.01f ? 1.f : 0.6f));
	const float Fwd = Throttle * C.RoverSpeedCmS * PowerFactor;
	RoverSpeedNow = Fwd;
	if (!FMath::IsNearlyZero(Fwd))
	{
		const FVector2D Dir(FMath::Cos(FMath::DegreesToRadians(RoverYaw)), FMath::Sin(FMath::DegreesToRadians(RoverYaw)));
		FVector2D NewPos = RoverPos + Dir * Fwd * Dt;
		NewPos.X = FMath::Clamp(NewPos.X, -FieldHalfSize, FieldHalfSize);
		NewPos.Y = FMath::Clamp(NewPos.Y, -FieldHalfSize, FieldHalfSize);
		bool bHit = false;
		for (int32 i = 0; i < RockPos.Num(); ++i)
		{
			if (FVector2D::Distance(NewPos, RockPos[i]) < RockRadius[i] + C.RoverRadiusCm)
			{
				bHit = true;
				break;
			}
		}
		if (bHit)
		{
			// bounce back a little, stop, count once per cooldown
			if (HitCooldown <= 0.f)
			{
				++RockHits; HitCooldown = 1.5f;
				AddScore(3, false);
				Say(TEXT("Rover hit a rock. Steer around it."));
				Recalc();
			}
			RoverPos -= Dir * 60.f * FMath::Sign(Fwd);
			Throttle = 0.f;
			RoverSpeedNow = 0.f;
		}
		else { RoverPos = NewPos; }
	}
	RoverAtSite = -1;
	for (int32 i = 0; i < Sites.Num(); ++i)
	{
		if (FVector2D::Distance(RoverPos, Sites[i].Pos) <= SiteRadius) { RoverAtSite = i; break; }
	}
	if (Rover)
	{
		Rover->SetActorLocationAndRotation(ToWorld(RoverPos), FRotator(0.f, RoverYaw, 0.f));
	}
}

void AMCSurfaceManager::DrillTick(float Dt)
{
	if (!bDrilling || bDrillPaused) { return; }
	const UMCSurfaceConfig& C = Cfg();
	const float Need = FMath::Max(0.1f, C.SecPerLevel * DepthLevel);
	DrillProgress = FMath::Min(1.f, DrillProgress + Dt / Need * (DrillPowerLevel / 2.f));
	if (DrillProgress >= 1.f)
	{
		bDrilling = false;
		bSampleReady = true;
		Say(TEXT("Drill reached depth. Drill Operator: extract the sample."));
	}
}

void AMCSurfaceManager::EnsureLinks()
{
	UWorld* W = GetWorld();
	if (!W) { return; }
	for (auto It = Links.CreateIterator(); It; ++It)
	{
		if (!It.Key() || !It.Value()) { if (It.Value()) { It.Value()->Destroy(); } It.RemoveCurrent(); }
	}
	for (FConstPlayerControllerIterator It = W->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || Links.Contains(PC)) { continue; }
		FActorSpawnParameters P;
		P.Owner = PC;
		if (AMCSurfaceLink* L = W->SpawnActor<AMCSurfaceLink>(AMCSurfaceLink::StaticClass(), GetActorTransform(), P))
		{
			Links.Add(PC, L);
		}
	}
}

void AMCSurfaceManager::Tick(float Dt)
{
	Super::Tick(Dt);
	if (bDrawMarkers && bStarted) { DrawMarkers(); }
	if (!HasAuthority() || !bStarted) { return; }

	LinkTimer -= Dt;
	if (LinkTimer <= 0.f) { LinkTimer = 1.f; EnsureLinks(); }
	if (bDone) { return; }

	ElapsedSec += Dt;
	if (bDebugBot) { DebugBotTick(Dt); }
	PowerTick(Dt);
	RoverTick(Dt);
	DrillTick(Dt);

	if (FinishClock >= 0.f)
	{
		FinishClock += Dt;
		if (FinishClock >= Cfg().CompleteDelaySec) { Finish(false); return; }
	}
	if (ElapsedSec >= TimeoutSec) { Finish(true); }
}

void AMCSurfaceManager::Finish(bool bTimeout)
{
	if (bDone) { return; }
	bDone = true;
	Throttle = Steer = 0.f;
	Recalc();
	FMCSurfReport R;
	R.Samples = Samples.Num();
	for (const FMCSurfSample& S : Samples) { if (S.Type == EMCSurfSampleType::Ice) { ++R.IceSamples; } }
	bool bGoal = false;
	for (const FMCSurfSample& S : Samples) { if (S.Type == EMCSurfSampleType::Ice && S.bAtChosenSite) { bGoal = true; } }
	R.bSuccess = bGoal;
	R.PowerUsedWh = PowerUsedWh;
	R.RockHits = RockHits;
	Score = FMath::Clamp(FMath::RoundToInt(Score * 100.f / 500.f), 0, 100); // normalise to 0..100 (500 = one ice sample at the rated site)
	R.Score = Score;
	R.TimeSec = ElapsedSec;
	Result = R;
	Say(bGoal ? FString::Printf(TEXT("SURFACE COMPLETE: %d sample(s), %d ice. Score %d."), R.Samples, R.IceSamples, Score)
	          : FString::Printf(TEXT("Surface phase ended (%s) without an ice sample from the rated site. Score %d."), bTimeout ? TEXT("time out") : TEXT("done"), Score));

	if (UMCMissionSubsystem* M = UMCMissionSubsystem::Get(this))
	{
		M->bS4Done = true;
		M->bS4Success = bGoal;
		M->S4Samples = R.Samples;
		M->S4IceSamples = R.IceSamples;
		M->S4SampleTypes.Reset();
		for (const FMCSurfSample& S : Samples) { M->S4SampleTypes.Add(S.Type == EMCSurfSampleType::Ice ? TEXT("ICE") : TEXT("REGOLITH")); }
		M->S4PowerUsedWh = PowerUsedWh;
		M->S4Score = Score;
		M->SetStageResult(4, Score, ElapsedSec, bGoal ? TEXT("Power is shared: a drill that runs out of battery stops, so cut non-essential loads first.") : TEXT("Pick the site the readings support before you drive."));
	}
	OnComplete.Broadcast(R);
	DebugLog(FString::Printf(TEXT("FINISH success=%d samples=%d ice=%d hits=%d powerWh=%.1f score=%d t=%.1f"), bGoal, R.Samples, R.IceSamples, RockHits, PowerUsedWh, Score, ElapsedSec));
	if (bTravelOnComplete && !NextMap.IsEmpty() && GetWorld())
	{
		GetWorldTimerManager().SetTimer(TravelTimer, this, &AMCSurfaceManager::DoTravel, 3.f, false);
	}
}

void AMCSurfaceManager::DoTravel()
{
	UWorld* W = GetWorld();
	if (!HasAuthority() || !W) { return; }
	const FString Current = UWorld::RemovePIEPrefix(W->GetOutermost()->GetName());
	if (Current == NextMap || FPackageName::GetShortName(Current) == FPackageName::GetShortName(NextMap)) { return; }
	FString URL = NextMap;
	if (W->GetNetMode() == NM_ListenServer && !URL.Contains(TEXT("listen"))) { URL += TEXT("?listen"); }
	UE_LOG(LogMissionCrew, Log, TEXT("Surface: ServerTravel to %s"), *URL);
	W->ServerTravel(URL);
}

void AMCSurfaceManager::DrawMarkers() const
{
#if ENABLE_DRAW_DEBUG
	UWorld* W = GetWorld();
	if (!W) { return; }
	const FVector X(1, 0, 0), Y(0, 1, 0);
	for (int32 i = 0; i < RockPos.Num() && i < RockRadius.Num(); ++i)
	{
		const FVector P = ToWorld(RockPos[i]) + FVector(0, 0, RockRadius[i] * 0.5f);
		DrawDebugSphere(W, P, RockRadius[i], 8, FColor(150, 90, 60), false, -1.f, 0, 3.f);
	}
	for (int32 i = 0; i < Sites.Num(); ++i)
	{
		const FColor Col = (i == ChosenSite) ? FColor::Green : FColor::Cyan;
		const FVector P = ToWorld(Sites[i].Pos) + FVector(0, 0, 10);
		DrawDebugCircle(W, P, SiteRadius, 48, Col, false, -1.f, 0, 8.f, X, Y, false);
		DrawDebugString(W, P + FVector(0, 0, 200), FString::Printf(TEXT("SITE %c"), (TCHAR)(TEXT('A') + i)), nullptr, Col, 0.f, false, 2.f);
	}
#endif
}

// ------------------------------------------------------------------ debug bot

void AMCSurfaceManager::RunDebugTest()
{
	if (!HasAuthority()) { return; }
	bFreeRoles = true;
	bDebugBot = true;
	BotClock = 0.f; BotStep = 0; BotWait = 0.f;
	if (!bStarted) { BeginSurface(); }
	DebugLog(FString::Printf(TEXT("SEEDCHECK seed=%d eventAt=%.2f sites=%d rocks=%d"), Seed, EventAt, Sites.Num(), RockPos.Num()));
	for (int32 i = 0; i < Sites.Num(); ++i)
	{
		DebugLog(FString::Printf(TEXT("SEEDCHECK site%d pos=(%.0f,%.0f) ice=%.3f depth=%d"), i, Sites[i].Pos.X, Sites[i].Pos.Y, IceFrac[i], IceDepth[i]));
	}
}

void AMCSurfaceManager::DebugBotTick(float Dt)
{
	BotClock += Dt;
	BotWait -= Dt;
	const auto Press = [this](EMCSurfRole R, int32 A) { HandleAction(MCSurface::MakeCode(R, A), 0); };

	// power handling runs in parallel with everything else
	if ((bPowerEvent || bBrownout || bDrillPaused) && BotWait <= 0.f && (bCommsOn || bInstrumentsOn) && BotStep >= 3)
	{
		if (bCommsOn) { Press(EMCSurfRole::Power, 1); } else { Press(EMCSurfRole::Power, 2); }
		DebugLog(FString::Printf(TEXT("bot cut load (event=%d brownout=%d paused=%d)"), bPowerEvent, bBrownout, bDrillPaused));
		BotWait = 0.5f;
		return;
	}
	switch (BotStep)
	{
	case 0:
		for (int32 k = 0; k < 2; ++k) { for (int32 i = 0; i < Sites.Num(); ++i) { Press(EMCSurfRole::Science, 1 + i); Press(EMCSurfRole::Science, 5); } }
		BotStep = 1; break;
	case 1:
	{
		int32 Best = 0; float BestV = -1e9f;
		for (int32 i = 0; i < Sites.Num(); ++i)
		{
			const float V = Sites[i].Hydrogen - (Sites[i].TempK - 100.f) * 0.2f;
			if (V > BestV) { BestV = V; Best = i; }
		}
		Press(EMCSurfRole::Science, 1 + Best);
		DebugLog(FString::Printf(TEXT("bot chose site %d (truth ice=%.2f likely=%d)"), Best, IceFrac[Best], IceFrac[Best] >= Cfg().IceLikelyThreshold));
		BotStep = 2; break;
	}
	case 2:
	{
		if (!Sites.IsValidIndex(ChosenSite)) { BotStep = 1; break; }
		const FVector2D To = Sites[ChosenSite].Pos - RoverPos;
		if (To.Size() < SiteRadius * 0.6f) { HandleDrive(0, 0, 0); BotStep = 3; DebugLog(FString::Printf(TEXT("bot arrived t=%.1f hits=%d"), ElapsedSec, RockHits)); break; }
		FVector2D Want = To.GetSafeNormal();
		for (int32 i = 0; i < RockPos.Num(); ++i)
		{
			const FVector2D Away = RoverPos - RockPos[i];
			const float D = Away.Size();
			const float Zone = RockRadius[i] + Cfg().RoverRadiusCm + 450.f;
			if (D < Zone && D > 1.f) { Want += Away / D * (2.5f * (Zone - D) / Zone + 0.5f); }
		}
		const float WantYaw = FMath::RadiansToDegrees(FMath::Atan2(Want.Y, Want.X));
		const float Diff = FMath::UnwindDegrees(WantYaw - RoverYaw);
		HandleDrive(FMath::Abs(Diff) < 70.f ? 1.f : 0.f, FMath::Clamp(Diff / 15.f, -1.f, 1.f), 0);
		break;
	}
	case 3:
		HandleDrive(0, 0, 0);
		if (RoverAtSite < 0) { BotStep = 2; break; }
		Press(EMCSurfRole::Drill, 3);
		Press(EMCSurfRole::Drill, 4);
		BotStep = 4; break;
	case 4:
		if (bSampleReady) { Press(EMCSurfRole::Drill, 5); BotStep = 5; BotWait = 1.f; }
		else if (!bDrilling && !bSampleReady) { BotStep = 3; }
		break;
	case 5:
		if (BotWait <= 0.f && !bDone) { DebugLog(FString::Printf(TEXT("bot sample done: samples=%d"), Samples.Num())); BotStep = 6; }
		break;
	default: break;
	}
}
