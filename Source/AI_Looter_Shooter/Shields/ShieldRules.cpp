#include "Shields/ShieldRules.h"
#include "Shields/ShieldDefinition.h"
#include "Progression/LevelRules.h"
#include "Shields/ShieldTuning.h"
#include "Math/RandomStream.h"
#include "Templates/TypeHash.h"

namespace
{
	/** Each thing a seed decides draws from its own stream, so adding one never moves the others. */
	constexpr uint32 BuildSalt = 0xB017D5u;
	constexpr uint32 TraitSalt = 0x7A417u;
	constexpr uint32 ElementSalt = 0xE1E3u;
	constexpr uint32 VarianceSalt = 0x5EED5u;

	/** Health this close to the most counts as full (FWoundsClose's reason: a float sum can land a hair under). */
	constexpr float FullTolerance = 0.01f;

	/** Within this share of each other, two wards are about the same. */
	constexpr float SameShare = 0.03f;

	FRandomStream StreamFor(int32 Seed, uint32 Salt)
	{
		return FRandomStream(static_cast<int32>(HashCombine(GetTypeHash(Seed), Salt)));
	}

	/** 1 plus a seeded share up to Variance either way. */
	float Varied(FRandomStream& Random, float Variance)
	{
		return 1.f + Random.FRandRange(-Variance, Variance);
	}
}

ShieldRules::FBuildLean ShieldRules::LeanOf(EShieldBuild Build)
{
	FBuildLean Lean;
	switch (Build)
	{
	case EShieldBuild::Bulwark:
		// A heavier box: a fifth more ward, slower to wake and to fill.
		Lean.Capacity = 1.2f;
		Lean.Delay = 1.1f;
		Lean.Rate = 0.8f;
		break;
	case EShieldBuild::Quickset:
		// A light one: wakes in seven tenths of the time and fills quicker, holding less.
		Lean.Capacity = 0.85f;
		Lean.Delay = 0.7f;
		Lean.Rate = 1.15f;
		break;
	default:
		break;
	}
	return Lean;
}

EShieldBuild ShieldRules::BuildFor(int32 Seed)
{
	FRandomStream Random = StreamFor(Seed, BuildSalt);
	return static_cast<EShieldBuild>(Random.RandRange(0, 2));
}

EShieldTrait ShieldRules::TraitFor(const UShieldDefinition& Definition, EWeaponRarity Rarity, int32 Seed)
{
	if (Rarity < EWeaponRarity::Rare)
	{
		return EShieldTrait::None;
	}
	FRandomStream Random = StreamFor(Seed, TraitSalt);
	if (Random.FRand() >= Definition.GetRarityInfo(Rarity).TraitChance)
	{
		return EShieldTrait::None;
	}
	return AllShieldTraits[Random.RandRange(0, UE_ARRAY_COUNT(AllShieldTraits) - 1)];
}

ELooterElement ShieldRules::NovaElementFor(int32 Seed)
{
	FRandomStream Random = StreamFor(Seed, ElementSalt);
	// Every element alike, Thunder and Rot too since they came in (2026-10-29).
	static constexpr ELooterElement Elements[] = { ELooterElement::Ember, ELooterElement::SoulLight, ELooterElement::GraveSalt,
		ELooterElement::Thunder, ELooterElement::Rot };
	return Elements[Random.RandRange(0, UE_ARRAY_COUNT(Elements) - 1)];
}

FShieldStats ShieldRules::ComputeStats(const UShieldDefinition& Definition, EWeaponRarity Rarity, int32 Level, int32 Seed, EShieldBuild Build)
{
	const FShieldRarityInfo& Tier = Definition.GetRarityInfo(Rarity);
	const FBuildLean Lean = LeanOf(Build);
	FRandomStream Random = StreamFor(Seed, VarianceSalt);
	const float Variance = FMath::Clamp(Definition.Variance, 0.f, 0.5f);
	// Compounding, as guns and creatures grow (the user, 2026-10-29 night: +9% a level).
	const float LevelScale = FLevelRules::Compounded(Definition.CapacityPerLevel, Level);

	FShieldStats Stats;
	Stats.Capacity = FMath::Max(Definition.BaseCapacity * Tier.CapacityMultiplier * LevelScale * Lean.Capacity * Varied(Random, Variance), 1.f);
	Stats.RechargeDelay = FMath::Max(Definition.BaseRechargeDelay * Tier.DelayMultiplier * Lean.Delay * Varied(Random, Variance), 0.f);
	Stats.RechargeRate = FMath::Max(Stats.Capacity * Definition.BaseRechargeShare * Tier.RateMultiplier * Lean.Rate * Varied(Random, Variance), 0.01f);
	return Stats;
}

FShieldStats ShieldRules::ComputeStats(const FShieldInstanceData& Ward)
{
	if (!Ward.Definition)
	{
		FShieldStats None;
		None.Capacity = 0.f;
		None.RechargeRate = 0.f;
		return None;
	}
	return ComputeStats(*Ward.Definition, Ward.Rarity, Ward.Level, Ward.Seed, Ward.Build);
}

FShieldInstanceData ShieldRules::Make(UShieldDefinition* Definition, EWeaponRarity Rarity, int32 Level, int32 Seed)
{
	if (!Definition)
	{
		return FShieldInstanceData();
	}
	const EShieldTrait Trait = TraitFor(*Definition, Rarity, Seed);
	return MakeWith(Definition, Rarity, Level, Seed, BuildFor(Seed), Trait,
		Trait == EShieldTrait::LastBreath ? NovaElementFor(Seed) : ELooterElement::None);
}

FShieldInstanceData ShieldRules::MakeWith(UShieldDefinition* Definition, EWeaponRarity Rarity, int32 Level, int32 Seed, EShieldBuild Build,
	EShieldTrait Trait, ELooterElement Element)
{
	FShieldInstanceData Ward;
	Ward.Definition = Definition;
	Ward.Rarity = Rarity;
	Ward.Level = FMath::Max(Level, 1);
	Ward.Seed = Seed;
	Ward.Build = Build;
	Ward.Trait = Trait;
	// Only Last Breath has an element; one asked for without an element gets its seed's.
	Ward.Element = Trait != EShieldTrait::LastBreath ? ELooterElement::None
		: (Element != ELooterElement::None ? Element : NovaElementFor(Seed));
	Ward.Stats = ComputeStats(Ward);
	return Ward;
}

FShieldInstanceData ShieldRules::Starter(UShieldDefinition* Definition, int32 Level)
{
	// Seed 0's numbers sit wherever the variance puts them; the starting ward is the plain box, so its build is set.
	return MakeWith(Definition, EWeaponRarity::Common, Level, 0, EShieldBuild::Standard, EShieldTrait::None);
}

EWeaponRarity ShieldRules::RollRarity(const UShieldDefinition& Definition, float Luck, FRandomStream& Random)
{
	constexpr int32 NumTiers = static_cast<int32>(EWeaponRarity::Legendary) + 1;
	float Weights[NumTiers];
	float Total = 0.f;
	for (int32 Tier = 0; Tier < NumTiers; ++Tier)
	{
		const float LuckScale = FMath::Pow(1.f + FMath::Max(Luck, 0.f), static_cast<float>(Tier));
		Weights[Tier] = FMath::Max(Definition.GetRarityInfo(static_cast<EWeaponRarity>(Tier)).Weight, 0.f) * LuckScale;
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

float ShieldRules::LiveCapacity(const FShieldStats& Stats, const FShieldTuning& Tuning)
{
	return FMath::Max(Stats.Capacity * Tuning.CapacityScale, 0.f);
}

float ShieldRules::LiveDelay(const FShieldStats& Stats, const FShieldTuning& Tuning)
{
	return FMath::Max(Stats.RechargeDelay * Tuning.DelayScale, 0.f);
}

float ShieldRules::LiveRate(const FShieldStats& Stats, const FShieldTuning& Tuning)
{
	return FMath::Max(Stats.RechargeRate * Tuning.RateScale, 0.f);
}

ShieldRules::FShieldSoak ShieldRules::Soak(float Charge, float Damage, float Spill)
{
	FShieldSoak Result;
	Damage = FMath::Max(Damage, 0.f);
	Charge = FMath::Max(Charge, 0.f);
	if (Charge <= 0.f)
	{
		Result.ToHealth = Damage;
		return Result;
	}
	if (Damage < Charge)
	{
		Result.ChargeAfter = Charge - Damage;
		Result.Absorbed = Damage;
		return Result;
	}
	// The hit that empties it: the ward takes what it held, and only a share of the rest gets through.
	Result.Absorbed = Charge;
	Result.ToHealth = (Damage - Charge) * FMath::Clamp(Spill, 0.f, 1.f);
	Result.bBroke = true;
	return Result;
}

void ShieldRules::FShieldCharge::Hurt()
{
	Quiet = 0.f;
	bFilling = false;
}

ShieldRules::ERefill ShieldRules::FShieldCharge::Step(float DeltaSeconds, float InCapacity, float InDelay, float InRate)
{
	DeltaSeconds = FMath::Max(DeltaSeconds, 0.f);
	InCapacity = FMath::Max(InCapacity, 0.f);
	if (Charge >= InCapacity - FullTolerance)
	{
		// Full (or the capacity came down under it: a smaller ward, a tuning): held at the most, the wait kept met.
		Charge = InCapacity;
		Quiet = FMath::Min(Quiet + DeltaSeconds, InDelay);
		bFilling = false;
		return ERefill::None;
	}
	float Remaining = DeltaSeconds;
	ERefill Event = ERefill::None;
	if (Quiet < InDelay)
	{
		const float Waited = FMath::Min(Remaining, InDelay - Quiet);
		Quiet += Waited;
		Remaining -= Waited;
		if (Quiet < InDelay)
		{
			return ERefill::None;
		}
	}
	if (!bFilling)
	{
		bFilling = true;
		Event = ERefill::Started;
	}
	Charge = FMath::Min(Charge + FMath::Max(InRate, 0.f) * Remaining, InCapacity);
	if (Charge >= InCapacity - FullTolerance)
	{
		Charge = InCapacity;
		bFilling = false;
		Event = ERefill::Filled;
	}
	return Event;
}

bool ShieldRules::FShieldCharge::IsFull(float InCapacity) const
{
	return Charge >= InCapacity - FullTolerance;
}

void ShieldRules::FShieldCharge::Fill(float InCapacity)
{
	Charge = FMath::Max(InCapacity, 0.f);
	bFilling = false;
}

float ShieldRules::TraitStrength(const UShieldDefinition* Definition, EWeaponRarity Rarity)
{
	return Definition ? FMath::Max(Definition->GetRarityInfo(Rarity).TraitStrength, 0.f) : 1.f;
}

float ShieldRules::NovaDamageAt(float Distance, float Strength, float LevelScale, const FShieldTuning& Tuning)
{
	if (Distance > Tuning.NovaRadius)
	{
		return 0.f;
	}
	const float Heart = Tuning.NovaDamage * FMath::Max(Strength, 0.f) * FMath::Max(LevelScale, 0.f);
	const float Span = FMath::Max(Tuning.NovaRadius - Tuning.NovaCore, 1.f);
	const float Along = FMath::Clamp((Distance - Tuning.NovaCore) / Span, 0.f, 1.f);
	return Heart * FMath::Lerp(1.f, Tuning.NovaEdge, Along);
}

float ShieldRules::BarbDamage(float InCapacity, float Strength, const FShieldTuning& Tuning)
{
	return FMath::Max(InCapacity, 0.f) * Tuning.BarbShare * FMath::Max(Strength, 0.f);
}

float ShieldRules::LodestoneOdds(float Strength, const FShieldTuning& Tuning)
{
	return FMath::Clamp(Tuning.LodestoneChance * FMath::Max(Strength, 0.f), 0.f, 0.9f);
}

int32 ShieldRules::LodestoneRounds(int32 MagazineSize, float Strength, const FShieldTuning& Tuning)
{
	if (MagazineSize <= 0)
	{
		return 0;
	}
	return FMath::Max(1, FMath::RoundToInt32(static_cast<float>(MagazineSize) * Tuning.LodestoneShare * FMath::Max(Strength, 0.f)));
}

float ShieldRules::HarvestRestore(float InCapacity, float Strength, const FShieldTuning& Tuning)
{
	return FMath::Max(InCapacity, 0.f) * FMath::Clamp(Tuning.HarvestShare * FMath::Max(Strength, 0.f), 0.f, 1.f);
}

bool ShieldRules::GraceAnswers(bool bReady, float WardCharge, float HealthShare, const FShieldTuning& Tuning)
{
	return bReady && WardCharge <= 0.f && HealthShare > 0.f && HealthShare < Tuning.GraceBelow;
}

float ShieldRules::GraceCharge(float InCapacity, float Strength, const FShieldTuning& Tuning)
{
	return FMath::Max(InCapacity, 0.f) * FMath::Clamp(Tuning.GraceRefill * FMath::Max(Strength, 0.f), 0.f, 1.f);
}

float ShieldRules::Worth(const FShieldStats& Stats)
{
	// What it soaks in a fight: its whole charge, and what it gets back in the first two seconds of a lull after its wait
	// (a quick ward earns some of a slower, bigger one's worth back between hits).
	const float TwoSeconds = 2.f;
	return Stats.Capacity + Stats.RechargeRate * TwoSeconds * (3.2f / FMath::Max(Stats.RechargeDelay, 0.5f));
}

int32 ShieldRules::Compare(const FShieldInstanceData& Ward, const FShieldInstanceData* Than)
{
	if (!Than || !Than->IsSet())
	{
		return 1;
	}
	const float Mine = Worth(Ward.Stats);
	const float Theirs = Worth(Than->Stats);
	if (FMath::Abs(Mine - Theirs) <= SameShare * FMath::Max(Theirs, 1.f))
	{
		return 0;
	}
	return Mine > Theirs ? 1 : -1;
}

uint32 ShieldRules::Identity(const FShieldInstanceData& Ward)
{
	uint32 Hash = GetTypeHash(Ward.Definition.Get());
	Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Ward.Rarity)));
	Hash = HashCombine(Hash, GetTypeHash(Ward.Level));
	Hash = HashCombine(Hash, GetTypeHash(Ward.Seed));
	Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Ward.Build)));
	Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Ward.Trait)));
	// Never 0: the screens read 0 as nothing there.
	return Hash == 0 ? 1u : Hash;
}
