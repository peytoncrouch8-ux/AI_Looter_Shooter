#pragma once

#include "CoreMinimal.h"
#include "Bounties/BountyTypes.h"
#include "Loot/Chest.h"
#include "BountyChest.generated.h"

class ABountyChest;

/** A bounty's strongbox gave its loot. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBountyChestLooted, ABountyChest& /*Chest*/);

/**
 * A won bounty's strongbox (Docs/Work/2026-10-22-Next/BOUNTY.md, "Rewards"): an AChest of kind Strongbox with the
 * bounty's own loot (BountyRules::ChestLoot: its tier's guns at a Strongbox's luck, two ammo pickups; the kind's own
 * silver roll), id Bounty_<board>_<serial>. Spawned in play where the fight ended (or at its arena's reward spot), it
 * drops in from ChestDrop over its spot, lands with a thud, dust and Bounty.Chest.Land, and can't be opened until it has
 * landed. Once its loot is out it tells the board (OnLooted), which forgets it was owed; until then the board keeps it
 * owed and stands it at its arena again on a later visit. Never placed in a level.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API ABountyChest : public AChest
{
	GENERATED_BODY()

public:
	ABountyChest();

	/** A strongbox for Owed on the ground at Spot (traced down to the ground there) facing Yaw, dropping in (bDrop) or standing. */
	static ABountyChest* SpawnFor(UWorld* World, FName BoardId, const FBountyChestOwed& Owed, const FVector& Spot, float Yaw, bool bDrop);

	virtual void Tick(float DeltaSeconds) override;
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;

	/** The bounty it's for. */
	int32 GetSerial() const { return Serial; }

	/** Still dropping in. */
	bool IsLanding() const { return DropLeft > 0.f; }

	/** Moves its drop on (the tick does; a test level never ticks, so tests call it). */
	void AdvanceDrop(float DeltaSeconds);

	/** Tells the board once its loot is out (the tick looks every frame it moves; the board looks too, and the tests). */
	void CheckLooted();

	FOnBountyChestLooted OnLooted;

private:
	void Land();

	int32 Serial = 0;
	/** How high over its spot it still is (cm), and how fast it falls (cm/s). */
	float DropLeft = 0.f;
	float DropSpeed = 0.f;
	FVector GroundSpot = FVector::ZeroVector;
	bool bLootTold = false;
};
