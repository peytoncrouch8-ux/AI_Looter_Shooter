#pragma once

#include "CoreMinimal.h"

class AActor;
class ASilverPickup;
class UObject;
class UWorld;
enum class EChestKind : uint8;
enum class ECreatureRank : uint8;
struct FRandomStream;

/**
 * Where silver comes from in the world (Docs/Progression.md, "Economy"): the loot code's hooks for a kill
 * (ULootDropComponent), a container (AChest) and a crate or barrel (ABreakableProp). Each rolls LooterEconomy's odds and
 * pops an ASilverPickup out as the ammo pops out. Silver drops where guns do: not in a practice area the player has left
 * once (Skyreach on a return visit, UAreaRulesSubsystem::DropsGunsAt), so practice never farms money.
 */
namespace SilverDrops
{
	/** Silver drops in WorldContextObject's world now (where guns do). */
	AI_LOOTER_SHOOTER_API bool DropsAt(const UObject* WorldContextObject);

	/**
	 * A kill's silver: rolled by rank and kind (LooterEconomy::KillOdds; bUnpaid for the Unpaid, Abel among them) at
	 * Level, popped out of Body a little lower and slower than the guns. Null when it rolled none or doesn't drop here.
	 */
	AI_LOOTER_SHOOTER_API ASilverPickup* DropForKill(const AActor& Body, ECreatureRank Rank, bool bUnpaid, int32 Level, FRandomStream& Random);

	/** A container's roll alone (LooterEconomy::ContainerOdds at Level), 0 for none or where it doesn't drop: the chest fans it out with the rest of its loot. */
	AI_LOOTER_SHOOTER_API int32 RollForContainer(const UObject* WorldContextObject, EChestKind Kind, int32 Level, FRandomStream& Random);

	/** A crate's or barrel's silver, now and then (LooterEconomy::BreakableOdds), popped up from From. Null for none. */
	AI_LOOTER_SHOOTER_API ASilverPickup* DropForBreakable(UWorld* World, int32 Level, const FVector& From, FRandomStream& Random);

	/** Spawns Amount at From and throws it with Velocity; null for none. */
	AI_LOOTER_SHOOTER_API ASilverPickup* Spill(UWorld* World, int32 Amount, const FVector& From, const FVector& Velocity);
}
