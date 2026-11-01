#pragma once

#include "CoreMinimal.h"

class ABargeBoarding;
class ABargeFurnace;

/**
 * The twist of Main 6 (MAINS_2_7.md section 10, step 6, "Open the soul cage"): what comes out of the furnace isn't Abel. All
 * in C++, played by USceneSubsystem, about 7 s, skippable. Through Sully's eyes at the cage: its glass cracks and its light
 * spikes; the cage bursts; a black shape in a keeper's coat (Abel's body in soot-black, smog pouring off it, a coal heart
 * glowing behind it, a black lantern in one fist) comes at Sully and throws them across the hall (the view knocked back,
 * rolling, onto the floor); it tears out through the furnace hall's back wall and streaks west into the dusk; Vane on the
 * horns ("Oh, that's not good." / "Gentlemen, abandon my barge. Leave the Warden."); Sully gets up where they landed. Its
 * end, played or skipped, leaves the cage burst and the barge dying (ABargeBoarding::NoteCageBurst, BeginDying). Main 6's
 * step 6 waits for Scene.CageBurst after the held Interact on the cage.
 */
namespace CageBurst
{
	/** "CageBurst": the missions hear Scene.CageBurst. */
	AI_LOOTER_SHOOTER_API FName SceneName();

	/** How long it runs when nothing is skipped (s). */
	AI_LOOTER_SHOOTER_API float Duration();

	/**
	 * Plays it at Furnace's cage. False when nothing played (scenes off, another scene playing): the missions are told it
	 * played all the same (so step 6 never waits on it) and the caller leaves the end state.
	 */
	AI_LOOTER_SHOOTER_API bool Play(ABargeBoarding& Barge, ABargeFurnace& Furnace);
}
