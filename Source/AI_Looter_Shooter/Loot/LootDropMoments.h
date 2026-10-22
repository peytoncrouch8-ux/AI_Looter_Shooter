#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

/**
 * How a gun's drop plays out by its rarity (Phase 12.3: bigger drop moments; ULootFanfareSubsystem plays it, AWeaponBase's
 * beam takes its height from it). The rules: a taller beam in the rarity's colour each tier up; Rare and up a stinger
 * (Loot.Drop.<Rarity>) as it lands; Epic and up a spin and a ring of light along the ground as it lands; a Legendary's
 * stinger the moment it drops (felt before it's seen), a 0.25 s slow-down of the world then (never inside another slow:
 * a boss's death, Sundown), and as it lands a column of light seen from 60 m, its name popping up over it, motes rising,
 * and Loot.Drop.Legendary.Land. No lights (Medium's rule). Every number a Looter.Loot.Drop.* cvar (LootDropMoments.cpp).
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

	/** The world slowed to SlowDilation for SlowSeconds of real time the moment it drops. 0 seconds: none. */
	float SlowSeconds = 0.f;
	float SlowDilation = 1.f;

	/** A column of light over it once it lands (as long as it lies there): cm tall, cm round, glow. 0 tall: none. */
	float ColumnHeight = 0.f;
	float ColumnRadius = 0.f;
	float ColumnGlow = 0.f;

	/** Its name popping up over it as it lands, for this long (s). 0: none. */
	float NamePopSeconds = 0.f;

	/** Motes of its colour rising round it as it lands. */
	int32 Motes = 0;
};

namespace LootDropMoments
{
	/** The moment of a gun of Rarity, from the live cvars. */
	AI_LOOTER_SHOOTER_API FDropMoment For(EWeaponRarity Rarity);

	/** The moment of a gun of Rarity at the cvars' defaults (what the tests read). */
	AI_LOOTER_SHOOTER_API FDropMoment Defaults(EWeaponRarity Rarity);

	/** Whether a drop may slow the world now: only from full speed (CurrentDilation 1), never on top of another slow. */
	AI_LOOTER_SHOOTER_API bool MaySlowWorld(float CurrentDilation);

	/** The Legendary's landing cue. */
	inline constexpr const TCHAR* LegendaryLandCue = TEXT("Loot.Drop.Legendary.Land");
}
