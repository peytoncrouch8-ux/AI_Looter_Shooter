#include "UI/World/EmberMarkWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

using namespace LooterUI;

// UEmberMarkWidget: the glyph, its glow and the word, in a fixed box (AEmberMarkActor pins its bottom middle over the creature).

TSharedRef<SWidget> UEmberMarkWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		USizeBox* Box = MakeSized(WidgetTree, Stack, BoxWidth, BoxHeight);
		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Box;

		// The glow, then the glyph over it, both in the upper part of the box; the word along its foot.
		Halo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		UOverlaySlot* HaloSlot = Stack->AddChildToOverlay(Halo);
		HaloSlot->SetHorizontalAlignment(HAlign_Center);
		HaloSlot->SetVerticalAlignment(VAlign_Top);

		Glyph = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Glyph->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		UOverlaySlot* GlyphSlot = Stack->AddChildToOverlay(Glyph);
		GlyphSlot->SetHorizontalAlignment(HAlign_Center);
		GlyphSlot->SetVerticalAlignment(VAlign_Top);
		GlyphSlot->SetPadding(FMargin(0.f, (HaloSize - IconSize) * 0.5f, 0.f, 0.f));

		Words = MakeFloatingText(WidgetTree, WordSize, Color::Text(), 40, ETextJustify::Center);
		UOverlaySlot* WordsSlot = Stack->AddChildToOverlay(Words);
		WordsSlot->SetHorizontalAlignment(HAlign_Center);
		WordsSlot->SetVerticalAlignment(VAlign_Bottom);
		Words->SetText(FText::FromString(Shown));
		ApplyBrushes();
	}
	return Super::RebuildWidget();
}

void UEmberMarkWidget::SetGlyph(FName IconName, const FLinearColor& Color)
{
	if (bBrushesSet && IconName == Icon && Color.Equals(Tint))
	{
		return;
	}
	Icon = IconName;
	Tint = Color;
	bBrushesSet = false;
	ApplyBrushes();
}

void UEmberMarkWidget::ApplyBrushes()
{
	if (!Halo || !Glyph)
	{
		return;
	}
	Halo->SetBrush(GlowBrush(FVector2D(HaloSize), FLinearColor(Tint.R, Tint.G, Tint.B, 1.f)));
	const FInkedIcon* Inked = Icon.IsNone() ? nullptr : FindInkedIcon(Icon);
	if (Inked)
	{
		// The power's own icon in its own colours (the socket's and the page's), so the mark says whose it is.
		Glyph->SetBrush(InkedIconBrush(Icon, *Inked, FVector2D(IconSize, IconSize)));
	}
	else
	{
		// The minimap's hostile diamond in the power's colour, centred where an icon would be.
		FSlateBrush Diamond = MarkerBrush(EMarker::Diamond, FLinearColor(Tint.R, Tint.G, Tint.B, 1.f));
		Diamond.ImageSize = FVector2D(DiamondSize);
		Glyph->SetBrush(Diamond);
	}
	bBrushesSet = true;
}

void UEmberMarkWidget::SetWords(const FString& InWords)
{
	if (InWords == Shown)
	{
		return;
	}
	Shown = InWords;
	if (Words)
	{
		Words->SetText(FText::FromString(Shown));
	}
}

void UEmberMarkWidget::SetLook(float Alpha, float Scale, float Glow)
{
	SetRenderOpacity(FMath::Clamp(Alpha, 0.f, 1.f));
	if (Glyph)
	{
		Glyph->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(Scale), FVector2D::ZeroVector, 0.f));
	}
	if (Halo)
	{
		// The glow breathes behind the glyph, never more than a hint.
		Halo->SetRenderTransform(FWidgetTransform(FVector2D::ZeroVector, FVector2D(0.7f + 0.3f * Scale), FVector2D::ZeroVector, 0.f));
		Halo->SetRenderOpacity(0.16f + 0.24f * FMath::Clamp(Glow, 0.f, 1.f));
	}
}
