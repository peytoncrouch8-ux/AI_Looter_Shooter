#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

class AActor;

/**
 * A curse: a strong perk with a real drawback. Numbers at 1 / 0 / false do nothing. The perk is always on; the drawback
 * is gone once the curse is lifted (FWeaponInstanceData::bCurseLifted).
 */
struct FWeaponCurse
{
	FName Key;
	FText Name;      // "Hungry"
	FText Perk;      // "+30% damage"
	FText Drawback;  // "Each reload costs 3% of your max health"
	// The perk (always on):
	float DamageMultiplier = 1.f;      // Hungry 1.3, Cold 1.2
	float FireRateMultiplier = 1.f;    // Restless 1.4
	float CritDamageMultiplier = 1.f;  // Cold 1.25: on top of the critical hit multiplier
	float CritMultiplierOverride = 0.f;// Unlucky 3: critical hits deal triple instead of LooterCombat's 1.5x
	float KillLootLuck = 0.f;          // Greedy 0.5: a kill with it rolls its loot at this much more luck
	float KillHealShare = 0.f;         // Grasping 0.05: a kill with it heals this share of max health
	// The drawback (gone once lifted):
	float ReloadHealthCostShare = 0.f; // Hungry 0.03 of max health per reload
	int32 RoundsPerShot = 1;           // Greedy 2
	float RecoilMultiplier = 1.f;      // Restless 2
	bool bBlocksSprint = false;        // Cold: no sprinting with it in hand
	float MaxHealthMultiplier = 1.f;   // Grasping 0.85 while it's in hand
	float MisfireChance = 0.f;         // Unlucky 0.125: one shot in eight misfires
};

/**
 * Cursed irons: a few Rare, Epic and Legendary guns drop with a curse. The curses live in a table in WeaponCurses.cpp,
 * keyed by names that never change once guns drop (a gun saves its curse's key, as it saves its parts' keys). Plain
 * numbers (damage, fire rate, recoil) reach the gun's stats through UWeaponRollLibrary::ComputeInstanceStats; the rest
 * hook in where they act (firing, reloading, sprinting, health, the loot roll).
 */
namespace WeaponCurses
{
	/** The share of Rare, Epic and Legendary guns that drop cursed. */
	inline constexpr float CurseChance = 0.06f;

	AI_LOOTER_SHOOTER_API TConstArrayView<FWeaponCurse> All();
	AI_LOOTER_SHOOTER_API const FWeaponCurse* Find(FName Key);
	/** The gun's curse, or null (not cursed, or a key that no longer exists). */
	AI_LOOTER_SHOOTER_API const FWeaponCurse* Of(const FWeaponInstanceData& Gun);
	/** Cursed and its drawback still in force. */
	AI_LOOTER_SHOOTER_API bool DrawbackActive(const FWeaponInstanceData& Gun);
	/** The curse a dropped gun rolls, from its seed and rarity (deterministic): none for Common, Uncommon and named guns. */
	AI_LOOTER_SHOOTER_API FName Roll(const FWeaponInstanceData& Gun);
	/** The curse's plain numbers on the gun's stats: damage and fire rate (perk), recoil (drawback unless lifted). */
	AI_LOOTER_SHOOTER_API void ApplyToStats(const FWeaponInstanceData& Gun, FWeaponStats& InOutStats);
	/** A critical hit's multiplier with this gun: LooterCombat's, or the curse's override, times its crit damage perk. */
	AI_LOOTER_SHOOTER_API float CritMultiplier(const FWeaponInstanceData& Gun);
	/** The curse of the gun in Holder's hand (a pawn with a UWeaponManagerComponent), or null. */
	AI_LOOTER_SHOOTER_API const FWeaponCurse* InHand(const AActor* Holder);
	/** InHand, but only while its drawback is in force (null once lifted): for the drawbacks that act on the holder (max health). */
	AI_LOOTER_SHOOTER_API const FWeaponCurse* DrawbackInHand(const AActor* Holder);
	/** The gun in Holder's hand is cursed Cold with its drawback in force: the locomotion won't sprint. */
	AI_LOOTER_SHOOTER_API bool BlocksSprint(const AActor* Holder);
}
