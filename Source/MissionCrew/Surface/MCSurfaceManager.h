// S4 Surface Exploration & Ice Detection: server-authoritative rules. Clients hold no logic: they read the replicated values below
// and send action codes (role*100+action) / drive input through their AMCSurfaceLink. See Vault/Status.md "S4 Surface".
// Place one in the surface level (its location is the origin of the field); the rover and the per-player links are spawned at runtime.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Surface/MCSurfaceTypes.h"
#include "MCSurfaceManager.generated.h"

class AMCSurfaceRover;
class AMCSurfaceLink;
class APlayerController;

UCLASS(Blueprintable)
class MISSIONCREW_API AMCSurfaceManager : public AActor
{
	GENERATED_BODY()
public:
	AMCSurfaceManager();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------------- Config ----------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TObjectPtr<UMCSurfaceConfig> Config;
	/** Every role may send every action (solo testing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Config") bool bFreeRoles = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") int32 SeedOverride = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bAutoStart = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bDebugAutoTest = false;
	/** Local players look through the rover chase camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bChaseCamera = true;
	/** Apply the crew roles recorded by earlier phases (retries once a second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bApplyRoles = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float RoleRetrySec = 30.f;
	/** Draw rocks and sites as debug shapes in the world (no models are created). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bDrawMarkers = true;
	/** After success / timeout ServerTravel to NextMap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bTravelOnComplete = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") FString NextMap = TEXT("/Game/MissionCrew/Maps/L_Samples");

	// ---------------- Replicated state ----------------
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bStarted = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bDone = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 Seed = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") float ElapsedSec = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") float TimeoutSec = 600.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") FString Message;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Field") TArray<FMCSurfSite> Sites;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Field") TArray<FVector2D> RockPos;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Field") TArray<float> RockRadius;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Field") float FieldHalfSize = 6500.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Field") float SiteRadius = 500.f;
	/** Science Analyst pick (-1 = none). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Field") int32 ChosenSite = -1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rover") TObjectPtr<AMCSurfaceRover> Rover;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rover") FVector2D RoverPos = FVector2D::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rover") float RoverYaw = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rover") int32 RockHits = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rover") float RoverSpeedNow = 0.f;
	/** Index of the site the rover is inside (-1 = none). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rover") int32 RoverAtSite = -1;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") float BatteryWh = 100.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") float CapacityWh = 100.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") float SolarNowW = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") float SupplyW = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") float DemandW = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") bool bCommsOn = true;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") bool bInstrumentsOn = true;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") int32 DrillPowerLevel = 2;
	/** Non-drill demand exceeds supply: rover crawls, instruments unusable. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") bool bBrownout = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") bool bPowerEvent = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") float EventLeftSec = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Power") float PowerUsedWh = 0.f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Drill") int32 DepthLevel = 1;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Drill") bool bDrilling = false;
	/** The drill wants to run but the supply is too low. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Drill") bool bDrillPaused = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Drill") float DrillProgress = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Drill") bool bSampleReady = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Drill") TArray<FMCSurfSample> Samples;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") FMCSurfReport Result;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") TArray<int32> RoleGood; // size 5, index 1..4
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Result") TArray<int32> RoleBad;

	// ---------------- Events ----------------
	/** Server only. Fires once with the final report. */
	UPROPERTY(BlueprintAssignable, Category = "Events") FMCSurfaceCompleteSignature OnComplete;

	// ---------------- API ----------------
	/** Server: draw the seeded field, spawn the rover. Safe to call twice. */
	UFUNCTION(BlueprintCallable, Category = "Surface") void BeginSurface();
	/** Server: every action. Code = Role*100+Action, Who = sender CrewRole (accepted when bFreeRoles or Who == role). */
	UFUNCTION(BlueprintCallable, Category = "Surface") bool HandleAction(int32 Code, int32 Who);
	/** Server: Rover Driver input (-1..1). */
	UFUNCTION(BlueprintCallable, Category = "Surface") bool HandleDrive(float InThrottle, float InSteer, int32 Who);
	/** Scripted run of the whole phase with a bot; prints "DEBUGTEST" lines. Server only. */
	UFUNCTION(BlueprintCallable, Category = "Surface") void RunDebugTest();
	UFUNCTION(BlueprintPure, Category = "Surface") FVector SiteWorldLocation(int32 Index) const;

protected:
	const UMCSurfaceConfig& Cfg() const;
	void InitField();
	void Say(const FString& Text);
	void AddScore(int32 RoleIdx, bool bGood);
	void PowerTick(float Dt);
	void RoverTick(float Dt);
	void DrillTick(float Dt);
	void EnsureLinks();
	void ApplyRolesTick();
	void Finish(bool bTimeout);
	void DoTravel();
	void RefreshReadings(int32 Site);
	void DrawMarkers() const;
	FVector ToWorld(const FVector2D& P) const;
	void DebugBotTick(float Dt);
	void DebugLog(const FString& Text) const;

	UPROPERTY(Transient) TObjectPtr<UMCSurfaceConfig> DefaultConfig;
	UPROPERTY(Transient) TMap<TObjectPtr<APlayerController>, TObjectPtr<AMCSurfaceLink>> Links;
	FRandomStream Stream;
	FTimerHandle RoleTimer, TravelTimer;
	float RoleElapsed = 0.f;
	float LinkTimer = 0.f;

	// server-only truth
	TArray<float> IceFrac;       // per site 0..1
	TArray<int32> IceDepth;      // per site 1..3
	TArray<float> NoiseA;        // per site fixed offsets so scans converge
	TArray<float> NoiseB;
	float Throttle = 0.f, Steer = 0.f;
	float EventAt = 0.f;
	bool bEventDone = false;
	float EventClock = 0.f;
	float HitCooldown = 0.f;
	float BrownoutSec = 0.f;
	bool bEventHandled = false;
	void Recalc();
	bool bWasBrownout = false;
	int32 DrillSite = -1;
	float FinishClock = -1.f;
	bool bDebugBot = false;
	float BotClock = 0.f;
	int32 BotStep = 0;
	float BotWait = 0.f;
};
