#include "Transit/MCStationCharacter.h"
#include "Transit/MCStation.h"
#include "Transit/MCStationLibrary.h"
#include "Bay/MCBayHudWidget.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"

AMCStationCharacter::AMCStationCharacter()
{
}

void AMCStationCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCStationCharacter, SeatedStation);
}

// ------------------------------------------------------------------ input (own context, E and Esc)

void AMCStationCharacter::BuildStationInput()
{
	if (StationMapping) return;
	StationMapping = NewObject<UInputMappingContext>(this, TEXT("IMC_StationRuntime"));
	StationUseAction = NewObject<UInputAction>(this, TEXT("IA_StationUse"));
	StationUseAction->ValueType = EInputActionValueType::Boolean;
	StationEscAction = NewObject<UInputAction>(this, TEXT("IA_StationEsc"));
	StationEscAction->ValueType = EInputActionValueType::Boolean;
	StationMapping->MapKey(StationUseAction, EKeys::E);
	StationMapping->MapKey(StationEscAction, EKeys::Escape);
}

void AMCStationCharacter::AddStationMapping()
{
	BuildStationInput();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController()) return;
	if (ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->RemoveMappingContext(StationMapping);
			FModifyContextOptions Opt;
			Opt.bIgnoreAllPressedKeysUntilRelease = false;
			Sub->AddMappingContext(StationMapping, 1, Opt);
		}
	}
}

void AMCStationCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildStationInput();
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(StationUseAction, ETriggerEvent::Started, this, &AMCStationCharacter::OnStationInteract);
		EIC->BindAction(StationEscAction, ETriggerEvent::Started, this, &AMCStationCharacter::OnStationEscape);
	}
}

void AMCStationCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	AddStationMapping();
	if (SeatedStation) ApplySeatState(nullptr);
}

void AMCStationCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AddStationMapping();
}

void AMCStationCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	AddStationMapping();
	if (SeatedStation) ApplySeatState(nullptr);
}

void AMCStationCharacter::UnPossessed()
{
	ShowTransitHud(false);
	Super::UnPossessed();
}

void AMCStationCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HasAuthority() && SeatedStation && SeatedStation->Occupant == this) SeatedStation->Occupant = nullptr;
	Super::EndPlay(Reason);
}

// ------------------------------------------------------------------ aim

void AMCStationCharacter::UpdateStationAim()
{
	AimedStation = nullptr;
	bStationRefused = false;
	if (SeatedStation || bCursorMode) return;
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	FVector Eye; FRotator Rot;
	PC->GetPlayerViewPoint(Eye, Rot);
	const FVector Dir = Rot.Vector();

	float BestDot = StationAimCos;
	for (TActorIterator<AMCStation> It(GetWorld()); It; ++It)
	{
		const FBox Box = It->Trigger ? It->Trigger->Bounds.GetBox() : FBox(It->GetActorLocation() - FVector(50.f), It->GetActorLocation() + FVector(50.f));
		const float Dist = FMath::Sqrt(Box.ComputeSquaredDistanceToPoint(Eye));
		if (Dist > It->UseRange) continue;
		const FVector To = Box.GetCenter() - Eye;
		const float Dot = To.IsNearlyZero() ? 1.f : FVector::DotProduct(To.GetSafeNormal(), Dir);
		if (Dist < 1.f || Dot > BestDot) { BestDot = Dot; AimedStation = *It; }
	}

	if (AimedStation)
	{
		const bool bFree = UMCStationLibrary::AreRolesFree(this);
		const int32 MyRole = GetCrewRole();
		const bool bOk = MyRole == 0 || AimedStation->CanBeUsedBy(MyRole, bFree);
		const FString Name = AimedStation->StationName.IsEmpty() ? AMCStation::RoleName(AimedStation->CrewRole) : AimedStation->StationName.ToString();
		if (!bOk)
		{
			bStationRefused = true;
			AimPrompt = FText::FromString(FString::Printf(TEXT("%s - station of another role (you are %s)"), *Name, *AMCStation::RoleName(MyRole)));
		}
		else if (!AimedStation->IsFree())
		{
			AimPrompt = FText::FromString(FString::Printf(TEXT("%s - occupied"), *Name));
		}
		else
		{
			AimPrompt = FText::FromString(FString::Printf(TEXT("[E] Sit at %s"), *Name));
		}
	}
}

void AMCStationCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsLocallyControlled()) return;
	if (SeatedStation)
	{
		AimPrompt = FText::FromString(TEXT("[E] / [Esc] Stand up"));
		if (!bCursorMode) SetCursorMode(true); // Tab must not leave the console
	}
	else if (!AimedSocket && !AimedActor && !bAimedLauncherPart)
	{
		UpdateStationAim();
	}
	else
	{
		AimedStation = nullptr;
	}
}

void AMCStationCharacter::OnStationInteract()
{
	if (SeatedStation) { ServerStandUp(); return; }
	if (AimedStation && !bStationRefused && AimedStation->IsFree() && !bCursorMode) ServerSitAtStation(AimedStation);
}

void AMCStationCharacter::OnStationEscape()
{
	if (SeatedStation) ServerStandUp();
}

// ------------------------------------------------------------------ seat logic (server)

bool AMCStationCharacter::SeatAt(AMCStation* Station, bool bIgnoreRole)
{
	if (!HasAuthority() || !Station) return false;
	if (SeatedStation == Station) return true;
	if (Station->Occupant && Station->Occupant != this) return false;
	if (!bIgnoreRole)
	{
		const int32 MyRole = GetCrewRole();
		if (MyRole != 0 && !Station->CanBeUsedBy(MyRole, UMCStationLibrary::AreRolesFree(this))) return false;
	}
	if (SeatedStation) StandUp();

	const FTransform Seat = Station->SeatPoint ? Station->SeatPoint->GetComponentTransform() : Station->GetActorTransform();
	FVector Loc = Seat.GetLocation();
	Loc.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FRotator Face(0.f, Station->GetActorRotation().Yaw, 0.f);
	TeleportTo(Loc, Face, false, true);
	if (AController* C = GetController()) C->SetControlRotation(Face);
	if (APlayerController* PC = Cast<APlayerController>(GetController())) PC->ClientSetRotation(Face);

	AMCStation* Prev = SeatedStation;
	Station->Occupant = this;
	SeatedStation = Station;
	ApplySeatState(Prev);
	Station->ForceNetUpdate();
	ForceNetUpdate();
	return true;
}

bool AMCStationCharacter::SeatAtOwnStation()
{
	if (!HasAuthority()) return false;
	AMCStation* S = AMCStation::FindForRole(this, GetCrewRole());
	return S ? SeatAt(S, true) : false;
}

void AMCStationCharacter::StandUp()
{
	if (!HasAuthority() || !SeatedStation) return;
	AMCStation* Prev = SeatedStation;
	if (Prev->Occupant == this) Prev->Occupant = nullptr;
	SeatedStation = nullptr;
	ApplySeatState(Prev);
	Prev->ForceNetUpdate();
	ForceNetUpdate();
}

void AMCStationCharacter::ServerSitAtStation_Implementation(AMCStation* Station)
{
	if (!Station) return;
	// Reach check (generous for latency).
	if (FVector::Dist(GetActorLocation(), Station->GetActorLocation()) > Station->UseRange + 600.f) return;
	SeatAt(Station, false);
}

void AMCStationCharacter::ServerStandUp_Implementation()
{
	StandUp();
}

void AMCStationCharacter::ServerTransitAction_Implementation(int32 Code)
{
	if (bRequireSeat && !SeatedStation && !UMCStationLibrary::AreRolesFree(this)) return;
	AActor* M = UMCStationLibrary::FindTransitManager(this);
	if (!M) return;
	UMCStationLibrary::CallHandleAction(M, Code, GetCrewRole());
}

// ------------------------------------------------------------------ seat state (server + clients)

void AMCStationCharacter::OnRep_Seat(AMCStation* Previous)
{
	ApplySeatState(Previous);
}

void AMCStationCharacter::ShowTransitHud(bool bShow)
{
	if (!bShow)
	{
		if (TransitHud) TransitHud->RemoveFromParent();
		TransitHud = nullptr;
		return;
	}
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (TransitHud || !PC || !PC->IsLocalController()) return;
	TSubclassOf<UUserWidget> Cls = TransitHudClass;
	if (!Cls) Cls = FindObject<UClass>(nullptr, TEXT("/Script/MissionCrew.MCTransitHudWidget"));
	if (!Cls) return;
	TransitHud = CreateWidget<UUserWidget>(PC, Cls);
	if (TransitHud) TransitHud->AddToViewport(20);
}

void AMCStationCharacter::ApplySeatState(AMCStation* /*Previous*/)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bSeated = SeatedStation != nullptr;
	if (Move)
	{
		if (bSeated) { Move->StopMovementImmediately(); Move->DisableMovement(); }
		else Move->SetMovementMode(MOVE_Walking);
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController()) return;

	if (bSeated)
	{
		PC->SetViewTargetWithBlend(SeatedStation, ViewBlendTime);
		SetCursorMode(true);
		if (Hud) Hud->SetVisibility(ESlateVisibility::Collapsed);
		ShowTransitHud(true);
	}
	else
	{
		ShowTransitHud(false);
		PC->SetViewTargetWithBlend(this, ViewBlendTime * 0.6f);
		SetCursorMode(false);
		if (Hud) Hud->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}
