#include "Bay/MCPartPickerWidget.h"
#include "Bay/MCBayCharacter.h"
#include "Bay/MCBayUI.h"
#include "Bay/MCRocketStack.h"
#include "Construction/MCConstructionManager.h"
#include "Construction/MCPartSocket.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

void UMCPickerRowProxy::HandleClicked()
{
	if (UMCPartPickerWidget* P = Owner.Get())
	{
		P->SelectIndex(Index);
	}
}

void UMCPartPickerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!PartList && WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
	}
	if (ConfirmButton) ConfirmButton->OnClicked.AddDynamic(this, &UMCPartPickerWidget::HandleConfirm);
	if (CancelButton) CancelButton->OnClicked.AddDynamic(this, &UMCPartPickerWidget::HandleCancel);
}

void UMCPartPickerWidget::NativeDestruct()
{
	if (AMCRocketStack* RS = AMCRocketStack::Find(GetWorld())) RS->ClearGhost();
	ClearRows();
	Super::NativeDestruct();
}

void UMCPartPickerWidget::BuildFallbackLayout()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FallbackCanvas"));
	WidgetTree->RootWidget = Canvas;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FallbackPanel"));
	Panel->SetBrushColor(FLinearColor(0.02f, 0.04f, 0.09f, 0.94f));
	Panel->SetPadding(FMargin(16.f));
	if (UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Panel))
	{
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetPosition(FVector2D::ZeroVector);
		S->SetSize(FVector2D(760.f, 640.f));
	}

	UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FallbackVB"));
	Panel->SetContent(VB);

	auto AddText = [&](TObjectPtr<UTextBlock>& Out, const TCHAR* Name, int32 Size) {
		Out = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo F = Out->GetFont();
		F.Size = Size;
		Out->SetFont(F);
		Out->SetAutoWrapText(true);
		if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(Out)) S->SetPadding(FMargin(0, 3));
	};

	AddText(TitleText, TEXT("TitleText"), 22);

	USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ListBox"));
	Box->SetHeightOverride(300.f);
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("PartList"));
	Box->AddChild(Scroll);
	PartList = Scroll;
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(Box)) S->SetPadding(FMargin(0, 6));

	AddText(DetailText, TEXT("DetailText"), 14);
	AddText(TWRText, TEXT("TWRText"), 18);
	AddText(FuelText, TEXT("FuelText"), 14);
	AddText(PowerText, TEXT("PowerText"), 14);
	AddText(CostText, TEXT("CostText"), 14);

	UHorizontalBox* HB = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	if (UVerticalBoxSlot* S = VB->AddChildToVerticalBox(HB)) S->SetPadding(FMargin(0, 10, 0, 0));

	auto AddButton = [&](TObjectPtr<UButton>& Out, const TCHAR* Name, const TCHAR* Label) {
		Out = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(Label));
		Out->AddChild(T);
		if (UHorizontalBoxSlot* S = HB->AddChildToHorizontalBox(Out)) { S->SetPadding(FMargin(6)); S->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); }
	};
	AddButton(ConfirmButton, TEXT("ConfirmButton"), TEXT("Confirm"));
	AddButton(CancelButton, TEXT("CancelButton"), TEXT("Cancel"));
}

void UMCPartPickerWidget::Open(AMCPartSocket* InSocket, AMCBayCharacter* InCharacter)
{
	Socket = InSocket;
	Character = InCharacter;
	Manager = MCBayUI::FindManager(GetWorld());
	Candidates.Reset();
	if (Manager && Socket)
	{
		const EMCCrewRole Role = Character ? (EMCCrewRole)Character->GetCrewRole() : EMCCrewRole::None;
		for (UMCPartDef* D : Manager->GetCandidates(Socket->SocketType, Role, Socket->GetGroup()))
		{
			Candidates.Add(D);
		}
	}
	if (TitleText && Socket)
	{
		TitleText->SetText(FText::FromString(FString::Printf(TEXT("%s - %s for the %s socket"), MCBayUI::GroupName(Socket->GetGroup()),
			Socket->Installed ? TEXT("swap the part") : TEXT("choose a part"), MCBayUI::SocketTypeName(Socket->SocketType))));
			TitleText->SetColorAndOpacity(FSlateColor(MCBayUI::GroupColor(Socket->GetGroup())));
	}
	PopulateRows();
	if (!IsInViewport()) AddToViewport(50);
	if (Character) Character->SetCursorMode(true);
	SelectIndex(Candidates.Num() > 0 ? 0 : INDEX_NONE);
	BP_OnOpened(Candidates.Num());
}

void UMCPartPickerWidget::ClearRows()
{
	if (PartList) PartList->ClearChildren();
	RowButtons.Reset();
	RowProxies.Reset();
}

void UMCPartPickerWidget::PopulateRows()
{
	ClearRows();
	if (!PartList) return;
	for (int32 i = 0; i < Candidates.Num(); ++i)
	{
		const UMCPartDef* D = Candidates[i];
		UButton* Btn = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetAutoWrapText(true);
		T->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s\n%s"), *D->DisplayName.ToString(), *MCBayUI::PartOneLiner(D), *MCBayUI::PartNumbers(D))));
		Btn->AddChild(T);
		UMCPickerRowProxy* Proxy = NewObject<UMCPickerRowProxy>(this);
		Proxy->Owner = this;
		Proxy->Index = i;
		Btn->OnClicked.AddDynamic(Proxy, &UMCPickerRowProxy::HandleClicked);
		PartList->AddChild(Btn);
		RowButtons.Add(Btn);
		RowProxies.Add(Proxy);
	}
	if (Candidates.Num() == 0)
	{
		UTextBlock* T = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(TEXT("No parts available for this socket and your role.")));
		PartList->AddChild(T);
	}
}

UMCPartDef* UMCPartPickerWidget::GetSelectedDef() const
{
	return Candidates.IsValidIndex(SelectedIndex) ? Candidates[SelectedIndex].Get() : nullptr;
}

void UMCPartPickerWidget::SelectIndex(int32 Index)
{
	SelectedIndex = Candidates.IsValidIndex(Index) ? Index : INDEX_NONE;
	for (int32 i = 0; i < RowButtons.Num(); ++i)
	{
		if (RowButtons[i])
		{
			RowButtons[i]->SetBackgroundColor(i == SelectedIndex ? FLinearColor(0.2f, 0.7f, 1.f, 1.f) : FLinearColor::White);
		}
	}
	UpdatePreview();
}

void UMCPartPickerWidget::UpdatePreview()
{
	const UMCPartDef* D = GetSelectedDef();
	if (AMCRocketStack* RS = AMCRocketStack::Find(GetWorld()))
	{
		if (D)
		{
			RS->SetGhostReplaces(Socket && Socket->Installed ? Socket.Get() : nullptr); // swap: the ghost replaces the installed part
			RS->ShowGhost(D);
		}
		else RS->ClearGhost();
	}
	if (ConfirmButton) ConfirmButton->SetIsEnabled(D != nullptr);
	if (!D)
	{
		if (DetailText) DetailText->SetText(FText::GetEmpty());
		if (TWRText) TWRText->SetText(FText::GetEmpty());
		if (FuelText) FuelText->SetText(FText::GetEmpty());
		if (PowerText) PowerText->SetText(FText::GetEmpty());
		if (CostText) CostText->SetText(FText::GetEmpty());
		return;
	}
	if (DetailText)
	{
		DetailText->SetText(FText::FromString(FString::Printf(TEXT("%s%s\n%s\n%s"),
			*D->DisplayName.ToString(),
			D->RealAnalog.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" (%s)"), *D->RealAnalog.ToString()),
			*MCBayUI::PartOneLiner(D), *MCBayUI::PartNumbers(D))));
	}
	if (!Manager) return;

	const FMCPreviewResult R = Manager->PreviewSwap(D, Socket ? Socket->Installed.Get() : nullptr); // swap: subtract the replaced part
	if (TWRText)
	{
		TWRText->SetText(FText::FromString(FString::Printf(TEXT("Predicted T/W: %.2f  (needs >= %.1f, good >= %.1f)"), R.TWR, MCConst::TWRRed, MCConst::TWRGreen)));
		TWRText->SetColorAndOpacity(FSlateColor(MCBayUI::TWRColor(R.TWR)));
	}
	if (FuelText)
	{
		FuelText->SetText(FText::FromString(FString::Printf(TEXT("Fuel margin: %+.0f%%"), R.FuelMargin * 100.f)));
		FuelText->SetColorAndOpacity(FSlateColor(MCBayUI::CheckColor(R.FuelState)));
	}
	if (PowerText)
	{
		PowerText->SetText(FText::FromString(FString::Printf(TEXT("Power balance: %+.0f W"), R.PowerBalance)));
		PowerText->SetColorAndOpacity(FSlateColor(MCBayUI::CheckColor(R.PowerState)));
	}
	if (CostText)
	{
		CostText->SetText(FText::FromString(FString::Printf(TEXT("%s %.0f kg (take-off mass %.0f kg)   Cost $%.0fM / $%.0fM%s"),
			MCBayUI::GroupName(R.Group), R.Group == EMCStackGroup::Ascent ? R.Ascent.MassKg : R.Lander.MassKg, R.TotalMassKg,
			R.TotalCostM, Manager->BudgetM, R.bAffordable ? TEXT("") : TEXT("   OVER BUDGET"))));
		CostText->SetColorAndOpacity(FSlateColor(R.bAffordable ? FLinearColor::White : MCBayUI::CheckColor(EMCCheckState::Red)));
	}
	if (ConfirmButton) ConfirmButton->SetIsEnabled(R.bAffordable);
}

void UMCPartPickerWidget::HandleConfirm() { Confirm(); }
void UMCPartPickerWidget::HandleCancel() { Cancel(); }

void UMCPartPickerWidget::Confirm()
{
	UMCPartDef* D = GetSelectedDef();
	if (D && Socket && Character)
	{
		if (Socket->Installed)
		{
			if (Socket->Installed != D) Character->ServerSwapPart(Socket, D);
		}
		else
		{
			Character->ServerPlacePart(Socket, D);
		}
	}
	Close();
}

void UMCPartPickerWidget::Cancel() { Close(); }

void UMCPartPickerWidget::Close()
{
	AMCBayCharacter* C = Character;
	Character = nullptr;
	if (AMCRocketStack* RS = AMCRocketStack::Find(GetWorld())) RS->ClearGhost();
	RemoveFromParent();
	if (C) C->OnPickerClosed(this);
}
