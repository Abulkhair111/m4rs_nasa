#include "Surface/MCSurfaceLink.h"
#include "Surface/MCSurfaceManager.h"
#include "Surface/MCSurfaceRover.h"
#include "Surface/MCSurfaceHudWidget.h"
#include "Transit/MCTransitManager.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

AMCSurfaceLink::AMCSurfaceLink()
{
	bReplicates = true;
	bOnlyRelevantToOwner = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	SetNetUpdateFrequency(5.f);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

AMCSurfaceManager* AMCSurfaceLink::GetManager()
{
	if (!Manager)
	{
		for (TActorIterator<AMCSurfaceManager> It(GetWorld()); It; ++It) { Manager = *It; break; }
	}
	return Manager;
}

void AMCSurfaceLink::ServerAction_Implementation(int32 Code)
{
	if (AMCSurfaceManager* M = GetManager())
	{
		M->HandleAction(Code, AMCTransitManager::GetCrewRoleOfController(Cast<AController>(GetOwner())));
	}
}

void AMCSurfaceLink::ServerDrive_Implementation(float Throttle, float Steer)
{
	if (AMCSurfaceManager* M = GetManager())
	{
		M->HandleDrive(Throttle, Steer, AMCTransitManager::GetCrewRoleOfController(Cast<AController>(GetOwner())));
	}
}

void AMCSurfaceLink::Tick(float Dt)
{
	Super::Tick(Dt);
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController()) { return; }
	AMCSurfaceManager* M = GetManager();
	if (!M) { return; }
	if (!Hud)
	{
		TSubclassOf<UMCSurfaceHudWidget> Cls = HudClass ? HudClass : TSubclassOf<UMCSurfaceHudWidget>(UMCSurfaceHudWidget::StaticClass());
		Hud = CreateWidget<UMCSurfaceHudWidget>(PC, Cls);
		if (Hud)
		{
			Hud->Link = this;
			Hud->AddToViewport(20);
			PC->SetShowMouseCursor(true);
			PC->SetInputMode(FInputModeGameAndUI().SetHideCursorDuringCapture(false));
		}
	}
	if (!bViewSet && M->bChaseCamera && M->Rover)
	{
		PC->SetViewTargetWithBlend(M->Rover, 1.0f);
		bViewSet = true;
	}
}

void AMCSurfaceLink::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Hud) { Hud->RemoveFromParent(); Hud = nullptr; }
	Super::EndPlay(Reason);
}
