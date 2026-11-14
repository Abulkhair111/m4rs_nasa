// Small code-UI helpers shared by the display widgets (agent I2). Header only.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Construction/MCConstructionTypes.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/ProgressBar.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"

namespace MCDisplay
{
	inline UTextBlock* MakeText(UWidgetTree* Tree, const TCHAR* Name, int32 Size, const FLinearColor& Col = FLinearColor::White, bool bWrap = false)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		FSlateFontInfo F = T->GetFont();
		F.Size = Size;
		T->SetFont(F);
		T->SetColorAndOpacity(FSlateColor(Col));
		T->SetAutoWrapText(bWrap);
		return T;
	}

	/** Dark translucent panel (root of a display). */
	inline UBorder* MakePanel(UWidgetTree* Tree, const TCHAR* Name, float Alpha = 0.85f)
	{
		UBorder* B = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		B->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.05f, Alpha));
		B->SetPadding(FMargin(32.f));
		return B;
	}

	/** Square colour swatch of a fixed size. Returns the SizeBox; the border is written to OutBorder. */
	inline USizeBox* MakeSwatch(UWidgetTree* Tree, const TCHAR* Name, float Size, UBorder*& OutBorder)
	{
		USizeBox* SB = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SB->SetWidthOverride(Size);
		SB->SetHeightOverride(Size);
		OutBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		OutBorder->SetBrushColor(FLinearColor::Gray);
		SB->AddChild(OutBorder);
		return SB;
	}

	inline FString CheckStateTag(EMCCheckState S)
	{
		return S == EMCCheckState::Green ? TEXT("OK") : (S == EMCCheckState::Yellow ? TEXT("WARN") : TEXT("FAIL"));
	}
}
