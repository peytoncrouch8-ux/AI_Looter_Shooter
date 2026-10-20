#pragma once

#include "CoreMinimal.h"
#include "Bosses/BossTypes.h"
#include "Creatures/GravemotherTypes.h"

/**
 * How the Gravemother fights in each phase of her boss fight (her UBossComponent's; Docs/Areas/RansomsRest.md, Side 3), the
 * Barrow fight the user picked (2026-10-15), as plain data and rules, so the tests check them without a fight.
 *  - "Fresh Meat" (100-66%): her bite up close, the Furrow (the ground heaves along a line toward her target for 2 s, then
 *    she ploughs it, throwing up a berm each side), and the Grave Clod lobbed at a player who keeps away (shot in her grip
 *    it bursts on her and stuns her).
 *  - "The Brood Wakes" (66-33%): she screams and slams the floor: four mounds bulge and spiderlings dig out (a mound shot
 *    while it bulges kills its spiderling). Now the Gravequake for whoever hugs her, and a furrow and a clod a little sooner.
 *  - "Mother's Fury" (33-0%): her headstone cracks off if it still stands, four more mounds, and she runs a quarter faster
 *    (the user's +25%); furrows and clods come sooner still. Her tells never shorten (the user's timings).
 */
namespace GravemotherFight
{
	/** How she fights in one phase: her furrow's cooldown against her own, her clod's and quake's, her speed. */
	struct FPace
	{
		/** The Furrow's cooldown against her own. */
		float FurrowCooldownScale = 1.f;
		/** Seconds between clods. */
		float ClodCooldown = 7.f;
		/** The Gravequake: whether she has it yet, and seconds between quakes. */
		bool bQuake = false;
		float QuakeCooldown = 9.f;
		/** Her chase and her plough against her own: 1.25 in her last third. */
		float SpeedScale = 1.f;
	};

	/** Her three phases, highest share first, at her brood's calls (Gravemother::BroodCallShare): their names on her bar. */
	AI_LOOTER_SHOOTER_API TArray<FBossPhase> MakePhases();

	/** How she fights in Phase (0-2; past the last, the last's). */
	AI_LOOTER_SHOOTER_API FPace PaceFor(int32 Phase);

	/** Her bar's show: her title, her death's cry and slow beat (her roars are her own moves, shaken by her own code). */
	AI_LOOTER_SHOOTER_API FBossShow MakeShow();

	/** Her weak spots stagger her: 7% of her health in crits within a few seconds. */
	AI_LOOTER_SHOOTER_API FBossStagger MakeStagger();

	/** Her loot's shower (the Legendary table's two guns and ammo, two boxes more), bursting from her body. */
	AI_LOOTER_SHOOTER_API FBossLootShowerSettings MakeLootShower();

	/** What her bar calls out while a clod burst on her has her stunned. */
	AI_LOOTER_SHOOTER_API FText StunCallout();

	/** Seconds she'll go with nobody to fight (the player off her ground) before her fight stands down. */
	inline constexpr float StandDownSeconds = 6.f;

	/** A clod's flight (s) to a target Distance cm off: AtTwentyMetres to one 20 m off (the user's 1.5 s), quicker nearer (a
	 *  lob's time grows as the root of its reach), never under 0.8 s or over 1.7 s. */
	AI_LOOTER_SHOOTER_API float ClodFlightSeconds(float Distance, float AtTwentyMetres = 1.5f);

	/** How far along her Furrow's line (cm, of Length) the split has crawled TellTime seconds into the tell: none before
	 *  AimAt, all of it by CrawlEnd, steady between. */
	AI_LOOTER_SHOOTER_API float HeaveFront(float TellTime, float AimAt, float CrawlEnd, float Length);

	/** Her headstone's state at Share of its health: intact over CrackedAt, cracked over ShatteredAt, shattered over 0. */
	AI_LOOTER_SHOOTER_API EGravemotherStone StoneStateFor(float Share, float CrackedAt, float ShatteredAt);

	/** A berm's rise RiseSeconds long, Age seconds in: 0 under the ground to 1 standing, eased. */
	AI_LOOTER_SHOOTER_API float BermRise(float Age, float RiseSeconds);
}
