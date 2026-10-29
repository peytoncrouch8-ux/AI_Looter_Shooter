#pragma once

#include "CoreMinimal.h"

/**
 * The cue names of the hit-reaction pass (HITS, 2026-10-28): the sounds a creature's stagger, wound, hat, snuffed or
 * popped body make on top of the hit sounds that already play. Plain names so a creature's code and the sound bank
 * (Art/Sounds/recipes/hit_feel.py) agree on one spelling; all are 3D cues played at the creature.
 */
namespace CreatureHitCue
{
	/** A spider's leg hit: a dry snap, 6 dB under the hit. */
	inline constexpr const TCHAR* SpiderLegCrack = TEXT("Creature.Spider.LegCrack");
	/** A spider staggered: a hiss-shriek, once per stagger. */
	inline constexpr const TCHAR* SpiderStagger = TEXT("Creature.Spider.Stagger");
	/** An Unpaid's hat knocked off (a felt thump and flap; the soft cloth flap of a shroud hit is in the same set). */
	inline constexpr const TCHAR* UnpaidHatOff = TEXT("Creature.Unpaid.HatOff");
	/** An Unpaid staggered: a choked gasp. */
	inline constexpr const TCHAR* UnpaidStagger = TEXT("Creature.Unpaid.Stagger");
	/** A wounded Unpaid's coal guttering (one-shot, about every 3 s, no loop). */
	inline constexpr const TCHAR* UnpaidGutter = TEXT("Creature.Unpaid.Gutter");
	/** An Unpaid snuffed by a coal crit kill. */
	inline constexpr const TCHAR* UnpaidSnuff = TEXT("Creature.Unpaid.Snuff");
	/** An Unpaid torn apart by a blast kill. */
	inline constexpr const TCHAR* UnpaidTear = TEXT("Creature.Unpaid.Tear");
	/** A slime staggered: a wet wobble. */
	inline constexpr const TCHAR* SlimeStagger = TEXT("Creature.Slime.Stagger");
	/** A slime popped by a core crit kill. */
	inline constexpr const TCHAR* SlimePop = TEXT("Creature.Slime.Pop");
	/** A killing hit that was far more than needed (overkill), layered on a blast kill's sound. */
	inline constexpr const TCHAR* HitOverkill = TEXT("Creature.Hit.Overkill");
}
