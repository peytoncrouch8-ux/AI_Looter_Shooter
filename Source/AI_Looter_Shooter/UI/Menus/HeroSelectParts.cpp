// HeroSelectParts: the painted portraits as brushes and widgets, a power's line on the hero panel, the screen's words.

#include "UI/Menus/HeroSelectParts.h"
#include "Embers/EmberDefinition.h"
#include "Embers/HeroDefinition.h"
#include "UI/Embers/EmbersWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

using namespace LooterUI;

namespace
{
	/** A power's medallion on the panel: the disc, the icon in it, the ember's glow standing in for an icon not drawn yet. */
	constexpr float MedallionSize = 54.f;
	constexpr float MedallionIcon = 38.f;

	/**
	 * Where the painted busts keep the face (the 1024 px sheets of Art/Portraits: the frame's window is about 0.1 to 0.9
	 * each way, the head in its upper middle): a square round hat and face, and a wide band at the eyes for the roster.
	 */
	const FBox2f FaceRegion(FVector2f(0.2f, 0.12f), FVector2f(0.8f, 0.72f));
	const FBox2f BandRegion(FVector2f(0.14f, 0.17f), FVector2f(0.86f, 0.52f));
}

FLinearColor HeroSelectParts::WithAlpha(FLinearColor Tone, float Alpha)
{
	Tone.A = Alpha;
	return Tone;
}

FSlateBrush HeroSelectParts::PortraitBrush(UTexture2D* Portrait, const FVector2D& Size, ECrop Crop)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(Portrait);
	Brush.ImageSize = Size;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	if (Crop == ECrop::Face)
	{
		Brush.SetUVRegion(FaceRegion);
	}
	else if (Crop == ECrop::Band)
	{
		Brush.SetUVRegion(BandRegion);
	}
	return Brush;
}

FLinearColor HeroSelectParts::GreyedTint()
{
	// Darkened, then washed toward the kit's slate grey (GreyedWash): most of the paint's colour goes, the picture stays.
	return FLinearColor(0.42f, 0.42f, 0.42f, 1.f);
}

FLinearColor HeroSelectParts::GreyedWash()
{
	return WithAlpha(Color::IconShade(), 0.42f);
}

UWidget* HeroSelectParts::MakePortrait(UWidgetTree* Tree, UTexture2D* Portrait, const FVector2D& Size, ECrop Crop, bool bGreyed, const FLinearColor& Tone)
{
	UOverlay* Picture = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	if (Portrait)
	{
		UImage* Image = MakeImage(Tree, PortraitBrush(Portrait, Size, Crop));
		if (bGreyed)
		{
			Image->SetColorAndOpacity(GreyedTint());
		}
		FillOverlaySlot(Picture->AddChildToOverlay(Image));
		if (bGreyed)
		{
			FillOverlaySlot(Picture->AddChildToOverlay(MakeImage(Tree, RectBrush(GreyedWash()))));
		}
	}
	else
	{
		// Not imported yet (Tools/Unreal/import_hero_portraits.py): a dark tile in the hero's colour holds the place.
		FillOverlaySlot(Picture->AddChildToOverlay(MakeImage(Tree, RectBrush(Color::Track(), WithAlpha(Tone, 0.6f), 1.f))));
	}
	if (Crop != ECrop::Whole)
	{
		// A cut picture's edge: a hairline, in the hero's colour (greyed, the kit's dim line).
		FillOverlaySlot(Picture->AddChildToOverlay(MakeImage(Tree, RectBrush(FLinearColor::Transparent,
			bGreyed ? Color::RowLine() : WithAlpha(Tone, 0.75f), 1.f))));
	}
	return MakeSized(Tree, Picture, Size.X, Size.Y);
}

UWidget* HeroSelectParts::MakeWorksStamp(UWidgetTree* Tree, int32 FontSize)
{
	UBorder* Stamp = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
	Stamp->SetBrush(RectBrush(WithAlpha(Color::Track(), 0.82f), Color::Accent(), FontSize >= 14 ? 2.f : 1.f));
	MarkBackground(Stamp);
	Stamp->SetPadding(FMargin(FontSize * 0.9f, FontSize * 0.3f, FontSize * 0.9f, FontSize * 0.35f));
	Stamp->SetContent(MakeText(Tree, InTheWorks, FontSize, Color::Accent(), true, 300));
	Stamp->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Stamp;
}

UWidget* HeroSelectParts::MakePowerRow(UWidgetTree* Tree, const UEmberDefinition& Power)
{
	const FLinearColor Tone = Power.Color;
	UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	// The medallion, as on the powers page: a dark disc ringed in the power's colour, its Inked icon; a power whose icon
	// isn't drawn yet shows its ember's glow instead.
	{
		UOverlay* Medallion = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		auto AddCentred = [Medallion](UWidget* Child)
		{
			UOverlaySlot* ChildSlot = Medallion->AddChildToOverlay(Child);
			ChildSlot->SetHorizontalAlignment(HAlign_Center);
			ChildSlot->SetVerticalAlignment(VAlign_Center);
		};
		UImage* Disc = MakeImage(Tree, CircleBrush(WithAlpha(Color::Track(), 0.85f)));
		MarkBackground(Disc);
		AddCentred(MakeSized(Tree, Disc, MedallionSize, MedallionSize));
		AddCentred(MakeSized(Tree, MakeImage(Tree, CircleBrush(FLinearColor::Transparent, WithAlpha(Tone, 0.9f), 2.f)), MedallionSize, MedallionSize));
		if (const FInkedIcon* Inked = FindInkedIcon(Power.IconName))
		{
			AddCentred(MakeImage(Tree, InkedIconBrush(Power.IconName, *Inked, FVector2D(MedallionIcon, MedallionIcon))));
		}
		else
		{
			AddCentred(MakeImage(Tree, GlowBrush(FVector2D(MedallionIcon, MedallionIcon), WithAlpha(Tone, 0.9f))));
			AddCentred(MakeSized(Tree, MakeImage(Tree, CircleBrush(WithAlpha(Tone, 0.95f))), 8.f, 8.f));
		}
		UHorizontalBoxSlot* MedallionSlot = Row->AddChildToHorizontalBox(MakeSized(Tree, Medallion, MedallionSize, MedallionSize));
		MedallionSlot->SetVerticalAlignment(VAlign_Top);
		MedallionSlot->SetPadding(FMargin(0.f, 2.f, 14.f, 0.f));
	}

	UVerticalBox* Words = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	// Its name, and its nature as a chip in its colour.
	{
		UHorizontalBox* Top = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Top->AddChildToHorizontalBox(MakeText(Tree, Power.DisplayName.ToString(), 15, Color::Title(), true, 100))->SetVerticalAlignment(VAlign_Center);
		UBorder* Chip = Tree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Chip->SetBrush(RectBrush(Color::Plate(), WithAlpha(Tone, 0.7f), 1.f));
		MarkBackground(Chip);
		Chip->SetPadding(FMargin(7.f, 1.f, 7.f, 2.f));
		Chip->SetContent(MakeText(Tree, Power.Nature.ToString(), 8, Tone, true, 200));
		UHorizontalBoxSlot* ChipSlot = Top->AddChildToHorizontalBox(Chip);
		ChipSlot->SetVerticalAlignment(VAlign_Center);
		ChipSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
		Words->AddChildToVerticalBox(Top);
	}
	// What it does, in the regular face (a long read), then its clock.
	UTextBlock* Description = MakeText(Tree, Power.Description.ToString(), 11, Color::Text());
	Description->SetFont(Font(11, false));
	Description->SetAutoWrapText(true);
	Words->AddChildToVerticalBox(Description)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	const FString Clock = FString::Printf(TEXT("Cooldown %s   ·   %s"), *UEmbersWidget::SecondsWords(Power.CooldownSeconds, false),
		Power.DurationSeconds > 0.f ? *FString::Printf(TEXT("Lasts %s"), *UEmbersWidget::SecondsWords(Power.DurationSeconds, true)) : TEXT("Instant"));
	Words->AddChildToVerticalBox(MakeText(Tree, Clock, 9, Color::TextDim(), true, 150))->SetPadding(FMargin(0.f, 5.f, 0.f, 0.f));
	UHorizontalBoxSlot* WordsSlot = Row->AddChildToHorizontalBox(Words);
	WordsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	return Row;
}

FString HeroSelectParts::ConfirmWords(const UHeroDefinition& Hero, bool bCanPick)
{
	return bCanPick ? FString::Printf(TEXT("Play as %s"), *Hero.DisplayName.ToString()) : FString(InTheWorks);
}
