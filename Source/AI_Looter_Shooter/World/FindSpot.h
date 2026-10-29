#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryLine.h"
#include "World/FindRules.h"
#include "FindSpot.generated.h"

class USceneComponent;

/**
 * One find off the path (Docs/Work/2026-10-28-Core/FINDS.md): a hidden data actor at the find, saying what it is, what it
 * counts by and what it pays. UFindSubsystem does the rest: it registers each spot as play begins, hears its Source opened,
 * read or killed, and makes the moment (the flag Find.<FindId>, the banner, the cue, the silver, the named gun, the XP).
 * Until found it puts out the tells: two crows circling low over it and a glint off its brass now and then.
 *
 * No tick, no collision, nothing drawn. Placed by Tools/Unreal/build_area_finds.py (folder Gameplay/Finds, tag Find):
 *     spot = actors.spawn_actor_from_class(unreal.FindSpot, unreal.Vector(x, y, z))
 *     spot.set_editor_property('find_id', 'WidowsBox')
 *     spot.set_editor_property('kind', unreal.FindKind.CACHE)
 *     spot.set_editor_property('source', chest)            # the chest or readable actor
 *     spot.set_editor_property('glint_offset', unreal.Vector(0, 0, 40))
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AFindSpot : public AActor
{
	GENERATED_BODY()

public:
	/** The tag every find spot carries (the build script's clean-up, the level test). */
	static const FName FindTag;

	AFindSpot();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Saved as the story flag Find.<FindId> (FindRules::FlagFor): never rename one once placed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find")
	FName FindId;

	/** Its name on the banner ("The Widow's Strongbox"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find")
	FText DisplayName;

	/** The map's row it counts in: Farm & Point, West Road, Town & Boot Hill, South Fields, The Mountain (FindRules::RegionOrder). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find")
	FText Region;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find")
	EFindKind Kind = EFindKind::Cache;

	/**
	 * What finding it is about: a chest (found once its loot is out, not on the tap) or a readable (found when read, or used).
	 * None for an Elite (SourceTag says who).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find")
	TSoftObjectPtr<AActor> Source;

	/** A mission event that finds it too (optional): "Readable.Find_RangerTag". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find")
	FName SourceEvent;

	/** An Elite's creature tag: the player's kill of a creature carrying it finds it ("Find_Tollman"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find")
	FName SourceTag;

	/**
	 * The zone level its experience and silver are paid at; 0 (the default): the area's zone at the spot (its highest level),
	 * else 1 where the level is no area's.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find", meta = (ClampMin = "0"))
	int32 ZoneLevel = 0;

	/** Silver at level 1 (grown as kill silver grows), tossed out as an ASilverPickup at the source on the find; 0 none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find|Pays", meta = (ClampMin = "0"))
	int32 Silver = 0;

	/** A named gun given once with the find (DA_Named_<Id>: "StillWater"), dropped by the source; None for none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find|Pays")
	FName NamedGun;

	/** What Hob says once the banner is done (UCaptionSubsystem, queued). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find|Pays")
	TArray<FStoryLine> Lines;

	/** The glint's place, from the spot (cm, world axes): the brass that catches the sun. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find|Tells")
	FVector GlintOffset = FVector(0.0, 0.0, 40.0);

	/** It glints while unfound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find|Tells")
	bool bGlint = true;

	/** Crows circling low over it while unfound (0: none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find|Tells", meta = (ClampMin = "0", ClampMax = "6"))
	int32 Crows = 2;

	/**
	 * How high over the spot the crows circle (cm; they rise and dip a little either way of it). 400 since 2026-10-29: the
	 * old 700, on the roost beacon's sky, put them 10-19 m up, black specks against a bright sky from the farm yard.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find|Tells", meta = (ClampMin = "100", Units = "cm"))
	float CrowHeight = 400.f;

	/** Props that tell of it and go once it's found (the bolt-hole's lantern, the miser's coins): hidden then. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Find|Tells")
	TArray<TSoftObjectPtr<AActor>> Tells;

	/** The source actor, when it's loaded; null for none. */
	AActor* GetSourceActor() const;

	/** The glint's world place. */
	FVector GetGlintLocation() const { return GetActorLocation() + GlintOffset; }

	/** Where its rewards come out: the source's place while there is one, else the spot's. */
	FVector GetRewardLocation() const;

	/** The rules' view of it (id, name, region, kind). */
	FFindEntry ToEntry() const;

	/** Shows or hides its tell props (found: hidden; forgotten again: shown). */
	void SetTellsShown(bool bShown);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;
};
