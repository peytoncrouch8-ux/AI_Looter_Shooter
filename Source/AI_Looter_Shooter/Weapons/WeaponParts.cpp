#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponDefinition.h"

namespace
{
	/** The option Roll (0-1) lands on among the allowed ones, by weight; null when none is allowed. */
	template <typename TOption, typename TAllowed>
	const TOption* PickByWeight(const TArray<TOption>& Options, float Roll, TAllowed IsAllowed)
	{
		float Total = 0.f;
		for (const TOption& Option : Options)
		{
			Total += IsAllowed(Option) ? Option.Weight : 0.f;
		}
		if (Total <= 0.f)
		{
			return nullptr;
		}
		float Remaining = Roll * Total;
		const TOption* Last = nullptr;
		for (const TOption& Option : Options)
		{
			if (!IsAllowed(Option) || Option.Weight <= 0.f)
			{
				continue;
			}
			Last = &Option;
			Remaining -= Option.Weight;
			if (Remaining < 0.f)
			{
				return &Option;
			}
		}
		return Last;
	}
}

FWeaponLook WeaponParts::Pick(const UWeaponDefinition& Definition, int32 Seed, EWeaponRarity Rarity)
{
	FWeaponLook Look;
	// A stream of its own from the same seed: the stats' variance draws from FRandomStream(Seed), and sharing its
	// numbers would tie each part's odds to a stat's roll.
	FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Seed), 0x9E3779B9u)));
	for (const FWeaponPartSlot& Slot : Definition.Parts)
	{
		const float Roll = Random.FRand();
		Look.Parts.Add(PickByWeight(Slot.Options, Roll, [Rarity](const FWeaponPartOption& Option)
		{
			return Option.Mesh && Option.MinRarity <= Rarity;
		}));
	}
	for (const FWeaponPaint& Paint : Definition.Paints)
	{
		const float Roll = Random.FRand();
		if (const FWeaponColorOption* Color = PickByWeight(Paint.Colors, Roll, [](const FWeaponColorOption&) { return true; }))
		{
			Look.Colors.Add(Paint.Slot, Color->Color);
		}
	}
	return Look;
}

FWeaponPartStats WeaponParts::CombinedStats(const FWeaponLook& Look)
{
	FWeaponPartStats Combined;
	for (const FWeaponPartOption* Part : Look.Parts)
	{
		if (Part)
		{
			Combined.Damage *= Part->Stats.Damage;
			Combined.FireRate *= Part->Stats.FireRate;
			Combined.MagazineSize *= Part->Stats.MagazineSize;
			Combined.ReloadTime *= Part->Stats.ReloadTime;
			Combined.Spread *= Part->Stats.Spread;
		}
	}
	return Combined;
}

FText WeaponParts::NamePrefix(const FWeaponLook& Look)
{
	const FWeaponPartOption* Namer = nullptr;
	for (const FWeaponPartOption* Part : Look.Parts)
	{
		// The earlier slot wins a tie.
		if (Part && !Part->NamePrefix.IsEmpty() && (!Namer || Part->NamePriority > Namer->NamePriority))
		{
			Namer = Part;
		}
	}
	return Namer ? Namer->NamePrefix : FText::GetEmpty();
}

float WeaponParts::RarityGlow(EWeaponRarity Rarity)
{
	return Rarity == EWeaponRarity::Common ? 0.6f : 2.5f;
}
