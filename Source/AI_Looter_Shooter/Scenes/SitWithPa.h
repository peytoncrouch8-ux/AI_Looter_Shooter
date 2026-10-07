#pragma once

#include "CoreMinimal.h"
#include "Story/StoryLine.h"

class AAbelKeeper;

/**
 * "Sit with Pa" (Docs/Areas/RansomsRest.md, Main 6's last step and "At zero"): the scene after Abel's fight, all in C++ with
 * no logic in Sequencer, played by USceneSubsystem. Abel kneels where he fell, his coal an ember, facing Ellis:
 *
 *  - "...El? You came home." He tells it straight: Ned shot him, the Deacon wept, and a tall fella sat on the lookout rail
 *    and never lifted a finger. "Bring Saint Ada home, El. I'll wait."
 *  - He rises and goes to the keeper's post, raises his ghost light, and the Keeper's Lantern catches from it: lit, it
 *    leans north-east, over the ridges, toward Ned (AKeeperLanternPost::LightKeepersLantern).
 *  - He goes to his board and sits on it facing the sunset, where AAbelOnBoard takes his place for good.
 *  - Hob: "Well. I've seen worse reunions."
 *
 * Ellis's eyes are the camera (the player held and hidden where they stand), easing to the keeper's post and then to a view
 * over Pa's shoulder into the sunset, and back to the player's own eyes at the end. Skipped, it leaves the world as played:
 * the lantern lit and leaning, Abel on his board. Main 6's last step waits for it (Scene.SitWithPa).
 */
namespace SitWithPa
{
	/** The scene's name: Main 6's last step waits for Scene.SitWithPa. */
	AI_LOOTER_SHOOTER_API FName SceneName();

	/** Its lines in order, the doc's words where it gives them (Abel's three, Hob's) and drafts between. */
	AI_LOOTER_SHOOTER_API TArray<FStoryLine> Lines();

	/** How long it runs when nothing is skipped (s). */
	AI_LOOTER_SHOOTER_API float Duration();

	/**
	 * Plays it with Abel as he kneels. False when nothing played: scenes are off (the missions hear it as played all the
	 * same; the caller puts the world in its end state) or another scene is playing (ask again later).
	 */
	AI_LOOTER_SHOOTER_API bool Play(AAbelKeeper& Abel);
}
