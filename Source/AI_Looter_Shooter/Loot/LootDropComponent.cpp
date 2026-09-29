#include "Loot/LootDropComponent.h"
#include "Loot/LootLibrary.h"
#include "Combat/HealthComponent.h"
#include "GameFramework/Actor.h"

void ULootDropComponent::BeginPlay()
{
	Super::BeginPlay();

	// Anything killable without its own table (every creature, the target dummy, future enemies) uses the default one.
	if (!LootTable)
	{
		LootTable = ULootLibrary::GetDefaultLootTable();
	}

	if (bDropOnDeath)
	{
		if (UHealthComponent* Health = GetOwner()->FindComponentByClass<UHealthComponent>())
		{
			Health->OnDeath.AddDynamic(this, &ULootDropComponent::HandleOwnerDeath);
		}
	}
}

void ULootDropComponent::HandleOwnerDeath(AController* Killer)
{
	DropLoot();
}

TArray<AActor*> ULootDropComponent::DropLoot()
{
	return ULootLibrary::SpawnLoot(this, LootTable, GetOwner()->GetActorLocation(), Level, ExtraLuck);
}
