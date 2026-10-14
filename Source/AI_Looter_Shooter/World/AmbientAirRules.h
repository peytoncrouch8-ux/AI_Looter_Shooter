#pragma once

#include "CoreMinimal.h"
#include "World/AmbientAirTypes.h"

/** How each kind of thing in the air looks and moves (AmbientAirRules::Look). */
struct FAirKindLook
{
	/** Across (cm; a leaf's length), and how long one lives (s). */
	float SizeMin = 1.f;
	float SizeMax = 1.f;
	float LifeMin = 8.f;
	float LifeMax = 16.f;
	/** How far round the view they're kept (cm, level): in the open, and under a roof. */
	float Reach = 800.f;
	float ReachUnderCover = 800.f;
	/** How high they float (cm): over the ground (pollen, fluff), or against the eye (motes). */
	float HeightMin = 0.f;
	float HeightMax = 200.f;
	/** Their own wander (cm/s), the breeze's carry and a full gust's (cm/s). */
	float Wander = 5.f;
	float Breeze = 0.f;
	float GustCarry = 0.f;
};

/**
 * The ambient air's rules, apart from the world so the tests can ask them (UAmbientAirSubsystem plays them): the budget,
 * each kind's share of it and look, where each belongs, how many are wanted out round the view, and their fades.
 */
namespace AmbientAirRules
{
	/** The most out at once, all kinds together: the caps add up to it. */
	inline constexpr int32 Budget = 250;

	/** Moths a lit lamp draws. */
	inline constexpr int32 MothsPerLamp = 4;

	/** Broadleaf trees near enough for the leaves' full share. */
	inline constexpr int32 TreesForFullLeaves = 8;

	/** A kind's most at once. */
	AI_LOOTER_SHOOTER_API int32 Cap(EAirKind Kind);

	AI_LOOTER_SHOOTER_API const FAirKindLook& Look(EAirKind Kind);

	/** Its reach round the view (cm), under a roof or not. */
	AI_LOOTER_SHOOTER_API float Reach(EAirKind Kind, bool bUnderCover);

	/** The lighting state is dusk (Ransom's Rest's Dusk); anything else, and a level without states, is day. */
	AI_LOOTER_SHOOTER_API bool IsDusk(FName Lighting);

	/** Ground that grows pollen and seed fluff: grass and hay. */
	AI_LOOTER_SHOOTER_API bool IsMeadow(EImpactMaterial Surface);

	/** A tree model whose leaves fall: an oak, a birch, an apple, a cottonwood, a willow... never a pine, a dead tree or a far card. */
	AI_LOOTER_SHOOTER_API bool IsBroadleaf(const FString& MeshName);

	/** Kinds placed by looking at a point (motes, pollen, fluff); leaves and moths come from their trees and lamps. */
	AI_LOOTER_SHOOTER_API bool IsPlacedByProbe(EAirKind Kind);

	/** A point with Place suits Kind in View's light: motes in the sun by day; pollen by day and fluff by day or dusk over open grass. */
	AI_LOOTER_SHOOTER_API bool Belongs(EAirKind Kind, const FAirPlace& Place, const FAirView& View);

	/**
	 * How many of Kind are wanted out round View now, Density (0 to 1) of its cap at most: none sheltered; motes by day as
	 * much as the place is sunlit (more under a roof); pollen by day and fluff (fewer at dusk) as much as it's meadow;
	 * leaves as many trees as are near; moths at dusk, MothsPerLamp a lit lamp.
	 */
	AI_LOOTER_SHOOTER_API int32 Target(EAirKind Kind, const FAirView& View, const FAirSurroundings& Around, float Density);

	/** 1 well inside the reach, 0 at its edge; thinned too as it comes within a hand of the eye (cm). */
	AI_LOOTER_SHOOTER_API float DistanceFade(float Level, float Reach, float ToEye);

	/** In over its first second, out over its last one and a half. */
	AI_LOOTER_SHOOTER_API float AgeFade(float Age, float Life);

	/** Its own color (before the light, for the unlit ones). */
	AI_LOOTER_SHOOTER_API FLinearColor Albedo(EAirKind Kind);

	AI_LOOTER_SHOOTER_API const TCHAR* Name(EAirKind Kind);
}
