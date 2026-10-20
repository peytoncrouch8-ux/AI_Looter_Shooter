#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Creatures/CreatureRank.h"
#include "Templates/Function.h"
#include "CreatureRankSettings.generated.h"

class ULootTable;

/** What one creature rank does: its tag, size, stats, loot, spawns and pack call. */
USTRUCT(BlueprintType)
struct FCreatureRankInfo
{
	GENERATED_BODY()

	/** The word its tag shows before the creature's name ("Restless"). None for Basic, and for bosses, whose names say it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tag")
	FText Word;

	/**
	 * The tag's color (word and name): the tier's own (LooterUI::Color::Tier*). Since the five tiers (the user's call,
	 * 2026-10-20) it no longer has to match a gun rarity: Restless's blue and Gravebound's purple happen to, Hallowed's
	 * yellow and Ethereal's red are enemy-tier colours only. Basic's is the tag's plain text color.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tag")
	FLinearColor Color = FLinearColor::White;

	/** The glow its body shows in its tier's light (spider eyes, a slime's core, an Unpaid's coal, a miner's lamp): usually Color. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tag")
	FLinearColor GlowColor = FLinearColor::White;

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
	 * The share of each hit it shrugs off (0.35: a hit for 100 takes 65), after every other scale. A critical hit pierces
	 * half of it (the user's call, 2026-10-20): 100 on a crit takes 82.5. UHealthComponent applies it once, to bullets,
	 * blows, grenades and ember powers alike.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0", ClampMax = "0.9"))
	float Defense = 0.f;

	/** How many traits it gets on top (UCreatureTraitComponent::RollTraits): Gravebound 1, Hallowed 2, Ethereal 3. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0", ClampMax = "7"))
	int32 TraitCount = 0;

	/**
	 * What it drops. None: the creature's own table (the default one, DA_LootTable_Default, unless it was given another).
	 * Until a rank's asset exists (Tools/Unreal/create_rank_assets.py makes them), a stand-in with the same odds is used.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TSoftObjectPtr<ULootTable> LootTable;

	/**
	 * Hallowed and Ethereal: their kill's silver against a Gravebound one of its kind (LooterEconomy::KillOdds): x1.6, x2.5.
	 * Regular, Restless, Gravebound, Legendary monsters and bosses pay amounts of their own and leave it at 1.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	float SilverMultiplier = 1.f;

	/**
	 * The share of regular spawns (placed Basic creatures promoted on arrival, and spawners rolling by their area) that are
	 * this tier on Ransom's Rest, the first area, and by the late areas; an area between rises with its place in the
	 * campaign (UAreaDefinition::GetCampaignRise). 0 for Basic (whatever is left) and for ranks never rolled.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawns", meta = (ClampMin = "0", ClampMax = "1"))
	float SpawnChanceFirst = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawns", meta = (ClampMin = "0", ClampMax = "1"))
	float SpawnChanceLate = 0.f;

	/**
	 * At most this many of the tier alive at once in an area for a roll to make another (Gravebound 4, Hallowed 2,
	 * Ethereal 1): a roll past it steps down a tier. Placed ranks count but are never capped. 0: no cap.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawns", meta = (ClampMin = "0"))
	int32 MaxAlivePerArea = 0;

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
 * under [/Script/AI_Looter_Shooter.CreatureRankSettings]. The defaults are the design the user approved on 2026-10-20
 * (Docs/Story.md, "Enemy ranks and legendary drops"): five tiers, Regular, Restless (blue), Gravebound (purple), Hallowed
 * (yellow) and Ethereal (red), each bigger, tougher, rarer and better paid, with loot tables whose legendary odds per kill
 * are 0.3%, 2.2%, 5.5%, 13.3% and 21.3%; Soulfed (21.4%) and bosses (30.3%) above them (Looter.Loot.RankOdds checks them).
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
	FCreatureRankInfo Hallowed;

	UPROPERTY(Config, EditAnywhere, Category = "Ranks")
	FCreatureRankInfo Ethereal;

	UPROPERTY(Config, EditAnywhere, Category = "Ranks")
	FCreatureRankInfo Legendary;

	UPROPERTY(Config, EditAnywhere, Category = "Ranks")
	FCreatureRankInfo Boss;

	/**
	 * An area's place in the campaign, for the tiers' spawn chances, read from its creatures' lowest level (the bands rise
	 * along the story): FirstAreaLevel (Ransom's Rest's 1) has SpawnChanceFirst, LateAreaLevel and above SpawnChanceLate,
	 * straight between (CampaignRise; UAreaDefinition::GetCampaignRise).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Spawns", meta = (ClampMin = "1"))
	int32 FirstAreaLevel = 1;

	UPROPERTY(Config, EditAnywhere, Category = "Spawns", meta = (ClampMin = "2"))
	int32 LateAreaLevel = 50;

	/** What a rank does (the project's settings). */
	static const FCreatureRankInfo& Get(ECreatureRank Rank);

	/**
	 * The loot table a rank drops from: its asset, or for Basic with none set, the game's default table. A rank whose asset
	 * doesn't exist yet gets a stand-in with the design's odds and the default table's guns and ammo.
	 */
	static ULootTable* GetLootTable(ECreatureRank Rank);

	/** The rank's name in code ("Rare"), for logs and commands. */
	static FString GetRankName(ECreatureRank Rank);

	/** The name players know it by: its word ("Restless"), "Regular" for Basic, the code name for a boss (logs, commands). */
	static FString GetTierName(ECreatureRank Rank);

	/** A rank from its name or its word, any case ("Epic", "gravebound", "Regular" for Basic); false when it's neither. */
	static bool ParseRank(const FString& Text, ECreatureRank& OutRank);

	// --- Spawns: how often a regular spawn is each tier, and the caps ---

	/** How far along the campaign an area whose creatures start at MinLevel is: 0 at FirstAreaLevel (and below), 1 from LateAreaLevel on. */
	static float CampaignRise(int32 MinLevel);

	/** A tier's share of regular spawns at Rise (0 to 1): SpawnChanceFirst to SpawnChanceLate. 0 for Basic and for ranks never rolled. */
	static float SpawnChance(ECreatureRank Rank, float Rise);

	/**
	 * The tier a regular spawn is at Rise, from Roll (uniform in 0 to 1): the rarest first, each its share after the ones
	 * above it (Ethereal below its chance, Hallowed in the next, then Gravebound, then Restless), else Basic.
	 */
	static ECreatureRank PickTier(float Roll, float Rise);

	/**
	 * Rolled kept under the tiers' caps (MaxAlivePerArea): while AliveOf(its tier) has reached the cap, it steps down a
	 * tier. AliveOf counts the tier's living ones in the area (placed and spawned).
	 */
	static ECreatureRank KeepUnderCaps(ECreatureRank Rolled, TFunctionRef<int32(ECreatureRank)> AliveOf);

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
