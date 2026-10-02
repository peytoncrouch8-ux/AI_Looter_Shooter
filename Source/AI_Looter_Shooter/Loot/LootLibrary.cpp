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

EAmmoType ULootLibrary::PickAmmoType(const ULootTable* LootTable, FRandomStream& Random, TOptional<EAmmoType> KillAmmo)
{
	// Each class's weight: what the table gives it (a class listed twice counts twice), or 1 each when it lists none.
	float Weights[LooterAmmo::NumTypes] = {};
	float Total = 0.f;
	if (LootTable)
	{
		for (const FAmmoLootEntry& Entry : LootTable->AmmoTypes)
		{
			if (LooterAmmo::IsValid(Entry.Type))
			{
				const float Weight = FMath::Max(Entry.Weight, 0.f);
				Weights[static_cast<int32>(Entry.Type)] += Weight;
				Total += Weight;
			}
		}
	}
	if (Total <= 0.f)
	{
		for (float& Weight : Weights)
		{
			Weight = 1.f;
		}
	}

	// The gun that made the kill finds its own ammo more often. Multiplying keeps a class the table never drops at zero.
	if (KillAmmo.IsSet() && LooterAmmo::IsValid(KillAmmo.GetValue()))
	{
		Weights[static_cast<int32>(KillAmmo.GetValue())] *= FMath::Max(LootTable ? LootTable->KillWeaponAmmoBias : 1.f, 1.f);
	}

	Total = 0.f;
	for (const float Weight : Weights)
	{
		Total += Weight;
	}
	float Pick = Random.FRand() * Total;
	EAmmoType Last = EAmmoType::AssaultRifle;
	for (const EAmmoType Type : LooterAmmo::AllTypes())
	{
		const float Weight = Weights[static_cast<int32>(Type)];
		if (Weight <= 0.f)
		{
			continue;
		}
		Last = Type;
		Pick -= Weight;
		if (Pick <= 0.f)
		{
			return Type;
		}
	}
	return Last;
}

int32 ULootLibrary::RollAmmoAmount(const ULootTable* LootTable, FRandomStream& Random)
{
	const int32 MinAmount = FMath::Max(LootTable ? LootTable->AmmoAmountMin : LooterLoot::KillAmmoAmountMin, 1);
	const int32 MaxAmount = FMath::Max(LootTable ? LootTable->AmmoAmountMax : LooterLoot::KillAmmoAmountMax, MinAmount);
	return Random.RandRange(MinAmount, MaxAmount);
}

FLootRoll ULootLibrary::RollLoot(const ULootTable* LootTable, int32 Level, float ExtraLuck, FRandomStream& Random, TOptional<EAmmoType> KillAmmo)
{
	FLootRoll Roll;
	if (!LootTable)
	{
		return Roll;
	}

	// Ammo: most kills leave a pickup or two, each of a random class (more often the kill weapon's) and a random amount
	// from the table's range, the same range for every class.
	if (Random.FRand() < LootTable->AmmoDropChance)
	{
		const int32 MinDrops = FMath::Max(LootTable->MinAmmoDrops, 0);
		const int32 Drops = Random.RandRange(MinDrops, FMath::Max(LootTable->MaxAmmoDrops, MinDrops));
		for (int32 Index = 0; Index < Drops; ++Index)
		{
			FAmmoDrop& Drop = Roll.Ammo.AddDefaulted_GetRef();
			Drop.Type = PickAmmoType(LootTable, Random, KillAmmo);
			Drop.Amount = RollAmmoAmount(LootTable, Random);
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
	return SpawnKillLoot(WorldContextObject, LootTable, Location, Level, ExtraLuck, {});
}

TArray<AActor*> ULootLibrary::SpawnKillLoot(UObject* WorldContextObject, const ULootTable* LootTable, FVector Location, int32 Level,
	float ExtraLuck, TOptional<EAmmoType> KillAmmo)
{
	TArray<AActor*> Spawned;
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || !LootTable)
	{
		return Spawned;
	}

	FRandomStream Random(FMath::Rand());
	const FLootRoll Roll = RollLoot(LootTable, Level, ExtraLuck, Random, KillAmmo);
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
