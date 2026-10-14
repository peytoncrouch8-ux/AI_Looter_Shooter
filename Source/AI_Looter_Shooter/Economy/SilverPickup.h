#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Loot/PickupPull.h"
#include "SilverPickup.generated.h"

class APawn;
class ULootTossComponent;
class URotatingMovementComponent;
class USphereComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Silver dropped as loot (Docs/Progression.md, "Economy"): a little pile of silver coins with the Inked icons' dark line
 * (SM_SilverCoins, from Art/Models/Loot/Coins.py; a flat disc stands in until it's imported), spinning under the small
 * white beam every ammo drop has. Kills, containers and crates pop it out (SilverDrops), and it lands and settles as the
 * ammo does.
 *
 * It comes to the player, Borderlands' way: once it has landed (or after CollectDelay), a living player whose body comes
 * within MagnetRadius, along a clear line (never through a wall: PickupPull), pulls it in, faster and faster, tossed in
 * a low arc with a glint as it sets off and a faint trail (FPickupFlight), and it's theirs as it reaches them: the
 * world's wallet takes its silver (UWalletSubsystem), a bright coin chime plays (Loot.Silver.Pickup) and the HUD's
 * readout flashes "+N". A wall coming between them on the way drops it where it is to wait again. It ticks only while
 * it's pulled; a timer looks for the player a few times a second meanwhile. Despawns after LifeSeconds.
 * The session keeps one lying on the ground (FSavedMapWorld::SilverPickups).
 *
 * Its motion is Advance, which the tick calls with the player; a test level, which doesn't tick, calls it directly.
 */
UCLASS(NotBlueprintable)
class AI_LOOTER_SHOOTER_API ASilverPickup : public AActor
{
	GENERATED_BODY()

public:
	ASilverPickup();

	/** Spawns Amount of silver at Location (not yet tossed); null for none. */
	static ASilverPickup* SpawnSilver(UWorld* World, int32 Amount, const FVector& Location);

	/** Throws it with the given velocity; it bounces and settles on whatever is below. */
	void Toss(const FVector& Velocity);

	int32 GetAmount() const { return Amount; }

	/** It's on its way to a player. */
	bool IsPulled() const { return bPulled; }

	/**
	 * Moves it on DeltaSeconds with Player (a living player's pawn, or null): it ages, a player near enough starts its
	 * pull (after CollectDelay), a pulled one flies to Player and is collected on reaching them. True when it was collected
	 * (and is gone).
	 */
	bool Advance(float DeltaSeconds, APawn* Player);

	/** Gives its silver to the world's wallet, chimes and goes. */
	void Collect();

	virtual void Tick(float DeltaSeconds) override;

	/** The coin pile's model (null until it's imported). */
	static UStaticMesh* FindModel();

	/** Seconds before an uncollected pile disappears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Silver", meta = (ClampMin = "0"))
	float LifeSeconds = 300.f;

	/** How near a player's body pulls it in (cm, from the body's surface): a little past an ammo pickup's two metres. */
	static constexpr float MagnetRadius = 400.f;

	/** It's theirs within this of the body (cm). */
	static constexpr float CollectDistance = 40.f;

	/** Seconds after it pops out before it can be pulled, so it's seen to drop first. */
	static constexpr float CollectDelay = 0.55f;

	/** The pull: its first speed (cm/s), how fast it gains (cm/s^2) and its most. */
	static constexpr float PullStartSpeed = 300.f;
	static constexpr float PullAcceleration = 3600.f;
	static constexpr float PullMaxSpeed = 2800.f;

	/** On its way it looks this often that nothing has come between it and the player (s). */
	static constexpr float LineCheckSeconds = 0.1f;

	/** The pile shows this much bigger than coins are, so it reads in the grass (about 13 cm across). */
	static constexpr float ModelScale = 1.6f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	/** Looks for a player near enough to pull it in (the timer). */
	void LookForPlayer();

	/** The nearest living player's pawn, or null. */
	APawn* FindPlayerPawn() const;

	/** Starts its pull toward Player: the toss stops, and it ticks until it's collected. */
	void StartPull(const APawn& Player);

	/** Lets go of a pull (the player fell, or a wall came between): it drops where it is and waits again. */
	void Drop();

	UFUNCTION()
	void HandleTossStopped(const FHitResult& ImpactResult);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Model;

	/** The ammo drops' thin white beam, on the root so it stands still while the coins spin. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Beam;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<ULootTossComponent> TossMovement;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<URotatingMovementComponent> Spin;

	int32 Amount = 0;
	float Age = 0.f;
	/** The pull's path, and how soon it looks again that nothing has come between it and the player (s). */
	FPickupFlight Flight;
	float LineCheckIn = 0.f;
	bool bPulled = false;
	bool bCollected = false;
	FTimerHandle LookTimer;
};
