#pragma once

#include "CoreMinimal.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

/** The small builders the pause hub's cards share (PauseMenuCards.cpp, PauseMenuMission.cpp, PauseMenuSession.cpp). */
namespace PauseMenuCardKit
{
	/** Bold or regular text in the kit's type, empty (no outline: the cards' words sit on glass). */
	inline UTextBlock* Words(UWidgetTree* Tree, int32 Size, bool bBold, const FLinearColor& Tint, int32 Spacing = 0)
	{
		UTextBlock* Text = LooterUI::MakeText(Tree, TEXT(""), Size, Tint, false, Spacing);
		Text->SetFont(LooterUI::Font(Size, bBold, Spacing));
		return Text;
	}

	inline void SetWords(const TWeakObjectPtr<UTextBlock>& Text, const FString& Value)
	{
		if (UTextBlock* Block = Text.Get())
		{
			Block->SetText(FText::FromString(Value));
		}
	}

	/** Child in Box, centred up and down, Left from what's before it. */
	inline UHorizontalBoxSlot* AddTo(UHorizontalBox* Box, UWidget* Child, float Left = 0.f)
	{
		UHorizontalBoxSlot* ChildSlot = Box->AddChildToHorizontalBox(Child);
		ChildSlot->SetVerticalAlignment(VAlign_Center);
		ChildSlot->SetPadding(FMargin(Left, 0.f, 0.f, 0.f));
		return ChildSlot;
	}

	/** A spring taking the room left in Box (what follows it sits at the right). */
	inline UHorizontalBoxSlot* AddFill(UWidgetTree* Tree, UHorizontalBox* Box)
	{
		UHorizontalBoxSlot* Spring = Box->AddChildToHorizontalBox(LooterUI::MakeSized(Tree, nullptr, 0.f));
		Spring->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		return Spring;
	}

	/** Content that shrinks rather than run past its room (a long hero or mission name). */
	inline UScaleBox* ShrinkToFit(UWidgetTree* Tree, UWidget* Content)
	{
		UScaleBox* Fit = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Fit->SetStretch(EStretch::ScaleToFit);
		Fit->SetStretchDirection(EStretchDirection::DownOnly);
		Fit->SetContent(Content);
		return Fit;
	}
}
