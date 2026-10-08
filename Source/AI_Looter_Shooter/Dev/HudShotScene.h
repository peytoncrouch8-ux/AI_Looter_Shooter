#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * The states Looter.HudShots (HudShotDevCommands.cpp) photographs the gameplay HUD in, and how each is made: through the
 * game's own paths (damage, health, experience, guns, the console's boss and mission commands), never the HUD's insides, so
 * the HUD reacts as it does in play. Developer builds only (the .cpp is empty in shipping).
 */
namespace HudShotScene
{
	/** One picture: what to set up, how long to let it settle, and the moment (if it has one) the picture waits for. */
	struct FStep
	{
		/** The state, in the file's name (<NN>_<Name>.png). */
		const TCHAR* Name;
		/** Seconds after Begin when the picture is taken (as soon after as Ready allows). */
		float Seconds;
		/** Sets the state up; null: the scene stays as it is. */
		void (*Begin)(UWorld&);
		/** The picture waits for this to be true, for a little longer than Seconds at most; null: no wait. */
		bool (*Ready)(UWorld&);
		/** Runs once the picture is requested and its gap has passed, to put back what Begin changed; null: nothing. */
		void (*After)(UWorld&);
		/** Seconds from the picture's request to the next step's Begin. */
		float GapSeconds;
	};

	/** The pictures in order; the file of step N (from 1) is <NN>_<Name>.png. */
	TConstArrayView<FStep> Steps();

	/** The running game's world, or null. */
	UWorld* FindGameWorld();

	/** The player has a character with a weapon manager and health, a progression, and a HUD: the scene can be set up. */
	bool IsPlayerReady(UWorld& World);

	/** The scene before the first picture: level 1 at full health, the rifle in hand, the tutorial on its dummies step. */
	void Prepare(UWorld& World);
}
