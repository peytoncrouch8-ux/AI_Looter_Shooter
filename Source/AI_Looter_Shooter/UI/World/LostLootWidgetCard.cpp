// ULostLootWidget: the chosen gun's card as the inventory shows one (its picture, name, level and rarity, its verdict against
// the gun in hand, damage and the first-glance stats, its parts, its notches and curse, where it was found), the claim bar
// under it, and CLAIMED stamped over it.

#include "UI/World/LostLootWidget.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LostLootRules.h"
#include "Loot/LostLootSubsystem.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "LooterLostLoot"

using namespace LooterUI;
using LoadoutRules::EStat;

namespace
{
	/** The card's columns for a stat: its name, the bar, its value. */
	constexpr float StatNameWidth = 92.f;
	constexpr float StatValueWidth = 62.f;
	constexpr float StatBarHeight = 5.f;
	const FVector2D CardPicture(300.f, 100.f);
	/** The card's title line: its size, the smallest a long name is set, and the room it has (the card less its padding). */
	constexpr int32 TitleSize = 19;
	constexpr int32 TitleMinSize = 14;
	constexpr float TitleWidth = LostLootLayout::CardWidth - 16.f - 10.f - 24.f;

	/** CLAIMED is pressed by hand: tilted, and it comes down from this much bigger. */
	constexpr float StampAngle = -8.f;
	constexpr float StampDrop = 1.9f;
	/** The card jolts this far (px) as the stamp lands, settling over JoltSeconds. */
	constexpr float JoltDepth = 3.f;
	constexpr float JoltSeconds = 0.12f;

	float Clamp01(float Value)
	{
		return FMath::Clamp(Value, 0.f, 1.f);
	}

	float EaseIn(float Alpha)
	{
		const float In = Clamp01(Alpha);
		return In * In * In;
	}

	UWidget* Rule(UWidgetTree* Tree)
	{
		return MakeSized(Tree, MakeImage(Tree, RectBrush(Color::RowLine())), 0.f, 1.f);
	}

	/** An orange section title inside the card. */
	UWidget* Section(UWidgetTree* Tree, const FText& Title)
	{
		return LoadoutParts::Label(Tree, Title.ToString(), 8, Color::Accent(), 300);
	}

	/** A stat's bar: the gun's rating in the kit's lit segment colour over the unlit, at whatever width it's given. */
	UWidget* StatBar(UWidgetTree* Tree, float Rating)
	{
		UHorizontalBox* Bar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const float Lit = Clamp01(Rating);
		auto Part = [Tree, Bar](float Weight, const FLinearColor& PartColor)
		{
			if (Weight < 0.005f)
			{
				return;
			}
			UHorizontalBoxSlot* PartSlot = Bar->AddChildToHorizontalBox(MakeSized(Tree, MakeImage(Tree, RectBrush(PartColor)), 0.f, StatBarHeight));
			FSlateChildSize Size(ESlateSizeRule::Fill);
			Size.Value = Weight;
			PartSlot->SetSize(Size);
			PartSlot->SetVerticalAlignment(VAlign_Center);
		};
		Part(Lit, Color::SegmentOn());
		Part(1.f - Lit, Color::SegmentOff());
		return Bar;
	}
}

// ---------------------------------------------------------------------------
// The card
// ---------------------------------------------------------------------------

void ULostLootWidget::RefreshCard()
{
	// While CLAIMED shows, the claimed gun's card stays under it.
	if (!CardBox || bStamping)
	{
		return;
	}
	CardBox->ClearChildren();
	const FLostLootEntry* Entry = GetChosenEntry();
	if (Entry)
	{
		CardTicket = Entry->Ticket;
		FillCard(*Entry);
	}
	else
	{
		CardTicket = INDEX_NONE;
		FillEmptyCard();
	}
	if (ClaimHolder)
	{
		ClaimHolder->SetContent(Entry ? MakeClaimBar(*Entry) : nullptr);
	}
	if (CardScroll)
	{
		CardScroll->ScrollToStart();
	}
}

void ULostLootWidget::FillCard(const FLostLootEntry& Entry)
{
	const FWeaponInstanceData& Gun = Entry.Weapon;

	// The ticket it's claimed by, and when it came in.
	{
		UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		const FString Ticket = FText::Format(LOCTEXT("CardTicket", "Claim ticket No. {0}"), FText::FromString(FLostLootStore::TicketText(Entry.Ticket))).ToString();
		UHorizontalBoxSlot* TicketSlot = Top->AddChildToHorizontalBox(LoadoutParts::Label(WidgetTree, Ticket, 9, Color::Accent(), 300));
		TicketSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TicketSlot->SetVerticalAlignment(VAlign_Center);
		const FText Ago = AgoWords(Entry.HandedInAt);
		if (!Ago.IsEmpty())
		{
			Top->AddChildToHorizontalBox(LoadoutParts::Label(WidgetTree, Ago.ToString(), 9, Color::TextDim(), 120))->SetVerticalAlignment(VAlign_Center);
		}
		CardBox->AddChildToVerticalBox(Top);
	}

	// The gun: its picture, its name in its rarity's colour, what it is.
	CardBox->AddChildToVerticalBox(MakeSized(WidgetTree, LoadoutParts::MakeGunPicture(WidgetTree, Gun, CardPicture), 0.f, CardPicture.Y + 6.f))
		->SetPadding(FMargin(0.f, 8.f, 0.f, 4.f));
	CardBox->AddChildToVerticalBox(LoadoutParts::MakeGunNameLine(WidgetTree, Gun, TitleSize, LooterWeaponText::Color(Gun), 40, TitleWidth, TitleMinSize));
	const FString Kind = Gun.Definition && !Gun.Definition->DisplayName.IsEmpty() ? Gun.Definition->DisplayName.ToString() : FString();
	CardBox->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, FText::Format(LOCTEXT("CardWhat", "Level {0} · {1} {2}"), FText::AsNumber(Gun.Level),
		UEnum::GetDisplayValueAsText(Gun.Rarity), FText::FromString(Kind)).ToString(), 10, Color::TextDim(), 120))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	const FString Flavor = LooterWeaponText::FlavorLine(Gun);
	if (!Flavor.IsEmpty())
	{
		UTextBlock* FlavorText = MakeText(WidgetTree, Flavor, 11, LooterWeaponText::FlavorColor());
		FlavorText->SetFont(LooterUI::Font(11, /*bBold*/ false));
		FlavorText->SetAutoWrapText(true);
		CardBox->AddChildToVerticalBox(FlavorText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}

	// Against the gun in hand, as the inventory's verdict reads it (only a gun of the same kind is compared).
	const UWeaponManagerComponent* Weapons = GetManager();
	const AWeaponBase* InHand = Weapons ? Weapons->GetActiveWeapon() : nullptr;
	const FWeaponInstanceData* Held = InHand ? &InHand->GetInstance() : nullptr;
	const LoadoutRules::EVerdict Verdict = LoadoutRules::Compare(Gun, Held);
	if (Verdict != LoadoutRules::EVerdict::None && Held)
	{
		const FText HeldName = FText::FromString(LooterWeaponText::Name(*Held));
		const FText Words = Verdict == LoadoutRules::EVerdict::Upgrade ? FText::Format(LOCTEXT("Better", "Better than your {0}"), HeldName)
			: Verdict == LoadoutRules::EVerdict::Weaker ? FText::Format(LOCTEXT("Weaker", "Weaker than your {0}"), HeldName)
			: FText::Format(LOCTEXT("Similar", "Much like your {0}"), HeldName);
		const FLinearColor WordsColor = Verdict == LoadoutRules::EVerdict::Upgrade ? Color::Better()
			: Verdict == LoadoutRules::EVerdict::Weaker ? Color::Worse() : Color::TextDim();
		CardBox->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, Words.ToString(), 9, WordsColor, 160))->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	// Damage big, then the four that decide how it fights.
	const FWeaponStats& Stats = Gun.Stats;
	CardBox->AddChildToVerticalBox(Rule(WidgetTree))->SetPadding(FMargin(0.f, 10.f, 0.f, 8.f));
	CardBox->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LoadoutRules::StatName(EStat::Damage), 8, Color::TextDim(), 260));
	CardBox->AddChildToVerticalBox(MakeText(WidgetTree, LoadoutRules::StatText(EStat::Damage, Stats), 28, Color::Title()))
		->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	for (const EStat Stat : LoadoutRules::FirstGlanceStats())
	{
		if (Stat == EStat::Damage)
		{
			continue;
		}
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, LoadoutParts::Label(WidgetTree, LoadoutRules::StatName(Stat), 8, Color::TextDim(), 140),
			StatNameWidth, 0.f))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* BarSlot = Line->AddChildToHorizontalBox(StatBar(WidgetTree, LoadoutRules::StatRating(Stat, Stats)));
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Value = MakeText(WidgetTree, LoadoutRules::StatText(Stat, Stats), 11, Color::Text());
		Value->SetJustification(ETextJustify::Right);
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Value, StatValueWidth, 0.f))->SetVerticalAlignment(VAlign_Center);
		CardBox->AddChildToVerticalBox(MakeSized(WidgetTree, Line, 0.f, 22.f))->SetPadding(FMargin(0.f, 1.f));
	}

	// Its parts, slot by slot, as the inventory's Inspect lists them, and what they add up to.
	if (Gun.Definition && !Gun.Definition->Parts.IsEmpty())
	{
		CardBox->AddChildToVerticalBox(Section(WidgetTree, LOCTEXT("Parts", "Parts")))->SetPadding(FMargin(0.f, 14.f, 0.f, 4.f));
		const FWeaponLook Look = WeaponParts::Pick(Gun);
		for (int32 Index = 0; Index < Gun.Definition->Parts.Num(); ++Index)
		{
			const FWeaponPartOption* Part = Look.Parts.IsValidIndex(Index) ? Look.Parts[Index] : nullptr;
			const FString PartName = !Part ? LOCTEXT("NoPart", "None").ToString()
				: (Part->DisplayName.IsEmpty() ? Part->Key.ToString() : Part->DisplayName.ToString());
			UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Line->AddChildToHorizontalBox(MakeSized(WidgetTree, LoadoutParts::Label(WidgetTree, Gun.Definition->Parts[Index].Name.ToString(), 8,
				Color::TextDim(), 140), StatNameWidth, 0.f))->SetVerticalAlignment(VAlign_Center);
			UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(LoadoutParts::FittedLabel(WidgetTree, PartName, 9,
				Part ? Color::Text() : Color::TextDim(), 60));
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NameSlot->SetVerticalAlignment(VAlign_Center);
			CardBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 2.f));
		}
		for (const LoadoutRules::FBonus& Bonus : LoadoutRules::Bonuses(LoadoutRules::PartTotals(Gun)))
		{
			CardBox->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LoadoutRules::BonusText(Bonus), 9,
				Bonus.Goodness > 0.f ? Color::Better() : Color::Worse(), 120))->SetPadding(FMargin(0.f, 2.f));
		}
	}

	// Its notches and its curse, which ride with it through the shelves.
	if (UWidget* Ideas = LoadoutParts::MakeGunIdeasRows(WidgetTree, Gun, 9))
	{
		CardBox->AddChildToVerticalBox(Section(WidgetTree, LOCTEXT("History", "History")))->SetPadding(FMargin(0.f, 14.f, 0.f, 6.f));
		CardBox->AddChildToVerticalBox(Ideas);
	}

	// Where it was found, as the ticket says.
	CardBox->AddChildToVerticalBox(Section(WidgetTree, LOCTEXT("Found", "Found")))->SetPadding(FMargin(0.f, 14.f, 0.f, 4.f));
	UTextBlock* Found = MakeText(WidgetTree, LostLootRules::ReasonWords(Entry.Reason, Entry.Map).ToString(), 11, Color::Text());
	Found->SetFont(LooterUI::Font(11, /*bBold*/ false));
	Found->SetAutoWrapText(true);
	CardBox->AddChildToVerticalBox(Found);
}

void ULostLootWidget::FillEmptyCard()
{
	const ULostLootSubsystem* LostLoot = GetLostLoot();
	const int32 Capacity = LostLoot ? LostLoot->GetCapacity() : LostLootRules::Capacity(0);
	CardBox->AddChildToVerticalBox(LoadoutParts::Label(WidgetTree, LOCTEXT("EmptyTitle", "Nothing to claim").ToString(), 16, Color::TextDim(), 120));
	UTextBlock* Body = MakeText(WidgetTree, FText::Format(LOCTEXT("EmptyBody", "A gun left lying for five minutes is brought here, and so is "
		"every gun left on the ground when you leave an area or call it a day. The shelves hold {0}; when they're full, the oldest "
		"goes to make room."), FText::AsNumber(Capacity)).ToString(), 11, Color::TextDim());
	Body->SetFont(LooterUI::Font(11, /*bBold*/ false));
	Body->SetAutoWrapText(true);
	CardBox->AddChildToVerticalBox(Body)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
}

UWidget* ULostLootWidget::MakeClaimBar(const FLostLootEntry& Entry)
{
	// Where it goes: the backpack (its count), a free slot, or nowhere.
	const UWeaponManagerComponent* Weapons = GetManager();
	const LostLootRules::ETake Where = Weapons ? LostLootRules::WhereItGoes(*Weapons) : LostLootRules::ETake::NoRoom;
	FText Words;
	switch (Where)
	{
	case LostLootRules::ETake::InBackpack:
		Words = FText::Format(LOCTEXT("IntoBackpack", "Into your backpack · {0} / {1}"), FText::AsNumber(Weapons->GetBackpackUsed()),
			FText::AsNumber(Weapons->BackpackCapacity));
		break;
	case LostLootRules::ETake::InSlot:
		Words = LOCTEXT("IntoSlot", "Your backpack is full: into a free slot");
		break;
	default:
		Words = LOCTEXT("NoRoomBar", "No room: your backpack and slots are full");
		break;
	}
	const bool bRoom = Where != LostLootRules::ETake::NoRoom;
	UHorizontalBox* Bar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* WordsText = MakeText(WidgetTree, Words.ToString(), 11, bRoom ? Color::TextDim() : Color::Worse());
	WordsText->SetAutoWrapText(true);
	UHorizontalBoxSlot* WordsSlot = Bar->AddChildToHorizontalBox(WordsText);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	// The call to action in orange when it can be claimed; a quiet one that says why not when it can't.
	ULooterButton* ClaimButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	ClaimButton->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), LostLootActions::Claim, Entry.Ticket, LOCTEXT("Claim", "Claim"),
		13, bRoom ? EButtonKind::Primary : EButtonKind::Mini);
	ClaimButton->bPlaysSounds = false;
	ClaimButton->OnButtonClicked.BindUObject(this, &ULostLootWidget::HandleButton);
	Bar->AddChildToHorizontalBox(MakeSized(WidgetTree, ClaimButton, 150.f, 38.f))->SetVerticalAlignment(VAlign_Center);
	return Bar;
}

// ---------------------------------------------------------------------------
// CLAIMED
// ---------------------------------------------------------------------------

UWidget* ULostLootWidget::MakeStamp()
{
	// CLAIMED and the ticket's number, pressed over the card in the kit's orange on dark glass, so it reads over the words.
	const FLinearColor Ink = Color::Accent();
	const FLinearColor Glass = Color::ScreenBg();
	UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Plate->SetBrush(RectBrush(FLinearColor(Glass.R, Glass.G, Glass.B, 0.82f), Ink, 3.f));
	MarkBackground(Plate);
	Plate->SetPadding(FMargin(28.f, 8.f, 28.f, 10.f));
	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UTextBlock* Word = MakeFloatingText(WidgetTree, 34, Ink, 520, ETextJustify::Center);
	Word->SetText(LOCTEXT("Claimed", "CLAIMED"));
	Words->AddChildToVerticalBox(Word)->SetHorizontalAlignment(HAlign_Center);
	StampTicketText = MakeFloatingText(WidgetTree, 11, Ink, 320, ETextJustify::Center);
	Words->AddChildToVerticalBox(StampTicketText)->SetHorizontalAlignment(HAlign_Center);
	Plate->SetContent(Words);
	Plate->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Plate->SetVisibility(ESlateVisibility::Collapsed);
	return Plate;
}

void ULostLootWidget::ApplyStamp()
{
	if (!StampWidget)
	{
		return;
	}
	const ESlateVisibility Wanted = bStamping ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (StampWidget->GetVisibility() != Wanted)
	{
		StampWidget->SetVisibility(Wanted);
	}
	if (!bStamping)
	{
		if (CardScroll)
		{
			CardScroll->SetRenderOpacity(1.f);
			CardScroll->SetRenderTranslation(FVector2D::ZeroVector);
		}
		return;
	}
	if (StampTicketText)
	{
		StampTicketText->SetText(FText::Format(LOCTEXT("StampTicket", "TICKET No. {0}"), FText::FromString(FLostLootStore::TicketText(Stamped.Ticket))));
	}
	// Slammed down from big, held, then lifting away with the claimed card fading under it.
	const float Slam = Clamp01(StampAge / StampSlamSeconds);
	const float Out = Clamp01((StampAge - StampSlamSeconds - StampHoldSeconds) / StampOutSeconds);
	StampWidget->SetRenderTransform(FWidgetTransform(FVector2D(0.f, -Out * 16.f), FVector2D(FMath::Lerp(StampDrop, 1.f, EaseIn(Slam))),
		FVector2D::ZeroVector, StampAngle));
	StampWidget->SetRenderOpacity(FMath::Min(1.f, Slam * 1.6f) * (1.f - Out));
	if (CardScroll)
	{
		// The card takes the blow: a little jolt down as the stamp lands, settling at once.
		const float SinceLanding = StampAge - StampSlamSeconds;
		const float Jolt = SinceLanding >= 0.f && SinceLanding < JoltSeconds ? JoltDepth * (1.f - SinceLanding / JoltSeconds) : 0.f;
		CardScroll->SetRenderTranslation(FVector2D(0.f, Jolt));
		CardScroll->SetRenderOpacity(1.f - 0.65f * Out);
	}
}

#undef LOCTEXT_NAMESPACE
