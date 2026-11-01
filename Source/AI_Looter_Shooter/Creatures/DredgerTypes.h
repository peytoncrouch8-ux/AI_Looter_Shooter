#pragma once

#include "CoreMinimal.h"
#include "DredgerTypes.generated.h"

/**
 * Vane's Dredgers (the user's pick, 2026-10-31: "Rifles + hose-men"; contract Docs/Work/2026-10-31-Wardens/CONTRACT.md,
 * DREDGER): the Company's men in rubber coats and gas masks. Each is one of two jobs, fixed by its class (ADredgerRifleman,
 * ADredgerHoseMan; ADredgerCreature::ClassFor). Values are only ever appended: spawners and saves may hold the byte.
 */
UENUM(BlueprintType)
enum class EDredgerRole : uint8
{
	/** Keeps 10-25 m, takes cover, peeks and fires bursts of real bullets, reloads, flanks in pairs. */
	Rifleman,
	/** Walks in and at 8 m latches the soul-hose: the player is pulled and drained until the line breaks. */
	HoseMan
};

/** What a rifleman is doing about its distance and cover (ADredgerRifleman, DredgerRiflemanCover.cpp). */
enum class EDredgerTactic : uint8
{
	/** Not fighting (calm, searching, noticing, held by a hit): the creature's own brain moves it. */
	None,
	/** On its way to the spot it chose (cover, or a spot in its band). */
	Advance,
	/** At a spot that covers it from its target: crouched behind it, rising to fire. */
	Covered,
	/** At a spot with no cover (none found): standing in its band, strafing a little. */
	Exposed,
	/** Its target came too close: backing away while it looks for cover farther off. */
	BackOff,
	/** Told to hold where it stands (the tender's deck: ADredgerCreature::SetHoldsGround). */
	Hold
};

/** Where the hose-man's soul-hose is (UDredgerHoseComponent). */
enum class EDredgerHoseState : uint8
{
	/** On the nozzle: only the feed hose from the tank shows. */
	Stowed,
	/** Thrown: its tip flying at the victim. */
	Flying,
	/** On the victim: pulling and draining. */
	Latched,
	/** Let go (missed, broken): whipping back to the nozzle. */
	Snapping
};

/** Why the soul-hose let go (DredgerRules::HoseBreak). */
enum class EDredgerHoseBreak : uint8
{
	None,
	/** The victim got the break range away (14 m). */
	Range,
	/** The line was cut by the world (a wall, a corner) a moment. */
	Blocked,
	/** The hose-man was staggered or knocked. */
	Stagger,
	/** His tank burst. */
	TankBurst,
	/** He died, or was held back from fighting. */
	OwnerDown,
	/** The victim died or is no longer one he may hunt (a safe zone, a scene). */
	VictimGone,
	/** Held too long: he lets go to breathe (and the fight moves on). */
	TooLong,
	/** The thrown tip missed. */
	Missed
};

/** A rifleman's distance keeping, as the rules choose it (DredgerRules::ChooseKeep). */
enum class EDredgerKeep : uint8
{
	/** Out past its band: its brain brings it in. */
	None,
	/** Too close: back away. */
	BackOff,
	/** In its band, walking to its spot. */
	ToSpot,
	/** At its spot. */
	Hold
};
