#include "Construction/MCPartSocket.h"
#include "Net/UnrealNetwork.h"
#include "Components/SceneComponent.h"

AMCPartSocket::AMCPartSocket()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);
	// Sockets are placed in the level, so they must exist on clients with stable names.
	bAlwaysRelevant = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AMCPartSocket::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCPartSocket, SocketType);
	DOREPLIFETIME(AMCPartSocket, Installed);
	DOREPLIFETIME(AMCPartSocket, InstalledActor);
	DOREPLIFETIME(AMCPartSocket, bBolted);
	DOREPLIFETIME(AMCPartSocket, bGrid);
	DOREPLIFETIME(AMCPartSocket, bReserved);
	DOREPLIFETIME(AMCPartSocket, bAlert);
}

void AMCPartSocket::NotifyChanged()
{
	BP_OnStateChanged();
	OnChanged.Broadcast();
}

void AMCPartSocket::OnRep_State()
{
	BP_OnStateChanged();
}

void AMCPartSocket::Install(UMCPartDef* Def, AActor* PartActor)
{
	if (!HasAuthority()) { return; }
	Installed = Def;
	InstalledActor = PartActor;
	bBolted = false;
	bGrid = false;
	bReserved = false;
	NotifyChanged();
}

void AMCPartSocket::Remove()
{
	if (!HasAuthority()) { return; }
	Installed = nullptr;
	InstalledActor = nullptr;
	bBolted = false;
	bGrid = false;
	bAlert = false;
	NotifyChanged();
}

void AMCPartSocket::SetBolted(bool bNew)   { if (HasAuthority()) { bBolted = bNew; NotifyChanged(); } }
void AMCPartSocket::SetGrid(bool bNew)     { if (HasAuthority()) { bGrid = bNew; NotifyChanged(); } }
void AMCPartSocket::SetReserved(bool bNew) { if (HasAuthority()) { bReserved = bNew; NotifyChanged(); } }
void AMCPartSocket::SetAlert(bool bNew)    { if (HasAuthority()) { bAlert = bNew; BP_OnStateChanged(); } }
