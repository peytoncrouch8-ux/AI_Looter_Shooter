#pragma once

#include "CoreMinimal.h"
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
	 * the first time they drop only ammo. Nothing enforces it yet (step 10 does).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	bool bPractice = false;

	/** The story mission the player's first arrival starts ("Main1"); None for none. A name until missions are data. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Story")
	FName OpeningMission;

	/** Order in lists (the station boards, Looter.Area.List): lower first, then by id. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	int32 SortOrder = 0;

	// --- Creatures (step 7) ---
	// The area's level band (its creatures' lowest and highest level) and the chances of a creature being promoted to
	// each rank go here, in an "Area|Creatures" category, for the level roll and the promotions rolled on arrival.

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
