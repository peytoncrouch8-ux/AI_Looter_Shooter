#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * The states Looter.MenuShots (MenuShotDevCommands.cpp) photographs the inventory in, and how each is made: a realistic
 * loadout given through the weapon manager, the inventory opened through the HUD, and the screen driven by key presses sent
 * through Slate, as a player's keys arrive. It touches nothing inside the screen, so the same run photographs any version
 * of it (the "before" and "after" of a redesign). Developer builds only (the .cpp is empty in shipping).
 */
namespace MenuShotScene
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
		/** Runs once the picture is requested and its gap has passed; null: nothing. */
		void (*After)(UWorld&);
		/** Seconds from the picture's request to the next step's Begin. */
		float GapSeconds;
	};

	/** The pictures in order; the file of step N (from 1) is <NN>_<Name>.png. */
	TConstArrayView<FStep> Steps();

	/** The running game's world, or null. */
	UWorld* FindGameWorld();

	/** The player has a character with a weapon manager and a HUD: the scene can be set up. */
	bool IsPlayerReady(UWorld& World);

	/**
	 * The scene before the first picture: three guns equipped (a Rare rifle in hand, an Epic shotgun Named by its notches, a
	 * Blooded Uncommon rifle), eight in the backpack (a cursed Legendary iron first, rifles and shotguns of every rarity, the
	 * named Heirloom when it exists), ammo of every kind, the mouse parked off the screen's lists, the tutorial on a step
	 * that never asks for the inventory.
	 */
	void Prepare(UWorld& World);
}
