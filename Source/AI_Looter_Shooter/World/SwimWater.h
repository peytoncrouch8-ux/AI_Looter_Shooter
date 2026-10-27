#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SwimWater.generated.h"

/**
 * A body of water the player can swim in (round 4; note 30: "entering the pond up there, it is as if the water disappears and
 * there are no swimming or underwater effects"): a pond or a tarn as plain data, placed by Tools/Unreal/build_area_swimwater.py
 * from the layout's water features (one per pond: Ransom's Rest's Larkspur Tarn, the tutorial island's ponds and the Wallow's
 * pools). The terrain's water mesh is only its look (no collision: players wade through it); this says where the water is.
 *
 *  - The actor stands on the water's surface (its Z is the surface; GetSurfaceZ), in the middle of its Shore.
 *  - Shore: the water's outline seen from above (world XY, cm), a little outside the real waterline: inside it, the surface
 *    is the actor's height. Whether a place is deep enough to swim is the player's own rule (SwimRules: the water over the
 *    feet), so a generous shore costs nothing.
 *  - DeepZone: where the bed lies deeper than a swim's start (measured by the build script along the real bed). Creatures
 *    never follow a player into it: USwimWaterSubsystem::IsDeepAt makes such a player off their ground (watched from the
 *    shore), and a prowling creature stops at its edge. Empty: a wading pool, nowhere to swim.
 *
 * No tick, no collision, nothing drawn: it registers with USwimWaterSubsystem as play begins.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API ASwimWater : public AActor
{
	GENERATED_BODY()

public:
	ASwimWater();

	/** The water's outline seen from above (world XY, cm), in order around it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swim Water")
	TArray<FVector2D> Shore;

	/** Where it's deep enough to swim (world XY, cm); empty for a wading pool. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swim Water")
	TArray<FVector2D> DeepZone;

	/** The deepest the bed lies under the surface (cm), as the build measured it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swim Water", meta = (ClampMin = "0"))
	float BedDepth = 0.f;

	/** The layout feature it was made from (larkspurTarn, pond, wallow_pool_1, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swim Water")
	FName WaterId;

	/** The surface's height (world cm). */
	float GetSurfaceZ() const { return static_cast<float>(GetActorLocation().Z); }

	/** Inside the shore, seen from above. */
	bool ContainsXY(const FVector2D& Point) const;

	/** Inside the deep zone, seen from above. */
	bool IsDeepXY(const FVector2D& Point) const;

	/** The shore's box (world XY), for a quick look before the polygon. */
	const FBox2D& GetBounds() const { return Bounds; }

	/** Works out the box again (after Shore changes; BeginPlay does it). */
	void RefreshBounds();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FBox2D Bounds = FBox2D(ForceInit);
	FBox2D DeepBounds = FBox2D(ForceInit);
};
