#pragma once

#include "CoreMinimal.h"
#include "Weapons/AmmoTypes.h"

class UPlayerThrowComponent;
class UWalletSubsystem;
class UWeaponManagerComponent;

/** How a trade at a vendor went, for the screen's words and sounds. */
enum class EVendorTrade : uint8
{
	/** Done: paid, or sold. */
	Done,
	/** The price is more than the silver carried. */
	NotEnoughSilver,
	/** The player can't carry more of it (ammo, grave salt). */
	Full,
	/** The gun of the day has nowhere to go: every equip slot and the backpack are full. */
	NoRoom,
	/** A vendor won't buy that gun (a named one). */
	NotForSale,
	/** The gun of the day is bought already. */
	SoldOut,
	/** Nothing there (a backpack place that's empty, no junk marked, no gun of the day). */
	Nothing
};

/** A trade's outcome: how it went, the silver paid or earned, and how much changed hands (rounds, jars or guns). */
struct AI_LOOTER_SHOOTER_API FVendorTradeResult
{
	EVendorTrade Result = EVendorTrade::Nothing;
	int32 Silver = 0;
	int32 Count = 0;

	bool IsDone() const { return Result == EVendorTrade::Done; }
};

/** A box of ammo as a vendor offers it now: the rounds that fit (a part box when the pool is nearly full), its price. */
struct AI_LOOTER_SHOOTER_API FVendorAmmoOffer
{
	EAmmoType Type = EAmmoType::AssaultRifle;
	int32 Rounds = 0;
	int32 Price = 0;
	int32 Carried = 0;
	int32 Max = 0;

	/** Nothing fits: the player carries all they can. */
	bool IsFull() const { return Rounds <= 0; }
};

/** The junk-marked guns in the backpack that a vendor would buy. */
struct AI_LOOTER_SHOOTER_API FVendorJunkTally
{
	int32 Count = 0;
	int32 Silver = 0;
	/** At least one is Rare or better: selling them all asks first. */
	bool bAnyRare = false;
};

/**
 * What a vendor does (Docs/Progression.md, "Economy"), apart from its screen so the tests drive it: sells ammo by the box
 * (LooterEconomy's prices at the player's level), grave salt by the jar (up to the three carried), and the gun of the day
 * (UWalletSubsystem::GetOffer); buys guns from the backpack (never an equipped one, never a named one), one at a time or
 * every junk-marked one at once. Every trade pays or is paid through the wallet, all or nothing.
 */
namespace VendorTrade
{
	/** The ammo a vendor stocks: what the game's guns use (the default loot table's ammo classes), else rifle, shells and pistol. */
	AI_LOOTER_SHOOTER_API TArray<EAmmoType> StockedAmmo();

	/** What a box of Type is now for the player Manager belongs to, at PlayerLevel. */
	AI_LOOTER_SHOOTER_API FVendorAmmoOffer AmmoOffer(const UWeaponManagerComponent& Manager, EAmmoType Type, int32 PlayerLevel);

	/** Buys a box of Type (what fits of it): Full when nothing fits, NotEnoughSilver when it can't be paid. */
	AI_LOOTER_SHOOTER_API FVendorTradeResult BuyAmmo(UWalletSubsystem& Wallet, UWeaponManagerComponent& Manager, EAmmoType Type, int32 PlayerLevel);

	/** Buys one jar of grave salt into Throw's count (it unlocks the counter): Full at the most carried. */
	AI_LOOTER_SHOOTER_API FVendorTradeResult BuyGrenade(UWalletSubsystem& Wallet, UPlayerThrowComponent& Throw, int32 PlayerLevel);

	/** The gun of the day has somewhere to go: room in the backpack, or a free equip slot. */
	AI_LOOTER_SHOOTER_API bool HasRoomForGun(const UWeaponManagerComponent& Manager);

	/**
	 * Buys the gun of the day as the wallet has it (call UWalletSubsystem::RefreshOffer first): into the backpack, else a
	 * free equip slot. Then it's sold out until the next one.
	 */
	AI_LOOTER_SHOOTER_API FVendorTradeResult BuyOffer(UWalletSubsystem& Wallet, UWeaponManagerComponent& Manager);

	/** Sells the backpack's gun at BackpackIndex (LooterEconomy::GunSellPrice). */
	AI_LOOTER_SHOOTER_API FVendorTradeResult SellGun(UWalletSubsystem& Wallet, UWeaponManagerComponent& Manager, int32 BackpackIndex);

	/** What the backpack's junk would fetch. */
	AI_LOOTER_SHOOTER_API FVendorJunkTally TallyJunk(const UWeaponManagerComponent& Manager);

	/** Sells every junk-marked backpack gun a vendor buys: Nothing when there's none. */
	AI_LOOTER_SHOOTER_API FVendorTradeResult SellAllJunk(UWalletSubsystem& Wallet, UWeaponManagerComponent& Manager);

	/** Turns the junk mark on the backpack's gun at BackpackIndex over; false when it can't be marked (a named gun, none there). */
	AI_LOOTER_SHOOTER_API bool ToggleJunk(UWeaponManagerComponent& Manager, int32 BackpackIndex);
}
