// S2 Launch & Transit: server-authoritative rules, C++ port of BP_TransitManager (see Vault/S2 Launch & Transit - Handoff.md).
// Clients hold NO logic: they read the replicated values/strings below and send an action code through a pawn/controller
// Server RPC that calls HandleAction(Code, Who) on the server (this actor is not owned by a client, so it has no RPC itself).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Construction/MCConstructionTypes.h"
#include "Transit/MCTransitTypes.h"
#include "MCTransitManager.generated.h"

class AMCConstructionManager;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCTransitManager : public AActor
{
	GENERATED_BODY()

public:
	AMCTransitManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------------- Config ----------------
	/** Optional tuning asset. Null = built-in defaults (all values are TBD placeholders). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TObjectPtr<UMCTransitConfig> Config;
	/** Every role may send every action (solo testing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Config") bool bFreeRoles = false;
	/** Use the default test loadout (rover, drill, legs; NSS, NIRVSS, MSolo) instead of the S1 loadout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bTestMode = false;
	/** 0 = random seed; otherwise the fixed seed (same seed = same events). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") int32 SeedOverride = 0;
	/** Start by itself after the S1 launch (binds AMCConstructionManager::OnLaunchComplete / polls bLaunched). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bAutoStart = true;
	/** Start immediately in BeginPlay with the test loadout (maps named L_Transit also do this). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bStartOnBeginPlay = false;
	/** After the arrival report, ServerTravel here (S3). Empty = stay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Flow") FString NextMapName = TEXT("/Game/MissionCrew/Maps/L_Orbit");
	/** Seconds the arrival report stays up before the travel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Flow") float TravelDelaySec = 12.f;
	/** Run the scripted bot (RunDebugTest) as soon as the transit starts; implies bFreeRoles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bDebugAutoTest = false;

	/** SERVER: after BeginTransit, re-apply the recorded crew roles (UMCMissionSubsystem) and seat every player at its own station. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bAutoSeat = true;
	/** Seconds after BeginTransit before the first seating attempt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float SeatDelaySec = 2.f;
	/** Keep retrying once per second for late joiners (clients reconnect after the travel) for this long. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float SeatRetrySec = 30.f;

	// ---------------- Replicated state (plain values, for the UI) ----------------
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") EMCTransitStage Stage = EMCTransitStage::GoPoll;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bStarted = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bDone = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 Seed = 0;
	/** Server world time when the current stage ends (0 = open-ended, e.g. GO poll). Use GetStageTimeLeft(). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") double StageEndTime = 0.0;
	/** Seconds since the start of the coast (0 before), updated ~5x per second. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") float ElapsedSec = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") float TransitDurationSec = 180.f;
	/** GO FOR LAUNCH pressed, index 1..4 (size 5). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") TArray<bool> GoFlags;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Ship") float Health = 100.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Ship") float FuelKg = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Ship") float FuelStartKg = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Ship") float LaunchReliability = 1.f;

	// Pilot: the DISPLAYED predicted miss (km). The true value is server-only and differs after the sensor fault.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pilot") float DispX = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pilot") float DispY = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pilot") float CorridorKm = 15.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pilot") TArray<FMCTransitWindow> Windows;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Systems") TArray<FMCTransitSwitch> Switches;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Systems") TArray<FMCTransitAlarm> Alarms;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Equipment") TArray<FMCTransitDevice> Devices;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Equipment") int32 ActiveDevice = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Science") TArray<FMCTransitInstrument> Instruments;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Science") int32 ActiveInstrument = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Science") float CalTarget = 50.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Science") float CalTolerance = 3.f;

	// Short strings built on the server (the HUD only shows them).
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Text") FString TopText;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Text") FString BoardText;
	/** Size 5, index 1..4 = role panel text, 0 = shared message line. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Text") TArray<FString> PanelText;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Message, Category = "Text") FString Message;

	// Results
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") int32 TransitScore = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") FMCTransitReport Result;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") TArray<int32> RoleGood;   // size 5
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") TArray<int32> RoleBad;    // size 5
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") FString ReportText;

	// ---------------- Events ----------------
	/** Server only. Fires once at arrival with the final report (hand-off to S3). */
	UPROPERTY(BlueprintAssignable, Category = "Events") FMCTransitCompleteSignature OnTransitComplete;
	UFUNCTION(BlueprintImplementableEvent, Category = "Events") void BP_OnMessage(const FString& Text);

	// ---------------- API ----------------
	/** Server: load the loadout, draw all random values (InitData), open the GO poll. Safe to call twice (ignored). */
	UFUNCTION(BlueprintCallable, Category = "Transit") void BeginTransit();
	/**
	 * Server: the single entry point for every player action. Code = Role*100+Action (90 = GO), Who = the sender's CrewRole
	 * (1..4, 0 = unassigned). Accepted when bFreeRoles or Who == the role in the code (GO: any Who 1..4, or 0 with free roles).
	 * Returns true if the action was accepted (even when it scored bad).
	 */
	UFUNCTION(BlueprintCallable, Category = "Transit") bool HandleAction(int32 Code, int32 Who);
	/** Scripted run of the whole phase with a bot; prints "DEBUGTEST" lines to the log. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Transit") void RunDebugTest();

	UFUNCTION(BlueprintPure, Category = "Transit") float GetStageTimeLeft() const;
	/** True error is hidden from clients; this tells the HUD that the display is currently stale (only the server knows; replicated). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pilot") bool bDisplayStale = false;
	/** Flight view (agent F3): the TRUE predicted miss in km (server ErrX/ErrY), replicated so the 3D ship can drift off the corridor
	 *  even while the Pilot's display (DispX/DispY) is stale. Visual only: no client logic reads it. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Flight") float ShipErrX = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Flight") float ShipErrY = 0.f;
	/** Flight view: counts accepted Pilot pulses; LastPulseDir is the EMCPilotAction of the last one (flare / RCS puff). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Flight") int32 PulseSeq = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Flight") int32 LastPulseDir = 0;
	/** Reads the CrewRole property of a controller's PlayerState by reflection (0 if absent). */
	UFUNCTION(BlueprintCallable, Category = "Transit") static int32 GetCrewRoleOfController(const AController* Controller);

protected:
	UFUNCTION() void OnRep_Message();
	UFUNCTION() void OnConstructionLaunched(const FMCLoadout& Loadout);

	const UMCTransitConfig& Cfg() const;
	void FindConstructionManager();
	void LoadLoadout(bool bUseDefault);
	void InitData();
	void AutoSeatTick();
	FTimerHandle SeatTimer;
	float SeatElapsed = 0.f;
	TSet<TWeakObjectPtr<APawn>> AutoSeated;
	void SetStage(EMCTransitStage NewStage, float DurationSec);
	void Say(const FString& Text);
	void Score(int32 Role, bool bGood);

	void StageTick(float Dt);
	void CoastTick(float Dt);
	void Arrive();
	void RebuildStrings();
	void DebugBotTick();
	void DebugLog(const FString& Text) const;

	bool DoGo(int32 Who);
	bool DoSystems(int32 Action, int32 Who);
	bool DoEquip(int32 Action);
	bool DoPilot(int32 Action);
	bool DoScience(int32 Action);
	void OpenWindow(int32 Index);
	void CloseWindow(int32 Index);
	void RaiseAlarm(int32 Index);
	void RefreshDisplay();
	void RefreshSwitchLabels();
	int32 FirstOpenWindow() const;
	float GetTrueMiss() const;

	// ---- server-only state (never replicated: clients must not know the answers) ----
	UPROPERTY(Transient) TObjectPtr<UMCTransitConfig> DefaultConfig;
	UPROPERTY(Transient) TObjectPtr<AMCConstructionManager> ConsMgr;
	FRandomStream Stream;
	FMCLoadout Loadout;
	bool bHaveLoadout = false;
	bool bBound = false;
	bool bAnomaly = false;
	float StageClock = 0.f;      // seconds since the stage started
	float RunClock = 0.f;        // seconds since BeginTransit (deterministic wobble)
	float CoastClock = 0.f;      // seconds since the coast started
	float StringTimer = 0.f;
	float PollTimer = 0.f;
	float FuelReserveKg = 0.f;
	float ErrX = 0.f, ErrY = 0.f;           // true predicted miss (km)
	float StaleX = 0.f, StaleY = 0.f;       // what the display keeps showing while stale
	float AnomalyRoll = 0.f;
	float AnomalyDirX = 0.f, AnomalyDirY = 0.f;
	float InitErrX = 0.f, InitErrY = 0.f;
	float FaultTime = 0.f, KickX = 0.f, KickY = 0.f;
	bool bFaultHappened = false;
	bool bFaultFixed = false;
	TArray<float> DriftX, DriftY;           // per segment (size 5): total drift in km
	TArray<float> SegmentStart;             // segment boundaries (coast seconds), size 6
	// alarms
	TArray<float> AlarmTime;                // per alarm, coast seconds
	TArray<int32> AlarmSwitch;              // switch to flip
	TArray<bool> AlarmWantOn;
	// equipment: button position -> step index
	TArray<TArray<int32>> DevicePerm;
	TArray<float> DeviceTestClock;
	TArray<float> WindowOpenAt;             // server copy
	TArray<float> InstWobblePhase;
	TArray<float> InstWobbleFreq;
	bool bDebugBot = false;
	float BotTimer = 0.f;
	int32 DebugPulseWindow = -1;
};
