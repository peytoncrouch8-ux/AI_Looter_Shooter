#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LootDropComponent.generated.h"

class AController;
class ULootTable;

/**
 * Add to enemies, chests, etc. Drops loot (ammo, sometimes a weapon) from a table when the owner's HealthComponent
 * dies, or when DropLoot is called. Without a table of its own it uses the game's default loot table. On a death, the
 * ammo leans toward the class of the gun that made the kill.
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

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleOwnerDeath(AController* Killer);
};
