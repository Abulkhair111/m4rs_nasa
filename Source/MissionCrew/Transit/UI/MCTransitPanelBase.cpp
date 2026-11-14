#include "Transit/UI/MCTransitPanelBase.h"
#include "Transit/MCTransitManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

const TCHAR* MCTransitUI::StageName(EMCTransitStage Stage)
{
	switch (Stage)
	{
	case EMCTransitStage::GoPoll:    return TEXT("GO / NO-GO");
	case EMCTransitStage::Countdown: return TEXT("COUNTDOWN");
	case EMCTransitStage::Ascent:    return TEXT("ASCENT");
	case EMCTransitStage::Coast:     return TEXT("COAST TO MOON");
	case EMCTransitStage::Arrival:   return TEXT("ARRIVAL");
	}
	return TEXT("?");
}

void UMCTransitClickRelay::Fire()
{
	if (UMCTransitPanelBase* P = Owner.Get()) P->SendAction(Code);
}

void UMCTransitPanelBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (WidgetTree && !WidgetTree->RootWidget) BuildFallbackLayout();
	BindButtons();
}

void UMCTransitPanelBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Manager)
	{
		FindTimer -= InDeltaTime;
		if (FindTimer <= 0.f)
		{
			FindTimer = 0.5f;
			if (UWorld* W = GetWorld())
			{
				for (TActorIterator<AMCTransitManager> It(W); It; ++It) { Manager = *It; break; }
			}
		}
		return;
	}
	RefreshTimer -= InDeltaTime;
	if (RefreshTimer <= 0.f)
	{
		RefreshTimer = RefreshHz > 0.f ? 1.f / RefreshHz : 0.1f;
		RefreshPanel();
	}
}

int32 UMCTransitPanelBase::GetLocalRole() const
{
	return AMCTransitManager::GetCrewRoleOfController(GetOwningPlayer());
}

void MCTransitUI::SendToPawn(APlayerController* PC, int32 Code, AMCTransitManager* Manager)
{
	if (!PC) return;
	struct FParams { int32 Code; };
	FParams Params{ Code };
	static const FName FuncName(TEXT("ServerTransitAction"));
	if (APawn* Pawn = PC->GetPawn())
	{
		if (UFunction* F = Pawn->FindFunction(FuncName)) { Pawn->ProcessEvent(F, &Params); return; }
	}
	if (UFunction* F = PC->FindFunction(FuncName)) { PC->ProcessEvent(F, &Params); return; }
	if (Manager && Manager->HasAuthority()) Manager->HandleAction(Code, AMCTransitManager::GetCrewRoleOfController(PC));
}

void UMCTransitPanelBase::SendAction(int32 Code)
{
	MCTransitUI::SendToPawn(GetOwningPlayer(), Code, Manager);
}

void UMCTransitPanelBase::BindButton(UButton* Button, int32 Code)
{
	if (!Button) return;
	UMCTransitClickRelay* R = NewObject<UMCTransitClickRelay>(this);
	R->Owner = this;
	R->Code = Code;
	Relays.Add(R);
	Button->OnClicked.AddDynamic(R, &UMCTransitClickRelay::Fire);
}

UVerticalBox* UMCTransitPanelBase::MakeRootBox(const TCHAR* Title, FLinearColor TitleColor)
{
	UBorder* Bg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBg"));
	Bg->SetBrushColor(MCTransitUI::Navy());
	Bg->SetPadding(FMargin(14.f));
	WidgetTree->RootWidget = Bg;
	UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelVB"));
	Bg->SetContent(VB);
	UTextBlock* T = MakeText(TEXT("TitleText"), 22, TitleColor);
	T->SetText(FText::FromString(Title));
	VB->AddChildToVerticalBox(T);
	return VB;
}

UTextBlock* UMCTransitPanelBase::MakeText(const TCHAR* Name, int32 Size, FLinearColor Color)
{
	UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	FSlateFontInfo F = T->GetFont();
	F.Size = Size;
	T->SetFont(F);
	T->SetColorAndOpacity(FSlateColor(Color));
	return T;
}

UButton* UMCTransitPanelBase::MakeButton(const TCHAR* Name, const FString& Label, UTextBlock*& OutLabel, float MinWidth)
{
	UButton* B = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	B->SetBackgroundColor(MCTransitUI::BtnOff());
	OutLabel = MakeText(*(FString(Name) + TEXT("_Label")), 16, FLinearColor::White);
	OutLabel->SetText(FText::FromString(Label));
	OutLabel->SetMinDesiredWidth(MinWidth);
	OutLabel->SetJustification(ETextJustify::Center);
	B->AddChild(OutLabel);
	return B;
}
