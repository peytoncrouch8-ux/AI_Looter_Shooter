// UHeroSelectWidget: the roster under the stage, a card per hero (the portrait's band at the eyes, the number key, a
// strip in their colour, name and role; greyed and stamped for a hero in the works), and the cards' look as the choice
// and the mouse move.

#include "UI/Menus/HeroSelectWidget.h"
#include "Embers/HeroDefinition.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Menus/HeroSelectParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

using namespace LooterUI;
using namespace HeroSelectParts;

namespace
{
	const FName ActionHero(TEXT("Hero"));

	/** The cards on the page (1600 x 900), side by side under the stage, and the portrait's band on each. */
	constexpr float CardWidth = 176.f;
	constexpr float CardHeight = 140.f;
	constexpr float CardGap = 12.f;
	const FVector2D BandSize(164.f, 80.f);
}

void UHeroSelectWidget::RebuildRoster()
{
	if (!RosterRow)
	{
		return;
	}
	RosterRow->ClearChildren();
	Cards.Reset();
	for (int32 Index = 0; Index < Heroes.Num(); ++Index)
	{
		FCard Card = MakeCard(Index, *Heroes[Index]);
		RosterRow->AddChildToHorizontalBox(MakeSized(WidgetTree, Card.Button, CardWidth, CardHeight))
			->SetPadding(FMargin(Index > 0 ? CardGap : 0.f, 0.f, 0.f, 0.f));
		Cards.Add(Card);
	}
	HoveredCard = INDEX_NONE;
	RestyleCards();
}

UHeroSelectWidget::FCard UHeroSelectWidget::MakeCard(int32 Index, const UHeroDefinition& Hero)
{
	FCard Card;
	const bool bPickable = CanConfirm(Index);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// The portrait's band at the eyes, its number key in the corner, and the stamp on a hero in the works.
	{
		UOverlay* Band = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		FillOverlaySlot(Band->AddChildToOverlay(MakePortrait(WidgetTree, Portraits.IsValidIndex(Index) ? Portraits[Index].Get() : nullptr,
			BandSize, ECrop::Band, !bPickable, Hero.Color)));
		UBorder* Cap = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Cap->SetBrush(RectBrush(Color::Plate(), Color::ScreenLine(), 1.f));
		MarkBackground(Cap);
		Cap->SetPadding(FMargin(5.f, 0.f, 5.f, 1.f));
		Cap->SetContent(MakeText(WidgetTree, FString::FromInt(Index + 1), 9, Color::Title(), true));
		UOverlaySlot* CapSlot = Band->AddChildToOverlay(Cap);
		CapSlot->SetHorizontalAlignment(HAlign_Left);
		CapSlot->SetVerticalAlignment(VAlign_Top);
		CapSlot->SetPadding(FMargin(4.f));
		if (!bPickable)
		{
			UOverlaySlot* StampSlot = Band->AddChildToOverlay(MakeWorksStamp(WidgetTree, 8));
			StampSlot->SetHorizontalAlignment(HAlign_Center);
			StampSlot->SetVerticalAlignment(VAlign_Bottom);
			StampSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}
		Column->AddChildToVerticalBox(MakeSized(WidgetTree, Band, BandSize.X, BandSize.Y));
	}
	// A strip in the hero's colour (the kit's dim line for one in the works), then the name and the role.
	Column->AddChildToVerticalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(bPickable ? Hero.Color : Color::RowLine())), BandSize.X, 3.f));
	Card.Name = LoadoutParts::FittedLabel(WidgetTree, Hero.DisplayName.ToString(), 13, bPickable ? Color::Title() : Color::TextDim(), 60);
	Column->AddChildToVerticalBox(Card.Name)->SetPadding(FMargin(2.f, 7.f, 2.f, 0.f));
	Column->AddChildToVerticalBox(LoadoutParts::FittedLabel(WidgetTree, Hero.Role.ToString(), 9, bPickable ? Color::CyanText() : Color::TextDim(), 200))
		->SetPadding(FMargin(2.f, 1.f, 2.f, 0.f));

	UOverlay* Box = LoadoutParts::MakeCard(WidgetTree, Column, FMargin(6.f, 6.f, 6.f, 8.f), 1.5f, Card.Fill, Card.Line);
	// The keys' focus look over the card (a bare button draws none of its own).
	Card.FocusLook = MakeFocusFrame(WidgetTree, false, false);
	FillOverlaySlot(Box->AddChildToOverlay(Card.FocusLook));

	Card.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	// The screen sounds the choice itself (a hover as it moves, a click for the mouse), so the card makes none.
	Card.Button->bPlaysSounds = false;
	Card.Button->SetupContent(Box, ActionHero, Index);
	Card.Button->SetCursor(EMouseCursor::Hand);
	Card.Button->OnButtonClicked.BindUObject(this, &UHeroSelectWidget::HandleButton);
	Card.Button->OnButtonHovered.BindUObject(this, &UHeroSelectWidget::HandleButtonHovered);
	return Card;
}

void UHeroSelectWidget::RestyleCards()
{
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		const FCard& Card = Cards[Index];
		if (!Card.Fill || !Card.Line)
		{
			continue;
		}
		// The chosen card's edge is orange on the lit fill; one under the mouse brightens its cyan edge.
		const bool bChosen = Index == Selected;
		const bool bHovered = Index == HoveredCard;
		Card.Fill->SetColorAndOpacity(bChosen ? Color::Tile() : (bHovered ? Color::Row() : LoadoutParts::Colors::CardFill()));
		Card.Line->SetColorAndOpacity(bChosen ? Color::Accent() : (bHovered ? Color::TileLine() : LoadoutParts::Colors::CardLine()));
	}
}
