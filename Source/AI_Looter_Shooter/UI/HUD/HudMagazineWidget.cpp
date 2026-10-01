#include "UI/HUD/HudMagazineWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	constexpr float GaugeWidth = UHudMagazineWidget::Width;
	constexpr float GaugeHeight = UHudMagazineWidget::Height;
	constexpr float MidLine = GaugeHeight * 0.5f;

	// The cartridge's outline in reference pixels (y down), base on the left: the rim, the groove cut in front of it, the
	// straight case up to its mouth, then the bullet, a little narrower: a short straight part and a pointed ogive nose.
	// The outline's center line keeps clear of the texture's edge so the stroke isn't cut off.
	constexpr float RimLeft = 1.5f;
	constexpr float RimRight = 11.f;
	constexpr float RimTop = 3.f;
	constexpr float GrooveRight = 17.f;
	constexpr float GrooveTop = 10.f;
	constexpr float CaseTop = 6.f;
	/** Placed so the case's inside ends three quarters of the way along the fill: the nose holds the first quarter fired. */
	constexpr float CaseMouth = 176.5f;
	constexpr float BulletTop = 9.f;
	constexpr float OgiveStart = 188.f;
	constexpr float Tip = 228.5f;
	constexpr int32 OgiveSegments = 10;

	constexpr float OutlineWidth = 2.f;
	/** How far the inside sits in from the outline's center line, leaving a thin gap inside the outline. */
	constexpr float Inset = 3.5f;
	/** Where the fill starts: just inside the case, in front of the head. */
	constexpr float InsideStart = GrooveRight + Inset;
	/** The quarter marks: short notches down from the top and up from the bottom, clear of the count's middle. */
	constexpr float TickLength = 8.f;
	constexpr float TickWidth = 1.5f;
	/** The count's left edge, a little in from where the fill starts. */
	constexpr float CountLeft = 27.f;
	/** The textures are drawn at twice the size they show at, so they stay crisp. */
	constexpr float PixelsPerUnit = 2.f;

	/** How fast the shown level eases toward the rounds left (per second, of the remaining gap): a quick, smooth drain. */
	constexpr float DrainSpeed = 18.f;
	/** The fill is a little see-through, so the count in the same color over it still stands out. */
	constexpr float FillAlpha = 0.8f;

	FVector2D Mirror(const FVector2D& Point)
	{
		return FVector2D(Point.X, GaugeHeight - Point.Y);
	}

	/** A rectangle's corners, clockwise on screen from the top-left. */
	TArray<FVector2D> Rect(float Left, float Top, float Right, float Bottom)
	{
		return { FVector2D(Left, Top), FVector2D(Right, Top), FVector2D(Right, Bottom), FVector2D(Left, Bottom) };
	}

	/**
	 * The nose's upper curve, Inward in from the outline's center line, from where it leaves the bullet's straight side to
	 * the tip on the axis. A tangent ogive: an arc that leaves the side smoothly and meets the axis in a point; the inside
	 * is the same arc around the same center, Inward smaller.
	 */
	TArray<FVector2D> OgiveUpper(float Inward)
	{
		const double Radius = MidLine - BulletTop;
		const double Length = Tip - OgiveStart;
		const double Rho = (Length * Length + Radius * Radius) / (2.0 * Radius);
		const FVector2D Center(OgiveStart, BulletTop + Rho);
		const double ArcRadius = Rho - Inward;
		const double EndAngle = FMath::Acos(FMath::Clamp((Rho - Radius) / ArcRadius, -1.0, 1.0));
		TArray<FVector2D> Points;
		for (int32 Step = 0; Step <= OgiveSegments; ++Step)
		{
			const double Angle = EndAngle * Step / OgiveSegments;
			Points.Add(Center + FVector2D(FMath::Sin(Angle), -FMath::Cos(Angle)) * ArcRadius);
		}
		return Points;
	}

	/** Where the fill ends: the inside's tip. */
	float InsideEnd()
	{
		static const float End = static_cast<float>(OgiveUpper(Inset).Last().X);
		return End;
	}

	/** The outline: the whole silhouette as one closed stroke, with the head (rim and groove) drawn solid. */
	const FVectorIcon& OutlineIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Shape;
			Shape.ViewBox = FVector2D(GaugeWidth, GaugeHeight);
			Shape.StrokeWidth = OutlineWidth;

			// Clockwise from the rim's top-left: along the top to the tip, then back along the mirrored bottom.
			const TArray<FVector2D> Top = {
				FVector2D(RimLeft, RimTop), FVector2D(RimRight, RimTop), FVector2D(RimRight, GrooveTop), FVector2D(GrooveRight, GrooveTop),
				FVector2D(GrooveRight, CaseTop), FVector2D(CaseMouth, CaseTop), FVector2D(CaseMouth, BulletTop) };
			const TArray<FVector2D> Nose = OgiveUpper(0.f);
			TArray<FVector2D> Line = Top;
			Line.Append(Nose);
			for (int32 Index = Nose.Num() - 2; Index >= 0; --Index)
			{
				Line.Add(Mirror(Nose[Index]));
			}
			for (int32 Index = Top.Num() - 1; Index >= 0; --Index)
			{
				Line.Add(Mirror(Top[Index]));
			}
			Line.Add(Top[0]);
			Shape.Strokes.Add(Line);

			// The head reads as the cartridge's base. The groove's block reaches as far as the outline's inner edge would,
			// so the gap before the fill matches the gap inside the outline.
			Shape.Fills.Add(Rect(RimLeft, RimTop, RimRight + 0.5f, GaugeHeight - RimTop));
			Shape.Fills.Add(Rect(RimRight, GrooveTop, GrooveRight + OutlineWidth * 0.5f, GaugeHeight - GrooveTop));
			return Shape;
		}();
		return Icon;
	}

	/** The inside of the case and the bullet: the fill, and (fainter) the empty part behind it. */
	const FVectorIcon& InsideIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Shape;
			Shape.ViewBox = FVector2D(GaugeWidth, GaugeHeight);
			// Fills must be convex: the case is a rectangle and the bullet one polygon, overlapping a pixel at the mouth.
			Shape.Fills.Add(Rect(InsideStart, CaseTop + Inset, CaseMouth - Inset, GaugeHeight - CaseTop - Inset));
			const TArray<FVector2D> Nose = OgiveUpper(Inset);
			TArray<FVector2D> Bullet = { FVector2D(CaseMouth - Inset - 1.f, BulletTop + Inset) };
			Bullet.Append(Nose);
			for (int32 Index = Nose.Num() - 2; Index >= 0; --Index)
			{
				Bullet.Add(Mirror(Nose[Index]));
			}
			Bullet.Add(FVector2D(CaseMouth - Inset - 1.f, GaugeHeight - BulletTop - Inset));
			Shape.Fills.Add(Bullet);
			return Shape;
		}();
		return Icon;
	}

	/** Faint notches at a quarter and half of the fill (the case's mouth marks three quarters). */
	const FVectorIcon& TicksIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Shape;
			Shape.ViewBox = FVector2D(GaugeWidth, GaugeHeight);
			Shape.StrokeWidth = TickWidth;
			const float InsideTop = CaseTop + Inset + TickWidth * 0.5f;
			const float InsideBottom = GaugeHeight - CaseTop - Inset - TickWidth * 0.5f;
			const float Fractions[] = { 0.25f, 0.5f };
			for (const float Fraction : Fractions)
			{
				const float X = InsideStart + Fraction * (InsideEnd() - InsideStart);
				Shape.Strokes.Add({ FVector2D(X, InsideTop), FVector2D(X, InsideTop + TickLength) });
				Shape.Strokes.Add({ FVector2D(X, InsideBottom - TickLength), FVector2D(X, InsideBottom) });
			}
			return Shape;
		}();
		return Icon;
	}

	/** The outline's color when nothing is wrong. */
	FLinearColor RestingLine()
	{
		return Color::Text() * FLinearColor(1.f, 1.f, 1.f, 0.85f);
	}

	UImage* AddLayer(UWidgetTree* Tree, UOverlay* Overlay, const FSlateBrush& Brush)
	{
		UImage* Image = MakeImage(Tree, Brush);
		// Left-aligned, so the fill's cropped picture starts where the full ones do.
		UOverlaySlot* LayerSlot = Overlay->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(HAlign_Left);
		LayerSlot->SetVerticalAlignment(VAlign_Center);
		return Image;
	}
}

TSharedRef<SWidget> UHudMagazineWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		const FVector2D FullSize(Width, Height);
		UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Layers->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Layers;

		// The empty inside is the bar's unlit part: faint, and it fades with the UI transparency setting.
		FillBrush = IconBrush(TEXT("HudMagazineInside"), InsideIcon(), PixelsPerUnit, FullSize, FLinearColor::White);
		UImage* EmptyImage = AddLayer(WidgetTree, Layers, FillBrush);
		EmptyImage->SetColorAndOpacity(Color::SegmentOff());
		MarkBackground(EmptyImage);
		FillImage = AddLayer(WidgetTree, Layers, FillBrush);
		UImage* Ticks = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudMagazineTicks"), TicksIcon(), PixelsPerUnit, FullSize, FLinearColor::White));
		Ticks->SetColorAndOpacity(Color::Outline() * FLinearColor(1.f, 1.f, 1.f, 0.7f));
		OutlineImage = AddLayer(WidgetTree, Layers, IconBrush(TEXT("HudMagazineOutline"), OutlineIcon(), PixelsPerUnit, FullSize, FLinearColor::White));
		OutlineImage->SetColorAndOpacity(RestingLine());

		// The count over the fill, by the base. Chakra Petch's digits sit in the middle of their line, so centering the
		// line centers them in the case.
		CountText = MakeFloatingText(WidgetTree, 40, Color::Text(), 0, ETextJustify::Left);
		UOverlaySlot* CountSlot = Layers->AddChildToOverlay(CountText);
		CountSlot->SetHorizontalAlignment(HAlign_Left);
		CountSlot->SetVerticalAlignment(VAlign_Center);
		CountSlot->SetPadding(FMargin(CountLeft, 0.f, 0.f, 0.f));

		ShownFraction = -1.f;
		ShownLength = -1.f;
		ShownRounds = INDEX_NONE;
		ShownState = INDEX_NONE;
		bWasReloading = false;
	}
	return Super::RebuildWidget();
}

void UHudMagazineWidget::SetMagazine(int32 Rounds, int32 Capacity, bool bReloading, float ReloadProgress, bool bSnap, float Pulse, float DeltaTime)
{
	if (!FillImage || !OutlineImage || !CountText)
	{
		return;
	}

	const float MagazineFraction = FMath::Clamp(static_cast<float>(Rounds) / FMath::Max(1, Capacity), 0.f, 1.f);
	const bool bEmpty = Rounds <= 0;
	const bool bLow = MagazineFraction <= LowFraction;

	// The level: the rounds left, or while reloading the reload's progress. A new gun shows its magazine at once, and a
	// reload starts its bar from empty; otherwise the level eases, so each shot reads as a quick drain.
	const float Target = bReloading ? FMath::Clamp(ReloadProgress, 0.f, 1.f) : MagazineFraction;
	if (bSnap || ShownFraction < 0.f || (bReloading && !bWasReloading))
	{
		ShownFraction = Target;
	}
	else
	{
		ShownFraction = FMath::FInterpTo(ShownFraction, Target, DeltaTime, DrainSpeed);
	}
	bWasReloading = bReloading;
	PaintFill(ShownFraction);

	if (Rounds != ShownRounds)
	{
		ShownRounds = Rounds;
		CountText->SetText(FText::FromString(FString::FromInt(Rounds)));
	}

	// Colors only change with the state, except an empty gun's beating outline.
	const int32 State = (bReloading ? 4 : 0) | (bEmpty ? 2 : 0) | (bLow ? 1 : 0);
	if (State != ShownState)
	{
		ShownState = State;
		FillImage->SetColorAndOpacity((bReloading || bLow ? Color::Accent() : Color::SegmentOn()) * FLinearColor(1.f, 1.f, 1.f, FillAlpha));
		CountText->SetColorAndOpacity(FSlateColor(bEmpty ? Color::Worse() : (bLow ? Color::Accent() : Color::Text())));
		OutlineImage->SetColorAndOpacity(RestingLine());
	}
	// Dry and not yet reloading: the outline beats with the HUD's reload prompt.
	if (bEmpty && !bReloading)
	{
		OutlineImage->SetColorAndOpacity(FMath::Lerp(Color::Accent(), Color::Worse(), FMath::Clamp(Pulse, 0.f, 1.f)));
	}
}

void UHudMagazineWidget::PaintFill(float Fraction)
{
	// The fill's picture is cropped, not squeezed: it shows the inside from the base up to the level, so the nose keeps its
	// shape and the level reads as a straight edge. (A clipping box can't do this inside an overlay or size box: they
	// shrink their child to fit.) In half pixels, so easing only repaints when the edge visibly moves.
	const float Level = Fraction > 0.f ? InsideStart + FMath::Clamp(Fraction, 0.f, 1.f) * (InsideEnd() - InsideStart) : 0.f;
	const float Length = FMath::RoundToFloat(Level * 2.f) * 0.5f;
	if (Length == ShownLength)
	{
		return;
	}
	ShownLength = Length;
	if (Length <= 0.f)
	{
		FillImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	FSlateBrush Cropped = FillBrush;
	Cropped.ImageSize = FVector2D(Length, Height);
	Cropped.SetUVRegion(FBox2f(FVector2f(0.f, 0.f), FVector2f(Length / Width, 1.f)));
	FillImage->SetBrush(Cropped);
	FillImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}
