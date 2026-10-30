#pragma once

#include "CoreMinimal.h"
#include "Elements/LooterElement.h"

class UClass;

/**
 * Touched creatures (the user's call, 2026-10-29 night): rare elemental versions of the kinds already in the game, no new
 * models: a Smoldering spider (Cinder), a Live-wire slime (Thunder), a Rotted miner (Rot), a Salt-crusted Unpaid
 * (Wardbreak), a Haloed Bell-Ringer (Hallow). Immune to their element, weak to its counter, a ward of their element when
 * Warded, their loot leaning to that element's parts, the element's glow on the body, and the tag's "Cinder-touched" word.
 * ACreatureBase rolls it (CreatureBaseRank.cpp, the area's TouchedChance and MaxTouchedAlive); the rules are here.
 */
namespace CreatureTouched
{
	/** The element a kind is touched by (the most specific class first: a Bell-Ringer and a dead miner are built on the Unpaid). None: the kind is never touched (the Stone Sentinel, bosses' own kinds, placeholders). */
	AI_LOOTER_SHOOTER_API ELooterElement ElementFor(const UClass* Kind);

	/** What breaks a Touched creature of Element (its weakness): Cinder and Thunder <- Wardbreak, Rot <- Cinder, Wardbreak <- Thunder, Hallow <- Rot. */
	AI_LOOTER_SHOOTER_API ELooterElement CounterOf(ELooterElement Element);

	/** The tag's word: "Cinder-touched", "Hallow-touched", "Wardbreak-touched", "Thunder-touched", "Rot-touched"; empty for None. */
	AI_LOOTER_SHOOTER_API FString Word(ELooterElement Element);

	/**
	 * Whether a creature rolls Touched this life: Roll (0 to 1) under Chance, with fewer than MaxAlive Touched alive now.
	 * Plain numbers, for the tests.
	 */
	AI_LOOTER_SHOOTER_API bool Rolls(float Roll, float Chance, int32 AliveNow, int32 MaxAlive);
}
