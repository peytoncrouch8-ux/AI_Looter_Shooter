#pragma once

#include "CoreMinimal.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponManagerTypes.generated.h"

class AWeaponBase;
class UWeaponDefinition;

// The small types UWeaponManagerComponent hands out: its events, its starting guns, and a carried gun's place.

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveWeaponChanged, AWeaponBase*, NewWeapon, AWeaponBase*, OldWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponMessage, const FText&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoPoolChanged, EAmmoType, Type, int32, Carried);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoPickedUp, EAmmoType, Type, int32, Amount);

USTRUCT(BlueprintType)
struct FStartingWeapon
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> Definition = nullptr;

	/** If false, the rarity below is used instead of a random roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	bool bRandomRarity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (EditCondition = "!bRandomRarity"))
	EWeaponRarity Rarity = EWeaponRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1"))
	int32 Level = 1;
};

/** A gun the player carries, as the gunsmith's bench names it: in an equip slot, or in the backpack. */
struct FCarriedGun
{
	/** In the backpack; otherwise in an equip slot. */
	bool bBackpack = false;

	/** The equip slot, or its place in the backpack. */
	int32 Index = INDEX_NONE;

	static FCarriedGun Equipped(int32 Slot)
	{
		FCarriedGun Gun;
		Gun.Index = Slot;
		return Gun;
	}

	static FCarriedGun InBackpack(int32 BackpackIndex)
	{
		FCarriedGun Gun;
		Gun.bBackpack = true;
		Gun.Index = BackpackIndex;
		return Gun;
	}

	bool operator==(const FCarriedGun& Other) const { return bBackpack == Other.bBackpack && Index == Other.Index; }
};
