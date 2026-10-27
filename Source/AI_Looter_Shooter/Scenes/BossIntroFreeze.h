#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * Holds actors still for a boss intro (Scenes/BossIntro.h): each one's own clock (AActor::CustomTimeDilation, which its
 * components' ticks run on too: a boss's fight clock, its brain, its moves, its pose) slowed to almost nothing, and given
 * back exactly as it was. An actor is held once however often it's asked; Release lets one go (the boss for its entrance),
 * ReleaseAll the rest. Whatever is gone by then is skipped.
 */
class AI_LOOTER_SHOOTER_API FBossIntroFreeze
{
public:
	~FBossIntroFreeze() { ReleaseAll(); }

	/** Holds Actor at Dilation (its own dilation remembered the first time). */
	void Hold(AActor& Actor, float Dilation);

	/** Holds every living creature (ACreatureBase) within Radius (cm) of Center; how many were newly held. */
	int32 HoldCreaturesNear(UWorld& World, const FVector& Center, float Radius, float Dilation);

	/** Gives Actor its own clock back (nothing if it isn't held). */
	void Release(const AActor* Actor);

	/** Gives every held actor its own clock back. */
	void ReleaseAll();

	bool IsHolding(const AActor* Actor) const;
	int32 Num() const { return Held.Num(); }

private:
	struct FHeld
	{
		TWeakObjectPtr<AActor> Actor;
		float Dilation = 1.f;
	};
	TArray<FHeld> Held;
};
