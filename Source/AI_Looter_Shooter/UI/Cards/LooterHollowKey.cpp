#include "UI/Cards/LooterHollowKey.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "UI/Style/LooterUIStyle.h"

// A hollow key cap: the user found the solid caps "bubbly" (2026-10-28), then picked the tag card's rounded, bolder caps
// in soul-light gold. The outline is the cap; a hold fills it.

using namespace LooterUI;

namespace
{
	constexpr float CapHeight = 26.f;
	constexpr float PadX = 8.f;
	constexpr float Radius = 5.f;
	constexpr float Line = 3.f;
	constexpr int32 FontSize = 14;

	/** Outlined like floating text: the cap floats under the card, over whatever the world shows (a loot beam's glare). */
	FSlateFontInfo CapFont()
	{
		return FloatingFont(FontSize, 100);
	}

	/**
	 * One loop round the cap at Offset from the outline's middle line (+ out, - in): its rounded corners keep their
	 * centres, so loops at different offsets run side by side, point for point.
	 */
	TArray<FVector2f> Loop(const FVector2f& Size, float Offset)
	{
		const float Inset = Line * 0.5f;
		const float R = FMath::Min(Radius, FMath::Min(Size.X, Size.Y) * 0.5f - Inset);
		const float LoopRadius = FMath::Max(0.f, R + Offset);
		const FVector2f Corners[4] = { FVector2f(Size.X - Inset - R, Inset + R), FVector2f(Size.X - Inset - R, Size.Y - Inset - R),
			FVector2f(Inset + R, Size.Y - Inset - R), FVector2f(Inset + R, Inset + R) };
		TArray<FVector2f> Points;
		constexpr int32 Steps = 6;
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			// y down: the top-right corner runs from straight up (-90) to the right (0), then on clockwise.
			const float From = -90.f + Corner * 90.f;
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				const float Angle = FMath::DegreesToRadians(From + 90.f * Step / Steps);
				Points.Add(Corners[Corner] + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * LoopRadius);
			}
		}
		return Points;
	}

	/**
	 * The outline as a closed band of triangles, Width wide, with a soft pixel at each side for smooth edges. A polyline
	 * left a notch where its two ends met, and a rounded-box brush with a see-through fill still painted the inside solid
	 * in game (2026-10-29: every cap read as a filled pale bubble).
	 */
	void PaintRing(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, const FVector2f& Size, float Width,
		const FLinearColor& Tint)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return;
		}
		const float Half = Width * 0.5f;
		constexpr float Soft = 0.8f;
		const TArray<FVector2f> Loops[4] = { Loop(Size, Half + Soft), Loop(Size, Half), Loop(Size, -Half), Loop(Size, -Half - Soft) };
		const FColor Solid = Tint.ToFColor(true);
		const FColor Clear = Tint.CopyWithNewOpacity(0.f).ToFColor(true);
		const FSlateRenderTransform& Transform = Geometry.GetAccumulatedRenderTransform();
		TArray<FSlateVertex> Verts;
		TArray<SlateIndex> Indices;
		const int32 Count = Loops[0].Num();
		for (int32 Ring = 0; Ring < 4; ++Ring)
		{
			const FColor Color = (Ring == 1 || Ring == 2) ? Solid : Clear;
			for (const FVector2f& Point : Loops[Ring])
			{
				Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Point, FVector2f(0.5f, 0.5f), Color));
			}
		}
		for (int32 Band = 0; Band < 3; ++Band)
		{
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const int32 Next = (Index + 1) % Count;
				const SlateIndex A = static_cast<SlateIndex>(Band * Count + Index);
				const SlateIndex B = static_cast<SlateIndex>(Band * Count + Next);
				const SlateIndex C = static_cast<SlateIndex>((Band + 1) * Count + Next);
				const SlateIndex D = static_cast<SlateIndex>((Band + 1) * Count + Index);
				Indices.Append({ A, B, C, A, C, D });
			}
		}
		const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
		const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*White);
		FSlateDrawElement::MakeCustomVerts(Elements, Layer, Handle, Verts, Indices, nullptr, 0, 0);
	}

	/**
	 * The label's size in the cap's own units, measured at the scale it is drawn at: the gun card over a looked-at gun is
	 * drawn smaller, where the font's hinted widths aren't the full-size widths scaled down, so a label measured at full size
	 * sat off centre there (the user, 2026-10-29: "the hold E and E aren't center").
	 */
	FVector2f MeasureLabel(const FString& Label, float Scale = 1.f)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return FVector2f(Label.Len() * 9.f, 16.f);
		}
		const float Safe = FMath::Max(Scale, 0.01f);
		return FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label, CapFont(), Safe)) / Safe;
	}
}

void SLooterHollowKey::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
}

void SLooterHollowKey::SetLabel(const FString& InLabel)
{
	if (Label != InLabel)
	{
		Label = InLabel;
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

void SLooterHollowKey::SetFill(float InFill)
{
	const float Clamped = FMath::Clamp(InFill, 0.f, 1.f);
	if (!FMath::IsNearlyEqual(Fill, Clamped))
	{
		Fill = Clamped;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SLooterHollowKey::SetDim(bool bInDim)
{
	if (bDim != bInDim)
	{
		bDim = bInDim;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

FVector2D SLooterHollowKey::ComputeDesiredSize(float) const
{
	const FVector2f Text = MeasureLabel(Label);
	return FVector2D(FMath::Max(CapHeight + 4.f, Text.X + PadX * 2.f), CapHeight);
}

int32 SLooterHollowKey::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());
	if (Size.X < 4.f || Size.Y < 4.f)
	{
		return LayerId;
	}
	const float Alpha = InWidgetStyle.GetColorAndOpacityTint().A * (bDim ? 0.6f : 1.f);
	const FLinearColor Gold = Color::SoulGold().CopyWithNewOpacity(Alpha);
	int32 Layer = LayerId;

	// A dark edge under the gold outline, so the hollow cap reads over bright ground and beams (PaintRing: why a band).
	PaintRing(OutDrawElements, Layer, AllottedGeometry, Size, Line + 3.f, Color::Outline().CopyWithNewOpacity(0.7f * Alpha));
	++Layer;
	// The fill, left to right as the hold runs.
	if (Fill > 0.f)
	{
		const FSlateRoundedBoxBrush Inside(Gold, Radius);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(FMath::Max(Size.X * Fill, Radius * 2.f), Size.Y),
			FSlateLayoutTransform(FVector2f(0.f, 0.f))), &Inside, ESlateDrawEffect::None, FLinearColor::White);
		++Layer;
	}
	PaintRing(OutDrawElements, Layer, AllottedGeometry, Size, Line, Gold);
	++Layer;

	// The key's name, gold on the hollow, dark where the fill has reached past its middle.
	const FVector2f Text = MeasureLabel(Label, AllottedGeometry.Scale);
	// The outlined letters draw about 1.5 px right of where they measure (seen on the gun card's E and HOLD E, 2026-10-29).
	const FVector2f At((Size.X - Text.X) * 0.5f - 1.5f, (Size.Y - Text.Y) * 0.5f);
	const FLinearColor Letters = Fill >= 0.5f ? Color::SlateGem().CopyWithNewOpacity(Alpha) : Gold;
	FSlateDrawElement::MakeText(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(Text, FSlateLayoutTransform(At)), Label, CapFont(),
		ESlateDrawEffect::None, Letters);
	return Layer;
}

TSharedRef<SWidget> ULooterHollowKey::RebuildWidget()
{
	Key = SNew(SLooterHollowKey);
	Key->SetLabel(Label);
	Key->SetFill(Fill);
	Key->SetDim(bDim);
	return Key.ToSharedRef();
}

void ULooterHollowKey::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Key.Reset();
}

void ULooterHollowKey::SetLabel(const FString& InLabel)
{
	Label = InLabel;
	if (Key.IsValid())
	{
		Key->SetLabel(Label);
	}
}

void ULooterHollowKey::SetFill(float InFill)
{
	Fill = FMath::Clamp(InFill, 0.f, 1.f);
	if (Key.IsValid())
	{
		Key->SetFill(Fill);
	}
}

void ULooterHollowKey::SetDim(bool bInDim)
{
	bDim = bInDim;
	if (Key.IsValid())
	{
		Key->SetDim(bDim);
	}
}
