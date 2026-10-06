#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "World/PlayableBoundary.h"
#include "SafeGround.generated.h"

class ULineBatchComponent;

/**
 * Ground where nothing hunts the player (Docs/Areas/RansomsRest.md, "Zones"): Delia's salt line round the farm, from the
 * start, and Main Street after Main 3. A creature chasing a player into one gives up and walks home, none takes a player
 * in one as its target (ACreatureBase, CreatureBaseHunting.cpp), and spawners never put a creature in one
 * (AEncounterSpawner). Seen from above: a polygon of corners in world X and Y (the area's build script sets them from its
 * layout's zone), or with fewer than three corners a circle round the actor. Its own story condition switches it on and
 * off, read as play begins and whenever the missions change (UEncounterSubsystem). It has no collision and nothing to
 * see: Looter.Encounter.Zones draws it. The design calls it a safe zone; the class is named apart from UMG's USafeZone,
 * since two reflected headers or classes can't share a name.
 *
 * From a build script (Python):
 *     zone = actors.spawn_actor_from_class(unreal.SafeGround, unreal.Vector(x, y, z))
 *     zone.set_editor_property('zone_id', 'MainStreet')
 *     zone.set_editor_property('corners', [unreal.Vector(x, y, ground_z), ...])   # cm, world space, in order
 *     when = unreal.StoryCondition()
 *     when.set_editor_property('after_missions', ['Main3'])
 *     zone.set_editor_property('active_when', when)
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASafeGround : public AActor
{
	GENERATED_BODY()

public:
	ASafeGround();

	/** Its name in logs and Looter.Encounter.Zones ("Farm"). None: the actor's name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone")
	FName ZoneId;

	/** Its outline seen from above (world, cm, corners in order either way round; Z is the ground's, for drawing only). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone")
	TArray<FVector> Corners;

	/** With fewer than three corners: a circle of this radius (cm) round the actor instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone", meta = (ClampMin = "0", Units = "cm"))
	float Radius = 0.f;

	/** When it's on. An empty condition: always (the farm). Main Street's: after Main3. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Safe Zone")
	FStoryCondition ActiveWhen;

	/** Whether a point lies inside it, seen from above, on or not. */
	UFUNCTION(BlueprintPure, Category = "Safe Zone")
	bool Contains(const FVector& Point) const;

	/** It's on now: its condition held when it last looked. */
	UFUNCTION(BlueprintPure, Category = "Safe Zone")
	bool IsActive() const { return bActive; }

	/** Reads its condition again and switches it on or off (the missions' changes call it; so can tests). */
	void RefreshStory();

	/** Reads Corners again: after changing them on a zone already playing. */
	void Rebuild();

	/** ZoneId, or the actor's name. */
	FName GetZoneId() const;

	/** The ground it covers (square cm). */
	double GetSurfaceArea() const;

	/** Draws its outline (on: green; off: dim) at knee and head height, with a post at each corner, as lines in BatchID. */
	void DrawZone(ULineBatchComponent& Lines, uint32 BatchID) const;

	virtual void PostLoad() override;
	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Its corners as plain geometry, as of the last Rebuild. */
	FPlayableBoundary Outline;
	bool bActive = false;
	bool bStoryApplied = false;
};
