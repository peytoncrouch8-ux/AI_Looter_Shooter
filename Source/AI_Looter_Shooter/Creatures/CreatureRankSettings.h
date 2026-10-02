#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Creatures/CreatureRank.h"
#include "CreatureRankSettings.generated.h"

class ULootTable;

/** What one creature rank does: its tag, size, stats, loot and pack call. */
USTRUCT(BlueprintType)
struct FCreatureRankInfo
{
	GENERATED_BODY()

	/** The word its tag shows before the creature's name ("Restless"). None for Basic, and for bosses, whose names say it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tag")
	FText Word;

	/**
	 * The word's color: the rarity color of the odds the rank carries (a purple tag means purple odds), so it matches the
	 * loot beams. A tag with no word shows the creature's name in it instead. Basic's is the tag's plain text color.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tag")
	FLinearColor Color = FLinearColor::White;

	/** Its size against the creature's own (BodyScale): ranked creatures are a little bigger. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "0.25", ClampMax = "4"))
	float Size = 1.f;

	/** Health, attack damage and kill experience, as multiples of what the creature's class (or the level) gives it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.01"))
	float HealthMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float XPMultiplier = 1.f;

	/** Levels on top of the creature's own: a Restless spider placed at level 3 is level 4. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	int32 LevelOffset = 0;

	/**
	 * What it drops. None: the creature's own table (the default one, DA_LootTable_Default, unless it was given another).
	 * Until a rank's asset exists (Tools/Unreal/create_rank_assets.py makes them), a stand-in with the same odds is used.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TSoftObjectPtr<ULootTable> LootTable;

	/**
	 * When one of this rank is hurt, every creature of its pack (ACreatureBase::PackTag) this close turns on the attacker
	 * too, if that's farther than the creature's own PackAlertRadius (an Epic spider calls every spider within 30 m).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pack", meta = (ClampMin = "0", Units = "cm"))
	float PackCallRadius = 0.f;

	/**
	 * Whether a creature placed with this rank comes back after a death. Legendary monsters and bosses don't: their
	 * return is the area's to decide. Promotions don't count: a promoted creature comes back as it was placed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Life")
	bool bRespawns = true;
};

/**
 * The creature ranks, tunable without code: Project Settings > Game > Creature Ranks, saved to Config/DefaultGame.ini
 * under [/Script/AI_Looter_Shooter.CreatureRankSettings]. The defaults are the design's (Docs/Story.md, "Enemy ranks and
 * legendary drops"): Restless blue, Gravebound purple and Soulfed orange, each a little bigger, with loot tables whose
 * legendary odds per kill are 0.3%, 2.2%, 11.8%, 21.4% and 30.3% from Basic to Boss (Looter.Loot.RankOdds checks them).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Creature Ranks"))
class AI_LOOTER_SHOOTER_API UCreatureRankSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UCreatureRankSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Ranks")
	FCreatureRankInfo Basic;

	UPROPERTY(Config, EditAnywhere, Category = "Ranks")
	FCreatureRankInfo Rare;

	UPROPERTY(Config, EditAnywhere, Category = "Ranks")
	FCreatureRankInfo Epic;

	UPROPERTY(Config, EditAnywhere, Category = "Ranks")
	FCreatureRankInfo Legendary;

	UPROPERTY(Config, EditAnywhere, Category = "Ranks")
	FCreatureRankInfo Boss;

	/** What a rank does (the project's settings). */
	static const FCreatureRankInfo& Get(ECreatureRank Rank);

	/**
	 * The loot table a rank drops from: its asset, or for Basic with none set, the game's default table. A rank whose asset
	 * doesn't exist yet gets a stand-in with the design's odds and the default table's guns and ammo.
	 */
	static ULootTable* GetLootTable(ECreatureRank Rank);

	/** The rank's name in code ("Rare"), for logs and commands. */
	static FString GetRankName(ECreatureRank Rank);

	/** A rank from its name or its word, any case ("Epic", "gravebound"); false when it's neither. */
	static bool ParseRank(const FString& Text, ECreatureRank& OutRank);

	/** A rank table's odds as the design has them: what create_rank_assets.py writes, and the stand-ins use until then. */
	struct FRankLootOdds
	{
		float WeaponDropChance = 0.3f;
		int32 MinWeaponDrops = 1;
		int32 MaxWeaponDrops = 1;
		float Luck = 0.f;
		int32 MinAmmoDrops = 1;
		int32 MaxAmmoDrops = 2;
	};
	static FRankLootOdds DesignLootOdds(ECreatureRank Rank);

private:
	/** Stand-ins for rank tables whose assets don't exist yet, made once each (kept here so they live as long as the settings). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULootTable>> StandInTables;
};
