#include "UI/HUD/HudVitalsWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

using namespace LooterUI;

namespace
{
	constexpr float BoxWidth = UHudVitalsWidget::Width;
	constexpr float BoxHeight = UHudVitalsWidget::Height;
	/** The ring's middle, both across and down: it sits in the box's left end. */
	constexpr float RingMiddle = UHudVitalsWidget::RingDiameter * 0.5f;

	constexpr float OutlineWidth = 2.f;
	/** The ring outline's center line, kept clear of the box's edge so the stroke isn't cut off. */
	constexpr float RingRadius = RingMiddle - OutlineWidth * 0.5f - 0.5f;
	/**
	 * The bar's outline (center line), well below the ring's middle so the bar comes out of the ring's lower side, its
	 * bottom a little above the ring's.
	 */
	constexpr float BarTop = 50.f;
	constexpr float BarBottom = 68.f;
	/** The far end's top corner; the end leans back toward the ring by the HUD's bar slant (degrees), like the other bars. */
	constexpr float BarEnd = UHudVitalsWidget::RingDiameter + UHudVitalsWidget::BarLength;
	constexpr float BarSlantDegrees = 16.f;
	/** How far the bar's inside sits in from the outline's center line, leaving a thin gap inside it (as the magazine does). */
	constexpr float Inset = 3.f;
	constexpr float FillTop = BarTop + Inset;
	constexpr float FillBottom = BarBottom - Inset;
	/** The inside's picture covers only the bar's band (a pixel over above and below), placed this far down the box. */
	constexpr float FillBoxTop = FillTop - 1.f;
	constexpr float FillBoxHeight = FillBottom - FillTop + 2.f;
	/** Points round the ring: at the drawn size (twice that in the texture) the facets stay well under a pixel. */
	constexpr int32 RingSegments = 64;
	/** The textures are drawn at twice the size they show at, so they stay crisp. */
	constexpr float PixelsPerUnit = 2.f;

	/** The health number in the ring, and the widest it gets before shrinking to fit (a long number still clears the ring). */
	constexpr int32 NumberSize = 28;
	constexpr float NumberMaxWidth = 60.f;

	/** Opacity when nothing is happening, and how long it stays fully visible after something happens (as the HUD's corners do). */
	constexpr float IdleOpacity = 0.6f;
	constexpr float ActivityHold = 3.f;
	/** After a hit the lost part lingers this long, then drains at this share of the bar per second. */
	constexpr float ChipHoldSeconds = 0.45f;
	constexpr float ChipDrainSpeed = 0.6f;
	/** The ring flashes red after a hit, fading over this long. */
	constexpr float HitFlashSeconds = 0.35f;
	/** The low-health beat, in radians per second. */
	constexpr float PulseSpeed = 6.f;

	/** Where a circle of Radius round the ring's middle crosses height Y, on its right side. */
	float RingRightX(float Radius, float Y)
	{
		const float Rise = Y - RingMiddle;
		return RingMiddle + FMath::Sqrt(FMath::Max(0.f, Radius * Radius - Rise * Rise));
	}

	/** The far end's leaning line at height Y, moved Shift toward the ring. */
	float FarEndX(float Y, float Shift)
	{
		return BarEnd - (Y - BarTop) * FMath::Tan(FMath::DegreesToRadians(BarSlantDegrees)) - Shift;
	}

	/** The inside's far end sits Inset in from the outline's, measured square to the leaning end. */
	float FarInsetShift()
	{
		return Inset / FMath::Cos(FMath::DegreesToRadians(BarSlantDegrees));
	}

	/** Where an empty bar starts (the near end's lowest point) and a full one ends (the far end's highest). */
	float FillStart()
	{
		static const float Start = RingRightX(RingRadius + Inset, FillBottom);
		return Start;
	}

	float FillEnd()
	{
		static const float End = FarEndX(FillTop, FarInsetShift());
		return End;
	}

	/** A circle of Radius round the ring's middle, clockwise on screen from the right, as a convex polygon. */
	TArray<FVector2D> CirclePoints(float Radius)
	{
		TArray<FVector2D> Points;
		Points.Reserve(RingSegments);
		for (int32 Index = 0; Index < RingSegments; ++Index)
		{
			const float Angle = 2.f * UE_PI * static_cast<float>(Index) / static_cast<float>(RingSegments);
			Points.Add(FVector2D(RingMiddle + FMath::Cos(Angle) * Radius, RingMiddle + FMath::Sin(Angle) * Radius));
		}
		return Points;
	}

	/** The outline: the ring, and the bar's top, far end and bottom, left open where they meet the ring (it closes them). */
	const FVectorIcon& OutlineIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Shape;
			Shape.ViewBox = FVector2D(BoxWidth, BoxHeight);
			Shape.StrokeWidth = OutlineWidth;
			TArray<FVector2D> Ring = CirclePoints(RingRadius);
			// A copy first: adding an element of the array to itself could read it after the array has moved.
			const FVector2D First = Ring[0];
			Ring.Add(First);
			Shape.Strokes.Add(Ring);
			Shape.Strokes.Add({
				FVector2D(RingRightX(RingRadius, BarTop), BarTop), FVector2D(FarEndX(BarTop, 0.f), BarTop),
				FVector2D(FarEndX(BarBottom, 0.f), BarBottom), FVector2D(RingRightX(RingRadius, BarBottom), BarBottom) });
			return Shape;
		}();
		return Icon;
	}

	/** The ring's disc, reaching under the ring's inner half. */
	const FVectorIcon& DiscIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Shape;
			Shape.ViewBox = FVector2D(UHudVitalsWidget::RingDiameter, UHudVitalsWidget::RingDiameter);
			Shape.Fills.Add(CirclePoints(RingRadius));
			return Shape;
		}();
		return Icon;
	}

	/**
	 * The bar's inside in its own band (y measured from FillBoxTop): the fill, the chip and (fainter) the empty part. One
	 * convex quad (fills must be convex): the near end follows the ring on a straight chord, which over the bar's height
	 * strays under a pixel from the curve, and the far end leans with the outline's.
	 */
	const FVectorIcon& BarInsideIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Shape;
			Shape.ViewBox = FVector2D(BoxWidth, FillBoxHeight);
			const float NearRadius = RingRadius + Inset;
			const float Shift = FarInsetShift();
			Shape.Fills.Add({
				FVector2D(RingRightX(NearRadius, FillTop), FillTop - FillBoxTop), FVector2D(FarEndX(FillTop, Shift), FillTop - FillBoxTop),
				FVector2D(FarEndX(FillBottom, Shift), FillBottom - FillBoxTop), FVector2D(RingRightX(NearRadius, FillBottom), FillBottom - FillBoxTop) });
			return Shape;
		}();
		return Icon;
	}

	/** The outline's color when nothing is wrong (as the magazine's). */
	FLinearColor RestingLine()
	{
		return Color::Text() * FLinearColor(1.f, 1.f, 1.f, 0.85f);
	}

	/** The lighter red the low-health beat swings to. */
	FLinearColor LowBeat()
	{
		return Hex(255, 225, 219);
	}

	/** The part the last hit took off, lingering before it drains. */
	FLinearColor ChipColor()
	{
		return Hex(255, 233, 221, 230);
	}

	/** The disc at rest (as an empty weapon slot's), and the red it throbs to at low health. */
	FLinearColor RestingDisc()
	{
		return Hex(7, 26, 40, 128);
	}

	FLinearColor LowDisc()
	{
		return Color::Health() * FLinearColor(1.f, 1.f, 1.f, 0.4f);
	}

	UImage* AddLayer(UWidgetTree* Tree, UOverlay* Overlay, const FSlateBrush& Brush, float Top = 0.f)
	{
		UImage* Image = MakeImage(Tree, Brush);
		// Top-left aligned, so a cropped bar starts where the full one does.
		UOverlaySlot* LayerSlot = Overlay->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(HAlign_Left);
		LayerSlot->SetVerticalAlignment(VAlign_Top);
		LayerSlot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
		return Image;
	}
}

TSharedRef<SWidget> UHudVitalsWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Layers->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Layers;
		Cluster = Layers;

		// The disc and the bar's empty part are the backgrounds: faint, and they fade with the UI transparency setting.
		DiscImage = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudVitalsDisc"), DiscIcon(), PixelsPerUnit,
			FVector2D(RingDiameter, RingDiameter), FLinearColor::White));
		MarkBackground(DiscImage);
		BarBrush = IconBrush(TEXT("HudVitalsBar"), BarInsideIcon(), PixelsPerUnit, FVector2D(Width, FillBoxHeight), FLinearColor::White);
		UImage* Track = AddLayer(WidgetTree, Layers, BarBrush, FillBoxTop);
		Track->SetColorAndOpacity(Color::SegmentOff());
		MarkBackground(Track);
		ChipImage = AddLayer(WidgetTree, Layers, BarBrush, FillBoxTop);
		ChipImage->SetColorAndOpacity(ChipColor());
		FillImage = AddLayer(WidgetTree, Layers, BarBrush, FillBoxTop);
		FillImage->SetColorAndOpacity(Color::Health());
		OutlineImage = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudVitalsOutline"), OutlineIcon(), PixelsPerUnit,
			FVector2D(Width, Height), FLinearColor::White));
		OutlineImage->SetColorAndOpacity(RestingLine());

		// In the ring: the number, shrinking to fit if it ever gets long, over a small "HP". Chakra Petch's digits sit in
		// the middle of their line, so the pair centered in the ring puts the number just above its middle.
		UVerticalBox* Readout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		HealthText = MakeFloatingText(WidgetTree, NumberSize, Color::Text(), 0, ETextJustify::Center);
		UScaleBox* NumberFit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		NumberFit->SetStretch(EStretch::ScaleToFit);
		NumberFit->SetStretchDirection(EStretchDirection::DownOnly);
		NumberFit->SetContent(HealthText);
		USizeBox* NumberBox = MakeSized(WidgetTree, NumberFit, 0.f);
		NumberBox->SetMaxDesiredWidth(NumberMaxWidth);
		Readout->AddChildToVerticalBox(NumberBox)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* Label = MakeFloatingText(WidgetTree, 10, Color::TextDim(), 150, ETextJustify::Center);
		Label->SetText(FText::FromString(TEXT("HP")));
		UVerticalBoxSlot* LabelSlot = Readout->AddChildToVerticalBox(Label);
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetPadding(FMargin(0.f, -4.f, 0.f, 0.f));
		UOverlaySlot* ReadoutSlot = Layers->AddChildToOverlay(MakeSized(WidgetTree, Readout, RingDiameter));
		ReadoutSlot->SetHorizontalAlignment(HAlign_Left);
		ReadoutSlot->SetVerticalAlignment(VAlign_Center);

		ShownFraction = -1.f;
		ChipFraction = -1.f;
		ChipHoldTime = 0.f;
		ShownFillLength = -1.f;
		ShownChipLength = -1.f;
		ShownPoints = INDEX_NONE;
		bColorsAtRest = false;
	}
	return Super::RebuildWidget();
}

void UHudVitalsWidget::SetHealth(float Health, float MaxHealth, float DeltaTime)
{
	if (!Cluster || !DiscImage || !ChipImage || !FillImage || !OutlineImage || !HealthText)
	{
		return;
	}
	PulseTime += DeltaTime;

	const float Fraction = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
	if (ShownFraction < 0.f)
	{
		ShownFraction = Fraction;
		ChipFraction = Fraction;
	}
	if (Fraction < ShownFraction - KINDA_SMALL_NUMBER)
	{
		// Took damage: the bar drops at once, the lost chunk lingers as a light chip and then drains; the ring flashes.
		ChipHoldTime = ChipHoldSeconds;
		HitFlash = HitFlashSeconds;
		Activity = ActivityHold;
	}
	else if (Fraction > ShownFraction + KINDA_SMALL_NUMBER)
	{
		ChipFraction = Fraction;
		Activity = ActivityHold;
	}
	ShownFraction = Fraction;
	if (ChipHoldTime > 0.f)
	{
		ChipHoldTime -= DeltaTime;
	}
	else
	{
		ChipFraction = FMath::FInterpConstantTo(ChipFraction, Fraction, DeltaTime, ChipDrainSpeed);
	}
	ChipFraction = FMath::Max(ChipFraction, Fraction);
	Activity = FMath::Max(0.f, Activity - DeltaTime);
	HitFlash = FMath::Max(0.f, HitFlash - DeltaTime);

	PaintBar(FillImage, Fraction, ShownFillLength);
	PaintBar(ChipImage, ChipFraction, ShownChipLength);

	const int32 Points = FMath::CeilToInt32(FMath::Max(Health, 0.f));
	if (Points != ShownPoints)
	{
		ShownPoints = Points;
		HealthText->SetText(FText::FromString(FString::FromInt(Points)));
	}

	// Colors only change while something beats or flashes; once it settles they're set to rest once and left alone.
	const bool bLow = Fraction <= LowFraction;
	const bool bAnimating = bLow || HitFlash > 0.f;
	if (bAnimating || !bColorsAtRest)
	{
		PaintColors(bLow, 0.5f + 0.5f * FMath::Sin(PulseTime * PulseSpeed));
		bColorsAtRest = !bAnimating;
	}

	// Full health and nothing happening: step back. Hurt, low, or just hit: full strength.
	const float Target = Activity > 0.f || Fraction < 0.999f ? 1.f : IdleOpacity;
	const float Opacity = Cluster->GetRenderOpacity();
	if (!FMath::IsNearlyEqual(Opacity, Target, 0.002f))
	{
		Cluster->SetRenderOpacity(FMath::FInterpTo(Opacity, Target, DeltaTime, 5.f));
	}
}

void UHudVitalsWidget::PaintColors(bool bLow, float Pulse)
{
	const float Flash = FMath::Clamp(HitFlash / HitFlashSeconds, 0.f, 1.f);
	if (bLow)
	{
		// Low: the ring and fill beat between red and a lighter red (a hit lights the ring up), the disc throbs red and
		// the number swings between red and white.
		const FLinearColor Beat = FMath::Lerp(Color::Health(), LowBeat(), FMath::Clamp(Pulse, 0.f, 1.f) * 0.6f);
		FillImage->SetColorAndOpacity(Beat);
		OutlineImage->SetColorAndOpacity(FMath::Lerp(Beat, LowBeat(), Flash));
		DiscImage->SetColorAndOpacity(FMath::Lerp(RestingDisc(), LowDisc(), FMath::Clamp(Pulse, 0.f, 1.f)));
		HealthText->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Health(), Color::Text(), FMath::Clamp(Pulse, 0.f, 1.f))));
		return;
	}
	FillImage->SetColorAndOpacity(Color::Health());
	OutlineImage->SetColorAndOpacity(FMath::Lerp(RestingLine(), Color::Health(), Flash));
	DiscImage->SetColorAndOpacity(RestingDisc());
	HealthText->SetColorAndOpacity(FSlateColor(Color::Text()));
}

void UHudVitalsWidget::PaintBar(UImage* Image, float Fraction, float& ShownLength) const
{
	// The bar's picture is cropped, not squeezed: it shows the inside from the near end up to the level, so the slanted
	// ends keep their shape and the level reads as a straight edge. In half pixels, so a draining chip only repaints when
	// its edge visibly moves.
	float Length = 0.f;
	if (Fraction > 0.f)
	{
		// A full bar shows the whole picture, the far end's soft edge included.
		const float Level = Fraction >= 1.f ? Width : FillStart() + Fraction * (FillEnd() - FillStart());
		Length = FMath::Min(FMath::RoundToFloat(Level * 2.f) * 0.5f, Width);
	}
	if (Length == ShownLength)
	{
		return;
	}
	ShownLength = Length;
	if (Length <= 0.f)
	{
		Image->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	FSlateBrush Cropped = BarBrush;
	Cropped.ImageSize = FVector2D(Length, FillBoxHeight);
	Cropped.SetUVRegion(FBox2f(FVector2f(0.f, 0.f), FVector2f(Length / Width, 1.f)));
	Image->SetBrush(Cropped);
	Image->SetVisibility(ESlateVisibility::HitTestInvisible);
}
