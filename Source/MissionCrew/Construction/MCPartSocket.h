#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCConstructionTypes.h"
#include "MCPartSocket.generated.h"

DECLARE_MULTICAST_DELEGATE(FMCSocketChanged);

/**
 * C++ replacement for BP_PartSocket. Holds only state; visuals (ghost, alert marker, the installed part's
 * mesh actor) belong to Blueprint children / agent F's part actors via the BlueprintImplementableEvents below.
 * All mutation is server-only; the manager listens to OnChanged (server) and recalculates.
 */
UCLASS(Blueprintable)
class MISSIONCREW_API AMCPartSocket : public AActor
{
	GENERATED_BODY()

public:
	AMCPartSocket();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Socket")
	EMCSocketType SocketType = EMCSocketType::Power;

	/** Which half of the vehicle this socket is on. Only parts with the same UMCPartDef::Group install here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Socket")
	EMCStackGroup SocketGroup = EMCStackGroup::Lander;

	/** Installed definition (null = empty). Replicated so clients can read it for the HUD / previews. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_State, Category = "Socket")
	TObjectPtr<UMCPartDef> Installed;

	/** Visual part actor attached by the installer (server sets it; replicated for lookups). */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Socket")
	TObjectPtr<AActor> InstalledActor;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_State, Category = "Socket") bool bBolted = false;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_State, Category = "Socket") bool bGrid = false;
	/** A part is flying/lowering towards this socket: nobody else may reserve it. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_State, Category = "Socket") bool bReserved = false;
	/** Red pulsing marker: installed but loose/off-grid, or a red check after preflight. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_State, Category = "Socket") bool bAlert = false;

	/** SocketGroup, except tank/engine/booster sockets are always Ascent. */
	EMCStackGroup GetGroup() const { return MCIsAscentOnlyType(SocketType) ? EMCStackGroup::Ascent : SocketGroup; }
	bool IsEmpty() const { return Installed == nullptr; }
	/** Tanks and solid boosters work without a cable. */
	bool IsSelfPowered() const { return Installed && (Installed->FuelCapKg > 0.f || Installed->SocketType == EMCSocketType::Booster || Installed->SocketType == EMCSocketType::Structure); }
	/** True if a tank/booster or the grid cable is connected: only live parts add power/thrust/science. */
	bool IsLive() const { return Installed && (IsSelfPowered() || bGrid); }

	/** Server only. Installs, resets bolt/grid flags, clears the reservation, notifies the manager. */
	void Install(UMCPartDef* Def, AActor* PartActor);
	/** Server only. Empties the socket and clears bolt/grid/alert. */
	void Remove();
	void SetBolted(bool bNew);
	void SetGrid(bool bNew);
	void SetReserved(bool bNew);
	void SetAlert(bool bNew);

	/** Server-side listeners (manager). */
	FMCSocketChanged OnChanged;

	/** Blueprint hook: refresh ghost / alert marker / hide installed visuals. Called on server and clients. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Socket") void BP_OnStateChanged();

protected:
	UFUNCTION() void OnRep_State();
	void NotifyChanged();
};
