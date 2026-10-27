#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SwimWaterSubsystem.generated.h"

class ASwimWater;

/**
 * The level's swimmable waters (ASwimWater registers as play begins): which water a point is in, and whether it's deep there.
 * A handful per level, looked through in a line with a box check first: a lookup costs next to nothing, so the player asks
 * every frame and every creature five times a second.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API USwimWaterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static USwimWaterSubsystem* Get(const UObject* WorldContext);

	void Register(ASwimWater* Water);
	void Unregister(ASwimWater* Water);

	/**
	 * The water whose shore holds Point (seen from above) and whose surface is at most Above over it and at most Below under
	 * it (cm); null for none. A point over a bridge or a cliff high above a pond isn't in it.
	 */
	ASwimWater* FindAt(const FVector& Point, float Above, float Below) const;

	/**
	 * Point is in deep water (a water's deep zone, at most DeepReachAbove over its surface): creatures don't follow a player
	 * there, and a prowl stops at its edge (Creatures/CreatureBaseHunting.cpp, CreatureBasePursuit.cpp).
	 */
	static bool IsDeepAt(const UObject* WorldContext, const FVector& Point);

	/** How far over a surface a point (an actor's middle) still counts as in its deep water (cm). */
	static constexpr float DeepReachAbove = 60.f;
	/** How far under a surface a point counts as in the water at all, for IsDeepAt (cm): deeper than any pond here. */
	static constexpr float DeepReachBelow = 1500.f;

	int32 NumWaters() const { return Waters.Num(); }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	TArray<TWeakObjectPtr<ASwimWater>> Waters;
};
