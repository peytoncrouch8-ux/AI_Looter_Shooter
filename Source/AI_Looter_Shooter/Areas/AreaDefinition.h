#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPath.h"
#include "AreaDefinition.generated.h"

class ACreatureBase;

/**
 * A stretch of an area whose creatures are of the same few levels (Docs/Progression.md, "Creature levels"): its outline
 * seen from above and its creatures' levels before their ranks'. An area's zones are looked through in order and the
 * first that holds a spot gives it its levels, so a small place listed first stands out of a bigger one listed after it.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FAreaLevelZone
{
	GENERATED_BODY()

	/** Its name in the area's layout (gameplay.levels.zones), for logs and tests ("sink"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Levels")
	FName Id;

	/** Its outline from above, in world cm (X north, Y east): three corners or more, or it holds nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Levels")
	TArray<FVector2D> Outline;

	/** Its creatures' lowest level before their ranks'. Each is rolled evenly from this to MaxLevel: a level of variety. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Levels", meta = (ClampMin = "1"))
	int32 MinLevel = 1;

	/** Its creatures' highest level before their ranks'; below MinLevel reads as it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Levels", meta = (ClampMin = "1"))
	int32 MaxLevel = 1;

	/** Its outline holds Point (seen from above). */
	bool Contains(const FVector2D& Point) const;

	/** Its highest level: MaxLevel, or MinLevel when that's above it. */
	int32 GetTop() const { return FMath::Max(MaxLevel, MinLevel); }
};

/**
 * A kind of creature at a set level in an area, never rolled: its boss or a Legendary monster, a level or two above the
 * ground it stands on once its rank's levels come on top.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FAreaKindLevel
{
	GENERATED_BODY()

	/** The kind (its subclasses too). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Levels")
	TSubclassOf<ACreatureBase> Kind;

	/** Its level before its rank's: Abel's 6 on Ransom's Rest, with his Boss rank's one on top, is level 7. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Levels", meta = (ClampMin = "1"))
	int32 Level = 1;
};

/**
 * One area of the world: a level the player can travel to (Skyreach, Ransom's Rest), where trips into it arrive, and
 * its rules. One data asset per area in /Game/Data/Areas, named DA_Area_<Id>; Tools/Unreal/create_area_assets.py makes
 * them. Sessions and console commands name an area by its id; the session picker and the station boards show its
 * DisplayName.
 *
 * Its map can be set before the level exists (Ransom's Rest's, until it's built): HasMap() says whether anyone can go
 * there yet. Without any area assets the game still runs: places are named after their level files, and Looter.Travel
 * takes a level instead.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API UAreaDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** "DA_Area_RansomsRest" is the area "RansomsRest". */
	static constexpr const TCHAR* AssetPrefix = TEXT("DA_Area_");

	/** Where the area assets are made and kept. */
	static constexpr const TCHAR* AssetFolder = TEXT("/Game/Data/Areas");

	/** The name people see: in the session picker and on the station boards ("Ransom's Rest"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	FText DisplayName;

	/**
	 * What words in a sentence call it, where that's shorter than DisplayName ("the Lily", for "The Gilded Lily"): the
	 * station board's "The line to the Lily isn't open yet." Empty: DisplayName.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	FText SpokenName;

	/** The area's level. A path rather than a reference, so it can name a level that isn't built yet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath Map;

	/**
	 * Where trips into the area arrive, by name, each starting with "Landing_" (AreaLandings): an actor in the level
	 * tagged with it, or a player start whose Player Start Tag it is. The first is where trips arrive unless they name
	 * another: the station's platform on Ransom's Rest, the jetty on Skyreach.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Travel")
	TArray<FName> Landings;

	/**
	 * Outside the story, for practice (Skyreach): its creatures give no experience, and once the player has left it for
	 * the first time they drop only ammo (GivesKillExperience, DropsGuns; UAreaRulesSubsystem plays them). Every station
	 * board lists it as "Skyreach (practice)" after the first cast-off.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	bool bPractice = false;

	/** The story mission the player's first arrival starts ("Main1"); None for none. A name until missions are data. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Story")
	FName OpeningMission;

	/** Order in lists (the station boards, Looter.Area.List): lower first, then by id. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	int32 SortOrder = 0;

	// --- Creatures: their levels and promotions (UAreaRulesSubsystem plays them) ---
	// A creature's level is where it stands, never the player's (the user's call, 2026-10-12): its kind's set level here
	// (KindLevels), else the levels of the zone it stands in (LevelZones), rolled; its rank's levels come on top. The
	// layout's gameplay.levels holds them (Tools/Unreal/create_area_assets.py writes them here).

	/**
	 * The lowest level of the area's creatures, before their ranks' levels (1 on Ransom's Rest, Skyreach's only level).
	 * Every level here is kept between it and MaxLevel. 0: no band, and creatures keep the level they were placed or
	 * spawned at (an area asset made before bands reads so).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Creatures", meta = (ClampMin = "0"))
	int32 MinLevel = 0;

	/** The highest level of the area's creatures, before their ranks' (6 on Ransom's Rest); below MinLevel reads as it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Creatures", meta = (ClampMin = "0"))
	int32 MaxLevel = 0;

	/**
	 * The area's stretches and their creatures' levels, rising along the story (Ransom's Rest: 1-2 on the bluff, 3-4 at
	 * the chapel and the fields, 5-6 on the deck). The first holding a spot gives it its levels; a spot in none is at the
	 * band's floor or one above.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Creatures")
	TArray<FAreaLevelZone> LevelZones;

	/** Kinds at a set level here, wherever they stand: the area's boss and Legendary monsters (Abel 6, the Gravemother 4). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Creatures")
	TArray<FAreaKindLevel> KindLevels;

	/**
	 * The chance a placed Basic creature is Restless (Rare) when the area's promotions roll on an arrival (8% on Ransom's
	 * Rest). A promotion lasts one life: it comes back as Basic.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Creatures", meta = (ClampMin = "0", ClampMax = "1"))
	float RarePromotionChance = 0.f;

	/** The chance it's Gravebound (Epic) instead (2% on Ransom's Rest). Legendary monsters are hand-placed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Creatures", meta = (ClampMin = "0", ClampMax = "1"))
	float EpicPromotionChance = 0.f;

	/** Its creatures take their levels from a band (MinLevel is set). */
	bool HasLevelBand() const { return MinLevel >= 1; }

	/** Killing its creatures gives experience: anywhere but a practice area, which is no road to levels. */
	bool GivesKillExperience() const { return !bPractice; }

	/**
	 * Its creatures may drop guns: anywhere but a practice area the player has already left once (bFirstCastOff), so a
	 * return visit stays practice and never a loot farm. The tutorial's own kills still drop them.
	 */
	bool DropsGuns(bool bFirstCastOff) const { return !(bPractice && bFirstCastOff); }

	/** The band's highest level: MaxLevel, or MinLevel when that's below it. */
	int32 GetBandTop() const { return FMath::Max(MaxLevel, MinLevel); }

	/** Level kept inside the band (at least 1 without one). */
	int32 KeepInBand(int32 Level) const;

	/** The zone holding Spot seen from above (the first listed that does), or null. */
	const FAreaLevelZone* FindLevelZone(const FVector& Spot) const;

	/**
	 * The levels a creature standing at Spot may be before its rank's: its zone's, or outside every zone the band's floor
	 * and one above; kept inside the band. Never the player's.
	 */
	void GetLevelsAt(const FVector& Spot, int32& OutLowest, int32& OutHighest) const;

	/** Kind's set level here (KindLevels, kept inside the band), or 0 when it has none. */
	int32 FindKindLevel(const UClass* Kind) const;

	/**
	 * A creature's own level here before its rank's, standing at Spot: Kind's set level if it has one, else Spot's levels
	 * (GetLevelsAt) picked by Roll (0 to 1, evenly). Asked only of an area with a band: where there's none, creatures keep
	 * the level they were placed or spawned at (UAreaRulesSubsystem::RollLevelIn).
	 */
	int32 PickLevel(const FVector& Spot, const UClass* Kind, float Roll) const;

	/**
	 * Loot that follows the player (a chest's guns, AChest::GetLootLevel): the player's level moved by Spread, kept inside
	 * the band; OwnLevel without a band. Creatures never use it: their level is where they stand (PickLevel).
	 */
	int32 LevelFor(int32 PlayerLevel, int32 Spread, int32 OwnLevel) const;

	/** An arrival here can promote creatures. */
	bool HasPromotions() const;

	/**
	 * The rank a placed Basic creature takes when the area's promotions roll, from Roll (uniform in 0 to 1): Epic below
	 * EpicPromotionChance, Rare in the RarePromotionChance after it, else Basic.
	 */
	ECreatureRank PickPromotion(float Roll) const;

	/** The area's id: the asset's name without its prefix ("RansomsRest"). */
	FName GetAreaId() const;

	/** The level's package name ("/Game/Maps/Lvl_RansomsRest"); empty when no map is set. */
	FString GetMapPackage() const;

	/** The level is set and in the game, so trips can go there. */
	bool HasMap() const;

	/** Where trips arrive unless they name another landing: the first one, or None (the level's own start). */
	FName GetDefaultLanding() const;

	/** The words name this area: its id, its asset name or its display name, ignoring case, spaces and punctuation. */
	bool IsNamed(const FString& Words) const;

	/** Every area asset in the project (loaded), by SortOrder, then id. Empty when there are none. */
	static TArray<UAreaDefinition*> LoadAll();

	/** The area the words name (IsNamed), or null. */
	static UAreaDefinition* FindByName(const FString& Words);

	/** The area played in this level ("/Game/Maps/Lvl_TutorialIsland" is Skyreach's), or null. */
	static UAreaDefinition* FindByMap(const FString& MapPackage);

	/**
	 * Kind's set level in the first area (by SortOrder) that gives it one, or 0: what the bestiary shows for a boss or a
	 * Legendary monster, whose level never depends on where it's met.
	 */
	static int32 FindKindLevelInAnyArea(const UClass* Kind);

	/** Lowercase letters and digits only: "Ransom's Rest" and "ransoms_rest" are both "ransomsrest". */
	static FString Simplify(const FString& Words);
};
