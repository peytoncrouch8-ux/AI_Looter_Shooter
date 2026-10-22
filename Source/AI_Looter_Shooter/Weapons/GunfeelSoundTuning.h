#pragma once

#include "CoreMinimal.h"
#include "Weapons/GunshotSound.h"

/**
 * Gunfeel, the sound side (Docs/Plan.md 12.1, Saved/Gunfeel/CONTRACT.md "GUNFEEL-SOUND"): which of a shot's layers play
 * (GunshotSound::PlayShot) and how loud, and what the shooter's surroundings make the land answer with.
 *
 * Every number is a Looter.Gunfeel.Sound.* console variable read through an accessor here each time it is asked, so the
 * player tunes it live in play. The rules are plain functions over numbers (Default holds the shipped ones) so tests drive
 * them without a world or a variable the player may have set.
 */
namespace GunfeelSound
{
	/** The numbers a variable starts at. */
	namespace Default
	{
		inline constexpr float Tail = 1.f;
		inline constexpr float Mech = 1.f;
		inline constexpr float Low = 1.f;
		inline constexpr float DistantFrom = 3000.f;
		inline constexpr float ProbeRange = 800.f;
	}

	/** How long a gun's look at its surroundings holds (s): a burst fires many shots from the same spot. */
	inline constexpr double ProbeSeconds = 0.25;

	// --- The live numbers (the console variables' current values) ---

	/** The tail's volume (the land answering), times this. */
	AI_LOOTER_SHOOTER_API float TailVolume();
	/** The shooter's own action (Mech) close to the ear, times this. */
	AI_LOOTER_SHOOTER_API float MechVolume();
	/** The shooter's own chest thump (Low), times this. */
	AI_LOOTER_SHOOTER_API float LowVolume();
	/** Someone else's shot farther than this from the listener (cm) is heard as the far one (Distant). */
	AI_LOOTER_SHOOTER_API float DistantFrom();
	/** How far the surroundings probe looks for a roof and walls (cm). */
	AI_LOOTER_SHOOTER_API float ProbeRange();

	// --- The rules ---

	/** What's round the shooter, for the tail: open land, walls close by (a street, a yard), or a room or a tunnel. */
	enum class ESurroundings : uint8
	{
		Open,
		Town,
		Inside,
	};

	/**
	 * The surroundings from the probe's hits: a roof over the shooter with walls on 2 or more sides is Inside; walls on 2
	 * or more sides, or a roof alone (a porch, an overhang, a bridge), is Town; anything less is Open. A roof alone isn't
	 * a room: a warehouse's tail under a porch roof or a ledge would be wrong.
	 */
	AI_LOOTER_SHOOTER_API ESurroundings Classify(bool bRoof, int32 WallSides);

	/** The tail layer for the surroundings. */
	AI_LOOTER_SHOOTER_API EGunshotLayer TailFor(ESurroundings Surroundings);

	/** Which of a shot's layers play, and the tail's. */
	struct FShotLayers
	{
		/** The punch (Weapon.<Kind>.Core): everyone hears it within DistantFrom. */
		bool bCore = false;
		/** The action and the chest thump: the shooter's own gun only. */
		bool bMech = false;
		bool bLow = false;
		/** The land's answer by the surroundings (Tail). */
		bool bTail = false;
		EGunshotLayer Tail = EGunshotLayer::TailOpen;
		/** The far shot, in place of the punch and the tail. */
		bool bDistant = false;
	};

	/**
	 * The layers of a shot: the shooter's own gun (bOwnGun: a locally controlled pawn's) plays the punch, the action, the
	 * chest thump and the tail; anyone else's plays the punch and the tail, or past DistantFromCm from the listener the far
	 * shot alone (its take carries its own field).
	 */
	AI_LOOTER_SHOOTER_API FShotLayers Choose(bool bOwnGun, float DistanceToListener, float DistantFromCm,
		ESurroundings Surroundings);

	/** Whether a gun's last look at its surroundings (at LastProbe, s; negative: never) is too old at Now. */
	AI_LOOTER_SHOOTER_API bool ProbeStale(double Now, double LastProbe, double MaxAge = ProbeSeconds);
}
