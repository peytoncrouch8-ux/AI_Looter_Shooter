#include "UI/Inventory/LoadoutRules.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"

LoadoutRules::EVerdict LoadoutRules::Compare(const FWeaponInstanceData& Candidate, const FWeaponInstanceData* Current)
{
	if (!Current || !Candidate.Definition || Candidate.Definition != Current->Definition)
	{
		return EVerdict::None;
	}
	auto DamagePerSecond = [](const FWeaponStats& Stats)
	{
		return Stats.Damage * Stats.PelletsPerShot * Stats.FireRate / 60.f;
	};
	const float Theirs = DamagePerSecond(Candidate.Stats);
	const float Mine = DamagePerSecond(Current->Stats);
	if (Theirs > Mine * 1.03f)
	{
		return EVerdict::Upgrade;
	}
	return Theirs < Mine * 0.97f ? EVerdict::Weaker : EVerdict::Similar;
}

int32 LoadoutRules::SortForSwap(const TArray<FWeaponInstanceData>& Backpack, const UWeaponDefinition* Kind, TArray<int32>& OutOrder)
{
	OutOrder.Reset(Backpack.Num());
	if (Kind)
	{
		for (int32 Index = 0; Index < Backpack.Num(); ++Index)
		{
			if (Backpack[Index].Definition == Kind)
			{
				OutOrder.Add(Index);
			}
		}
	}
	const int32 NumKind = OutOrder.Num();
	for (int32 Index = 0; Index < Backpack.Num(); ++Index)
	{
		if (!Kind || Backpack[Index].Definition != Kind)
		{
			OutOrder.Add(Index);
		}
	}
	return NumKind;
}
