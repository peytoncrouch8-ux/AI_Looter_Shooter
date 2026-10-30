#include "Loot/LootLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/AmmoPickup.h"
#include "Loot/LootFanfareSubsystem.h"
#include "Loot/LootTable.h"
#include "Shields/ShieldDefinition.h"
#include "Shields/ShieldPickup.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Affixes/WeaponRollLibrary.h"
#include "AI_Looter_Shooter.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* DefaultLootTablePath = TEXT("/Game/Weapons/Data/DA_LootTable_Default.DA_LootTable_Default");

	// Legendaries rarer early (the user, 2026-10-27: "Rarer, bosses still drop"; they'd found them "super easy to get").
	// 1 in 300 a gun puts a Regular kill (a gun on 30% of kills) at 0.1% and a Strongbox (2 guns) at 0.67%, under 1%.
	TAutoConsoleVariable<float> CVarEarlyLegendaryShare(TEXT("Looter.Loot.EarlyLegendary.Share"), 1.f / 300.f,
		TEXT("In the early areas, the most a dropped gun's chance of being legendary may be (0-1; 1 = no cap). Soulfed, bosses and ")
		TEXT("bounties keep their odds."), ECVF_Default);
	// Ransom's Rest's creatures are level 1-6 before their ranks' (+1, +2) and its chests keep in its band; the Lily starts at 9.
	TAutoConsoleVariable<int32> CVarEarlyLegendaryMaxLevel(TEXT("Looter.Loot.EarlyLegendary.MaxLevel"), 8,
		TEXT("Guns of this level and under drop under the early legendary cap (0 = never capped)."), ECVF_Default);

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

EAmmoType ULootLibrary::PickAmmoType(const ULootTable* LootTable, FRandomStream& Random, TOptional<EAmmoType> KillAmmo, bool bNoRockets)
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
	if (bNoRockets)
	{
		Weights[static_cast<int32>(EAmmoType::Rocket)] = 0.f;
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

FLootRoll ULootLibrary::RollLoot(const ULootTable* LootTable, int32 Level, float ExtraLuck, FRandomStream& Random, TOptional<EAmmoType> KillAmmo,
	bool bWeapons, ELooterElement Lean)
{
	FLootRoll Roll;
	if (!LootTable)
	{
		return Roll;
	}
	// Ammo first, then the guns, from the same stream: a seeded roll repeats. Each gun's stats come from its own seed, and
	// its level is the killer's (a creature passes its own to its loot).
	Roll.Ammo = RollAmmo(LootTable, Random, KillAmmo);
	if (!bWeapons)
	{
		return Roll;
	}
	// A table that drops wards turns some of its gun picks into a ward of the same rarity (the weapon-drop share split), drawn
	// after the picks, so a table without wards rolls exactly as before. The level caps a legendary in the early areas.
	const float WardShare = LootTable->bDropsShields ? FShieldTuning::Live().DropShare : 0.f;
	for (const FLootWeaponPick& Pick : RollWeaponPicks(LootTable, ExtraLuck, Random, Level))
	{
		if (WardShare > 0.f && ShieldRules::IsWardDrop(Random.FRand(), WardShare))
		{
			// Its own fresh seed, as a gun's is.
			Roll.Shields.Add(ShieldRules::Make(UShieldDefinition::GetWardBox(), Pick.Rarity, Level, FMath::Rand()));
			continue;
		}
		Roll.Weapons.Add(RollDroppedWeapon(Pick.Definition, Pick.Rarity, Level, Lean));
	}
	return Roll;
}

FWeaponInstanceData ULootLibrary::RollDroppedWeapon(UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level, ELooterElement Lean)
{
	FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponLeaning(Definition, Rarity, Level, Lean);
	// Only guns that drop can come cursed (the curse is drawn from the gun's own seed, so the loot stream doesn't move);
	// mission rewards, the tutorial's rack and starting guns are made without this.
	Gun.Curse = WeaponCurses::Roll(Gun);
	if (!Gun.Curse.IsNone())
	{
		Gun.Stats = UWeaponRollLibrary::ComputeInstanceStats(Gun);
	}
	return Gun;
}

void ULootLibrary::KeepRocketsForLaunchers(FLootRoll& Roll, const ULootTable* LootTable, FRandomStream& Random, bool bLauncher)
{
	if (bLauncher)
	{
		return;
	}
	for (const FWeaponInstanceData& Gun : Roll.Weapons)
	{
		// A launcher in the same drop brings its rockets with it.
		if (Gun.Definition && Gun.Definition->Kind == EWeaponKind::Launcher)
		{
			return;
		}
	}
	for (FAmmoDrop& Drop : Roll.Ammo)
	{
		if (Drop.Type == EAmmoType::Rocket)
		{
			Drop.Type = PickAmmoType(LootTable, Random, {}, true);
			Drop.Amount = LooterLoot::AmmoAmountFor(Drop.Type, RollAmmoAmount(LootTable, Random));
		}
	}
}

bool ULootLibrary::AnyPlayerCarriesLauncher(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return true;
	}
	bool bAnyArmed = false;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Controller = It->Get();
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		const UWeaponManagerComponent* Weapons = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
		if (!Weapons)
		{
			continue;
		}
		bAnyArmed = true;
		for (const AWeaponBase* Gun : Weapons->GetWeapons())
		{
			if (Gun && Gun->GetInstance().Definition && Gun->GetInstance().Definition->Kind == EWeaponKind::Launcher)
			{
				return true;
			}
		}
		for (const FWeaponInstanceData& Kept : Weapons->GetBackpack())
		{
			if (Kept.Definition && Kept.Definition->Kind == EWeaponKind::Launcher)
			{
				return true;
			}
		}
	}
	return !bAnyArmed;
}

TArray<FAmmoDrop> ULootLibrary::RollAmmo(const ULootTable* LootTable, FRandomStream& Random, TOptional<EAmmoType> KillAmmo)
{
	TArray<FAmmoDrop> Ammo;
	if (!LootTable)
	{
		return Ammo;
	}
	// Most kills leave a pickup or two (ranked creatures more), each of a random class (more often the kill weapon's) and a
	// random amount from the table's range, the same range for every class (rockets read it as one to three rockets).
	if (Random.FRand() < LootTable->AmmoDropChance)
	{
		const int32 MinDrops = FMath::Max(LootTable->MinAmmoDrops, 0);
		const int32 Drops = Random.RandRange(MinDrops, FMath::Max(LootTable->MaxAmmoDrops, MinDrops));
		for (int32 Index = 0; Index < Drops; ++Index)
		{
			FAmmoDrop& Drop = Ammo.AddDefaulted_GetRef();
			Drop.Type = PickAmmoType(LootTable, Random, KillAmmo);
			Drop.Amount = LooterLoot::AmmoAmountFor(Drop.Type, RollAmmoAmount(LootTable, Random));
		}
	}
	return Ammo;
}

TArray<FLootWeaponPick> ULootLibrary::RollWeaponPicks(const ULootTable* LootTable, float ExtraLuck, FRandomStream& Random, int32 Level)
{
	TArray<FLootWeaponPick> Picks;
	if (!LootTable)
	{
		return Picks;
	}
	// Only some kills. Rarity follows each weapon's own odds (Common most, Legendary least), shifted by luck, a legendary
	// capped in the early areas (the same draws either way: only the odds move).
	if (Random.FRand() < LootTable->WeaponDropChance)
	{
		const float LegendaryCap = LegendaryCapFor(LootTable, Level);
		const int32 MinWeapons = FMath::Max(LootTable->MinWeaponDrops, 0);
		const int32 Weapons = Random.RandRange(MinWeapons, FMath::Max(LootTable->MaxWeaponDrops, MinWeapons));
		for (int32 Index = 0; Index < Weapons; ++Index)
		{
			if (UWeaponDefinition* Definition = PickWeaponWith(LootTable, Random))
			{
				FLootWeaponPick& Pick = Picks.AddDefaulted_GetRef();
				Pick.Definition = Definition;
				Pick.Rarity = UWeaponRollLibrary::RollRarityWith(Definition, LootTable->Luck + ExtraLuck, Random, LegendaryCap,
					LootTable->MinWeaponRarity);
			}
		}
	}
	return Picks;
}

float ULootLibrary::LegendaryCapFor(const ULootTable* LootTable, int32 Level)
{
	const int32 MaxLevel = CVarEarlyLegendaryMaxLevel.GetValueOnGameThread();
	if (!LootTable || LootTable->bKeepsLegendaryOdds || Level <= NoLevel || Level > MaxLevel)
	{
		return 1.f;
	}
	return FMath::Clamp(CVarEarlyLegendaryShare.GetValueOnGameThread(), 0.f, 1.f);
}

ULootTable* ULootLibrary::KeepingLegendaryOdds(ULootTable* Table, UObject* Outer)
{
	if (!Table || Table->bKeepsLegendaryOdds)
	{
		return Table;
	}
	ULootTable* Copy = DuplicateObject<ULootTable>(Table, Outer ? Outer : GetTransientPackage());
	Copy->SetFlags(RF_Transient);
	Copy->bKeepsLegendaryOdds = true;
	return Copy;
}

TArray<AActor*> ULootLibrary::SpawnLoot(UObject* WorldContextObject, const ULootTable* LootTable, FVector Location, int32 Level, float ExtraLuck)
{
	return SpawnKillLoot(WorldContextObject, LootTable, Location, Level, ExtraLuck, {});
}

TArray<AActor*> ULootLibrary::SpawnKillLoot(UObject* WorldContextObject, const ULootTable* LootTable, FVector Location, int32 Level,
	float ExtraLuck, TOptional<EAmmoType> KillAmmo, bool bWeapons, ELooterElement Lean)
{
	TArray<AActor*> Spawned;
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || !LootTable)
	{
		return Spawned;
	}

	FRandomStream Random(FMath::Rand());
	FLootRoll Roll = RollLoot(LootTable, Level, ExtraLuck, Random, KillAmmo, bWeapons, Lean);
	KeepRocketsForLaunchers(Roll, LootTable, Random, AnyPlayerCarriesLauncher(World));
	const FVector SpawnLocation = Location + FVector(0.f, 0.f, 60.f);

	if (UE_LOG_ACTIVE(LogLooter, Verbose))
	{
		TArray<FString> Items;
		for (const FWeaponInstanceData& Instance : Roll.Weapons)
		{
			Items.Add(FString::Printf(TEXT("%s %s"), *UEnum::GetDisplayValueAsText(Instance.Rarity).ToString(), *GetNameSafe(Instance.Definition)));
		}
		for (const FShieldInstanceData& Ward : Roll.Shields)
		{
			Items.Add(FString::Printf(TEXT("%s ward"), *UEnum::GetDisplayValueAsText(Ward.Rarity).ToString()));
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
			// Its rarity is heard and seen as it lands.
			ULootFanfareSubsystem::ExpectLanding(Weapon);
			Spawned.Add(Weapon);
		}
	}

	// Wards fly out as the guns do, and sting as they land by their rarity (AShieldPickup).
	for (const FShieldInstanceData& Ward : Roll.Shields)
	{
		if (AShieldPickup* Pickup = AShieldPickup::SpawnWard(World, Ward, SpawnLocation))
		{
			Pickup->Toss(RandomTossVelocity());
			Spawned.Add(Pickup);
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
