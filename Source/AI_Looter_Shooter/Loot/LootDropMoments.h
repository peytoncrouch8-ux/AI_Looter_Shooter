#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

/**
 * How a gun's drop plays out by its rarity (Phase 12.3: bigger drop moments; ULootFanfareSubsystem plays it, AWeaponBase's
 * beam takes its height from it). The rules: a taller beam in the rarity's colour each tier up; Rare and up a stinger
 * (Loot.Drop.<Rarity>) as it lands; Epic and up a spin and a ring of light along the ground as it lands; a Legendary's
 * stinger the moment it drops (felt before it's seen), and as it lands its burst (FLegendaryBurst: the user, 2026-10-27,
 * "There should be a galactic explosion instead of a game stutter"), a column of light seen from 60 m, its name popping up
 * over it, motes rising, and Loot.Drop.Legendary.Land. The world's clock is never touched (the 0.25 s slow at 0.3 the drop
 * had read to the user as the game stuttering). No lights (Medium's rule). Every number a Looter.Loot.Drop.* cvar
 * (LootDropMoments.cpp).
 */
struct AI_LOOTER_SHOOTER_API FDropMoment
{
	/** The steady loot beam (AWeaponBase::RefreshLootBeam): cm tall, glow, cm round. 0 tall: none (a Common). */
	float BeamHeight = 0.f;
	float BeamGlow = 2.f;
	float BeamRadius = 6.f;

	/** The landing's sting (Loot.Drop.<Rarity>; none for a Common), and whether it plays as it drops instead. */
	FName Stinger;
	bool bStingerAtDrop = false;
	/** Heard flat (wherever the player looks) rather than from where it fell. */
	bool bStingerFlat = false;
	/** A second sound as it lands (a Legendary's impact). */
	FName LandCue;

	/** A spin as it lands: degrees a second at its start, easing back to the pickup's own over SpinSeconds. */
	float SpinRate = 0.f;
	float SpinSeconds = 0.f;

	/** A ring of light running out along the ground as it lands: to RingRadius cm over RingSeconds. 0: none. */
	float RingRadius = 0.f;
	float RingSeconds = 0.f;

	/** Its burst as it lands (a Legendary's: LootDropMoments::Burst has its numbers). */
	bool bBurst = false;

	/** A column of light over it once it lands (as long as it lies there): cm tall, cm round, glow. 0 tall: none. */
	float ColumnHeight = 0.f;
	float ColumnRadius = 0.f;
	float ColumnGlow = 0.f;

	/** Its name popping up over it as it lands, for this long (s). 0: none. */
	float NamePopSeconds = 0.f;

	/** Motes of its colour rising round it as it lands. */
	int32 Motes = 0;
};

/**
 * A Legendary's landing burst, in its colour, all code-drawn light (FWeaponFX: shapes and streaks, no lights, nothing
 * Nanite), read from 40 m: a white crack and a star of its colour over it, a glowing core, two rings racing out over the
 * ground and a third tilted like a galaxy's disc, a shock facing the eye, and star sparks: spiral arms of them swirling out
 * round it and a fountain of them thrown up and falling (ULootFanfareSubsystem draws the sparks each frame: a short trail
 * and a twinkling head, never thinner on screen than SparkMinAngle). Sizes in cm and seconds; every one a
 * Looter.Loot.Drop.Burst.* cvar.
 */
struct AI_LOOTER_SHOOTER_API FLegendaryBurst
{
	/** Every size times this (0: no burst), every strength times Glow; its heart this high over the ground. */
	float Scale = 1.f;
	float Glow = 1.f;
	float Lift = 120.f;

	/** The crack: a white star this wide, gone in this long. Then a star of its colour, wider and slower. */
	float FlashSize = 700.f;
	float FlashSeconds = 0.16f;
	float StarSize = 1100.f;
	float StarSeconds = 0.4f;

	/** A ball of its light at the heart, blooming to this size and cooling over this long. */
	float CoreSize = 420.f;
	float CoreSeconds = 0.8f;

	/** The rings over the ground: the first fast, the second wider and slower. */
	float RingReach = 1400.f;
	float RingSeconds = 0.9f;
	float OuterReach = 2200.f;
	float OuterSeconds = 1.5f;

	/** The galaxy's disc: a ring tilted this far off the ground, and the shock in the air facing the eye. */
	float TiltReach = 900.f;
	float TiltSeconds = 0.75f;
	float TiltDegrees = 25.f;
	float ShockReach = 1000.f;
	float ShockSeconds = 0.3f;

	/** The spiral arms: how many, sparks in each, how far they swirl out, how far round they wind (turns), over how long. */
	int32 Arms = 3;
	int32 ArmSparks = 16;
	float ArmReach = 1300.f;
	float ArmTurns = 0.35f;
	float ArmSeconds = 1.4f;

	/** The fountain: sparks thrown up this fast (cm/s), falling back over this long. */
	int32 Fountain = 24;
	float FountainSpeed = 1500.f;
	float FountainSeconds = 1.6f;

	/** A spark: its trail's width (cm) and length (s of its flight), its head's size, and the least it shows (radians). */
	float SparkWidth = 6.f;
	float SparkTrail = 0.08f;
	float SparkHead = 18.f;
	float SparkMinAngle = 0.0025f;
};

namespace LootDropMoments
{
	/** The moment of a gun of Rarity, from the live cvars. */
	AI_LOOTER_SHOOTER_API FDropMoment For(EWeaponRarity Rarity);

	/** The moment of a gun of Rarity at the cvars' defaults (what the tests read). */
	AI_LOOTER_SHOOTER_API FDropMoment Defaults(EWeaponRarity Rarity);

	/** A Legendary's landing burst, from the live cvars; and at their defaults. */
	AI_LOOTER_SHOOTER_API FLegendaryBurst Burst();
	AI_LOOTER_SHOOTER_API FLegendaryBurst DefaultBurst();

	/** The Legendary's landing cue. */
	inline constexpr const TCHAR* LegendaryLandCue = TEXT("Loot.Drop.Legendary.Land");
}
