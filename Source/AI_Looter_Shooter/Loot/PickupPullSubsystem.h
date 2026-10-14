#pragma once

#include "CoreMinimal.h"
#include "Loot/PickupPull.h"
#include "Subsystems/WorldSubsystem.h"
#include "PickupPullSubsystem.generated.h"

class AActor;
class APawn;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The last metre of a pickup that counts the moment it's reached (ammo run past: AAmmoPickup::CollectInto): the pickup
 * itself goes at once, as it always has, and a copy of its model flies on into the player along a low arc, spinning
 * up and shrinking as it arrives, with a glint as it sets off and a faint trail (PickupPull). Looks only: what it gave
 * was given already, and when. Only for a living player within FlightRange of the body and along a clear line, never
 * through a wall; otherwise the pickup simply vanishes as before.
 *
 * A few mesh components on one transient actor, reused (at most MaxFlights at once: past that, a pickup just vanishes);
 * it ticks only while something flies.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPickupPullSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UPickupPullSubsystem* Get(const UObject* WorldContextObject);

	/**
	 * Flies a copy of Mesh from From (the pickup's model as it was) into Player, Pickup (going now) left out of the line
	 * of sight. True when it flies; false for no mesh or player, a dead player, one out of reach or behind something, or
	 * every flight in use.
	 */
	bool FlyIn(UStaticMesh* Mesh, const FTransform& From, APawn* Player, const AActor* Pickup, const FLinearColor& GlintColor);

	/** Moves every flight on (the tick calls it; a test level, which doesn't tick, calls it directly). */
	void Advance(float DeltaSeconds);

	/** Flights under way, and mesh components made so far (reused, never more than MaxFlights). */
	int32 NumFlying() const;
	int32 NumPooled() const { return Pool.Num(); }

	/** At most this many at once. */
	static constexpr int32 MaxFlights = 6;
	/** A flight starts only this near the body (cm from its surface): an ammo pickup's reach (180) and its size, with room. */
	static constexpr float FlightRange = 260.f;
	/** The flight: its first speed (cm/s), how fast it gains (cm/s^2), its most, and the longest it may take (s). */
	static constexpr float StartSpeed = 420.f;
	static constexpr float Acceleration = 5200.f;
	static constexpr float MaxSpeed = 2600.f;
	static constexpr float MostSeconds = 0.6f;
	/** It ends this much of its size, shrinking over the last part of the way. */
	static constexpr float EndScale = 0.35f;

	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FFlight
	{
		TWeakObjectPtr<APawn> Player;
		FPickupFlight Path;
		FVector Scale = FVector::OneVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector LastShown = FVector::ZeroVector;
		FLinearColor GlintColor = FLinearColor::White;
		bool bActive = false;
	};

	/** A free mesh component (made the first time one's needed), or INDEX_NONE when all are flying. */
	int32 FindFreeSlot();
	/** Hides a slot's mesh and frees it. */
	void EndFlight(int32 Slot);

	UPROPERTY(Transient)
	TObjectPtr<AActor> Holder;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Pool;

	/** One per pooled component, in the same order. */
	TArray<FFlight> Flights;
};
