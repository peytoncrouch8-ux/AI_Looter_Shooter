#include "UI/Cards/LooterHollowKey.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
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

	FVector2f MeasureLabel(const FString& Label)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return FVector2f(Label.Len() * 9.f, 16.f);
		}
		return FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label, CapFont()));
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

	// A dark edge round the outside first, so the hollow cap reads over bright ground and beams.
	const FSlateRoundedBoxBrush Edge(FLinearColor::Transparent, Radius + 1.5f, Color::Outline().CopyWithNewOpacity(0.7f * Alpha), 1.5f);
	FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(Size + FVector2f(3.f, 3.f), FSlateLayoutTransform(FVector2f(-1.5f, -1.5f))),
		&Edge, ESlateDrawEffect::None, FLinearColor::White);
	++Layer;
	// The fill, left to right as the hold runs.
	if (Fill > 0.f)
	{
		const FSlateRoundedBoxBrush Inside(Gold, Radius);
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f(FMath::Max(Size.X * Fill, Radius * 2.f), Size.Y),
			FSlateLayoutTransform(FVector2f(0.f, 0.f))), &Inside, ESlateDrawEffect::None, FLinearColor::White);
		++Layer;
	}
	const FSlateRoundedBoxBrush Ring(FLinearColor::Transparent, Radius, Gold, Line);
	FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), &Ring, ESlateDrawEffect::None, FLinearColor::White);
	++Layer;

	// The key's name, gold on the hollow, dark where the fill has reached past its middle.
	const FVector2f Text = MeasureLabel(Label);
	const FVector2f At((Size.X - Text.X) * 0.5f, (Size.Y - Text.Y) * 0.5f);
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
