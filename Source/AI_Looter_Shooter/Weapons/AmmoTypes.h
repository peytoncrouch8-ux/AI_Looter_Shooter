#pragma once

#include "CoreMinimal.h"
#include "AmmoTypes.generated.h"

/**
 * Ammo classes, one per weapon type (never per rarity): every assault rifle, Common to Legendary, uses AR ammo. The
 * player carries one shared pool per class (like Borderlands), so ammo found for a gun type you don't own yet is
 * waiting for you when you do.
 */
UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	AssaultRifle,
	Shotgun,
	Pistol,
	SMG,
	Sniper,
	/** The launcher's rockets (2026-10-21; appended, so the classes above keep their numbers and saved pools their slots). */
	Rocket,
	Count UMETA(Hidden)
};

/**
 * Game-wide ammo rules: names, how much the player can carry and how much a spawned ammo box holds. Ammo deliberately
 * has no class colors: the rarity colors are the game's only color code, so colored ammo read as rarity tiers.
 */
namespace LooterAmmo
{
	inline constexpr int32 NumTypes = static_cast<int32>(EAmmoType::Count);

	struct FInfo
	{
		const TCHAR* Name;         // "AR Ammo"
		int32 MaxCarried;          // pool cap
		int32 PickupAmount;        // one box from Looter.SpawnAmmo (kills and chests drop LooterLoot's amounts instead)
		const TCHAR* Short;        // "AR": the ammo class on the HUD
		/**
		 * The most a kill's pickup of this class holds, its "box" (the user, 2026-10-29 night: AR 18-36, SMG 24-48, pistol
		 * 12-24, shotgun 4-8, sniper 3-6; the least is half of it; a chest 1.5 boxes). LooterLoot::AmmoAmountFor scales a
		 * table's AR-rounds roll by KillBox / 36. Rockets don't read it (LooterLoot::RocketsFor).
		 */
		int32 KillBox;
	};

	AI_LOOTER_SHOOTER_API const FInfo& GetInfo(EAmmoType Type);

	inline bool IsValid(EAmmoType Type) { return static_cast<int32>(Type) >= 0 && static_cast<int32>(Type) < NumTypes; }

	/** Every real ammo type, for iterating. */
	AI_LOOTER_SHOOTER_API TConstArrayView<EAmmoType> AllTypes();
}
