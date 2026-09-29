#include "Loot/LootLibrary.h"
#include "Loot/AmmoPickup.h"
#include "Loot/LootTable.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Affixes/WeaponRollLibrary.h"
#include "AI_Looter_Shooter.h"
#include "Engine/World.h"

namespace
{
	const TCHAR* DefaultLootTablePath = TEXT("/Game/Weapons/Data/DA_LootTable_Default.DA_LootTable_Default");

	/** Pops loot upward and outward in a random direction so several drops fan out around the body. */
	FVector RandomTossVelocity()
	{
		const FVector Outward = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector();
		return Outward * FMath::FRandRange(150.f, 300.f) + FVector(0.f, 0.f, FMath::FRandRange(400.f, 550.f));
	}
}

ULootTable* ULootLibrary::GetDefaultLootTable()
{
	static TSoftObjectPtr<ULootTable> Default{ FSoftObjectPath(DefaultLootTablePath) };
	return Default.LoadSynchronous();
}

UWeaponDefinition* ULootLibrary::PickWeaponWith(const ULootTable* LootTable, FRandomStream& Random)
{
	if (!LootTable)
	{
		return nullptr;
	}

	float Total = 0.f;
	for (const FLootTableEntry& Entry : LootTable->Entries)
	{
		Total += Entry.Weapon ? FMath::Max(Entry.Weight, 0.f) : 0.f;
	}
	if (Total <= 0.f)
	{
		return nullptr;
	}

	float Pick = Random.FRand() * Total;
	UWeaponDefinition* Last = nullptr;
	for (const FLootTableEntry& Entry : LootTable->Entries)
	{
		if (!Entry.Weapon || Entry.Weight <= 0.f)
		{
			continue;
		}
		Last = Entry.Weapon;
		Pick -= Entry.Weight;
		if (Pick <= 0.f)
		{
			return Entry.Weapon;
		}
	}
	return Last;
}

EAmmoType ULootLibrary::PickAmmoType(const ULootTable* LootTable, FRandomStream& Random)
{
	float Total = 0.f;
	if (LootTable)
	{
		for (const FAmmoLootEntry& Entry : LootTable->AmmoTypes)
		{
			Total += LooterAmmo::IsValid(Entry.Type) ? FMath::Max(Entry.Weight, 0.f) : 0.f;
		}
	}
	if (Total <= 0.f)
	{
		const TConstArrayView<EAmmoType> Types = LooterAmmo::AllTypes();
		return Types[Random.RandHelper(Types.Num())];
	}

	float Pick = Random.FRand() * Total;
	EAmmoType Last = EAmmoType::AssaultRifle;
	for (const FAmmoLootEntry& Entry : LootTable->AmmoTypes)
	{
		if (!LooterAmmo::IsValid(Entry.Type) || Entry.Weight <= 0.f)
		{
			continue;
		}
		Last = Entry.Type;
		Pick -= Entry.Weight;
		if (Pick <= 0.f)
		{
			return Entry.Type;
		}
	}
	return Last;
}

FLootRoll ULootLibrary::RollLoot(const ULootTable* LootTable, int32 Level, float ExtraLuck, FRandomStream& Random)
{
	FLootRoll Roll;
	if (!LootTable)
	{
		return Roll;
	}

	// Ammo: most kills leave a box or two, each of a random class.
	if (Random.FRand() < LootTable->AmmoDropChance)
	{
		const int32 MinBoxes = FMath::Max(LootTable->MinAmmoDrops, 0);
		const int32 Boxes = Random.RandRange(MinBoxes, FMath::Max(LootTable->MaxAmmoDrops, MinBoxes));
		for (int32 Index = 0; Index < Boxes; ++Index)
		{
			FAmmoDrop& Drop = Roll.Ammo.AddDefaulted_GetRef();
			Drop.Type = PickAmmoType(LootTable, Random);
			Drop.Amount = LooterAmmo::GetInfo(Drop.Type).BoxAmount;
		}
	}

	// Weapons: only some kills. Rarity follows each weapon's own odds (Common most, Legendary least), shifted by luck.
	if (Random.FRand() < LootTable->WeaponDropChance)
	{
		const int32 MinWeapons = FMath::Max(LootTable->MinWeaponDrops, 0);
		const int32 Weapons = Random.RandRange(MinWeapons, FMath::Max(LootTable->MaxWeaponDrops, MinWeapons));
		for (int32 Index = 0; Index < Weapons; ++Index)
		{
			if (UWeaponDefinition* Definition = PickWeaponWith(LootTable, Random))
			{
				const EWeaponRarity Rarity = UWeaponRollLibrary::RollRarityWith(Definition, LootTable->Luck + ExtraLuck, Random);
				Roll.Weapons.Add(UWeaponRollLibrary::RollWeaponWithRarity(Definition, Rarity, Level));
			}
		}
	}
	return Roll;
}

TArray<AActor*> ULootLibrary::SpawnLoot(UObject* WorldContextObject, const ULootTable* LootTable, FVector Location, int32 Level, float ExtraLuck)
{
	TArray<AActor*> Spawned;
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || !LootTable)
	{
		return Spawned;
	}

	FRandomStream Random(FMath::Rand());
	const FLootRoll Roll = RollLoot(LootTable, Level, ExtraLuck, Random);
	const FVector SpawnLocation = Location + FVector(0.f, 0.f, 60.f);

	if (UE_LOG_ACTIVE(LogLooter, Verbose))
	{
		TArray<FString> Items;
		for (const FWeaponInstanceData& Instance : Roll.Weapons)
		{
			Items.Add(FString::Printf(TEXT("%s %s"), *UEnum::GetDisplayValueAsText(Instance.Rarity).ToString(), *GetNameSafe(Instance.Definition)));
		}
		for (const FAmmoDrop& Drop : Roll.Ammo)
		{
			Items.Add(FString::Printf(TEXT("%d %s"), Drop.Amount, LooterAmmo::GetInfo(Drop.Type).Name));
		}
		const AActor* Source = Cast<AActor>(WorldContextObject) ? Cast<AActor>(WorldContextObject) : WorldContextObject->GetTypedOuter<AActor>();
		UE_LOG(LogLooter, Verbose, TEXT("%s dropped: %s"), *GetNameSafe(Source), Items.IsEmpty() ? TEXT("nothing") : *FString::Join(Items, TEXT(", ")));
	}

	for (const FWeaponInstanceData& Instance : Roll.Weapons)
	{
		const FRotator SpawnRotation(0.f, FMath::FRandRange(0.f, 360.f), 0.f);
		if (AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(WorldContextObject, Instance, FTransform(SpawnRotation, SpawnLocation)))
		{
			Weapon->Toss(RandomTossVelocity());
			Spawned.Add(Weapon);
		}
	}

	for (const FAmmoDrop& Drop : Roll.Ammo)
	{
		if (AAmmoPickup* Pickup = AAmmoPickup::SpawnAmmo(World, Drop.Type, Drop.Amount, SpawnLocation))
		{
			Pickup->Toss(RandomTossVelocity() * 0.8f);
			Spawned.Add(Pickup);
		}
	}
	return Spawned;
}
