#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"

class ACreatureBase;

/**
 * A pack's leader (FIGHTS.md section 2, 2026-10-28: "an elite in most fights"; an encounter group's bLeader marks it,
 * UCreaturePackComponent::SetLeader makes it one). A leader wears Rallying, and when it falls the packmates near it falter:
 * their turns go back, their wind-ups come to nothing, they stand Looter.Fight.LeaderFalter (ACreatureBase::OnPackLeaderDown),
 * a Basic spider or slime may break off (Looter.Fight.LeaderBreakChance), and the pack's frenzy and rallies stop for
 * Looter.Fight.LeaderLull (UFightDirectorSubsystem::IsInLeaderLull). The rules are plain functions the tests check without a
 * level; OnLeaderDown plays them in the leader's world.
 */
namespace PackLeaderRules
{
	/** What a packmate's break-off roll at its leader's fall looks at. */
	struct FBreakOffAsk
	{
		ECreatureRank Rank = ECreatureRank::Basic;
		/** Its kind may break off at all (UEncounterSettings::RetreatKinds: the spiders and slimes). */
		bool bAllowedKind = true;
		/** It's after a player (chasing or attacking). */
		bool bHunting = true;
		/** Already running (its pack's break-off). */
		bool bRetreating = false;
		bool bDead = false;
	};

	/** Everything but the dice says it may break off: Basic, of a kind that does, alive, hunting, not running already. */
	AI_LOOTER_SHOOTER_API bool WantsBreakOffRoll(const FBreakOffAsk& Ask);

	/** It breaks off with Roll (uniform 0-1) under Chance, if it may. */
	AI_LOOTER_SHOOTER_API bool BreaksOff(const FBreakOffAsk& Ask, float Roll, float Chance);

	/** What the leader's fall sees of another creature. */
	struct FPackmateLook
	{
		/** The leader itself (it falters at nothing). */
		bool bSelf = false;
		bool bAlive = true;
		/** Of its pack (ACreatureBase::SharesPackWith). */
		bool bSharesPack = true;
		/** A boss or a Legendary monster (its own fight), or held back from fighting: unshaken. */
		bool bOwnFight = false;
		/** Its distance from the leader (cm). */
		float Distance = 0.f;
	};

	/** Whether it falters at the leader's fall: alive, of its pack, not the leader, not its own fight, within Reach. */
	AI_LOOTER_SHOOTER_API bool Falters(const FPackmateLook& Look, float Reach);

	/** Two creatures' pack, as ACreatureBase::SharesPackWith reads it: the same pack tag, or with either untagged, the same class. */
	AI_LOOTER_SHOOTER_API bool SamePack(FName TagA, const UClass* ClassA, FName TagB, const UClass* ClassB);

	/**
	 * Leader fell (its pack component hears its death): the director notes it (the lull, its frenzy ended, the LeaderDown
	 * event), the Fight.LeaderDown sting plays (2D: the player hears the fight turn), and each packmate within
	 * Looter.Fight.LeaderReach answers (ACreatureBase::OnPackLeaderDown).
	 */
	AI_LOOTER_SHOOTER_API void OnLeaderDown(ACreatureBase& Leader);
}
