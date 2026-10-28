#include "UI/Cards/LooterSegmentBar.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "UI/Style/LooterUIStyle.h"

// The gun card's stat bar: ten skewed segments like the HUD's slanted bars, so a rating reads at a glance as a count.

using namespace LooterUI;

namespace
{
	/** The segments lean like the cards' cuts: the top edge sits this far right of the bottom. */
	constexpr float Lean = 2.5f;
	/** The compared gun's tick: 3 px wide, past the bar by 3 px above and below. */
	constexpr float TickWidth = 3.f;
	constexpr float TickReach = 3.f;
}

int32 LooterSegmentBarRules::LitSegments(float Fill)
{
	if (Fill <= 0.f)
	{
		return 0;
	}
	return FMath::Clamp(FMath::RoundToInt(Fill * Segments), 1, Segments);
}

void SLooterSegmentBar::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
}

void SLooterSegmentBar::SetFill(float InFill)
{
	Fill = FMath::Clamp(InFill, 0.f, 1.f);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SLooterSegmentBar::SetShown(float InShown)
{
	if (!FMath::IsNearlyEqual(Shown, InShown))
	{
		Shown = FMath::Clamp(InShown, 0.f, 1.f);
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SLooterSegmentBar::SetTick(float InTick)
{
	Tick = InTick;
	Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SLooterSegmentBar::ComputeDesiredSize(float) const
{
	return FVector2D(120.f, LooterSegmentBarRules::Height + TickReach * 2.f);
}

int32 SLooterSegmentBar::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());
	if (Size.X < 10.f || !FSlateApplication::IsInitialized())
	{
		return LayerId;
	}
	using namespace LooterSegmentBarRules;
	const float Alpha = InWidgetStyle.GetColorAndOpacityTint().A;
	const float Top = (Size.Y - Height) * 0.5f;
	const float Bottom = Top + Height;
	const float SegmentWidth = (Size.X - Lean - Gap * (Segments - 1)) / Segments;
	// Lit up to the share drawn yet: the intro fills it segment by segment, the last one partway.
	const float LitWidth = static_cast<float>(LitSegments(Fill)) * Shown;

	TArray<FSlateVertex> Verts;
	TArray<SlateIndex> Indices;
	const FSlateRenderTransform& Transform = AllottedGeometry.GetAccumulatedRenderTransform();
	auto Quad = [&](float X0, float X1, float Y0, float Y1, float Skew, const FLinearColor& Tint)
	{
		FLinearColor Faded = Tint;
		Faded.A *= Alpha;
		const FColor C = Faded.ToFColor(true);
		const SlateIndex Base = static_cast<SlateIndex>(Verts.Num());
		Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(X0 + Skew, Y0), FVector2f(0.5f, 0.5f), C));
		Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(X1 + Skew, Y0), FVector2f(0.5f, 0.5f), C));
		Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(X1, Y1), FVector2f(0.5f, 0.5f), C));
		Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(X0, Y1), FVector2f(0.5f, 0.5f), C));
		Indices.Append({ Base, static_cast<SlateIndex>(Base + 1), static_cast<SlateIndex>(Base + 2), Base, static_cast<SlateIndex>(Base + 2),
			static_cast<SlateIndex>(Base + 3) });
	};

	const FLinearColor Unlit = Color::ScreenLine().CopyWithNewOpacity(0.14f);
	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const float X0 = Index * (SegmentWidth + Gap);
		const float X1 = X0 + SegmentWidth;
		Quad(X0, X1, Top, Bottom, Lean, Unlit);
		const float Lit = FMath::Clamp(LitWidth - static_cast<float>(Index), 0.f, 1.f);
		if (Lit > 0.f)
		{
			Quad(X0, X0 + SegmentWidth * Lit, Top, Bottom, Lean, Color::Text());
		}
	}
	if (Tick >= 0.f)
	{
		// Where the compared gun's rating ends, as the bar would light it.
		const float Lit = static_cast<float>(LitSegments(Tick));
		const float X = FMath::Clamp(Lit * (SegmentWidth + Gap) - Gap * 0.5f, 0.f, Size.X - TickWidth);
		Quad(X - TickWidth * 0.5f, X + TickWidth * 0.5f, Top - TickReach, Bottom + TickReach, Lean, Color::Accent());
	}
	const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*White);
	FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, Handle, Verts, Indices, nullptr, 0, 0);
	return LayerId;
}

TSharedRef<SWidget> ULooterSegmentBar::RebuildWidget()
{
	Bar = SNew(SLooterSegmentBar);
	Bar->SetFill(Fill);
	Bar->SetShown(Shown);
	Bar->SetTick(Tick);
	return Bar.ToSharedRef();
}

void ULooterSegmentBar::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Bar.Reset();
}

void ULooterSegmentBar::SetFill(float InFill)
{
	Fill = FMath::Clamp(InFill, 0.f, 1.f);
	if (Bar.IsValid())
	{
		Bar->SetFill(Fill);
	}
}

void ULooterSegmentBar::SetShown(float InShown)
{
	Shown = FMath::Clamp(InShown, 0.f, 1.f);
	if (Bar.IsValid())
	{
		Bar->SetShown(Shown);
	}
}

void ULooterSegmentBar::SetTick(float InTick)
{
	Tick = InTick;
	if (Bar.IsValid())
	{
		Bar->SetTick(Tick);
	}
}
