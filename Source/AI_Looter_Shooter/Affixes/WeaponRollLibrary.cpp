#include "Affixes/WeaponRollLibrary.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "AI_Looter_Shooter.h"
#include "Engine/World.h"

EWeaponRarity UWeaponRollLibrary::RollRarity(const UWeaponDefinition* Definition, float Luck)
{
	FRandomStream Random(FMath::Rand());
	return RollRarityWith(Definition, Luck, Random);
}

EWeaponRarity UWeaponRollLibrary::RollRarityWith(const UWeaponDefinition* Definition, float Luck, FRandomStream& Random)
{
	if (!Definition)
	{
		return EWeaponRarity::Common;
	}

	constexpr int32 NumTiers = static_cast<int32>(EWeaponRarity::Legendary) + 1;
	float Weights[NumTiers];
	float Total = 0.f;
	for (int32 Tier = 0; Tier < NumTiers; ++Tier)
	{
		const float LuckScale = FMath::Pow(1.f + FMath::Max(Luck, 0.f), static_cast<float>(Tier));
		Weights[Tier] = Definition->GetRarityInfo(static_cast<EWeaponRarity>(Tier)).Weight * LuckScale;
		Total += Weights[Tier];
	}

	if (Total <= 0.f)
	{
		return EWeaponRarity::Common;
	}

	float Pick = Random.FRand() * Total;
	for (int32 Tier = 0; Tier < NumTiers; ++Tier)
	{
		Pick -= Weights[Tier];
		if (Pick <= 0.f)
		{
			return static_cast<EWeaponRarity>(Tier);
		}
	}
	return EWeaponRarity::Legendary;
}

FWeaponStats UWeaponRollLibrary::ComputeStats(const UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level, int32 Seed)
{
	if (!Definition)
	{
		return FWeaponStats();
	}

	FRandomStream Stream(Seed);
	const float Variance = Definition->StatVariance;
	auto Vary = [&Stream, Variance](float Value)
	{
		return Value * Stream.FRandRange(1.f - Variance, 1.f + Variance);
	};

	const FWeaponRarityInfo& RarityInfo = Definition->GetRarityInfo(Rarity);
	const FWeaponStats& Base = Definition->BaseStats;
	const float LevelScale = 1.f + Definition->DamagePerLevel * FMath::Max(Level - 1, 0);

	// Roll order is fixed so a given seed always produces the same weapon.
	FWeaponStats Stats;
	Stats.Damage = Vary(Base.Damage) * RarityInfo.DamageMultiplier * LevelScale;
	Stats.FireRate = Vary(Base.FireRate) * RarityInfo.FireRateMultiplier;
	Stats.MagazineSize = FMath::Max(1, FMath::RoundToInt(Vary(static_cast<float>(Base.MagazineSize)) * RarityInfo.MagazineMultiplier));
	Stats.ReloadTime = Vary(Base.ReloadTime) * RarityInfo.ReloadTimeMultiplier;
	Stats.Spread = Vary(Base.Spread) * RarityInfo.SpreadMultiplier;
	Stats.Range = Base.Range;
	Stats.PelletsPerShot = Base.PelletsPerShot;
	return Stats;
}

FWeaponInstanceData UWeaponRollLibrary::RollWeapon(UWeaponDefinition* Definition, int32 Level, float Luck)
{
	return RollWeaponWithRarity(Definition, RollRarity(Definition, Luck), Level);
}

FWeaponInstanceData UWeaponRollLibrary::RollWeaponWithRarity(UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level)
{
	FWeaponInstanceData Instance;
	Instance.Definition = Definition;
	Instance.Rarity = Rarity;
	Instance.Level = FMath::Max(Level, 1);
	Instance.Seed = FMath::Rand();
	Instance.Stats = ComputeStats(Definition, Instance.Rarity, Instance.Level, Instance.Seed);
	return Instance;
}

AWeaponBase* UWeaponRollLibrary::SpawnWeapon(UObject* WorldContextObject, const FWeaponInstanceData& Instance, const FTransform& Transform)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || !Instance.Definition)
	{
		UE_LOG(LogLooter, Warning, TEXT("SpawnWeapon: missing world or weapon definition"));
		return nullptr;
	}

	TSubclassOf<AWeaponBase> WeaponClass = Instance.Definition->WeaponClass;
	if (!WeaponClass)
	{
		WeaponClass = AWeaponBase::StaticClass();
	}

	AWeaponBase* Weapon = World->SpawnActorDeferred<AWeaponBase>(WeaponClass, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Weapon)
	{
		Weapon->InitializeFromInstance(Instance);
		Weapon->FinishSpawning(Transform);
	}
	return Weapon;
}

FLinearColor UWeaponRollLibrary::GetRarityColor(const UWeaponDefinition* Definition, EWeaponRarity Rarity)
{
	return Definition ? Definition->GetRarityInfo(Rarity).Color : FLinearColor::White;
}
