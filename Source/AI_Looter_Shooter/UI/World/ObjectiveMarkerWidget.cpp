#include "UI/World/ObjectiveMarkerWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

using namespace LooterUI;

// UObjectiveMarkerWidget: the glow and the mark, in a fixed box (AObjectiveMarkerActor pins its bottom middle over the paper).

namespace
{
	/** Twice the drawn size at the pop's peak, so it stays crisp as it swells. */
	constexpr float MarkPixelsPerUnit = 2.8f;

	/** The mark in a 24-unit box: a dark-edged orange ring-diamond, and a small solid one in its middle. */
	FPaintedIcon MakeMarkPicture()
	{
		FPaintedIcon Picture;
		Picture.ViewBox = FVector2D(24.f, 24.f);
		const TArray<FVector2D> Ring = { FVector2D(12.f, 2.5f), FVector2D(21.5f, 12.f), FVector2D(12.f, 21.5f), FVector2D(2.5f, 12.f),
			FVector2D(12.f, 2.5f) };
		auto Diamond = [](float Half)
		{
			return TArray<FVector2D>{ FVector2D(12.f, 12.f - Half), FVector2D(12.f + Half, 12.f), FVector2D(12.f, 12.f + Half), FVector2D(12.f - Half, 12.f) };
		};

		// The dark edge under everything, so the orange holds on sunlit planks and pale stone alike.
		FPaintLayer Edge;
		Edge.Strokes.Add(Ring);
		Edge.StrokeWidth = 4.4f;
		Edge.Fills.Add(Diamond(5.2f));
		Edge.Color = Color::Outline();
		Picture.Layers.Add(Edge);

		FPaintLayer Line;
		Line.Strokes.Add(Ring);
		Line.StrokeWidth = 2.2f;
		Line.Color = Color::Accent();
		Picture.Layers.Add(Line);

		FPaintLayer Dot;
		Dot.Fills.Add(Diamond(3.8f));
		Dot.Color = Color::Accent();
		Picture.Layers.Add(Dot);
		return Picture;
	}

	const FPaintedIcon& MarkPicture()
	{
		static const FPaintedIcon Picture = MakeMarkPicture();
		return Picture;
	}
}

TSharedRef<SWidget> UObjectiveMarkerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		USizeBox* Box = MakeSized(WidgetTree, Stack, BoxSize, BoxSize);
		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Box;

		Halo = MakeImage(WidgetTree, GlowBrush(FVector2D(HaloSize), Color::Accent()));
		UOverlaySlot* HaloSlot = Stack->AddChildToOverlay(Halo);
		HaloSlot->SetHorizontalAlignment(HAlign_Center);
		HaloSlot->SetVerticalAlignment(VAlign_Center);

		Mark = MakeImage(WidgetTree, PaintedIconBrush(TEXT("ObjectiveMarker"), MarkPicture(), MarkPixelsPerUnit, FVector2D(MarkSize)));
		Mark->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UOverlaySlot* MarkSlot = Stack->AddChildToOverlay(Mark);
		MarkSlot->SetHorizontalAlignment(HAlign_Center);
		MarkSlot->SetVerticalAlignment(VAlign_Center);
	}
	return Super::RebuildWidget();
}

void UObjectiveMarkerWidget::SetLook(float Alpha, float Scale, float Glow)
{
	SetRenderOpacity(FMath::Clamp(Alpha, 0.f, 1.f));
	if (Mark)
	{
		Mark->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(Scale), FVector2D::ZeroVector, 0.f));
	}
	if (Halo)
	{
		// The glow breathes behind the mark, never more than a hint (a glow like the ember marks', not a panel: the UI
		// transparency leaves it, as it leaves outlines and icons).
		Halo->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(0.7f + 0.3f * Scale), FVector2D::ZeroVector, 0.f));
		Halo->SetRenderOpacity(0.18f + 0.22f * FMath::Clamp(Glow, 0.f, 1.f));
	}
}
