#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"

class UEmberDefinition;
class UHeroDefinition;
class UImage;
class UTexture2D;
class UWidget;
class UWidgetTree;

/**
 * The hero select's building blocks, shared with the main menu's session cards (which name their hero): the painted
 * portraits as brushes and widgets (whole, or the face's band for a card; greyed for a hero in the works), a power's
 * line on the hero panel, and the screen's words.
 */
namespace HeroSelectParts
{
	/** What the roster, the panel and the stage say of a hero not playable yet. */
	inline const TCHAR* const InTheWorks = TEXT("In the works");

	/** How a portrait is cut: whole (its painted frame and name plate), the face's square, or a wide band at the eyes. */
	enum class ECrop : uint8
	{
		Whole,
		Face,
		Band,
	};

	/** Tone with Alpha. */
	FLinearColor WithAlpha(FLinearColor Tone, float Alpha);

	/** The portrait texture as a brush of Size, cut by Crop (a UV region, so one texture serves every size). */
	FSlateBrush PortraitBrush(UTexture2D* Portrait, const FVector2D& Size, ECrop Crop);

	/** The image's tint and the wash over it for a hero in the works, so the painting reads greyed (no material needed). */
	FLinearColor GreyedTint();
	FLinearColor GreyedWash();

	/**
	 * A portrait as a widget of Size: the painting cut by Crop, greyed for a hero in the works (bGreyed), with a hairline
	 * round a cut. With no texture (not imported yet) a dark tile in the hero's colour stands in.
	 */
	UWidget* MakePortrait(UWidgetTree* Tree, UTexture2D* Portrait, const FVector2D& Size, ECrop Crop, bool bGreyed, const FLinearColor& Tone);

	/** A small stamp across a greyed portrait: IN THE WORKS, outlined in orange on a dark plate. */
	UWidget* MakeWorksStamp(UWidgetTree* Tree, int32 FontSize);

	/**
	 * A power's line on the hero panel: its medallion (a dark disc ringed in its colour, its Inked icon, or an ember's glow
	 * while it has none), its name and nature, what it does, and its clock.
	 */
	UWidget* MakePowerRow(UWidgetTree* Tree, const UEmberDefinition& Power);

	/** "Play as Ellis Ransom" for a hero that can be picked, "In the works" for one that can't. */
	FString ConfirmWords(const UHeroDefinition& Hero, bool bCanPick);
}
