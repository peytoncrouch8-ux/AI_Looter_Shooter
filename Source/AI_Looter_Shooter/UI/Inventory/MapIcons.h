#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "UI/Inventory/MapPins.h"

/**
 * The map page's pin art, drawn as the kit's vector icons (LooterUI::IconBrush; no texture assets): the crow's-foot mark burnt
 * in every roost's board for a Hob's roost (the same icon on the minimap and its rim, in the roost's one colour), a locomotive
 * for a station, an anvil for a gunsmith's bench, a candlelit stele for a vigil stone, a coin for a store, the mission marks (? for an offer, a mission to pick up; ! for a turn-in: MissionMarkIcons) and the waypoint's ring for the
 * objective. Most sit in a dark glass badge ringed in their colour, so they read over any ground; the colours come from the
 * kit's palette.
 */
namespace MapIcons
{
	/** A pin's colour: the objective orange, offers and turn-ins gold, roosts their beacon's colour, stations pale, vigil stones brass, stores silver, benches ivory. */
	AI_LOOTER_SHOOTER_API FLinearColor Color(EMapPinKind Kind);

	/** A pin's size on the page (page units): its badge's, or its icon's where it has none. */
	AI_LOOTER_SHOOTER_API float Size(EMapPinKind Kind);

	/** It sits in a dark round badge (the objective's ring stands alone). */
	AI_LOOTER_SHOOTER_API bool HasBadge(EMapPinKind Kind);

	/** The icon's brush at a size, tinted. */
	AI_LOOTER_SHOOTER_API FSlateBrush IconBrush(EMapPinKind Kind, float IconSize, const FLinearColor& Tint);

	/** The badge behind a pin: dark glass ringed in Ring (thicker when it's pointed at or chosen). */
	AI_LOOTER_SHOOTER_API FSlateBrush BadgeBrush(const FLinearColor& Ring, bool bLit);

	/** Hob's roost's icon at a size, tinted: the map's roost pin, and the minimap's roosts on the map and on its rim. */
	AI_LOOTER_SHOOTER_API FSlateBrush RoostBrush(float IconSize, const FLinearColor& Tint);

	/** The share of a badge its icon fills. */
	inline constexpr float IconShare = 0.62f;
}
