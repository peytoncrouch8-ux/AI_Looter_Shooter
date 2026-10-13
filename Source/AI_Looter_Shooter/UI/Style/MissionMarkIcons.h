#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "UI/Style/LooterUIStyle.h"

/**
 * The givers' marks: a ? over someone (or a board, or an artifact) with a mission to offer, a ! where a mission waits to be
 * turned in (the user's call, 2026-10-13: the other way round from Borderlands). Gold for the story's missions, white for
 * side jobs. One look everywhere they show: over the giver in the world (UGiverMarkWidget), on the minimap, on the map
 * page's pins, on the offer card and the Missions page. Painted in code with the kit's painted-icon brushes
 * (LooterUI::PaintedIconBrush), never textures: a bold glyph with a light top and a deeper underside inside a dark ink
 * line, so it reads over sky, grass or the map's glass.
 */
namespace MissionMarkIcons
{
	/** What a mark says. */
	enum class EMark : uint8
	{
		/** A mission to pick up: ? */
		Offer,
		/** A mission waiting to be turned in: ! */
		TurnIn,
	};

	/** The mark's colour for rings, words and glows: gold for a main mission, white for a side one. */
	AI_LOOTER_SHOOTER_API FLinearColor Color(bool bMain);

	/** Its deeper shade (the glyph's underside, a gold card's edge). */
	AI_LOOTER_SHOOTER_API FLinearColor ShadeColor(bool bMain);

	/** The glyph in its own colours at Size; keep Tint white, its alpha fades it. */
	AI_LOOTER_SHOOTER_API FSlateBrush MarkBrush(EMark Mark, bool bMain, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White);

	/** The soft glow behind a mark in the world, tinted gold or white at Alpha. */
	AI_LOOTER_SHOOTER_API FSlateBrush HaloBrush(bool bMain, const FVector2D& Size, float Alpha);

	/** The painted glyph itself (24 x 24 view box), for whoever draws it at another resolution. */
	AI_LOOTER_SHOOTER_API const LooterUI::FPaintedIcon& Icon(EMark Mark, bool bMain);

	/** "?" (an offer) or "!" (a turn-in). */
	AI_LOOTER_SHOOTER_API const TCHAR* Glyph(EMark Mark);
}
