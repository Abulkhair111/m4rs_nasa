// S6 Return to Earth & Debrief: server-authoritative rules (pattern of AMCTransitManager).
// Place ONE AMCReturnManager in the return map (or any map; it starts by itself after StartDelaySec). It also spawns, on every
// client, the full-screen HUD (UMCReturnHudWidget: only the local role's panel), the 3D chase view (AMCReturnView) and, on the
// server, one AMCReturnLink per player controller (the owned actor that carries the Server RPC; this actor has no owner).
//
// Action code = Role*100 + Action (see MCReturnTypes.h). Roles: 1 Systems, 2 Data, 3 Pilot, 4 Recovery.
// Timeline: Cruise -> Reentry (radio BLACKOUT in its first BlackoutSec) -> Landing -> Complete (summary on every client).
// Reentry blackout event: a report that is NOT queued when the reentry starts is lost; a queued one is sent when the link returns.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Return/MCReturnTypes.h"
#include "Return/MCMissionSummary.h"
#include "MCReturnManager.generated.h"

class UMCReturnHudWidget;
class AMCReturnView;
class AMCReturnManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMCReturnCompleteSignature, int32, Score);

/** One per player controller (server spawns it, owner = the controller). Carries the client -> server action RPC. */
UCLASS()
class MISSIONCREW_API AMCReturnLink : public AActor
{
	GENERATED_BODY()
public:
	AMCReturnLink();
	UFUNCTION(Server, Reliable) void ServerAction(int32 Code);
	/** The link owned by this controller (client: replicated copy). */
	static AMCReturnLink* FindFor(UWorld* World, const APlayerController* PC);
};

UCLASS(Blueprintable)
class MISSIONCREW_API AMCReturnManager : public AActor
{
	GENERATED_BODY()
public:
	AMCReturnManager();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------------- Config ----------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TObjectPtr<UMCReturnConfig> Config;
	/** Every role may send every action (solo testing); the HUD then shows role tabs for an unassigned player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Config") bool bFreeRoles = false;
	/** Start by itself StartDelaySec after BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bAutoStart = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float StartDelaySec = 4.f;
	/** 0 = mission seed from the mission subsystem, else random; otherwise this fixed seed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") int32 SeedOverride = 0;
	/** Time multiplier (debug). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float TimeScale = 1.f;
	/** Create the HUD / chase view on the local machine. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bCreateLocalUI = true;
	/** Run the scripted bot as soon as the phase starts; implies bFreeRoles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bDebugAutoTest = false;
	/** SERVER: re-apply recorded crew roles (UMCMissionSubsystem) for this long after BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float RoleRetrySec = 30.f;

	// ---------------- Replicated state ----------------
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") EMCReturnStage Stage = EMCReturnStage::Cruise;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bStarted = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bDone = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bBlackout = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 Seed = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") float ElapsedSec = 0.f;
	/** Whole timeline 0..1 (for the view). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") float Progress = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 Score = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") TArray<int32> RoleGood;  // size 5
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") TArray<int32> RoleBad;   // size 5
	/** Report: 0 draft, 1 queued, 2 sent, 3 lost. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 ReportState = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bBeaconOn = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bChuteOpen = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bLanded = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") float Health = 100.f;
	/** TRUE miss values for the 3D view (visual only; the Pilot's readout is the stale-able one in PanelText[3]). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Flight") float ShipAngleErr = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Flight") float ShipCrossKm = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Flight") float LandMissKm = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Flight") int32 PulseSeq = 0;
	/** Systems: switch s has an open warning. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Systems") TArray<bool> SwitchFault;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Text") FString TopText;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Text") FString BoardText;
	/** Size 5: index 1..4 = role panel body text. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Text") TArray<FString> PanelText;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Text") FString Message;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") FString LessonText;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") FMCSummaryData Summary;

	/** Server only: fires once when S6 ends (hand-off to the summary). */
	UPROPERTY(BlueprintAssignable, Category = "Events") FMCReturnCompleteSignature OnComplete;

	// ---------------- API ----------------
	/** Server: draw the seeded events and start the timeline. Ignored if already started. */
	UFUNCTION(BlueprintCallable, Category = "Return") void BeginReturn();
	/** Server: single entry point for player actions (Code = Role*100+Action, Who = sender's CrewRole 0..4). True if accepted. */
	UFUNCTION(BlueprintCallable, Category = "Return") bool HandleAction(int32 Code, int32 Who);
	/** Server: scripted run of the whole phase with a bot; "DEBUGTEST" lines in the log. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Return") void RunDebugTest();
	UFUNCTION(BlueprintPure, Category = "Return") static int32 GetCrewRoleOf(const APlayerController* PC);
	UFUNCTION(BlueprintPure, Category = "Return") static AMCReturnManager* Find(const UObject* WorldContext);

protected:
	const UMCReturnConfig& Cfg() const;
	void SetupLocalUI();
	void ServerHousekeeping(float Dt);
	void Say(const FString& Text);
	void Mark(int32 Role, bool bGood, const FString& Note);
	void EnterStage(EMCReturnStage S);
	void Finish();
	void RebuildStrings();
	void PilotTick(float Dt);
	void SystemsTick();
	void BotTick();
	void DebugLog(const FString& Text) const;
	void Touchdown();

	bool DoSystems(int32 Action);
	bool DoData(int32 Action);
	bool DoPilot(int32 Action);
	bool DoRecovery(int32 Action);

	UPROPERTY(Transient) TObjectPtr<UMCReturnConfig> DefaultConfig;
	UPROPERTY(Transient) TObjectPtr<UMCReturnHudWidget> Hud;
	UPROPERTY(Transient) TObjectPtr<AMCReturnView> View;

	// ---- server-only (clients must not know the answers) ----
	FRandomStream Stream;
	float RunClock = 0.f, StageClock = 0.f, StartTimer = 0.f, StringTimer = 0.f, LinkTimer = 0.f, RoleTimer = 0.f, LocalTimer = 0.f;
	float RoleElapsed = 0.f;
	bool bRolesDone = false;
	// systems
	TArray<float> AlarmAt;
	TArray<int32> AlarmSwitchIdx;
	TArray<int32> AlarmState;      // 0 pending, 1 open, 2 fixed, 3 missed
	TArray<float> AlarmOpenedAt;
	int32 WarningsFixed = 0, WarningsMissed = 0;
	// data
	int32 BadSet = 0;
	bool bDataChecked = false;
	bool bSetFixed[3] = { false, false, false };
	bool bReportComplete = false;
	// recovery
	int32 FaultItem = 0;
	bool bItemChecked[3] = { false, false, false };
	bool bFaultSecured = false, bEquipPassed = false, bPrepared = false, bHandedOver = false, bBeaconWindowClosed = false, bBeaconInWindow = false;
	float LandedAt = -1.f;
	// pilot
	float AngleErr = 0.f, CrossKm = 0.f, BiasAngle = 1.f, BiasCross = 1.f, PhaseA = 0.f, PhaseB = 0.f;
	float StaleAngle = 0.f, StaleCross = 0.f;
	float CorridorTime = 0.f, ReentryTime = 0.f;
	bool bInZone = false, bPilotLanded = false;
	// bot
	bool bBot = false;
	float BotTimer = 0.f;
	FString LessonKeep;
};
