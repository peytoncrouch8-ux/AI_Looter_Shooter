// ULoadoutWidget: the rows (an equip slot's on the left, a backpack gun's or a free place's on the right) in the "Wide
// Sweep" row look (ULooterCardBackdrop, Row), and how each looks as the cursor moves. The ward rows are
// LoadoutWidgetWard.cpp; the counts, the ammo line and the prompt bar LoadoutWidgetPrompts.cpp.

#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Cards/LooterCardBackdrop.h"
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
	// The room a gun's name has on its row, worked out from the row's parts so a long name is fitted before it's drawn
	// (LoadoutParts::FitTextToWidth) and never runs over the damage beside it.
	/** A row's content padding: the left keeps clear of the row's tick, then the right. */
	constexpr float RowPaddingLeft = 18.f;
	constexpr float RowPaddingRight = 12.f;
	/** A backpack row also gives up its scroll box slot's right padding and the scroll bar. */
	constexpr float ListRightGap = 16.f;
	/** Slot rows: the number badge and its gap, the icon, the words' padding either side, the damage's size. */
	constexpr float SlotBadgeRoom = 24.f + 10.f;
	constexpr float SlotIconWidth = 104.f;
	constexpr float SlotIconHeight = 34.f;
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
	/** The JUNK tag (a word longer than NEW, with its outline) and its gap, left free beside a gun marked junk. */
	constexpr float JunkMarkRoom = 48.f;
	/** A row being dragged stays in its place, faded. */
	constexpr float DragSourceOpacity = 0.5f;

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

	/**
	 * JUNK: a small outlined tag, in the dim text colour, on a backpack gun marked junk (J, or the vendor's right-click). Quieter
	 * than NEW's filled cyan: the gun is waiting to be sold, not to be looked at. Built only for a gun marked (the list is
	 * rebuilt when the mark changes).
	 */
	UWidget* MakeJunkMark(UWidgetTree* Tree)
	{
		UTextBlock* Word = Label(Tree, TEXT("Junk"), 7, Color::TextDim(), 150);
		Word->SetShadowColorAndOpacity(FLinearColor::Transparent);
		UBorder* Tag = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Tag->SetBrush(RectBrush(Color::Plate(), Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.6f), 1.f));
		Tag->SetPadding(FMargin(5.f, 0.f));
		Tag->SetVerticalAlignment(VAlign_Center);
		Tag->SetContent(Word);
		Tag->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Tag;
	}

	/** A name and the tags after it (JUNK when it has one, then NEW), the name taking the room there is. */
	UWidget* NameWithMark(UWidgetTree* Tree, UWidget* Name, UWidget* Mark, UWidget* JunkMark = nullptr)
	{
		UHorizontalBox* Line = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(Name);
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		if (JunkMark)
		{
			UHorizontalBoxSlot* JunkSlot = Line->AddChildToHorizontalBox(MakeSized(Tree, JunkMark, 0.f, 13.f));
			JunkSlot->SetVerticalAlignment(VAlign_Center);
			JunkSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		}
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

ULoadoutWidget::FRow ULoadoutWidget::WrapRow(UWidget* Content, const FLinearColor& Rarity, bool bHasItem, float Height, FName Action, int32 Index)
{
	FRow Row;
	Row.Rarity = Rarity;
	Row.bFilled = bHasItem;
	ULooterCardBackdrop* Fill = nullptr;
	ULooterCardBackdrop* Lines = nullptr;
	UOverlay* Box = ULooterCardBackdrop::MakeLayered(WidgetTree, ELooterBackdropShape::Row, Content,
		FMargin(RowPaddingLeft, 6.f, RowPaddingRight, 6.f), Fill, Lines);
	Row.Fill = Fill;
	Row.Lines = Lines;
	// The tick on its left wears the item's rarity, so the list reads by colour at a glance.
	if (bHasItem)
	{
		Fill->SetRowTick(Rarity);
		Lines->SetRowTick(Rarity);
	}

	// The flash as a gun lands here: the lit wash in the row's own shape, over the glass, faded out by TickMotion. Not marked
	// as a background: MarkBackground would take over its opacity, which is the fade, and it's gone in half a second.
	Row.Flash = WidgetTree->ConstructWidget<ULooterCardBackdrop>(ULooterCardBackdrop::StaticClass());
	Row.Flash->SetShape(ELooterBackdropShape::Row);
	Row.Flash->SetPart(ELooterBackdropPart::Fill);
	Row.Flash->SetRowState(ELooterRowState::Lit);
	Row.Flash->SetRenderOpacity(0.f);
	Box->InsertChildAt(1, Row.Flash);
	if (UOverlaySlot* FlashSlot = Cast<UOverlaySlot>(Row.Flash->Slot))
	{
		FillOverlaySlot(FlashSlot);
	}

	// A word on the top-right corner while the row is being moved, is the swap target, or is the ward worn.
	Row.ChipText = Label(WidgetTree, TEXT(""), 7, Color::AccentDark(), 150);
	Row.ChipText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	Row.Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Row.Chip->SetPadding(FMargin(7.f, 1.f));
	Row.Chip->SetContent(Row.ChipText);
	Row.Chip->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* ChipSlot = Box->AddChildToOverlay(Row.Chip);
	ChipSlot->SetHorizontalAlignment(HAlign_Right);
	ChipSlot->SetVerticalAlignment(VAlign_Top);
	// Left of the row's number column (damage, ward strength), so the chip never sits on the number.
	ChipSlot->SetPadding(FMargin(0.f, 4.f, LooterBackdrop::RowCut + 84.f, 0.f));

	Row.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	// A gun's row is pressed through the screen (a click or a drag) and sounds its action; an empty slot's row is a plain
	// button whose own click would sound twice with it.
	Row.Button->bPlaysSounds = bHasItem;
	Row.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, Height), Action, Index);
	Row.Button->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandleRowClicked);
	Row.Button->OnButtonHovered.BindUObject(this, &ULoadoutWidget::HandleRowHovered);
	return Row;
}

void ULoadoutWidget::ApplyRowState(const FRow& Row, ELooterRowState State) const
{
	Row.Fill->SetRowState(State);
	Row.Lines->SetRowState(State);
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

	// The slot's number, as the number keys take it in hand (on an orange plate while it's in hand: Restyle).
	UBorder* Badge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Badge->SetHorizontalAlignment(HAlign_Center);
	Badge->SetVerticalAlignment(VAlign_Center);
	UTextBlock* BadgeText = MakeText(WidgetTree, FString::FromInt(SlotIndex + 1), 12, Color::Title());
	BadgeText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	Badge->SetContent(BadgeText);
	UHorizontalBoxSlot* BadgeSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Badge, 24.f, 24.f));
	BadgeSlot->SetVerticalAlignment(VAlign_Center);
	BadgeSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));

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
		const bool bInHand = SlotIndex == GetActiveSlot();
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, FString::Printf(TEXT("%s · Lv %d%s"), *KindLabel(*Item), Item->Level,
			bInHand ? TEXT(" · In hand") : TEXT("")), 8, Color::TextDim(), 140))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
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
	LoadoutRules::SortBackpack(Inventory->GetBackpack(), SlotItem(TargetSlot), Sort, ListOrder);

	// The guns in the sort's order (best for slot leads with the target's kind, whose rows carry the compare arrows), then a
	// faint EMPTY row per free place: the backpack's room reads at a glance, as it did before the 2026-10-08 redesign.
	for (int32 Row = 0; Row < ListOrder.Num(); ++Row)
	{
		const FRow Made = MakeBackpackRow(Row);
		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(ListBox->AddChild(Made.Button)))
		{
			RowSlot->SetPadding(FMargin(0.f, Row > 0 ? LoadoutLayout::RowGap : 0.f, 8.f, 0.f));
		}
		ListRows.Add(Made);
	}
	NumEmptyRows = FMath::Max(Inventory->BackpackCapacity - ListOrder.Num(), 0);
	for (int32 Free = 0; Free < NumEmptyRows; ++Free)
	{
		if (UScrollBoxSlot* FreeSlot = Cast<UScrollBoxSlot>(ListBox->AddChild(MakeEmptyBackpackRow())))
		{
			FreeSlot->SetPadding(FMargin(0.f, ListOrder.Num() + Free > 0 ? LoadoutLayout::RowGap : 0.f, 8.f, 0.f));
		}
	}
}

UWidget* ULoadoutWidget::MakeEmptyBackpackRow()
{
	// The word on the row's middle line (a filling box, its one slot centred).
	UVerticalBox* Middle = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UVerticalBoxSlot* WordSlot = Middle->AddChildToVerticalBox(Label(WidgetTree, TEXT("— Empty"), 8, Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.6f), 200));
	WordSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordSlot->SetVerticalAlignment(VAlign_Center);
	ULooterCardBackdrop* Fill = nullptr;
	ULooterCardBackdrop* Lines = nullptr;
	UOverlay* Box = ULooterCardBackdrop::MakeLayered(WidgetTree, ELooterBackdropShape::Row, Middle, FMargin(RowPaddingLeft, 0.f, RowPaddingRight, 0.f), Fill, Lines);
	Fill->SetRowState(ELooterRowState::Empty);
	Lines->SetRowState(ELooterRowState::Empty);
	// Nothing to press or hover: a gun dragged over it lands anywhere on the backpack (FindDropTarget).
	UWidget* Sized = MakeSized(WidgetTree, Box, 0.f, LoadoutLayout::BackpackRowHeight);
	Sized->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Sized;
}

ULoadoutWidget::FRow ULoadoutWidget::MakeBackpackRow(int32 Row)
{
	const FWeaponInstanceData* Item = ListItem(Row);
	check(Item);
	const FLinearColor Rarity = RarityColor(Item);
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeGunPicture(WidgetTree, *Item, FVector2D(PackIconWidth, PackIconHeight)),
		PackIconWidth, PackIconHeight))->SetVerticalAlignment(VAlign_Center);

	// As on a slot's row: the damage as wide as its number, the name fitted to the rest (less the NEW and JUNK tags' room while
	// they show: the JUNK tag is built only for a gun marked junk, so its room is known now).
	const FString Damage = LooterWeaponText::DamageString(Item->Stats);
	const float NumberWidth = NumberColumnWidth(Damage, PackNumberSize, PackNumberMinWidth);
	const float NameWidth = LoadoutLayout::BackpackWidth - ListRightGap - RowPaddingLeft - RowPaddingRight - PackIconWidth - PackWordsPadding
		- NumberWidth - PackMarkRoom - (IsNew(LoadoutRules::GunIdentity(*Item)) ? NewMarkRoom : 0.f) - (Item->bJunk ? JunkMarkRoom : 0.f);

	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UWidget* NewMark = MakeNewMark(WidgetTree);
	UWidget* JunkMark = Item->bJunk ? MakeJunkMark(WidgetTree) : nullptr;
	Words->AddChildToVerticalBox(NameWithMark(WidgetTree, MakeGunNameLine(WidgetTree, *Item, PackNameSize, Rarity, 50, NameWidth, PackNameMinSize),
		NewMark, JunkMark));
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

	FRow Made = WrapRow(Line, Rarity, true, LoadoutLayout::BackpackRowHeight, ActionBackpack, Row);
	Made.NewMark = NewMark;
	Made.JunkMark = JunkMark;
	Made.Identity = LoadoutRules::GunIdentity(*Item);
	return Made;
}

// ---------------------------------------------------------------------------
// How the rows look as the cursor moves
// ---------------------------------------------------------------------------

void ULoadoutWidget::Restyle()
{
	const int32 ActiveSlot = GetActiveSlot();
	const bool bBrowsingBackpack = Zone == EZone::Backpack && !PickedSlot.IsSet() && !bItemDrag;
	const bool bDropValid = bItemDrag && !DropActionText(DropTarget).IsEmpty();
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
		// Lit under the cursor, while picked up, and as the place a dragged gun would land.
		const bool bDropHere = bDropValid && DropTarget.Kind == EDropKind::Slot && DropTarget.Index == SlotIndex;
		ApplyRowState(Row, bCursor || bPicked || bDropHere ? ELooterRowState::Lit : (bItem ? ELooterRowState::Normal : ELooterRowState::Empty));
		// Dragging: the gun's own row stays in its place, faded.
		Row.Button->SetRenderOpacity(bItemDrag && PressZone == EZone::Slots && PressIndex == SlotIndex ? DragSourceOpacity : 1.f);

		// The number on an orange plate while its gun is in hand, as the HUD lights the slot in hand.
		Row.Badge->SetBrush(RectBrush(bInHand ? Color::Accent() : FLinearColor::Transparent));
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
	for (int32 Row = 0; Row < ListRows.Num(); ++Row)
	{
		const FRow& Made = ListRows[Row];
		const bool bCursor = Zone == EZone::Backpack && CursorIndex == Row && !bItemDrag;
		const bool bSource = bItemDrag && PressZone == EZone::Backpack && ListOrder.IsValidIndex(Row) && ListOrder[Row] == PressIndex;
		const bool bDropHere = bDropValid && DropTarget.Kind == EDropKind::BackpackRow && DropTarget.Index == Row;
		ApplyRowState(Made, bCursor || bDropHere ? ELooterRowState::Lit : ELooterRowState::Normal);
		Made.Button->SetRenderOpacity(bSource ? DragSourceOpacity : 1.f);
		if (Made.NewMark)
		{
			Made.NewMark->SetVisibility(IsNew(Made.Identity) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	RestyleWards();
}
