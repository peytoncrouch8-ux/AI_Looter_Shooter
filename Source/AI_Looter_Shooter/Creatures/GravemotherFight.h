#pragma once

#include "CoreMinimal.h"
#include "Bosses/BossTypes.h"

/**
 * How the Gravemother fights in each phase of her boss fight (her UBossComponent's; Docs/Areas/RansomsRest.md, Side 3): as
 * plain data and rules, so the tests check them without a fight.
 *  - "Fresh Meat" (100-66%): her bite up close, her charge with its long telegraph and crack, and venom spat at a player
 *    who keeps away from her (a cone of slow pellets after she rears with a green glow at her fangs).
 *  - "The Brood Wakes" (66-33%): she screams and her first brood claws up round her; now she slams the ground at whoever
 *    hugs her (the Gravequake: a ring of cracks spreads out from under her through its wind-up, showing how far it reaches,
 *    then bursts), and charges a little more often.
 *  - "Mother's Fury" (33-0%): she screams, her second brood comes, and she's quicker in everything: a shorter (still long)
 *    telegraph, more charges, a wider spit, and the seam of each charge's crack burns a while after she's passed.
 */
namespace GravemotherFight
{
	/** How she fights in one phase: her own charge and chase scaled, the spit and the quake as they are then. */
	struct FPace
	{
		/** Her charge's cooldown and telegraph (and its aim, with it), and her chase, against her own. */
		float ChargeCooldownScale = 1.f;
		float ChargeTelegraphScale = 1.f;
		float ChaseScale = 1.f;

		/** Venom: seconds between spits, its pellets and its cone (degrees). */
		float SpitCooldown = 7.f;
		int32 SpitPellets = 5;
		float SpitSpread = 16.f;

		/** The Gravequake: whether she has it yet, and seconds between quakes. */
		bool bQuake = false;
		float QuakeCooldown = 9.f;

		/** Her charge's crack burns a while after her dash: standing on its seam hurts. */
		bool bBurningCracks = false;
	};

	/** Her three phases, highest share first, at her brood's calls (Gravemother::BroodCallShare): their names on her bar. */
	AI_LOOTER_SHOOTER_API TArray<FBossPhase> MakePhases();

	/** How she fights in Phase (0-2; past the last, the last's). */
	AI_LOOTER_SHOOTER_API FPace PaceFor(int32 Phase);

	/** Her venom: Pellets slow pellets in a Spread-degree cone from her fangs (the tell is her own rear and glow). */
	AI_LOOTER_SHOOTER_API FBossVolley Spit(int32 Pellets, float Spread);

	/** Her bar's show: her title, her death's cry and slow beat (her roars are her own moves, shaken by her own code). */
	AI_LOOTER_SHOOTER_API FBossShow MakeShow();

	/** Her weak spots (head and abdomen) stagger her: 7% of her health in crits within a few seconds. */
	AI_LOOTER_SHOOTER_API FBossStagger MakeStagger();

	/** Her loot's shower (the Legendary table's two guns and ammo, two boxes more), bursting from her body. */
	AI_LOOTER_SHOOTER_API FBossLootShowerSettings MakeLootShower();

	/** Seconds she'll go with nobody to fight (the player off her ground) before her fight stands down. */
	inline constexpr float StandDownSeconds = 6.f;
}
