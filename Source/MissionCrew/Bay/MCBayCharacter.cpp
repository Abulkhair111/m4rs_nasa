#include "Bay/MCBayCharacter.h"
#include "Bay/MCBayUI.h"
#include "Bay/MCBodyAnimComponent.h"
#include "Bay/MCBayHudWidget.h"
#include "Bay/MCPartMover.h"
#include "Bay/MCPartPickerWidget.h"
#include "Bay/MCRocketStack.h"
#include "Bay/UI/MCBayMenuWidget.h"
#include "Construction/MCConstructionManager.h"
#include "Construction/MCPartSocket.h"
#include "Interact/MCInteractable.h"

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
	}
}

void AMCBayCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	AddMappingContext();
	EnsureHud();
}

void AMCBayCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AddMappingContext(); // listen server / standalone host
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
	EnsureHud();
}

void AMCBayCharacter::UnPossessed()
{
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
	if (Picker && Picker->IsInViewport())
	{
		Picker->Cancel(); // Esc / P closes the picker first
		return;
	}
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
	if (bAimedLauncherPart)
	{
		const AMCConstructionManager* M = GetManager();
		if (M && M->Rockets.Num() > 0) ServerSelectLauncher((M->SelectedLever + 1) % M->Rockets.Num());
		return;
	}
	if (AimedSocket)
	{
		if (AimedSocket->bReserved) return;
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
	ServerRelease();
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

void AMCBayCharacter::ServerInteract_Implementation(AActor* Target)
{
	if (!Target) return;
	// Reach check (generous for latency).
	if (FVector::Dist(GetPawnViewLocation(), Target->GetActorLocation()) > SocketReach + InteractRange + 300.f) return;

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
