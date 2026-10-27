#pragma once

#include "CoreMinimal.h"

/**
 * The Gravemother's playtest tuning (the user, 2026-10-27: "way too easy and die too quickly as far as progression goes",
 * "I can just sit above the sink and shoot her", "you need to break the tombstone on her back before you can damage her"),
 * every number a cvar (Looter.Gravemother.*).
 *
 * Her health and her hits against the fight as first tuned (300 health, a 12 bite, at a spider's level 1): the class's own
 * numbers carry the defaults (so the Ledger's page, read from the class, shows them); a cvar changed in play applies to
 * the next Gravemother spawned (AGravemotherCreature::BeginPlay scales by the cvar over its default).
 */
namespace GravemotherTuning
{
	/** The user's "~1.6x health" (Main's round 2 contract). */
	inline constexpr float DefaultHealthScale = 1.6f;
	/** "Harder hits": her bite, Furrow, Gravequake and Grave Clod all come from her bite's damage. */
	inline constexpr float DefaultDamageScale = 1.25f;
	/** Her headstone glows as the target this long after a shrugged hit (s), and this long as her fight starts. */
	inline constexpr float DefaultMarkSeconds = 1.6f;
	inline constexpr float DefaultMarkAtFightSeconds = 3.f;
	/** Hob's hint comes after this many shrugged hits of a kind (once a Gravemother; 0: never). */
	inline constexpr int32 DefaultHintAfter = 3;

	/** Looter.Gravemother.HealthScale, .DamageScale (never under 0.05). */
	AI_LOOTER_SHOOTER_API float HealthScale();
	AI_LOOTER_SHOOTER_API float DamageScale();
	/** Looter.Gravemother.StoneGuards: nothing hurts her while her headstone stands (1 on). */
	AI_LOOTER_SHOOTER_API bool StoneGuards();
	/** Looter.Gravemother.RimShrug: nothing from a shooter off her hunting ground hurts her or her stone (1 on). */
	AI_LOOTER_SHOOTER_API bool RimShrug();
	/** Looter.Gravemother.RimSlack: how far past her hunting ground (cm, out and up) a shooter still counts as on it, so a
	 *  jump on a berm or the ramp's first steps aren't shrugged; the rim is 12 m up. */
	inline constexpr float DefaultRimSlack = 300.f;
	AI_LOOTER_SHOOTER_API float RimSlack();
	/** Looter.Gravemother.StoneMarkSeconds, .StoneMarkAtFight (0: no mark). */
	AI_LOOTER_SHOOTER_API float MarkSeconds();
	AI_LOOTER_SHOOTER_API float MarkAtFightSeconds();
	/** Looter.Gravemother.ShrugHintAfter. */
	AI_LOOTER_SHOOTER_API int32 HintAfter();
}
