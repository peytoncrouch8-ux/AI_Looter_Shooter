#pragma once

#include "CoreMinimal.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/StoryLine.h"

class AColdOpenSet;

/**
 * The cold open (Docs/Story.md: Cold open; Docs/Areas/RansomsRest.md: Main 1, and Scope cuts, item 3: the cheap cold
 * open), a scene for USceneSubsystem::Play, all on its timeline with nothing in Sequencer. It plays on the story's first
 * arrival, after the first cast-off's ride (UColdOpenSubsystem), from the set the level has (AColdOpenSet):
 *  1. Seven days ago. Behind the white the arrival holds, the level goes to dusk and the view goes onto the gang's skiff's
 *     deck, inside a bank of evening cloud in the west. REVENANT rises through the white and the white thins on the
 *     skiff gliding out of the cloud, moving as the packet skiff was, the player free to look around; the gang stands on
 *     deck in silhouette. It glides low over the plains, drops into Gravewind Canyon and comes up it toward the Mooring
 *     Ledge under Ransom's Point. The Deacon, at Ellis's shoulder: "Home, kid." Then "Ransom's Rest. Seven days ago." Fade.
 *  2. Dusk on Ransom's Point, through Ellis's eyes: the gang on the lookout's deck, the Deacon with Saint Ada's ember. The
 *     chapel bell. "Put her back, Deacon. Not here. Not my home." Abel calls "El!" from the bluff path; Ellis turns to the
 *     rail; Lucky Ned fires; Abel falls and his lantern rolls off the path into the dark. Ellis turns back and draws; the
 *     Deacon is faster: "I'm sorry, kid." The shot, and the sky turns over: lying on the boards, Ellis sees a tall man in
 *     a stovepipe hat sitting far along the railing, watching. Black.
 * A skip lands at the end, in black; the grave wake-up (GraveWake) comes next either way.
 */
namespace ColdOpen
{
	/** The scene ("ColdOpen": Main 1's first step waits for Scene.ColdOpen), and the lighting state it plays in. */
	AI_LOOTER_SHOOTER_API FName SceneName();
	AI_LOOTER_SHOOTER_API FName DuskState();

	/** When the skiff's beats come, as shares of the set's skiff time: the Deacon's line, the caption, the fade out. */
	inline constexpr float HomeShare = 0.33f;
	inline constexpr float SevenDaysShare = 0.48f;
	inline constexpr float SkiffFadeBeforeCut = 2.5f;
	inline constexpr float FadeSeconds = 2.f;

	/** The Point's beats, in seconds after the cut to it. */
	inline constexpr float CutFadeIn = 1.2f;
	inline constexpr float BellAt = 0.8f;
	inline constexpr float PutHerBackAt = 2.4f;
	inline constexpr float AbelRunsAt = 6.2f;
	inline constexpr float AbelRunSeconds = 3.f;
	inline constexpr float ElAt = 6.8f;
	inline constexpr float ToRailAt = 7.f;
	inline constexpr float ToRailSeconds = 1.f;
	inline constexpr float NedAimsAt = 8.6f;
	inline constexpr float NedFiresAt = 9.2f;
	inline constexpr float AbelFallSeconds = 0.7f;
	inline constexpr float LanternRollSeconds = 2.2f;
	inline constexpr float TurnBackAt = 10.4f;
	inline constexpr float TurnBackSeconds = 0.8f;
	inline constexpr float DeaconAimsAt = 11.f;
	inline constexpr float SorryAt = 11.4f;
	inline constexpr float DeaconFiresAt = 12.6f;
	inline constexpr float FallAt = 12.7f;
	inline constexpr float FallSeconds = 2.6f;
	inline constexpr float BlackAt = 18.f;
	inline constexpr float PointSeconds = 20.4f;

	/** How long the whole cold open runs with a skiff time of SkiffSeconds. */
	AI_LOOTER_SHOOTER_API float DurationFor(float SkiffSeconds);

	/** What it says, in order, each line with its speaker and seconds: its captions. */
	AI_LOOTER_SHOOTER_API TArray<FStoryLine> Lines();

	/**
	 * The cold open from Set, as a scene: Title rises through the white as it begins (REVENANT); Next runs once it has
	 * ended, played or skipped, with the player still held and the screen black (the wake-up takes over from there).
	 */
	AI_LOOTER_SHOOTER_API FScenePlay Make(USceneSubsystem& Scenes, AColdOpenSet& Set, const FText& Title, TFunction<void()> Next);
}
