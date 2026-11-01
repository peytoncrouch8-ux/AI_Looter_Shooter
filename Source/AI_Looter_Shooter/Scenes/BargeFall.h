#pragma once

#include "CoreMinimal.h"

class ABargeBoarding;
class ALamplighterShip;

/**
 * Main 6's last beat (MAINS_2_7.md section 10, step 7, scene BargeFall): from the Lamplighter's deck, about 12 s, skippable.
 * Sully safe aboard, the dead barge drifts west over the Rest raining soul coal off her keel, her back half burning; ahead
 * of her a black streak (what was Abel) comes down on the burial boards of Gravewind Point; she crosses the Rim and goes
 * down into Gravewind Canyon: a fireball off the canyon's floor, the boom across the valley, a smoke column rising from
 * below the Rim. Hob and Finch say what they see (DA_Lines_Main6Fall). The barge moves as ABargeBoarding::ApplyFallAt says;
 * played or skipped, it ends with her gone (FinishFall: Pilot.BargeFell, the wreck shown). W2's sortie plays it once the
 * player stands on her deck in step 7 and holds the ship still through it; Main 6's step 7 waits for Scene.BargeFall.
 */
namespace BargeFall
{
	/** "BargeFall": the missions hear Scene.BargeFall. */
	AI_LOOTER_SHOOTER_API FName SceneName();

	/** How long it runs when nothing is skipped (s): BargeRules::FallSeconds and the look back. */
	AI_LOOTER_SHOOTER_API float Duration();

	/**
	 * Plays it. Ship may be null (the view is then the held player's own eyes wherever they stand). False when nothing played
	 * (scenes off, another scene): the barge is left fallen at once and the missions are told the scene played.
	 */
	AI_LOOTER_SHOOTER_API bool Play(ABargeBoarding& Barge, ALamplighterShip* Ship);
}
