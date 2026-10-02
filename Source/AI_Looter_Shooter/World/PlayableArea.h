#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/PlayableBoundary.h"
#include "PlayableArea.generated.h"

class UBoxComponent;
class ULineBatchComponent;

/**
 * The level's playable boundary, placed by the area's build script from its layout: a polygon of corners seen from
 * above, each edge closed or open. Open edges are where a drop is part of play (the Rim's lip, a deck, a falls): fall
 * recovery brings back whoever drops off one (UFallRecoverySubsystem). Closed edges run along the foot of rock too
 * steep to walk, and an invisible wall about 50 m tall stands behind each one as a backstop. The walls use the
 * PlayableBounds collision profile (Config/DefaultEngine.ini), which blocks walking pawns (the player and creatures)
 * and nothing else, so bullets, the camera, the minimap's bake and PCG's traces pass through them; and
 * LooterWorld::StaticGeometryParams skips the whole actor. The minimap dims what lies outside and draws the closed
 * edges as its boundary line.
 *
 * The game builds the walls from the properties as it starts; they never exist in the editor or in the saved level,
 * so a script sets only Corners and OpenEdges (and the wall settings if it wants others). With no playable area in the
 * level (the tutorial island), fall recovery and the minimap work as they always have. Looter.World.Bounds draws it,
 * in the game or in the editor.
 *
 * From a build script (Python), with heights from a trace onto the terrain:
 *     actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
 *     area = actors.spawn_actor_from_class(unreal.PlayableArea, unreal.Vector(0, 0, 0))
 *     area.set_editor_property('corners', [unreal.Vector(x, y, ground_z), ...])   # cm, world space, in order
 *     area.set_editor_property('open_edges', [False, True, ...])                  # one per edge
 */
UCLASS()
class AI_LOOTER_SHOOTER_API APlayableArea : public AActor
{
	GENERATED_BODY()

public:
	APlayableArea();

	/** The walls' collision profile (Config/DefaultEngine.ini): it blocks walking pawns and nothing else. */
	static const FName WallProfile;

	/**
	 * The boundary's corners in order around the area (either way round), in world space (cm): the actor's own
	 * transform doesn't move them. X and Y make the polygon. Z is the ground's height at the corner, which only the
	 * walls use: each runs from WallDepth below its lower corner to WallHeight above its higher one.
	 */
	UPROPERTY(EditAnywhere, Category = "Playable Area")
	TArray<FVector> Corners;

	/**
	 * One flag per edge: edge i runs from corner i to corner i + 1, and the last one back to corner 0. True marks an
	 * open edge, where a drop is part of play: no wall stands there, and fall recovery brings back whoever drops 5 m
	 * off it. Missing entries count as closed.
	 */
	UPROPERTY(EditAnywhere, Category = "Playable Area")
	TArray<bool> OpenEdges;

	/** How far the walls rise above the higher of their two corners (cm). */
	UPROPERTY(EditAnywhere, Category = "Playable Area|Walls", meta = (ClampMin = "100"))
	float WallHeight = 5000.f;

	/** How far the walls reach below the lower of their two corners (cm), so a dip along an edge is closed too. */
	UPROPERTY(EditAnywhere, Category = "Playable Area|Walls", meta = (ClampMin = "0"))
	float WallDepth = 1000.f;

	/** How thick the walls are (cm). */
	UPROPERTY(EditAnywhere, Category = "Playable Area|Walls", meta = (ClampMin = "10"))
	float WallThickness = 200.f;

	/** How far behind the closed edges the walls stand (cm); 0 puts their inner face on the edge. */
	UPROPERTY(EditAnywhere, Category = "Playable Area|Walls", meta = (ClampMin = "0"))
	float WallSetback = 0.f;

	/** The level's playable area: the first one with a usable boundary, or null when there is none (the tutorial island). */
	static APlayableArea* Find(const UWorld* World);

	/** Whether a point lies inside the boundary, seen from above (height doesn't count). */
	UFUNCTION(BlueprintPure, Category = "Playable Area")
	bool Contains(const FVector& Point) const;

	/** The boundary as plain geometry (world XY), as of the last time the properties were read. */
	const FPlayableBoundary& GetBoundary() const { return Shape; }

	/**
	 * Reads the properties again and builds the walls anew. The game does this as it starts; call it after changing
	 * the properties of an area that is already playing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Playable Area")
	void Rebuild();

	/** The walls Rebuild made, one per closed edge (none outside the game). */
	const TArray<TObjectPtr<UBoxComponent>>& GetWalls() const { return Walls; }

	/** Draws the boundary (closed edges orange, open ones cyan) and where its walls stand, as lines that stay in BatchID until it is cleared. */
	void DrawBounds(ULineBatchComponent& Lines, uint32 BatchID) const;

	virtual void PostLoad() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;

private:
	/** Where one wall stands, in world space. */
	struct FWallBox
	{
		int32 Edge = INDEX_NONE;
		FVector Center = FVector::ZeroVector;
		/** Half its length along the edge, half its thickness and half its height (cm). */
		FVector Extent = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
	};

	/** The boundary as the properties describe it right now. */
	FPlayableBoundary MakeShape() const;
	/** Where the walls stand for that boundary, with their heights from the corners. */
	TArray<FWallBox> MakeWallBoxes(const FPlayableBoundary& Outline) const;

	FPlayableBoundary Shape;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> Walls;
};
