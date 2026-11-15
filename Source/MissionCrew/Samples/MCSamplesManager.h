// S5 Sample Preservation & Return Preparation: server-authoritative rules (C++). Pattern of AMCTransitManager.
// Clients hold no logic: they read the replicated state below; buttons go through AMCSamplesRelay -> HandleAction(Code, Role).
// Place one in the S5 map (default: starts by itself ~1 s after BeginPlay, reading the sample list S4 left in UMCMissionSubsystem).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Samples/MCSamplesTypes.h"
#include "MCSamplesManager.generated.h"

class AMCSamplesRelay;
class UUserWidget;
class AController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMCSamplesCompleteSignature);

UCLASS(Blueprintable)
class MISSIONCREW_API AMCSamplesManager : public AActor
{
	GENERATED_BODY()
public:
	AMCSamplesManager();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float Dt) override;

	// ---- Config ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TObjectPtr<UMCSamplesConfig> Config;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Config") bool bFreeRoles = false;
	/** 0 = random; otherwise fixed seed (same seed = same samples, leaks and event time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") int32 SeedOverride = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bAutoStart = true;
	/** Run RunDebugTest right after the start (implies bFreeRoles, no travel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bDebugAutoTest = false;
	/** Force the sample count (0 = from UMCMissionSubsystem, else config default 3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") int32 SampleCountOverride = 0;
	/** Cooler fault (active container) / hold-time warning (passive container) at a seeded time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bEnableEvent = true;
	/** ServerTravel here when the phase is complete. Empty or missing package = stay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") FString ReturnMapPath = TEXT("/Game/MissionCrew/Maps/L_Return");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bTravelOnComplete = true;
	/** Client: create the role panel automatically and show the mouse cursor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bCreatePanel = true;
	/** Optional Blueprint child of the panel. Null = UMCSamplesPanel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TSubclassOf<UUserWidget> PanelClass;

	// ---- Replicated state ----
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bStarted = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bDone = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 Seed = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") float ElapsedSec = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") TArray<FMCSample> Samples;
	/** Scientist's current (unlabelled) selection for the next sample, -1 = none. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 PendType = -1;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 PendCond = -1;
	/** Handoff queue sizes: waiting for Scientist / Technician / Cargo. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 WaitSci = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 WaitTech = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 WaitCargo = 0;
	/** Per role (index 1..4, size 5): the routine check is available. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") TArray<bool> RoutineReady;

	// Container / thermal / board
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") int32 ContainerIdx = -1;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") TArray<FString> ContainerNames;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") TArray<FString> ModeNames;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") int32 CoolMode = 2;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") float TempC = -30.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") float LimitC = 4.f;
	/** Seconds until TempC crosses LimitC with the current cooling/leak (999 = never). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") float HoldLeftSec = 999.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") float MassKg = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") float PowerW = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") float PowerBudgetW = 100.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Thermal") float InRangeFrac = 1.f;

	// Event
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Event") EMCSamplesEvent EventKind = EMCSamplesEvent::None;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Event") bool bEventActive = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Event") bool bEventFixed = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Event") bool bStorageUpdated = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Message, Category = "Text") FString Message;

	// Result
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") int32 Score = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") int32 SamplesOk = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") TArray<int32> RoleGood;  // size 5
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") TArray<int32> RoleBad;   // size 5
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") FString ReportText;

	/** Server: fires once when the phase is complete (before the optional travel). */
	UPROPERTY(BlueprintAssignable, Category = "Events") FMCSamplesCompleteSignature OnComplete;

	// ---- API ----
	/** Server: read the sample list, draw random values, open the pipeline. Ignored when already started. */
	UFUNCTION(BlueprintCallable, Category = "Samples") void BeginSamples();
	/** Server: THE entry point for player actions (Code = Role*100+Action). True if accepted (even if scored bad). */
	UFUNCTION(BlueprintCallable, Category = "Samples") bool HandleAction(int32 Code, int32 Who);
	/** Server: scripted bot plays the whole phase and logs "DEBUGTEST" lines. */
	UFUNCTION(BlueprintCallable, Category = "Samples") void RunDebugTest();

	UFUNCTION(BlueprintPure, Category = "Samples") int32 CountSecured() const;
	UFUNCTION(BlueprintPure, Category = "Samples") static FString StorageName(int32 C);
	UFUNCTION(BlueprintPure, Category = "Samples") static FString TypeName(int32 T);

protected:
	UFUNCTION() void OnRep_Message();
	const UMCSamplesConfig& Cfg() const;
	void Say(const FString& T);
	void Award(int32 Role, bool bGood);
	FMCSample* FirstAt(EMCSampleStage S);
	bool HasWork(int32 Role) const;
	void SimTick(float Dt);
	void UpdateLimit();
	void UpdateBoard();
	void EventTick();
	void Complete(bool bTimedOut);
	void SpawnRelays();
	void RoleApplyTick();
	void ClientPanelTick();
	void DebugBot();
	void DebugLog(const FString& T) const;
	float CoolRateNow() const;
	float LeakNow() const;
	const FMCSamplesContainer* Cont() const;

	bool DoSystems(int32 A);
	bool DoScientist(int32 A);
	bool DoTech(int32 A);
	bool DoCargo(int32 A);

	UPROPERTY(Transient) TObjectPtr<UMCSamplesConfig> DefaultConfig;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> Panel;
	UPROPERTY(Transient) TMap<TObjectPtr<AController>, TObjectPtr<AMCSamplesRelay>> Relays;

	// server-only truth (clients must not know the answers)
	FRandomStream Stream;
	TArray<uint8> TrueType;
	TArray<bool> LeakRoll;
	float ClockSec = 0.f;
	float EventAt = 50.f, EventStart = 0.f;
	float InRangeSec = 0.f, TotalSec = 0.f;
	float SampleClock = 0.f;
	float RoutineCD[5] = { 0, 0, 0, 0, 0 };
	float IdleClock[5] = { 0, 0, 0, 0, 0 };
	float IdleMax[5] = { 0, 0, 0, 0, 0 };
	bool bEventResponded = false, bEventFired = false, bInsulated = false, bContainerScored = false;
	float RoleApplyElapsed = 0.f;
	float RelayTimer = 0.f;
	float DoneTime = 0.f;
	bool bTravelled = false;
	bool bDebugBot = false;
	float BotTimer = 0.f;
};
