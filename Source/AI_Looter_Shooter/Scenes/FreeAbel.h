#pragma once

#include "CoreMinimal.h"
#include "Story/StoryLine.h"

class AAbelKeeper;

/**
 * "Free Abel" (Main 7 "Given to the Wind", step 3; MAINS_2_7.md section 11, "Freeing him"): the scene after the last call at
 * the keeper's post, all in C++ with no logic in Sequencer, played by USceneSubsystem. Replaces "Sit with Pa" (2026-11-01).
 * Abel kneels where he fell, his black lantern gone into the canyon:
 *
 *  - The last of the smog boils off him into the fog; what's left is gold-white and scorched, but whole: Abel himself.
 *  - He looks at Sully, then at the keeper's post: "That's my lantern. You've been looking after it." / "I can't get home on
 *    my own. Will you carry me?" (DA_Lines_FreeAbel; the code's words until it's made).
 *  - He goes to the post and steps into his own lantern: he fades into it, and it burns like a sunrise.
 *  - Sully takes it down, lit, as the scene ends (AKeeperLanternPost::TakeDownLit).
 *
 * Sully's eyes are the camera (the player held and hidden where they stand), easing to the keeper's post and back. Skipped,
 * it leaves the world as played: Abel gone into his lantern, the lantern lit and carried. Main 7's scene step waits for it
 * (Scene.FreeAbel).
 */
namespace FreeAbel
{
	/** The scene's name: Main 7's step 3 waits for Scene.FreeAbel. */
	AI_LOOTER_SHOOTER_API FName SceneName();

	/** Its lines in order (Abel's two), the writer's set DA_Lines_FreeAbel when it's made, else these. */
	AI_LOOTER_SHOOTER_API TArray<FStoryLine> Lines();

	/** How long it runs when nothing is skipped (s). */
	AI_LOOTER_SHOOTER_API float Duration();

	/**
	 * Plays it with Abel as he kneels. False when nothing played: scenes are off (the missions hear it as played all the
	 * same; the caller puts the world in its end state) or another scene is playing (ask again later).
	 */
	AI_LOOTER_SHOOTER_API bool Play(AAbelKeeper& Abel);
}
