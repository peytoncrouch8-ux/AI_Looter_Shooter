#include "Weapons/WeaponCurses.h"
#include "Combat/CombatRules.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "GameFramework/Actor.h"

namespace
{
	FWeaponCurse MakeCurse(const TCHAR* Key, const TCHAR* Perk, const TCHAR* Drawback)
	{
		FWeaponCurse Curse;
		Curse.Key = FName(Key);
		Curse.Name = FText::FromString(Key);
		Curse.Perk = FText::FromString(Perk);
		Curse.Drawback = FText::FromString(Drawback);
		return Curse;
	}

	/**
	 * The curses. A gun saves its curse's key, so a key never changes once guns drop (as part keys never do); a curse's
	 * numbers can be tuned and every gun with it follows. Roll picks by place in the list, which only moves the curses
	 * guns drop with from then on.
	 */
	TArray<FWeaponCurse> MakeTable()
	{
		TArray<FWeaponCurse> Table;
		// Room for all of them up front: each curse is filled in through a reference into the list.
		Table.Reserve(6);

		FWeaponCurse& Hungry = Table.Add_GetRef(MakeCurse(TEXT("Hungry"), TEXT("+30% damage"), TEXT("Each reload costs 3% of your max health")));
		Hungry.DamageMultiplier = 1.3f;
		Hungry.ReloadHealthCostShare = 0.03f;

		FWeaponCurse& Greedy = Table.Add_GetRef(MakeCurse(TEXT("Greedy"), TEXT("Kills with it drop rarer loot"), TEXT("Each shot spends 2 rounds")));
		Greedy.KillLootLuck = 0.5f;
		Greedy.RoundsPerShot = 2;

		FWeaponCurse& Restless = Table.Add_GetRef(MakeCurse(TEXT("Restless"), TEXT("+40% fire rate"), TEXT("Recoil doubles")));
		Restless.FireRateMultiplier = 1.4f;
		Restless.RecoilMultiplier = 2.f;

		FWeaponCurse& Cold = Table.Add_GetRef(MakeCurse(TEXT("Cold"), TEXT("+20% damage and +25% critical damage"), TEXT("No sprinting with it in hand")));
		Cold.DamageMultiplier = 1.2f;
		Cold.CritDamageMultiplier = 1.25f;
		Cold.bBlocksSprint = true;

		FWeaponCurse& Grasping = Table.Add_GetRef(MakeCurse(TEXT("Grasping"), TEXT("Kills with it heal 5% of your max health"),
			TEXT("15% less max health with it in hand")));
		Grasping.KillHealShare = 0.05f;
		Grasping.MaxHealthMultiplier = 0.85f;

		FWeaponCurse& Unlucky = Table.Add_GetRef(MakeCurse(TEXT("Unlucky"), TEXT("Critical hits deal triple damage"), TEXT("One shot in eight misfires")));
		Unlucky.CritMultiplierOverride = 3.f;
		Unlucky.MisfireChance = 0.125f;

		return Table;
	}

	/** The gun in Holder's hand, or null. */
	const AWeaponBase* HeldWeapon(const AActor* Holder)
	{
		const UWeaponManagerComponent* Manager = Holder ? Holder->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
		const AWeaponBase* Weapon = Manager ? Manager->GetActiveWeapon() : nullptr;
		return IsValid(Weapon) ? Weapon : nullptr;
	}
}

TConstArrayView<FWeaponCurse> WeaponCurses::All()
{
	// Made on first use rather than at startup, when the text system may not be up yet.
	static const TArray<FWeaponCurse> Table = MakeTable();
	return Table;
}

const FWeaponCurse* WeaponCurses::Find(FName Key)
{
	if (Key.IsNone())
	{
		return nullptr;
	}
	for (const FWeaponCurse& Curse : All())
	{
		if (Curse.Key == Key)
		{
			return &Curse;
		}
	}
	return nullptr;
}

const FWeaponCurse* WeaponCurses::Of(const FWeaponInstanceData& Gun)
{
	return Find(Gun.Curse);
}

bool WeaponCurses::DrawbackActive(const FWeaponInstanceData& Gun)
{
	return !Gun.bCurseLifted && Of(Gun) != nullptr;
}

FName WeaponCurses::Roll(const FWeaponInstanceData& Gun)
{
	// Only the rarer rolled guns: commons and uncommons stay plain, and a named gun is one gun, never a cursed copy.
	if (Gun.Named || Gun.Rarity < EWeaponRarity::Rare)
	{
		return NAME_None;
	}
	const TConstArrayView<FWeaponCurse> Curses = All();
	if (Curses.IsEmpty())
	{
		return NAME_None;
	}
	// A stream of its own from the seed and rarity, so a curse never follows the parts', the stats', the wear's or the
	// nickname's rolls, and the same seed at another rarity rolls afresh.
	FRandomStream Random(static_cast<int32>(HashCombine(static_cast<uint32>(Gun.Seed), HashCombine(static_cast<uint32>(Gun.Rarity), 0xC1A55EDu))));
	if (Random.FRand() >= CurseChance)
	{
		return NAME_None;
	}
	return Curses[Random.RandHelper(Curses.Num())].Key;
}

void WeaponCurses::ApplyToStats(const FWeaponInstanceData& Gun, FWeaponStats& InOutStats)
{
	const FWeaponCurse* Curse = Of(Gun);
	if (!Curse)
	{
		return;
	}
	InOutStats.Damage *= Curse->DamageMultiplier;
	InOutStats.FireRate *= Curse->FireRateMultiplier;
	if (!Gun.bCurseLifted)
	{
		InOutStats.Recoil *= Curse->RecoilMultiplier;
	}
}

float WeaponCurses::CritMultiplier(const FWeaponInstanceData& Gun)
{
	// Both are perks, so they stay once the curse is lifted.
	const FWeaponCurse* Curse = Of(Gun);
	if (!Curse)
	{
		return LooterCombat::CriticalHitMultiplier;
	}
	const float Base = Curse->CritMultiplierOverride > 0.f ? Curse->CritMultiplierOverride : LooterCombat::CriticalHitMultiplier;
	return Base * Curse->CritDamageMultiplier;
}

const FWeaponCurse* WeaponCurses::InHand(const AActor* Holder)
{
	const AWeaponBase* Weapon = HeldWeapon(Holder);
	return Weapon ? Of(Weapon->GetInstance()) : nullptr;
}

const FWeaponCurse* WeaponCurses::DrawbackInHand(const AActor* Holder)
{
	const AWeaponBase* Weapon = HeldWeapon(Holder);
	return Weapon && DrawbackActive(Weapon->GetInstance()) ? Of(Weapon->GetInstance()) : nullptr;
}

bool WeaponCurses::BlocksSprint(const AActor* Holder)
{
	const FWeaponCurse* Curse = DrawbackInHand(Holder);
	return Curse && Curse->bBlocksSprint;
}
