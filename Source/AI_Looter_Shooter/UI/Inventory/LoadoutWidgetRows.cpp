// ULoadoutWidget: the list of everything carried (an equip slot's row, a backpack gun's row) and how each looks as the
// cursor moves. The counts, the ammo line and the prompt bar are LoadoutWidgetPrompts.cpp.

#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	/** The rarity stripe down a row's left edge, and how far it keeps clear of the row's cut top-left corner. */
	constexpr float StripeWidth = 4.f;
	constexpr float StripeTop = 10.f;
	/** A row's fill carries a breath of its gun's rarity, so the list reads by colour at a glance. */
	constexpr float RarityTint = 0.08f;

	// The room a gun's name has on its row, worked out from the row's parts so a long name is fitted before it's drawn
	// (LoadoutParts::FitTextToWidth) and never runs over the damage beside it.
	/** A row's content padding: the stripe's side, and the right. */
	constexpr float RowPaddingLeft = 14.f + StripeWidth;
	constexpr float RowPaddingRight = 12.f;
	/** A backpack row also gives up its scroll box slot's right padding and the scroll bar. */
	constexpr float ListRightGap = 16.f;
	/** Slot rows: the number badge and its gap, the icon, the words' padding either side, the damage's size. */
	constexpr float SlotBadgeRoom = 26.f + 12.f;
	constexpr float SlotIconWidth = 112.f;
	constexpr float SlotIconHeight = 38.f;
	constexpr float SlotWordsPadding = 14.f + 10.f;
	constexpr int32 SlotNameSize = 12;
	constexpr int32 SlotNameMinSize = 9;
	constexpr int32 SlotNumberSize = 17;
	constexpr float SlotNumberMinWidth = 64.f;
	/** Backpack rows: the icon, the words' padding, the verdict mark and its gap, the damage's size. */
	constexpr float PackIconWidth = 78.f;
	constexpr float PackIconHeight = 26.f;
	constexpr float PackWordsPadding = 12.f + 8.f;
	constexpr float PackMarkRoom = 18.f + 8.f;
	constexpr int32 PackNameSize = 10;
	constexpr int32 PackNameMinSize = 8;
	constexpr int32 PackNumberSize = 13;
	constexpr float PackNumberMinWidth = 58.f;
	/** The NEW tag and its gap, left free beside a new gun's name. */
	constexpr float NewMarkRoom = 40.f;

	/** The damage column: as wide as its number is drawn ("125 x12" is wider than "34"), and never narrower than MinWidth. */
	float NumberColumnWidth(const FString& Number, int32 Size, float MinWidth)
	{
		return FMath::Max(MinWidth, FMath::CeilToFloat(LoadoutParts::MeasureText(Number, LooterUI::Font(Size)) + 4.f));
	}

	/** "Assault Rifle": the gun's kind as its definition names it. */
	FString KindLabel(const FWeaponInstanceData& Gun)
	{
		return Gun.Definition ? Gun.Definition->DisplayName.ToString() : FString();
	}

	/** NEW: a small cyan tag on a gun found since the screen was last looked through. */
	UWidget* MakeNewMark(UWidgetTree* Tree)
	{
		UTextBlock* Word = Label(Tree, TEXT("New"), 7, Color::Track(), 150);
		Word->SetShadowColorAndOpacity(FLinearColor::Transparent);
		UBorder* Tag = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Tag->SetBrush(RectBrush(Color::CyanText()));
		Tag->SetPadding(FMargin(5.f, 0.f));
		Tag->SetVerticalAlignment(VAlign_Center);
		Tag->SetContent(Word);
		Tag->SetVisibility(ESlateVisibility::Collapsed);
		return Tag;
	}

	/** A name and the NEW tag after it, the name taking the room there is. */
	UWidget* NameWithMark(UWidgetTree* Tree, UWidget* Name, UWidget* Mark)
	{
		UHorizontalBox* Line = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(Name);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* MarkSlot = Line->AddChildToHorizontalBox(MakeSized(Tree, Mark, 0.f, 13.f));
		MarkSlot->SetVerticalAlignment(VAlign_Center);
		MarkSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		return Line;
	}

	/** A number over a small word, right-aligned: "34 / DMG". */
	UWidget* KeyNumber(UWidgetTree* Tree, const FString& Number, int32 Size, const TCHAR* Word)
	{
		UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UTextBlock* Value = MakeText(Tree, Number, Size, Color::Text());
		Value->SetJustification(ETextJustify::Right);
		Box->AddChildToVerticalBox(Value)->SetHorizontalAlignment(HAlign_Right);
		if (Word)
		{
			UTextBlock* Under = Label(Tree, Word, 7, Color::TextDim(), 200);
			Under->SetJustification(ETextJustify::Right);
			Box->AddChildToVerticalBox(Under)->SetHorizontalAlignment(HAlign_Right);
		}
		return Box;
	}
}

// ---------------------------------------------------------------------------
// Rows
// ---------------------------------------------------------------------------

ULoadoutWidget::FRow ULoadoutWidget::WrapRow(UWidget* Content, const FLinearColor& Rarity, bool bHasGun, float Height, FName Action, int32 Index)
{
	FRow Row;
	Row.Rarity = Rarity;
	UOverlay* Box = MakeCard(WidgetTree, Content, FMargin(14.f + StripeWidth, 6.f, 12.f, 6.f), 1.3f, Row.Fill, Row.Line);

	// Over the fill, a breath of the rarity (a background, so the transparency setting fades it with the fill) and the
	// rarity stripe down the left edge.
	if (bHasGun)
	{
		FSlateBrush TintBrush = ShapeBrush(EShape::Control, false, Rarity * FLinearColor(1.f, 1.f, 1.f, RarityTint));
		TintBrush.ImageSize *= 1.3f;
		UImage* Tint = MakeImage(WidgetTree, TintBrush);
		MarkBackground(Tint);
		Box->InsertChildAt(1, Tint);
		if (UOverlaySlot* TintSlot = Cast<UOverlaySlot>(Tint->Slot))
		{
			FillOverlaySlot(TintSlot);
		}
		UOverlaySlot* StripeSlot = Box->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Rarity)), StripeWidth, 0.f));
		StripeSlot->SetHorizontalAlignment(HAlign_Left);
		StripeSlot->SetVerticalAlignment(VAlign_Fill);
		StripeSlot->SetPadding(FMargin(1.f, StripeTop, 0.f, 3.f));
	}

	// The flash as a gun lands here: the row's shape in the rarity's colour, faded out by TickMotion.
	FSlateBrush FlashBrush = ShapeBrush(EShape::Control, false, FLinearColor::White);
	FlashBrush.ImageSize *= 1.3f;
	Row.Flash = MakeImage(WidgetTree, FlashBrush);
	Row.Flash->SetColorAndOpacity(FLinearColor::Transparent);
	FillOverlaySlot(Box->AddChildToOverlay(Row.Flash));

	// A word on the top-right corner while the row is being moved or is the swap target.
	Row.ChipText = Label(WidgetTree, TEXT(""), 7, Color::AccentDark(), 150);
	Row.ChipText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	Row.Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Row.Chip->SetPadding(FMargin(7.f, 1.f));
	Row.Chip->SetContent(Row.ChipText);
	Row.Chip->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* ChipSlot = Box->AddChildToOverlay(Row.Chip);
	ChipSlot->SetHorizontalAlignment(HAlign_Right);
	ChipSlot->SetVerticalAlignment(VAlign_Top);
	ChipSlot->SetPadding(FMargin(0.f, 4.f, 10.f, 0.f));

	Row.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	// A gun's row is pressed through the screen (a click or a drag) and sounds its action; an empty slot's row is a plain
	// button whose own click would sound twice with it.
	Row.Button->bPlaysSounds = bHasGun;
	Row.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, Height), Action, Index);
	Row.Button->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandleRowClicked);
	Row.Button->OnButtonHovered.BindUObject(this, &ULoadoutWidget::HandleRowHovered);
	return Row;
}

void ULoadoutWidget::RebuildSlots()
{
	SlotList->ClearChildren();
	SlotRows.Reset();
	for (int32 SlotIndex = 0; SlotIndex < NumSlots(); ++SlotIndex)
	{
		const FRow Row = MakeSlotRow(SlotIndex);
		SlotList->AddChildToVerticalBox(Row.Button)->SetPadding(FMargin(0.f, SlotIndex > 0 ? LoadoutLayout::RowGap : 0.f, 0.f, 0.f));
		SlotRows.Add(Row);
	}
}

ULoadoutWidget::FRow ULoadoutWidget::MakeSlotRow(int32 SlotIndex)
{
	const FWeaponInstanceData* Item = SlotItem(SlotIndex);
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// The slot's number, as the number keys take it in hand (lit while it's in hand: Restyle).
	UBorder* Badge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Badge->SetHorizontalAlignment(HAlign_Center);
	Badge->SetVerticalAlignment(VAlign_Center);
	UTextBlock* BadgeText = MakeText(WidgetTree, FString::FromInt(SlotIndex + 1), 12, Color::Title());
	BadgeText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	Badge->SetContent(BadgeText);
	UHorizontalBoxSlot* BadgeSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Badge, 26.f, 26.f));
	BadgeSlot->SetVerticalAlignment(VAlign_Center);
	BadgeSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));

	UWidget* NewMark = nullptr;
	if (Item)
	{
		const FLinearColor Rarity = RarityColor(Item);
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeGunPicture(WidgetTree, *Item, FVector2D(SlotIconWidth, SlotIconHeight)),
			SlotIconWidth, SlotIconHeight))->SetVerticalAlignment(VAlign_Center);

		// The damage column is as wide as its number; the name gets the rest (less the NEW tag's room while it shows) and
		// is set smaller to fit it, then ends in "..." (a Named gun's nickname makes the longest names).
		const FString Damage = LooterWeaponText::DamageString(Item->Stats);
		const float NumberWidth = NumberColumnWidth(Damage, SlotNumberSize, SlotNumberMinWidth);
		const float NameWidth = LoadoutLayout::ListWidth - RowPaddingLeft - RowPaddingRight - SlotBadgeRoom - SlotIconWidth - SlotWordsPadding
			- NumberWidth - (IsNew(LoadoutRules::GunIdentity(*Item)) ? NewMarkRoom : 0.f);

		UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		NewMark = MakeNewMark(WidgetTree);
		Words->AddChildToVerticalBox(NameWithMark(WidgetTree, MakeGunNameLine(WidgetTree, *Item, SlotNameSize, Rarity, 50, NameWidth, SlotNameMinSize),
			NewMark));
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, FString::Printf(TEXT("%s · Lv %d"), *KindLabel(*Item), Item->Level), 8,
			Color::TextDim(), 140))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
		UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(Words);
		WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		WordsSlot->SetVerticalAlignment(VAlign_Center);
		WordsSlot->SetPadding(FMargin(14.f, 0.f, 10.f, 0.f));

		// The one number a slot is judged by at a glance.
		UHorizontalBoxSlot* NumberSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree,
			KeyNumber(WidgetTree, Damage, SlotNumberSize, TEXT("Dmg")), NumberWidth, 0.f));
		NumberSlot->SetVerticalAlignment(VAlign_Center);
	}
	else
	{
		const bool bBackpackEmpty = !Manager.IsValid() || Manager->GetBackpack().IsEmpty();
		UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Words->AddChildToVerticalBox(Label(WidgetTree, TEXT("Empty slot"), 11, Color::TextDim(), 200));
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, bBackpackEmpty ? TEXT("The next gun you pick up goes here") : TEXT("E: choose a gun from the backpack"),
			8, Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.7f), 120))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
		UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(Words);
		WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		WordsSlot->SetVerticalAlignment(VAlign_Center);
	}

	FRow Row = WrapRow(Line, RarityColor(Item), Item != nullptr, LoadoutLayout::SlotRowHeight, ActionSlot, SlotIndex);
	Row.Badge = Badge;
	Row.BadgeText = BadgeText;
	Row.NewMark = NewMark;
	Row.Identity = Item ? LoadoutRules::GunIdentity(*Item) : 0;
	return Row;
}

void ULoadoutWidget::RebuildList()
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	if (!Inventory || !ListBox)
	{
		return;
	}
	ListBox->ClearChildren();
	ListRows.Reset();
	NumSameKind = LoadoutRules::SortBackpack(Inventory->GetBackpack(), SlotItem(TargetSlot), Sort, ListOrder);

	for (int32 Row = 0; Row < ListOrder.Num(); ++Row)
	{
		// Sorted best for the slot, the other kinds follow the target's kind under a quiet divider.
		if (Row == NumSameKind && NumSameKind > 0)
		{
			UHorizontalBox* Divider = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Divider->AddChildToHorizontalBox(Label(WidgetTree, TEXT("Other kinds"), 7, Color::TextDim(), 260))->SetVerticalAlignment(VAlign_Center);
			UHorizontalBoxSlot* RuleSlot = Divider->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::RowLine())), 0.f, 1.f));
			RuleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			RuleSlot->SetVerticalAlignment(VAlign_Center);
			RuleSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
			if (UScrollBoxSlot* DividerSlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Divider)))
			{
				DividerSlot->SetPadding(FMargin(0.f, 10.f, 8.f, 4.f));
			}
		}
		const FRow Card = MakeBackpackRow(Row);
		if (UScrollBoxSlot* CardSlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Card.Button)))
		{
			CardSlot->SetPadding(FMargin(0.f, Row > 0 ? LoadoutLayout::RowGap : 0.f, 8.f, 0.f));
		}
		ListRows.Add(Card);
	}
	if (ListOrder.IsEmpty())
	{
		UTextBlock* Empty = Label(WidgetTree, TEXT("Empty. Guns you pick up with your slots full land here."), 8,
			Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.7f), 120);
		Empty->SetAutoWrapText(true);
		if (UScrollBoxSlot* EmptySlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Empty)))
		{
			EmptySlot->SetPadding(FMargin(4.f, 6.f, 8.f, 0.f));
		}
	}
}

ULoadoutWidget::FRow ULoadoutWidget::MakeBackpackRow(int32 Row)
{
	const FWeaponInstanceData* Item = ListItem(Row);
	check(Item);
	const FLinearColor Rarity = RarityColor(Item);
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeGunPicture(WidgetTree, *Item, FVector2D(PackIconWidth, PackIconHeight)),
		PackIconWidth, PackIconHeight))->SetVerticalAlignment(VAlign_Center);

	// As on a slot's row: the damage as wide as its number, the name fitted to the rest.
	const FString Damage = LooterWeaponText::DamageString(Item->Stats);
	const float NumberWidth = NumberColumnWidth(Damage, PackNumberSize, PackNumberMinWidth);
	const float NameWidth = LoadoutLayout::ListWidth - ListRightGap - RowPaddingLeft - RowPaddingRight - PackIconWidth - PackWordsPadding
		- NumberWidth - PackMarkRoom - (IsNew(LoadoutRules::GunIdentity(*Item)) ? NewMarkRoom : 0.f);

	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UWidget* NewMark = MakeNewMark(WidgetTree);
	Words->AddChildToVerticalBox(NameWithMark(WidgetTree, MakeGunNameLine(WidgetTree, *Item, PackNameSize, Rarity, 50, NameWidth, PackNameMinSize),
		NewMark));
	Words->AddChildToVerticalBox(FittedLabel(WidgetTree, FString::Printf(TEXT("%s · Lv %d"), *KindLabel(*Item), Item->Level), 7,
		Color::TextDim(), 140))->SetPadding(FMargin(0.f, 1.f, 0.f, 0.f));
	UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(Words);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(12.f, 0.f, 8.f, 0.f));

	UHorizontalBoxSlot* NumberSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree,
		KeyNumber(WidgetTree, Damage, PackNumberSize, nullptr), NumberWidth, 0.f));
	NumberSlot->SetVerticalAlignment(VAlign_Center);

	// Against the gun it would replace (the same kind only): green up, red down, or level.
	UOverlay* Mark = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	const LoadoutRules::EVerdict Verdict = LoadoutRules::Compare(*Item, SlotItem(TargetSlot));
	if (Verdict == LoadoutRules::EVerdict::Upgrade || Verdict == LoadoutRules::EVerdict::Weaker)
	{
		const bool bUp = Verdict == LoadoutRules::EVerdict::Upgrade;
		UOverlaySlot* ArrowSlot = Mark->AddChildToOverlay(MakeImage(WidgetTree, IconBrush(bUp ? TEXT("ArrowUp") : TEXT("ArrowDown"), ArrowIcon(bUp), 4.f,
			FVector2D(12.f, 10.f), bUp ? Color::Better() : Color::Worse())));
		ArrowSlot->SetHorizontalAlignment(HAlign_Center);
		ArrowSlot->SetVerticalAlignment(VAlign_Center);
	}
	else if (Verdict == LoadoutRules::EVerdict::Similar)
	{
		UOverlaySlot* LevelSlot = Mark->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::TextDim())), 10.f, 2.f));
		LevelSlot->SetHorizontalAlignment(HAlign_Center);
		LevelSlot->SetVerticalAlignment(VAlign_Center);
	}
	UHorizontalBoxSlot* MarkSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, 18.f, 18.f));
	MarkSlot->SetVerticalAlignment(VAlign_Center);
	MarkSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

	FRow Card = WrapRow(Line, Rarity, true, LoadoutLayout::BackpackRowHeight, ActionBackpack, Row);
	Card.NewMark = NewMark;
	Card.Identity = LoadoutRules::GunIdentity(*Item);
	return Card;
}

// ---------------------------------------------------------------------------
// How the rows look as the cursor moves
// ---------------------------------------------------------------------------

void ULoadoutWidget::Restyle()
{
	const int32 ActiveSlot = GetActiveSlot();
	const bool bBrowsingBackpack = Zone == EZone::Backpack && !PickedSlot.IsSet() && !bItemDrag;
	auto SetChip = [](const FRow& Row, const TCHAR* Word, const FLinearColor& Back, const FLinearColor& Text)
	{
		if (!Word)
		{
			Row.Chip->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}
		Row.Chip->SetBrush(RectBrush(Back));
		Row.ChipText->SetText(FText::FromString(FString(Word).ToUpper()));
		Row.ChipText->SetColorAndOpacity(FSlateColor(Text));
		Row.Chip->SetVisibility(ESlateVisibility::HitTestInvisible);
	};

	for (int32 SlotIndex = 0; SlotIndex < SlotRows.Num(); ++SlotIndex)
	{
		const FRow& Row = SlotRows[SlotIndex];
		const bool bItem = SlotItem(SlotIndex) != nullptr;
		const bool bInHand = bItem && SlotIndex == ActiveSlot;
		const bool bCursor = Zone == EZone::Slots && CursorIndex == SlotIndex && !bItemDrag;
		const bool bPicked = PickedSlot.IsSet() && PickedSlot.GetValue() == SlotIndex;
		const bool bTarget = bBrowsingBackpack && SlotIndex == TargetSlot;

		FLinearColor Fill = bItem ? Colors::CardFill() : Colors::EmptyFill();
		FLinearColor Line = bItem ? Colors::CardLine() : Colors::CardLine() * FLinearColor(1.f, 1.f, 1.f, 0.5f);
		// While a slot is picked up every other slot is somewhere it can go; while browsing the backpack, the target is lit.
		if ((PickedSlot.IsSet() && !bPicked) || bTarget)
		{
			Line = Color::TileLine();
		}
		if (bCursor)
		{
			Fill = Color::Tile();
			Line = Color::Accent();
		}
		if (bPicked)
		{
			Fill = Colors::PickedFill();
			Line = Color::Accent();
		}
		// Dragging: the gun's own row stays marked, and the one it would land on lights up.
		if (bItemDrag)
		{
			if (PressZone == EZone::Slots && PressIndex == SlotIndex)
			{
				Fill = Colors::PickedFill();
			}
			if (DropTarget.Kind == EDropKind::Slot && DropTarget.Index == SlotIndex && !DropActionText(DropTarget).IsEmpty())
			{
				Fill = Colors::DropFill();
				Line = Color::Accent();
			}
		}
		Row.Fill->SetColorAndOpacity(Fill);
		Row.Line->SetColorAndOpacity(Line);

		// The number badge is lit orange while its gun is in hand, as the HUD lights the slot in hand.
		Row.Badge->SetBrush(bInHand ? RectBrush(Color::Accent()) : RectBrush(Color::Plate(), Color::Hairline() * FLinearColor(1.f, 1.f, 1.f, 0.5f), 1.f));
		Row.BadgeText->SetColorAndOpacity(FSlateColor(bInHand ? Color::AccentDark() : (bItem ? Color::Title() : Color::TextDim())));

		if (bPicked)
		{
			SetChip(Row, TEXT("Moving"), Color::Accent(), Color::AccentDark());
		}
		else if (bTarget)
		{
			SetChip(Row, TEXT("Swap target"), Color::Hairline(), Color::Track());
		}
		else
		{
			SetChip(Row, nullptr, FLinearColor::Transparent, FLinearColor::Transparent);
		}
		if (Row.NewMark)
		{
			Row.NewMark->SetVisibility(IsNew(Row.Identity) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	// Stored anywhere on the backpack, a dragged gun swaps with the row it's over.
	const bool bDropValid = bItemDrag && !DropActionText(DropTarget).IsEmpty();
	for (int32 Row = 0; Row < ListRows.Num(); ++Row)
	{
		const FRow& Card = ListRows[Row];
		const bool bCursor = Zone == EZone::Backpack && CursorIndex == Row && !bItemDrag;
		const bool bSource = bItemDrag && PressZone == EZone::Backpack && ListOrder.IsValidIndex(Row) && ListOrder[Row] == PressIndex;
		FLinearColor Fill = bCursor ? Color::Tile() : Colors::CardFill();
		FLinearColor Line = bCursor ? Color::Accent() : Colors::CardLine();
		if (bSource)
		{
			Fill = Colors::PickedFill();
		}
		if (bDropValid && DropTarget.Kind == EDropKind::BackpackRow && DropTarget.Index == Row)
		{
			Fill = Colors::DropFill();
			Line = Color::Accent();
		}
		Card.Fill->SetColorAndOpacity(Fill);
		Card.Line->SetColorAndOpacity(Line);
		if (Card.NewMark)
		{
			Card.NewMark->SetVisibility(IsNew(Card.Identity) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

