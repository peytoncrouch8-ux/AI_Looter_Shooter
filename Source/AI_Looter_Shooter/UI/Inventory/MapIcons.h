#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "UI/Inventory/MapPins.h"

/**
 * The map page's pin art, drawn as the kit's vector icons (LooterUI::IconBrush; no texture assets): a headstone with a
 * cross for a respawn grave, a locomotive for a station, an anvil for a gunsmith's bench, a chest, a star for a turn-in
 * and the waypoint's ring for the objective. Most sit in a dark glass badge ringed in their colour, so they read over
 * any ground; the colours come from the kit's palette.
 */
namespace MapIcons
{
	/** A pin's colour: the objective orange, a turn-in green, graves cyan, stations pale, benches ivory, chests gold. */
	AI_LOOTER_SHOOTER_API FLinearColor Color(EMapPinKind Kind);

	/** A pin's size on the page (page units): its badge's, or its icon's where it has none. */
	AI_LOOTER_SHOOTER_API float Size(EMapPinKind Kind);

	/** It sits in a dark round badge (the objective's ring and a looted chest stand alone). */
	AI_LOOTER_SHOOTER_API bool HasBadge(EMapPinKind Kind);

	/** The icon's brush at a size, tinted. */
	AI_LOOTER_SHOOTER_API FSlateBrush IconBrush(EMapPinKind Kind, float IconSize, const FLinearColor& Tint);

	/** The badge behind a pin: dark glass ringed in Ring (thicker when it's pointed at or chosen). */
	AI_LOOTER_SHOOTER_API FSlateBrush BadgeBrush(const FLinearColor& Ring, bool bLit);

	/** The share of a badge its icon fills. */
	inline constexpr float IconShare = 0.62f;
}
