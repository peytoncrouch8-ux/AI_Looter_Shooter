#include "UI/Cards/LooterHollowKey.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "UI/Style/LooterUIStyle.h"

// A hollow key cap: the user found the solid orange caps "bubbly" (2026-10-28). The outline is the cap; a hold fills it.

using namespace LooterUI;

namespace
{
	constexpr float CapHeight = 20.f;
	constexpr float PadX = 6.f;
	constexpr float Chamfer = 5.f;
	constexpr float Line = 2.f;
	constexpr int32 FontSize = 12;

	/** Outlined like floating text: the cap now floats under the card, over whatever the world shows (a loot beam's glare). */
	FSlateFontInfo CapFont()
	{
		return FloatingFont(FontSize, 100);
	}

	FVector2f MeasureLabel(const FString& Label)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return FVector2f(Label.Len() * 8.f, 14.f);
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
	return FVector2D(FMath::Max(CapHeight + 2.f, Text.X + PadX * 2.f), CapHeight);
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
	const FLinearColor Orange = Color::Accent().CopyWithNewOpacity(Color::Accent().A * Alpha);
	int32 Layer = LayerId;

	// The fill, left to right as the hold runs (a plain rectangle inside the outline: the chamfers read from the line).
	if (Fill > 0.f && FSlateApplication::IsInitialized())
	{
		const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2f((Size.X - Line * 2.f) * Fill, Size.Y - Line * 2.f),
			FSlateLayoutTransform(FVector2f(Line, Line))), White, ESlateDrawEffect::None, Orange);
		++Layer;
	}

	// The outline: chamfered top-left and bottom-right, like the kit's controls.
	const float H = Line * 0.5f;
	TArray<FVector2f> Ring = { FVector2f(Chamfer, H), FVector2f(Size.X - H, H), FVector2f(Size.X - H, Size.Y - Chamfer), FVector2f(Size.X - Chamfer, Size.Y - H),
		FVector2f(H, Size.Y - H), FVector2f(H, Chamfer), FVector2f(Chamfer, H) };
	// A dark edge under the orange line first, so the hollow cap reads over bright ground and beams.
	FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), TArray<FVector2f>(Ring), ESlateDrawEffect::None,
		Color::Outline().CopyWithNewOpacity(0.7f * Alpha), true, Line + 2.f);
	++Layer;
	FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Ring), ESlateDrawEffect::None, Orange, true, Line);
	++Layer;

	// The key's name, orange on the hollow, dark where the fill has reached past its middle.
	const FVector2f Text = MeasureLabel(Label);
	const FVector2f At((Size.X - Text.X) * 0.5f, (Size.Y - Text.Y) * 0.5f);
	const FLinearColor Letters = Fill >= 0.5f ? Color::AccentDark().CopyWithNewOpacity(Alpha) : Orange;
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
