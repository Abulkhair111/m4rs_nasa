#include "Construction/MCPartSocket.h"
#include "Net/UnrealNetwork.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AMCPartSocket::AMCPartSocket()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);
	// Sockets are placed in the level, so they must exist on clients with stable names.
	bAlwaysRelevant = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	// Marker: engine Cylinder squashed into a flat disc (no self-made geometry).
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = true;
	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(GetRootComponent());
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetCastShadow(false);
	Marker->SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cyl(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Mat(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Cyl.Succeeded()) Marker->SetStaticMesh(Cyl.Object);
	if (Mat.Succeeded()) Marker->SetMaterial(0, Mat.Object);
	Marker->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.02f));
}

void AMCPartSocket::BeginPlay()
{
	Super::BeginPlay();
	if (Marker)
	{
		Marker->SetRelativeScale3D(FVector(MarkerDiameterCm / 100.f, MarkerDiameterCm / 100.f, 0.02f));
		MarkerMID = Marker->CreateDynamicMaterialInstance(0);
	}
	RefreshMarker();
}

void AMCPartSocket::RefreshMarker()
{
	if (!Marker || GetNetMode() == NM_DedicatedServer) return;
	const bool bEmpty = Installed == nullptr && !bReserved;
	const bool bShow = bShowMarker && (bEmpty || bAlert);
	Marker->SetVisibility(bShow);
	bMarkerPulse = bShow && bAlert;
	SetActorTickEnabled(bMarkerPulse);
	if (MarkerMID)
	{
		MarkerMID->SetVectorParameterValue(TEXT("Color"), bAlert ? FLinearColor(1.f, 0.1f, 0.1f) : FLinearColor(0.2f, 0.6f, 1.f));
	}
	if (!bMarkerPulse)
	{
		const float S = MarkerDiameterCm / 100.f;
		Marker->SetRelativeScale3D(FVector(S, S, 0.02f));
	}
}

void AMCPartSocket::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bMarkerPulse && Marker)
	{
		const float S = (MarkerDiameterCm / 100.f) * (1.f + 0.25f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f));
		Marker->SetRelativeScale3D(FVector(S, S, 0.02f));
	}
}

void AMCPartSocket::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCPartSocket, SocketType);
	DOREPLIFETIME(AMCPartSocket, SocketGroup);
	DOREPLIFETIME(AMCPartSocket, Installed);
	DOREPLIFETIME(AMCPartSocket, InstalledActor);
	DOREPLIFETIME(AMCPartSocket, bBolted);
	DOREPLIFETIME(AMCPartSocket, bGrid);
	DOREPLIFETIME(AMCPartSocket, bReserved);
	DOREPLIFETIME(AMCPartSocket, bAlert);
}

void AMCPartSocket::NotifyChanged()
{
	RefreshMarker();
	BP_OnStateChanged();
	OnChanged.Broadcast();
}

void AMCPartSocket::OnRep_State()
{
	RefreshMarker();
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
void AMCPartSocket::SetAlert(bool bNew)    { if (HasAuthority()) { bAlert = bNew; RefreshMarker(); BP_OnStateChanged(); } }
