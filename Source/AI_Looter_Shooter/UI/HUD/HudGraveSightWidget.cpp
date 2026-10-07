// UHudGraveSightWidget: Grave Sight's flash on the screen: the dark glass at the edges (a background) and the cyan lines
// painted over it.

#include "UI/HUD/HudGraveSightWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/Texture2D.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"
#include "Styling/WidgetStyle.h"

using namespace LooterUI;

namespace
{
	/** The vignette's texture: soft enough stretched over any screen. */
	constexpr int32 VignetteSize = 64;

	/** Clear out to this share of the way from the middle to a corner; darkest in the corners. */
	constexpr float VignetteClear = 0.42f;

	/** The dark glass's own alpha, scaled by this at the darkest (the corners): the world still shows through. */
	constexpr float VignetteStrength = 0.85f;

	/** The frame, inset from the screen's edges, its corners cut like the kit's panels (Slate units: pixels at 1080p). */
	constexpr float FrameInset = 26.f;
	constexpr float FrameCut = 22.f;
	constexpr float FrameThickness = 1.5f;

	/** The kit's corner brackets, just inside the frame. */
	constexpr float BracketInset = 12.f;
	constexpr float BracketLength = 56.f;
	constexpr float BracketThickness = 3.f;

	/** Faint scan lines down the screen, as the kit's glass has them. */
	constexpr float ScanSpacing = 6.f;
	constexpr float ScanStrength = 0.22f;

	/** The sweep: a brighter line with two fading after it, top to bottom once over the flash. */
	constexpr int32 SweepTrail = 2;
	constexpr float SweepTrailSpacing = 5.f;
	constexpr float SweepThickness = 2.f;

	/** The ring: opening from this share of the screen's short side to that one, fading as it opens. */
	constexpr float RingFrom = 0.06f;
	constexpr float RingTo = 0.6f;
	constexpr int32 RingSegments = 64;
	constexpr float RingThickness = 2.f;

	/**
	 * The vignette's shape: white, clear in the middle and more solid toward the edges and corners, tinted by its brush.
	 * Made once and kept for the session, like the kit's other generated textures (no texture asset to manage).
	 */
	UTexture2D* VignetteTexture()
	{
		static UTexture2D* Texture = nullptr;
		if (Texture)
		{
			return Texture;
		}
		TArray<uint8> Pixels;
		Pixels.SetNumUninitialized(VignetteSize * VignetteSize * 4);
		for (int32 Y = 0; Y < VignetteSize; ++Y)
		{
			for (int32 X = 0; X < VignetteSize; ++X)
			{
				// From the middle (0) to a corner (1), so the edges' middles sit about halfway in.
				const float U = (X + 0.5f) / VignetteSize * 2.f - 1.f;
				const float V = (Y + 0.5f) / VignetteSize * 2.f - 1.f;
				const float Out = FMath::Sqrt(U * U + V * V) / UE_SQRT_2;
				const uint8 Shade = static_cast<uint8>(FMath::SmoothStep(VignetteClear, 1.f, Out) * 255.f + 0.5f);
				const int32 Index = (Y * VignetteSize + X) * 4;
				Pixels[Index + 0] = 255;
				Pixels[Index + 1] = 255;
				Pixels[Index + 2] = 255;
				Pixels[Index + 3] = Shade;
			}
		}
		Texture = UTexture2D::CreateTransient(VignetteSize, VignetteSize, PF_B8G8R8A8, FName(TEXT("UI_GraveSightVignette")), Pixels);
		Texture->SRGB = true;
		Texture->Filter = TF_Bilinear;
		Texture->NeverStream = true;
		Texture->UpdateResource();
		Texture->AddToRoot();
		return Texture;
	}

	/** The vignette as a brush in the kit's dark glass, stretched over whatever it fills. */
	FSlateBrush VignetteBrush()
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(VignetteTexture());
		Brush.ImageSize = FVector2D(VignetteSize, VignetteSize);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		const FLinearColor Glass = Color::ScreenBg();
		Brush.TintColor = FSlateColor(FLinearColor(Glass.R, Glass.G, Glass.B, Glass.A * VignetteStrength));
		return Brush;
	}

	FLinearColor Faded(const FLinearColor& Base, float Strength)
	{
		return FLinearColor(Base.R, Base.G, Base.B, Base.A * FMath::Clamp(Strength, 0.f, 1.f));
	}

	void DrawLines(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, TArray<FVector2f> Points,
		const FLinearColor& Tint, float Thickness)
	{
		if (Points.Num() >= 2 && Tint.A > 0.f)
		{
			FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Tint, true, Thickness);
		}
	}
}

TSharedRef<SWidget> UHudGraveSightWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		Vignette = MakeImage(WidgetTree, VignetteBrush());
		// It darkens the world behind the lines: a background, so the UI transparency setting fades it.
		MarkBackground(Vignette);
		FillOverlaySlot(Root->AddChildToOverlay(Vignette));
		WidgetTree->RootWidget = Root;
	}
	TSharedRef<SWidget> Built = Super::RebuildWidget();
	// Hidden until the first flash's first frame.
	SetVisibility(Alpha > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	return Built;
}

void UHudGraveSightWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Its lines move every frame of a flash (and it's collapsed between flashes, when that costs nothing).
	ForceVolatile(true);
}

void UHudGraveSightWidget::SetFlash(float InAlpha, float InProgress)
{
	Alpha = FMath::Clamp(InAlpha, 0.f, 1.f);
	Progress = FMath::Clamp(InProgress, 0.f, 1.f);
	SetRenderOpacity(Alpha);
	const ESlateVisibility Wanted = Alpha > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (GetVisibility() != Wanted)
	{
		SetVisibility(Wanted);
	}
}

int32 UHudGraveSightWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Under = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	// Its opacity (the flash's strength) comes in the style, as it does for the vignette image: the lines take it too.
	const float Strength = InWidgetStyle.GetColorAndOpacityTint().A;
	const FVector2f Size(AllottedGeometry.GetLocalSize());
	if (Strength <= 0.f || Size.X <= 2.f * FrameInset || Size.Y <= 2.f * FrameInset)
	{
		return Under;
	}
	const int32 Layer = Under + 1;
	const float Left = FrameInset;
	const float Top = FrameInset;
	const float Right = Size.X - FrameInset;
	const float Bottom = Size.Y - FrameInset;

	// Faint scan lines over the whole view, like the kit's dark glass.
	const FLinearColor Scan = Faded(Color::ScreenLine(), ScanStrength * Strength);
	for (float Y = Top + ScanSpacing; Y < Bottom; Y += ScanSpacing)
	{
		DrawLines(OutDrawElements, Layer, AllottedGeometry, { FVector2f(Left, Y), FVector2f(Right, Y) }, Scan, 1.f);
	}

	// The sweep, top to bottom once over the flash, its trail fading behind it.
	const float SweepY = FMath::Lerp(Top, Bottom, Progress);
	for (int32 Trail = 0; Trail <= SweepTrail; ++Trail)
	{
		const float Y = SweepY - Trail * SweepTrailSpacing;
		if (Y > Top)
		{
			DrawLines(OutDrawElements, Layer, AllottedGeometry, { FVector2f(Left, Y), FVector2f(Right, Y) },
				Faded(Color::SegmentOn(), Strength / (1.f + 2.f * Trail)), Trail == 0 ? SweepThickness : 1.f);
		}
	}

	// The frame, its corners cut as the kit's panels are.
	DrawLines(OutDrawElements, Layer, AllottedGeometry, {
		FVector2f(Left + FrameCut, Top), FVector2f(Right - FrameCut, Top), FVector2f(Right, Top + FrameCut),
		FVector2f(Right, Bottom - FrameCut), FVector2f(Right - FrameCut, Bottom), FVector2f(Left + FrameCut, Bottom),
		FVector2f(Left, Bottom - FrameCut), FVector2f(Left, Top + FrameCut), FVector2f(Left + FrameCut, Top) },
		Faded(Color::TileLine(), Strength), FrameThickness);

	// The kit's corner brackets, just inside it.
	const FLinearColor Bracket = Faded(Color::SegmentOn(), Strength);
	const float InLeft = Left + BracketInset;
	const float InTop = Top + BracketInset;
	const float InRight = Right - BracketInset;
	const float InBottom = Bottom - BracketInset;
	DrawLines(OutDrawElements, Layer, AllottedGeometry, { FVector2f(InLeft, InTop + BracketLength), FVector2f(InLeft, InTop),
		FVector2f(InLeft + BracketLength, InTop) }, Bracket, BracketThickness);
	DrawLines(OutDrawElements, Layer, AllottedGeometry, { FVector2f(InRight - BracketLength, InTop), FVector2f(InRight, InTop),
		FVector2f(InRight, InTop + BracketLength) }, Bracket, BracketThickness);
	DrawLines(OutDrawElements, Layer, AllottedGeometry, { FVector2f(InRight, InBottom - BracketLength), FVector2f(InRight, InBottom),
		FVector2f(InRight - BracketLength, InBottom) }, Bracket, BracketThickness);
	DrawLines(OutDrawElements, Layer, AllottedGeometry, { FVector2f(InLeft + BracketLength, InBottom), FVector2f(InLeft, InBottom),
		FVector2f(InLeft, InBottom - BracketLength) }, Bracket, BracketThickness);

	// The ring, opening out from the middle of the view (where the player looks: at what the sight found), fading as it goes.
	const float Short = FMath::Min(Size.X, Size.Y);
	const float Opened = FMath::InterpEaseOut(0.f, 1.f, Progress, 2.f);
	const float Radius = Short * FMath::Lerp(RingFrom, RingTo, Opened);
	const FVector2f Middle = Size * 0.5f;
	TArray<FVector2f> Ring;
	Ring.Reserve(RingSegments + 1);
	for (int32 Segment = 0; Segment <= RingSegments; ++Segment)
	{
		const float Angle = UE_TWO_PI * Segment / RingSegments;
		Ring.Add(Middle + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
	}
	DrawLines(OutDrawElements, Layer, AllottedGeometry, MoveTemp(Ring), Faded(Color::SegmentOn(), Strength * (1.f - Opened)), RingThickness);
	return Layer;
}
