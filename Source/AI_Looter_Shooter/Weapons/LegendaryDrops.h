#pragma once

#include "CoreMinimal.h"
#include "Weapons/LegendaryTuning.h"

class UNamedWeaponDefinition;
struct FLootRoll;
struct FRandomStream;

/**
 * Each Ransom's Rest boss's own legendary (the user's call, 2026-10-29 night, Borderlands 2's dedicated drops): its loot
 * shower (ABossLootShower::Throw) rolls it on top of the boss's table, Looter.Legendary.<Name>.DropChance a kill, the first
 * kill and every vigil stone refight alike (a refight's loot rolls as the first's). One a kill at most. The boss names it in
 * FBossLootShowerSettings::DedicatedLegendary by its named gun's id (DA_Named_<Id>, Tools/Unreal/create_named_weapons.py).
 */
namespace LegendaryDrops
{
	/** The named guns' ids (never rename: the assets and the bosses' settings name them). */
	inline constexpr const TCHAR* GravesilkId = TEXT("Gravesilk");
	inline constexpr const TCHAR* CompanyScripId = TEXT("CompanyScrip");
	inline constexpr const TCHAR* LamplighterId = TEXT("Lamplighter");

	/**
	 * Adds Named at Level to InOutRoll's guns when Roll01 comes under its signature's chance (LegendaryRules::DropChance),
	 * unless the roll has it already (one a kill). True if it was added.
	 */
	AI_LOOTER_SHOOTER_API bool AddDedicated(UNamedWeaponDefinition* Named, int32 Level, float Roll01, const FLegendaryTuning& Tuning,
		FLootRoll& InOutRoll);

	/** A boss's kill: the named gun NamedId (None: nothing) rolled from Random into InOutRoll. True if it drops. */
	AI_LOOTER_SHOOTER_API bool RollForBoss(FName NamedId, int32 Level, FRandomStream& Random, FLootRoll& InOutRoll);
}
