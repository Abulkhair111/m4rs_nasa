// S3 Orbit & Landing: server-authoritative rules (pattern of AMCTransitManager). Clients hold NO logic: they read the
// replicated values below and send action codes (Role*100+Action) through their own AMCOrbitRelay (spawned by this
// manager, owned by the player controller) which calls HandleAction(Code, CrewRole) on the server.
// Each client builds only its own role panel (UMCOrbitHudWidget). Hidden truths (real hazards, true ice, safe zone) stay server-only.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Orbit/MCOrbitTypes.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "MCOrbitManager.generated.h"

class AMCOrbitRelay;
class UMCOrbitHudWidget;
class APlayerController;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCOrbitManager : public AActor
{
	GENERATED_BODY()

public:
	AMCOrbitManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------------- Config ----------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TObjectPtr<UMCOrbitConfig> Config;
	/** Every role may send every action (solo testing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Config") bool bFreeRoles = false;
	/** 0 = random seed; otherwise the fixed seed (same seed = same events). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") int32 SeedOverride = 0;
	/** Start by itself in BeginPlay (place the manager in the S3 map). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bAutoStart = true;
	/** Run the scripted bot (RunDebugTest) right after the start; implies bFreeRoles and no map travel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bDebugAutoTest = false;
	/** Map to ServerTravel to when the report has been shown. Empty = no travel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") FString NextMapName = TEXT("/Game/MissionCrew/Maps/L_Surface");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bTravelOnComplete = true;
	/** SERVER: re-apply the recorded crew roles (UMCMissionSubsystem) after the map change, for this long (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float RoleApplySec = 30.f;
	/** Optional Blueprint child of the HUD widget. Null = UMCOrbitHudWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TSubclassOf<UMCOrbitHudWidget> HudClass;
	/** Create the HUD and show the mouse cursor on the local player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bCreateHud = true;

	// ---------------- Replicated state ----------------
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") EMCOrbitStage Stage = EMCOrbitStage::GoPoll;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bStarted = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bDone = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 Seed = 0;
	/** Server world time when the stage ends (0 = open-ended). Use GetStageTimeLeft(). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") double StageEndTime = 0.0;
	/** Seconds since BeginOrbit (drives the orbit animation of the view), ~5x per second. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") float ElapsedSec = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") TArray<bool> GoFlags;   // 1..4
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") TArray<bool> NoGoFlags; // 1..4

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Ship") float Health = 100.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Ship") float FuelKg = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Ship") float FuelStartKg = 0.f;

	// Sites / hazards / marks
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Sites") TArray<FMCOrbitSite> Sites;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Sites") TArray<FMCOrbitHazard> Hazards;
	/** Proposed by the Analyst (-1 none), the chosen landing site (-1 until lock), the site the Safety Officer looks at. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Sites") int32 ProposedSite = -1;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Sites") int32 ChosenSite = -1;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Sites") int32 SafetyViewSite = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Sites") bool bSiteConfirmed = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Sites") bool bSiteRejected = false;

	// Systems
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Systems") TArray<FMCOrbitRail> Rails;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Systems") bool bLimitsReported = false;

	// Descent (metres; frame: site centre = origin, +X east, +Y north)
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float AltM = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float VertSpeed = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float PosX = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float PosY = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float VelX = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float VelY = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float Throttle = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float TargetX = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float TargetY = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float SafeRadiusM = 35.f;
	/** False while the radar rail is off: the Pilot altitude/speed readout is invalid. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") bool bAltValid = true;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") bool bLegsDeployed = false;
	/** Hazard surprise: boulder field at the site centre (radius BoulderRadiusM). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") bool bBoulderField = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") float BoulderRadiusM = 60.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") int32 SafetyZoneClear = -1;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") int32 DivertZone = -1;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Descent") bool bTouchedDown = false;

	// Text built on the server (the HUD only shows them). PanelText index 1..4 = role panel, 0 = shared line.
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Text") FString TopText;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Text") FString BoardText;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Text") TArray<FString> PanelText;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Message, Category = "Text") FString Message;

	// Results
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") int32 OrbitScore = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") FMCOrbitReport Result;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") TArray<int32> RoleGood;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") TArray<int32> RoleBad;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") FString ReportText;

	// ---------------- Events ----------------
	/** Server only. Fires once with the final report, just before the travel to NextMapName. */
	UPROPERTY(BlueprintAssignable, Category = "Events") FMCOrbitCompleteSignature OnComplete;
	UFUNCTION(BlueprintImplementableEvent, Category = "Events") void BP_OnMessage(const FString& Text);

	// ---------------- API ----------------
	/** Server: read S1/S2 results from UMCMissionSubsystem, draw all random values, open the GO poll. Safe to call twice. */
	UFUNCTION(BlueprintCallable, Category = "Orbit") void BeginOrbit();
	/**
	 * Server: the single entry point for every player action. Code = Role*100+Action (90 GO, 91 NO-GO). Accepted when bFreeRoles
	 * or Who == the role in the code. Returns true if accepted (even when it scored bad).
	 */
	UFUNCTION(BlueprintCallable, Category = "Orbit") bool HandleAction(int32 Code, int32 Who);
	/** Scripted run of the whole phase with a bot; prints "DEBUGTEST" lines to the log. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Orbit") void RunDebugTest();
	UFUNCTION(BlueprintPure, Category = "Orbit") float GetStageTimeLeft() const;
	UFUNCTION(BlueprintPure, Category = "Orbit") static int32 GetCrewRoleOfController(const AController* Controller);
	static const TCHAR* StageName(EMCOrbitStage S);
	/** The relay owned by this local controller (client side lookup), or null. */
	static AMCOrbitRelay* FindRelayFor(UWorld* World, const AController* Controller);
	static void SendAction(APlayerController* PC, int32 Code);

	/** Effective touchdown limits as the Pilot would be told (server truth). */
	double NowSec() const { const AGameStateBase* G = GetWorld() ? GetWorld()->GetGameState() : nullptr; return G ? G->GetServerWorldTimeSeconds() : (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0); }
	float VertLimit() const;
	float HorizLimit() const;

protected:
	UFUNCTION() void OnRep_Message();

	const UMCOrbitConfig& Cfg() const;
	void SetStage(EMCOrbitStage NewStage, float DurationSec);
	void Say(const FString& Text);
	void Score(int32 Role, bool bGood);
	bool RailOn(int32 Index) const;

	void StageTick(float Dt);
	void DescentTick(float Dt);
	void StartDescent();
	void Touchdown(bool bTimeout);
	void FinishReport();
	void RebuildStrings();
	void EnsureRelays();
	void EnsureHud();
	void RoleApplyTick();
	void DebugBotTick(float Dt);
	void DebugLog(const FString& Text) const;
	void LockSite();
	void BuildFinalScores();

	bool DoGo(int32 Who, bool bGo);
	bool DoSystems(int32 Action);
	bool DoSafety(int32 Action);
	bool DoPilot(int32 Action);
	bool DoAnalyst(int32 Action);
	void SetThrottle(float T);
	void Nudge(float Dx, float Dy);

	int32 HazardIndex(int32 Site, int32 Slot) const { return Site * MCOrbit::HazardsPerSite + Slot; }
	static FString CompassOf(float Dx, float Dy);
	FString RailListText() const;

	// ---- server-only state (never replicated: clients must not know the answers) ----
	UPROPERTY(Transient) TObjectPtr<UMCOrbitConfig> DefaultConfig;
	UPROPERTY(Transient) TObjectPtr<UMCOrbitHudWidget> Hud;
	FRandomStream Stream;
	bool bBegun = false;
	int32 MarkScored = 0;
	bool bTraveled = false;
	float StageClock = 0.f, RunClock = 0.f, StringTimer = 0.f, RelayTimer = 0.f, RoleTimer = 0.f, HudTimer = 0.f;
	float RoleApplyElapsed = 0.f;
	float PollHoldUntil = 0.f;
	// truth
	TArray<float> IceTrue;
	TArray<int32> TrueRank;            // site -> rank 1..n (1 = best ice)
	TArray<bool> HazardReal;
	TArray<int32> IssueRole;           // GO poll: 1 if role has a hidden issue
	TArray<FString> IssueText;
	TArray<bool> IssueCleared;
	bool bLegsOkS2 = true;
	float LimitVert = 3.f, LimitHoriz = 1.5f;
	int32 SafeZone = 0;
	TArray<float> ZoneDensity, ZoneIce;
	float EventTime = 7.f, RailTripTime = 15.f;
	bool bEventFired = false, bRailTripFired = false;
	int32 TrippedRail = -1;
	float DescentClock = 0.f;
	int32 Approach = 0;                // 0 nominal, 1 steep, 2 shallow
	bool bInsertionDone = false;
	bool bInsertionAuto = false;
	int32 RailsOnAtDescent = 0;
	bool bTripFixed = false;
	bool bEventResolved = false;
	bool bPilotIssue = false;
	float InitialFuel = 0.f;
	bool bHasIceInst = false;
	float CalibFrac = 0.5f;
	float MissKmS2 = 0.f;
	bool bInCorridorS2 = true;
	FString MsgLatest;
	// debug bot
	bool bDebugBot = false;
	float BotTimer = 0.f;
	int32 BotStep = 0;
};
