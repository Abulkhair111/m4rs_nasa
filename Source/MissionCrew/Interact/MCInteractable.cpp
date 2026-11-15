#include "Interact/MCInteractable.h"
#include "Bay/MCBayCharacter.h"
#include "Bay/MCBayUI.h"
#include "Construction/MCConstructionManager.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"

// ------------------------------------------------------------------ base

AMCInteractable::AMCInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false; // level-placed; state is read from the replicated manager

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	TraceBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TraceBox"));
	TraceBox->SetupAttachment(Root);
	TraceBox->SetBoxExtent(FVector(30.f, 30.f, 30.f));
	TraceBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TraceBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TraceBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	TraceBox->SetGenerateOverlapEvents(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	LabelText->SetupAttachment(Root);
	LabelText->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetWorldSize(14.f);
	LabelText->SetTextRenderColor(FColor::White);
	LabelText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMCInteractable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (LabelText)
	{
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetVisibility(!Label.IsEmpty());
	}
}

AMCConstructionManager* AMCInteractable::GetManager() const
{
	if (!CachedManager) CachedManager = MCBayUI::FindManager(GetWorld());
	return CachedManager;
}

bool AMCInteractable::IsAllowedFor(const AMCBayCharacter* Who) const
{
	if (Department <= 0) return true;
	const AMCConstructionManager* M = GetManager();
	const int32 CrewRoleNum = Who ? Who->GetCrewRole() : 0;
	return M ? M->RoleAllowed((EMCCrewRole)CrewRoleNum, Department) : true;
}

const TCHAR* AMCInteractable::DepartmentName(int32 Dept)
{
	switch (Dept)
	{
	case 1: return TEXT("Power Engineer");
	case 2: return TEXT("Construction Engineer");
	case 3: return TEXT("Propulsion Engineer");
	case 4: return TEXT("Science Specialist");
	default: return TEXT("crew");
	}
}

bool AMCInteractable::CheckRole(const AMCBayCharacter* Who, const TCHAR* WhatFor) const
{
	if (IsAllowedFor(Who)) return true;
	if (AMCConstructionManager* M = GetManager())
	{
		M->Say(FString::Printf(TEXT("Only the %s %s."), DepartmentName(Department), WhatFor));
	}
	return false;
}

FText AMCInteractable::GetPromptFor(const AMCBayCharacter* Who) const
{
	if (!IsAllowedFor(Who))
	{
		return FText::FromString(FString::Printf(TEXT("%s (%s only)"), *Prompt.ToString(), DepartmentName(Department)));
	}
	return FText::FromString(FString::Printf(TEXT("[E] %s"), *Prompt.ToString()));
}

void AMCInteractable::Use(AMCBayCharacter* Who) { BP_OnUsed(Who); }
void AMCInteractable::Release(AMCBayCharacter* /*Who*/) {}

// ------------------------------------------------------------------ ready button

AMCReadyButton::AMCReadyButton()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	Department = 1;
	Prompt = FText::FromString(TEXT("Toggle READY"));
	Label = TEXT("READY");
}

FText AMCReadyButton::GetPromptFor(const AMCBayCharacter* Who) const
{
	const AMCConstructionManager* M = GetManager();
	const bool bOn = M && M->DeptReady.IsValidIndex(Department) && M->DeptReady[Department];
	FString S = FString::Printf(TEXT("%s: %s"), DepartmentName(Department), bOn ? TEXT("READY") : TEXT("not ready"));
	S += IsAllowedFor(Who) ? TEXT("\n[E] Toggle READY") : TEXT("\n(other department's button)");
	return FText::FromString(S);
}

void AMCReadyButton::Use(AMCBayCharacter* Who)
{
	AMCConstructionManager* M = GetManager();
	if (!M || !CheckRole(Who, TEXT("presses this READY button"))) return;
	M->ToggleReady(Department);
	Super::Use(Who);
}

// ------------------------------------------------------------------ launch button

AMCLaunchButton::AMCLaunchButton()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.2f;
	Department = 2;
	Prompt = FText::FromString(TEXT("LAUNCH"));
	Label = TEXT("LAUNCH");
}

FText AMCLaunchButton::GetPromptFor(const AMCBayCharacter* Who) const
{
	const AMCConstructionManager* M = GetManager();
	if (!M) return Prompt;
	int32 Ready = 0;
	for (int32 d = 1; d < MCConst::NumRoles; ++d) if (M->DeptReady.IsValidIndex(d) && M->DeptReady[d]) ++Ready;
	bool bRed = false, bYellow = false;
	for (EMCCheckState S : M->Checks) { bRed |= (S == EMCCheckState::Red); bYellow |= (S == EMCCheckState::Yellow); }
	FString S = FString::Printf(TEXT("LAUNCH - ready %d/4, %s"), Ready,
		bRed ? TEXT("red checks (will abort)") : (bYellow ? TEXT("warnings (press twice)") : TEXT("all green")));
	S += IsAllowedFor(Who) ? TEXT("\n[E] Launch") : TEXT("\n(Construction Engineer only)");
	return FText::FromString(S);
}

void AMCLaunchButton::Use(AMCBayCharacter* Who)
{
	AMCConstructionManager* M = GetManager();
	if (!M || !CheckRole(Who, TEXT("presses LAUNCH"))) return;
	M->TryLaunch(); // says why when it refuses (not ready, red, first yellow press)
	Super::Use(Who);
}

// ------------------------------------------------------------------ fuel pump

AMCFuelPump::AMCFuelPump()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	Department = 3;
	Prompt = FText::FromString(TEXT("Hold to pump fuel"));
	Label = TEXT("FUEL PUMP");
}

FText AMCFuelPump::GetPromptFor(const AMCBayCharacter* Who) const
{
	const AMCConstructionManager* M = GetManager();
	FString S = FString::Printf(TEXT("%s"), Rate >= 0.f ? TEXT("Fuel pump") : TEXT("Fuel drain"));
	if (M) S += FString::Printf(TEXT("\nFuel %.0f / %.0f kg (need %.0f)"), M->FuelKg, M->FuelMaxKg, M->FuelRequiredKg);
	S += IsAllowedFor(Who) ? (Rate >= 0.f ? TEXT("\n[Hold E] Fill") : TEXT("\n[Hold E] Drain")) : TEXT("\n(Propulsion Engineer only)");
	return FText::FromString(S);
}

void AMCFuelPump::Use(AMCBayCharacter* Who)
{
	if (!Who || !GetManager() || !CheckRole(Who, TEXT("works the fuel pump"))) return;
	Operator = Who;
	SetActorTickEnabled(true);
	Super::Use(Who);
}

void AMCFuelPump::Release(AMCBayCharacter* Who)
{
	if (Operator.Get() == Who) Operator.Reset();
	SetActorTickEnabled(false);
}

void AMCFuelPump::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AMCBayCharacter* Who = Operator.Get();
	AMCConstructionManager* M = GetManager();
	if (!Who || !M || !HasAuthority()
		|| FVector::Dist(Who->GetActorLocation(), GetActorLocation()) > MaxOperatorDistance)
	{
		Operator.Reset();
		SetActorTickEnabled(false);
		return;
	}
	M->AddFuel(Rate * DeltaSeconds);
}

// ------------------------------------------------------------------ rocket lever

AMCRocketLever::AMCRocketLever()
{
	Department = 2;
	Prompt = FText::FromString(TEXT("Select launcher"));
	Label = TEXT("LAUNCHER");
}

FText AMCRocketLever::GetPromptFor(const AMCBayCharacter* Who) const
{
	const AMCConstructionManager* M = GetManager();
	if (!M || !M->Rockets.IsValidIndex(LeverIndex)) return FText::FromString(TEXT("Launcher lever (no such launcher)"));
	const FMCRocketDef& R = M->Rockets[LeverIndex];
	FString S = FString::Printf(TEXT("%s: payload %.0f kg, $%.0fM, reliability %.0f%%%s"), *R.Name.ToString(), R.PayloadKg, R.CostM,
		R.Reliability * 100.f, M->SelectedLever == LeverIndex ? TEXT("  [SELECTED]") : TEXT(""));
	S += IsAllowedFor(Who) ? TEXT("\n[E] Select this launcher") : TEXT("\n(Construction Engineer only)");
	return FText::FromString(S);
}

void AMCRocketLever::Use(AMCBayCharacter* Who)
{
	AMCConstructionManager* M = GetManager();
	if (!M || !CheckRole(Who, TEXT("chooses the launcher"))) return;
	M->SelectRocket(LeverIndex);
	Super::Use(Who);
}

// ------------------------------------------------------------------ tools

const TCHAR* AMCToolItem::ToolName(EMCToolType T)
{
	return T == EMCToolType::Wrench ? TEXT("wrench") : (T == EMCToolType::Cable ? TEXT("cable reel") : TEXT("nothing"));
}

AMCToolItem::AMCToolItem()
{
	Department = 0; // set from ToolType on use
	Prompt = FText::FromString(TEXT("Take tool"));
	Label = TEXT("TOOL");
}

FText AMCToolItem::GetPromptFor(const AMCBayCharacter* Who) const
{
	const int32 Dept = DepartmentOfTool(ToolType);
	const AMCConstructionManager* M = GetManager();
	const bool bAllowed = !M || M->RoleAllowed((EMCCrewRole)(Who ? Who->GetCrewRole() : 0), Dept);
	if (!bAllowed) return FText::FromString(FString::Printf(TEXT("%s (%s only)"), ToolName(ToolType), DepartmentName(Dept)));
	if (Who && Who->HeldTool == ToolType) return FText::FromString(FString::Printf(TEXT("[E] Put down the %s"), ToolName(ToolType)));
	return FText::FromString(FString::Printf(TEXT("[E] Take the %s"), ToolName(ToolType)));
}

void AMCToolItem::Use(AMCBayCharacter* Who)
{
	AMCConstructionManager* M = GetManager();
	if (!Who) return;
	if (Who->HeldTool == ToolType) { Who->HeldTool = EMCToolType::None; return; }
	const int32 Dept = DepartmentOfTool(ToolType);
	if (M && !M->RoleAllowed((EMCCrewRole)Who->GetCrewRole(), Dept))
	{
		M->Say(FString::Printf(TEXT("Only the %s uses the %s."), DepartmentName(Dept), ToolName(ToolType)));
		return;
	}
	Who->HeldTool = ToolType;
	Super::Use(Who);
}

AMCToolStand::AMCToolStand()
{
	Prompt = FText::FromString(TEXT("Tools"));
	Label = TEXT("TOOLS & WIRING");
}

FText AMCToolStand::GetPromptFor(const AMCBayCharacter* Who) const
{
	const TCHAR* Cur = Who ? AMCToolItem::ToolName(Who->HeldTool) : TEXT("nothing");
	return FText::FromString(FString::Printf(TEXT("Tools & wiring (holding: %s)\n[E] Next tool (wrench: Construction, cable reel: Power)"), Cur));
}

void AMCToolStand::Use(AMCBayCharacter* Who)
{
	if (!Who) return;
	AMCConstructionManager* M = GetManager();
	// Cycle: nothing -> wrench -> cable -> nothing, skipping tools this role may not use.
	const EMCToolType Order[] = { EMCToolType::Wrench, EMCToolType::Cable };
	int32 Start = 0; // index of the first candidate after the current tool
	if (Who->HeldTool == EMCToolType::Wrench) Start = 1;
	else if (Who->HeldTool == EMCToolType::Cable) Start = 2;
	for (int32 i = Start; i < 2; ++i)
	{
		const int32 Dept = AMCToolItem::DepartmentOfTool(Order[i]);
		if (!M || M->RoleAllowed((EMCCrewRole)Who->GetCrewRole(), Dept))
		{
			Who->HeldTool = Order[i];
			if (M) M->Say(FString::Printf(TEXT("You took the %s."), AMCToolItem::ToolName(Order[i])));
			Super::Use(Who);
			return;
		}
	}
	if (Who->HeldTool != EMCToolType::None && M) M->Say(TEXT("Tool put back."));
	Who->HeldTool = EMCToolType::None;
	Super::Use(Who);
}

// ------------------------------------------------------------------ live status labels (every machine, from replicated state)

void AMCReadyButton::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AMCConstructionManager* M = GetManager();
	if (!M || !LabelText) return;
	const bool bOn = M->DeptReady.IsValidIndex(Department) && M->DeptReady[Department];
	LabelText->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s"), DepartmentName(Department), bOn ? TEXT("READY") : TEXT("NOT READY"))));
	LabelText->SetTextRenderColor(bOn ? FColor(60, 255, 90) : FColor(255, 80, 60));
}

void AMCLaunchButton::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const AMCConstructionManager* M = GetManager();
	if (!M || !LabelText) return;
	int32 Ready = 0;
	for (int32 d = 1; d < MCConst::NumRoles; ++d) if (M->DeptReady.IsValidIndex(d) && M->DeptReady[d]) ++Ready;
	LabelText->SetText(FText::FromString(FString::Printf(TEXT("LAUNCH\nready %d/4"), Ready)));
	LabelText->SetTextRenderColor(Ready >= 4 ? FColor(60, 255, 90) : FColor(255, 200, 60));
}
