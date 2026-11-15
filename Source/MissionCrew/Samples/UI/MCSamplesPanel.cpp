#include "Samples/UI/MCSamplesPanel.h"
#include "Samples/MCSamplesManager.h"
#include "Samples/MCSamplesRelay.h"
#include "Transit/MCTransitManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UMCSamplesClickRelay::Fire() { if (UMCSamplesPanel* P = Owner.Get()) P->SendAction(Code); }

int32 UMCSamplesPanel::GetLocalRole() const { return AMCTransitManager::GetCrewRoleOfController(GetOwningPlayer()); }

void UMCSamplesPanel::SendAction(int32 Code)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC || !GetWorld()) return;
	for (TActorIterator<AMCSamplesRelay> It(GetWorld()); It; ++It)
		if (It->GetOwner() == PC) { It->ServerAction(Code); return; }
}

UTextBlock* UMCSamplesPanel::AddText(UVerticalBox* Box, int32 Size, FLinearColor Color)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo F = T->GetFont();
	F.Size = Size;
	T->SetFont(F);
	T->SetColorAndOpacity(FSlateColor(Color));
	T->SetAutoWrapText(true);
	T->SetWrapTextAt(380.f);
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(T)) S->SetPadding(FMargin(0.f, 3.f));
	return T;
}

UButton* UMCSamplesPanel::AddButton(UVerticalBox* Box, const FString& Label, int32 Code)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>();
	B->SetBackgroundColor(FLinearColor(0.08f, 0.16f, 0.26f, 1.f));
	UTextBlock* L = WidgetTree->ConstructWidget<UTextBlock>();
	L->SetText(FText::FromString(Label));
	L->SetMinDesiredWidth(300.f);
	L->SetJustification(ETextJustify::Center);
	B->AddChild(L);
	UMCSamplesClickRelay* R = NewObject<UMCSamplesClickRelay>(this);
	R->Owner = this;
	R->Code = Code;
	Relays.Add(R);
	B->OnClicked.AddDynamic(R, &UMCSamplesClickRelay::Fire);
	if (UVerticalBoxSlot* S = Box->AddChildToVerticalBox(B)) S->SetPadding(FMargin(0.f, 2.f));
	return B;
}

void UMCSamplesPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree || WidgetTree->RootWidget) return;
	UBorder* Bg = WidgetTree->ConstructWidget<UBorder>();
	Bg->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.10f, 0.88f));
	Bg->SetPadding(FMargin(14.f));
	UOverlay* Frame = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Frame;
	if (UOverlaySlot* FS = Cast<UOverlaySlot>(Frame->AddChild(Bg))) { FS->SetHorizontalAlignment(HAlign_Left); FS->SetVerticalAlignment(VAlign_Bottom); FS->SetPadding(FMargin(16.f)); }
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
	Bg->SetContent(Root);
	const FLinearColor Cyan(0.25f, 0.88f, 1.f, 1.f), Amber(1.f, 0.75f, 0.2f, 1.f), Dim(0.55f, 0.65f, 0.75f, 1.f);
	HeaderText = AddText(Root, 20, Cyan);
	MsgText = AddText(Root, 14, Amber);

	RoleBoxes.Init(nullptr, 5);
	RoleText.Init(nullptr, 5);
	RoutineBtn.Init(nullptr, 5);
	for (int32 R = 1; R <= 4; ++R)
	{
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>();
		Root->AddChildToVerticalBox(V);
		RoleBoxes[R] = V;
		RoleText[R] = AddText(V, 14, Dim);
		switch (R)
		{
		case 1:
			AddButton(V, TEXT("COOLING OFF"), 100); AddButton(V, TEXT("COOLING ECO"), 101);
			AddButton(V, TEXT("COOLING NOMINAL"), 102); AddButton(V, TEXT("COOLING BOOST"), 103);
			AddButton(V, TEXT("BACKUP COOLING"), 104); AddButton(V, TEXT("DEPLOY INSULATION COVER"), 105);
			break;
		case 2:
			AddButton(V, TEXT("Type: ICE"), 200); AddButton(V, TEXT("Type: REGOLITH"), 201); AddButton(V, TEXT("Type: ROCK"), 202);
			AddButton(V, TEXT("Store: FROZEN"), 203); AddButton(V, TEXT("Store: COOL"), 204); AddButton(V, TEXT("Store: AMBIENT"), 205);
			AddButton(V, TEXT("LABEL + STORE SAMPLE"), 206); AddButton(V, TEXT("UPDATE STORAGE CONDITIONS"), 207);
			break;
		case 3:
			AddButton(V, TEXT("Container: PASSIVE COLD BOX"), 300); AddButton(V, TEXT("Container: ACTIVE COOLER"), 301);
			AddButton(V, TEXT("Container: HYBRID CASE"), 302); AddButton(V, TEXT("PLACE SAMPLE"), 303);
			AddButton(V, TEXT("SEAL"), 304); AddButton(V, TEXT("PACKAGING CHECK"), 305);
			break;
		default:
			AddButton(V, TEXT("LOAD CONTAINER"), 400); AddButton(V, TEXT("SECURE CONTAINER"), 401);
			break;
		}
		RoutineBtn[R] = AddButton(V, TEXT("ROUTINE CHECK (when idle)"), R * 100 + 9);
	}
}

void UMCSamplesPanel::NativeTick(const FGeometry& G, float Dt)
{
	Super::NativeTick(G, Dt);
	Timer -= Dt;
	if (Timer > 0.f || !Manager || RoleBoxes.Num() < 5) return;
	Timer = 0.1f;
	const AMCSamplesManager& M = *Manager;
	const int32 Mine = GetLocalRole();
	static const TCHAR* Names[] = { TEXT("?"), TEXT("SYSTEMS OPERATOR"), TEXT("SAMPLE SCIENTIST"), TEXT("SAMPLE TECHNICIAN"), TEXT("CARGO OPERATOR") };
	if (HeaderText)
		HeaderText->SetText(FText::FromString(M.bDone ? FString(TEXT("S5 COMPLETE")) : FString::Printf(TEXT("S5 SAMPLES - %s  [%.0fs]"), Names[FMath::Clamp(Mine, 0, 4)], M.ElapsedSec)));
	if (MsgText) MsgText->SetText(FText::FromString(M.bDone ? M.ReportText : M.Message));
	for (int32 R = 1; R <= 4; ++R)
	{
		const bool bShow = M.bFreeRoles || R == Mine;
		RoleBoxes[R]->SetVisibility(bShow && !M.bDone ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!bShow) continue;
		if (RoutineBtn[R]) RoutineBtn[R]->SetIsEnabled(M.RoutineReady.IsValidIndex(R) && M.RoutineReady[R]);
		const FString Cont = M.ContainerNames.IsValidIndex(M.ContainerIdx) ? M.ContainerNames[M.ContainerIdx] : FString(TEXT("none yet"));
		FString S;
		switch (R)
		{
		case 1:
		{
			FString Alarm;
			if (M.bEventActive && !M.bEventFixed)
				Alarm = M.EventKind == EMCSamplesEvent::CoolerFault ? TEXT("\n!! COOLER FAULT: use BACKUP") : TEXT("\n!! HOLD TIME LOW: insulation");
			S = FString::Printf(TEXT("Container: %s\nTemp %.1f C  (limit %.1f)  hold %.0fs\nCooling: %s   Power %.0f/%.0f W   Mass %.1f kg\nIn range: %.0f%%%s"),
				*Cont, M.TempC, M.LimitC, M.HoldLeftSec, M.ModeNames.IsValidIndex(M.CoolMode) ? *M.ModeNames[M.CoolMode] : TEXT("-"),
				M.PowerW, M.PowerBudgetW, M.MassKg, M.InRangeFrac * 100.f, *Alarm);
			break;
		}
		case 2:
		{
			FString Hint = TEXT("no sample waiting");
			for (const FMCSample& X : M.Samples) if (X.Stage == EMCSampleStage::Raw) { Hint = FString::Printf(TEXT("Sample %d: %s"), X.Id, *X.Hint); break; }
			const FString Upd = (M.bEventActive && M.EventKind == EMCSamplesEvent::CoolerFault && !M.bStorageUpdated) ? FString(TEXT("\n!! Update storage conditions")) : FString();
			S = FString::Printf(TEXT("%s\nSelected: %s / %s\nWaiting: %d%s"), *Hint, *AMCSamplesManager::TypeName(M.PendType), *AMCSamplesManager::StorageName(M.PendCond), M.WaitSci, *Upd);
			break;
		}
		case 3:
			S = FString::Printf(TEXT("Container: %s\nQueue: %d"), M.ContainerIdx >= 0 ? *Cont : TEXT("choose one!"), M.WaitTech);
			for (const FMCSample& X : M.Samples) if (X.Stage == EMCSampleStage::Sealing) S += FString::Printf(TEXT("\nSealing %d: %.0f%%"), X.Id, X.SealProgress * 100.f);
			break;
		default:
			S = FString::Printf(TEXT("Ready to load/secure: %d\nSecured: %d / %d"), M.WaitCargo, M.CountSecured(), M.Samples.Num());
			break;
		}
		if (RoleText[R]) RoleText[R]->SetText(FText::FromString(S));
	}
}
