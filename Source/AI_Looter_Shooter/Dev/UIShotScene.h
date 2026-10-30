#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * The states Looter.UIShots (UIShotDevCommands.cpp) photographs the menus in, and how each is made. Two halves, one per
 * run of the game (Tools/uishots.ps1 runs both): on the menu map the main menu (arrived, Settings lit, the settings page
 * opened from it, the session picker), and on a level the pause menu (its list, a row of it marked, then its settings on
 * Graphics, a row focused, Audio, Interface, Controls) and the level-up popup (during its count, then holding). The screens are driven through their own key
 * handling (HandleNavKey, as a player's keys arrive) and the HUDs' own open and close calls, so the same run photographs
 * any version of them (a redesign's before and after). Developer builds only (the .cpp is empty in shipping).
 */
namespace UIShotScene
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

	/** Which half of the run a world is for. */
	enum class EHalf : uint8
	{
		/** Not ready yet (loading, no HUD or menu yet). */
		None,
		/** The main menu's map (AMainMenuHUD with its menu up). */
		Menu,
		/** A level being played (ALooterHUD with its gameplay HUD and a pawn). */
		Level,
	};

	/** The running game's world, or null. */
	UWorld* FindGameWorld();

	/** The half World is ready for, or None while it isn't ready for either. */
	EHalf FindHalf(UWorld& World);

	/** A half's pictures in order; step I's file is <FirstNumber + I>_<Name>.png. */
	TConstArrayView<FStep> Steps(EHalf Half);

	/** The number of a half's first picture: the menu's are 01-04, the level's follow them. */
	int32 FirstNumber(EHalf Half);

	/** Seconds between Prepare and the first step: the first frames, streaming, eye adaptation. */
	float WarmupSeconds(EHalf Half);

	/** The scene before the first picture: the mouse parked off every button, no menu left open. */
	void Prepare(UWorld& World, EHalf Half);

	/** The half's name for the log ("menu", "level"). */
	const TCHAR* HalfName(EHalf Half);
}
