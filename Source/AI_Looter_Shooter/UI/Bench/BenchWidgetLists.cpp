// UBenchWidget: its three columns: the guns carried, the chosen gun's slots, and the parts box's parts for the chosen slot.

#include "UI/Bench/BenchWidget.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Bench/BenchRules.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutStage.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponPartSwap.h"
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
// BenchLayout is spelled out: the inventory's layout (LoadoutParts) has names of its own for the stand and the columns.
namespace Layout = BenchLayout;

namespace
{
	/** How many of a part's stat changes its row has room for (the stats card under the stand shows them all). */
	constexpr int32 RowChanges = 3;

	/** A list's sub-heading, between groups of rows. */
	void AddHeading(UWidgetTree* Tree, UScrollBox* List, const TCHAR* Text, bool bFirst)
	{
		if (UScrollBoxSlot* HeadingSlot = Cast<UScrollBoxSlot>(List->AddChild(Label(Tree, Text, 8, Color::TextDim(), 220))))
		{
			HeadingSlot->SetPadding(FMargin(0.f, bFirst ? 4.f : 14.f, 0.f, 0.f));
		}
	}

	FLinearColor Faded(const FLinearColor& Tint, float Alpha)
	{
		return Tint * FLinearColor(1.f, 1.f, 1.f, Alpha);
	}
}

// ---------------------------------------------------------------------------
// The guns carried
// ---------------------------------------------------------------------------

void UBenchWidget::RebuildGuns()
{
	GunList->ClearChildren();
	GunRows.Reset();
	GunsCount->SetText(FText::AsNumber(Guns.Num()));
	if (Guns.IsEmpty())
	{
		GunList->AddChild(MakeNote(TEXT("No guns"), TEXT("Bring a gun to the bench to work on it.")));
		return;
	}
	for (int32 Row = 0; Row < Guns.Num(); ++Row)
	{
		// The equipped guns, then the backpack's under their own heading.
		const bool bGroupStarts = Row == 0 || Guns[Row].bBackpack != Guns[Row - 1].bBackpack;
		if (bGroupStarts)
		{
			AddHeading(WidgetTree, GunList, Guns[Row].bBackpack ? TEXT("Backpack") : TEXT("Equipped"), Row == 0);
		}
		const FRow Made = MakeGunRow(Row);
		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(GunList->AddChild(Made.Button)))
		{
			RowSlot->SetPadding(FMargin(0.f, 6.f, 6.f, 0.f));
		}
		GunRows.Add(Made);
	}
}

UBenchWidget::FRow UBenchWidget::MakeGunRow(int32 Row)
{
	FRow Made;
	const UWeaponManagerComponent* Inventory = Manager.Get();
	const FWeaponInstanceData* Item = Inventory ? Inventory->FindCarriedGun(Guns[Row]) : nullptr;
	check(Item);
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeGunPicture(WidgetTree, *Item, FVector2D(72.f, 24.f)), 72.f, 24.f))
		->SetVerticalAlignment(VAlign_Center);

	// Where it's carried, its level, and a named gun's fixed parts (it can't be changed here).
	FString Where = Guns[Row].bBackpack ? FString(TEXT("Backpack"))
		: FString::Printf(TEXT("Slot %d · %s"), Guns[Row].Index + 1,
			LoadoutCarry::Label(LoadoutCarry::ForSlot(Guns[Row].Index, Inventory->GetWeapons().Num(), Inventory->GetActiveSlot())));
	Where += FString::Printf(TEXT(" · Lv %d"), Item->Level);
	if (!WeaponPartSwap::CanModify(*Item))
	{
		Where += TEXT(" · Fixed parts");
	}
	// Its name in its rarity's color, with the cracked coin before it when it's cursed, as on the loadout's cards.
	UVerticalBox* Text = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Text->AddChildToVerticalBox(MakeGunNameLine(WidgetTree, *Item, 10, LooterWeaponText::Color(*Item), 50));
	Text->AddChildToVerticalBox(FittedLabel(WidgetTree, Where, 8, Color::TextDim(), 120))->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	UHorizontalBoxSlot* TextSlot = Line->AddChildToHorizontalBox(Text);
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TextSlot->SetVerticalAlignment(VAlign_Center);
	TextSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));

	UOverlay* Box = MakeCard(WidgetTree, Line, FMargin(12.f, 7.f), 1.3f, Made.Fill, Made.Line);
	Made.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Made.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, Layout::GunRowHeight), BenchActions::Gun, Row);
	// The screen plays its rows' clicks and hovers itself (as the cursor moves by keys too), so the button stays quiet.
	Made.Button->bPlaysSounds = false;
	Made.Button->OnButtonClicked.BindUObject(this, &UBenchWidget::HandleRowClicked);
	Made.Button->OnButtonHovered.BindUObject(this, &UBenchWidget::HandleRowHovered);
	return Made;
}

// ---------------------------------------------------------------------------
// The chosen gun's slots
// ---------------------------------------------------------------------------

void UBenchWidget::RebuildSlots()
{
	SlotList->ClearChildren();
	SlotRows.Reset();
	SlotsTitle->SetText(FText::FromString(bScrapping ? TEXT("KEEP ONE PART") : TEXT("ON THIS GUN")));
	ScrapButton->SetVisibility(bScrapping ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	StopScrappingButton->SetVisibility(bScrapping ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	const FWeaponInstanceData* Item = ChosenItem();
	const UWeaponManagerComponent* Inventory = Manager.Get();
	// The button reads as out of reach when the gun can't be scrapped; pressed, it says why.
	ScrapButton->SetRenderOpacity(Item && Inventory && Inventory->CanScrap(ChosenRef()) ? 1.f : 0.45f);
	if (!Item)
	{
		SlotsCount->SetText(FText::GetEmpty());
		return;
	}
	int32 Filled = 0;
	for (int32 SlotIndex = 0; SlotIndex < NumSlots(); ++SlotIndex)
	{
		Filled += BenchRules::CurrentPart(*Item, SlotIndex) ? 1 : 0;
	}
	SlotsCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Filled, NumSlots())));
	for (int32 SlotIndex = 0; SlotIndex < NumSlots(); ++SlotIndex)
	{
		const FRow Made = MakeSlotRow(SlotIndex);
		SlotList->AddChildToVerticalBox(Made.Button)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		SlotRows.Add(Made);
	}
}

UBenchWidget::FRow UBenchWidget::MakeSlotRow(int32 SlotIndex)
{
	FRow Made;
	const FWeaponInstanceData* Item = ChosenItem();
	check(Item);
	const bool bFilled = BenchRules::CurrentPart(*Item, SlotIndex) != nullptr;
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(Label(WidgetTree, BenchRules::SlotLabel(SlotNameAt(SlotIndex)), 8, Color::TextDim(), 220));
	Content->AddChildToVerticalBox(FittedLabel(WidgetTree, BenchRules::SlotPartName(*Item, SlotIndex), 10, bFilled ? Color::Text() : Color::TextDim(), 60))
		->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));

	UOverlay* Box = MakeCard(WidgetTree, Content, FMargin(14.f, 9.f), 1.3f, Made.Fill, Made.Line);

	// KEEP, on the part chosen to keep while the gun is being scrapped.
	UTextBlock* Tag = Label(WidgetTree, TEXT("Keep"), 7, Color::AccentDark(), 150);
	Tag->SetShadowColorAndOpacity(FLinearColor::Transparent);
	Made.Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Made.Chip->SetBrush(RectBrush(Color::Accent()));
	Made.Chip->SetPadding(FMargin(7.f, 1.f));
	Made.Chip->SetContent(Tag);
	Made.Chip->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* ChipSlot = Box->AddChildToOverlay(Made.Chip);
	ChipSlot->SetHorizontalAlignment(HAlign_Right);
	ChipSlot->SetVerticalAlignment(VAlign_Center);
	ChipSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));

	Made.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Made.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, Layout::SlotRowHeight), BenchActions::Slot, SlotIndex);
	// The screen plays its rows' clicks and hovers itself (as the cursor moves by keys too), so the button stays quiet.
	Made.Button->bPlaysSounds = false;
	Made.Button->OnButtonClicked.BindUObject(this, &UBenchWidget::HandleRowClicked);
	Made.Button->OnButtonHovered.BindUObject(this, &UBenchWidget::HandleRowHovered);
	return Made;
}

// ---------------------------------------------------------------------------
// The parts box, for the chosen slot
// ---------------------------------------------------------------------------

void UBenchWidget::RebuildParts()
{
	PartList->ClearChildren();
	PartRows.Reset();
	PartEntries.Reset();
	const UWeaponManagerComponent* Inventory = Manager.Get();
	const TArray<FBoxedWeaponPart> NoParts;
	const TArray<FBoxedWeaponPart>& Box = Inventory ? Inventory->GetPartsBox() : NoParts;
	PartsCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Box.Num(), UWeaponManagerComponent::MaxBoxedParts)));
	PartsSub->SetText(FText::GetEmpty());

	const FWeaponInstanceData* Item = ChosenItem();
	if (!Item)
	{
		PartList->AddChild(MakeNote(TEXT("Nothing to work on"), TEXT("Carry a gun to fit parts to it.")));
		return;
	}
	if (bScrapping)
	{
		PartList->AddChild(MakeNote(TEXT("Scrapping keeps one part"), TEXT("Choose the part to keep from the gun's slots. It goes in the parts box; the rest of the gun is gone.")));
		return;
	}
	if (!CanChange())
	{
		PartList->AddChild(MakeNote(TEXT("Fixed parts"), FString::Printf(TEXT("%s is a named gun: its parts are its own. It can't be changed or scrapped."),
			*LooterWeaponText::Name(*Item))));
		return;
	}

	const FName SlotName = SlotNameAt(ChosenSlot);
	PartEntries = BenchRules::PartsForSlot(Box, *Item, SlotName);
	int32 NumFit = 0;
	for (const BenchRules::FPartEntry& Entry : PartEntries)
	{
		NumFit += Entry.Fits() ? 1 : 0;
	}
	PartsSub->SetText(FText::FromString(FString::Printf(TEXT("%s · %d fit this gun"), *BenchRules::SlotLabel(SlotName), NumFit).ToUpper()));
	if (PartEntries.IsEmpty())
	{
		if (Box.IsEmpty())
		{
			PartList->AddChild(MakeNote(TEXT("The box is empty"), TEXT("Scrap a gun you don't need: you keep one of its parts, to fit to another gun of its kind.")));
		}
		else
		{
			const FString SlotWord = BenchRules::SlotLabel(SlotName).ToLower();
			PartList->AddChild(MakeNote(FString::Printf(TEXT("No %s parts"), *SlotWord),
				FString::Printf(TEXT("Scrap a gun of this kind and keep its %s, or choose another slot."), *SlotWord)));
		}
		return;
	}
	for (int32 Row = 0; Row < PartEntries.Num(); ++Row)
	{
		// The ones that don't fit follow under their own heading.
		if (!PartEntries[Row].Fits() && (Row == 0 || PartEntries[Row - 1].Fits()))
		{
			AddHeading(WidgetTree, PartList, TEXT("Don't fit"), Row == 0);
		}
		const FRow Made = MakePartRow(Row);
		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(PartList->AddChild(Made.Button)))
		{
			RowSlot->SetPadding(FMargin(0.f, 6.f, 6.f, 0.f));
		}
		PartRows.Add(Made);
	}
}

UBenchWidget::FRow UBenchWidget::MakePartRow(int32 Row)
{
	FRow Made;
	const BenchRules::FPartEntry& Entry = PartEntries[Row];
	const FBoxedWeaponPart* Part = RowPart(Row);
	const FWeaponInstanceData* Item = ChosenItem();
	check(Part && Item);

	UVerticalBox* Text = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* NameSlot = Top->AddChildToHorizontalBox(FittedLabel(WidgetTree, BenchRules::PartName(*Part), 10,
		Entry.Fits() ? Color::Text() : Color::TextDim(), 50));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);
	// Another kind's part (a shotgun's barrel under a rifle's slot) says whose it is.
	if (Part->Definition && Part->Definition != Item->Definition)
	{
		UHorizontalBoxSlot* KindSlot = Top->AddChildToHorizontalBox(Label(WidgetTree, Part->Definition->DisplayName.ToString(), 7, Color::TextDim(), 150));
		KindSlot->SetVerticalAlignment(VAlign_Center);
		KindSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	}
	Text->AddChildToVerticalBox(Top);

	if (Entry.Fits())
	{
		// What fitting it changes against the part in the slot now, the biggest changes first.
		const TArray<BenchRules::FStatChange> Changes = BenchRules::StatChanges(UWeaponRollLibrary::ComputeInstanceStats(*Item),
			WeaponPartSwap::PreviewStats(*Item, *Part));
		UHorizontalBox* Chips = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 Index = 0; Index < FMath::Min(Changes.Num(), RowChanges); ++Index)
		{
			Chips->AddChildToHorizontalBox(Label(WidgetTree, Changes[Index].Text, 8, Changes[Index].bBetter ? Color::Better() : Color::Worse(), 60))
				->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		}
		if (Changes.IsEmpty())
		{
			Chips->AddChildToHorizontalBox(Label(WidgetTree, TEXT("Same stats"), 8, Color::TextDim(), 60));
		}
		else if (Changes.Num() > RowChanges)
		{
			Chips->AddChildToHorizontalBox(Label(WidgetTree, FString::Printf(TEXT("+%d more"), Changes.Num() - RowChanges), 8, Color::TextDim(), 60));
		}
		Text->AddChildToVerticalBox(Chips)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	else
	{
		Text->AddChildToVerticalBox(FittedLabel(WidgetTree, WeaponPartSwap::CheckText(Entry.Check).ToString(), 8, Color::Worse(), 100))
			->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		// Dimmed: there to see, not to fit.
		Text->SetRenderOpacity(0.6f);
	}

	UOverlay* Box = MakeCard(WidgetTree, Text, FMargin(14.f, 8.f), 1.3f, Made.Fill, Made.Line);
	Made.Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Made.Button->SetupContent(MakeSized(WidgetTree, Box, 0.f, Layout::PartRowHeight), BenchActions::Part, Row);
	// The screen plays its rows' clicks and hovers itself (as the cursor moves by keys too), so the button stays quiet.
	Made.Button->bPlaysSounds = false;
	Made.Button->OnButtonClicked.BindUObject(this, &UBenchWidget::HandleRowClicked);
	Made.Button->OnButtonHovered.BindUObject(this, &UBenchWidget::HandleRowHovered);
	return Made;
}

UWidget* UBenchWidget::MakeNote(const FString& Title, const FString& Body)
{
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Content->AddChildToVerticalBox(Label(WidgetTree, Title, 10, Color::Title(), 200));
	UTextBlock* BodyText = MakeText(WidgetTree, Body, 10, Color::TextDim());
	BodyText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(BodyText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	UImage* Fill = nullptr;
	UImage* Line = nullptr;
	UOverlay* Card = MakeCard(WidgetTree, Content, FMargin(16.f, 12.f), 1.3f, Fill, Line);
	Fill->SetColorAndOpacity(Colors::EmptyFill());
	Line->SetColorAndOpacity(Faded(Colors::CardLine(), 0.6f));
	Card->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Card;
}

// ---------------------------------------------------------------------------
// The cursor and the choices, shown
// ---------------------------------------------------------------------------

void UBenchWidget::Restyle()
{
	const FLinearColor ChosenLine = Faded(Color::Accent(), 0.55f);
	for (int32 Row = 0; Row < GunRows.Num(); ++Row)
	{
		const bool bChosen = Row == ChosenGun;
		const bool bCursor = Column == EColumn::Guns && CursorIndex == Row;
		FLinearColor Fill = bChosen ? Colors::InHandFill() : Colors::CardFill();
		FLinearColor Line = bChosen ? ChosenLine : Colors::CardLine();
		if (bCursor)
		{
			Fill = bChosen ? Colors::InHandCursor() : Color::Tile();
			Line = Color::Accent();
		}
		GunRows[Row].Fill->SetColorAndOpacity(Fill);
		GunRows[Row].Line->SetColorAndOpacity(Line);
	}

	const FWeaponInstanceData* Item = ChosenItem();
	for (int32 SlotIndex = 0; SlotIndex < SlotRows.Num(); ++SlotIndex)
	{
		const bool bFilled = Item && BenchRules::CurrentPart(*Item, SlotIndex);
		const bool bCursor = Column == EColumn::Slots && CursorIndex == SlotIndex;
		const bool bChosen = SlotIndex == ChosenSlot && !bScrapping;
		FLinearColor Fill = bFilled ? Colors::CardFill() : Colors::EmptyFill();
		FLinearColor Line = Colors::CardLine();
		bool bKeep = false;
		if (bScrapping)
		{
			// Every part but the one kept goes with the gun.
			bKeep = bCursor && bFilled;
			Line = bKeep ? Color::Accent() : (bFilled ? Faded(Color::Worse(), 0.45f) : Faded(Colors::CardLine(), 0.5f));
			Fill = bKeep ? Colors::PickedFill() : Fill;
		}
		else
		{
			if (bChosen)
			{
				Fill = Colors::InHandFill();
				Line = ChosenLine;
			}
			if (bCursor)
			{
				Fill = bChosen ? Colors::InHandCursor() : Color::Tile();
				Line = Color::Accent();
			}
		}
		SlotRows[SlotIndex].Fill->SetColorAndOpacity(Fill);
		SlotRows[SlotIndex].Line->SetColorAndOpacity(Line);
		SlotRows[SlotIndex].Chip->SetVisibility(bKeep ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	for (int32 Row = 0; Row < PartRows.Num(); ++Row)
	{
		const bool bFits = PartEntries.IsValidIndex(Row) && PartEntries[Row].Fits();
		const bool bCursor = Column == EColumn::Parts && CursorIndex == Row;
		FLinearColor Fill = bFits ? Colors::CardFill() : Colors::EmptyFill();
		FLinearColor Line = bFits ? Colors::CardLine() : Faded(Colors::CardLine(), 0.5f);
		if (bCursor)
		{
			// The cursor on a part that doesn't fit is lit in the warning's color: pressing it only says why.
			Fill = bFits ? Color::Tile() : Colors::EmptyFill();
			Line = bFits ? Color::Accent() : Faded(Color::Worse(), 0.8f);
		}
		PartRows[Row].Fill->SetColorAndOpacity(Fill);
		PartRows[Row].Line->SetColorAndOpacity(Line);
	}
}

const TArray<UBenchWidget::FRow>& UBenchWidget::RowsOf(EColumn InColumn) const
{
	switch (InColumn)
	{
	case EColumn::Slots: return SlotRows;
	case EColumn::Parts: return PartRows;
	case EColumn::Guns:  break;
	}
	return GunRows;
}
