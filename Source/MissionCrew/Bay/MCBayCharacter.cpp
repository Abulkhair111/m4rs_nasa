#include "Bay/MCBayCharacter.h"
#include "Bay/MCBayUI.h"
#include "Bay/MCBodyAnimComponent.h"
#include "Bay/MCBayHudWidget.h"
#include "Bay/MCPartMover.h"
#include "Bay/MCPartPickerWidget.h"
#include "Bay/MCRocketStack.h"
#include "Bay/UI/MCBayMenuWidget.h"
#include "Transit/MCStation.h"
#include "Transit/MCStationLibrary.h"
#include "Transit/Flight/MCFlightView.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Construction/MCConstructionManager.h"
#include "Construction/MCPartSocket.h"
#include "Interact/MCInteractable.h"
#include "Shelf/MCShelfPart.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/LocalPlayer.h"
#include "Net/UnrealNetwork.h"

AMCBayCharacter::AMCBayCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = true; // mouse yaw turns the body; pitch only moves the camera

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;

	BodyAnim = CreateDefaultSubobject<UMCBodyAnimComponent>(TEXT("BodyAnim"));

	GetCharacterMovement()->MaxWalkSpeed = 450.f;
	GetCharacterMovement()->JumpZVelocity = 420.f;
	GetCharacterMovement()->AirControl = 0.4f;
}

void AMCBayCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AMCBayCharacter, HeldTool, COND_OwnerOnly);
	DOREPLIFETIME(AMCBayCharacter, SeatedStation);
}

// ------------------------------------------------------------------ input

void AMCBayCharacter::BuildInput()
{
	if (Mapping) return;
	Mapping = NewObject<UInputMappingContext>(this, TEXT("IMC_BayRuntime"));

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type) {
		UInputAction* A = NewObject<UInputAction>(this, Name);
		A->ValueType = Type;
		return A;
	};
	MoveAction     = MakeAction(TEXT("IA_BayMove"), EInputActionValueType::Axis2D);
	LookAction     = MakeAction(TEXT("IA_BayLook"), EInputActionValueType::Axis2D);
	JumpAction     = MakeAction(TEXT("IA_BayJump"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(TEXT("IA_BayInteract"), EInputActionValueType::Boolean);
	RemoveAction   = MakeAction(TEXT("IA_BayRemove"), EInputActionValueType::Boolean);
	CursorAction   = MakeAction(TEXT("IA_BayCursor"), EInputActionValueType::Boolean);
	DropAction     = MakeAction(TEXT("IA_BayDrop"), EInputActionValueType::Boolean);
	MenuAction     = MakeAction(TEXT("IA_BayMenu"), EInputActionValueType::Boolean);

	// Move: X = right, Y = forward. W/S go through a YXZ swizzle.
	auto MapMove = [this](const FKey& Key, bool bSwizzle, bool bNegate) {
		FEnhancedActionKeyMapping& M = Mapping->MapKey(MoveAction, Key);
		if (bSwizzle) M.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Mapping));
		if (bNegate)  M.Modifiers.Add(NewObject<UInputModifierNegate>(Mapping));
	};
	MapMove(EKeys::W, true, false);
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::D, false, false);
	MapMove(EKeys::A, false, true);

	// Look: mouse delta; pitch is negated so pushing the mouse up looks up.
	{
		FEnhancedActionKeyMapping& M = Mapping->MapKey(LookAction, EKeys::Mouse2D);
		UInputModifierNegate* N = NewObject<UInputModifierNegate>(Mapping);
		N->bX = false; N->bY = true; N->bZ = false;
		M.Modifiers.Add(N);
	}
	Mapping->MapKey(JumpAction, EKeys::SpaceBar);
	Mapping->MapKey(InteractAction, EKeys::E);
	Mapping->MapKey(RemoveAction, EKeys::R);
	Mapping->MapKey(CursorAction, EKeys::Tab);
	Mapping->MapKey(DropAction, EKeys::G);
	Mapping->MapKey(MenuAction, EKeys::Escape);
	Mapping->MapKey(MenuAction, EKeys::P); // Esc stops PIE inside the editor, so P works too
}

void AMCBayCharacter::AddMappingContext()
{
	BuildInput();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController()) return;
	if (ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Sub->RemoveMappingContext(Mapping);
			FModifyContextOptions Opt;
			Opt.bIgnoreAllPressedKeysUntilRelease = false;
			Sub->AddMappingContext(Mapping, 0, Opt);
		}
	}
}

void AMCBayCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInput();
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMCBayCharacter::Move);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMCBayCharacter::Look);
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AMCBayCharacter::OnInteractPressed);
		EIC->BindAction(InteractAction, ETriggerEvent::Completed, this, &AMCBayCharacter::OnInteractReleased);
		EIC->BindAction(DropAction, ETriggerEvent::Started, this, &AMCBayCharacter::OnDropPressed);
		EIC->BindAction(RemoveAction, ETriggerEvent::Started, this, &AMCBayCharacter::OnRemovePressed);
		EIC->BindAction(CursorAction, ETriggerEvent::Started, this, &AMCBayCharacter::OnCursorPressed);
		EIC->BindAction(MenuAction, ETriggerEvent::Started, this, &AMCBayCharacter::OnMenuPressed);
		BuildStationInput();
		EIC->BindAction(StationUseAction, ETriggerEvent::Started, this, &AMCBayCharacter::OnStationInteract);
		EIC->BindAction(StationEscAction, ETriggerEvent::Started, this, &AMCBayCharacter::OnStationEscape);
		EIC->BindAction(StationViewAction, ETriggerEvent::Started, this, &AMCBayCharacter::OnStationViewToggle);
	}
}

void AMCBayCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	AddMappingContext();
	AddStationMapping();
	EnsureHud();
	if (SeatedStation) ApplySeatState(nullptr);
}

void AMCBayCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AddMappingContext(); // listen server / standalone host
	AddStationMapping();
	EnsureHud();
	if (AMCConstructionManager* M = GetManager())
	{
		AMCPartMover::BindToManager(M);
	}
}

void AMCBayCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	AddMappingContext(); // client: controller arrives after the pawn
	AddStationMapping();
	EnsureHud();
	if (SeatedStation) ApplySeatState(nullptr);
}

void AMCBayCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HasAuthority() && SeatedStation && SeatedStation->Occupant == this) SeatedStation->Occupant = nullptr;
	ShowTransitHud(false);
	Super::EndPlay(Reason);
}

void AMCBayCharacter::UnPossessed()
{
	ShowTransitHud(false);
	if (Picker) Picker->RemoveFromParent();
	Picker = nullptr;
	if (Menu) { Menu->Close(); Menu = nullptr; }
	if (Hud) Hud->RemoveFromParent();
	Hud = nullptr;
	Super::UnPossessed();
}

void AMCBayCharacter::Move(const FInputActionValue& Value)
{
	if (bCursorMode) return;
	const FVector2D V = Value.Get<FVector2D>();
	if (Controller)
	{
		const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), V.Y);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), V.X);
	}
}

void AMCBayCharacter::Look(const FInputActionValue& Value)
{
	if (bCursorMode) return;
	const FVector2D V = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(V.X);
	AddControllerPitchInput(V.Y);
}

void AMCBayCharacter::OnCursorPressed()
{
	if (Picker && Picker->IsInViewport())
	{
		Picker->Cancel();
		return;
	}
	ToggleCursorMode();
}

void AMCBayCharacter::OnMenuPressed()
{
	if (SeatedStation) { ServerStandUp(); return; } // Esc / P stand up first; the pause menu is for standing players
	if (Picker && Picker->IsInViewport())
	{
		Picker->Cancel(); // Esc / P closes the picker first
		return;
	}
	if (SelectedPart) { ClearSelection(true); return; } // Esc / P first clears a shelf selection
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController()) return;
	if (!Menu)
	{
		Menu = CreateWidget<UMCBayMenuWidget>(PC, MenuClass ? MenuClass : TSubclassOf<UMCBayMenuWidget>(UMCBayMenuWidget::StaticClass()));
	}
	if (Menu) Menu->Toggle();
}

void AMCBayCharacter::ToggleCursorMode() { SetCursorMode(!bCursorMode); }

void AMCBayCharacter::SetCursorMode(bool bEnable)
{
	bCursorMode = bEnable;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController()) return;
	PC->SetShowMouseCursor(bEnable);
	if (bEnable)
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
	}
}

void AMCBayCharacter::EnsureHud()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!bAutoCreateHud || Hud || !PC || !PC->IsLocalController()) return;
	TSubclassOf<UMCBayHudWidget> Cls = HudClass ? HudClass : TSubclassOf<UMCBayHudWidget>(UMCBayHudWidget::StaticClass());
	Hud = CreateWidget<UMCBayHudWidget>(PC, Cls);
	if (Hud) Hud->AddToViewport(10);
}

// ------------------------------------------------------------------ aim / interact

AMCConstructionManager* AMCBayCharacter::GetManager() const
{
	if (!CachedManager) CachedManager = MCBayUI::FindManager(GetWorld());
	return CachedManager;
}

int32 AMCBayCharacter::GetCrewRole() const
{
	const APlayerState* PS = GetPlayerState();
	if (!PS) return 0;
	if (const FProperty* P = PS->GetClass()->FindPropertyByName(TEXT("CrewRole")))
	{
		const void* Addr = P->ContainerPtrToValuePtr<void>(PS);
		if (const FIntProperty* IP = CastField<FIntProperty>(P)) return IP->GetPropertyValue(Addr);
		if (const FByteProperty* BP = CastField<FByteProperty>(P)) return BP->GetPropertyValue(Addr);
		if (const FEnumProperty* EP = CastField<FEnumProperty>(P)) return (int32)EP->GetUnderlyingProperty()->GetSignedIntPropertyValue(Addr);
	}
	return 0;
}

static FText MakeSocketPrompt(const AMCPartSocket* S, EMCToolType Tool)
{
	if (S->bReserved) return FText::FromString(TEXT("Part incoming..."));
	if (!S->Installed)
	{
		return FText::FromString(FString::Printf(TEXT("[E] Choose a part for the %s socket"), MCBayUI::SocketTypeName(S->SocketType)));
	}
	FString Hint;
	if (Tool == EMCToolType::Wrench) Hint = S->bBolted ? TEXT("\n[E] Loosen the bolts") : TEXT("\n[E] Tighten the bolts");
	else if (Tool == EMCToolType::Cable) Hint = S->IsSelfPowered() ? TEXT("\n(needs no cable)") : (S->bGrid ? TEXT("\n[E] Disconnect the cable") : TEXT("\n[E] Connect the cable"));
	else if (!S->bBolted) Hint = TEXT("\n[E] Swap this part   [R] Remove");
	else Hint = TEXT("\nBolted: take the wrench from the tools stand to loosen it");
	return FText::FromString(FString::Printf(TEXT("%s%s%s\n%s\n%s%s"),
		*S->Installed->DisplayName.ToString(),
		S->bBolted ? TEXT("  [bolted]") : TEXT("  [LOOSE]"),
		S->IsSelfPowered() ? TEXT("") : (S->bGrid ? TEXT(" [wired]") : TEXT(" [not wired]")),
		*MCBayUI::PartOneLiner(S->Installed), *MCBayUI::PartNumbers(S->Installed), *Hint));
}

void AMCBayCharacter::UpdateAim()
{
	AimedSocket = nullptr;
	AimedActor = nullptr;
	AimedInteractable = nullptr;
	bAimedViaStack = false;
	bAimedLauncherPart = false;
	AimPrompt = FText::GetEmpty();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || bCursorMode) return;

	FVector Eye; FRotator Rot;
	PC->GetPlayerViewPoint(Eye, Rot);
	const FVector Dir = Rot.Vector();

	// World interactables (buttons, levers, pumps, tool stand) win over everything else: short trace, Visibility channel.
	{
		FHitResult IHit;
		FCollisionQueryParams IQ(SCENE_QUERY_STAT(BayInteractable), false, this);
		if (GetWorld()->LineTraceSingleByChannel(IHit, Eye, Eye + Dir * InteractRange, ECC_Visibility, IQ))
		{
			if (AMCInteractable* I = Cast<AMCInteractable>(IHit.GetActor()))
			{
				AimedInteractable = I;
				AimedActor = I;
				AimPrompt = I->GetPromptFor(this);
				return;
			}
		}
	}

	// Pieces of the real rocket and of the wall display (long range, line of sight needed).
	{
		FMCStackHit Best; bool bFound = false;
		for (TActorIterator<AMCRocketStack> It(GetWorld()); It; ++It)
		{
			FMCStackHit H;
			if (It->TracePart(Eye, Dir, StackAimRange, H) && (!bFound || H.Dist < Best.Dist))
			{
				FHitResult Block;
				FCollisionQueryParams Q(SCENE_QUERY_STAT(BayStackAim), false, this);
				Q.AddIgnoredActor(*It);
				if (GetWorld()->LineTraceSingleByChannel(Block, Eye, Eye + Dir * FMath::Max(0.f, H.Dist - 5.f), ECC_Visibility, Q)) continue;
				Best = H; bFound = true;
			}
		}
		if (bFound)
		{
			if (Best.bPreset)
			{
				const AMCConstructionManager* M = GetManager();
				bAimedLauncherPart = true;
				AimPrompt = FText::FromString(FString::Printf(TEXT("Launcher: %s\n[E] Next launcher"),
					M && !M->SelectedLauncher.IsNone() ? *M->SelectedLauncher.ToString() : TEXT("none")));
				return;
			}
			if (Best.Socket)
			{
				AimedSocket = Best.Socket;
				bAimedViaStack = true;
				AimPrompt = MakeSocketPrompt(AimedSocket, HeldTool);
				return;
			}
		}
	}

	float BestDot = AimConeCos;
	for (TActorIterator<AMCPartSocket> It(GetWorld()); It; ++It)
	{
		const FVector To = It->GetActorLocation() - Eye;
		const float Dist = To.Size();
		if (Dist > SocketReach || Dist < 1.f) continue;
		const float Dot = FVector::DotProduct(To / Dist, Dir);
		if (Dot > BestDot) { BestDot = Dot; AimedSocket = *It; }
	}

	if (AimedSocket)
	{
		AimPrompt = MakeSocketPrompt(AimedSocket, HeldTool);
		return;
	}

	// Nothing else to aim at: plain scenery gets no prompt.
}

void AMCBayCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (IsLocallyControlled())
	{
		UpdateAim();
		if (SeatedStation)
		{
			AimPrompt = FText::FromString(TEXT("[E] / [Esc] Stand up"));
			if (!bCursorMode) SetCursorMode(true); // Tab must not leave the console
		}
		else if (!AimedSocket && !AimedActor && !bAimedLauncherPart)
		{
			UpdateStationAim(); // stations only when no interactable / socket / launcher part is aimed
		}
		else
		{
			AimedStation = nullptr;
		}
		if (SelectedPart && !SeatedStation)
		{
			const AMCConstructionManager* M = GetManager();
			if (AimedSocket && !AimedSocket->Installed && !AimedSocket->bReserved)
			{
				const FString Why = AMCShelfPart::WhyNotPlaceable(SelectedPart, AimedSocket, M, GetCrewRole());
				AimPrompt = FText::FromString(Why.IsEmpty()
					? FString::Printf(TEXT("[E] Install %s here"), *SelectedPart->DisplayName.ToString())
					: FString::Printf(TEXT("Cannot install %s here:\n%s"), *SelectedPart->DisplayName.ToString(), *Why));
			}
			const FString Sel = FString::Printf(TEXT("Selected: %s: aim at a free %s socket and press E   ([G] cancel)"),
				*SelectedPart->DisplayName.ToString(), MCBayUI::SocketTypeName(SelectedPart->SocketType));
			AimPrompt = AimPrompt.IsEmpty() ? FText::FromString(Sel) : FText::FromString(AimPrompt.ToString() + TEXT("\n") + Sel);
		}
		if (!NoticeText.IsEmpty() && GetWorld()->GetTimeSeconds() < NoticeUntil)
		{
			AimPrompt = AimPrompt.IsEmpty() ? FText::FromString(NoticeText) : FText::FromString(AimPrompt.ToString() + TEXT("\n") + NoticeText);
		}
		if (HeldTool != EMCToolType::None)
		{
			const FString Held = FString::Printf(TEXT("Holding: %s   [G] put down"), HeldTool == EMCToolType::Wrench ? TEXT("wrench (bolts)") : TEXT("cable reel (wires)"));
			AimPrompt = AimPrompt.IsEmpty() ? FText::FromString(Held) : FText::FromString(AimPrompt.ToString() + TEXT("\n") + Held);
		}
	}
}

void AMCBayCharacter::OnInteractPressed()
{
	if (bCursorMode) return;
	if (AMCShelfPart* SP = Cast<AMCShelfPart>(AimedInteractable))
	{
		OnShelfPartPressed(SP);
		return;
	}
	if (bAimedLauncherPart)
	{
		const AMCConstructionManager* M = GetManager();
		if (M && M->Rockets.Num() > 0) ServerSelectLauncher((M->SelectedLever + 1) % M->Rockets.Num());
		return;
	}
	if (AimedSocket)
	{
		if (AimedSocket->bReserved) return;
		if (SelectedPart && HeldTool == EMCToolType::None) { PlaceSelected(AimedSocket); return; }
		if (!AimedSocket->Installed) { OpenPicker(AimedSocket); return; }
		if (HeldTool != EMCToolType::None) { ServerInteract(AimedSocket); return; }
		// Installed part, no tool: unbolted = pick another one (swap); bolted = the server says to unbolt first.
		if (!AimedSocket->bBolted) OpenPicker(AimedSocket);
		else ServerRemovePart(AimedSocket);
		return;
	}
	if (AimedInteractable) { bHoldingInteract = true; ServerInteract(AimedInteractable); return; }
	if (AimedActor) ServerInteract(AimedActor);
}

void AMCBayCharacter::OnInteractReleased()
{
	if (!bHoldingInteract) return;
	bHoldingInteract = false;
	ServerEndInteract();
}

void AMCBayCharacter::OnDropPressed()
{
	if (bCursorMode) return;
	if (SelectedPart) { ClearSelection(true); return; } // G first clears a shelf selection
	ServerRelease();
}

// ------------------------------------------------------------------ shelf selection (local) and placement

void AMCBayCharacter::SelectPart(UMCPartDef* Def, AMCShelfPart* From)
{
	SelectedPart = Def;
	SelectedFrom = From;
}

void AMCBayCharacter::ClearSelection(bool bShowNote)
{
	const bool bHad = SelectedPart != nullptr;
	SelectedPart = nullptr;
	SelectedFrom = nullptr;
	if (bShowNote && bHad) ShowNotice(TEXT("Selection cleared"), 2.f);
}

void AMCBayCharacter::ShowNotice(const FString& Text, float Seconds)
{
	NoticeText = Text;
	NoticeUntil = GetWorld() ? GetWorld()->GetTimeSeconds() + Seconds : 0.0;
}

void AMCBayCharacter::OnShelfPartPressed(AMCShelfPart* Part)
{
	if (!Part || !Part->Def) return;
	if (!Part->IsAllowedFor(this))
	{
		ShowNotice(FString::Printf(TEXT("Only the %s can take the %s."), *AMCShelfPart::DepartmentTitle(Part->Def->Department), *Part->Def->DisplayName.ToString()), 4.f);
		return;
	}
	if (Part->Def->IsTool()) return;
	SelectPart(Part->Def, Part);
	Part->PlayPickup();
	NoticeText.Empty();
}

void AMCBayCharacter::PlaceSelected(AMCPartSocket* Socket)
{
	if (!Socket || !SelectedPart) return;
	const FString Why = AMCShelfPart::WhyNotPlaceable(SelectedPart, Socket, GetManager(), GetCrewRole());
	if (!Why.IsEmpty())
	{
		ShowNotice(Why, 4.f);
		return;
	}
	const AMCShelfPart* From = SelectedFrom.Get();
	ServerPlacePartFrom(Socket, SelectedPart, From ? From->GetPickupLocation() : FVector::ZeroVector, From != nullptr);
	if (SelectedFrom.IsValid()) SelectedFrom->PlayPickup();
	ClearSelection(false);
}

void AMCBayCharacter::OnRemovePressed()
{
	if (bCursorMode || !AimedSocket || !AimedSocket->Installed) return;
	ServerRemovePart(AimedSocket);
}

void AMCBayCharacter::OpenPicker(AMCPartSocket* Socket)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!Socket || !PC || !PC->IsLocalController()) return;
	if (!Picker)
	{
		TSubclassOf<UMCPartPickerWidget> Cls = PickerClass ? PickerClass : TSubclassOf<UMCPartPickerWidget>(UMCPartPickerWidget::StaticClass());
		Picker = CreateWidget<UMCPartPickerWidget>(PC, Cls);
	}
	if (Picker) Picker->Open(Socket, this);
}

void AMCBayCharacter::OnPickerClosed(UMCPartPickerWidget* /*Closed*/)
{
	SetCursorMode(false);
}

// ------------------------------------------------------------------ server RPCs

void AMCBayCharacter::ServerPlacePart_Implementation(AMCPartSocket* Socket, UMCPartDef* Def)
{
	AMCConstructionManager* M = GetManager();
	if (!M || !Socket || !Def) return;
	AMCPartMover::BindToManager(M);
	M->PlacePart(Socket, Def, (EMCCrewRole)GetCrewRole());
}

void AMCBayCharacter::ServerPlacePartFrom_Implementation(AMCPartSocket* Socket, UMCPartDef* Def, FVector From, bool bHasFrom)
{
	if (bHasFrom) AMCPartMover::SetNextStart(From);
	ServerPlacePart_Implementation(Socket, Def);
	AMCPartMover::ClearNextStart();
}

void AMCBayCharacter::ServerInteract_Implementation(AActor* Target)
{
	if (!Target) return;
	// Reach check (generous for latency).
	if (FVector::Dist(GetPawnViewLocation(), Target->GetActorLocation()) > FMath::Max(SocketReach + InteractRange + 300.f, StackAimRange + 300.f)) return;

	if (AMCPartSocket* S = Cast<AMCPartSocket>(Target))
	{
		if (AMCConstructionManager* M = GetManager())
		{
			if (HeldTool != EMCToolType::None && S->Installed)
			{
				M->ApplyTool(S, HeldTool, (EMCCrewRole)GetCrewRole());
			}
		}
		return;
	}
	if (AMCInteractable* I = Cast<AMCInteractable>(Target))
	{
		if (ActiveInteractable && ActiveInteractable != I) ActiveInteractable->Release(this);
		ActiveInteractable = I;
		I->Use(this);
		return;
	}
	BP_OnInteract(Target);
}

void AMCBayCharacter::ServerEndInteract_Implementation()
{
	if (ActiveInteractable) ActiveInteractable->Release(this);
	ActiveInteractable = nullptr;
}

void AMCBayCharacter::ServerRelease_Implementation()
{
	HeldTool = EMCToolType::None;
	if (ActiveInteractable) ActiveInteractable->Release(this);
	ActiveInteractable = nullptr;
	BP_OnRelease();
}

void AMCBayCharacter::ServerRemovePart_Implementation(AMCPartSocket* Socket)
{
	AMCConstructionManager* M = GetManager();
	if (!M || !Socket || !Socket->Installed) return;
	AActor* Visual = Socket->InstalledActor;
	if (M->RemovePart(Socket, (EMCCrewRole)GetCrewRole()) && Visual)
	{
		Visual->Destroy();
	}
}

void AMCBayCharacter::ServerSwapPart_Implementation(AMCPartSocket* Socket, UMCPartDef* Def)
{
	AMCConstructionManager* M = GetManager();
	if (!M || !Socket || !Def || !Socket->Installed || Socket->bReserved || Socket->Installed == Def) return;
	AMCPartMover::BindToManager(M);
	UMCPartDef* OldDef = Socket->Installed;
	AActor* OldVisual = Socket->InstalledActor;
	if (!M->RemovePart(Socket, (EMCCrewRole)GetCrewRole())) return; // bolted / wrong department: manager says why
	if (OldVisual) OldVisual->Destroy();
	if (!M->PlacePart(Socket, Def, (EMCCrewRole)GetCrewRole()))
	{
		Socket->Install(OldDef, nullptr); // refused (budget, department, fit): put the old part back
	}
}

void AMCBayCharacter::ServerSelectLauncher_Implementation(int32 LeverIndex)
{
	if (AMCConstructionManager* M = GetManager()) M->SelectRocket(LeverIndex);
}

void AMCBayCharacter::ServerSetHeldTool_Implementation(EMCToolType Tool)
{
	HeldTool = Tool;
}

// ================================================================== stations (moved from AMCStationCharacter)

// ------------------------------------------------------------------ input (own context, E and Esc)

void AMCBayCharacter::BuildStationInput()
{
	if (StationMapping) return;
	StationMapping = NewObject<UInputMappingContext>(this, TEXT("IMC_StationRuntime"));
	StationUseAction = NewObject<UInputAction>(this, TEXT("IA_StationUse"));
	StationUseAction->ValueType = EInputActionValueType::Boolean;
	StationEscAction = NewObject<UInputAction>(this, TEXT("IA_StationEsc"));
	StationEscAction->ValueType = EInputActionValueType::Boolean;
	StationMapping->MapKey(StationUseAction, EKeys::E);
	StationMapping->MapKey(StationEscAction, EKeys::Escape);
	StationViewAction = NewObject<UInputAction>(this, TEXT("IA_StationView"));
	StationViewAction->ValueType = EInputActionValueType::Boolean;
	StationMapping->MapKey(StationViewAction, EKeys::V);
	// The station context sits above the bay context (priority 1). By default an action consumes its key, which
	// would swallow E (bay interact) and Esc (pause menu) before they reach the bay actions. Let them fall through.
	StationUseAction->bConsumeInput = false;
	StationEscAction->bConsumeInput = false;
	StationViewAction->bConsumeInput = false;
}

void AMCBayCharacter::AddStationMapping()
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
// ------------------------------------------------------------------ aim

void AMCBayCharacter::UpdateStationAim()
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
void AMCBayCharacter::OnStationInteract()
{
	if (SeatedStation) { ServerStandUp(); return; }
	if (AimedStation && !bStationRefused && AimedStation->IsFree() && !bCursorMode) ServerSitAtStation(AimedStation);
}

void AMCBayCharacter::OnStationEscape()
{
	if (SeatedStation) ServerStandUp();
}

void AMCBayCharacter::OnStationViewToggle()
{
	ToggleFlightView();
}

void AMCBayCharacter::ToggleFlightView()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!SeatedStation || !PC || !PC->IsLocalController()) return;
	AMCFlightView* FV = AMCFlightView::Find(this);
	if (!FV) return;
	bFlightViewActive = !bFlightViewActive;
	AActor* Target = bFlightViewActive ? static_cast<AActor*>(FV) : static_cast<AActor*>(SeatedStation);
	PC->SetViewTargetWithBlend(Target, ViewBlendTime);
}

// ------------------------------------------------------------------ seat logic (server)

bool AMCBayCharacter::SeatAt(AMCStation* Station, bool bIgnoreRole)
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

bool AMCBayCharacter::SeatAtOwnStation()
{
	if (!HasAuthority()) return false;
	AMCStation* S = AMCStation::FindForRole(this, GetCrewRole());
	if (!S && GetCrewRole() <= 0)
	{
		// No lobby role (direct test play): take the first free station so a solo tester is seated and can use any console.
		for (TActorIterator<AMCStation> It(GetWorld()); It; ++It) { if (It->IsFree()) { S = *It; break; } }
	}
	return S ? SeatAt(S, true) : false;
}

void AMCBayCharacter::StandUp()
{
	if (!HasAuthority() || !SeatedStation) return;
	AMCStation* Prev = SeatedStation;
	if (Prev->Occupant == this) Prev->Occupant = nullptr;
	SeatedStation = nullptr;
	ApplySeatState(Prev);
	Prev->ForceNetUpdate();
	ForceNetUpdate();
}

void AMCBayCharacter::ServerSitAtStation_Implementation(AMCStation* Station)
{
	if (!Station) return;
	// Reach check (generous for latency).
	if (FVector::Dist(GetActorLocation(), Station->GetActorLocation()) > Station->UseRange + 600.f) return;
	SeatAt(Station, false);
}

void AMCBayCharacter::ServerStandUp_Implementation()
{
	StandUp();
}

void AMCBayCharacter::ServerTransitAction_Implementation(int32 Code)
{
	if (bRequireSeat && !SeatedStation && !UMCStationLibrary::AreRolesFree(this)) return;
	AActor* M = UMCStationLibrary::FindTransitManager(this);
	if (!M) return;
	UMCStationLibrary::CallHandleAction(M, Code, GetCrewRole());
}
// ------------------------------------------------------------------ seat state (server + clients)

void AMCBayCharacter::OnRep_Seat(AMCStation* Previous)
{
	ApplySeatState(Previous);
}

void AMCBayCharacter::ShowTransitHud(bool bShow)
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

void AMCBayCharacter::ApplySeatState(AMCStation* /*Previous*/)
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
		// Every seated player starts in the 3D flight chase view (rocket + controls overlay, no view of the crew) when the
		// level has an AMCFlightView; V toggles back to the station camera.
		AMCFlightView* FV = AMCFlightView::Find(this);
		bFlightViewActive = FV != nullptr;
		PC->SetViewTargetWithBlend(bFlightViewActive ? static_cast<AActor*>(FV) : static_cast<AActor*>(SeatedStation), ViewBlendTime);
		SetCursorMode(true);
		if (Hud) Hud->SetVisibility(ESlateVisibility::Collapsed);
		ShowTransitHud(true);
	}
	else
	{
		ShowTransitHud(false);
		bFlightViewActive = false;
		PC->SetViewTargetWithBlend(this, ViewBlendTime * 0.6f);
		SetCursorMode(false);
		if (Hud) Hud->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}
