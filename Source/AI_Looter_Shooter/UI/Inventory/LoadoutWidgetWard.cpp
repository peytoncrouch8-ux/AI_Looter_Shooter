// ULoadoutWidget: the wards (SHIELD, 2026-10-22; in the backpack since 2026-10-31, the user: "Wards should go to the
// backpack side of the loadout tab ... guns and wards take up the same backpack space"). The ward worn has the WARD row on
// the left under the slots, in a slot row's look; the spares are rows of the backpack list on the right, in a backpack gun
// row's look (icon, name in the rarity's colour, kind line, the capacity big with WARD under it, an arrow against the ward
// worn). What the keys do on them (wear, stow, drop) is LoadoutWidgetWardKeys.cpp; their card and the showcase's icon
// LoadoutWidgetWardCard.cpp.

#include "UI/Inventory/LoadoutWidget.h"
#include "Audio/LooterSound.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
#include "UI/Inventory/LoadoutBackpackRules.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Templates/UnrealTemplate.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	/** The ward box's icon in the showcase. */
	constexpr float ShowcaseIconSize = 240.f;

	// The rows' room, as LoadoutWidgetRows.cpp lays out a gun's, so a ward's row lines up with a gun's.
	constexpr float RowPaddingLeft = 18.f;
	constexpr float RowPaddingRight = 12.f;
	constexpr float ListRightGap = 16.f;
	/** The WARD row, as a slot's: the badge's room (left empty: a ward has no number key), the icon box, the words' padding. */
	constexpr float SlotBadgeRoom = 24.f + 10.f;
	constexpr float SlotIconWidth = 104.f;
	constexpr float SlotIconHeight = 34.f;
	constexpr float SlotWordsPadding = 14.f + 10.f;
	constexpr int32 SlotNameSize = 12;
	constexpr int32 SlotNumberSize = 17;
	constexpr float SlotNumberMinWidth = 64.f;
	/** A backpack ward's row, as a backpack gun's. */
	constexpr float PackIconWidth = 78.f;
	constexpr float PackIconHeight = 26.f;
	constexpr float PackWordsPadding = 12.f + 8.f;
	constexpr float PackMarkRoom = 18.f + 8.f;
	constexpr int32 PackNameSize = 10;
	constexpr int32 PackNumberSize = 13;
	constexpr float PackNumberMinWidth = 58.f;
	constexpr float NewMarkRoom = 40.f;

	FName WardIconName(const FShieldInstanceData* Ward)
	{
		return Ward && Ward->Definition ? Ward->Definition->IconName : FName(TEXT("WardBox"));
	}

	/** The ward box's Inked icon, as large as fits the box's height, centred in a box of a gun picture's size. */
	UWidget* WardPicture(UWidgetTree* Tree, const FShieldInstanceData* Ward, const FVector2D& Box, const FLinearColor& Tint = FLinearColor::White)
	{
		UOverlay* Picture = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		const FName Name = WardIconName(Ward);
		if (const FInkedIcon* Inked = FindInkedIcon(Name))
		{
			const float Side = FMath::Min(Box.X, Box.Y);
			UOverlaySlot* ImageSlot = Picture->AddChildToOverlay(MakeImage(Tree, InkedIconBrush(Name, *Inked, FVector2D(Side, Side), Tint)));
			ImageSlot->SetHorizontalAlignment(HAlign_Center);
			ImageSlot->SetVerticalAlignment(VAlign_Center);
		}
		return MakeSized(Tree, Picture, Box.X, Box.Y);
	}

	/** The number column: as wide as its number is drawn, never narrower than MinWidth (LoadoutWidgetRows.cpp's rule). */
	float NumberColumnWidth(const FString& Number, int32 Size, float MinWidth)
	{
		return FMath::Max(MinWidth, FMath::CeilToFloat(MeasureText(Number, LooterUI::Font(Size)) + 4.f));
	}

	/** A number over a small word, right-aligned: "65 / WARD", as a gun's "34 / DMG". */
	UWidget* KeyNumber(UWidgetTree* Tree, const FString& Number, int32 Size, const TCHAR* Word)
	{
		UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UTextBlock* Value = MakeText(Tree, Number, Size, Color::Text());
		Value->SetJustification(ETextJustify::Right);
		Box->AddChildToVerticalBox(Value)->SetHorizontalAlignment(HAlign_Right);
		UTextBlock* Under = Label(Tree, Word, 7, Color::TextDim(), 200);
		Under->SetJustification(ETextJustify::Right);
		Box->AddChildToVerticalBox(Under)->SetHorizontalAlignment(HAlign_Right);
		return Box;
	}

	/** NEW: the gun rows' cyan tag, on a ward found since the screen was last looked through. */
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

	/** "Ward Box · Lv 3 · Barbwire": the kind line, and the trait's name when it has one. */
	FString WardKindLine(const FShieldInstanceData& Ward)
	{
		const FString Trait = ShieldText::TraitName(Ward.Trait);
		return Trait.IsEmpty() ? ShieldText::KindLine(Ward) : FString::Printf(TEXT("%s · %s"), *ShieldText::KindLine(Ward), *Trait);
	}

	/** The name, fitted to its room (smaller, then "..."), in the rarity's colour. */
	UTextBlock* WardName(UWidgetTree* Tree, const FShieldInstanceData& Ward, int32 Size, float Width)
	{
		UTextBlock* Name = Label(Tree, ShieldText::Name(Ward), Size, ShieldText::Color(Ward), 50);
		FitTextToWidth(Name, Width, FMath::Max(Size - 3, 8));
		return Name;
	}
}

// ---------------------------------------------------------------------------
// What the rows hold
// ---------------------------------------------------------------------------

UPlayerShieldComponent* ULoadoutWidget::GetShield() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory ? UPlayerShieldComponent::Find(Inventory->GetOwner()) : nullptr;
}

const FShieldInstanceData* ULoadoutWidget::WornWard() const
{
	const UPlayerShieldComponent* Shield = GetShield();
	return Shield && Shield->HasWard() ? &Shield->GetWard() : nullptr;
}

const FShieldInstanceData* ULoadoutWidget::ListWard(int32 Row, int32* OutSpare) const
{
	const UPlayerShieldComponent* Shield = GetShield();
	const int32 Spare = Shield && ListOrder.IsValidIndex(Row) ? LoadoutBackpack::WardOf(ListOrder[Row]) : INDEX_NONE;
	if (OutSpare)
	{
		*OutSpare = Spare;
	}
	return Spare != INDEX_NONE && Shield->GetSpares().IsValidIndex(Spare) ? &Shield->GetSpares()[Spare] : nullptr;
}

const FShieldInstanceData* ULoadoutWidget::CursorWard() const
{
	if (Zone == EZone::Wards)
	{
		return WornWard();
	}
	return Zone == EZone::Backpack ? ListWard(CursorIndex) : nullptr;
}

bool ULoadoutWidget::IsOnWard() const
{
	return Zone == EZone::Wards || (Zone == EZone::Backpack && ListWard(CursorIndex) != nullptr);
}

// ---------------------------------------------------------------------------
// The rows
// ---------------------------------------------------------------------------

void ULoadoutWidget::BuildWardSection(UVerticalBox* Column)
{
	if (!Column)
	{
		return;
	}
	// The header as the others are (WARD and its count), then its row, under the slots (the gaps RowCenterY counts on).
	UTextBlock* Count = nullptr;
	WardHeader = MakeSectionHeader(TEXT("Ward"), Count);
	WardCount = Count;
	WardList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(WardHeader)->SetPadding(FMargin(0.f, LoadoutLayout::SectionGap, 0.f, 0.f));
	Column->AddChildToVerticalBox(WardList)->SetPadding(FMargin(0.f, LoadoutLayout::HeaderGap, 0.f, 0.f));

	// The showcase's stand-in for a ward: its icon, big, where the gun turns.
	if (Page && !WardShowcase)
	{
		WardShowcase = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		WardShowcase->SetVisibility(ESlateVisibility::Hidden);
		UCanvasPanelSlot* ShowSlot = Page->AddChildToCanvas(WardShowcase);
		ShowSlot->SetSize(FVector2D(ShowcaseIconSize, ShowcaseIconSize));
		ShowSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ShowSlot->SetPosition(LoadoutLayout::ShowcaseTopLeft + LoadoutLayout::ShowcaseSize * 0.5f);
	}
}

void ULoadoutWidget::RebuildWards()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (Shield != BoundShield.Get())
	{
		// The ward's changes refresh the screen as the guns' do.
		if (UPlayerShieldComponent* Old = BoundShield.Get())
		{
			Old->OnWardsChanged.RemoveDynamic(this, &ULoadoutWidget::Refresh);
		}
		BoundShield = Shield;
		if (Shield)
		{
			Shield->OnWardsChanged.AddUniqueDynamic(this, &ULoadoutWidget::Refresh);
		}
	}
	if (!WardList)
	{
		return;
	}
	WardList->ClearChildren();
	WardRows.Reset();
	// No shield on the holder (a stand-in without health): no section at all, so the list walks as it did.
	const ESlateVisibility Section = Shield ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	WardList->SetVisibility(Section);
	if (WardHeader)
	{
		WardHeader->SetVisibility(Section);
	}
	if (!Shield)
	{
		if (Zone == EZone::Wards)
		{
			Zone = EZone::Slots;
			CursorIndex = TargetSlot;
		}
		return;
	}
	const FRow Made = MakeWornWardRow();
	WardList->AddChildToVerticalBox(Made.Button);
	WardRows.Add(Made);
	if (WardCount)
	{
		// As EQUIPPED counts its slots: one place, worn or not.
		WardCount->SetText(FText::FromString(Shield->HasWard() ? TEXT("1 / 1") : TEXT("0 / 1")));
	}
}

ULoadoutWidget::FRow ULoadoutWidget::MakeWornWardRow()
{
	const FShieldInstanceData* Ward = WornWard();
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// The slot rows' badge room, left empty (a ward has no number key), so the icons line up down the column.
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()), SlotBadgeRoom, 24.f))
		->SetVerticalAlignment(VAlign_Center);

	UWidget* NewMark = nullptr;
	if (Ward)
	{
		Line->AddChildToHorizontalBox(WardPicture(WidgetTree, Ward, FVector2D(SlotIconWidth, SlotIconHeight)))->SetVerticalAlignment(VAlign_Center);
		const FString Capacity = ShieldText::CapacityString(Ward->Stats);
		const float NumberWidth = NumberColumnWidth(Capacity, SlotNumberSize, SlotNumberMinWidth);
		const float NameWidth = LoadoutLayout::ListWidth - RowPaddingLeft - RowPaddingRight - SlotBadgeRoom - SlotIconWidth - SlotWordsPadding
			- NumberWidth - (IsNew(ShieldRules::Identity(*Ward)) ? NewMarkRoom : 0.f);

		UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		NewMark = MakeNewMark(WidgetTree);
		Words->AddChildToVerticalBox(NameWithMark(WidgetTree, WardName(WidgetTree, *Ward, SlotNameSize, NameWidth), NewMark));
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, WardKindLine(*Ward) + TEXT(" · Worn"), 8, Color::TextDim(), 140))
			->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
		UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(Words);
		WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		WordsSlot->SetVerticalAlignment(VAlign_Center);
		WordsSlot->SetPadding(FMargin(14.f, 0.f, 10.f, 0.f));

		// The one number a ward is judged by at a glance, as a slot's damage: what it holds.
		UHorizontalBoxSlot* NumberSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, KeyNumber(WidgetTree, Capacity, SlotNumberSize, TEXT("Ward")),
			NumberWidth, 0.f));
		NumberSlot->SetVerticalAlignment(VAlign_Center);
	}
	else
	{
		// (Read from the spares: the list is rebuilt after this row.)
		const UPlayerShieldComponent* Shield = GetShield();
		const bool bSpares = Shield && !Shield->GetSpares().IsEmpty();
		UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Words->AddChildToVerticalBox(Label(WidgetTree, TEXT("No ward worn"), 11, Color::TextDim(), 200));
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, bSpares ? TEXT("E: choose a ward from the backpack")
			: TEXT("Wards drop like guns: hold Interact to wear one"), 8, Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.7f), 120))
			->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
		UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(Words);
		WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		WordsSlot->SetVerticalAlignment(VAlign_Center);
	}

	FRow Made = WrapRow(Line, Ward ? ShieldText::Color(*Ward) : Color::TextDim(), Ward != nullptr, LoadoutLayout::WardRowHeight, ActionWard, 0);
	// Its clicks are the screen's actions (RunAction), sounded by them (a ward isn't dragged).
	Made.Button->bPlaysSounds = false;
	Made.NewMark = NewMark;
	Made.Identity = Ward ? ShieldRules::Identity(*Ward) : 0;
	return Made;
}

ULoadoutWidget::FRow ULoadoutWidget::MakeBackpackWardRow(int32 Row)
{
	const FShieldInstanceData* Ward = ListWard(Row);
	check(Ward);
	const FLinearColor Rarity = ShieldText::Color(*Ward);
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Line->AddChildToHorizontalBox(WardPicture(WidgetTree, Ward, FVector2D(PackIconWidth, PackIconHeight)))->SetVerticalAlignment(VAlign_Center);

	// As a backpack gun's row: the number as wide as it's drawn, the name fitted to the rest (less the NEW tag's room).
	const FString Capacity = ShieldText::CapacityString(Ward->Stats);
	const float NumberWidth = NumberColumnWidth(Capacity, PackNumberSize, PackNumberMinWidth);
	const float NameWidth = LoadoutLayout::BackpackWidth - ListRightGap - RowPaddingLeft - RowPaddingRight - PackIconWidth - PackWordsPadding
		- NumberWidth - PackMarkRoom - (IsNew(ShieldRules::Identity(*Ward)) ? NewMarkRoom : 0.f);
	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UWidget* NewMark = MakeNewMark(WidgetTree);
	Words->AddChildToVerticalBox(NameWithMark(WidgetTree, WardName(WidgetTree, *Ward, PackNameSize, NameWidth), NewMark));
	Words->AddChildToVerticalBox(FittedLabel(WidgetTree, WardKindLine(*Ward), 7, Color::TextDim(), 140))->SetPadding(FMargin(0.f, 1.f, 0.f, 0.f));
	UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(Words);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(12.f, 0.f, 8.f, 0.f));

	// The capacity with WARD under it: the word tells a ward's row from a gun's at a glance.
	UHorizontalBoxSlot* NumberSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, KeyNumber(WidgetTree, Capacity, PackNumberSize, TEXT("Ward")),
		NumberWidth, 0.f));
	NumberSlot->SetVerticalAlignment(VAlign_Center);

	// Against the ward worn (ShieldRules::Compare): green up, red down, a dash when about the same; none worn, up.
	UOverlay* Mark = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	const int32 Verdict = ShieldRules::Compare(*Ward, WornWard());
	if (Verdict != 0)
	{
		UOverlaySlot* ArrowSlot = Mark->AddChildToOverlay(MakeImage(WidgetTree, IconBrush(Verdict > 0 ? TEXT("ArrowUp") : TEXT("ArrowDown"),
			ArrowIcon(Verdict > 0), 4.f, FVector2D(12.f, 10.f), Verdict > 0 ? Color::Better() : Color::Worse())));
		ArrowSlot->SetHorizontalAlignment(HAlign_Center);
		ArrowSlot->SetVerticalAlignment(VAlign_Center);
	}
	else
	{
		UOverlaySlot* LevelSlot = Mark->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::TextDim())), 10.f, 2.f));
		LevelSlot->SetHorizontalAlignment(HAlign_Center);
		LevelSlot->SetVerticalAlignment(VAlign_Center);
	}
	UHorizontalBoxSlot* MarkSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, 18.f, 18.f));
	MarkSlot->SetVerticalAlignment(VAlign_Center);
	MarkSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

	FRow Made = WrapRow(Line, Rarity, true, LoadoutLayout::BackpackRowHeight, ActionBackpack, Row);
	// The row's click is the screen's (RunAction: wear it), sounded by the action; it isn't dragged.
	Made.Button->bPlaysSounds = false;
	Made.NewMark = NewMark;
	Made.Identity = ShieldRules::Identity(*Ward);
	return Made;
}

void ULoadoutWidget::RestyleWards()
{
	for (int32 Row = 0; Row < WardRows.Num(); ++Row)
	{
		const FRow& Made = WardRows[Row];
		const bool bItem = WornWard() != nullptr;
		const bool bCursor = Zone == EZone::Wards && CursorIndex == Row && !bItemDrag;
		ApplyRowState(Made, bCursor ? ELooterRowState::Lit : (bItem ? ELooterRowState::Normal : ELooterRowState::Empty));
		// The ward worn wears a WORN word on its corner, as a slot shows its gun in hand.
		if (bItem)
		{
			Made.Chip->SetBrush(RectBrush(Color::Hairline()));
			Made.ChipText->SetText(FText::FromString(TEXT("WORN")));
			Made.ChipText->SetColorAndOpacity(FSlateColor(Color::Track()));
			Made.Chip->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Made.Chip->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Made.NewMark)
		{
			Made.NewMark->SetVisibility(IsNew(Made.Identity) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}
