// ULoadoutWidget: the card of the ward under the cursor (its numbers against the one worn, its trait, the worn one's state)
// on the plain card in the middle, and its Inked icon in the showcase where a gun would turn. The ward rows and keys are
// LoadoutWidgetWard.cpp.

#include "UI/Inventory/LoadoutWidget.h"
#include "Shields/PlayerShieldComponent.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
#include "UI/Inventory/LoadoutParts.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
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
	/** The ward box's icon in the showcase (it fits the showcase's height). */
	constexpr float ShowcaseIconSize = 240.f;
	/** The card: its title as a gun's on Inspect, a stat row's name column and the bar's thickness. */
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
		AddCardEmpty(TEXT("No ward worn"), TEXT("A ward soaks hits before your health does, and fills again after a few seconds unhurt. Wards drop "
			"like guns: tap Interact on one to take it, hold to wear it."));
		return;
	}

	// Its rarity colours the showcase ring, as a gun's does.
	CardRarity = ShieldText::Color(*Ward);
	bCardBreathes = Ward->Rarity >= EWeaponRarity::Epic;

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

