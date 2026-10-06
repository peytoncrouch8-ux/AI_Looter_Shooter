#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Templates/SubclassOf.h"
#include "EncounterGroup.generated.h"

class ACreatureBase;

/** How an encounter group's creatures get their ranks (EncounterRules::PickRank). */
UENUM(BlueprintType)
enum class EEncounterRankRoll : uint8
{
	/**
	 * The area's promotion chances, as placed creatures get them on an arrival (8% Restless and 2% Gravebound on Ransom's
	 * Rest: "plus 8% of spawns"); Basic in a level that is no area's.
	 */
	Area,
	/** The group's own chances (RareChance, EpicChance). */
	Chances,
	/** All of them one rank (Rank): "4, one of them Restless" is a group of three and a group of one, Fixed at Rare. */
	Fixed,
};

/**
 * One kind of creature an encounter spawner brings (AEncounterSpawner::Groups): any creature class, how many each wave,
 * their ranks, level and size, which waves it joins, and its kind's cap across the level. Everything it brings is
 * spawned in play (ACreatureBase::SpawnAtRuntime), so it never comes back once killed.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FEncounterGroup
{
	GENERATED_BODY()

	/** What it brings: any creature (a spider, a slime; the Unpaid from step 16). None: the group brings nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group")
	TSubclassOf<ACreatureBase> CreatureClass;

	/** How many each wave it joins brings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group", meta = (ClampMin = "1", ClampMax = "16"))
	int32 Count = 4;

	/** How their ranks are rolled: the area's promotions, the group's own chances, or one rank for all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group|Rank")
	EEncounterRankRoll RankRoll = EEncounterRankRoll::Area;

	/** The rank they all are, with RankRoll Fixed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group|Rank",
		meta = (EditCondition = "RankRoll == EEncounterRankRoll::Fixed", EditConditionHides))
	ECreatureRank Rank = ECreatureRank::Basic;

	/** With RankRoll Chances: the chance each is Restless (Rare). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group|Rank",
		meta = (ClampMin = "0", ClampMax = "1", EditCondition = "RankRoll == EEncounterRankRoll::Chances", EditConditionHides))
	float RareChance = 0.f;

	/** With RankRoll Chances: the chance each is Gravebound (Epic) instead, rolled first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group|Rank",
		meta = (ClampMin = "0", ClampMax = "1", EditCondition = "RankRoll == EEncounterRankRoll::Chances", EditConditionHides))
	float EpicChance = 0.f;

	/** Their own level, before their rank's. 0: from the area's band round the player's level, as placed creatures get theirs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group", meta = (ClampMin = "0"))
	int32 Level = 0;

	/** Their size (BodyScale): 0.45 makes spiderlings. 0 keeps the class's. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group", meta = (ClampMin = "0", ClampMax = "5"))
	float BodyScale = 0.f;

	/** Their health against their class's, before their level and rank multiply it: spiderlings have 0.2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group", meta = (ClampMin = "0.01"))
	float HealthScale = 1.f;

	/** The first wave it joins, counted from 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group|Waves", meta = (ClampMin = "1"))
	int32 FirstWave = 1;

	/** The last wave it joins (a Restless one with the second wave only: 2 and 2). 0: every wave from FirstWave on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group|Waves", meta = (ClampMin = "0"))
	int32 LastWave = 0;

	/**
	 * At most this many of its class alive at once in the level, counting every spawner's and everything else of the class
	 * (a boss's adds too). 0: the class's cap in Project Settings > Game > Encounters (the Unpaid's 12), if it has one.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Group", meta = (ClampMin = "0"))
	int32 MaxAliveOfClass = 0;
};
