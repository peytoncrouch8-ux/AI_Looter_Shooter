#include "UI/Menus/MainMenuArt.h"
#include "UI/Cards/LooterHollowKey.h"
#include "UI/Style/LooterReticleArt.h"
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
	TArray<FVector2D> Arc(const FVector2D& Centre, float Radius, float FromDegrees, float ToDegrees, float Step = 5.f)
	{
		TArray<FVector2D> Points;
		const int32 Steps = FMath::Max(1, FMath::CeilToInt((ToDegrees - FromDegrees) / Step));
		for (int32 Index = 0; Index <= Steps; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(FromDegrees, ToDegrees, float(Index) / Steps));
			Points.Add(Centre + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
		}
		return Points;
	}

	/** A line with the kit's dark edge under it: the edge layer, then the line's. */
	void AddEdgedLine(FPaintedIcon& Icon, const TArray<TArray<FVector2D>>& Strokes, float Width, const FLinearColor& Tint, float Opacity)
	{
		FPaintLayer Edge;
		Edge.Strokes = Strokes;
		Edge.StrokeWidth = Width + 3.f;
		Edge.Color = Color::Outline();
		Edge.Opacity = 0.7f * Opacity;
		Icon.Layers.Add(Edge);
		FPaintLayer Line;
		Line.Strokes = Strokes;
		Line.StrokeWidth = Width;
		Line.Color = Tint;
		Line.Opacity = Opacity;
		Icon.Layers.Add(Line);
	}

	/** The gold run's margin round its arc: half its width, its edge, and the pad. */
	constexpr float LitRunWidth = 5.f;
	constexpr float LitRunMargin = LitRunWidth * 0.5f + 1.5f + LooterReticle::Pad;
}

LooterUI::FPaintedIcon MainMenuArt::LogoMark()
{
	// The user's pick (2026-10-29, "6 Spur Rowel" of Docs/Work/2026-10-29-UI3Build/logos/logos2.png, points from marks2.json):
	// a drifter's spur, ten sharp points round a hub that still glows. Mark space is centred on 0,0 at the mockup's real
	// size (the ring's radius 34); the view box is 44 wide so the points reach the ring as in the mockup.
	FPaintedIcon Result;
	Result.ViewBox = FVector2D(44.f, 44.f);
	auto V = [](float X, float Y) { return FVector2D(X + 22.f, Y + 22.f); };

	// The rowel in salt, the ring round its hub cut out (polygons in one layer fill even-odd).
	FPaintLayer Rowel;
	Rowel.Fills.Add({ V(-1.77f, -8.31f), V(0.f, -21.f), V(1.77f, -8.31f), V(3.46f, -7.77f), V(12.34f, -16.99f), V(6.32f, -5.69f), V(7.36f, -4.25f), V(19.97f, -6.49f), V(8.45f, -0.89f), V(8.45f, 0.89f), V(19.97f, 6.49f), V(7.36f, 4.25f), V(6.32f, 5.69f), V(12.34f, 16.99f), V(3.46f, 7.77f), V(1.77f, 8.31f), V(0.f, 21.f), V(-1.77f, 8.31f), V(-3.46f, 7.77f), V(-12.34f, 16.99f), V(-6.32f, 5.69f), V(-7.36f, 4.25f), V(-19.97f, 6.49f), V(-8.45f, 0.89f), V(-8.45f, -0.89f), V(-19.97f, -6.49f), V(-7.36f, -4.25f), V(-6.32f, -5.69f), V(-12.34f, -16.99f), V(-3.46f, -7.77f) });
	Rowel.Fills.Add({ V(6.2f, 0.f), V(5.83f, 2.12f), V(4.75f, 3.99f), V(3.1f, 5.37f), V(1.08f, 6.11f), V(-1.08f, 6.11f), V(-3.1f, 5.37f), V(-4.75f, 3.99f), V(-5.83f, 2.12f), V(-6.2f, 0.f), V(-5.83f, -2.12f), V(-4.75f, -3.99f), V(-3.1f, -5.37f), V(-1.08f, -6.11f), V(1.08f, -6.11f), V(3.1f, -5.37f), V(4.75f, -3.99f), V(5.83f, -2.12f) });
	Rowel.Color = Color::SaltLine();
	Result.Layers.Add(Rowel);

	// The hub, glowing gold in the cut, with its axle hole.
	FPaintLayer Hub;
	Hub.Fills.Add({ V(4.2f, 0.f), V(3.95f, 1.44f), V(3.22f, 2.7f), V(2.1f, 3.64f), V(0.73f, 4.14f), V(-0.73f, 4.14f), V(-2.1f, 3.64f), V(-3.22f, 2.7f), V(-3.95f, 1.44f), V(-4.2f, 0.f), V(-3.95f, -1.44f), V(-3.22f, -2.7f), V(-2.1f, -3.64f), V(-0.73f, -4.14f), V(0.73f, -4.14f), V(2.1f, -3.64f), V(3.22f, -2.7f), V(3.95f, -1.44f) });
	Hub.Fills.Add({ V(1.4f, 0.f), V(0.99f, 0.99f), V(0.f, 1.4f), V(-0.99f, 0.99f), V(-1.4f, 0.f), V(-0.99f, -0.99f), V(0.f, -1.4f), V(0.99f, -0.99f) });
	Hub.Color = Color::SoulGold();
	Result.Layers.Add(Hub);
	return Result;
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

LooterUI::FPaintedIcon MainMenuArt::TagOutline(float Width, float Height)
{
	// c3's cham(x, y, 66, 24, 5): every corner cut; the kit's tags cut two, and so does this one (top-left, bottom-right),
	// so it reads as the kit's.
	const float P = LooterReticle::Pad;
	const float Cut = 5.f * Layout::Grow;
	const TArray<FVector2D> Outline = { FVector2D(P + Cut, P), FVector2D(P + Width, P), FVector2D(P + Width, P + Height - Cut),
		FVector2D(P + Width - Cut, P + Height), FVector2D(P, P + Height), FVector2D(P, P + Cut), FVector2D(P + Cut, P) };
	FPaintedIcon Result;
	Result.ViewBox = FVector2D(Width + P * 2.f, Height + P * 2.f);
	AddEdgedLine(Result, { Outline }, 2.f, Color::SaltLine(), 0.7f);
	return Result;
}

FVector2D MainMenuArt::LitRunCentre(float Radius, float HalfDegrees)
{
	// Drawn round 180 degrees: its leftmost point is the circle's left edge, its ends HalfDegrees above and below.
	const float Rise = Radius * FMath::Sin(FMath::DegreesToRadians(HalfDegrees));
	return FVector2D(Radius + LitRunMargin, Rise + LitRunMargin);
}

LooterUI::FPaintedIcon MainMenuArt::LitRun(float Radius, float HalfDegrees)
{
	const FVector2D Centre = LitRunCentre(Radius, HalfDegrees);
	const float Bow = Radius * (1.f - FMath::Cos(FMath::DegreesToRadians(HalfDegrees)));
	FPaintedIcon Result;
	Result.ViewBox = FVector2D(Bow + LitRunMargin * 2.f, (Centre.Y - LitRunMargin) * 2.f + LitRunMargin * 2.f);
	AddEdgedLine(Result, { Arc(Centre, Radius, 180.f - HalfDegrees, 180.f + HalfDegrees, 0.5f) }, LitRunWidth, Color::SoulGold(), 1.f);
	return Result;
}

UWidget* MainMenuArt::MakePageHeader(UWidgetTree* Tree, const FString& Title)
{
	UHorizontalBox* Header = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Header->AddChildToHorizontalBox(MakeSized(Tree, MakeImage(Tree, RectBrush(Color::SoulGold())), 3.f, 18.f))->SetVerticalAlignment(VAlign_Center);
	UHorizontalBoxSlot* WordsSlot = Header->AddChildToHorizontalBox(MakeText(Tree, Title, 15, Color::SlateText(), true, 400));
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	UHorizontalBoxSlot* RuleSlot = Header->AddChildToHorizontalBox(MakeSized(Tree, MakeImage(Tree, RectBrush(Color::ScreenLine())), 0.f, 1.f));
	RuleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RuleSlot->SetVerticalAlignment(VAlign_Center);
	RuleSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	return Header;
}

UWidget* MainMenuArt::MakeKeyHint(UWidgetTree* Tree, const FString& Key, const FString& Text)
{
	// c3's key: a hollow gold cap (the kit's ULooterHollowKey), then its words, 18 px spaced 2, floating (no plate).
	UHorizontalBox* Hint = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	ULooterHollowKey* Cap = Tree->ConstructWidget<ULooterHollowKey>(ULooterHollowKey::StaticClass());
	Cap->SetLabel(Key.ToUpper());
	Hint->AddChildToHorizontalBox(Cap)->SetVerticalAlignment(VAlign_Center);
	UTextBlock* Words = MakeFloatingText(Tree, 18, Color::SlateText(), 111);
	Words->SetText(FText::FromString(Text.ToUpper()));
	UHorizontalBoxSlot* WordsSlot = Hint->AddChildToHorizontalBox(Words);
	WordsSlot->SetVerticalAlignment(VAlign_Center);
	WordsSlot->SetPadding(FMargin(10.f * Layout::Grow, 0.f, 0.f, 0.f));
	return Hint;
}
