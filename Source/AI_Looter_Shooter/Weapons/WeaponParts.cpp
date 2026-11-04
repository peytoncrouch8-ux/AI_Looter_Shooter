#include "Weapons/WeaponParts.h"
#include "Affixes/WeaponMakers.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarTierSlot(TEXT("Looter.Loot.Parts.TierSlot"), 1,
		TEXT("A rolled Rare or better gun has one slot (by its seed) that always rolls a part of its own tier or better (1), or ")
		TEXT("every slot rolls as before 2026-11-03 (0). Saved guns keep their parts either way; a new roll reads it."),
		ECVF_Default);

	/** The tier slot's own stream (TierSlot): the parts' and the paints' draws never move for it. */
	constexpr uint32 TierSlotSalt = 0x71E25107u;

	/** The slot offers a part of Tier or better that a gun of Rarity can have. */
	bool OffersTier(const FWeaponPartSlot& Slot, EWeaponRarity Tier, EWeaponRarity Rarity)
	{
		return Slot.Options.ContainsByPredicate([Tier, Rarity](const FWeaponPartOption& Option)
		{
			return Option.Mesh && Option.MinRarity >= Tier && Option.MinRarity <= Rarity;
		});
	}

	/**
	 * The option Roll (0-1) lands on among the allowed ones, by WeightOf (a maker's leanings can weigh a part up); null
	 * when none is allowed.
	 */
	template <typename TOption, typename TAllowed, typename TWeight>
	const TOption* PickByWeight(const TArray<TOption>& Options, float Roll, TAllowed IsAllowed, TWeight WeightOf)
	{
		float Total = 0.f;
		for (const TOption& Option : Options)
		{
			Total += IsAllowed(Option) ? WeightOf(Option) : 0.f;
		}
		if (Total <= 0.f)
		{
			return nullptr;
		}
		float Remaining = Roll * Total;
		const TOption* Last = nullptr;
		for (const TOption& Option : Options)
		{
			const float Weight = WeightOf(Option);
			if (!IsAllowed(Option) || Weight <= 0.f)
			{
				continue;
			}
			Last = &Option;
			Remaining -= Weight;
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

FWeaponLook WeaponParts::Pick(const UWeaponDefinition& Definition, int32 Seed, EWeaponRarity Rarity, TConstArrayView<FName> SavedParts,
	const FWeaponMaker* Maker, ELooterElement Lean)
{
	FWeaponLook Look;
	// A stream of its own from the same seed: the stats' variance draws from FRandomStream(Seed), and sharing its
	// numbers would tie each part's odds to a stat's roll.
	FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Seed), 0x9E3779B9u)));
	const bool bSaved = SavedParts.Num() == Definition.Parts.Num();
	// The tier slot, on a fresh roll only: a saved gun is what it was saved as, and a lost part of one is picked again by
	// the plain draw. Until a slot takes a part of the tier, every slot from the chosen one on tries for one.
	const EWeaponRarity Tier = bSaved ? EWeaponRarity::Common : TierSlotRarity(Definition, Rarity);
	int32 TierFrom = bSaved ? INDEX_NONE : TierSlot(Definition, Seed, Rarity);
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
		const auto Allowed = [&](const FWeaponPartOption& Option)
		{
			return Option.Mesh && Option.MinRarity <= Rarity && Fits(Option, Definition, Look.Parts);
		};
		const auto WeightOf = [Maker, Lean](const FWeaponPartOption& Option)
		{
			// A Touched creature's element leans the pick toward its parts (never forces it).
			const float Leaning = Lean != ELooterElement::None && Option.Element == Lean ? ElementLeanWeight : 1.f;
			return WeaponMakers::PartWeight(Maker, Option) * Leaning;
		};
		const FWeaponPartOption* Part = nullptr;
		if (TierFrom != INDEX_NONE && Index >= TierFrom)
		{
			// The same draw among the slot's parts of the tier and up. None fitting (a long magazine on a stub barrel) hands
			// the rule on to the next slot.
			Part = PickByWeight(Slot.Options, Roll, [&](const FWeaponPartOption& Option) { return Allowed(Option) && Option.MinRarity >= Tier; },
				WeightOf);
			TierFrom = Part ? INDEX_NONE : TierFrom;
		}
		Look.Parts.Add(Part ? Part : PickByWeight(Slot.Options, Roll, Allowed, WeightOf));
	}
	for (const FWeaponPaint& Paint : Definition.Paints)
	{
		const float Roll = Random.FRand();
		if (const FWeaponColorOption* Color = PickByWeight(Paint.Colors, Roll, [](const FWeaponColorOption&) { return true; },
			[](const FWeaponColorOption& Option) { return Option.Weight; }))
		{
			Look.Colors.Add(Paint.Slot, Color->Color);
		}
	}
	// The maker's paints over the definition's, from their own stream: the definition's draws above never move.
	if (Maker)
	{
		WeaponMakers::PaintLook(*Maker, Seed, Look);
	}
	return Look;
}

FWeaponLook WeaponParts::Pick(const FWeaponInstanceData& Instance)
{
	return Instance.Definition ? Pick(*Instance.Definition, Instance.Seed, Instance.Rarity, Instance.Parts, WeaponMakers::Of(Instance))
		: FWeaponLook();
}

EWeaponRarity WeaponParts::TierSlotRarity(const UWeaponDefinition& Definition, EWeaponRarity Rarity)
{
	// Its own tier where a slot offers one, else the best below it down to Rare: few guns have a Legendary part, and a
	// Legendary should be sure of at least what an Epic is.
	for (int32 Tier = static_cast<int32>(Rarity); Tier >= static_cast<int32>(TierSlotFrom); --Tier)
	{
		const EWeaponRarity AtLeast = static_cast<EWeaponRarity>(Tier);
		if (Definition.Parts.ContainsByPredicate([AtLeast, Rarity](const FWeaponPartSlot& Slot) { return OffersTier(Slot, AtLeast, Rarity); }))
		{
			return AtLeast;
		}
	}
	return EWeaponRarity::Common;
}

int32 WeaponParts::TierSlot(const UWeaponDefinition& Definition, int32 Seed, EWeaponRarity Rarity)
{
	const EWeaponRarity Tier = TierSlotRarity(Definition, Rarity);
	if (CVarTierSlot.GetValueOnAnyThread() == 0 || Tier < TierSlotFrom)
	{
		return INDEX_NONE;
	}
	TArray<int32, TInlineAllocator<8>> Offering;
	for (int32 Index = 0; Index < Definition.Parts.Num(); ++Index)
	{
		if (OffersTier(Definition.Parts[Index], Tier, Rarity))
		{
			Offering.Add(Index);
		}
	}
	// Every slot that offers one as likely (a better barrel as often as a better sight), from a stream of its own.
	FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Seed), TierSlotSalt)));
	return Offering.IsEmpty() ? INDEX_NONE : Offering[Random.RandHelper(Offering.Num())];
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

FString WeaponParts::BaseName(const FWeaponInstanceData& Instance)
{
	if (Instance.Named && !Instance.Named->DisplayName.IsEmpty())
	{
		return Instance.Named->DisplayName.ToString();
	}
	if (!Instance.Definition)
	{
		return TEXT("Unknown");
	}
	FText Prefix = NamePrefix(Pick(Instance));
	// A gun no part names goes by its maker ("Meridian Longhorn" rather than a bare "Longhorn").
	if (Prefix.IsEmpty())
	{
		Prefix = WeaponMakers::NameWord(Instance);
	}
	const FString Weapon = Instance.Definition->DisplayName.ToString();
	return Prefix.IsEmpty() ? Weapon : FString::Printf(TEXT("%s %s"), *Prefix.ToString(), *Weapon);
}

EPartEffect WeaponParts::EffectOf(const FWeaponLook& Look)
{
	for (const FWeaponPartOption* Part : Look.Parts)
	{
		if (Part && Part->Effect != EPartEffect::None)
		{
			return Part->Effect;
		}
	}
	return EPartEffect::None;
}

EPartEffect WeaponParts::EffectOf(const FWeaponInstanceData& Instance)
{
	return Instance.Definition ? EffectOf(Pick(Instance)) : EPartEffect::None;
}

ELooterElement WeaponParts::ElementOf(const FWeaponLook& Look)
{
	for (const FWeaponPartOption* Part : Look.Parts)
	{
		if (Part && Part->Element != ELooterElement::None)
		{
			return Part->Element;
		}
	}
	return ELooterElement::None;
}

ELooterElement WeaponParts::ElementOf(const FWeaponInstanceData& Instance)
{
	return Instance.Definition ? ElementOf(Pick(Instance)) : ELooterElement::None;
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
	const float Rolled = FMath::Lerp(Range.X, Range.Y, Random.FRand());
	// Its maker's on top (Windfall's salvage comes battered); no draw, so the roll above is the same with or without one.
	return FMath::Clamp(Rolled + WeaponMakers::ExtraWear(Instance), 0.f, 1.f);
}

float WeaponParts::RarityGlow(EWeaponRarity Rarity)
{
	return Rarity == EWeaponRarity::Common ? 0.6f : 2.5f;
}
