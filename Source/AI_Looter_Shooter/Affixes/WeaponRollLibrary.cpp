#include "Affixes/WeaponRollLibrary.h"
#include "Affixes/WeaponMakers.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponCurses.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponNotches.h"
#include "Weapons/WeaponParts.h"
#include "AI_Looter_Shooter.h"
#include "Engine/World.h"

EWeaponRarity UWeaponRollLibrary::RollRarity(const UWeaponDefinition* Definition, float Luck)
{
	FRandomStream Random(FMath::Rand());
	return RollRarityWith(Definition, Luck, Random);
}

double UWeaponRollLibrary::CapLegendaryWeight(double Legendary, double Others, float LegendaryCap)
{
	if (LegendaryCap >= 1.f || Legendary <= 0.0)
	{
		return Legendary;
	}
	const double Cap = FMath::Max(static_cast<double>(LegendaryCap), 0.0);
	const double Total = Legendary + Others;
	if (Total <= 0.0 || Legendary / Total <= Cap)
	{
		return Legendary;
	}
	// Share = L / (L + Others) = Cap  =>  L = Cap x Others / (1 - Cap).
	return Cap * FMath::Max(Others, 0.0) / (1.0 - Cap);
}

EWeaponRarity UWeaponRollLibrary::RollRarityWith(const UWeaponDefinition* Definition, float Luck, FRandomStream& Random, float LegendaryCap,
	EWeaponRarity MinRarity)
{
	if (!Definition)
	{
		return MinRarity;
	}

	constexpr int32 NumTiers = static_cast<int32>(EWeaponRarity::Legendary) + 1;
	constexpr int32 Top = NumTiers - 1;
	// A floor (a better chest's: ULootTable::MinWeaponRarity) takes the tiers under it out; the rest keep their odds against
	// each other. Without one (Common) every tier draws exactly as it always has.
	const int32 Floor = FMath::Clamp(static_cast<int32>(MinRarity), 0, Top);
	float Weights[NumTiers];
	float Total = 0.f;
	for (int32 Tier = 0; Tier < NumTiers; ++Tier)
	{
		const float LuckScale = FMath::Pow(1.f + FMath::Max(Luck, 0.f), static_cast<float>(Tier));
		Weights[Tier] = Tier < Floor ? 0.f : Definition->GetRarityInfo(static_cast<EWeaponRarity>(Tier)).Weight * LuckScale;
		Total += Weights[Tier];
	}
	// An early area's cap on a legendary (only then: an uncapped roll draws exactly as it always has, seeded tests too).
	if (LegendaryCap < 1.f)
	{
		const float Others = Total - Weights[Top];
		Weights[Top] = static_cast<float>(CapLegendaryWeight(Weights[Top], Others, LegendaryCap));
		Total = Others + Weights[Top];
	}

	if (Total <= 0.f)
	{
		return static_cast<EWeaponRarity>(Floor);
	}

	float Pick = Random.FRand() * Total;
	int32 LastWeighted = Floor;
	// From the floor up: a tier under it never comes, even on a pick of exactly 0.
	for (int32 Tier = Floor; Tier < NumTiers; ++Tier)
	{
		Pick -= Weights[Tier];
		if (Pick <= 0.f)
		{
			return static_cast<EWeaponRarity>(Tier);
		}
		LastWeighted = Weights[Tier] > 0.f ? Tier : LastWeighted;
	}
	// Float rounding left a sliver: the rarest tier that can come at all (never a legendary the cap took to nothing).
	return static_cast<EWeaponRarity>(LastWeighted);
}

FWeaponStats UWeaponRollLibrary::ComputeStats(const UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level, int32 Seed)
{
	return ComputeStatsWithParts(Definition, Rarity, Level, Seed, {});
}

FWeaponStats UWeaponRollLibrary::ComputeStatsWithParts(const UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level, int32 Seed,
	TConstArrayView<FName> Parts, TOptional<float> FixedQuality)
{
	if (!Definition)
	{
		return FWeaponStats();
	}

	FRandomStream Stream(Seed);
	const float Variance = Definition->StatVariance;
	// Each stat's variance: drawn from the seed, or a named gun's fixed point of it (turned around for the stats where
	// less is better). A fixed point draws nothing, and a rolled gun draws and rounds exactly as it always has (the
	// stream's range is in doubles).
	auto Vary = [&Stream, Variance, &FixedQuality](float Value, bool bLessIsBetter) -> double
	{
		if (FixedQuality.IsSet())
		{
			const float Quality = FMath::Clamp(FixedQuality.GetValue(), 0.f, 1.f);
			return Value * FMath::Lerp(1.f - Variance, 1.f + Variance, bLessIsBetter ? 1.f - Quality : Quality);
		}
		return Value * Stream.FRandRange(1.f - Variance, 1.f + Variance);
	};

	const FWeaponRarityInfo& RarityInfo = Definition->GetRarityInfo(Rarity);
	const FWeaponStats& Base = Definition->BaseStats;
	const float LevelScale = 1.f + Definition->DamagePerLevel * FMath::Max(Level - 1, 0);
	// The gun's parts shift its stats: their percentages add up (capped), each rolled within its range from the seed (or
	// at the named gun's fixed point of it).
	const FWeaponPartTotals Part = WeaponParts::CombinedStats(WeaponParts::Pick(*Definition, Seed, Rarity, Parts), Seed, FixedQuality);
	auto Scale = [](float Percent) { return FMath::Max(1.f + Percent * 0.01f, 0.05f); };

	// Roll order is fixed so a given seed always produces the same weapon (the magazine's variance is drawn even when a
	// part sets the capacity, so the draws after it don't move).
	FWeaponStats Stats;
	Stats.Damage = Vary(Base.Damage, false) * RarityInfo.DamageMultiplier * LevelScale * Scale(Part.Damage);
	Stats.FireRate = Vary(Base.FireRate, false) * RarityInfo.FireRateMultiplier * Scale(Part.FireRate);
	const float BaseMagazine = Vary(static_cast<float>(Base.MagazineSize), false);
	Stats.MagazineSize = FMath::Max(1, FMath::RoundToInt((Part.Magazine > 0 ? Part.Magazine : BaseMagazine) * RarityInfo.MagazineMultiplier));
	Stats.ReloadTime = Vary(Base.ReloadTime, true) * RarityInfo.ReloadTimeMultiplier * Scale(Part.Reload);
	// +40% accuracy shoots 1/1.4 as wide.
	Stats.Spread = Vary(Base.Spread, true) * RarityInfo.SpreadMultiplier / Scale(Part.Accuracy);
	Stats.Range = Base.Range * Scale(Part.Range);
	Stats.PelletsPerShot = Base.PelletsPerShot;
	Stats.Recoil = Base.Recoil * Scale(Part.Recoil);
	Stats.Handling = Base.Handling * Scale(Part.Handling);
	Stats.Zoom = FMath::Max(Part.Zoom > 0.f ? Part.Zoom : Base.Zoom, 1.f);
	return Stats;
}

FWeaponStats UWeaponRollLibrary::ComputeInstanceStats(const FWeaponInstanceData& Instance)
{
	// A named gun is one gun: its stats sit at its fixed quality on every copy, and follow that quality if it's tuned.
	const UNamedWeaponDefinition* Named = Instance.Named;
	FWeaponStats Stats = ComputeStatsWithParts(Instance.Definition, Instance.Rarity, Instance.Level, Instance.Seed, Instance.Parts,
		Named ? TOptional<float>(Named->StatQuality) : TOptional<float>());
	if (!Instance.Definition)
	{
		return Stats;
	}
	// Its maker's handling trade (none for a named gun or one made without a maker), outside the parts' caps.
	WeaponMakers::ApplyToStats(Instance, Stats);
	// Then what the gun has been through: its notches' damage, and its curse's perk and drawback. A gun with neither (every
	// gun found before notches and curses) keeps exactly the numbers it had.
	const float NotchDamage = WeaponNotches::DamageMultiplier(Instance.Kills);
	if (NotchDamage != 1.f)
	{
		Stats.Damage *= NotchDamage;
	}
	WeaponCurses::ApplyToStats(Instance, Stats);
	return Stats;
}

FWeaponInstanceData UWeaponRollLibrary::RollWeapon(UWeaponDefinition* Definition, int32 Level, float Luck)
{
	return RollWeaponWithRarity(Definition, RollRarity(Definition, Luck), Level);
}

FWeaponInstanceData UWeaponRollLibrary::RollWeaponWithRarity(UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level)
{
	return RollWeaponLeaning(Definition, Rarity, Level, ELooterElement::None);
}

FWeaponInstanceData UWeaponRollLibrary::RollWeaponLeaning(UWeaponDefinition* Definition, EWeaponRarity Rarity, int32 Level, ELooterElement Lean)
{
	FWeaponInstanceData Instance;
	Instance.Definition = Definition;
	Instance.Rarity = Rarity;
	Instance.Level = FMath::Max(Level, 1);
	Instance.Seed = FMath::Rand();
	// Its maker first, from the seed (its own stream): the maker leans the parts' picks, so it must be known before them.
	Instance.Maker = WeaponMakers::Roll(Definition, Instance.Seed, Instance.Rarity);
	// The parts are picked once, here, and kept with the gun: parts added later never change it.
	if (Definition)
	{
		Instance.Parts = WeaponParts::PartKeys(WeaponParts::Pick(*Definition, Instance.Seed, Instance.Rarity, {}, WeaponMakers::Of(Instance), Lean));
	}
	// As a weapon rebuilds them (a new gun has no notches or curse yet): its parts', then its maker's.
	Instance.Stats = ComputeInstanceStats(Instance);
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
