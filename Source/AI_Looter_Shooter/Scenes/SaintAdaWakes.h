#pragma once

#include "CoreMinimal.h"
#include "Story/StoryLine.h"

class AAbelOnBoard;
class AActor;
class AReliquaryLantern;
class UWorld;

/**
 * "Saint Ada Wakes" (Main 7, "Given to the Wind", step 5; MAINS_2_7.md section 11; lane MAIN67): the Rest's payoff, all in
 * C++ with no logic in Sequencer, played by USceneSubsystem once Abel's lantern is set on her Reliquary (AReliquaryLantern):
 *
 *  - Inside, close on the lid: Brother Finch steps back; the lantern flares and Abel's light pours out of it into her
 *    recess in a stream of motes; the coal under ash catches; Saint Ada blazes white-gold (AChapelReliquary's blaze), her
 *    bell sounding once in the tower by itself.
 *  - Outside, high over the knoll, looking west: a ring of light washes out over the valley and the soul smog burns off
 *    in a wave (the lighting state blends DuskSmog -> Dusk); across the Rest the Unpaid go quiet and sink into their
 *    graves; the barge's coal goes dark and is gone.
 *  - Toward Gravewind Point: the Gravewind lifts the six dead off the burial boards and the coal's freed souls with them, a
 *    river of lights going west into the sunset past the wreck's smoke (ASoulFollowers::Release).
 *  - Inside again: Abel stands by the Reliquary, himself, a soul in his keeper's coat (his AAbelOnBoard, standing, soul-lit);
 *    Delia has come up the north road and stands in the door. DA_Lines_SaintAdaWakes: "Ma. You came up the hill. Your
 *    knees." / "Hush. You held the door." / "I'll keep her now. From in here. She won't go out again." / "Warden. Whatever
 *    they took me for, there's more of it. Go and get them back."
 *
 * Sully's eyes are the scene's camera (the player held and hidden where they stand); the view comes back to them facing
 * Delia at the door, for the turn-in (card, then her words). Skipped, or with scenes off (FinishAtOnce), it leaves the world
 * as played: the flag Main7.SaintAdaAwake set (the Reliquary lit, Abel's soul there, the coal and the followers gone, the
 * shrouds off the boards), the light at clean dusk, the Unpaid near gone. Main 7's last step waits for Scene.SaintAdaWakes.
 */
namespace SaintAdaWakes
{
	/** "SaintAdaWakes": the missions hear Scene.SaintAdaWakes once it has played (or been skipped, or passed over). */
	AI_LOOTER_SHOOTER_API FName SceneName();

	/** How long it runs unskipped (s). */
	AI_LOOTER_SHOOTER_API float Duration();

	/** Its four lines (DA_Lines_SaintAdaWakes when made; these drafts otherwise), in order. */
	AI_LOOTER_SHOOTER_API TArray<FStoryLine> Lines();

	/** Plays it from the lantern set on her. False when nothing played (scenes off, or another playing): call FinishAtOnce. */
	AI_LOOTER_SHOOTER_API bool Play(AReliquaryLantern& Rest);

	/** Its end state without the scene, and the missions told it played (scenes off, the console). */
	AI_LOOTER_SHOOTER_API void FinishAtOnce(AReliquaryLantern& Rest);

	/**
	 * The world as the scene leaves it: Saint Ada awake (the flag), lit, Abel's soul by her, the lantern on her lid, the coal
	 * and its souls gone, the light at Dusk (bInstant: at once, else blended), the Unpaid within reach sunk. Safe to call
	 * twice.
	 */
	AI_LOOTER_SHOOTER_API void ApplyEndState(AReliquaryLantern& Rest, bool bInstant);

	/**
	 * The Unpaid round Center (cm) go quiet: held passive, nothing to shoot or be hurt by, a puff of their soul-light and the
	 * dissolve's sigh. With OutSinking they're handed back to be lowered into the ground (the scene does it over seconds, then
	 * they're gone); without, they're gone at once. Returns how many.
	 */
	AI_LOOTER_SHOOTER_API int32 QuietUnpaid(UWorld* World, const FVector& Center, float Radius, TArray<TWeakObjectPtr<AActor>>* OutSinking);

	// --- Who's in it (SaintAdaWakesWorld.cpp): each nearest the Reliquary, or null ---

	/** Abel's soul by the Reliquary (the AAbelOnBoard tagged Speaker_Abel nearest her). */
	AI_LOOTER_SHOOTER_API AAbelOnBoard* FindAbelSoul(const AReliquaryLantern& Rest);
	/** Delia at the chapel door (tagged Speaker_DeliaChapel), and Brother Finch at the Reliquary (Speaker_FinchChapel). */
	AI_LOOTER_SHOOTER_API AActor* FindDelia(const AReliquaryLantern& Rest);
	AI_LOOTER_SHOOTER_API AActor* FindFinch(const AReliquaryLantern& Rest);
	/** Where the six dead lie on the burial boards (round the deck's three lantern posts), and Gravewind Point's place. */
	AI_LOOTER_SHOOTER_API TArray<FVector> DeadOnTheBoards(const UWorld* World);
	AI_LOOTER_SHOOTER_API bool FindPoint(const UWorld* World, FVector& OutPoint);
	/** Her bell sounds once in the tower by itself (swung if its own rule lets it, else its toll alone): nobody rang it. */
	AI_LOOTER_SHOOTER_API void RingHerBell(UWorld* World);
}
