#include "UI/World/WeaponLabelWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
#include "Affixes/WeaponMakers.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"

using namespace LooterUI;

namespace
{
	/**
	 * A folded label's dot (widget units): the lit core and the dark halo round it. The playtest (2026-10-27) lost the old
	 * 12-unit dot in a legendary beam's glow: a core this big with a light rim, in a dark halo, reads over bright and dark
	 * ground alike, and a pile's dots still stay a scatter.
	 */
	constexpr float FoldedCoreSize = 18.f;
	constexpr float FoldedHaloSize = 26.f;
	/** The core's light rim (widget units), a little thicker than the kit's hairlines so it shows at the label range's far end. */
	constexpr float FoldedRimWidth = 2.f;
}

TSharedRef<SWidget> UWeaponLabelWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The card (plate and lines) and, in its place when folded, the dot: collapsing the card leaves the dot's size alone,
		// so a folded label takes no more room than its dot.
		UOverlay* Card = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Card"));
		UOverlaySlot* CardSlot = Root->AddChildToOverlay(Card);
		CardSlot->SetHorizontalAlignment(HAlign_Fill);
		CardSlot->SetVerticalAlignment(VAlign_Fill);
		Lines = Card;
		UOverlay* Dot = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("FoldedDot"));
		FoldedDot = Dot;
		UOverlaySlot* DotSlot = Root->AddChildToOverlay(Dot);
		DotSlot->SetHorizontalAlignment(HAlign_Center);
		DotSlot->SetVerticalAlignment(VAlign_Bottom);
		FoldedHalo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("FoldedHalo"));
		UOverlaySlot* HaloSlot = Dot->AddChildToOverlay(FoldedHalo);
		HaloSlot->SetHorizontalAlignment(HAlign_Center);
		HaloSlot->SetVerticalAlignment(VAlign_Center);
		FoldedCore = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("FoldedCore"));
		UOverlaySlot* CoreSlot = Dot->AddChildToOverlay(FoldedCore);
		CoreSlot->SetHorizontalAlignment(HAlign_Center);
		CoreSlot->SetVerticalAlignment(VAlign_Center);

		auto AddFill = [Card](UWidget* Child, const FMargin& ChildPadding)
		{
			UOverlaySlot* ChildSlot = Card->AddChildToOverlay(Child);
			ChildSlot->SetHorizontalAlignment(HAlign_Fill);
			ChildSlot->SetVerticalAlignment(VAlign_Fill);
			ChildSlot->SetPadding(ChildPadding);
		};

		PlateFill = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		PlateFill->SetBrush(ShapeBrush(EShape::Control, false, FLinearColor::FromSRGBColor(FColor(7, 26, 40, 225))));
		MarkBackground(PlateFill);
		AddFill(PlateFill, FMargin(0.f));
		PlateLine = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		PlateLine->SetBrush(ShapeBrush(EShape::Control, true, Color::ScreenLine()));
		AddFill(PlateLine, FMargin(0.f));

		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		auto AddLine = [Box](UWidget* Line)
		{
			Box->AddChildToVerticalBox(Line)->SetHorizontalAlignment(HAlign_Center);
		};

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Name"));
		FlavorText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Flavor"));
		TraitText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Trait"));

		// The maker's line: its mark and name, over the gun's name. ApplyContent sizes the mark and shows the row only while
		// the label is looked at, for a gun with a maker.
		UHorizontalBox* MakerLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MakerLine"));
		MakerGlyph = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MakerGlyph"));
		MakerLine->AddChildToHorizontalBox(MakerGlyph)->SetVerticalAlignment(VAlign_Center);
		MakerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Maker"));
		UHorizontalBoxSlot* MakerTextSlot = MakerLine->AddChildToHorizontalBox(MakerText);
		MakerTextSlot->SetVerticalAlignment(VAlign_Center);
		MakerTextSlot->SetPadding(FMargin(5.f, 0.f, 0.f, 0.f));
		MakerRow = MakerLine;
		StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Stats"));
		PromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Prompt"));

		// A cursed iron's line: the cracked coin and the curse's name, in the curse's own brass, so the name above keeps the
		// rarity's color. ApplyContent sizes the coin and shows the row only for a cursed gun.
		UHorizontalBox* CurseLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CurseLine"));
		CurseGlyph = MakeImage(WidgetTree, CrackedCoinBrush(FVector2D(16.f, 16.f)));
		CurseLine->AddChildToHorizontalBox(CurseGlyph)->SetVerticalAlignment(VAlign_Center);
		CurseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Curse"));
		UHorizontalBoxSlot* CurseTextSlot = CurseLine->AddChildToHorizontalBox(CurseText);
		CurseTextSlot->SetVerticalAlignment(VAlign_Center);
		CurseTextSlot->SetPadding(FMargin(5.f, 0.f, 0.f, 0.f));
		CurseRow = CurseLine;

		AddLine(MakerRow);
		AddLine(NameText);
		AddLine(CurseRow);
		AddLine(TraitText);
		AddLine(FlavorText);
		AddLine(StatsText);
		AddLine(PromptText);
		AddFill(Box, FMargin(12.f, 6.f));

		ApplyContent();
	}
	return Super::RebuildWidget();
}

void UWeaponLabelWidget::SetWeapon(const AWeaponBase* Weapon)
{
	if (!Weapon)
	{
		return;
	}

	const FWeaponInstanceData& Instance = Weapon->GetInstance();
	const FWeaponStats& S = Instance.Stats;
	Name = FText::FromString(LooterWeaponText::Name(Instance).ToUpper());
	// The line in its own words, as written: it's said, not a label.
	Flavor = FText::FromString(LooterWeaponText::FlavorLine(Instance));
	// A cursed iron's curse ("HUNGRY"), empty for any other gun.
	Curse = FText::FromString(LooterWeaponText::CurseString(Instance).ToUpper());
	Stats = FText::FromString(FString::Printf(TEXT("LV %d   %s DMG   %.0f RPM   %d MAG"),
		Instance.Level, *LooterWeaponText::DamageString(S), S.FireRate, S.MagazineSize));
	NameColor = LooterWeaponText::Color(Instance);
	// Its element and effect ("EMBER · RICOCHET"), in the element's colour: what makes it worth walking over to.
	Trait = FText::FromString(LooterWeaponText::TraitLine(Instance));
	TraitColor = LooterWeaponText::TraitColor(Instance);
	// Its maker ("MERIDIAN") and mark: who made it, so its name and feel promise something.
	Maker = FText::FromString(LooterWeaponText::MakerString(Instance));
	const FWeaponMaker* Made = WeaponMakers::Of(Instance);
	MakerIcon = Made && LooterWeaponText::MakerMark(Instance) ? Made->MarkIcon : NAME_None;
	ApplyContent();
}

FText UWeaponLabelWidget::GetMakerText() const
{
	return MakerText ? MakerText->GetText() : FText::GetEmpty();
}

bool UWeaponLabelWidget::IsMakerShown() const
{
	const ESlateVisibility Shown = MakerRow ? MakerRow->GetVisibility() : ESlateVisibility::Collapsed;
	return Shown != ESlateVisibility::Collapsed && Shown != ESlateVisibility::Hidden && !MakerText->GetText().IsEmpty();
}

FText UWeaponLabelWidget::GetTraitText() const
{
	return TraitText ? TraitText->GetText() : FText::GetEmpty();
}

float UWeaponLabelWidget::PopScaleAt(float Age)
{
	// Up from small past its size in a blink, then settling: 0.3 -> 1.25 over 0.12 s, back to 1 by 0.32 s.
	constexpr float RiseSeconds = 0.12f;
	constexpr float SettleSeconds = 0.2f;
	if (Age <= RiseSeconds)
	{
		return FMath::Lerp(0.3f, 1.25f, FMath::Clamp(Age / RiseSeconds, 0.f, 1.f));
	}
	return FMath::Lerp(1.25f, 1.f, FMath::SmoothStep(0.f, 1.f, FMath::Clamp((Age - RiseSeconds) / SettleSeconds, 0.f, 1.f)));
}

float UWeaponLabelWidget::PopOpacityAt(float Age, float Seconds)
{
	// Full until its last 0.8 s, then fading out.
	constexpr float FadeSeconds = 0.8f;
	return FMath::Clamp((Seconds - Age) / FadeSeconds, 0.f, 1.f);
}

void UWeaponLabelWidget::SetPop(float Age, float Seconds)
{
	if (!bPop)
	{
		bPop = true;
		bIsFocused = false;
		ApplyContent();
	}
	SetRenderScale(FVector2D(PopScaleAt(Age)));
	SetRenderOpacity(PopOpacityAt(Age, Seconds));
}

FText UWeaponLabelWidget::GetNameText() const
{
	return NameText ? NameText->GetText() : FText::GetEmpty();
}

FText UWeaponLabelWidget::GetFlavorText() const
{
	return FlavorText ? FlavorText->GetText() : FText::GetEmpty();
}

bool UWeaponLabelWidget::IsFlavorShown() const
{
	const ESlateVisibility Shown = FlavorText ? FlavorText->GetVisibility() : ESlateVisibility::Collapsed;
	return Shown != ESlateVisibility::Collapsed && Shown != ESlateVisibility::Hidden && !FlavorText->GetText().IsEmpty();
}

FText UWeaponLabelWidget::GetCurseText() const
{
	return CurseText ? CurseText->GetText() : FText::GetEmpty();
}

bool UWeaponLabelWidget::IsCurseShown() const
{
	const ESlateVisibility Shown = CurseRow ? CurseRow->GetVisibility() : ESlateVisibility::Collapsed;
	return Shown != ESlateVisibility::Collapsed && Shown != ESlateVisibility::Hidden && !CurseText->GetText().IsEmpty();
}

void UWeaponLabelWidget::SetFocused(bool bFocused)
{
	if (bIsFocused != bFocused)
	{
		bIsFocused = bFocused;
		ApplyContent();
	}
}

void UWeaponLabelWidget::SetFolded(bool bInFolded)
{
	if (bFolded != bInFolded)
	{
		bFolded = bInFolded;
		ApplyContent();
	}
}

bool UWeaponLabelWidget::IsDotShown() const
{
	return FoldedDot && FoldedDot->GetVisibility() == ESlateVisibility::HitTestInvisible;
}

void UWeaponLabelWidget::ApplyContent()
{
	if (!NameText)
	{
		return;
	}

	// Unfocused: just the outlined name floating over the loot. Focused: a full plate with stats and the prompt.
	const ESlateVisibility Detail = bIsFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	PlateFill->SetVisibility(bIsFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	PlateLine->SetVisibility(bIsFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	NameText->SetText(Name);
	StyleFloatingText(NameText, bPop ? 24 : (bIsFocused ? 17 : 14), NameColor, bPop ? 120 : 80);

	// Its element and effect, from afar as up close (a pop's bigger): the reason to walk over.
	TraitText->SetText(Trait);
	StyleFloatingText(TraitText, bPop ? 15 : (bIsFocused ? 13 : 11), TraitColor, 120);
	TraitText->SetVisibility(Trait.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	// A cursed iron: the coin and the curse's name under the name, from afar as up close (it is the warning), the coin
	// a size up when the label opens.
	CurseText->SetText(Curse);
	StyleFloatingText(CurseText, bIsFocused ? 13 : 12, Color::Curse(), 80);
	const float CoinSize = bIsFocused ? 19.f : 16.f;
	CurseGlyph->SetBrush(CrackedCoinBrush(FVector2D(CoinSize, CoinSize)));
	CurseRow->SetVisibility(Curse.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	// Its maker over its name, with the details (from afar the name alone, which a gun no part names already takes from
	// its maker): the mark white as every Inked icon, the name in dim text so the gun's own name leads.
	MakerText->SetText(Maker);
	StyleFloatingText(MakerText, 11, LooterWeaponText::MakerColor(), 160);
	const LooterUI::FInkedIcon* MarkIcon = LooterUI::FindInkedIcon(MakerIcon);
	MakerGlyph->SetVisibility(MarkIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (MarkIcon)
	{
		MakerGlyph->SetBrush(InkedIconBrush(MakerIcon, *MarkIcon, FVector2D(17.f, 17.f)));
	}
	MakerRow->SetVisibility(bIsFocused && !Maker.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	// A named gun's line under its name, with the details: the name alone floats over the loot from afar.
	FlavorText->SetText(Flavor);
	StyleFloatingText(FlavorText, 12, LooterWeaponText::FlavorColor(), 20);
	FlavorText->SetVisibility(bIsFocused && !Flavor.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	StatsText->SetText(Stats);
	StyleFloatingText(StatsText, 11, Color::TextDim(), 60);
	StatsText->SetVisibility(Detail);

	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	const FString Key = Bindings ? Bindings->GetKey(TEXT("Interact")).GetDisplayName().ToString().ToUpper() : FString(TEXT("E"));
	PromptText->SetText(FText::FromString(FString::Printf(TEXT("[%s] PICK UP  |  HOLD EQUIP"), *Key)));
	StyleFloatingText(PromptText, 13, Color::Accent(), 150);
	PromptText->SetVisibility(Detail);

	// A drop's name pop: the name and its line alone (no curse row: the gun's own label carries the warning).
	if (bPop)
	{
		CurseRow->SetVisibility(ESlateVisibility::Collapsed);
		FlavorText->SetVisibility(ESlateVisibility::Collapsed);
		MakerRow->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Folded (one of a pile): its rarity's dot where the name would hang, the card gone. Looked at or popping, never.
	const bool bShowDot = bFolded && !bIsFocused && !bPop;
	Lines->SetVisibility(bShowDot ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	FoldedDot->SetVisibility(bShowDot ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bShowDot)
	{
		// A dark halo (the kit's outline colour) under a core in the rarity's colour with a light rim: the dark reads in a
		// legendary beam's white glow, the light over dark ground, and the colour tells the rarity from either.
		FSlateBrush Halo = CircleBrush(Color::Outline());
		Halo.ImageSize = FVector2D(FoldedHaloSize, FoldedHaloSize);
		FoldedHalo->SetBrush(Halo);
		FSlateBrush Core = CircleBrush(NameColor, Color::Text(), FoldedRimWidth);
		Core.ImageSize = FVector2D(FoldedCoreSize, FoldedCoreSize);
		FoldedCore->SetBrush(Core);
	}
	ShownDotSize = bShowDot ? FVector2D(FoldedHaloSize, FoldedHaloSize) : FVector2D::ZeroVector;
}
