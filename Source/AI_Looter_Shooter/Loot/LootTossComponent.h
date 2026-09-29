#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "LootTossComponent.generated.h"

/**
 * Throws loot (dropped weapons, ammo boxes) so it pops out, lands and stays near where it dropped. It bounces off walls
 * and steep rock like any projectile, but once it touches walkable ground it hops at most once more and then settles,
 * so loot never slides down a hillside. Give the thrown body collision that blocks only WorldStatic, so loot from one
 * drop doesn't land on each other.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API ULootTossComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

public:
	ULootTossComponent();

	/** Throws the component this moves (the owner's body) with the given velocity. */
	void Throw(const FVector& InVelocity);

	/** Surfaces at least this flat (impact normal Z) count as ground loot settles on. 0.7 is about 45 degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0", ClampMax = "1"))
	float WalkableFloorZ = 0.7f;

	/** Landing on ground with a bounce slower than this settles at once instead of hopping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	float SettleSpeed = 150.f;

protected:
	virtual void OnRegister() override;
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta) override;

private:
	/** What gets thrown. Kept here because the movement component forgets its updated component whenever it stops. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Body;

	int32 GroundImpacts = 0;
};
