// ULoadoutWidget: the card of the gun under the cursor (at a glance: what it is, its damage big, four stats against the
// gun it would replace, its best part bonuses, its notches or curse as badges; on Inspect: everything), and the showcase
// that puts the same gun on show.

#include "UI/Inventory/LoadoutWidget.h"
#include "UI/Inventory/LoadoutGunStage.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Inventory/LoadoutRules.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponNotches.h"
#include "Weapons/WeaponParts.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;
using namespace LoadoutParts;
using LoadoutRules::EStat;

namespace
{
	/** A stat row's columns: its name, then the bar, its value, the arrow and the change. */
	constexpr float StatNameWidth = 92.f;
	constexpr float StatValueWidth = 62.f;
	constexpr float StatArrowWidth = 18.f;
	constexpr float StatDeltaWidth = 56.f;
	constexpr float StatBarHeight = 5.f;
	/** The card's title: its type size, the smallest a long name is set, and the line it fits (the card less its padding
	 * either side and a scroll bar's room). */
	constexpr int32 TitleSize = 19;
	constexpr int32 TitleMinSize = 14;
	constexpr float TitleWidth = LoadoutLayout::CardWidth - 20.f - 14.f - 10.f;
	/** Rarities from here up have a glow that breathes. */
	constexpr EWeaponRarity BreathingRarity = EWeaponRarity::Epic;

	/** A thin rule across the card between its parts. */
	UWidget* Rule(UWidgetTree* Tree)
	{
		return MakeSized(Tree, MakeImage(Tree, RectBrush(Color::RowLine())), 0.f, 1.f);
	}

	/** The up (better) or down (worse) arrow, in its colour. */
	UWidget* ChangeArrow(UWidgetTree* Tree, bool bBetter, const FVector2D& Size)
	{
		return MakeImage(Tree, IconBrush(bBetter ? TEXT("ArrowUp") : TEXT("ArrowDown"), ArrowIcon(bBetter), 4.f, Size,
			bBetter ? Color::Better() : Color::Worse()));
	}

	/**
	 * A stat's bar: the gun's rating in cyan; against another gun, what it gains in green or what it would lose in red,
	 * after the part they share. Built of weighted parts, so it fits whatever width it's given.
	 */
	UWidget* StatBar(UWidgetTree* Tree, float Rating, TOptional<float> Before)
	{
		UHorizontalBox* Bar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
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
		const float Now = FMath::Clamp(Rating, 0.f, 1.f);
		const float Then = FMath::Clamp(Before.Get(Now), 0.f, 1.f);
		Part(FMath::Min(Now, Then), Color::SegmentOn());
		if (Now > Then)
		{
			Part(Now - Then, Color::Better());
		}
		else
		{
			Part(Then - Now, Color::Worse() * FLinearColor(1.f, 1.f, 1.f, 0.7f));
		}
		Part(1.f - FMath::Max(Now, Then), Color::SegmentOff());
		return Bar;
	}

	/** A small framed tag: a glyph and words (the curse, the notches). */
	UWidget* Badge(UWidgetTree* Tree, UWidget* Glyph, const TArray<TPair<FString, FLinearColor>>& Words, const FLinearColor& Edge)
	{
		UHorizontalBox* Line = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Line->AddChildToHorizontalBox(Glyph)->SetVerticalAlignment(VAlign_Center);
		for (const TPair<FString, FLinearColor>& Word : Words)
		{
			UHorizontalBoxSlot* WordSlot = Line->AddChildToHorizontalBox(Label(Tree, Word.Key, 9, Word.Value, 140));
			WordSlot->SetVerticalAlignment(VAlign_Center);
			WordSlot->SetPadding(FMargin(7.f, 0.f, 0.f, 0.f));
		}
		UBorder* Frame = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Frame->SetBrush(RectBrush(Color::Plate() * FLinearColor(1.f, 1.f, 1.f, 0.8f), Edge, 1.f));
		MarkBackground(Frame);
		Frame->SetPadding(FMargin(8.f, 3.f, 10.f, 3.f));
		Frame->SetContent(Line);
		return Frame;
	}

	/** An orange section title inside the card (Inspect's parts). */
	UWidget* Section(UWidgetTree* Tree, const TCHAR* Title)
	{
		return Label(Tree, Title, 8, Color::Accent(), 300);
	}
}

// ---------------------------------------------------------------------------
// The card
// ---------------------------------------------------------------------------

void ULoadoutWidget::RefreshCard()
{
	if (!CardBox)
	{
		return;
	}
	CardBox->ClearChildren();
	const FWeaponInstanceData* Gun = CursorItem();

	// A different gun (or an empty slot) slides its card in (TickMotion).
	const uint32 Identity = Gun ? LoadoutRules::GunIdentity(*Gun) : static_cast<uint32>(0x51070000 + CursorIndex);
	if (Identity != CardIdentity)
	{
		CardIdentity = Identity;
		CardIntroAge = 0.f;
		if (CardScroll)
		{
			CardScroll->ScrollToStart();
		}
	}

	if (!Gun)
	{
		CardRarity = Color::TextDim();
		bCardBreathes = false;
		CardStripe->SetColorAndOpacity(Colors::CardLine());
		CardLine->SetColorAndOpacity(Colors::CardLine());
		CardGlow->SetColorAndOpacity(FLinearColor::Transparent);
		const UWeaponManagerComponent* Inventory = Manager.Get();
		const bool bBackpackGuns = Inventory && !Inventory->GetBackpack().IsEmpty();
		if (NumWeapons() == 0 && !bBackpackGuns)
		{
			AddCardEmpty(TEXT("No guns yet"), TEXT("Look at a gun and tap Interact to pick it up, or hold it to take it straight in hand."));
		}
		else
		{
			AddCardEmpty(FString::Printf(TEXT("Slot %d is empty"), CursorIndex + 1),
				bBackpackGuns ? TEXT("E: choose a gun from the backpack for it.") : TEXT("The next gun you pick up goes here."));
		}
		return;
	}

	// The card wears the gun's rarity: its top edge, its outline and a glow behind it.
	CardRarity = RarityColor(Gun);
	bCardBreathes = Gun->Rarity >= BreathingRarity;
	CardStripe->SetColorAndOpacity(CardRarity);
	CardLine->SetColorAndOpacity(CardRarity * FLinearColor(1.f, 1.f, 1.f, 0.65f));
	CardGlow->SetColorAndOpacity(CardRarity * FLinearColor(1.f, 1.f, 1.f, 0.14f));

	// Over the name: where it is, or how it compares with the gun it would replace.
	if (Zone == EZone::Slots)
	{
		const bool bInHand = CursorIndex == GetActiveSlot();
		AddCardBanner(FString::Printf(TEXT("Slot %d%s"), CursorIndex + 1, bInHand ? TEXT(" · in hand") : TEXT("")),
			bInHand ? Color::Accent() : Color::TextDim());
	}
	else
	{
		const FWeaponInstanceData* Target = SlotItem(TargetSlot);
		const int32 SlotNumber = TargetSlot + 1;
		switch (LoadoutRules::Compare(*Gun, Target))
		{
		case LoadoutRules::EVerdict::Upgrade:
			AddCardBanner(FString::Printf(TEXT("Upgrade over slot %d"), SlotNumber), Color::Better(), 1);
			break;
		case LoadoutRules::EVerdict::Weaker:
			AddCardBanner(FString::Printf(TEXT("Weaker than slot %d"), SlotNumber), Color::Worse(), -1);
			break;
		case LoadoutRules::EVerdict::Similar:
			AddCardBanner(FString::Printf(TEXT("About the same as slot %d"), SlotNumber), Color::TextDim());
			break;
		default:
			AddCardBanner(Target ? FString::Printf(TEXT("Backpack · slot %d holds another kind"), SlotNumber)
				: FString::Printf(TEXT("Backpack · slot %d is free"), SlotNumber), Color::TextDim());
			break;
		}
	}

	// Its name in its rarity's colour, then what it is.
	// A long name (a legendary's, a Named gun's nickname) is set smaller to fit one line, down to TitleMinSize; one longer
	// still wraps to a second line rather than leaving the card.
	UTextBlock* Name = Label(WidgetTree, LooterWeaponText::Name(*Gun), TitleSize, CardRarity, 40);
	FitTextToWidth(Name, TitleWidth, TitleMinSize);
	Name->SetAutoWrapText(true);
	CardBox->AddChildToVerticalBox(Name)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	const FString Kind = Gun->Definition ? Gun->Definition->DisplayName.ToString() : FString();
	CardBox->AddChildToVerticalBox(Label(WidgetTree, FString::Printf(TEXT("%s · Lv %d"), *Kind, Gun->Level), 9, Color::TextDim(), 160))
		->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	// A named gun's line under its name, as written: its voice, the card's one flourish.
	const FString Flavor = LooterWeaponText::FlavorLine(*Gun);
	if (!Flavor.IsEmpty())
	{
		UTextBlock* FlavorText = MakeText(WidgetTree, Flavor, 11, LooterWeaponText::FlavorColor());
		FlavorText->SetFont(LooterWeaponText::FlavorFont(11));
		FlavorText->SetAutoWrapText(true);
		CardBox->AddChildToVerticalBox(FlavorText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}

	AddCardStats(*Gun, Baseline());
	if (bInspecting)
	{
		AddCardInspect(*Gun, Baseline());
	}
	else
	{
		// What its parts do best, in one line: the gun's character at a glance.
		TArray<FString> Lines;
		for (const LoadoutRules::FBonus& Bonus : LoadoutRules::TopBonuses(LoadoutRules::PartTotals(*Gun)))
		{
			Lines.Add(LoadoutRules::BonusText(Bonus));
		}
		if (!Lines.IsEmpty())
		{
			CardBox->AddChildToVerticalBox(Label(WidgetTree, FString::Join(Lines, TEXT("  ·  ")), 9, Color::CyanText(), 120))
				->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
		}
		AddCardBadges(*Gun);
	}
}

void ULoadoutWidget::AddCardBanner(const FString& Words, const FLinearColor& WordsColor, int32 Arrow)
{
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (Arrow != 0)
	{
		UHorizontalBoxSlot* ArrowSlot = Line->AddChildToHorizontalBox(ChangeArrow(WidgetTree, Arrow > 0, FVector2D(12.f, 10.f)));
		ArrowSlot->SetVerticalAlignment(VAlign_Center);
		ArrowSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
	Line->AddChildToHorizontalBox(Label(WidgetTree, Words, 9, WordsColor, 220))->SetVerticalAlignment(VAlign_Center);
	CardBox->AddChildToVerticalBox(Line);
}

void ULoadoutWidget::AddCardStats(const FWeaponInstanceData& Gun, const FWeaponInstanceData* Against)
{
	const FWeaponStats& S = Gun.Stats;
	const FWeaponStats* B = Against ? &Against->Stats : nullptr;
	CardBox->AddChildToVerticalBox(Rule(WidgetTree))->SetPadding(FMargin(0.f, 12.f, 0.f, 10.f));

	// Damage, big: the number the gun is judged by, with its change beside it.
	{
		UHorizontalBox* Hero = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UVerticalBox* Number = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Number->AddChildToVerticalBox(Label(WidgetTree, LoadoutRules::StatName(EStat::Damage), 8, Color::TextDim(), 260));
		Number->AddChildToVerticalBox(MakeText(WidgetTree, LoadoutRules::StatText(EStat::Damage, S), 30, Color::Title()));
		Hero->AddChildToHorizontalBox(Number)->SetVerticalAlignment(VAlign_Bottom);
		if (B && LoadoutRules::CompareStat(EStat::Damage, S, *B) != LoadoutRules::EChange::Same)
		{
			const bool bBetter = LoadoutRules::CompareStat(EStat::Damage, S, *B) == LoadoutRules::EChange::Better;
			UHorizontalBoxSlot* ArrowSlot = Hero->AddChildToHorizontalBox(ChangeArrow(WidgetTree, bBetter, FVector2D(14.f, 11.f)));
			ArrowSlot->SetVerticalAlignment(VAlign_Bottom);
			ArrowSlot->SetPadding(FMargin(14.f, 0.f, 6.f, 12.f));
			UHorizontalBoxSlot* DeltaSlot = Hero->AddChildToHorizontalBox(MakeText(WidgetTree, LoadoutRules::DeltaText(EStat::Damage, S, *B), 14,
				bBetter ? Color::Better() : Color::Worse()));
			DeltaSlot->SetVerticalAlignment(VAlign_Bottom);
			DeltaSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 7.f));
		}
		CardBox->AddChildToVerticalBox(Hero)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	// The rest at first glance (fire rate, magazine, reload, accuracy); every one on Inspect.
	const TConstArrayView<EStat> Stats = bInspecting ? LoadoutRules::AllStats() : LoadoutRules::FirstGlanceStats();
	for (const EStat Stat : Stats)
	{
		if (Stat == EStat::Damage)
		{
			continue;
		}
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Label(WidgetTree, LoadoutRules::StatName(Stat), 8, Color::TextDim(), 140), StatNameWidth, 0.f))
			->SetVerticalAlignment(VAlign_Center);
		const float Rating = LoadoutRules::StatRating(Stat, S);
		UHorizontalBoxSlot* BarSlot = Line->AddChildToHorizontalBox(StatBar(WidgetTree, Rating,
			B ? TOptional<float>(LoadoutRules::StatRating(Stat, *B)) : TOptional<float>()));
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Value = MakeText(WidgetTree, LoadoutRules::StatText(Stat, S), 11, Color::Text());
		Value->SetJustification(ETextJustify::Right);
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Value, StatValueWidth, 0.f))->SetVerticalAlignment(VAlign_Center);

		// Against the gun it would replace: green up for better, red down for worse, whichever way the number moved.
		const LoadoutRules::EChange Change = B ? LoadoutRules::CompareStat(Stat, S, *B) : LoadoutRules::EChange::Same;
		UWidget* Arrow = Change == LoadoutRules::EChange::Same ? static_cast<UWidget*>(WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass()))
			: ChangeArrow(WidgetTree, Change == LoadoutRules::EChange::Better, FVector2D(10.f, 8.f));
		// The arrow and change columns only take room while there's a gun to compare with.
		UHorizontalBoxSlot* ArrowSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Arrow, B ? StatArrowWidth : 0.f, 0.f));
		ArrowSlot->SetVerticalAlignment(VAlign_Center);
		ArrowSlot->SetPadding(FMargin(B ? 8.f : 0.f, 0.f, 0.f, 0.f));
		UTextBlock* Delta = MakeText(WidgetTree, B ? LoadoutRules::DeltaText(Stat, S, *B) : FString(), 9,
			Change == LoadoutRules::EChange::Better ? Color::Better() : Color::Worse());
		Delta->SetJustification(ETextJustify::Right);
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Delta, B ? StatDeltaWidth : 0.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		CardBox->AddChildToVerticalBox(MakeSized(WidgetTree, Line, 0.f, 22.f))->SetPadding(FMargin(0.f, 1.f));
	}
}

void ULoadoutWidget::AddCardBadges(const FWeaponInstanceData& Gun)
{
	// Its curse and its notches as two small tags; their story (the perk, the drawback, the tiers) waits behind Inspect.
	UHorizontalBox* Badges = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	if (const FWeaponCurse* Curse = WeaponCurses::Of(Gun))
	{
		const FLinearColor CoinTint(1.f, 1.f, 1.f, Gun.bCurseLifted ? 0.55f : 1.f);
		Badges->AddChildToHorizontalBox(Badge(WidgetTree, MakeImage(WidgetTree, CrackedCoinBrush(FVector2D(16.f, 16.f), CoinTint)),
			{ { Curse->Name.ToString(), Color::Curse() }, { Gun.bCurseLifted ? FString(TEXT("Lifted")) : FString(TEXT("Cursed")),
				Gun.bCurseLifted ? Color::Better() : Color::TextDim() } }, Color::Curse() * FLinearColor(1.f, 1.f, 1.f, 0.6f)));
	}
	if (Gun.Kills > 0)
	{
		const ENotchTier Tier = WeaponNotches::TierFor(Gun.Kills);
		TArray<TPair<FString, FLinearColor>> Words = { { FText::AsNumber(Gun.Kills).ToString(), Color::Text() } };
		if (Tier != ENotchTier::None)
		{
			Words.Add({ WeaponNotches::TierName(Tier).ToString(), NotchTierColor(Tier) });
		}
		UHorizontalBoxSlot* NotchSlot = Badges->AddChildToHorizontalBox(Badge(WidgetTree, MakeImage(WidgetTree, TallyBrush(FVector2D(16.f, 16.f),
			NotchTierColor(Tier))), Words, Color::Hairline() * FLinearColor(1.f, 1.f, 1.f, 0.4f)));
		NotchSlot->SetPadding(FMargin(Badges->GetChildrenCount() > 1 ? 8.f : 0.f, 0.f, 0.f, 0.f));
	}
	if (Badges->HasAnyChildren())
	{
		CardBox->AddChildToVerticalBox(Badges)->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
}

void ULoadoutWidget::AddCardInspect(const FWeaponInstanceData& Gun, const FWeaponInstanceData* Against)
{
	// How it fires and feeds.
	const UWeaponManagerComponent* Inventory = Manager.Get();
	const EAmmoType Ammo = Gun.Definition ? Gun.Definition->AmmoType : EAmmoType::AssaultRifle;
	CardBox->AddChildToVerticalBox(Label(WidgetTree, FString::Printf(TEXT("%s · %s · %d carried"), *LooterWeaponText::FireModeName(Gun), *AmmoName(Gun),
		Inventory ? Inventory->GetAmmo(Ammo) : 0), 9, Color::TextDim(), 140))->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));

	// Its parts, slot by slot, and what they add up to.
	if (Gun.Definition && !Gun.Definition->Parts.IsEmpty())
	{
		CardBox->AddChildToVerticalBox(Section(WidgetTree, TEXT("Parts")))->SetPadding(FMargin(0.f, 16.f, 0.f, 4.f));
		const FWeaponLook Look = WeaponParts::Pick(Gun);
		for (int32 Index = 0; Index < Gun.Definition->Parts.Num(); ++Index)
		{
			const FWeaponPartOption* Part = Look.Parts.IsValidIndex(Index) ? Look.Parts[Index] : nullptr;
			const FString PartName = !Part ? FString(TEXT("None")) : (Part->DisplayName.IsEmpty() ? Part->Key.ToString() : Part->DisplayName.ToString());
			UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Label(WidgetTree, Gun.Definition->Parts[Index].Name.ToString(), 8, Color::TextDim(), 140),
				StatNameWidth, 0.f))->SetVerticalAlignment(VAlign_Center);
			UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(FittedLabel(WidgetTree, PartName, 9, Part ? Color::Text() : Color::TextDim(), 60));
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			NameSlot->SetVerticalAlignment(VAlign_Center);
			CardBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.f, 2.f));
		}
		const TArray<LoadoutRules::FBonus> Bonuses = LoadoutRules::Bonuses(LoadoutRules::PartTotals(Gun));
		if (!Bonuses.IsEmpty())
		{
			CardBox->AddChildToVerticalBox(Section(WidgetTree, TEXT("Part bonuses")))->SetPadding(FMargin(0.f, 14.f, 0.f, 4.f));
			for (const LoadoutRules::FBonus& Bonus : Bonuses)
			{
				CardBox->AddChildToVerticalBox(Label(WidgetTree, LoadoutRules::BonusText(Bonus), 9, Bonus.Goodness > 0.f ? Color::Better() : Color::Worse(), 120))
					->SetPadding(FMargin(0.f, 2.f));
			}
		}
	}

	// Its notches and its curse in full: the count and tier, the curse's perk and its drawback.
	if (UWidget* Ideas = MakeGunIdeasRows(WidgetTree, Gun, 9))
	{
		CardBox->AddChildToVerticalBox(Section(WidgetTree, TEXT("History")))->SetPadding(FMargin(0.f, 16.f, 0.f, 6.f));
		CardBox->AddChildToVerticalBox(Ideas);
	}
}

void ULoadoutWidget::AddCardEmpty(const FString& Title, const FString& Words)
{
	CardBox->AddChildToVerticalBox(Label(WidgetTree, Title, 16, Color::TextDim(), 120));
	UTextBlock* Body = MakeText(WidgetTree, Words, 10, Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.8f));
	Body->SetFont(LooterUI::Font(10, /*bBold*/ false));
	Body->SetAutoWrapText(true);
	CardBox->AddChildToVerticalBox(Body)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
}

// ---------------------------------------------------------------------------
// The showcase
// ---------------------------------------------------------------------------

void ULoadoutWidget::RefreshShowcase()
{
	ALoadoutGunStage* StagePtr = Stage.Get();
	const FWeaponInstanceData* Gun = CursorItem();
	if (StagePtr)
	{
		if (Gun)
		{
			StagePtr->ShowGun(*Gun);
		}
		else
		{
			StagePtr->ClearGun();
		}
	}
	const bool bShown = StagePtr && StagePtr->HasGun() && StageMaterial;
	if (StageImage)
	{
		// Hidden, not collapsed: an empty showcase still takes a dragged gun (in hand).
		StageImage->SetRenderOpacity(bShown ? 1.f : 0.f);
	}
	if (ShowcaseHint)
	{
		ShowcaseHint->SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void ULoadoutWidget::ApplyMode()
{
	if (!StageSlot)
	{
		return;
	}
	// Inspect puts the list away and gives the gun the room; the card stays where it is.
	const FVector2D TopLeft = bInspecting ? LoadoutLayout::InspectShowcaseTopLeft : LoadoutLayout::ShowcaseTopLeft;
	const FVector2D Size = bInspecting ? LoadoutLayout::InspectShowcaseSize : LoadoutLayout::ShowcaseSize;
	StageSlot->SetPosition(TopLeft);
	StageSlot->SetSize(Size);
	ApplyStageBrush();
	if (ListColumn)
	{
		ListColumn->SetVisibility(bInspecting ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (ShowcaseHint && ShowcaseHintSlot)
	{
		ShowcaseHint->SetText(FText::FromString(FString(bInspecting ? TEXT("Drag to turn") : TEXT("Drag to turn · click to inspect")).ToUpper()));
		ShowcaseHintSlot->SetPosition(FVector2D(TopLeft.X + Size.X * 0.5f, TopLeft.Y + Size.Y + 6.f));
	}
}
