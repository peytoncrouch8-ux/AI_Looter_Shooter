#include "Weapons/LegendaryDrops.h"
#include "AI_Looter_Shooter.h"
#include "Loot/LootLibrary.h"
#include "Math/RandomStream.h"
#include "Weapons/LegendaryRules.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponTypes.h"

bool LegendaryDrops::AddDedicated(UNamedWeaponDefinition* Named, int32 Level, float Roll01, const FLegendaryTuning& Tuning, FLootRoll& InOutRoll)
{
	if (!Named || !Named->Weapon)
	{
		return false;
	}
	// One a kill: a roll that has it already (asked twice for one death) gets no second.
	if (InOutRoll.Weapons.ContainsByPredicate([Named](const FWeaponInstanceData& Gun) { return Gun.Named == Named; }))
	{
		return false;
	}
	if (!LegendaryRules::RollsDrop(LegendaryRules::DropChance(Named->Signature, Tuning), Roll01))
	{
		return false;
	}
	InOutRoll.Weapons.Add(Named->MakeInstance(Level));
	return true;
}

bool LegendaryDrops::RollForBoss(FName NamedId, int32 Level, FRandomStream& Random, FLootRoll& InOutRoll)
{
	if (NamedId.IsNone())
	{
		return false;
	}
	// Rolled whether or not the asset is in (the stream's draws stay the same either way).
	const float Roll = Random.GetFraction();
	UNamedWeaponDefinition* Named = UNamedWeaponDefinition::FindByName(NamedId.ToString());
	if (!Named)
	{
		UE_LOG(LogLooter, Warning, TEXT("A boss's legendary %s isn't in the project: run Tools/Unreal/create_named_weapons.py."), *NamedId.ToString());
		return false;
	}
	if (Named->Signature == ELegendarySignature::None)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s has no signature: run Tools/Unreal/create_named_weapons.py again."), *Named->GetName());
	}
	const bool bDropped = AddDedicated(Named, Level, Roll, FLegendaryTuning::Live(), InOutRoll);
	UE_LOG(LogLooter, Log, TEXT("The boss's own legendary %s: rolled %.3f against %.3f, %s."), *NamedId.ToString(), Roll,
		LegendaryRules::DropChance(Named->Signature, FLegendaryTuning::Live()), bDropped ? TEXT("it drops") : TEXT("not this time"));
	return bDropped;
}
