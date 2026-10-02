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
	};

	AI_LOOTER_SHOOTER_API const FInfo& GetInfo(EAmmoType Type);

	inline bool IsValid(EAmmoType Type) { return static_cast<int32>(Type) >= 0 && static_cast<int32>(Type) < NumTypes; }

	/** Every real ammo type, for iterating. */
	AI_LOOTER_SHOOTER_API TConstArrayView<EAmmoType> AllTypes();
}
