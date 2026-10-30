#include "Missions/MissionRewards.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponMakers.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Elements/ElementRules.h"
#include "Missions/MissionDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponPartSwap.h"
#include "Weapons/WeaponParts.h"

// A reward gun made to the mission's word: a maker, an element (the Ransom's Rest tuning the user took, 2026-10-29 night:
// Main 4 gives a Saint Maud Uncommon with Hallow rounds, Knocking Below a Cinder Rare). Kept apart from MissionRewards.cpp,
// which hands the rewards over.

namespace
{
	/** Fits Element's part into the first slot that has one the gun can take (its rarity, the other parts' Requires). */
	bool FitElementPart(FWeaponInstanceData& Gun, ELooterElement Element)
	{
		const UWeaponDefinition* Kind = Gun.Definition;
		if (!Kind)
		{
			return false;
		}
		for (const FWeaponPartSlot& Slot : Kind->Parts)
		{
			for (const FWeaponPartOption& Option : Slot.Options)
			{
				if (Option.Element != Element)
				{
					continue;
				}
				FBoxedWeaponPart Part;
				Part.Definition = Gun.Definition;
				Part.Slot = Slot.Name;
				Part.Key = Option.Key;
				FBoxedWeaponPart Replaced;
				// The bench's own fitting: the gun keeps its seed, so the part's numbers fall as if it had rolled with it.
				if (WeaponPartSwap::Fit(Gun, Part, Replaced) && WeaponParts::ElementOf(Gun) == Element)
				{
					return true;
				}
			}
		}
		return false;
	}
}

FWeaponInstanceData MissionRewards::MakeRewardGun(UWeaponDefinition* Kind, EWeaponRarity Rarity, int32 Level, const FMissionRewards& Rewards)
{
	FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponLeaning(Kind, Rarity, Level, Rewards.GunElement);
	if (!Kind)
	{
		return Gun;
	}
	if (!Rewards.GunMaker.IsNone())
	{
		if (const FWeaponMaker* Maker = WeaponMakers::Find(Rewards.GunMaker))
		{
			// Picked again with the maker's leanings, from the same seed, as a gun of that maker would roll.
			Gun.Maker = Maker->Key;
			Gun.Parts = WeaponParts::PartKeys(WeaponParts::Pick(*Kind, Gun.Seed, Gun.Rarity, {}, Maker, Rewards.GunElement));
			Gun.Stats = UWeaponRollLibrary::ComputeInstanceStats(Gun);
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Missions: a reward gun names no maker %s; it keeps the one it rolled."), *Rewards.GunMaker.ToString());
		}
	}
	if (Rewards.GunElement != ELooterElement::None && WeaponParts::ElementOf(Gun) != Rewards.GunElement && !FitElementPart(Gun, Rewards.GunElement))
	{
		UE_LOG(LogLooter, Warning, TEXT("Missions: %s has no %s part a %d-rarity gun can take; the reward gun keeps its own."),
			*Kind->GetName(), *ElementRules::Word(Rewards.GunElement), static_cast<int32>(Rarity));
	}
	return Gun;
}

FString MissionRewards::GunWords(const FMissionRewards& Rewards)
{
	FString Words;
	if (const FWeaponMaker* Maker = Rewards.GunMaker.IsNone() ? nullptr : WeaponMakers::Find(Rewards.GunMaker))
	{
		Words += Maker->Name.ToString() + TEXT(", ");
	}
	if (Rewards.GunElement != ELooterElement::None)
	{
		// The card's word in plain case ("Hallow", not the callout's "HALLOW").
		Words += ElementRules::Word(Rewards.GunElement).Left(1) + ElementRules::Word(Rewards.GunElement).Mid(1).ToLower() + TEXT(", ");
	}
	return Words;
}
