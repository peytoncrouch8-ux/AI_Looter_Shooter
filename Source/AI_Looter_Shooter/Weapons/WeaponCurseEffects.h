#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

class AActor;
class AWeaponBase;

/**
 * Where a cursed iron's curse acts in play (the curses and their numbers are WeaponCurses'): a shot's misfire and the
 * rounds it spends, a reload's toll on health, the holder's max health while it's in hand, a kill's heal and loot luck.
 * The plain numbers (damage, fire rate, recoil, critical hits) reach the gun through its stats and its bullets instead.
 * The rules are plain functions, so the tests roll them from a seeded stream; the rest apply them to a gun's holder.
 * Cold's no-sprint is the locomotion's (WeaponCurses::BlocksSprint).
 */
namespace WeaponCurseEffects
{
	/** What one pull of the trigger does. */
	struct FShotPlan
	{
		/** Rounds it takes from the magazine: Greedy's 2, or what's left when that's fewer. */
		int32 Rounds = 1;
		/** The hammer falls on a dud (Unlucky): the round is spent and no bullet leaves the barrel. */
		bool bMisfire = false;
	};

	/**
	 * The next shot of Gun with RoundsInMagazine (at least 1) left. Only a gun whose drawback is in force draws from Rolls,
	 * and only when it can misfire, so other guns never disturb the stream.
	 */
	AI_LOOTER_SHOOTER_API FShotPlan PlanShot(const FWeaponInstanceData& Gun, int32 RoundsInMagazine, FRandomStream& Rolls);

	/** Health a reload with Gun costs a holder of MaxHealth (Hungry: a share of it); 0 for the rest, and once lifted. */
	AI_LOOTER_SHOOTER_API float ReloadHealthCost(const FWeaponInstanceData& Gun, float MaxHealth);

	/** Its holder's max health multiplier while Gun is in hand (Grasping's 0.85); 1 for the rest, and once lifted. */
	AI_LOOTER_SHOOTER_API float MaxHealthMultiplier(const FWeaponInstanceData& Gun);

	/** Health a kill with Gun gives back to a holder of MaxHealth (Grasping: a share of it; a perk, so lifted too). */
	AI_LOOTER_SHOOTER_API float KillHeal(const FWeaponInstanceData& Gun, float MaxHealth);

	/** Extra luck a kill with this gun rolls its loot at (Greedy's; a perk); 0 for none. */
	AI_LOOTER_SHOOTER_API float KillLootLuck(const AWeaponBase* KillWeapon);

	/**
	 * Sets Holder's max health for the gun in their hand now (UHealthComponent::SetMaxHealthScale: its health keeps its
	 * share). Leaving is a gun on its way out of their hand, counted as gone already. Nothing for a holder without health.
	 */
	AI_LOOTER_SHOOTER_API void RefreshHolder(AActor* Holder, const AWeaponBase* Leaving = nullptr);

	/** Takes a finished reload's toll from Gun's holder (Hungry): never their last point of health. */
	AI_LOOTER_SHOOTER_API void PayReload(const AWeaponBase& Gun);

	/** A creature killed with Gun: its curse's kill perks for its holder (Grasping's heal). */
	AI_LOOTER_SHOOTER_API void ApplyKillPerks(const AWeaponBase& Gun);
}
