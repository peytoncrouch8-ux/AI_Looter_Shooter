#include "UI/World/GiverMarkWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

using namespace LooterUI;

namespace
{
	/** The glow round the glyph, as wide as the box. */
	constexpr float HaloSize = 72.f;
}

TSharedRef<SWidget> UGiverMarkWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		// A fixed box with the glyph standing on its foot (the actor pins the box's bottom middle over the giver's head),
		// its glow behind it.
		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		USizeBox* Box = MakeSized(WidgetTree, Stack, BoxWidth, BoxHeight);
		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Box;

		Halo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		UOverlaySlot* HaloSlot = Stack->AddChildToOverlay(Halo);
		HaloSlot->SetHorizontalAlignment(HAlign_Center);
		HaloSlot->SetVerticalAlignment(VAlign_Bottom);

		Glyph = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		// It grows and pops from its foot, so it never sinks into the giver's head.
		Glyph->SetRenderTransformPivot(FVector2D(0.5f, 1.f));
		UOverlaySlot* GlyphSlot = Stack->AddChildToOverlay(Glyph);
		GlyphSlot->SetHorizontalAlignment(HAlign_Center);
		GlyphSlot->SetVerticalAlignment(VAlign_Bottom);
		GlyphSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
		ApplyBrushes();
	}
	return Super::RebuildWidget();
}

void UGiverMarkWidget::SetMark(MissionMarkIcons::EMark InMark, bool bInMain)
{
	if (bBrushesSet && InMark == Mark && bInMain == bMain)
	{
		return;
	}
	Mark = InMark;
	bMain = bInMain;
	bBrushesSet = false;
	ApplyBrushes();
}

void UGiverMarkWidget::ApplyBrushes()
{
	if (!Halo || !Glyph)
	{
		return;
	}
	Halo->SetBrush(MissionMarkIcons::HaloBrush(bMain, FVector2D(HaloSize), 1.f));
	Glyph->SetBrush(MissionMarkIcons::MarkBrush(Mark, bMain, FVector2D(GlyphSize)));
	bBrushesSet = true;
}

void UGiverMarkWidget::SetLook(float Alpha, float Scale, float Bob, float Glow)
{
	SetRenderOpacity(FMath::Clamp(Alpha, 0.f, 1.f));
	if (Glyph)
	{
		Glyph->SetRenderTransform(FWidgetTransform(FVector2D(0.f, -Bob), FVector2D(Scale), FVector2D::ZeroVector, 0.f));
	}
	if (Halo)
	{
		// The glow follows the glyph up and down and breathes, never brighter than a hint behind it.
		Halo->SetRenderTransform(FWidgetTransform(FVector2D(0.f, -Bob), FVector2D(0.6f + 0.4f * Scale), FVector2D::ZeroVector, 0.f));
		Halo->SetRenderOpacity(0.18f + 0.22f * FMath::Clamp(Glow, 0.f, 1.f));
	}
}
