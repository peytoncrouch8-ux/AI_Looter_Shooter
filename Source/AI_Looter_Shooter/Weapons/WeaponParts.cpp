#include "Weapons/WeaponParts.h"
#include "Weapons/NamedWeaponDefinition.h"
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

	/** The part's requirement holds against the parts picked so far (slots before it). */
	bool Fits(const FWeaponPartOption& Option, const UWeaponDefinition& Definition, TConstArrayView<const FWeaponPartOption*> Picked)
	{
		const FWeaponPartRequirement& Need = Option.Requires;
		if (Need.Slot.IsNone())
		{
			return true;
		}
		for (int32 Index = 0; Index < Picked.Num(); ++Index)
		{
			if (Definition.Parts[Index].Name == Need.Slot)
			{
				return Picked[Index] && Picked[Index]->Length >= Need.MinLength;
			}
		}
		return false;
	}

	float Capped(float Total, const WeaponParts::FCap& Cap)
	{
		return FMath::Clamp(Total, Cap.Min, Cap.Max);
	}
}

FWeaponLook WeaponParts::Pick(const UWeaponDefinition& Definition, int32 Seed, EWeaponRarity Rarity, TConstArrayView<FName> SavedParts)
{
	FWeaponLook Look;
	// A stream of its own from the same seed: the stats' variance draws from FRandomStream(Seed), and sharing its
	// numbers would tie each part's odds to a stat's roll.
	FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Seed), 0x9E3779B9u)));
	const bool bSaved = SavedParts.Num() == Definition.Parts.Num();
	for (int32 Index = 0; Index < Definition.Parts.Num(); ++Index)
	{
		const FWeaponPartSlot& Slot = Definition.Parts[Index];
		const float Roll = Random.FRand();
		if (bSaved)
		{
			// The gun keeps the parts it was saved with (an empty slot stays empty).
			if (SavedParts[Index].IsNone())
			{
				Look.Parts.Add(nullptr);
				continue;
			}
			if (const FWeaponPartOption* Saved = Slot.Options.FindByPredicate([&SavedParts, Index](const FWeaponPartOption& Option)
				{
					return Option.Mesh && Option.Key == SavedParts[Index];
				}))
			{
				Look.Parts.Add(Saved);
				continue;
			}
		}
		Look.Parts.Add(PickByWeight(Slot.Options, Roll, [&](const FWeaponPartOption& Option)
		{
			return Option.Mesh && Option.MinRarity <= Rarity && Fits(Option, Definition, Look.Parts);
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

FWeaponLook WeaponParts::Pick(const FWeaponInstanceData& Instance)
{
	return Instance.Definition ? Pick(*Instance.Definition, Instance.Seed, Instance.Rarity, Instance.Parts) : FWeaponLook();
}

TArray<FName> WeaponParts::PartKeys(const FWeaponLook& Look)
{
	TArray<FName> Keys;
	for (const FWeaponPartOption* Part : Look.Parts)
	{
		Keys.Add(Part ? Part->Key : NAME_None);
	}
	return Keys;
}

FWeaponPartTotals WeaponParts::CombinedStats(const FWeaponLook& Look, int32 Seed, TOptional<float> FixedQuality)
{
	FWeaponPartTotals Totals;
	for (int32 Index = 0; Index < Look.Parts.Num(); ++Index)
	{
		const FWeaponPartOption* Part = Look.Parts[Index];
		if (!Part)
		{
			continue;
		}
		// Each part rolls from its own stream (the gun's seed and the part's key), in a fixed stat order.
		const uint32 PartHash = Part->Key.IsNone() ? static_cast<uint32>(Index) : GetTypeHash(Part->Key);
		FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Seed), HashCombine(PartHash, 0x51ED270Bu))));
		// Where in its range each percentage lands: the next roll, or a named gun's fixed point, turned around for the
		// stats where less is better. A fixed point draws nothing, and a rolled gun draws as it always has.
		auto Roll = [&Random, &FixedQuality](bool bLessIsBetter)
		{
			if (FixedQuality.IsSet())
			{
				const float Quality = FMath::Clamp(FixedQuality.GetValue(), 0.f, 1.f);
				return bLessIsBetter ? 1.f - Quality : Quality;
			}
			return Random.FRand();
		};
		const FWeaponPartStats& Stats = Part->Stats;
		Totals.Damage += Stats.Damage.At(Roll(false));
		Totals.Accuracy += Stats.Accuracy.At(Roll(false));
		Totals.Range += Stats.Range.At(Roll(false));
		Totals.FireRate += Stats.FireRate.At(Roll(false));
		Totals.Reload += Stats.Reload.At(Roll(true));
		Totals.Recoil += Stats.Recoil.At(Roll(true));
		Totals.Handling += Stats.Handling.At(Roll(false));
		// Capacity and zoom are the part's own (the last part that sets one wins).
		Totals.Magazine = Stats.Magazine > 0 ? Stats.Magazine : Totals.Magazine;
		Totals.Zoom = Stats.Zoom > 0.f ? Stats.Zoom : Totals.Zoom;
	}
	Totals.Damage = Capped(Totals.Damage, DamageCap);
	Totals.Accuracy = Capped(Totals.Accuracy, AccuracyCap);
	Totals.Range = Capped(Totals.Range, RangeCap);
	Totals.FireRate = Capped(Totals.FireRate, FireRateCap);
	Totals.Reload = Capped(Totals.Reload, ReloadCap);
	Totals.Recoil = Capped(Totals.Recoil, RecoilCap);
	Totals.Handling = Capped(Totals.Handling, HandlingCap);
	return Totals;
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

float WeaponParts::Wear(const FWeaponInstanceData& Instance)
{
	// A named gun's own wear, the same on every copy (Heirloom's: a keeper's gun, worn but cared for).
	if (Instance.Named && Instance.Named->Wear >= 0.f)
	{
		return FMath::Clamp(Instance.Named->Wear, 0.f, 1.f);
	}
	// From least to most worn by rarity; a stream of its own, so it never shifts the parts' or the stats' rolls.
	static const FVector2f Ranges[] = { { 0.45f, 1.f }, { 0.3f, 0.85f }, { 0.15f, 0.65f }, { 0.05f, 0.45f }, { 0.f, 0.25f } };
	const FVector2f Range = Ranges[FMath::Clamp(static_cast<int32>(Instance.Rarity), 0, static_cast<int32>(UE_ARRAY_COUNT(Ranges)) - 1)];
	FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Instance.Seed), 0x57EA7u)));
	return FMath::Lerp(Range.X, Range.Y, Random.FRand());
}

float WeaponParts::RarityGlow(EWeaponRarity Rarity)
{
	return Rarity == EWeaponRarity::Common ? 0.6f : 2.5f;
}
