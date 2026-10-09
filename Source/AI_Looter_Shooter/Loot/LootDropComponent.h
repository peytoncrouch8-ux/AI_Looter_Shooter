#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LootDropComponent.generated.h"

class AController;
class ASoulMotePickup;
class ULootTable;
struct FRandomStream;

/**
 * Add to enemies, chests, etc. Drops loot (ammo, sometimes a weapon) from a table when the owner's HealthComponent
 * dies, or when DropLoot is called. Without a table of its own it uses the game's default loot table. On a death, the
 * ammo leans toward the class of the gun that made the kill, a Greedy iron's kill rolls at its curse's extra luck, and a
 * practice area the player has already left once (Skyreach, UAreaRulesSubsystem::DropsGuns) drops only the ammo.
 * A creature's death may also leave soul-motes (DropSoulMotes), which heal; they are rolled apart from the table, so
 * the loot's own odds don't move, and they drop in a practice area too. So may a grave-salt grenade (AGrenadePickup, by
 * FThrowRules::DropChance for the creature's rank and the grenades the player carries), also apart from the table.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API ULootDropComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Rolls the table and spawns what it gives (weapons and ammo). */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	TArray<AActor*> DropLoot();

	/** Leave empty to use the default loot table. A ranked creature carries its rank's here (UCreatureRankSettings). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TObjectPtr<ULootTable> LootTable;

	/** Item level of dropped weapons. A creature keeps it at its own level (ACreatureBase::SetLevel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "1"))
	int32 Level = 1;

	/** Added on top of the loot table's luck (e.g. elite versions of an enemy). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0"))
	float ExtraLuck = 0.f;

	/** Automatically drop when the owner's HealthComponent reports death. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	bool bDropOnDeath = true;

	/**
	 * Whether a creature's death may leave soul-motes. Follows bDropOnDeath (a creature whose loot is turned off leaves
	 * none), except for a boss: its loot is thrown by its own shower, which turns bDropOnDeath off, and it leaves its
	 * motes all the same.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	bool bDropSoulMotes = true;

	/**
	 * Rolls and spawns the soul-motes the owner's death leaves, from Random (a seeded stream repeats it): by its rank,
	 * 12% Basic, 25% Restless, 50% Gravebound and above, always 2 to 3 from a boss (FRecoverySettings). Only creatures
	 * leave them. Returns what it dropped.
	 */
	TArray<ASoulMotePickup*> DropSoulMotes(FRandomStream& Random) const;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOwnerDeath(AController* Killer);
};
