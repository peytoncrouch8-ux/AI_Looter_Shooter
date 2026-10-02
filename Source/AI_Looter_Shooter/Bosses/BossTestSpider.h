#pragma once

#include "CoreMinimal.h"
#include "Bosses/BossTypes.h"

class UBossComponent;
class UWorld;

/**
 * The test boss that shows the framework before Abel exists (build step 21, Looter.Boss.Test): a big Boss-rank brown
 * spider with three phases. At 50% it can't be hurt while a wave of three spiderlings lives; from 25% it spits volleys of
 * venom pellets every few seconds. A 15 m fog-wall ring closes round its spot. The tests fight the same boss.
 */
namespace BossTestSpider
{
	/** Its size against a brown spider's, and its toughness: twelve spiders' health before its level and rank. */
	inline constexpr float Size = 1.6f;
	inline constexpr float HealthScale = 12.f;
	inline constexpr float SealRadius = 1500.f;
	/** Phases it can be asked for (Looter.Boss.Test [phases]); three is the design's. */
	inline constexpr int32 MinPhases = 1;
	inline constexpr int32 MaxPhases = 5;
	inline constexpr int32 DefaultPhases = 3;

	/** The tag on the test boss, so a second Looter.Boss.Test replaces it. */
	AI_LOOTER_SHOOTER_API FName Tag();

	/**
	 * Its phases: "The Brood Stirs" from full health, "Under Silk" at 50% (untargetable while three spiderlings live, at
	 * most 45 s), "Venom Rain" at 25% (a five-pellet volley every 3 s); four adds "Old Hunger" at 75%, five "Last Legs" at
	 * 10% (volleys every 2 s). Fewer leave out the later ones.
	 */
	AI_LOOTER_SHOOTER_API TArray<FBossPhase> MakePhases(int32 PhaseCount = DefaultPhases);

	/** The spiderlings' wave and the volley, as its phases use them. */
	AI_LOOTER_SHOOTER_API FBossAddWave Brood();
	AI_LOOTER_SHOOTER_API FBossVolley VenomVolley();

	/** Sets a boss component up as the test boss: its name, phases and wall. */
	AI_LOOTER_SHOOTER_API void Configure(UBossComponent& Boss, int32 PhaseCount = DefaultPhases);

	/**
	 * Spawns the test boss standing on Feet, facing Yaw: a Boss-rank brown spider spawned in play (never back once killed)
	 * with its boss component set up. Its fight hasn't started (StartFight).
	 */
	AI_LOOTER_SHOOTER_API UBossComponent* Spawn(UWorld* World, const FVector& Feet, float Yaw, int32 PhaseCount = DefaultPhases);
}
