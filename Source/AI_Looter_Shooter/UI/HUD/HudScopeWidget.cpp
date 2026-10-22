// UHudScopeWidget: the sniper's scope picture over the zoomed view (the Longhorn's: AWeaponBase::UpdateScope shows it).

#include "UI/HUD/HudScopeWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateLayoutTransform.h"
#include "Styling/WidgetStyle.h"

using namespace LooterUI;

namespace
{
	/** The mask's picture: a 64 x 64 square of ink with a round hole (radius LensUnits) and its metal rim. */
	constexpr float MaskBox = 64.f;
	constexpr float LensUnits = 30.5f;
	constexpr float RimUnits = 0.9f;
	constexpr float MaskPixelsPerUnit = 24.f;

	/** The cross: its gap round the dot and where its heavy posts start (shares of the lens's radius), its lines (px at 1080). */
	constexpr float CrossGap = 0.035f;
	constexpr float PostFrom = 0.55f;
	constexpr float FineWidth = 1.5f;
	constexpr float PostWidth = 4.f;
	constexpr float DotSize = 5.f;
	/** The breath bar: under the middle (share of the radius down), its width (share of the radius) and height (px at 1080). */
	constexpr float BreathDown = 0.62f;
	constexpr float BreathWidth = 0.32f;
	constexpr float BreathHeight = 6.f;

	TArray<FVector2D> Circle(const FVector2D& Centre, double Radius, int32 Sides)
	{
		TArray<FVector2D> Points;
		for (int32 Step = 0; Step < Sides; ++Step)
		{
			const double Angle = UE_TWO_PI * Step / Sides;
			Points.Add(Centre + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
		}
		return Points;
	}

	FPaintedIcon MaskPicture()
	{
		FPaintedIcon Picture;
		Picture.ViewBox = FVector2D(MaskBox, MaskBox);
		const FVector2D Middle(MaskBox * 0.5, MaskBox * 0.5);
		FPaintLayer Ink;
		Ink.Color = Color::Ink();
		// Even-odd: the square with the circle in it leaves the lens open.
		Ink.Fills.Add({ FVector2D(-1.0, -1.0), FVector2D(MaskBox + 1.0, -1.0), FVector2D(MaskBox + 1.0, MaskBox + 1.0), FVector2D(-1.0, MaskBox + 1.0) });
		Ink.Fills.Add(Circle(Middle, LensUnits, 128));
		Picture.Layers.Add(MoveTemp(Ink));
		FPaintLayer Rim;
		Rim.Color = Color::MetalLow();
		TArray<FVector2D> Ring = Circle(Middle, LensUnits + RimUnits * 0.5, 128);
		const FVector2D First = Ring[0];
		Ring.Add(First);
		Rim.Strokes.Add(MoveTemp(Ring));
		Rim.StrokeWidth = RimUnits;
		Picture.Layers.Add(MoveTemp(Rim));
		return Picture;
	}

	void Line(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, const FVector2f& From, const FVector2f& To,
		const FLinearColor& Tint, float Width)
	{
		TArray<FVector2f> Points = { From, To };
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Tint, true, Width);
	}

	FLinearColor Faded(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Color.A * Alpha);
	}
}

TSharedRef<SWidget> UHudScopeWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Root;
	}
	if (!bBrushesMade)
	{
		bBrushesMade = true;
		MaskBrush = PaintedIconBrush(TEXT("ScopeLensMask"), MaskPicture(), MaskPixelsPerUnit, FVector2D(MaskBox, MaskBox));
		FillBrush = RectBrush(FLinearColor::White);
	}
	TSharedRef<SWidget> Built = Super::RebuildWidget();
	SetVisibility(Strength > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	return Built;
}

void UHudScopeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// The breath bar and the fade move every frame the scope is up (collapsed otherwise).
	ForceVolatile(true);
}

void UHudScopeWidget::ShowScope(float InStrength, float InBreathLeft, bool bInHolding, bool bInWinded)
{
	Strength = FMath::Clamp(InStrength, 0.f, 1.f);
	BreathLeft = FMath::Clamp(InBreathLeft, 0.f, 1.f);
	bHolding = bInHolding;
	bWinded = bInWinded;
	const ESlateVisibility Wanted = Strength > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (GetVisibility() != Wanted)
	{
		SetVisibility(Wanted);
	}
}

int32 UHudScopeWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Under = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FVector2f Screen(AllottedGeometry.GetLocalSize());
	if (Strength <= 0.f || Screen.X < 2.f || Screen.Y < 2.f)
	{
		return Under;
	}
	const float Alpha = Strength * InWidgetStyle.GetColorAndOpacityTint().A;
	const float Scale = Screen.Y / 1080.f;
	const FVector2f Middle = Screen * 0.5f;
	const float Radius = Screen.Y * LensShare;

	// The lens: the mask's square round the hole, ink beside it to the screen's edges.
	int32 Layer = Under + 1;
	const float Side = Radius * MaskBox / LensUnits;
	const FVector2f Corner = Middle - FVector2f(Side, Side) * 0.5f;
	FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(Side, Side), FSlateLayoutTransform(Corner)),
		&MaskBrush, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, Alpha));
	const FLinearColor Ink = Faded(Color::Ink(), Alpha);
	const float Left = FMath::Max(Corner.X + 1.f, 0.f);
	if (Left > 0.f)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(Left, Screen.Y), FSlateLayoutTransform(FVector2f(0.f, 0.f))),
			&FillBrush, ESlateDrawEffect::None, Ink);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(Left, Screen.Y),
			FSlateLayoutTransform(FVector2f(Screen.X - Left, 0.f))), &FillBrush, ESlateDrawEffect::None, Ink);
	}
	const float Top = FMath::Max(Corner.Y + 1.f, 0.f);
	if (Top > 0.f)
	{
		// A screen taller than the mask (it's a share of the height across): ink above and below it too.
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(Screen.X, Top), FSlateLayoutTransform(FVector2f(0.f, 0.f))),
			&FillBrush, ESlateDrawEffect::None, Ink);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(Screen.X, Top),
			FSlateLayoutTransform(FVector2f(0.f, Screen.Y - Top))), &FillBrush, ESlateDrawEffect::None, Ink);
	}

	// The cross: fine in the middle, heavy posts toward the rim, a gap round the orange dot.
	++Layer;
	const FLinearColor Cross = Faded(Color::CyanText(), Alpha);
	const float Gap = Radius * CrossGap;
	const float Post = Radius * PostFrom;
	const FVector2f Axes[] = { FVector2f(1.f, 0.f), FVector2f(-1.f, 0.f), FVector2f(0.f, 1.f), FVector2f(0.f, -1.f) };
	for (const FVector2f& Axis : Axes)
	{
		Line(OutDrawElements, Layer, AllottedGeometry, Middle + Axis * Gap, Middle + Axis * Post, Cross, FineWidth * Scale);
		// No post at the top: the target stands there.
		if (Axis.Y >= 0.f || Axis.X != 0.f)
		{
			Line(OutDrawElements, Layer, AllottedGeometry, Middle + Axis * Post, Middle + Axis * Radius, Cross, PostWidth * Scale);
		}
	}
	const float Dot = DotSize * Scale;
	FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(Dot, Dot), FSlateLayoutTransform(Middle - FVector2f(Dot, Dot) * 0.5f)),
		&FillBrush, ESlateDrawEffect::None, Faded(Color::Accent(), Alpha));

	// The breath: shown while it's held or coming back.
	if (bHolding || bWinded || BreathLeft < 0.999f)
	{
		++Layer;
		const FVector2f Size(Radius * BreathWidth, BreathHeight * Scale);
		const FVector2f At = Middle + FVector2f(-Size.X * 0.5f, Radius * BreathDown);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(At)), &FillBrush,
			ESlateDrawEffect::None, Faded(Color::Track(), Alpha));
		const FVector2f Filled(Size.X * BreathLeft, Size.Y);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer + 1, AllottedGeometry.ToPaintGeometry(Filled, FSlateLayoutTransform(At)), &FillBrush,
			ESlateDrawEffect::None, Faded(bWinded ? Color::Danger() : Color::Accent(), Alpha));
		Layer += 1;
	}
	return Layer;
}
