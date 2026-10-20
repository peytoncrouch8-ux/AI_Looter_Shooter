// The boss bar's armour bar (Main's call for the Gravemother's headstone, 2026-10-20): a thin slanted granite bar right under
// the boss's own, as long as its track, ink-rimmed over a dark track (a background: it fades with the UI transparency), its
// fill lit along its top and shaded along its foot, a hit's chip trailing it and a flash toward white; the phase line steps
// down under it while it shows. Made the first time a boss asks for it; it fades out when the armour breaks.

#include "UI/HUD/HudBossBarWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/** Where the boss bar's pieces are (as HudBossBarWidgetLayout.cpp lays them out). */
	constexpr float ArmourClusterSide = 60.f;
	constexpr float ArmourBossBarTop = 36.f;
	constexpr float ArmourBossBarHeight = 30.f;
	constexpr float ArmourTrackInset = 6.f;
	constexpr float ArmourSlant = -16.f;
	constexpr float ArmourPhaseGap = 4.f;
	/** The armour's bar: 3 px under the boss's, 8 tall, as long as its track; its ink rim 1.5 px. */
	constexpr float ArmourGap = 3.f;
	constexpr float ArmourHeight = 8.f;
	constexpr float ArmourWidth = UHudBossBarWidget::BarWidth - ArmourTrackInset * 2.f;
	constexpr float ArmourRim = 1.5f;
	constexpr float ArmourInnerWidth = ArmourWidth - ArmourRim * 2.f;
	/** Its fill's lit top line and shaded foot (px). */
	constexpr float ArmourTopLine = 2.f;
	constexpr float ArmourFoot = 1.5f;
	/** The phase line steps down this far while it shows. */
	constexpr float ArmourPhaseDrop = ArmourGap + ArmourHeight;
	/** It fades in or out over this long (s); a hit's chip holds, then drains (a share of the bar per second), as the boss's. */
	constexpr float ArmourFadeSeconds = 0.3f;
	constexpr float ArmourChipHoldSeconds = 0.45f;
	constexpr float ArmourChipDrain = 0.6f;
	/** A hit's flash toward white on its fill, and how fast it fades (1/s). */
	constexpr float ArmourFlashWhite = 0.55f;
	constexpr float ArmourFlashFade = 5.f;
	constexpr float ArmourTrackOpacity = 0.88f;

	USizeBox* AddLength(UWidgetTree* Tree, UOverlay* Track, UWidget* Content)
	{
		USizeBox* Length = MakeSized(Tree, Content, 0.f, ArmourHeight - ArmourRim * 2.f);
		Length->SetWidthOverride(0.f);
		UOverlaySlot* LengthSlot = Track->AddChildToOverlay(Length);
		LengthSlot->SetHorizontalAlignment(HAlign_Left);
		LengthSlot->SetVerticalAlignment(VAlign_Fill);
		return Length;
	}
}

void UHudBossBarWidget::SetArmour(bool bShown, float InFraction)
{
	if (!bShown)
	{
		bArmourWanted = false;
		return;
	}
	BuildArmour();
	bArmourWanted = true;
	const float NewFraction = FMath::Clamp(InFraction, 0.f, 1.f);
	if (FMath::IsNearlyEqual(NewFraction, ArmourFraction, 0.0005f))
	{
		return;
	}
	if (NewFraction < ArmourFraction)
	{
		// A hit: the lost part lingers as a chip, and the fill flashes.
		ArmourChipHold = ArmourChipHoldSeconds;
		ArmourFlash = 1.f;
	}
	ArmourFraction = NewFraction;
	ArmourChipFraction = FMath::Max(ArmourChipFraction, ArmourFraction);
	ApplyArmour();
}

void UHudBossBarWidget::BuildArmour()
{
	UCanvasPanel* Box = Cast<UCanvasPanel>(Cluster);
	if (ArmourBar || !Box || !WidgetTree)
	{
		return;
	}
	// The ink rim, and inside it the dark track (a background), the chip and the fill cut to length from the left.
	UOverlay* Rimmed = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	FillOverlaySlot(Rimmed->AddChildToOverlay(MakeImage(WidgetTree, RectBrush(Color::Ink()))));
	UOverlay* Track = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	FillOverlaySlot(Rimmed->AddChildToOverlay(Track), FMargin(ArmourRim));
	UImage* Glass = MakeImage(WidgetTree, RectBrush(Color::Track().CopyWithNewOpacity(ArmourTrackOpacity)));
	MarkBackground(Glass);
	FillOverlaySlot(Track->AddChildToOverlay(Glass));
	ArmourChipBox = AddLength(WidgetTree, Track, MakeImage(WidgetTree, RectBrush(Color::GraniteLight().CopyWithNewOpacity(0.6f))));

	UOverlay* Fill = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	ArmourFillImage = MakeImage(WidgetTree, RectBrush(FLinearColor::White));
	FillOverlaySlot(Fill->AddChildToOverlay(ArmourFillImage));
	UOverlaySlot* TopSlot = Fill->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::GraniteLight())), 0.f, ArmourTopLine));
	TopSlot->SetHorizontalAlignment(HAlign_Fill);
	TopSlot->SetVerticalAlignment(VAlign_Top);
	UOverlaySlot* FootSlot = Fill->AddChildToOverlay(MakeSized(WidgetTree, MakeImage(WidgetTree, RectBrush(Color::GraniteDark())), 0.f, ArmourFoot));
	FootSlot->SetHorizontalAlignment(HAlign_Fill);
	FootSlot->SetVerticalAlignment(VAlign_Bottom);
	ArmourFillBox = AddLength(WidgetTree, Track, Fill);

	// Leaning like every HUD bar, right under the boss's track.
	USizeBox* Sized = MakeSized(WidgetTree, Rimmed, ArmourWidth, ArmourHeight);
	Sized->SetRenderShear(FVector2D(ArmourSlant, 0.f));
	UCanvasPanelSlot* BarSlot = Box->AddChildToCanvas(Sized);
	BarSlot->SetPosition(FVector2D(ArmourClusterSide + ArmourTrackInset, ArmourBossBarTop + ArmourBossBarHeight + ArmourGap));
	BarSlot->SetSize(FVector2D(ArmourWidth, ArmourHeight));
	Sized->SetRenderOpacity(0.f);
	Sized->SetVisibility(ESlateVisibility::Collapsed);
	ArmourBar = Sized;
	ArmourOpacity = 0.f;
	ShownArmourFill = -1.f;
	ShownArmourChip = -1.f;
	ApplyArmour();
}

void UHudBossBarWidget::ApplyArmour()
{
	if (!ArmourFillBox || !ArmourChipBox || !ArmourFillImage)
	{
		return;
	}
	// In half pixels, so a draining chip repaints only when its end moves.
	const float FillWidth = FMath::RoundToFloat(ArmourInnerWidth * ArmourFraction * 2.f) * 0.5f;
	const float ChipWidth = FMath::RoundToFloat(ArmourInnerWidth * ArmourChipFraction * 2.f) * 0.5f;
	if (FillWidth != ShownArmourFill)
	{
		ShownArmourFill = FillWidth;
		ArmourFillBox->SetWidthOverride(FillWidth);
		ArmourFillBox->SetVisibility(FillWidth > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (ChipWidth != ShownArmourChip)
	{
		ShownArmourChip = ChipWidth;
		ArmourChipBox->SetWidthOverride(ChipWidth);
		ArmourChipBox->SetVisibility(ChipWidth > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	ArmourFillImage->SetColorAndOpacity(FMath::Lerp(Color::Granite(), FLinearColor::White, ArmourFlashWhite * ArmourFlash));
}

void UHudBossBarWidget::TickArmour(float DeltaSeconds)
{
	if (!ArmourBar)
	{
		return;
	}
	const float Target = bArmourWanted ? 1.f : 0.f;
	if (ArmourOpacity != Target)
	{
		ArmourOpacity = FMath::FInterpConstantTo(ArmourOpacity, Target, DeltaSeconds, 1.f / ArmourFadeSeconds);
		ArmourBar->SetRenderOpacity(ArmourOpacity);
		ArmourBar->SetVisibility(ArmourOpacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		// The phase line steps down under it while it shows.
		if (UCanvasPanelSlot* LineSlot = PhaseLabel ? Cast<UCanvasPanelSlot>(PhaseLabel->Slot) : nullptr)
		{
			const float Top = ArmourBossBarTop + ArmourBossBarHeight + ArmourPhaseGap + ArmourPhaseDrop * ArmourOpacity;
			LineSlot->SetPosition(FVector2D(LineSlot->GetPosition().X, Top));
		}
	}
	bool bMoved = false;
	if (ArmourChipFraction > ArmourFraction)
	{
		if (ArmourChipHold > 0.f)
		{
			ArmourChipHold -= DeltaSeconds;
		}
		else
		{
			ArmourChipFraction = FMath::Max(ArmourFraction, ArmourChipFraction - ArmourChipDrain * DeltaSeconds);
			bMoved = true;
		}
	}
	if (ArmourFlash > 0.f)
	{
		ArmourFlash = FMath::Max(0.f, ArmourFlash - ArmourFlashFade * DeltaSeconds);
		bMoved = true;
	}
	if (bMoved)
	{
		ApplyArmour();
	}
}
