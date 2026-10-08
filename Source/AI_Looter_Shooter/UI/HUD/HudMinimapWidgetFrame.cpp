// The minimap's frame: the gunmetal bezel round the map, its turning ticks and N, the notch, and the place's name.
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "World/MinimapSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

using namespace LooterUI;

namespace
{
	/**
	 * The frame's measures in map pixels at 100% size (the map's radius is 86); all of it grows with the map. The bezel runs
	 * from the map's edge out to 95, and the N's disc rides its middle.
	 */
	constexpr float FrameMapRadius = UHudMinimapWidget::Diameter * 0.5f;
	constexpr float FrameBezelOuter = 95.f;
	constexpr float FrameBezelMiddle = (FrameMapRadius + FrameBezelOuter) * 0.5f;
	/** The bezel's and the ticks' pictures: a square round the map's centre, just past the bezel's outer ink line. */
	constexpr float FrameBox = 192.f;
	constexpr float FrameCenter = FrameBox * 0.5f;
	/** Points round the bezel's circles: at the texture's size the facets stay far under a pixel. */
	constexpr int32 FrameCircleSides = 128;
	/** The pictures are drawn at twice their size at 100%, so they stay crisp up to the biggest map (150%). */
	constexpr float FramePixelsPerUnit = 2.f;

	/** The N's disc: its picture's box (the disc is 19 across, its orange ring just inside) and the letter's size. */
	constexpr float BadgeBox = 22.f;
	constexpr float BadgeRadius = 9.5f;
	constexpr int32 BadgeTextSize = 12;
	/**
	 * The notch: a triangle 14 wide and 9.5 tall in a box with room for its ink edge, centred this far above the map's
	 * centre, so its tip reaches into the bezel and its top stands a little proud of it.
	 */
	constexpr float NotchWidth = 18.f;
	constexpr float NotchHeight = 12.5f;
	constexpr float NotchCenter = 93.75f;

	/** Under the bezel: the gap, then the place in 15 px and the area in 10.5 px, letter-spaced as the mockup's. */
	constexpr float PlaceGap = 7.f;
	constexpr int32 PlaceTextSize = 15;
	constexpr int32 PlaceSpacing = 167;
	constexpr float AreaTextSize = 10.5f;
	constexpr int32 AreaSpacing = 333;

	/** A circle of Radius round Center in Sides points; Closed repeats the first point at the end, for a stroke. */
	TArray<FVector2D> FrameCircle(const FVector2D& Center, float Radius, int32 Sides, bool bClosed)
	{
		TArray<FVector2D> Points;
		Points.Reserve(Sides + 1);
		for (int32 Step = 0; Step < Sides; ++Step)
		{
			const double Angle = UE_TWO_PI * Step / Sides;
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		if (bClosed)
		{
			Points.Add(FVector2D(Points[0])); // a copy: adding an element of the array itself trips TArray's check (the add may reallocate)
		}
		return Points;
	}

	/** A point Radius out from the frame's centre, Degrees clockwise from up. */
	FVector2D FramePoint(float Degrees, float Radius)
	{
		const double Angle = FMath::DegreesToRadians(Degrees);
		return FVector2D(FrameCenter + Radius * FMath::Sin(Angle), FrameCenter - Radius * FMath::Cos(Angle));
	}

	/** One ring stroked in Tone, Width wide, round the frame's centre. */
	FPaintLayer FrameRing(float Radius, float Width, const FLinearColor& Tone, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Strokes.Add(FrameCircle(FVector2D(FrameCenter), Radius, FrameCircleSides, true));
		Layer.StrokeWidth = Width;
		Layer.Color = Tone;
		Layer.Opacity = Opacity;
		return Layer;
	}

	/**
	 * The bezel, lit from above like the HUD's other metalwork: a gunmetal ring between two ink lines, and a cyan hairline
	 * just inside it, over the map's edge.
	 */
	const FPaintedIcon& BezelPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(FrameBox);
			// The ring: the outer circle with the map's cut out of it (even-odd), light at the top, dark at the bottom.
			FPaintLayer Metal;
			Metal.Fills.Add(FrameCircle(FVector2D(FrameCenter), FrameBezelOuter, FrameCircleSides, false));
			Metal.Fills.Add(FrameCircle(FVector2D(FrameCenter), FrameMapRadius, FrameCircleSides, false));
			Metal.Color = Color::BarMetalHi();
			Metal.GradientTo = Color::BarMetalLow();
			Metal.GradientStart = FVector2D(FrameCenter, FrameCenter - FrameBezelOuter);
			Metal.GradientEnd = FVector2D(FrameCenter, FrameCenter + FrameBezelOuter);
			Result.Layers.Add(Metal);
			Result.Layers.Add(FrameRing(FrameBezelOuter, 1.8f, Color::Ink()));
			Result.Layers.Add(FrameRing(FrameMapRadius, 1.6f, Color::Ink()));
			Result.Layers.Add(FrameRing(FrameMapRadius - 1.4f, 1.f, Color::Hairline(), 0.75f));
			return Result;
		}();
		return Picture;
	}

	/**
	 * The bezel's ticks, drawn with north up (the ring turns with the view): every 30 degrees, the quarters long and right
	 * across the bezel. North has none: the N's disc sits there.
	 */
	const FPaintedIcon& TickPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(FrameBox);
			FPaintLayer Short;
			Short.StrokeWidth = 1.4f;
			FPaintLayer Long;
			Long.StrokeWidth = 2.2f;
			for (int32 Degrees = 30; Degrees < 360; Degrees += 30)
			{
				// Ends pulled in by their round caps, so the short ticks stay inside the bezel and the long ones in its ink.
				const bool bQuarter = Degrees % 90 == 0;
				FPaintLayer& Layer = bQuarter ? Long : Short;
				const float Inner = bQuarter ? FrameMapRadius + 0.5f : 88.5f;
				const float Outer = bQuarter ? FrameBezelOuter - 0.5f : 93.5f;
				Layer.Strokes.Add(TArray<FVector2D>{ FramePoint(Degrees, Inner), FramePoint(Degrees, Outer) });
			}
			for (FPaintLayer* Layer : { &Short, &Long })
			{
				Layer->Color = Color::Text();
				Layer->Opacity = 0.8f;
				Result.Layers.Add(*Layer);
			}
			return Result;
		}();
		return Picture;
	}

	/** The N's disc: ink with an orange ring; the letter goes over it. */
	const FPaintedIcon& BadgePicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(BadgeBox);
			const FVector2D Center(BadgeBox * 0.5f);
			FPaintLayer Disc;
			Disc.Fills.Add(FrameCircle(Center, BadgeRadius, 48, false));
			Disc.Color = Color::Ink();
			Result.Layers.Add(Disc);
			FPaintLayer Ring;
			Ring.Strokes.Add(FrameCircle(Center, BadgeRadius, 48, true));
			Ring.StrokeWidth = 1.6f;
			Ring.Color = Color::Accent();
			Result.Layers.Add(Ring);
			return Result;
		}();
		return Picture;
	}

	/** The notch: an orange triangle with an ink edge, pointing down at the map, the way you face. */
	const FPaintedIcon& NotchPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(NotchWidth, NotchHeight);
			const TArray<FVector2D> Triangle = { FVector2D(2.f, 1.5f), FVector2D(16.f, 1.5f), FVector2D(9.f, 11.f) };
			FPaintLayer Fill;
			Fill.Fills.Add(Triangle);
			Fill.Color = Color::Accent();
			Result.Layers.Add(Fill);
			FPaintLayer Edge;
			Edge.Strokes.Add(TArray<FVector2D>{ Triangle[0], Triangle[1], Triangle[2], Triangle[0] });
			Edge.StrokeWidth = 1.4f;
			Edge.Color = Color::Ink();
			Result.Layers.Add(Edge);
			return Result;
		}();
		return Picture;
	}
}

void UHudMinimapWidget::BuildFrame(UCanvasPanel* Layer)
{
	if (!WidgetTree || !Layer)
	{
		return;
	}
	// Each piece is centred on its spot; ScaleFrame sizes and places them, TurnFrame turns them. The pictures are drawn
	// once for the session.
	auto AddPiece = [Layer](UWidget* Piece, const FVector2D& Alignment)
	{
		UCanvasPanelSlot* PieceSlot = Layer->AddChildToCanvas(Piece);
		PieceSlot->SetAlignment(Alignment);
		return PieceSlot;
	};
	Bezel = MakeImage(WidgetTree, PaintedIconBrush(TEXT("MinimapBezel"), BezelPicture(), FramePixelsPerUnit, FVector2D(FrameBox)));
	AddPiece(Bezel, FVector2D(0.5f, 0.5f));
	Ticks = MakeImage(WidgetTree, PaintedIconBrush(TEXT("MinimapTicks"), TickPicture(), FramePixelsPerUnit, FVector2D(FrameBox)));
	Ticks->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	AddPiece(Ticks, FVector2D(0.5f, 0.5f));

	// The N on its disc, upright as it circles.
	UOverlay* Badge = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Badge->SetVisibility(ESlateVisibility::HitTestInvisible);
	FillOverlaySlot(Badge->AddChildToOverlay(
		MakeImage(WidgetTree, PaintedIconBrush(TEXT("MinimapNorthDisc"), BadgePicture(), FramePixelsPerUnit, FVector2D(BadgeBox)))));
	North = MakeFloatingText(WidgetTree, BadgeTextSize, Color::Accent(), 0, ETextJustify::Center);
	North->SetText(FText::FromString(TEXT("N")));
	UOverlaySlot* LetterSlot = Badge->AddChildToOverlay(North);
	LetterSlot->SetHorizontalAlignment(HAlign_Center);
	LetterSlot->SetVerticalAlignment(VAlign_Center);
	NorthBadge = Badge;
	AddPiece(Badge, FVector2D(0.5f, 0.5f));

	// The way you face, fixed at the top; over the N as it passes.
	Notch = MakeImage(WidgetTree, PaintedIconBrush(TEXT("MinimapNotch"), NotchPicture(), FramePixelsPerUnit, FVector2D(NotchWidth, NotchHeight)));
	AddPiece(Notch, FVector2D(0.5f, 0.5f));

	// Under the bezel, centred: the place over the area, in floating outlined type.
	UVerticalBox* Lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Lines->SetVisibility(ESlateVisibility::HitTestInvisible);
	PlaceLabel = MakeFloatingText(WidgetTree, PlaceTextSize, Color::Text(), PlaceSpacing, ETextJustify::Center);
	Lines->AddChildToVerticalBox(PlaceLabel)->SetHorizontalAlignment(HAlign_Center);
	AreaLabel = MakeFloatingText(WidgetTree, FMath::FloorToInt32(AreaTextSize), Color::TextDim(), AreaSpacing, ETextJustify::Center);
	// The kit's sizes are whole; this one sits between two of them.
	FSlateFontInfo AreaFont = AreaLabel->GetFont();
	AreaFont.Size = AreaTextSize;
	AreaLabel->SetFont(AreaFont);
	Lines->AddChildToVerticalBox(AreaLabel)->SetHorizontalAlignment(HAlign_Center);
	PlaceBlock = Lines;
	AddPiece(Lines, FVector2D(0.5f, 0.f))->SetAutoSize(true);
	ShowPlace(FText::GetEmpty(), FText::GetEmpty());

	ScaleFrame();
}

void UHudMinimapWidget::ScaleFrame()
{
	const float Radius = GetDiameter() * 0.5f;
	const FVector2D Center(Radius);
	// The N's disc and the notch grow like the markers (more gently than the map), so they stay readable on a small map.
	const float MarkerScale = FMath::Sqrt(Scale);
	auto SetPiece = [](UWidget* Piece, const FVector2D& Position, const FVector2D& Size)
	{
		if (UCanvasPanelSlot* PieceSlot = Piece ? Cast<UCanvasPanelSlot>(Piece->Slot) : nullptr)
		{
			PieceSlot->SetPosition(Position);
			PieceSlot->SetSize(Size);
		}
	};
	SetPiece(Bezel, Center, FVector2D(FrameBox * Scale));
	SetPiece(Ticks, Center, FVector2D(FrameBox * Scale));
	SetPiece(Notch, Center - FVector2D(0.f, NotchCenter * Scale), FVector2D(NotchWidth, NotchHeight) * MarkerScale);
	// The disc moves every frame (TurnFrame); only its size changes here.
	if (UCanvasPanelSlot* BadgeSlot = NorthBadge ? Cast<UCanvasPanelSlot>(NorthBadge->Slot) : nullptr)
	{
		BadgeSlot->SetSize(FVector2D(BadgeBox * MarkerScale));
	}
	if (North)
	{
		StyleFloatingText(North, FMath::Max(1, FMath::RoundToInt32(BadgeTextSize * MarkerScale)), Color::Accent(), 0, ETextJustify::Center);
	}
	if (UCanvasPanelSlot* LinesSlot = PlaceBlock ? Cast<UCanvasPanelSlot>(PlaceBlock->Slot) : nullptr)
	{
		LinesSlot->SetPosition(FVector2D(Radius, Radius + FrameBezelOuter * Scale + PlaceGap));
	}
}

void UHudMinimapWidget::TurnFrame(float ViewYaw)
{
	// The map is drawn north up and turned back by the view's yaw (NativeTick), so the ticks turn the same way and the N
	// rides the bezel's middle toward the north, as the needle does toward a waypoint.
	if (Ticks)
	{
		Ticks->SetRenderTransformAngle(-ViewYaw);
	}
	if (UCanvasPanelSlot* BadgeSlot = NorthBadge ? Cast<UCanvasPanelSlot>(NorthBadge->Slot) : nullptr)
	{
		const float Radius = GetDiameter() * 0.5f;
		const FVector2D Toward = UMinimapSubsystem::ViewOffset(FVector::ForwardVector, ViewYaw, 1.f).GetSafeNormal();
		BadgeSlot->SetPosition(FVector2D(Radius) + Toward * (FrameBezelMiddle * Scale));
	}
}

void UHudMinimapWidget::UpdatePlace()
{
	const UWorld* World = GetWorld();
	if (!World || !PlaceLabel)
	{
		return;
	}
	// The area is found as play begins (UAreaRulesSubsystem); a level that is no area's goes by its file's name. The game
	// names no smaller places yet: when it does (a town's square, a landmark), the place goes on the big line and the
	// area under it, through ShowPlace.
	const UAreaRulesSubsystem* Rules = World->GetSubsystem<UAreaRulesSubsystem>();
	const UAreaDefinition* Area = Rules ? Rules->GetArea() : nullptr;
	const FText AreaName = Area && !Area->DisplayName.IsEmpty() ? Area->DisplayName
		: FText::FromString(USessionSubsystem::PlaceName(USessionSubsystem::MapOf(World)));
	ShowPlace(FText::GetEmpty(), AreaName);
	bPlaceKnown = true;
}

void UHudMinimapWidget::ShowPlace(const FText& Place, const FText& Area)
{
	if (!PlaceLabel || !AreaLabel)
	{
		return;
	}
	// With no place of its own, the area takes the big line and the small one folds away.
	const bool bHasPlace = !Place.IsEmpty();
	const FText BigLine = (bHasPlace ? Place : Area).ToUpper();
	PlaceLabel->SetText(BigLine);
	PlaceLabel->SetVisibility(BigLine.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	const bool bShowArea = bHasPlace && !Area.IsEmpty();
	AreaLabel->SetText(bShowArea ? Area.ToUpper() : FText::GetEmpty());
	AreaLabel->SetVisibility(bShowArea ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
