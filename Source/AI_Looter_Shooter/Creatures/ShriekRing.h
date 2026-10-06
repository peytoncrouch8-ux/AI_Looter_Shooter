#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShriekRing.generated.h"

class ACharacter;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * A Gravebound Unpaid's shriek, seen (Docs/Areas/RansomsRest.md, "Enemies by rank"): a ring of glowing dashes in the
 * shrieker's coal color that runs out over the ground from where it shrieked, then fades. When it passes the one it
 * shrieked at, it slows them (UMovementSlowComponent) for its seconds, once; out past its reach, or well above or below
 * it, they're spared. Opaque glow on one instanced mesh (M_StylizedSurface's, as the boss's fog wall): no translucency.
 * It stays where it was sent out whatever the shrieker does after, and goes by itself.
 */
UCLASS(NotPlaceable)
class AI_LOOTER_SHOOTER_API AShriekRing : public AActor
{
	GENERATED_BODY()

public:
	AShriekRing();

	/**
	 * Sends a ring out from Center (on the ground) to InMaxRadius in InColor; it slows InVictim to InSlowMultiplier of their
	 * speed for InSlowSeconds when it reaches them.
	 */
	static AShriekRing* Spawn(UWorld* World, const FVector& Center, float InMaxRadius, const FLinearColor& InColor, ACharacter* InVictim,
		float InSlowMultiplier, float InSlowSeconds);

	virtual void Tick(float DeltaSeconds) override;

	/** Moves it on by DeltaSeconds: it widens, slows its victim as it passes them, fades and goes. Its tick calls it; tests call it. */
	void Advance(float DeltaSeconds);

	float GetRadius() const { return Radius; }
	float GetMaxRadius() const { return MaxRadius; }

	/** It has reached its victim and slowed them. */
	bool HasSlowed() const { return bSlowed; }

	/** How fast it runs out (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shriek", meta = (ClampMin = "100", Units = "cm/s"))
	float Speed = 1800.f;

	/** It reaches a victim this far above or below its ground (cm, to their middle), and no farther. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shriek", meta = (ClampMin = "0", Units = "cm"))
	float ReachUpDown = 250.f;

private:
	/** Lays the dashes round the ring as it is now. */
	void Draw();

	UPROPERTY(VisibleAnywhere, Category = "Shriek")
	TObjectPtr<UInstancedStaticMeshComponent> Dashes;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> GlowBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Glow;

	TWeakObjectPtr<ACharacter> Victim;
	FLinearColor Color = FLinearColor::White;
	float MaxRadius = 900.f;
	float Radius = 0.f;
	float Age = 0.f;
	float SlowMultiplier = 0.5f;
	float SlowSeconds = 2.f;
	bool bSlowed = false;
};
