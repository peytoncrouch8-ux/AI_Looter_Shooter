#pragma once

#include "CoreMinimal.h"

class AActor;
class UClass;

/**
 * What a creature spills when a bullet hits it (FWeaponFX::SpawnCreatureHit), by its kind. One place, keyed by the creature's
 * class, so a new creature is given its look here (or falls back to ichor) without touching the creature's own code.
 */
enum class ECreatureBlood : uint8
{
	/** Not a creature: no splash of its own. */
	None,
	/** Spiders and the Venom Spitter: a yellow-green splash of ichor (the look every creature hit had before). */
	Ichor,
	/** The Gravemother: thick, dark ichor, more of it. */
	DarkIchor,
	/** Slimes: gel flung out in the slime's own color. */
	Gel,
	/** The Unpaid, the Bell-Ringer and Abel: embers in the coal's color and a breath of grave dust. */
	Embers,
	/** Dead miners and the Foreman: embers and grave dust, with rock dust and chips shaken out of them. */
	MinerEmbers
};

namespace CreatureBlood
{
	/** What a hit on Actor spills (None for anything that isn't a creature). */
	AI_LOOTER_SHOOTER_API ECreatureBlood Of(const AActor* Actor);

	/** What a hit on a creature of Class spills (its most specific kind wins: the Gravemother is a spider, but her own). */
	AI_LOOTER_SHOOTER_API ECreatureBlood OfClass(const UClass* Class);

	/**
	 * The color of what Actor spills: an Unpaid's (or a miner's) coal at its rank, a slime's gel as its material tints it,
	 * the ichor's for the spiders. The kind's own color when Actor says nothing.
	 */
	AI_LOOTER_SHOOTER_API FLinearColor TintOf(const AActor* Actor);

	/** The kind's own color (linear), when the creature doesn't say. */
	AI_LOOTER_SHOOTER_API FLinearColor DefaultTint(ECreatureBlood Blood);

	/** Whether its shots are venom (a glob hisses past; pellets and buckshot whiz): the spiders' and the slimes'. */
	AI_LOOTER_SHOOTER_API bool IsWet(ECreatureBlood Blood);

	/** The kind's name, for logs and tests. */
	AI_LOOTER_SHOOTER_API const TCHAR* Name(ECreatureBlood Blood);
}
