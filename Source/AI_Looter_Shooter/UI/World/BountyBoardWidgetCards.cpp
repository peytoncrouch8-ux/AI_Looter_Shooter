// UBountyBoardWidget's cards: a posting's card (its tier, state, mark, traits, posse, flavour, where, pay) or an empty
// slot's, the chosen one outlined in orange, made again only when their words or the choice changed.

#include "UI/World/BountyBoardWidget.h"
#include "Bounties/BountyBoard.h"
#include "Bounties/BountyBoardSubsystem.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "LooterBounty"

using namespace LooterUI;

namespace
{
	const FName ActionCard(TEXT("Card"));

	constexpr float CardWidth = 384.f;
	constexpr float CardGap = 16.f;
	constexpr float CardMinHeight = 372.f;
	constexpr float CardWrap = CardWidth - 36.f;
	constexpr float PipSize = 12.f;

	/** A tier's colour on the board: its mark's rank's tag colour. */
	FLinearColor TierColor(EBountyTier Tier)
	{
		switch (Tier)
		{
		case EBountyTier::Dangerous:
			return Color::TierHallowed();
		case EBountyTier::Deadly:
			return Color::TierEthereal();
		default:
			return Color::TierGravebound();
		}
	}

	/** Its state word's colour: new orange, taken cyan, won green. */
	FLinearColor StateColor(const FBountyCardView& Card)
	{
		switch (Card.State)
		{
		case EBountyState::Taken:
			return Color::CyanText();
		case EBountyState::Won:
			return Color::Better();
		default:
			return Color::Accent();
		}
	}

	UTextBlock* Wrapped(UWidgetTree* Tree, const FText& Words, int32 Size, const FLinearColor& Tint)
	{
		UTextBlock* Text = MakeText(Tree, Words.ToString(), Size, Tint);
		Text->SetAutoWrapText(true);
		Text->SetWrapTextAt(CardWrap);
		return Text;
	}
}

FString UBountyBoardWidget::Signature(const FBountyCardView& Card)
{
	return FString::Printf(TEXT("%d|%d|%d|%s|%s|%s|%s|%s|%d%d%d%d"), Card.bEmpty ? 1 : 0, static_cast<int32>(Card.Tier), static_cast<int32>(Card.State),
		*Card.Title.ToString(), *Card.Reward.ToString(), *Card.StateWord.ToString(), *Card.EmptyLine.ToString(), *Card.Traits.ToString(),
		Card.bCanTake ? 1 : 0, Card.bCanCollect ? 1 : 0, Card.bCanGiveUp ? 1 : 0, Card.bNew ? 1 : 0);
}

void UBountyBoardWidget::ShowCards(bool bForce)
{
	const UBountyBoardSubsystem* Bounties = GetBounties();
	if (!CardRow || !Bounties)
	{
		return;
	}
	const FBountyBoardRecord& Record = Bounties->GetRecord();
	if (RotationText)
	{
		RotationText->SetText(Bounties->DescribeNextRotation());
	}
	if (PaidText)
	{
		PaidText->SetText(Record.Paid > 0 ? FText::Format(LOCTEXT("PaidCount", "Paid out: {0}"), FText::AsNumber(Record.Paid)) : FText::GetEmpty());
	}
	TArray<FBountyCardView> Cards = Bounties->MakeCards();
	FString Now;
	for (const FBountyCardView& Card : Cards)
	{
		Now += Signature(Card);
		Now += TEXT("#");
	}
	if (!bForce && Now == ShownSignature && Selected == ShownSelected)
	{
		return;
	}
	Shown = MoveTemp(Cards);
	ShownSignature = MoveTemp(Now);
	ShownSelected = Selected;
	CardRow->ClearChildren();
	for (int32 CardIndex = 0; CardIndex < Shown.Num(); ++CardIndex)
	{
		UWidget* Card = MakeCard(CardIndex, Shown[CardIndex], CardIndex == Selected);
		ULooterButton* Click = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		Click->SetupContent(Card, ActionCard, CardIndex, EButtonKind::Bare);
		Click->bPlaysSounds = false;
		Click->OnButtonClicked.BindUObject(this, &UBountyBoardWidget::HandleButton);
		UHorizontalBoxSlot* CardSlot = CardRow->AddChildToHorizontalBox(MakeSized(WidgetTree, Click, CardWidth));
		CardSlot->SetPadding(FMargin(CardIndex == 0 ? 0.f : CardGap, 0.f, 0.f, 0.f));
		CardSlot->SetVerticalAlignment(VAlign_Fill);
	}
	MakeButtons();
}

UWidget* UBountyBoardWidget::MakeCardHeader(const FBountyCardView& Card)
{
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// Its tier's diamonds, one to three, and its word, in the mark's tier colour.
	const FLinearColor Tint = TierColor(Card.Tier);
	const int32 Pips = static_cast<int32>(Card.Tier) + 1;
	for (int32 Pip = 0; Pip < Pips; ++Pip)
	{
		UImage* Diamond = MakeImage(WidgetTree, MarkerBrush(EMarker::Diamond, Tint));
		UHorizontalBoxSlot* PipSlot = Header->AddChildToHorizontalBox(MakeSized(WidgetTree, Diamond, PipSize, PipSize));
		PipSlot->SetVerticalAlignment(VAlign_Center);
		PipSlot->SetPadding(FMargin(Pip == 0 ? 0.f : 3.f, 0.f, 0.f, 0.f));
	}
	UTextBlock* Word = MakeText(WidgetTree, Card.TierWord.ToString(), 12, Tint, true, 180);
	UHorizontalBoxSlot* WordSlot = Header->AddChildToHorizontalBox(Word);
	WordSlot->SetVerticalAlignment(VAlign_Center);
	WordSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	Header->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	if (!Card.StateWord.IsEmpty())
	{
		UTextBlock* State = MakeText(WidgetTree, Card.StateWord.ToString(), 11, StateColor(Card), true, 160);
		Header->AddChildToHorizontalBox(MakeShapeBox(WidgetTree, EShape::Control, Color::Plate(), StateColor(Card), State,
			FMargin(8.f, 2.f)))->SetVerticalAlignment(VAlign_Center);
	}
	return Header;
}

UWidget* UBountyBoardWidget::MakeCard(int32 CardIndex, const FBountyCardView& Card, bool bChosen)
{
	UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (Card.bEmpty)
	{
		// An empty slot: a bare tack on the board, and when the next goes up.
		UTextBlock* None = MakeText(WidgetTree, LOCTEXT("EmptyTitle", "No posting").ToString(), 16, Color::TextDim(), true, 120);
		Body->AddChildToVerticalBox(None)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		Body->AddChildToVerticalBox(Wrapped(WidgetTree, Card.EmptyLine, 12, Color::TextDim()))->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
	}
	else
	{
		Body->AddChildToVerticalBox(MakeCardHeader(Card));
		// The mark: its name large, its rank, kind and levels under it.
		UTextBlock* Name = MakeText(WidgetTree, Card.Title.ToString(), 22, Color::Title(), false, 20);
		Name->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
		Body->AddChildToVerticalBox(Name)->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		Body->AddChildToVerticalBox(Wrapped(WidgetTree, Card.KindLine, 12, Color::TextDim()))->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

		// What it can do: its traits, then its posse's twist.
		Body->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("Traits", "Known for").ToString()))->SetPadding(FMargin(0.f, 12.f, 0.f, 4.f));
		Body->AddChildToVerticalBox(Wrapped(WidgetTree, Card.Traits, 13, Color::CyanText()));
		if (!Card.Posse.IsEmpty())
		{
			Body->AddChildToVerticalBox(Wrapped(WidgetTree, Card.Posse, 12, Color::Accent()))->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		}
		Body->AddChildToVerticalBox(Wrapped(WidgetTree, Card.Flavour, 11, Color::TextDim()))->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		Body->AddChildToVerticalBox(Wrapped(WidgetTree, Card.Where, 12, Color::Text()))->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

		// What it pays.
		Body->AddChildToVerticalBox(MakeSection(WidgetTree, LOCTEXT("Pays", "Pays").ToString()))->SetPadding(FMargin(0.f, 12.f, 0.f, 4.f));
		Body->AddChildToVerticalBox(Wrapped(WidgetTree, Card.Reward, 13, Color::Better()));
	}
	// The chosen card's action, at its foot.
	if (bChosen && !Card.bEmpty)
	{
		FText Action;
		if (Card.bCanCollect)
		{
			Action = LOCTEXT("CardCollect", "[E] Collect the bounty");
		}
		else if (Card.bCanTake)
		{
			Action = LOCTEXT("CardTake", "[E] Take it");
		}
		else if (Card.bCanGiveUp)
		{
			Action = LOCTEXT("CardGiveUp", "[R] twice to give it up");
		}
		else
		{
			Action = LOCTEXT("CardBusy", "Another bounty is under way");
		}
		USizeBox* Spacer = MakeSized(WidgetTree, nullptr, 0.f, 0.f);
		Body->AddChildToVerticalBox(Spacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UTextBlock* ActionText = MakeText(WidgetTree, Action.ToString(), 12, Card.bCanTake || Card.bCanCollect ? Color::Accent() : Color::TextDim(), true, 100);
		Body->AddChildToVerticalBox(ActionText)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
	USizeBox* Sized = MakeSized(WidgetTree, MakeShapeBox(WidgetTree, EShape::Control, Color::Tile(), bChosen ? Color::Focus() : Color::TileLine(), Body,
		FMargin(16.f, 14.f)), CardWidth);
	Sized->SetMinDesiredHeight(CardMinHeight);
	return Sized;
}

#undef LOCTEXT_NAMESPACE
