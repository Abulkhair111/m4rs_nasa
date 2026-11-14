#include "Construction/MCConstructionManager.h"
#include "Construction/MCPartSocket.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "MissionCrew.h"

namespace
{
	const TCHAR* CheckLabels[MCConst::NumChecks] = {
		TEXT("Launcher chosen"), TEXT("Mass vs payload"), TEXT("Budget"), TEXT("Power balance"), TEXT("Comms link"),
		TEXT("Fuel"), TEXT("Thrust (T/W)"), TEXT("Ice science"), TEXT("Power grid"), TEXT("Structure (bolts)")
	};
	const TCHAR* RoleLabels[MCConst::NumRoles] = {
		TEXT("Solo"), TEXT("Power"), TEXT("Construction"), TEXT("Propulsion"), TEXT("Science")
	};
	const TCHAR* StateLabel(EMCCheckState S)
	{
		return S == EMCCheckState::Green ? TEXT("OK") : (S == EMCCheckState::Yellow ? TEXT("WARN") : TEXT("FAIL"));
	}
}

AMCConstructionManager::AMCConstructionManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.25f;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(10.f);

	Checks.Init(EMCCheckState::Red, MCConst::NumChecks);
	DeptReady.Init(false, MCConst::NumRoles);
	RoleGood.Init(0, MCConst::NumRoles);
	RoleBad.Init(0, MCConst::NumRoles);

	// Placeholder launchers (TBD in the design doc). Index = lever number.
	Rockets = {
		{ FName("Falcon 9"),      1800.f, 60.f,  0.95f },
		{ FName("Vulcan"),        3000.f, 110.f, 0.97f },
		{ FName("Falcon Heavy"),  5500.f, 150.f, 0.93f }
	};
}

void AMCConstructionManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCConstructionManager, bActive);
	DOREPLIFETIME(AMCConstructionManager, bLaunched);
	DOREPLIFETIME(AMCConstructionManager, PhaseEndTime);
	DOREPLIFETIME(AMCConstructionManager, RocketName);
	DOREPLIFETIME(AMCConstructionManager, RocketPayloadKg);
	DOREPLIFETIME(AMCConstructionManager, RocketCostM);
	DOREPLIFETIME(AMCConstructionManager, RocketReliability);
	DOREPLIFETIME(AMCConstructionManager, SelectedLever);
	DOREPLIFETIME(AMCConstructionManager, FuelKg);
	DOREPLIFETIME(AMCConstructionManager, FuelMaxKg);
	DOREPLIFETIME(AMCConstructionManager, DryMassKg);
	DOREPLIFETIME(AMCConstructionManager, FuelRequiredKg);
	DOREPLIFETIME(AMCConstructionManager, TotalMassKg);
	DOREPLIFETIME(AMCConstructionManager, TotalCostM);
	DOREPLIFETIME(AMCConstructionManager, PowerGenW);
	DOREPLIFETIME(AMCConstructionManager, PowerUseW);
	DOREPLIFETIME(AMCConstructionManager, StorageWh);
	DOREPLIFETIME(AMCConstructionManager, DataMbps);
	DOREPLIFETIME(AMCConstructionManager, BandwidthMbps);
	DOREPLIFETIME(AMCConstructionManager, ScienceValue);
	DOREPLIFETIME(AMCConstructionManager, Reliability);
	DOREPLIFETIME(AMCConstructionManager, ThrustN);
	DOREPLIFETIME(AMCConstructionManager, TWR);
	DOREPLIFETIME(AMCConstructionManager, IceCount);
	DOREPLIFETIME(AMCConstructionManager, UnboltedCount);
	DOREPLIFETIME(AMCConstructionManager, OffGridCount);
	DOREPLIFETIME(AMCConstructionManager, bHasEngine);
	DOREPLIFETIME(AMCConstructionManager, bHasLidar);
	DOREPLIFETIME(AMCConstructionManager, BoosterCount);
	DOREPLIFETIME(AMCConstructionManager, LanderTotals);
	DOREPLIFETIME(AMCConstructionManager, AscentTotals);
	DOREPLIFETIME(AMCConstructionManager, Checks);
	DOREPLIFETIME(AMCConstructionManager, DeptReady);
	DOREPLIFETIME(AMCConstructionManager, RoleGood);
	DOREPLIFETIME(AMCConstructionManager, RoleBad);
	DOREPLIFETIME(AMCConstructionManager, Report);
	DOREPLIFETIME(AMCConstructionManager, Message);
	DOREPLIFETIME(AMCConstructionManager, MessageTime);
}

void AMCConstructionManager::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		DiscoverSockets();
		Recalc();
	}
}

void AMCConstructionManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Time running out has no hard rule yet (design TBD); only announce it once.
	if (HasAuthority() && bActive && PhaseEndTime > 0.0 && GetTimeLeft() <= 0.f)
	{
		PhaseEndTime = 0.0;
		Say(TEXT("Time is up! Launch now or the window is lost."));
	}
}

// ------------------------------------------------------------------ queries

float AMCConstructionManager::GetTimeLeft() const
{
	if (PhaseEndTime <= 0.0) { return 0.f; }
	const UWorld* World = GetWorld();
	const AGameStateBase* GS = World ? World->GetGameState() : nullptr;
	const double Now = GS ? GS->GetServerWorldTimeSeconds() : (World ? World->GetTimeSeconds() : 0.0);
	return FMath::Max(0.f, static_cast<float>(PhaseEndTime - Now));
}

EMCCheckState AMCConstructionManager::GetCheck(EMCCheck Index) const
{
	return Checks.IsValidIndex((int32)Index) ? Checks[(int32)Index] : EMCCheckState::Red;
}

bool AMCConstructionManager::RoleAllowed(EMCCrewRole InRole, int32 RequiredDepartment) const
{
	return bFreeRoles || InRole == EMCCrewRole::None || (int32)InRole == RequiredDepartment;
}

TArray<UMCPartDef*> AMCConstructionManager::GetCandidates(EMCSocketType SocketType, EMCCrewRole InRole, EMCStackGroup Group) const
{
	TArray<UMCPartDef*> Out;
	for (UMCPartDef* D : Catalog)
	{
		if (D && D->ToolType == EMCToolType::None && D->SocketType == SocketType && D->GetStackGroup() == Group
			&& RoleAllowed(InRole, D->Department))
		{
			Out.Add(D);
		}
	}
	return Out;
}

FMCPreviewResult AMCConstructionManager::Preview(const UMCPartDef* Def) const
{
	FMCPreviewResult R;
	if (!Def) { return R; }

	const bool bAsc = Def->GetStackGroup() == EMCStackGroup::Ascent;
	R.Group = Def->GetStackGroup();

	const float DryNew = DryMassKg + Def->MassKg;
	const float FuelMaxNew = FuelMaxKg + Def->FuelCapKg;
	const float FuelNew = FMath::Min(FuelKg, FuelMaxNew);
	R.TotalMassKg = DryNew + FuelNew; // take-off mass: lander + ascent
	R.TotalCostM = TotalCostM + Def->CostM;
	R.bAffordable = R.TotalCostM <= BudgetM;

	// Group sub-results. Mass of the group that does not receive the part is unchanged.
	R.Lander.MassKg = LanderTotals.MassKg + (bAsc ? 0.f : Def->MassKg);
	R.Lander.CostM = LanderTotals.CostM + (bAsc ? 0.f : Def->CostM);
	R.Lander.PowerBalance = (LanderTotals.PowerGenW - LanderTotals.PowerUseW) + (bAsc ? 0.f : Def->PowerW);
	R.Lander.ScienceValue = LanderTotals.ScienceValue + (bAsc ? 0.f : Def->ScienceValue);
	R.Lander.IceCount = LanderTotals.IceCount + ((!bAsc && Def->bIceCapable) ? 1 : 0);
	R.Ascent.MassKg = AscentTotals.MassKg + (bAsc ? Def->MassKg : 0.f);
	R.Ascent.CostM = AscentTotals.CostM + (bAsc ? Def->CostM : 0.f);
	R.Ascent.ThrustN = AscentTotals.ThrustN + (bAsc ? Def->ThrustN : 0.f);
	R.Ascent.FuelMaxKg = AscentTotals.FuelMaxKg + (bAsc ? Def->FuelCapKg : 0.f);
	R.Ascent.BoosterCount = AscentTotals.BoosterCount + ((bAsc && Def->SocketType == EMCSocketType::Booster) ? 1 : 0);

	const float ThrustNew = R.Ascent.ThrustN; // take-off thrust comes from the ascent group only
	R.TWR = R.TotalMassKg > KINDA_SMALL_NUMBER ? ThrustNew / (R.TotalMassKg * MCConst::MoonG) : 0.f;
	const bool bEngine = bHasEngine || R.Ascent.BoosterCount > 0 || (bAsc && Def->SocketType == EMCSocketType::Engine);
	const bool bLidar = bHasLidar || Def->SocketType == EMCSocketType::Lidar;
	R.TWRState = (!bEngine || R.TWR < MCConst::TWRRed) ? EMCCheckState::Red
		: ((R.TWR >= MCConst::TWRGreen && bLidar) ? EMCCheckState::Green : EMCCheckState::Yellow);

	const float Required = 0.9f * DryNew;
	R.FuelMargin = Required > KINDA_SMALL_NUMBER ? (FuelNew - Required) / Required : 0.f;
	R.FuelState = (FuelMaxNew <= 0.f || R.FuelMargin < 0.f) ? EMCCheckState::Red
		: (R.FuelMargin >= 0.1f ? EMCCheckState::Green : EMCCheckState::Yellow);

	const float Gen = PowerGenW + FMath::Max(Def->PowerW, 0.f);
	const float Use = PowerUseW + FMath::Max(-Def->PowerW, 0.f);
	R.PowerBalance = Gen - Use;
	const float Storage = StorageWh + Def->StorageWh;
	R.PowerState = (Gen <= 0.f || Gen < Use) ? EMCCheckState::Red
		: ((Gen >= 1.1f * Use && Storage > 0.f) ? EMCCheckState::Green : EMCCheckState::Yellow);
	return R;
}

// ------------------------------------------------------------------ sockets

void AMCConstructionManager::DiscoverSockets()
{
	if (!GetWorld()) { return; }
	for (TActorIterator<AMCPartSocket> It(GetWorld()); It; ++It)
	{
		RegisterSocket(*It);
	}
}

void AMCConstructionManager::RegisterSocket(AMCPartSocket* Socket)
{
	if (!HasAuthority() || !Socket || Sockets.Contains(Socket)) { return; }
	Sockets.Add(Socket);
	Socket->OnChanged.AddUObject(this, &AMCConstructionManager::OnSocketChanged);
}

void AMCConstructionManager::OnSocketChanged()
{
	Recalc();
}

// ------------------------------------------------------------------ state changes (server)

void AMCConstructionManager::SetRocketFields(int32 Index)
{
	if (Rockets.IsValidIndex(Index))
	{
		const FMCRocketDef& R = Rockets[Index];
		SelectedLever = Index;
		RocketName = R.Name;
		RocketPayloadKg = R.PayloadKg;
		RocketCostM = R.CostM;
		RocketReliability = R.Reliability;
	}
	else
	{
		SelectedLever = -1;
		RocketName = NAME_None;
		RocketPayloadKg = 0.f;
		RocketCostM = 0.f;
		RocketReliability = 1.f;
	}
}

void AMCConstructionManager::StartPhase()
{
	if (!HasAuthority()) { return; }
	DiscoverSockets();
	bActive = true;
	bLaunched = false;
	bYellowWarned = false;
	bPreflightRan = false;
	for (bool& B : DeptReady) { B = false; }
	for (int32& V : RoleGood) { V = 0; }
	for (int32& V : RoleBad) { V = 0; }
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	const double Now = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	PhaseEndTime = Now + PhaseDurationSec;
	Recalc();
	Say(TEXT("Assembly phase started."));
}

void AMCConstructionManager::SelectRocket(int32 LeverIndex)
{
	if (!HasAuthority() || !bActive || bLaunched) { return; }
	if (!Rockets.IsValidIndex(LeverIndex)) { return; }
	SetRocketFields(LeverIndex);
	bYellowWarned = false;
	Recalc();
	Say(FString::Printf(TEXT("Launcher: %s (payload %.0f kg)"), *RocketName.ToString(), RocketPayloadKg));
}

void AMCConstructionManager::AddFuel(float DeltaKg)
{
	if (!HasAuthority() || !bActive || bLaunched) { return; }
	FuelKg = FMath::Clamp(FuelKg + DeltaKg, 0.f, FuelMaxKg);
	bYellowWarned = false;
	Recalc();
}

void AMCConstructionManager::ToggleReady(int32 Department)
{
	if (!HasAuthority() || !bActive || bLaunched || !DeptReady.IsValidIndex(Department) || Department < 1) { return; }
	DeptReady[Department] = !DeptReady[Department];
	Say(FString::Printf(TEXT("%s: %s"), RoleLabels[Department], DeptReady[Department] ? TEXT("READY") : TEXT("not ready")));
}

bool AMCConstructionManager::PlacePart(AMCPartSocket* Socket, UMCPartDef* Def, EMCCrewRole InRole)
{
	if (!HasAuthority()) { return false; }
	FString Why;
	if (!bActive || bLaunched)                                     { Why = TEXT("Assembly is not active."); }
	else if (!Socket || !Def || Def->IsTool())                     { Why = TEXT("Invalid part or socket."); }
	else if (!Socket->IsEmpty() || Socket->bReserved)              { Why = TEXT("Socket is already taken."); }
	else if (Def->SocketType != Socket->SocketType)                { Why = TEXT("That part does not fit this socket."); }
	else if (Def->GetStackGroup() != Socket->GetGroup())           { Why = Def->GetStackGroup() == EMCStackGroup::Ascent
		? TEXT("Ascent part (tank, engine, booster): it goes on the ascent vehicle, not the lander.")
		: TEXT("Lander part: it goes on the lander, not the ascent vehicle."); }
	else if (!RoleAllowed(InRole, Def->Department))                  { Why = TEXT("That part belongs to another department."); }
	else if (TotalCostM + Def->CostM > BudgetM)                    { Why = TEXT("Over budget."); }
	if (!Why.IsEmpty())
	{
		Say(Why);
		return false;
	}

	Socket->SetReserved(true);
	if (OnPlacementApproved.IsBound())
	{
		OnPlacementApproved.Broadcast(Socket, Def, InRole, Def->bHeavy);
	}
	else
	{
		FinishPlacement(Socket, Def, nullptr, InRole);
	}
	return true;
}

void AMCConstructionManager::FinishPlacement(AMCPartSocket* Socket, UMCPartDef* Def, AActor* PartActor, EMCCrewRole InRole)
{
	if (!HasAuthority() || !Socket || !Def) { return; }
	Socket->Install(Def, PartActor); // clears bReserved, resets bolt/grid, triggers Recalc through OnChanged
	// Good unless a Science part cannot detect ice.
	const bool bGood = !(Def->Department == (int32)EMCCrewRole::Science && !Def->bIceCapable);
	Score(InRole, bGood);
	bYellowWarned = false;
	Say(FString::Printf(TEXT("Installed: %s"), *Def->DisplayName.ToString()));
}

bool AMCConstructionManager::RemovePart(AMCPartSocket* Socket, EMCCrewRole InRole)
{
	if (!HasAuthority() || !bActive || bLaunched || !Socket || Socket->IsEmpty()) { return false; }
	if (Socket->bBolted) { Say(TEXT("Unbolt with the wrench first.")); return false; }
	if (!RoleAllowed(InRole, Socket->Installed->Department)) { Say(TEXT("That part belongs to another department.")); return false; }
	Socket->Remove();
	bYellowWarned = false;
	return true;
}

bool AMCConstructionManager::ApplyTool(AMCPartSocket* Target, EMCToolType Tool, EMCCrewRole InRole)
{
	if (!HasAuthority() || !bActive || bLaunched || !Target || Target->IsEmpty()) { return false; }
	if (Tool == EMCToolType::Wrench)
	{
		if (!RoleAllowed(InRole, (int32)EMCCrewRole::Construction)) { Say(TEXT("Only the Construction Engineer uses the wrench.")); return false; }
		Target->SetBolted(!Target->bBolted);
		Score(EMCCrewRole::Construction, true);
	}
	else if (Tool == EMCToolType::Cable)
	{
		if (!RoleAllowed(InRole, (int32)EMCCrewRole::Power)) { Say(TEXT("Only the Power Engineer uses the cable reel.")); return false; }
		if (Target->IsSelfPowered()) { Say(TEXT("Tanks and boosters do not need power cables.")); return false; }
		Target->SetGrid(!Target->bGrid); // triggers Recalc
		Score(EMCCrewRole::Power, PowerGenW >= PowerUseW);
	}
	else
	{
		return false;
	}
	bYellowWarned = false;
	return true;
}

// ------------------------------------------------------------------ recalculation

void AMCConstructionManager::Recalc()
{
	if (!HasAuthority()) { return; }

	float Rel = RocketReliability > 0.f ? RocketReliability : 1.f;
	FMCGroupTotals L, A;
	L.MassKg = LanderBaseMassKg;
	L.CostM = LanderBaseCostM;
	bool bLidar = false;

	for (const AMCPartSocket* S : Sockets)
	{
		if (!S || S->IsEmpty()) { continue; }
		const UMCPartDef* D = S->Installed;
		const bool bLive = S->IsLive();
		FMCGroupTotals& G = (D->GetStackGroup() == EMCStackGroup::Ascent) ? A : L;

		G.MassKg += D->MassKg;
		G.CostM += D->CostM;
		G.FuelMaxKg += D->FuelCapKg;
		++G.PartCount;
		Rel *= D->GetReliabilityFactor();

		if (!S->bBolted) { ++G.UnboltedCount; }
		if (!bLive && !S->IsSelfPowered()) { ++G.OffGridCount; }
		if (!bLive) { continue; }

		G.PowerGenW += FMath::Max(D->PowerW, 0.f);
		G.PowerUseW += FMath::Max(-D->PowerW, 0.f);
		G.StorageWh += D->StorageWh;
		G.DataMbps += D->DataMbps;
		G.BandwidthMbps += D->BandwidthMbps;
		G.ScienceValue += D->ScienceValue;
		G.ThrustN += D->ThrustN;
		if (D->bIceCapable) { ++G.IceCount; }
		if (D->SocketType == EMCSocketType::Engine) { ++G.EngineCount; }
		if (D->SocketType == EMCSocketType::Booster) { ++G.BoosterCount; }
		if (D->SocketType == EMCSocketType::Lidar) { bLidar = true; }
	}

	// Fuel is carried by the ascent group (tanks only exist there). Whole-vehicle mass is the take-off mass.
	FuelMaxKg = A.FuelMaxKg;
	FuelKg = FMath::Clamp(FuelKg, 0.f, FuelMaxKg);
	DryMassKg = L.MassKg + A.MassKg;
	A.MassKg += FuelKg;
	TotalMassKg = DryMassKg + FuelKg;
	FuelRequiredKg = 0.9f * DryMassKg;
	TotalCostM = L.CostM + A.CostM + RocketCostM;
	Reliability = Rel;

	LanderTotals = L;
	AscentTotals = A;

	// Lander drives comms, science, ice and storage; power counts every consumer (ascent hardware may draw too).
	PowerGenW = L.PowerGenW + A.PowerGenW;
	PowerUseW = L.PowerUseW + A.PowerUseW;
	StorageWh = L.StorageWh + A.StorageWh;
	DataMbps = L.DataMbps; BandwidthMbps = L.BandwidthMbps; ScienceValue = L.ScienceValue;
	IceCount = L.IceCount;
	// Ascent drives thrust and fuel.
	ThrustN = A.ThrustN;
	BoosterCount = A.BoosterCount;
	bHasEngine = A.EngineCount > 0;
	UnboltedCount = L.UnboltedCount + A.UnboltedCount;
	OffGridCount = L.OffGridCount + A.OffGridCount;
	bHasLidar = bLidar;
	TWR = TotalMassKg > KINDA_SMALL_NUMBER ? ThrustN / (TotalMassKg * MCConst::MoonG) : 0.f;

	RecalcChecks();
	BuildReport();
	UpdateAlerts();
}

void AMCConstructionManager::RecalcChecks()
{
	auto Set = [this](EMCCheck I, EMCCheckState S) { Checks[(int32)I] = S; };

	Set(EMCCheck::Launcher, SelectedLever >= 0 ? EMCCheckState::Green : EMCCheckState::Red);

	// Mass vs payload (payload counts dry + fuel, i.e. total mass).
	Set(EMCCheck::Mass, RocketPayloadKg <= 0.f || TotalMassKg > RocketPayloadKg ? EMCCheckState::Red
		: (TotalMassKg <= 0.9f * RocketPayloadKg ? EMCCheckState::Green : EMCCheckState::Yellow));

	Set(EMCCheck::Budget, TotalCostM > BudgetM ? EMCCheckState::Red
		: (TotalCostM <= 0.9f * BudgetM ? EMCCheckState::Green : EMCCheckState::Yellow));

	Set(EMCCheck::Power, (PowerGenW <= 0.f || PowerGenW < PowerUseW) ? EMCCheckState::Red
		: ((PowerGenW >= 1.1f * PowerUseW && StorageWh > 0.f) ? EMCCheckState::Green : EMCCheckState::Yellow));

	// Comms: no link or data above the link is red; 10% headroom is green. (Exact rule TBD in the BP; same shape as the HUD pressure.)
	Set(EMCCheck::Comms, (BandwidthMbps <= 0.f || DataMbps > BandwidthMbps) ? EMCCheckState::Red
		: (BandwidthMbps >= 1.1f * DataMbps ? EMCCheckState::Green : EMCCheckState::Yellow));

	Set(EMCCheck::Fuel, (FuelMaxKg <= 0.f || FuelKg < FuelRequiredKg) ? EMCCheckState::Red
		: (FuelKg >= 1.1f * FuelRequiredKg ? EMCCheckState::Green : EMCCheckState::Yellow));

	// Take-off from the Moon: ascent thrust (engines + boosters) vs the whole stack (lander + ascent + fuel).
	Set(EMCCheck::Thrust, ((!bHasEngine && BoosterCount <= 0) || TWR < MCConst::TWRRed) ? EMCCheckState::Red
		: ((TWR >= MCConst::TWRGreen && bHasLidar) ? EMCCheckState::Green : EMCCheckState::Yellow));

	Set(EMCCheck::IceScience, IceCount <= 0 ? EMCCheckState::Red
		: ((IceCount >= 2 && ScienceValue >= 50.f) ? EMCCheckState::Green : EMCCheckState::Yellow));

	Set(EMCCheck::Grid, OffGridCount > 0 ? EMCCheckState::Yellow : EMCCheckState::Green);
	Set(EMCCheck::Structure, UnboltedCount > 0 ? EMCCheckState::Red : EMCCheckState::Green);
}

void AMCConstructionManager::BuildReport()
{
	FString Out;
	Out += FString::Printf(TEXT("Launcher: %s | Mass %.0f / %.0f kg | Cost $%.0fM / $%.0fM\n"),
		SelectedLever >= 0 ? *RocketName.ToString() : TEXT("none"), TotalMassKg, RocketPayloadKg, TotalCostM, BudgetM);
	Out += FString::Printf(TEXT("LANDER: %.0f kg, $%.0fM | Power %.0f / %.0f W | Science %.0f, ice x%d | Link %.1f / %.1f Mbps | loose %d, unwired %d\n"),
		LanderTotals.MassKg, LanderTotals.CostM, LanderTotals.PowerGenW, LanderTotals.PowerUseW, LanderTotals.ScienceValue, LanderTotals.IceCount,
		LanderTotals.BandwidthMbps, LanderTotals.DataMbps, LanderTotals.UnboltedCount, LanderTotals.OffGridCount);
	Out += FString::Printf(TEXT("ASCENT: %.0f kg, $%.0fM | T/W %.2f (take-off %.0f kg, thrust %.1f kN) | Fuel %.0f / %.0f kg (need %.0f) | boosters %d | loose %d\n"),
		AscentTotals.MassKg, AscentTotals.CostM, TWR, TotalMassKg, ThrustN / 1000.f, FuelKg, FuelMaxKg, FuelRequiredKg,
		BoosterCount, AscentTotals.UnboltedCount);
	for (int32 i = 0; i < MCConst::NumChecks; ++i)
	{
		Out += FString::Printf(TEXT("[%s] %s\n"), StateLabel(Checks[i]), CheckLabels[i]);
	}
	Out += TEXT("ACCURACY\n");
	for (int32 r = 1; r < MCConst::NumRoles; ++r)
	{
		const int32 Total = RoleGood[r] + RoleBad[r];
		Out += FString::Printf(TEXT("%s %d/%d\n"), RoleLabels[r], RoleGood[r], Total);
	}
	Report = Out;
}

void AMCConstructionManager::UpdateAlerts()
{
	auto IsRed = [this](EMCCheck I) { return bPreflightRan && Checks[(int32)I] == EMCCheckState::Red; };
	for (AMCPartSocket* S : Sockets)
	{
		if (!S) { continue; }
		bool bAlert = false;
		if (!S->IsEmpty())
		{
			bAlert = !S->bBolted || (!S->IsLive() && !S->IsSelfPowered());
			switch (S->SocketType)
			{
			case EMCSocketType::Power:
			case EMCSocketType::Battery: bAlert |= IsRed(EMCCheck::Power); break;
			case EMCSocketType::Comms:   bAlert |= IsRed(EMCCheck::Comms); break;
			case EMCSocketType::Engine:
			case EMCSocketType::Booster:
			case EMCSocketType::Lidar:  bAlert |= IsRed(EMCCheck::Thrust); break;
			case EMCSocketType::Tank:    bAlert |= IsRed(EMCCheck::Fuel); break;
			case EMCSocketType::Science:
			case EMCSocketType::Drill:
			case EMCSocketType::Rover:   bAlert |= IsRed(EMCCheck::IceScience); break;
			default: break;
			}
		}
		if (S->bAlert != bAlert) { S->SetAlert(bAlert); }
	}
}

void AMCConstructionManager::RunPreflight()
{
	if (!HasAuthority()) { return; }
	Recalc();
	bPreflightRan = true;
	auto Mark = [this](EMCCrewRole R, std::initializer_list<EMCCheck> Which)
	{
		for (EMCCheck C : Which) { Score(R, Checks[(int32)C] != EMCCheckState::Red); }
	};
	Mark(EMCCrewRole::Power,        { EMCCheck::Power });
	Mark(EMCCrewRole::Construction, { EMCCheck::Launcher, EMCCheck::Mass, EMCCheck::Structure });
	Mark(EMCCrewRole::Propulsion,   { EMCCheck::Fuel, EMCCheck::Thrust });
	Mark(EMCCrewRole::Science,      { EMCCheck::IceScience });
	Recalc(); // refresh report and alerts with the new flag and scores
}

bool AMCConstructionManager::TryLaunch()
{
	if (!HasAuthority() || !bActive || bLaunched) { return false; }
	for (int32 d = 1; d < MCConst::NumRoles; ++d)
	{
		if (!DeptReady[d]) { Say(FString::Printf(TEXT("%s is not READY."), RoleLabels[d])); return false; }
	}
	RunPreflight();

	bool bRed = false, bYellow = false;
	for (EMCCheckState S : Checks)
	{
		bRed |= (S == EMCCheckState::Red);
		bYellow |= (S == EMCCheckState::Yellow);
	}
	if (bRed) { Say(TEXT("LAUNCH ABORTED: red checks remain.")); bYellowWarned = false; return false; }
	if (bYellow && !bYellowWarned)
	{
		bYellowWarned = true;
		Say(TEXT("Warnings remain. Press LAUNCH again to go anyway."));
		return false;
	}

	bLaunched = true;
	bActive = false;
	PhaseEndTime = 0.0;
	Say(TEXT("LAUNCH!"));
	UE_LOG(LogMissionCrew, Log, TEXT("Construction: launch complete, T/W %.2f, mass %.0f kg"), TWR, TotalMassKg);
	OnLaunchComplete.Broadcast(BuildLoadout());
	return true;
}

FMCLoadout AMCConstructionManager::BuildLoadout() const
{
	FMCLoadout L;
	L.RocketName = RocketName;
	L.FuelKg = FuelKg;
	L.TotalMassKg = TotalMassKg;
	L.TWR = TWR;
	L.Reliability = Reliability;
	L.ScienceValue = ScienceValue;
	L.IceCount = IceCount;
	for (const AMCPartSocket* S : Sockets)
	{
		if (S && !S->IsEmpty())
		{
			FMCLoadoutEntry E;
			E.SocketType = S->SocketType;
			E.Def = S->Installed;
			E.bBolted = S->bBolted;
			E.bGrid = S->bGrid;
			L.Parts.Add(E);
		}
	}
	return L;
}

// ------------------------------------------------------------------ log / messages

void AMCConstructionManager::Score(EMCCrewRole InRole, bool bGood)
{
	if (!HasAuthority()) { return; }
	const int32 R = FMath::Clamp((int32)InRole, 0, MCConst::NumRoles - 1);
	(bGood ? RoleGood : RoleBad)[R] += 1;
	BuildReport();
}

void AMCConstructionManager::Say(const FString& Text)
{
	if (!HasAuthority()) { return; }
	Message = Text;
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	MessageTime = GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	BP_OnMessage(Text);
	UE_LOG(LogMissionCrew, Log, TEXT("Construction: %s"), *Text);
}

void AMCConstructionManager::OnRep_Message()
{
	BP_OnMessage(Message);
}
