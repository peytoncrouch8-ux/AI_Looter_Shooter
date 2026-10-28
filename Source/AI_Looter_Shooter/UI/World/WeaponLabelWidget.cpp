#include "UI/World/WeaponLabelWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/Style/WeaponText.h"
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
#include "HAL/PlatformTime.h"

// The label from afar (name, trait line, curse, the folded dot, the drop's name pop) and which of its looks shows. The
// looked-at card over the gun is WeaponLabelWidgetCard.cpp.

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
	/**
	 * A look back at the same gun this soon (seconds) after looking away plays no intro again: the focus drops for a frame
	 * when a body crosses the line of sight, and the card mustn't unfold and tick each time.
	 */
	constexpr double RefocusQuietSeconds = 0.3;

	bool IsOnScreen(const UWidget* Widget)
	{
		const ESlateVisibility Shown = Widget ? Widget->GetVisibility() : ESlateVisibility::Collapsed;
		return Shown != ESlateVisibility::Collapsed && Shown != ESlateVisibility::Hidden;
	}
}

TSharedRef<SWidget> UWeaponLabelWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// Three looks, one showing at a time: the lines from afar, the dot when folded, the card when looked at. Each
		// collapses whole, so a folded label takes no more room than its dot and a far one no more than its lines.
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Lines"));
		UOverlaySlot* LinesSlot = Root->AddChildToOverlay(Box);
		LinesSlot->SetHorizontalAlignment(HAlign_Fill);
		LinesSlot->SetVerticalAlignment(VAlign_Fill);
		LinesSlot->SetPadding(FMargin(12.f, 6.f));
		Lines = Box;

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

		// The card stands on its tether's diamond, at the label's foot where the name hung (the component's pivot is 0.5, 1).
		UOverlaySlot* StackSlot = Root->AddChildToOverlay(BuildFocusStack());
		StackSlot->SetHorizontalAlignment(HAlign_Center);
		StackSlot->SetVerticalAlignment(VAlign_Bottom);

		auto AddLine = [Box](UWidget* Line)
		{
			Box->AddChildToVerticalBox(Line)->SetHorizontalAlignment(HAlign_Center);
		};

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Name"));
		TraitText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Trait"));

		// A cursed iron's line: the cracked coin and the curse's name, in the curse's own brass, so the name above keeps the
		// rarity's color. ApplyContent shows the row only for a cursed gun.
		UHorizontalBox* CurseLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CurseLine"));
		CurseGlyph = MakeImage(WidgetTree, CrackedCoinBrush(FVector2D(16.f, 16.f)));
		CurseLine->AddChildToHorizontalBox(CurseGlyph)->SetVerticalAlignment(VAlign_Center);
		CurseText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Curse"));
		UHorizontalBoxSlot* CurseTextSlot = CurseLine->AddChildToHorizontalBox(CurseText);
		CurseTextSlot->SetVerticalAlignment(VAlign_Center);
		CurseTextSlot->SetPadding(FMargin(5.f, 0.f, 0.f, 0.f));
		CurseRow = CurseLine;

		AddLine(NameText);
		AddLine(CurseRow);
		AddLine(TraitText);

		ApplyContent();
	}
	return Super::RebuildWidget();
}

void UWeaponLabelWidget::SetWeapon(const AWeaponBase* InWeapon)
{
	if (!InWeapon)
	{
		return;
	}

	const FWeaponInstanceData& Instance = InWeapon->GetInstance();
	Gun = Instance;
	Weapon = InWeapon;
	bHasGun = true;
	bCardStale = true;
	Name = FText::FromString(LooterWeaponText::Name(Instance).ToUpper());
	// The line in its own words, as written: it's said, not a label.
	Flavor = FText::FromString(LooterWeaponText::FlavorLine(Instance));
	// A cursed iron's curse ("HUNGRY"), empty for any other gun.
	Curse = FText::FromString(LooterWeaponText::CurseString(Instance).ToUpper());
	NameColor = LooterWeaponText::Color(Instance);
	// Its element and effect ("EMBER · RICOCHET"), in the element's colour: what makes it worth walking over to.
	Trait = FText::FromString(LooterWeaponText::TraitLine(Instance));
	TraitColor = LooterWeaponText::TraitColor(Instance);
	// Its maker ("MERIDIAN"): the card's sweep carries it while looked at.
	Maker = FText::FromString(LooterWeaponText::MakerString(Instance));
	ApplyContent();
	if (bIsFocused)
	{
		UpdateCardFromViewer();
	}
}

FText UWeaponLabelWidget::GetMakerText() const
{
	return Maker;
}

bool UWeaponLabelWidget::IsMakerShown() const
{
	return IsCardShown() && !Maker.IsEmpty();
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
	return IsOnScreen(FocusStack) && IsOnScreen(FlavorText) && !FlavorText->GetText().IsEmpty();
}

FText UWeaponLabelWidget::GetCurseText() const
{
	return CurseText ? CurseText->GetText() : FText::GetEmpty();
}

bool UWeaponLabelWidget::IsCurseShown() const
{
	return IsOnScreen(Lines) && IsOnScreen(CurseRow) && !CurseText->GetText().IsEmpty();
}

void UWeaponLabelWidget::SetFocused(bool bFocused)
{
	if (bIsFocused == bFocused)
	{
		return;
	}
	bIsFocused = bFocused;
	// A gun newly looked at unfolds its card with its sound; a look back within a blink just shows it again.
	const double Now = FPlatformTime::Seconds();
	if (bFocused)
	{
		bIntroPending = Now - LostFocusAt > RefocusQuietSeconds;
	}
	else
	{
		LostFocusAt = Now;
	}
	ApplyContent();
	if (bFocused)
	{
		UpdateCardFromViewer();
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

	// From afar: the outlined name floating over the loot (bigger as a drop's pop).
	NameText->SetText(Name);
	StyleFloatingText(NameText, bPop ? 24 : 14, NameColor, bPop ? 120 : 80);

	// Its element and effect, from afar (a pop's bigger): the reason to walk over.
	TraitText->SetText(Trait);
	StyleFloatingText(TraitText, bPop ? 15 : 11, TraitColor, 120);
	TraitText->SetVisibility(Trait.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	// A cursed iron: the coin and the curse's name under the name, from afar (it is the warning; up close the card's
	// special line carries it). A drop's name pop leaves it to the gun's own label.
	CurseText->SetText(Curse);
	StyleFloatingText(CurseText, 12, Color::Curse(), 80);
	CurseRow->SetVisibility(Curse.IsEmpty() || bPop ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	// Looked at: the card over the gun in place of the lines. A named gun's line floats over the card, as written.
	const bool bShowCard = bIsFocused && !bPop && bHasGun;
	FocusStack->SetVisibility(bShowCard ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	FlavorText->SetText(Flavor);
	StyleFloatingText(FlavorText, 12, LooterWeaponText::FlavorColor(), 20);
	FlavorText->SetVisibility(bShowCard && !Flavor.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	TetherDiamond->SetColorAndOpacity(NameColor);

	// Folded (one of a pile): its rarity's dot where the name would hang. Looked at or popping, never.
	const bool bShowDot = bFolded && !bIsFocused && !bPop;
	Lines->SetVisibility(bShowDot || bShowCard ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
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
