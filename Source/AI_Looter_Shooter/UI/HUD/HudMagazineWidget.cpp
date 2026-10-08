#include "UI/HUD/HudMagazineWidget.h"
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
	// The cartridge is drawn in the mockup's frame: lying down, L along it from the base (0) to the tip (230) and A across
	// it (0-60). Standing, A runs left to right and L up the screen: the pictures are that frame turned a quarter.
	constexpr float CartridgeAcross = 60.f;
	constexpr float CartridgeLength = 230.f;
	/** Screen pixels per unit along the cartridge (196 on screen). */
	constexpr float PixelsAlong = UHudMagazineWidget::Height / CartridgeLength;
	/** The textures are drawn at about twice the size they show at, so they stay crisp. */
	constexpr float CartridgePixelsPerUnit = 2.f;
	/** Bezier curves are flattened into this many straight pieces. */
	constexpr int32 CurveSegments = 12;

	/** The fill runs from just inside the head to past the tip (where the nose cuts it), the mockup's 211 units. */
	constexpr float FillStart = 15.f;
	constexpr float FillRun = 211.f;
	/** The fill's two tones split this far across: the light one on the left, as lit from there. */
	constexpr float ToneSplit = 24.f;
	/** The hatch over the fill: fine diagonal ink lines. */
	constexpr float HatchSpacing = 6.f;
	constexpr float HatchWidth = 1.4f;
	constexpr float HatchOpacity = 0.13f;
	/** The faint marks across the inside at a half and three quarters of the fill. */
	constexpr float MarkFrom = 9.f;
	constexpr float MarkTo = 51.f;
	constexpr float MarkWidth = 1.2f;
	/**
	 * The doubled outline: a dark line under a light one. The dark one reaches past the silhouette, so the outlines'
	 * pictures have OutlineMargin of room on every side and are scaled up to match.
	 */
	constexpr float DarkOutlineWidth = 4.6f;
	constexpr float LightOutlineWidth = 1.8f;
	constexpr float OutlineMargin = 3.f;
	/** The leading edge along the fill's top, in screen pixels. */
	constexpr float EdgeThickness = 1.5f;

	/** The counts: sizes, the dark outline round them, the room across the inside they shrink to fit, and their place. */
	constexpr int32 CountSize = 23;
	constexpr int32 ReserveSize = 14;
	constexpr int32 CountOutline = 2;
	constexpr float CountRoom = 44.f;
	/** Above the head (rim and groove, the bottom 13 px), clear of the cartridge's base. */
	constexpr float CountsBottom = 10.f;
	/** The two lines sit closer than their fonts' line heights, as stacked numbers. */
	constexpr float ReserveTuck = -6.f;

	/** How fast the shown level eases toward the rounds left (per second, of the remaining gap): a quick, smooth drain. */
	constexpr float DrainSpeed = 18.f;

	FLinearColor CartridgeWithAlpha(FLinearColor Tone, float Alpha)
	{
		Tone.A = Alpha;
		return Tone;
	}

	/** A point of the lying frame, standing (and moved by Offset within a bigger picture). */
	FVector2D Upright(double Along, double Across, double Offset = 0.0)
	{
		return FVector2D(Across + Offset, CartridgeLength - Along + Offset);
	}

	/** Appends a cubic Bezier's points after its start (which the list already ends with), in the lying frame. */
	void AppendCurve(TArray<FVector2D>& Points, const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3)
	{
		for (int32 Step = 1; Step <= CurveSegments; ++Step)
		{
			const double T = static_cast<double>(Step) / CurveSegments;
			const double U = 1.0 - T;
			Points.Add(P0 * (U * U * U) + P1 * (3.0 * U * U * T) + P2 * (3.0 * U * T * T) + P3 * (T * T * T));
		}
	}

	/**
	 * A closed shape from its upper side (lying frame, base to tip, ending on the axis at the tip): the side, then the same
	 * side mirrored across the axis back to the base. Turned upright and moved by Offset; the first point isn't repeated.
	 */
	TArray<FVector2D> MirroredShape(const TArray<FVector2D>& Side, float Offset)
	{
		TArray<FVector2D> Points;
		Points.Reserve(Side.Num() * 2);
		for (const FVector2D& Point : Side)
		{
			Points.Add(Upright(Point.X, Point.Y, Offset));
		}
		// The tip is on the axis: the mirrored side starts from the point before it.
		for (int32 Index = Side.Num() - 2; Index >= 0; --Index)
		{
			Points.Add(Upright(Side[Index].X, CartridgeAcross - Side[Index].Y, Offset));
		}
		return Points;
	}

	/**
	 * The silhouette's upper side, as the mockup draws it (x along, y across): the head's chamfered rim and its groove,
	 * the case, the shoulder, then the ogive nose to the tip.
	 */
	const TArray<FVector2D>& SilhouetteSide()
	{
		static const TArray<FVector2D> Side = []()
		{
			TArray<FVector2D> Points = { FVector2D(0.f, 4.f), FVector2D(2.f, 2.f), FVector2D(9.f, 2.f), FVector2D(9.f, 9.f), FVector2D(15.f, 9.f),
				FVector2D(15.f, 5.f), FVector2D(176.f, 5.f), FVector2D(186.f, 10.f) };
			AppendCurve(Points, FVector2D(186.f, 10.f), FVector2D(206.f, 12.f), FVector2D(222.f, 20.f), FVector2D(230.f, 30.f));
			return Points;
		}();
		return Side;
	}

	/** The inside's upper side, a little in from the silhouette: where the fill and its edge show. */
	const TArray<FVector2D>& InsideSide()
	{
		static const TArray<FVector2D> Side = []()
		{
			TArray<FVector2D> Points = { FVector2D(17.f, 7.5f), FVector2D(175.f, 7.5f), FVector2D(184.f, 12.f) };
			AppendCurve(Points, FVector2D(184.f, 12.f), FVector2D(202.f, 14.f), FVector2D(216.f, 21.f), FVector2D(223.f, 30.f));
			return Points;
		}();
		return Side;
	}

	/** A shape painted in one flat colour (white for a picture that's tinted per use). */
	FPaintedIcon FlatShape(const TArray<FVector2D>& Shape, const FLinearColor& Tone)
	{
		FPaintedIcon Icon;
		Icon.ViewBox = FVector2D(CartridgeAcross, CartridgeLength);
		FPaintLayer Layer;
		Layer.Fills.Add(Shape);
		Layer.Color = Tone;
		Icon.Layers.Add(Layer);
		return Icon;
	}

	/** The whole silhouette, white: the empty inside behind the fill (tinted dark, a background). */
	const FPaintedIcon& BodyIcon()
	{
		static const FPaintedIcon Icon = FlatShape(MirroredShape(SilhouetteSide(), 0.f), FLinearColor::White);
		return Icon;
	}

	/** The inside, white: the leading edge is a thin band cropped from it. */
	const FPaintedIcon& InsideMaskIcon()
	{
		static const FPaintedIcon Icon = FlatShape(MirroredShape(InsideSide(), 0.f), FLinearColor::White);
		return Icon;
	}

	/**
	 * The parts of the line X + Y = Sum inside Polygon (any simple polygon), each pulled in by Trim at both ends so its
	 * round caps stay inside: a hatch line, rising to the right.
	 */
	void AppendHatchLine(TArray<TArray<FVector2D>>& Lines, const TArray<FVector2D>& Polygon, double Sum, double Trim)
	{
		TArray<double, TInlineAllocator<8>> Crossings;
		for (int32 Index = 0; Index < Polygon.Num(); ++Index)
		{
			const FVector2D& A = Polygon[Index];
			const FVector2D& B = Polygon[(Index + 1) % Polygon.Num()];
			const double SideA = A.X + A.Y - Sum;
			const double SideB = B.X + B.Y - Sum;
			if ((SideA > 0.0) != (SideB > 0.0))
			{
				Crossings.Add(FMath::Lerp(A.X, B.X, SideA / (SideA - SideB)));
			}
		}
		Crossings.Sort();
		// Along the line, X moves by the length over the square root of two.
		const double TrimX = Trim / FMath::Sqrt(2.0);
		for (int32 Index = 0; Index + 1 < Crossings.Num(); Index += 2)
		{
			const double From = Crossings[Index] + TrimX;
			const double To = Crossings[Index + 1] - TrimX;
			if (To > From)
			{
				Lines.Add({ FVector2D(From, Sum - From), FVector2D(To, Sum - To) });
			}
		}
	}

	/**
	 * The full fill in one colour: the inside in two flat tones (Light on the left, Dark the rest, split by a gradient
	 * one unit wide so the line between them is smooth) under a fine diagonal hatch in ink.
	 */
	FPaintedIcon FillIcon(const FLinearColor& Light, const FLinearColor& Dark)
	{
		const TArray<FVector2D> Inside = MirroredShape(InsideSide(), 0.f);
		FPaintedIcon Icon = FlatShape(Inside, Light);
		FPaintLayer& Tones = Icon.Layers[0];
		Tones.GradientTo = Dark;
		Tones.GradientStart = FVector2D(ToneSplit - 0.5f, 0.f);
		Tones.GradientEnd = FVector2D(ToneSplit + 0.5f, 0.f);

		FPaintLayer Hatch;
		Hatch.StrokeWidth = HatchWidth;
		Hatch.Color = Color::Ink();
		Hatch.Opacity = HatchOpacity;
		const double Step = HatchSpacing * FMath::Sqrt(2.0);
		for (double Sum = Step * 0.5; Sum < CartridgeAcross + CartridgeLength; Sum += Step)
		{
			AppendHatchLine(Hatch.Strokes, Inside, Sum, HatchWidth * 0.5);
		}
		Icon.Layers.Add(Hatch);
		return Icon;
	}

	const FPaintedIcon& CyanFillIcon() { static const FPaintedIcon Icon = FillIcon(Color::XPLight(), Color::XPDark()); return Icon; }
	const FPaintedIcon& OrangeFillIcon() { static const FPaintedIcon Icon = FillIcon(Color::AccentLight(), Color::Accent()); return Icon; }

	/** The faint marks across the inside at a half and three quarters of the fill. */
	const FVectorIcon& MarksIcon()
	{
		static const FVectorIcon Icon = []()
		{
			FVectorIcon Marks;
			Marks.ViewBox = FVector2D(CartridgeAcross, CartridgeLength);
			Marks.StrokeWidth = MarkWidth;
			for (const float Fraction : { 0.5f, 0.75f })
			{
				const float Along = FillStart + Fraction * FillRun;
				Marks.Strokes.Add({ Upright(Along, MarkFrom), Upright(Along, MarkTo) });
			}
			return Marks;
		}();
		return Icon;
	}

	/** The silhouette's outline, Width wide, in a picture OutlineMargin bigger on every side. */
	FVectorIcon OutlineIcon(float Width)
	{
		FVectorIcon Outline;
		Outline.ViewBox = FVector2D(CartridgeAcross + OutlineMargin * 2.f, CartridgeLength + OutlineMargin * 2.f);
		Outline.StrokeWidth = Width;
		TArray<FVector2D> Line = MirroredShape(SilhouetteSide(), OutlineMargin);
		const FVector2D First = Line[0];
		Line.Add(First);
		Outline.Strokes.Add(Line);
		return Outline;
	}

	const FVectorIcon& DarkOutlineIcon() { static const FVectorIcon Icon = OutlineIcon(DarkOutlineWidth); return Icon; }
	const FVectorIcon& LightOutlineIcon() { static const FVectorIcon Icon = OutlineIcon(LightOutlineWidth); return Icon; }

	UImage* AddCartridgeLayer(UWidgetTree* Tree, UOverlay* Overlay, const FSlateBrush& Brush, EVerticalAlignment V = VAlign_Fill)
	{
		UImage* Image = MakeImage(Tree, Brush);
		// Left- and bottom-aligned, so the fill's cropped picture starts where the full ones do.
		UOverlaySlot* LayerSlot = Overlay->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(HAlign_Left);
		LayerSlot->SetVerticalAlignment(V);
		return Image;
	}

	/** A count line: outlined dark, shrinking to fit across the inside when it's long. */
	UWidget* MakeCountLine(UWidgetTree* Tree, UTextBlock*& OutText, int32 Size, const FLinearColor& Tone)
	{
		OutText = MakeFloatingText(Tree, Size, Tone, 0, ETextJustify::Center);
		FSlateFontInfo LineFont = FloatingFont(Size);
		LineFont.OutlineSettings.OutlineSize = CountOutline;
		OutText->SetFont(LineFont);
		UScaleBox* Fit = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		Fit->SetStretch(EStretch::ScaleToFit);
		Fit->SetStretchDirection(EStretchDirection::DownOnly);
		Fit->SetContent(OutText);
		return MakeSized(Tree, Fit, CountRoom);
	}
}

TSharedRef<SWidget> UHudMagazineWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		const FVector2D FullSize(Width, Height);
		UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		USizeBox* Root = MakeSized(WidgetTree, Layers, Width, Height);
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;

		// The empty inside: dark, and it fades with the UI transparency setting.
		UImage* Body = AddCartridgeLayer(WidgetTree, Layers, PaintedIconBrush(TEXT("HudCartridgeBody"), BodyIcon(), CartridgePixelsPerUnit,
			FullSize, CartridgeWithAlpha(Color::Track(), 0.76f)));
		MarkBackground(Body);

		// The fill and its leading edge, cropped to the level from the bottom (PaintFill).
		CyanFillBrush = PaintedIconBrush(TEXT("HudCartridgeFillCyan"), CyanFillIcon(), CartridgePixelsPerUnit, FullSize);
		OrangeFillBrush = PaintedIconBrush(TEXT("HudCartridgeFillOrange"), OrangeFillIcon(), CartridgePixelsPerUnit, FullSize);
		EdgeBrush = PaintedIconBrush(TEXT("HudCartridgeInside"), InsideMaskIcon(), CartridgePixelsPerUnit, FullSize);
		FillImage = AddCartridgeLayer(WidgetTree, Layers, CyanFillBrush, VAlign_Bottom);
		FillImage->SetVisibility(ESlateVisibility::Hidden);
		EdgeImage = AddCartridgeLayer(WidgetTree, Layers, EdgeBrush, VAlign_Bottom);
		EdgeImage->SetColorAndOpacity(CartridgeWithAlpha(Color::Text(), 0.9f));
		EdgeImage->SetVisibility(ESlateVisibility::Hidden);

		AddCartridgeLayer(WidgetTree, Layers, IconBrush(TEXT("HudCartridgeMarks"), MarksIcon(), CartridgePixelsPerUnit, FullSize,
			CartridgeWithAlpha(Color::Text(), 0.4f)));

		// The doubled outline. Its pictures reach past the silhouette: laid out at the cartridge's size and scaled up about
		// its middle, which leaves the layout alone.
		const FVector2D OutlineScale((CartridgeAcross + OutlineMargin * 2.f) / CartridgeAcross, (CartridgeLength + OutlineMargin * 2.f) / CartridgeLength);
		UImage* DarkOutline = AddCartridgeLayer(WidgetTree, Layers, IconBrush(TEXT("HudCartridgeOutlineDark"), DarkOutlineIcon(),
			CartridgePixelsPerUnit, FullSize, Color::Ink()));
		DarkOutline->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		DarkOutline->SetRenderScale(OutlineScale);
		OutlineImage = AddCartridgeLayer(WidgetTree, Layers, IconBrush(TEXT("HudCartridgeOutlineLight"), LightOutlineIcon(),
			CartridgePixelsPerUnit, FullSize, FLinearColor::White));
		OutlineImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		OutlineImage->SetRenderScale(OutlineScale);
		OutlineImage->SetColorAndOpacity(Color::Text());

		// The counts inside, by the base, centred: the magazine's rounds over the reserve.
		UVerticalBox* Counts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UTextBlock* Count = nullptr;
		UTextBlock* Reserve = nullptr;
		Counts->AddChildToVerticalBox(MakeCountLine(WidgetTree, Count, CountSize, Color::Text()))->SetHorizontalAlignment(HAlign_Center);
		UVerticalBoxSlot* ReserveSlot = Counts->AddChildToVerticalBox(MakeCountLine(WidgetTree, Reserve, ReserveSize, Color::ReserveText()));
		ReserveSlot->SetHorizontalAlignment(HAlign_Center);
		ReserveSlot->SetPadding(FMargin(0.f, ReserveTuck, 0.f, 0.f));
		CountText = Count;
		ReserveText = Reserve;
		UOverlaySlot* CountsSlot = Layers->AddChildToOverlay(Counts);
		CountsSlot->SetHorizontalAlignment(HAlign_Center);
		CountsSlot->SetVerticalAlignment(VAlign_Bottom);
		CountsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, CountsBottom));

		bOrangeFill = false;
		ShownFraction = -1.f;
		ShownLength = -1.f;
		ShownRounds = INDEX_NONE;
		ShownReserve = INDEX_NONE;
		ShownState = INDEX_NONE;
		bWasReloading = false;
	}
	return Super::RebuildWidget();
}

void UHudMagazineWidget::SetMagazine(int32 Rounds, int32 Capacity, int32 Reserve, bool bReloading, float ReloadProgress, bool bSnap,
	float Pulse, float DeltaTime)
{
	if (!FillImage || !EdgeImage || !OutlineImage || !CountText || !ReserveText)
	{
		return;
	}

	const float MagazineFraction = FMath::Clamp(static_cast<float>(Rounds) / FMath::Max(1, Capacity), 0.f, 1.f);
	const bool bEmpty = Rounds <= 0;
	const bool bLow = MagazineFraction <= LowFraction;
	const bool bNoReserve = Reserve <= 0;

	// The level: the rounds left, or while reloading the reload's progress. A new gun shows its magazine at once, and a
	// reload starts its bar from empty; otherwise the level eases, so each shot reads as a quick drain from the tip.
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
	// Orange when low, and for the reload's fill; a change of colour crops the other picture.
	const bool bOrange = bReloading || bLow;
	if (bOrange != bOrangeFill)
	{
		bOrangeFill = bOrange;
		ShownLength = -1.f;
	}
	PaintFill(ShownFraction);

	if (Rounds != ShownRounds)
	{
		ShownRounds = Rounds;
		CountText->SetText(FText::FromString(FString::FromInt(Rounds)));
	}
	if (Reserve != ShownReserve)
	{
		ShownReserve = Reserve;
		ReserveText->SetText(FText::FromString(FString::Printf(TEXT("/%d"), FMath::Max(0, Reserve))));
	}

	// Colours only change with the state, except an empty gun's beating outline.
	const int32 State = (bReloading ? 8 : 0) | (bEmpty ? 4 : 0) | (bLow ? 2 : 0) | (bNoReserve ? 1 : 0);
	if (State != ShownState)
	{
		ShownState = State;
		CountText->SetColorAndOpacity(FSlateColor(bEmpty ? Color::Worse() : (bLow ? Color::Accent() : Color::Text())));
		ReserveText->SetColorAndOpacity(FSlateColor(bNoReserve ? Color::Worse() : Color::ReserveText()));
		OutlineImage->SetColorAndOpacity(Color::Text());
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
	const float Level = Fraction > 0.f ? (FillStart + FMath::Clamp(Fraction, 0.f, 1.f) * FillRun) * PixelsAlong : 0.f;
	const float Length = FMath::RoundToFloat(Level * 2.f) * 0.5f;
	if (Length == ShownLength)
	{
		return;
	}
	ShownLength = Length;
	if (Length <= 0.f)
	{
		FillImage->SetVisibility(ESlateVisibility::Hidden);
		EdgeImage->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const float Top = 1.f - Length / Height;
	FSlateBrush Cropped = bOrangeFill ? OrangeFillBrush : CyanFillBrush;
	Cropped.ImageSize = FVector2D(Width, Length);
	Cropped.SetUVRegion(FBox2f(FVector2f(0.f, Top), FVector2f(1.f, 1.f)));
	FillImage->SetBrush(Cropped);
	FillImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	// The leading edge: a thin band of the inside's silhouette at the level, so it spans exactly the inside's width there
	// (narrowing up the nose, nothing past the tip).
	const float Band = FMath::Min(EdgeThickness, Length);
	FSlateBrush Edge = EdgeBrush;
	Edge.ImageSize = FVector2D(Width, Band);
	Edge.SetUVRegion(FBox2f(FVector2f(0.f, Top), FVector2f(1.f, Top + Band / Height)));
	EdgeImage->SetBrush(Edge);
	EdgeImage->SetRenderTranslation(FVector2D(0.f, Band - Length));
	EdgeImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}
