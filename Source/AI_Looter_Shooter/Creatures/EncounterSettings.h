#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "EncounterSettings.generated.h"

class ACreatureBase;

/** The most creatures of one kind (and its children) alive at once in a level. */
USTRUCT(BlueprintType)
struct FEncounterClassCap
{
	GENERATED_BODY()

	/** The kind: a creature class, by path, so it can name one that isn't made yet (it caps nothing until it is). */
	UPROPERTY(EditAnywhere, Category = "Caps")
	TSoftClassPtr<ACreatureBase> CreatureClass;

	UPROPERTY(EditAnywhere, Category = "Caps", meta = (ClampMin = "1"))
	int32 MaxAlive = 12;
};

/**
 * The encounters' caps, tunable without code: Project Settings > Game > Encounters, saved to Config/DefaultGame.ini under
 * [/Script/AI_Looter_Shooter.EncounterSettings]. The defaults are the area's performance plan (Docs/Areas/RansomsRest.md):
 * at most 16 creatures of any kind within 80 m of the player, and at most 12 Unpaid at once. Spawners keep to them before
 * spawning (AEncounterSpawner); what a cap holds back waits its turn.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Encounters"))
class AI_LOOTER_SHOOTER_API UEncounterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEncounterSettings();

	/** At most this many creatures of any kind alive within NearPlayerRadius of the player; a spawn there waits for room. */
	UPROPERTY(Config, EditAnywhere, Category = "Caps", meta = (ClampMin = "1"))
	int32 MaxCreaturesNearPlayer = 16;

	/** How near the player (cm) MaxCreaturesNearPlayer counts. */
	UPROPERTY(Config, EditAnywhere, Category = "Caps", meta = (ClampMin = "100", Units = "cm"))
	float NearPlayerRadius = 8000.f;

	/**
	 * Kinds with a cap of their own across the level, whoever spawned them (spawners, a boss's adds, the console): the
	 * Unpaid's 12. A spawner's group can set its own (FEncounterGroup::MaxAliveOfClass).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Caps")
	TArray<FEncounterClassCap> ClassCaps;

	/** Nothing appears nearer the player than this (cm): a wave comes round a fight, not into the player's face. */
	UPROPERTY(Config, EditAnywhere, Category = "Spawning", meta = (ClampMin = "0", Units = "cm"))
	float MinSpawnDistanceFromPlayer = 800.f;

	// --- Packs (UCreaturePackComponent, PackRules): modest, readable behaviours, not tactics ---

	/**
	 * A pack chasing one player spreads its approaches this far apart (degrees a member, either side of the middle), at
	 * most FlankMaxDegrees: they come in on spread angles, each spiraling in from its own side, not in a line. 0: no flanking.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Packs", meta = (ClampMin = "0", ClampMax = "60", Units = "deg"))
	float FlankStepDegrees = 20.f;

	UPROPERTY(Config, EditAnywhere, Category = "Packs", meta = (ClampMin = "0", ClampMax = "75", Units = "deg"))
	float FlankMaxDegrees = 35.f;

	/** Within this (cm, at size 1) of its target a flanker runs straight at it. */
	UPROPERTY(Config, EditAnywhere, Category = "Packs", meta = (ClampMin = "0", Units = "cm"))
	float FlankReleaseDistance = 450.f;

	/** A creature's packmates count within this (cm): those it flanks with, and whether its pack is gone. */
	UPROPERTY(Config, EditAnywhere, Category = "Packs", meta = (ClampMin = "100", Units = "cm"))
	float PackRadius = 3000.f;

	/**
	 * A Basic creature of these kinds (and their children) whose pack is gone, hurt to RetreatHealthShare or less, may
	 * break off: RetreatChance, rolled once a life. The spiders and slimes, by path; the Unpaid don't (the dead never run,
	 * and a phase-step would carry one back at once).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Packs")
	TArray<TSoftClassPtr<ACreatureBase>> RetreatKinds;

	UPROPERTY(Config, EditAnywhere, Category = "Packs", meta = (ClampMin = "0", ClampMax = "1"))
	float RetreatHealthShare = 0.35f;

	UPROPERTY(Config, EditAnywhere, Category = "Packs", meta = (ClampMin = "0", ClampMax = "1"))
	float RetreatChance = 0.5f;

	/** It runs this long (s), this far away at most (cm, at size 1), at this share of its chase speed, then turns again. */
	UPROPERTY(Config, EditAnywhere, Category = "Packs", meta = (ClampMin = "0", Units = "s"))
	float RetreatSeconds = 2.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Packs", meta = (ClampMin = "0", Units = "cm"))
	float RetreatDistance = 700.f;

	UPROPERTY(Config, EditAnywhere, Category = "Packs", meta = (ClampMin = "0.1", ClampMax = "1.5"))
	float RetreatSpeedShare = 0.85f;

	/** No rank sting within this (s) of the last anywhere in the level: two Restless turning together sting once. */
	UPROPERTY(Config, EditAnywhere, Category = "Packs", meta = (ClampMin = "0", Units = "s"))
	float RankStingRest = 1.5f;

	/** Whether Class (or a parent of it) is one of RetreatKinds. */
	bool MayRetreat(const UClass* Class) const;

	/** The project's settings. */
	static const UEncounterSettings& Get();

	/**
	 * The cap on Class's kind: the entry for it or for its nearest parent that has one (0: none), and the class that cap
	 * counts (OutCountedClass: that entry's).
	 */
	int32 FindClassCap(const UClass* Class, const UClass** OutCountedClass = nullptr) const;
};
