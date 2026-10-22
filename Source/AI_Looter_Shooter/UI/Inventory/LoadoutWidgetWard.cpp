// ULoadoutWidget: the WARD section (SHIELD, 2026-10-22): the ward worn and the spares carried as rows between the equip
// slots and the backpack, the card of the ward under the cursor (its numbers against the one worn, its trait), its Inked
// icon in the showcase, and what the keys do on its rows (wear a spare, stow, drop).

#include "UI/Inventory/LoadoutWidget.h"
#include "Audio/LooterSound.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
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
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;
using namespace LoadoutParts;

namespace
{
	/** A ward's row's button action (its index is the row: 0 the ward worn, then the spares). */
	const FName ActionWard(TEXT("Ward"));
	/** The ward box's icon on a row (the worn one's bigger), and in the showcase. */
	constexpr float WornIconSize = 40.f;
	constexpr float SpareIconSize = 28.f;
	constexpr float ShowcaseIconSize = 300.f;
	/** The card: its title as a gun's (LoadoutWidgetInspect.cpp), a stat row's name column and the bar's thickness. */
	constexpr int32 WardTitleSize = 19;
	constexpr int32 WardTitleMinSize = 14;
	constexpr float WardTitleWidth = LoadoutLayout::CardWidth - 44.f;
	constexpr float WardStatNameWidth = 92.f;
	constexpr float WardStatValueWidth = 62.f;
	constexpr float WardBarHeight = 5.f;

	FName WardIconName(const FShieldInstanceData* Ward)
	{
		return Ward && Ward->Definition ? Ward->Definition->IconName : FName(TEXT("WardBox"));
	}

	UWidget* WardIcon(UWidgetTree* Tree, const FShieldInstanceData* Ward, float Size, const FLinearColor& Tint = FLinearColor::White)
	{
		const FName Name = WardIconName(Ward);
		if (const FInkedIcon* Inked = FindInkedIcon(Name))
		{
			return MakeSized(Tree, MakeImage(Tree, InkedIconBrush(Name, *Inked, FVector2D(Size, Size), Tint)), Size, Size);
		}
		return MakeSized(Tree, Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass()), Size, Size);
	}

	/** A thin rule across the card. */
	UWidget* WardRule(UWidgetTree* Tree)
	{
		return MakeSized(Tree, MakeImage(Tree, RectBrush(Color::RowLine())), 0.f, 1.f);
	}

	/** A stat's bar: its rating in cyan, against another ward what it gains in green or loses in red (the guns' bars). */
	UWidget* WardBar(UWidgetTree* Tree, float Rating, TOptional<float> Before)
	{
		UHorizontalBox* Bar = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		auto Part = [Tree, Bar](float Weight, const FLinearColor& PartColor)
		{
			if (Weight < 0.005f)
			{
				return;
			}
			UHorizontalBoxSlot* PartSlot = Bar->AddChildToHorizontalBox(MakeSized(Tree, MakeImage(Tree, RectBrush(PartColor)), 0.f, WardBarHeight));
			FSlateChildSize Size(ESlateSizeRule::Fill);
			Size.Value = Weight;
			PartSlot->SetSize(Size);
			PartSlot->SetVerticalAlignment(VAlign_Center);
		};
		const float Now = FMath::Clamp(Rating, 0.f, 1.f);
		const float Then = FMath::Clamp(Before.Get(Now), 0.f, 1.f);
		Part(FMath::Min(Now, Then), Color::SegmentOn());
		Part(FMath::Abs(Now - Then), Now > Then ? Color::Better() : Color::Worse() * FLinearColor(1.f, 1.f, 1.f, 0.7f));
		Part(1.f - FMath::Max(Now, Then), Color::SegmentOff());
		return Bar;
	}

	/** How each of a ward's numbers rates on a card's bar (0-1): bigger, sooner and quicker read fuller. */
	float CapacityRating(const FShieldStats& S) { return FMath::Clamp(S.Capacity / 160.f, 0.f, 1.f); }
	float WaitRating(const FShieldStats& S) { return FMath::Clamp(1.f - (S.RechargeDelay - 1.f) / 4.f, 0.f, 1.f); }
	float FillRating(const FShieldStats& S) { return FMath::Clamp(1.f - (S.SecondsToFill() - 1.f) / 6.f, 0.f, 1.f); }
}

// ---------------------------------------------------------------------------
// The rows
// ---------------------------------------------------------------------------

UPlayerShieldComponent* ULoadoutWidget::GetShield() const
{
	const UWeaponManagerComponent* Inventory = Manager.Get();
	return Inventory ? UPlayerShieldComponent::Find(Inventory->GetOwner()) : nullptr;
}

const FShieldInstanceData* ULoadoutWidget::WardItem(int32 Row) const
{
	const UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return nullptr;
	}
	if (Row == 0)
	{
		return Shield->HasWard() ? &Shield->GetWard() : nullptr;
	}
	const TArray<FShieldInstanceData>& Spares = Shield->GetSpares();
	return Spares.IsValidIndex(Row - 1) ? &Spares[Row - 1] : nullptr;
}

void ULoadoutWidget::BuildWardSection(UVerticalBox* Column)
{
	if (!Column)
	{
		return;
	}
	// The header as the others are: WARD in orange, its count, a rule to the end.
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Line->AddChildToHorizontalBox(Label(WidgetTree, TEXT("Ward"), 10, Color::Accent(), 300))->SetVerticalAlignment(VAlign_Center);
	WardCount = Label(WidgetTree, TEXT(""), 10, Color::TextDim(), 200);
	UHorizontalBoxSlot* CountSlot = Line->AddChildToHorizontalBox(WardCount);
	CountSlot->SetVerticalAlignment(VAlign_Center);
	CountSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	UHorizontalBoxSlot* RuleSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::RowLine())), 0.f, 1.f));
	RuleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RuleSlot->SetVerticalAlignment(VAlign_Center);
	RuleSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	WardList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	WardHeader = Line;

	// Between the slots and the backpack (the column: Equipped header, slots, Backpack header, list, ammo).
	if (UVerticalBoxSlot* HeadSlot = Cast<UVerticalBoxSlot>(Column->InsertChildAt(2, Line)))
	{
		HeadSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
	if (UVerticalBoxSlot* ListSlot = Cast<UVerticalBoxSlot>(Column->InsertChildAt(3, WardList)))
	{
		ListSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

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
	const int32 NumRows = Shield ? 1 + Shield->GetSpares().Num() : 0;
	const ESlateVisibility Section = Shield ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	WardList->SetVisibility(Section);
	if (WardHeader)
	{
		WardHeader->SetVisibility(Section);
	}
	if (NumRows == 0 && Zone == EZone::Wards)
	{
		Zone = EZone::Slots;
		CursorIndex = TargetSlot;
	}
	for (int32 Row = 0; Row < NumRows; ++Row)
	{
		const FRow Made = MakeWardRow(Row);
		WardList->AddChildToVerticalBox(Made.Button)->SetPadding(FMargin(0.f, Row > 0 ? LoadoutLayout::RowGap : 0.f, 0.f, 0.f));
		WardRows.Add(Made);
	}
	if (WardCount)
	{
		WardCount->SetText(FText::FromString(FString::Printf(TEXT("%d / %d spare"), Shield ? Shield->GetSpares().Num() : 0,
			UPlayerShieldComponent::MaxSpares)));
	}
}

ULoadoutWidget::FRow ULoadoutWidget::MakeWardRow(int32 Row)
{
	const FShieldInstanceData* Ward = WardItem(Row);
	const bool bWorn = Row == 0;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const float IconSize = bWorn ? WornIconSize : SpareIconSize;
	Line->AddChildToHorizontalBox(WardIcon(WidgetTree, Ward, IconSize, Ward ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.35f)))
		->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* Words = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (Ward)
	{
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, ShieldText::Name(*Ward), bWorn ? 12 : 10, ShieldText::Color(*Ward), 50));
		const FString Trait = ShieldText::TraitName(Ward->Trait);
		const FString Under = Trait.IsEmpty() ? ShieldText::KindLine(*Ward) : FString::Printf(TEXT("%s · %s"), *ShieldText::KindLine(*Ward), *Trait);
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, Under, bWorn ? 8 : 7, Trait.IsEmpty() ? Color::TextDim() : Color::CyanText(), 140))
			->SetPadding(FMargin(0.f, bWorn ? 3.f : 1.f, 0.f, 0.f));
	}
	else
	{
		Words->AddChildToVerticalBox(Label(WidgetTree, TEXT("No ward worn"), 11, Color::TextDim(), 200));
		const UPlayerShieldComponent* Shield = GetShield();
		const bool bSpares = Shield && !Shield->GetSpares().IsEmpty();
		Words->AddChildToVerticalBox(FittedLabel(WidgetTree, bSpares ? TEXT("E: wear a spare") : TEXT("Wards drop like guns: hold Interact to wear one"),
			8, Color::TextDim() * FLinearColor(1.f, 1.f, 1.f, 0.7f), 120))->SetPadding(FMargin(0.f, 3.f, 0.f, 0.f));
	}
	UHorizontalBoxSlot* WordsSlot = Line->AddChildToHorizontalBox(Words);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(12.f, 0.f, 10.f, 0.f));

	if (Ward)
	{
		// The one number a ward is judged by at a glance: what it holds.
		UVerticalBox* Number = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UTextBlock* Value = MakeText(WidgetTree, ShieldText::CapacityString(Ward->Stats), bWorn ? 17 : 13, Color::Text());
		Value->SetJustification(ETextJustify::Right);
		Number->AddChildToVerticalBox(Value)->SetHorizontalAlignment(HAlign_Right);
		if (bWorn)
		{
			UTextBlock* Word = Label(WidgetTree, TEXT("Ward"), 7, Color::TextDim(), 200);
			Word->SetJustification(ETextJustify::Right);
			Number->AddChildToVerticalBox(Word)->SetHorizontalAlignment(HAlign_Right);
		}
		Line->AddChildToHorizontalBox(Number)->SetVerticalAlignment(VAlign_Center);
		if (!bWorn)
		{
			// Against the ward worn: green up, red down, or level.
			const UPlayerShieldComponent* Shield = GetShield();
			const int32 Verdict = ShieldRules::Compare(*Ward, Shield && Shield->HasWard() ? &Shield->GetWard() : nullptr);
			UWidget* Mark = Verdict == 0 ? static_cast<UWidget*>(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::TextDim())), 10.f, 2.f))
				: static_cast<UWidget*>(MakeImage(WidgetTree, IconBrush(Verdict > 0 ? TEXT("ArrowUp") : TEXT("ArrowDown"), ArrowIcon(Verdict > 0), 4.f, FVector2D(12.f, 10.f),
					Verdict > 0 ? Color::Better() : Color::Worse())));
			UHorizontalBoxSlot* MarkSlot = Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Mark, 18.f, 0.f));
			MarkSlot->SetVerticalAlignment(VAlign_Center);
			MarkSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		}
	}

	const FLinearColor Rarity = Ward ? ShieldText::Color(*Ward) : Color::TextDim();
	FRow Made = WrapRow(Line, Rarity, Ward != nullptr, bWorn ? LoadoutLayout::WardRowHeight : LoadoutLayout::BackpackRowHeight, ActionWard, Row);
	// Its own clicks and hovers (a ward isn't a gun the screen's drag can take), sounded by the action.
	Made.Button->bPlaysSounds = false;
	Made.Button->OnButtonClicked.BindUObject(this, &ULoadoutWidget::HandleWardClicked);
	Made.Button->OnButtonHovered.BindUObject(this, &ULoadoutWidget::HandleWardHovered);
	Made.Identity = Ward ? ShieldRules::Identity(*Ward) : 0;
	return Made;
}

void ULoadoutWidget::RestyleWards()
{
	for (int32 Row = 0; Row < WardRows.Num(); ++Row)
	{
		const FRow& Made = WardRows[Row];
		const bool bItem = WardItem(Row) != nullptr;
		const bool bCursor = Zone == EZone::Wards && CursorIndex == Row && !bItemDrag;
		FLinearColor Fill = bItem ? Colors::CardFill() : Colors::EmptyFill();
		FLinearColor Line = bItem ? Colors::CardLine() : Colors::CardLine() * FLinearColor(1.f, 1.f, 1.f, 0.5f);
		if (bCursor)
		{
			Fill = Color::Tile();
			Line = Color::Accent();
		}
		Made.Fill->SetColorAndOpacity(Fill);
		Made.Line->SetColorAndOpacity(Line);
		// The ward worn wears a WORN word on its corner, as a slot shows its gun in hand.
		if (Row == 0 && bItem)
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
	}
}

// ---------------------------------------------------------------------------
// The card and the showcase
// ---------------------------------------------------------------------------

void ULoadoutWidget::RefreshWardCard()
{
	if (!CardBox)
	{
		return;
	}
	CardBox->ClearChildren();
	const FShieldInstanceData* Ward = WardItem(CursorIndex);
	const uint32 Identity = Ward ? ShieldRules::Identity(*Ward) : static_cast<uint32>(0x3A4D0000 + CursorIndex);
	if (Identity != CardIdentity)
	{
		CardIdentity = Identity;
		CardIntroAge = 0.f;
		if (CardScroll)
		{
			CardScroll->ScrollToStart();
		}
	}
	const UPlayerShieldComponent* Shield = GetShield();
	if (!Ward)
	{
		CardRarity = Color::TextDim();
		bCardBreathes = false;
		CardStripe->SetColorAndOpacity(Colors::CardLine());
		CardLine->SetColorAndOpacity(Colors::CardLine());
		CardGlow->SetColorAndOpacity(FLinearColor::Transparent);
		AddCardEmpty(TEXT("No ward worn"), TEXT("A ward soaks hits before your health does, and fills again after a few seconds unhurt. Wards drop "
			"like guns: tap Interact on one to take it, hold to wear it."));
		return;
	}

	// The card wears the ward's rarity, as a gun's does.
	CardRarity = ShieldText::Color(*Ward);
	bCardBreathes = Ward->Rarity >= EWeaponRarity::Epic;
	CardStripe->SetColorAndOpacity(CardRarity);
	CardLine->SetColorAndOpacity(CardRarity * FLinearColor(1.f, 1.f, 1.f, 0.65f));
	CardGlow->SetColorAndOpacity(CardRarity * FLinearColor(1.f, 1.f, 1.f, 0.14f));

	const bool bWorn = CursorIndex == 0;
	const FShieldInstanceData* Against = !bWorn && Shield && Shield->HasWard() ? &Shield->GetWard() : nullptr;
	if (bWorn)
	{
		AddCardBanner(TEXT("Ward · worn"), Color::Accent());
	}
	else
	{
		const int32 Verdict = ShieldRules::Compare(*Ward, Against);
		AddCardBanner(!Against ? FString(TEXT("Spare · no ward worn")) : (Verdict > 0 ? FString(TEXT("Better than the ward worn"))
			: (Verdict < 0 ? FString(TEXT("Weaker than the ward worn")) : FString(TEXT("About the same as the ward worn")))),
			Verdict > 0 ? Color::Better() : (Verdict < 0 && Against ? Color::Worse() : Color::TextDim()), Against ? Verdict : 0);
	}

	UTextBlock* Name = Label(WidgetTree, ShieldText::Name(*Ward), WardTitleSize, CardRarity, 40);
	FitTextToWidth(Name, WardTitleWidth, WardTitleMinSize);
	Name->SetAutoWrapText(true);
	CardBox->AddChildToVerticalBox(Name)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	CardBox->AddChildToVerticalBox(Label(WidgetTree, ShieldText::KindLine(*Ward), 9, Color::TextDim(), 160))->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	CardBox->AddChildToVerticalBox(WardRule(WidgetTree))->SetPadding(FMargin(0.f, 12.f, 0.f, 10.f));

	// Its capacity, big: the number a ward is judged by, with its change beside it.
	const FShieldStats& S = Ward->Stats;
	{
		UHorizontalBox* Hero = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UVerticalBox* Number = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Number->AddChildToVerticalBox(Label(WidgetTree, TEXT("Ward"), 8, Color::TextDim(), 260));
		Number->AddChildToVerticalBox(MakeText(WidgetTree, ShieldText::CapacityString(S), 30, Color::Title()));
		Hero->AddChildToHorizontalBox(Number)->SetVerticalAlignment(VAlign_Bottom);
		if (Against)
		{
			const int32 Gain = FMath::RoundToInt32(S.Capacity - Against->Stats.Capacity);
			if (Gain != 0)
			{
				UHorizontalBoxSlot* DeltaSlot = Hero->AddChildToHorizontalBox(MakeText(WidgetTree, FString::Printf(TEXT("%+d"), Gain), 14,
					Gain > 0 ? Color::Better() : Color::Worse()));
				DeltaSlot->SetVerticalAlignment(VAlign_Bottom);
				DeltaSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 7.f));
			}
		}
		CardBox->AddChildToVerticalBox(Hero)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	// Its wait and its refill, with bars, against the ward worn.
	struct FWardStatRow
	{
		const TCHAR* Name;
		FString Value;
		float Rating;
		TOptional<float> Before;
	};
	const FShieldStats* B = Against ? &Against->Stats : nullptr;
	const FWardStatRow Rows[] = {
		{ TEXT("Holds"), ShieldText::CapacityString(S), CapacityRating(S), B ? TOptional<float>(CapacityRating(*B)) : TOptional<float>() },
		{ TEXT("Wait"), ShieldText::DelayString(S), WaitRating(S), B ? TOptional<float>(WaitRating(*B)) : TOptional<float>() },
		{ TEXT("Refill"), ShieldText::RateString(S), FillRating(S), B ? TOptional<float>(FillRating(*B)) : TOptional<float>() },
		{ TEXT("Full in"), FString::Printf(TEXT("%.1f s"), S.RechargeDelay + S.SecondsToFill()), FillRating(S), B ? TOptional<float>(FillRating(*B)) : TOptional<float>() },
	};
	for (const FWardStatRow& Stat : Rows)
	{
		UHorizontalBox* StatLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		StatLine->AddChildToHorizontalBox(MakeSized(WidgetTree, Label(WidgetTree, Stat.Name, 8, Color::TextDim(), 140), WardStatNameWidth, 0.f))
			->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* BarSlot = StatLine->AddChildToHorizontalBox(WardBar(WidgetTree, Stat.Rating, Stat.Before));
		BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Value = MakeText(WidgetTree, Stat.Value, 11, Color::Text());
		Value->SetJustification(ETextJustify::Right);
		StatLine->AddChildToHorizontalBox(MakeSized(WidgetTree, Value, WardStatValueWidth, 0.f))->SetVerticalAlignment(VAlign_Center);
		CardBox->AddChildToVerticalBox(MakeSized(WidgetTree, StatLine, 0.f, 22.f))->SetPadding(FMargin(0.f, 1.f));
	}

	// Its trait, in full (its element's colour for Last Breath).
	const FString Trait = ShieldText::TraitLine(*Ward);
	if (!Trait.IsEmpty())
	{
		CardBox->AddChildToVerticalBox(Label(WidgetTree, TEXT("Trait"), 8, Color::Accent(), 300))->SetPadding(FMargin(0.f, 14.f, 0.f, 4.f));
		UTextBlock* Line = MakeText(WidgetTree, Trait, 10, ShieldText::TraitColor(*Ward));
		Line->SetAutoWrapText(true);
		CardBox->AddChildToVerticalBox(Line);
	}
	// The worn ward as it stands now.
	if (bWorn && Shield)
	{
		const FString State = Shield->IsBroken() ? FString(TEXT("Broken: filling after its wait"))
			: (Shield->IsFull() ? FString(TEXT("Full")) : FString::Printf(TEXT("%d of %d"), FMath::CeilToInt32(Shield->GetCharge()),
				FMath::RoundToInt32(Shield->GetCapacity())));
		CardBox->AddChildToVerticalBox(Label(WidgetTree, State, 9, Color::TextDim(), 140))->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}
}

void ULoadoutWidget::RefreshWardShowcase()
{
	if (!WardShowcase)
	{
		return;
	}
	const FShieldInstanceData* Ward = Zone == EZone::Wards ? WardItem(CursorIndex) : nullptr;
	const FInkedIcon* Inked = Ward ? FindInkedIcon(WardIconName(Ward)) : nullptr;
	if (!Inked || bInspecting)
	{
		WardShowcase->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	WardShowcase->SetBrush(InkedIconBrush(WardIconName(Ward), *Inked, FVector2D(ShowcaseIconSize, ShowcaseIconSize)));
	WardShowcase->SetVisibility(ESlateVisibility::HitTestInvisible);
}

// ---------------------------------------------------------------------------
// The keys and the mouse on a ward's row
// ---------------------------------------------------------------------------

TArray<ULoadoutWidget::FPrompt> ULoadoutWidget::WardPrompts() const
{
	TArray<FPrompt> Prompts;
	const UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return Prompts;
	}
	const bool bSpares = !Shield->GetSpares().IsEmpty();
	if (CursorIndex == 0)
	{
		if (bSpares)
		{
			Prompts.Add({ TEXT("E"), Shield->HasWard() ? TEXT("Choose a spare") : TEXT("Wear a spare"), EAction::Activate });
		}
		if (Shield->HasWard())
		{
			if (Shield->HasSpareRoom())
			{
				Prompts.Add({ TEXT("C"), TEXT("Stow"), EAction::Stow });
			}
			Prompts.Add({ TEXT("Q"), TEXT("Drop"), EAction::Drop });
		}
	}
	else if (WardItem(CursorIndex))
	{
		Prompts.Add({ TEXT("E"), TEXT("Wear"), EAction::Activate });
		Prompts.Add({ TEXT("Q"), TEXT("Drop"), EAction::Drop });
	}
	return Prompts;
}

bool ULoadoutWidget::RunWardAction(EAction Action)
{
	switch (Action)
	{
	case EAction::Activate:
	case EAction::Hold:
		WardActivate();
		return true;
	case EAction::Stow:
		WardStow();
		return true;
	case EAction::Drop:
		WardDrop();
		return true;
	case EAction::Move:
	case EAction::Inspect:
		// A ward isn't moved between slots or turned on the stand.
		PlayCue(LooterSoundCue::Denied);
		return true;
	default:
		return false;
	}
}

void ULoadoutWidget::WardActivate()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return;
	}
	if (CursorIndex == 0)
	{
		// The ward worn: off to the spares to choose its replacement (or with none worn, the first spare goes on).
		if (Shield->GetSpares().IsEmpty())
		{
			PlayCue(LooterSoundCue::Denied);
			return;
		}
		if (!Shield->HasWard())
		{
			bool bWorn = false;
			{
				TGuardValue<bool> RefreshAfter(bApplyingAction, true);
				bWorn = Shield->EquipSpare(0);
			}
			Refresh();
			FlashRow(EZone::Wards, 0);
			PlayCue(bWorn ? Sounds::Equip : LooterSoundCue::Denied);
			return;
		}
		PlayCue(LooterSoundCue::Click);
		MoveCursorTo(EZone::Wards, 1, false, false);
		return;
	}
	bool bWorn = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bWorn = Shield->EquipSpare(CursorIndex - 1);
	}
	if (bWorn)
	{
		// The cursor follows the ward to the worn row: its card greets it there.
		CursorIndex = 0;
	}
	Refresh();
	if (bWorn)
	{
		FlashRow(EZone::Wards, 0);
	}
	PlayCue(bWorn ? Sounds::Equip : LooterSoundCue::Denied);
}

void ULoadoutWidget::WardStow()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (!Shield || CursorIndex != 0)
	{
		return;
	}
	bool bStowed = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bStowed = Shield->StowWard();
	}
	Refresh();
	PlayCue(bStowed ? Sounds::Stow : LooterSoundCue::Denied);
}

void ULoadoutWidget::WardDrop()
{
	UPlayerShieldComponent* Shield = GetShield();
	if (!Shield)
	{
		return;
	}
	bool bDropped = false;
	{
		TGuardValue<bool> RefreshAfter(bApplyingAction, true);
		bDropped = CursorIndex == 0 ? Shield->DropWard() : Shield->DropSpare(CursorIndex - 1);
	}
	Refresh();
	PlayCue(bDropped ? Sounds::Drop : LooterSoundCue::Denied);
}

void ULoadoutWidget::HandleWardHovered(ULooterButton* Button)
{
	if (Button && !bInspecting)
	{
		MoveCursorTo(EZone::Wards, Button->Index, false, false);
	}
}

void ULoadoutWidget::HandleWardClicked(ULooterButton* Button)
{
	HandleWardHovered(Button);
	WardActivate();
	// Clicking handed keyboard focus to the game viewport (LooterButton); take it back for the screen's keys.
	SetKeyboardFocus();
}
