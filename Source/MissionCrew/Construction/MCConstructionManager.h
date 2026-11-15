#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCConstructionTypes.h"
#include "MCConstructionManager.generated.h"

class AMCPartSocket;

/** Server-side: a placement was approved; a mover (agent F) should animate the part and call FinishPlacement. */
DECLARE_MULTICAST_DELEGATE_FourParams(FMCPlacementApproved, AMCPartSocket* /*Socket*/, UMCPartDef* /*Def*/, EMCCrewRole /*InRole*/, bool /*bHeavy*/);

/**
 * C++ port of BP_ConstructionManager. One per bay, replicated, always relevant. Single source of truth for the
 * S1 loadout. Clients only read the replicated values. This actor is NOT owned by any client, so clients cannot
 * call Server RPCs on it: the pawn / controller exposes the RPCs (ServerPlacePart, ServerApplyTool, ...) and calls
 * the server-only functions below.
 */
UCLASS(Blueprintable)
class MISSIONCREW_API AMCConstructionManager : public AActor
{
	GENERATED_BODY()

public:
	AMCConstructionManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------------- Config (set in the Blueprint child / defaults) ----------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float LanderBaseMassKg = 1200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float LanderBaseCostM = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float BudgetM = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") float PhaseDurationSec = 600.f;
	/** InRole 0 / free-for-all (solo testing): every role may do everything. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bFreeRoles = true;
	/** Server: StartPhase() on BeginPlay when the map name contains "AssemblyBay". Calling StartPhase again is harmless. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") bool bAutoStart = true;
	/** After a successful launch: travel to the flight deck map (L_Transit). The S1 result goes through UMCMissionSubsystem. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Flow") bool bTravelToTransit = true;
	/** Seconds between the launch ("Launching...") and the ServerTravel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Flow") float TravelDelaySec = 4.f;
	/** Package path of the deck map. Travel is skipped when this is the current map. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Flow") FString TransitMapName = TEXT("/Game/MissionCrew/Maps/L_Transit");
	/** Rocket options (levers). Values are placeholders until the design doc fixes them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TArray<FMCRocketDef> Rockets;
	/** All selectable part definitions (tools excluded automatically by ToolType). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TArray<TObjectPtr<UMCPartDef>> Catalog;

	// ---------------- Replicated state ----------------
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bActive = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") bool bLaunched = false;
	/** Server world time when the phase ends; use GetTimeLeft(). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "State") double PhaseEndTime = 0.0;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rocket") FName RocketName;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rocket") float RocketPayloadKg = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rocket") float RocketCostM = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rocket") float RocketReliability = 1.f;
	/** -1 = none chosen. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rocket") int32 SelectedLever = -1;
	/** Name of the chosen launcher (== Rockets[SelectedLever].Name, NAME_None if none). AMCRocketStack picks its preset by this. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Rocket") FName SelectedLauncher;
	/** Visual presets per launcher (data assets; matched to Rockets by LauncherName). Filled in the manager Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config") TArray<TObjectPtr<UMCRocketPreset>> LauncherPresets;
	/** The preset of SelectedLauncher, or null. Works on every machine. */
	UFUNCTION(BlueprintPure, Category = "Rocket") UMCRocketPreset* GetSelectedPreset() const;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float FuelKg = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float FuelMaxKg = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float DryMassKg = 0.f;
	/** 0.9 x dry mass. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float FuelRequiredKg = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float TotalMassKg = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float TotalCostM = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float PowerGenW = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float PowerUseW = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float StorageWh = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float DataMbps = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float BandwidthMbps = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float ScienceValue = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float Reliability = 1.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float ThrustN = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") float TWR = 0.f;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") int32 IceCount = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") int32 UnboltedCount = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") int32 OffGridCount = 0;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") bool bHasEngine = false;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") bool bHasLidar = false;
	/** Solid boosters installed (ascent group). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") int32 BoosterCount = 0;
	/**
	 * Group sub-totals (two stacks, see Vault/Decisions.md 2026-11-14). The flat fields above stay as whole-vehicle values:
	 * TotalMassKg = Lander.MassKg + Ascent.MassKg = take-off mass; Storage, Data, Bandwidth, Science and IceCount come from
	 * the LANDER group (power also counts ascent consumers); ThrustN, FuelMaxKg, TWR come from the ASCENT group.
	 */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") FMCGroupTotals LanderTotals;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Totals") FMCGroupTotals AscentTotals;

	/** Size MCConst::NumChecks, indexed by EMCCheck. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Checks") TArray<EMCCheckState> Checks;
	/** Size NumRoles; index 1..4 = departments. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Checks") TArray<bool> DeptReady;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Checks") TArray<int32> RoleGood;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Checks") TArray<int32> RoleBad;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Checks") FString Report;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Message, Category = "Checks") FString Message;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Checks") double MessageTime = 0.0;

	// ---------------- Events ----------------
	/** Server only. Bind in the game mode to call ConstructionFinished. */
	UPROPERTY(BlueprintAssignable, Category = "Events") FMCLaunchCompleteSignature OnLaunchComplete;
	/** Server only, native. Bound by the part mover (agent F). If unbound, PlacePart installs instantly. */
	FMCPlacementApproved OnPlacementApproved;

	UFUNCTION(BlueprintImplementableEvent, Category = "Events") void BP_OnMessage(const FString& Text);

	// ---------------- Queries (any machine) ----------------
	UFUNCTION(BlueprintPure, Category = "Construction") float GetTimeLeft() const;
	UFUNCTION(BlueprintPure, Category = "Construction") EMCCheckState GetCheck(EMCCheck Index) const;
	/** Catalog defs that fit a socket type AND group for this role (tools excluded). */
	UFUNCTION(BlueprintCallable, Category = "Construction") TArray<UMCPartDef*> GetCandidates(EMCSocketType SocketType, EMCCrewRole InRole, EMCStackGroup Group) const;
	/** Predicted totals if Def were installed, bolted and wired. Uses the replicated totals, so it works on clients. */
	UFUNCTION(BlueprintCallable, Category = "Construction") FMCPreviewResult Preview(const UMCPartDef* Def) const;
	/** Agent I2: like Preview, but NewDef replaces OldDef (the installed part of the socket), so the old part is subtracted. OldDef may be null. */
	UFUNCTION(BlueprintCallable, Category = "Construction") FMCPreviewResult PreviewSwap(const UMCPartDef* NewDef, const UMCPartDef* OldDef) const;
	UFUNCTION(BlueprintPure, Category = "Construction") bool RoleAllowed(EMCCrewRole InRole, int32 RequiredDepartment) const;

	// ---------------- Server API (call from pawn/controller RPCs; all guard HasAuthority) ----------------
	UFUNCTION(BlueprintCallable, Category = "Construction") void StartPhase();
	UFUNCTION(BlueprintCallable, Category = "Construction") void SelectRocket(int32 LeverIndex);
	UFUNCTION(BlueprintCallable, Category = "Construction") void AddFuel(float DeltaKg);
	UFUNCTION(BlueprintCallable, Category = "Construction") void ToggleReady(int32 Department);
	/** Validates and reserves; the mover then calls FinishPlacement. Returns true if accepted. */
	UFUNCTION(BlueprintCallable, Category = "Construction") bool PlacePart(AMCPartSocket* Socket, UMCPartDef* Def, EMCCrewRole InRole);
	/** Called by the mover on arrival (or instantly if no mover is bound). */
	UFUNCTION(BlueprintCallable, Category = "Construction") void FinishPlacement(AMCPartSocket* Socket, UMCPartDef* Def, AActor* PartActor, EMCCrewRole InRole);
	/** Removes an unbolted part. Caller destroys the visual actor. */
	UFUNCTION(BlueprintCallable, Category = "Construction") bool RemovePart(AMCPartSocket* Socket, EMCCrewRole InRole);
	/** Wrench (bolt) or cable (grid) on a socket. */
	UFUNCTION(BlueprintCallable, Category = "Construction") bool ApplyTool(AMCPartSocket* Target, EMCToolType Tool, EMCCrewRole InRole);
	/** All four ready, no red; yellow needs a second press. Broadcasts OnLaunchComplete on success. */
	UFUNCTION(BlueprintCallable, Category = "Construction") bool TryLaunch();
	UFUNCTION(BlueprintCallable, Category = "Construction") void RunPreflight();
	UFUNCTION(BlueprintCallable, Category = "Construction") void Recalc();
	UFUNCTION(BlueprintCallable, Category = "Construction") FMCLoadout BuildLoadout() const;
	UFUNCTION(BlueprintCallable, Category = "Construction") void Score(EMCCrewRole InRole, bool bGood);
	UFUNCTION(BlueprintCallable, Category = "Construction") void Say(const FString& Text);

	UFUNCTION(BlueprintCallable, Category = "Construction") void RegisterSocket(AMCPartSocket* Socket);

protected:
	UFUNCTION() void OnRep_Message();

	void DiscoverSockets();
	void RecalcChecks();
	void BuildReport();
	void UpdateAlerts();
	void OnSocketChanged();
	void SetRocketFields(int32 Index);
	void TravelToTransit();
	FTimerHandle TravelTimer;

	UPROPERTY() TArray<TObjectPtr<AMCPartSocket>> Sockets;
	bool bYellowWarned = false;
	bool bPreflightRan = false;
};
