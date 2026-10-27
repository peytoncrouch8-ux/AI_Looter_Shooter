#include "UI/Menus/MainMenuArt.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

namespace
{
	TArray<FVector2D> RectFill(float Left, float Top, float Right, float Bottom)
	{
		return { FVector2D(Left, Top), FVector2D(Right, Top), FVector2D(Right, Bottom), FVector2D(Left, Bottom) };
	}

	/** Points along a circle from one angle to another (degrees, y down: 90 is straight down), every few degrees. */
	TArray<FVector2D> Arc(const FVector2D& Centre, float Radius, float FromDegrees, float ToDegrees)
	{
		TArray<FVector2D> Points;
		constexpr float Step = 5.f;
		const int32 Steps = FMath::Max(1, FMath::CeilToInt((ToDegrees - FromDegrees) / Step));
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(FromDegrees, ToDegrees, float(Index) / Steps));
			Points.Add(Centre + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
		}
		return Points;
	}
}

const FPaintedIcon& MainMenuArt::TitleMark()
{
	static const FPaintedIcon Picture = []
	{
		FPaintedIcon Result;
		Result.ViewBox = FVector2D(64.f, 64.f);
		const FVector2D Centre(32.f, 32.f);

		// The horizon, out of the ring both ways.
		FPaintLayer Horizon;
		Horizon.Strokes.Add({ FVector2D(1.f, 32.f), FVector2D(11.f, 32.f) });
		Horizon.Strokes.Add({ FVector2D(53.f, 32.f), FVector2D(63.f, 32.f) });
		Horizon.StrokeWidth = 1.2f;
		Horizon.Color = Color::Hairline();
		Horizon.Opacity = 0.85f;
		Result.Layers.Add(Horizon);

		// The coin's ring, broken top and bottom where the ember passes through it.
		FPaintLayer Ring;
		Ring.Strokes.Add(Arc(Centre, 19.f, 115.f, 245.f));
		Ring.Strokes.Add(Arc(Centre, 19.f, -65.f, 65.f));
		Ring.StrokeWidth = 2.8f;
		Ring.Color = Color::Accent();
		Result.Layers.Add(Ring);

		// The ember: a tall lozenge, hollowed by a slit (listed together they fill even-odd), lit at its top.
		FPaintLayer Ember;
		Ember.Fills.Add({ FVector2D(32.f, 3.f), FVector2D(38.5f, 20.f), FVector2D(36.5f, 36.f), FVector2D(32.f, 61.f),
			FVector2D(27.5f, 36.f), FVector2D(25.5f, 20.f) });
		Ember.Fills.Add({ FVector2D(32.f, 15.f), FVector2D(34.6f, 26.f), FVector2D(32.f, 44.f), FVector2D(29.4f, 26.f) });
		Ember.Color = Color::AccentLight();
		Ember.GradientTo = Color::Accent();
		Ember.GradientStart = FVector2D(32.f, 3.f);
		Ember.GradientEnd = FVector2D(32.f, 50.f);
		Result.Layers.Add(Ember);
		return Result;
	}();
	return Picture;
}

const FPaintedIcon& MainMenuArt::Shade()
{
	static const FPaintedIcon Picture = []
	{
		FPaintedIcon Result;
		Result.ViewBox = FVector2D(64.f, 4.f);
		// Solid under the menu's words, then thinning out past them.
		constexpr float HoldTo = 30.f;
		FPaintLayer Solid;
		Solid.Fills.Add(RectFill(0.f, 0.f, HoldTo, 4.f));
		Solid.Color = Color::Backdrop();
		Result.Layers.Add(Solid);
		FPaintLayer Thin;
		Thin.Fills.Add(RectFill(HoldTo, 0.f, 64.f, 4.f));
		Thin.Color = Color::Backdrop();
		Thin.GradientTo = Color::Backdrop().CopyWithNewOpacity(0.f);
		Thin.GradientStart = FVector2D(HoldTo, 0.f);
		Thin.GradientEnd = FVector2D(64.f, 0.f);
		Result.Layers.Add(Thin);
		return Result;
	}();
	return Picture;
}

const FPaintedIcon& MainMenuArt::RowWash()
{
	static const FPaintedIcon Picture = []
	{
		FPaintedIcon Result;
		Result.ViewBox = FVector2D(32.f, 4.f);
		FPaintLayer Wash;
		Wash.Fills.Add(RectFill(0.f, 0.f, 32.f, 4.f));
		Wash.Color = Color::FocusWash();
		Wash.GradientTo = Color::FocusWash().CopyWithNewOpacity(0.f);
		Wash.GradientStart = FVector2D(0.f, 0.f);
		Wash.GradientEnd = FVector2D(32.f, 0.f);
		Result.Layers.Add(Wash);
		return Result;
	}();
	return Picture;
}

UWidget* MainMenuArt::MakePageHeader(UWidgetTree* Tree, const FString& Title)
{
	UHorizontalBox* Header = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Header->AddChildToHorizontalBox(MakeSized(Tree, MakeImage(Tree, RectBrush(Color::Accent())), 3.f, 18.f))->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* WordsSlot = Header->AddChildToHorizontalBox(MakeText(Tree, Title, 15, Color::Title(), true, 400));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	UHorizontalBoxSlot* RuleSlot = Header->AddChildToHorizontalBox(MakeSized(Tree, MakeImage(Tree, RectBrush(Color::ScreenLine())), 0.f, 1.f));
	RuleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RuleSlot->SetVerticalAlignment(VAlign_Center);
	RuleSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	return Header;
}
