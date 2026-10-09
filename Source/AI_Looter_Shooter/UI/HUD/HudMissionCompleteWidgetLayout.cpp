// UHudMissionCompleteWidget's pictures and widgets: the ranger's star medal with its rays, the title between its rules, the
// mission's name and the rewards row. What it shows and when is HudMissionCompleteWidget.cpp's.

#include "UI/HUD/HudMissionCompleteWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	/**
	 * The medal's pictures, in their own box, its centre this far over the box's middle (at 1080p the rays' tips stay under
	 * the boss bar's cluster, which ends 126 px down); drawn at 2 texels a unit.
	 */
	constexpr float MedalArt = 80.f;
	constexpr float MedalDrop = -72.f;
	constexpr float CompletePixelsPerUnit = 2.f;
	/** The tracker's medal (HudMissionTrackerWidgetLayout.cpp) at 2.25 times its size: ink rim, gunmetal ring, glass, star. */
	constexpr float MedalRim = 36.f;
	constexpr float MedalRing = 32.8f;
	constexpr float MedalGlass = 27.f;
	constexpr float StarOuter = 23.4f;
	constexpr float StarInner = 10.1f;
	/** The rays behind it: four long and four short, orange, turning slowly while it shows. */
	constexpr float RaysArt = 200.f;
	constexpr float RayFrom = 42.f;
	constexpr float LongRayTo = 64.f;
	constexpr float ShortRayTo = 54.f;
	constexpr float RayWidth = 2.5f;
	constexpr float RayOpacity = 0.75f;

	/** A warm glow behind the title, so it lifts off a bright sky without a panel. */
	constexpr float GlowWidth = 640.f;
	constexpr float GlowHeight = 150.f;
	constexpr float GlowOpacity = 0.16f;

	/** "MISSION COMPLETE": orange, letter-spaced, its middle this far under the box's middle; its rules either side. */
	constexpr int32 TitleSize = 40;
	constexpr int32 TitleSpacing = 240;
	constexpr float TitleDrop = 2.f;
	constexpr float RuleLength = 90.f;
	constexpr float RuleThickness = 2.f;
	constexpr float RuleDiamond = 3.5f;
	constexpr float RuleArtHeight = 8.f;
	constexpr float RuleGap = 18.f;

	/** The mission's name, and the rewards row under it. */
	constexpr int32 NameSize = 22;
	constexpr int32 NameSpacing = 120;
	constexpr float NameDrop = 52.f;
	constexpr float RewardsDrop = 94.f;

	FVector2D MedalAt(float X, float Y)
	{
		return FVector2D(MedalArt * 0.5f + X, MedalArt * 0.5f + Y);
	}

	TArray<FVector2D> Circle(const FVector2D& Center, float Radius)
	{
		TArray<FVector2D> Points;
		constexpr int32 Segments = 64;
		for (int32 Index = 0; Index < Segments; ++Index)
		{
			const float Angle = 2.f * PI * static_cast<float>(Index) / static_cast<float>(Segments);
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Points;
	}

	TArray<FVector2D> Closed(TArray<FVector2D> Points)
	{
		if (!Points.IsEmpty())
		{
			// A copy of the first point: adding a reference to the array's own element could read it after a reallocation.
			const FVector2D First = Points[0];
			Points.Add(First);
		}
		return Points;
	}

	FPaintLayer Fill(TArray<TArray<FVector2D>> Shapes, const FLinearColor& LayerColor, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Fills = MoveTemp(Shapes);
		Layer.Color = LayerColor;
		Layer.Opacity = Opacity;
		return Layer;
	}

	FPaintLayer Stroke(TArray<TArray<FVector2D>> Lines, float Width, const FLinearColor& LayerColor, float Opacity = 1.f)
	{
		FPaintLayer Layer;
		Layer.Strokes = MoveTemp(Lines);
		Layer.StrokeWidth = Width;
		Layer.Color = LayerColor;
		Layer.Opacity = Opacity;
		return Layer;
	}

	/** The rays: four long ones on the axes, four short ones between, from a little outside the medal. */
	const FPaintedIcon& RaysPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(RaysArt);
			const FVector2D Center(RaysArt * 0.5f);
			TArray<TArray<FVector2D>> Rays;
			for (int32 Index = 0; Index < 8; ++Index)
			{
				const float Angle = FMath::DegreesToRadians(-90.f + 45.f * static_cast<float>(Index));
				const FVector2D Way(FMath::Cos(Angle), FMath::Sin(Angle));
				Rays.Add({ Center + Way * RayFrom, Center + Way * (Index % 2 == 0 ? LongRayTo : ShortRayTo) });
			}
			Result.Layers.Add(Stroke(MoveTemp(Rays), RayWidth, Color::Accent(), RayOpacity));
			return Result;
		}();
		return Picture;
	}

	/** The medal's metal: an ink rim and a gunmetal ring lit from the top, open where the glass is. */
	const FPaintedIcon& MedalMetalPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(MedalArt);
			const FVector2D Center = MedalAt(0.f, 0.f);
			Result.Layers.Add(Fill({ Circle(Center, MedalRim), Circle(Center, MedalGlass) }, Color::Ink()));
			FPaintLayer Ring = Fill({ Circle(Center, MedalRing), Circle(Center, MedalGlass) }, Color::BarMetalHi());
			Ring.GradientTo = Color::BarMetalLow();
			Ring.GradientStart = Center - FVector2D(0.f, MedalRing);
			Ring.GradientEnd = Center + FVector2D(0.f, MedalRing);
			Result.Layers.Add(Ring);
			return Result;
		}();
		return Picture;
	}

	/** The dark glass inside the ring: the banner's one background. */
	const FPaintedIcon& MedalGlassPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(MedalArt);
			Result.Layers.Add(Fill({ Circle(MedalAt(0.f, 0.f), MedalGlass) }, Color::ScreenBg().CopyWithNewOpacity(0.9f)));
			return Result;
		}();
		return Picture;
	}

	/** The cyan hairline inside the ring and the ranger's star: orange in a heavy ink edge, lit along its upper left. */
	const FPaintedIcon& MedalStarPicture()
	{
		static const FPaintedIcon Picture = []
		{
			FPaintedIcon Result;
			Result.ViewBox = FVector2D(MedalArt);
			const FVector2D Center = MedalAt(0.f, 0.f);
			Result.Layers.Add(Stroke({ Closed(Circle(Center, MedalGlass + 0.9f)) }, 1.6f, Color::Hairline(), 0.6f));
			TArray<FVector2D> Star;
			for (int32 Index = 0; Index < 10; ++Index)
			{
				const float Angle = FMath::DegreesToRadians(-90.f + 36.f * static_cast<float>(Index));
				Star.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (Index % 2 == 0 ? StarOuter : StarInner));
			}
			Result.Layers.Add(Fill({ Star }, Color::Accent()));
			Result.Layers.Add(Stroke({ Closed(Star) }, 3.2f, Color::Ink()));
			Result.Layers.Add(Stroke({ { Center + FVector2D(-17.1f, -5.9f), Center + FVector2D(-4.7f, -6.5f), Center + FVector2D(0.f, -18.2f) } },
				2.f, FLinearColor::White, 0.55f));
			return Result;
		}();
		return Picture;
	}

	/**
	 * A cyan rule fading out away from the title, with a small orange diamond at its end by the title: bAfter is the one
	 * after it (its diamond on the left, fading toward the right).
	 */
	const FPaintedIcon& RulePicture(bool bAfter)
	{
		auto Make = [](bool bRight)
		{
			FPaintedIcon Result;
			const float Total = RuleLength + RuleDiamond * 2.f + 2.f;
			Result.ViewBox = FVector2D(Total, RuleArtHeight);
			const float MidY = RuleArtHeight * 0.5f;
			const float LineFrom = bRight ? RuleDiamond * 2.f + 2.f : 0.f;
			const float LineTo = LineFrom + RuleLength;
			FPaintLayer Line = Fill({ { FVector2D(LineFrom, MidY - RuleThickness * 0.5f), FVector2D(LineTo, MidY - RuleThickness * 0.5f),
				FVector2D(LineTo, MidY + RuleThickness * 0.5f), FVector2D(LineFrom, MidY + RuleThickness * 0.5f) } },
				Color::Hairline().CopyWithNewOpacity(bRight ? 1.f : 0.f));
			Line.GradientTo = Color::Hairline().CopyWithNewOpacity(bRight ? 0.f : 1.f);
			Line.GradientStart = FVector2D(LineFrom, MidY);
			Line.GradientEnd = FVector2D(LineTo, MidY);
			Result.Layers.Add(MoveTemp(Line));
			const float DiamondX = bRight ? RuleDiamond : Total - RuleDiamond;
			Result.Layers.Add(Fill({ { FVector2D(DiamondX, MidY - RuleDiamond), FVector2D(DiamondX + RuleDiamond, MidY),
				FVector2D(DiamondX, MidY + RuleDiamond), FVector2D(DiamondX - RuleDiamond, MidY) } }, Color::Accent()));
			return Result;
		};
		static const FPaintedIcon Before = Make(false);
		static const FPaintedIcon After = Make(true);
		return bAfter ? After : Before;
	}
}

void UHudMissionCompleteWidget::BuildTree()
{
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	const FVector2D Centre(Width * 0.5f, Height * 0.5f);
	// Everything by its middle, on the box's centre line.
	auto Place = [Canvas](UWidget* Child, const FVector2D& Position)
	{
		UCanvasPanelSlot* ChildSlot = Canvas->AddChildToCanvas(Child);
		ChildSlot->SetAutoSize(true);
		ChildSlot->SetPosition(Position);
		ChildSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	};

	// Back to front: the glow, the rays, the medal (metal, glass, star), the title, the name, the rewards.
	Place(MakeImage(WidgetTree, GlowBrush(FVector2D(GlowWidth, GlowHeight), Color::Accent().CopyWithNewOpacity(GlowOpacity))),
		Centre + FVector2D(0.f, TitleDrop));
	const FVector2D MedalCentre = Centre + FVector2D(0.f, MedalDrop);
	UImage* RaysImage = MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudCompleteRays"), RaysPicture(), CompletePixelsPerUnit, FVector2D(RaysArt)));
	Place(RaysImage, MedalCentre);
	Rays = RaysImage;
	const FVector2D MedalSize(MedalArt);
	Place(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudCompleteMedalMetal"), MedalMetalPicture(), CompletePixelsPerUnit, MedalSize)), MedalCentre);
	UImage* Glass = MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudCompleteMedalGlass"), MedalGlassPicture(), CompletePixelsPerUnit, MedalSize));
	MarkBackground(Glass);
	Place(Glass, MedalCentre);
	Place(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudCompleteMedalStar"), MedalStarPicture(), CompletePixelsPerUnit, MedalSize)), MedalCentre);

	UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	const FVector2D RuleSize(RuleLength + RuleDiamond * 2.f + 2.f, RuleArtHeight);
	TitleRow->AddChildToHorizontalBox(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudCompleteRuleIn"), RulePicture(false),
		CompletePixelsPerUnit, RuleSize)))->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Words = MakeFloatingText(WidgetTree, TitleSize, Color::Accent(), TitleSpacing, ETextJustify::Center);
	Words->SetText(NSLOCTEXT("LooterHUD", "MissionComplete", "MISSION COMPLETE"));
	UHorizontalBoxSlot* WordsSlot = TitleRow->AddChildToHorizontalBox(Words);
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(RuleGap, 0.f));
	TitleRow->AddChildToHorizontalBox(MakeImage(WidgetTree, PaintedIconBrush(TEXT("HudCompleteRuleOut"), RulePicture(true),
		CompletePixelsPerUnit, RuleSize)))->SetVerticalAlignment(VAlign_Center);
	Place(TitleRow, Centre + FVector2D(0.f, TitleDrop));
	Title = TitleRow;

	NameText = MakeFloatingText(WidgetTree, NameSize, FLinearColor::White, NameSpacing, ETextJustify::Center);
	Place(NameText, Centre + FVector2D(0.f, NameDrop));

	RewardsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Place(RewardsRow, Centre + FVector2D(0.f, RewardsDrop));

	USizeBox* Root = MakeSized(WidgetTree, Canvas, Width, Height);
	// It pops from its middle.
	Root->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	WidgetTree->RootWidget = Root;
	Banner = Root;
}
