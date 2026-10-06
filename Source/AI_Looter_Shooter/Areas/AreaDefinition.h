#pragma once

#include "CoreMinimal.h"
#include "Creatures/CreatureRank.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPath.h"
#include "AreaDefinition.generated.h"

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

	// --- Creatures: the level band and promotions (UAreaRulesSubsystem plays them) ---

	/**
	 * The lowest level of the area's creatures, before their rank's levels (1 on Ransom's Rest). Their level follows the
	 * player's inside the band, give or take one: a player who rushes meets the floor, one who comes back strong the
	 * ceiling. 0: no band, and creatures keep the level they were placed at (an area asset made before bands read so).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Creatures", meta = (ClampMin = "0"))
	int32 MinLevel = 0;

	/** The highest level of the area's creatures, before their rank's (10 on Ransom's Rest); below MinLevel reads as it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Creatures", meta = (ClampMin = "0"))
	int32 MaxLevel = 0;

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

	/**
	 * A creature's level here before its rank's: the player's level moved by Spread (-1, 0 or +1, rolled; a boss's is 0),
	 * kept inside the band. Without a band, OwnLevel: the level it was placed or spawned at.
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

	/** Lowercase letters and digits only: "Ransom's Rest" and "ransoms_rest" are both "ransomsrest". */
	static FString Simplify(const FString& Words);
};
