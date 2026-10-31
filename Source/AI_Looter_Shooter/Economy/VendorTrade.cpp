#include "Economy/VendorTrade.h"
#include "AI_Looter_Shooter.h"
#include "Economy/EconomyRules.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Player/PlayerThrowComponent.h"
#include "Player/PlayerThrowRules.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"

TArray<EAmmoType> VendorTrade::StockedAmmo()
{
	TArray<EAmmoType> Types;
	if (const ULootTable* Table = ULootLibrary::GetDefaultLootTable())
	{
		for (const FAmmoLootEntry& Entry : Table->AmmoTypes)
		{
			if (LooterAmmo::IsValid(Entry.Type) && Entry.Weight > 0.f)
			{
				Types.AddUnique(Entry.Type);
			}
		}
	}
	if (Types.IsEmpty())
	{
		Types = { EAmmoType::AssaultRifle, EAmmoType::Shotgun, EAmmoType::Pistol };
	}
	// In the game's own order, so the screen's rows never shuffle.
	Types.Sort([](EAmmoType A, EAmmoType B) { return static_cast<uint8>(A) < static_cast<uint8>(B); });
	return Types;
}

FVendorAmmoOffer VendorTrade::AmmoOffer(const UWeaponManagerComponent& Manager, EAmmoType Type, int32 PlayerLevel)
{
	FVendorAmmoOffer Offer;
	Offer.Type = Type;
	Offer.Carried = Manager.GetAmmo(Type);
	Offer.Max = Manager.GetMaxAmmo(Type);
	Offer.Rounds = FMath::Clamp(Offer.Max - Offer.Carried, 0, LooterEconomy::AmmoBoxRounds(Type));
	Offer.Price = LooterEconomy::AmmoPrice(Type, Offer.Rounds, PlayerLevel);
	return Offer;
}

FVendorTradeResult VendorTrade::BuyAmmo(UWalletSubsystem& Wallet, UWeaponManagerComponent& Manager, EAmmoType Type, int32 PlayerLevel)
{
	FVendorTradeResult Result;
	const FVendorAmmoOffer Offer = AmmoOffer(Manager, Type, PlayerLevel);
	if (Offer.IsFull())
	{
		Result.Result = EVendorTrade::Full;
		return Result;
	}
	Result.Silver = Offer.Price;
	if (!Wallet.Spend(Offer.Price))
	{
		Result.Result = EVendorTrade::NotEnoughSilver;
		return Result;
	}
	Result.Count = Manager.AddAmmo(Type, Offer.Rounds);
	Result.Result = EVendorTrade::Done;
	UE_LOG(LogLooter, Log, TEXT("Vendors: bought %d %s for %d silver."), Result.Count, LooterAmmo::GetInfo(Type).Name, Offer.Price);
	return Result;
}

int32 VendorTrade::RefillPrice(EAmmoType Type, int32 Rounds, int32 PlayerLevel)
{
	if (Rounds <= 0)
	{
		return 0;
	}
	// Buying box by box asks the full price for each whole box and a part box's share for the last, as BuyAmmo does.
	const int32 Box = LooterEconomy::AmmoBoxRounds(Type);
	return (Rounds / Box) * LooterEconomy::AmmoPrice(Type, Box, PlayerLevel) + LooterEconomy::AmmoPrice(Type, Rounds % Box, PlayerLevel);
}

FVendorRefillTally VendorTrade::TallyRefill(const UWeaponManagerComponent& Manager, int32 PlayerLevel)
{
	FVendorRefillTally Tally;
	for (const EAmmoType Type : StockedAmmo())
	{
		const int32 Lacks = FMath::Max(Manager.GetMaxAmmo(Type) - Manager.GetAmmo(Type), 0);
		if (Lacks > 0)
		{
			++Tally.Types;
			Tally.Rounds += Lacks;
			Tally.Silver += RefillPrice(Type, Lacks, PlayerLevel);
		}
	}
	return Tally;
}

FVendorTradeResult VendorTrade::RefillAllAmmo(UWalletSubsystem& Wallet, UWeaponManagerComponent& Manager, int32 PlayerLevel)
{
	FVendorTradeResult Result;
	const FVendorRefillTally Tally = TallyRefill(Manager, PlayerLevel);
	if (Tally.IsFull())
	{
		Result.Result = EVendorTrade::Full;
		return Result;
	}
	Result.Silver = Tally.Silver;
	// One payment, all or nothing: the rounds go in only once it's paid.
	if (!Wallet.Spend(Tally.Silver))
	{
		Result.Result = EVendorTrade::NotEnoughSilver;
		return Result;
	}
	for (const EAmmoType Type : StockedAmmo())
	{
		const int32 Lacks = FMath::Max(Manager.GetMaxAmmo(Type) - Manager.GetAmmo(Type), 0);
		Result.Count += Lacks > 0 ? Manager.AddAmmo(Type, Lacks) : 0;
	}
	Result.Result = EVendorTrade::Done;
	UE_LOG(LogLooter, Log, TEXT("Vendors: refilled %d rounds over %d ammo types for %d silver."), Result.Count, Tally.Types, Result.Silver);
	return Result;
}

FVendorTradeResult VendorTrade::BuyGrenade(UWalletSubsystem& Wallet, UPlayerThrowComponent& Throw, int32 PlayerLevel)
{
	FVendorTradeResult Result;
	if (Throw.GetGrenades() >= FThrowRules::MaxGrenades)
	{
		Result.Result = EVendorTrade::Full;
		return Result;
	}
	Result.Silver = LooterEconomy::GrenadePrice(PlayerLevel);
	if (!Wallet.Spend(Result.Silver))
	{
		Result.Result = EVendorTrade::NotEnoughSilver;
		return Result;
	}
	Result.Count = Throw.AddGrenades(1, EGrenadeChange::Given);
	Result.Result = EVendorTrade::Done;
	return Result;
}

bool VendorTrade::HasRoomForGun(const UWeaponManagerComponent& Manager)
{
	// The backpack's room is shared with the spare wards (UWeaponManagerComponent::HasBackpackRoom); a slot must be open.
	return Manager.HasBackpackRoom() || Manager.HasFreeSlot();
}

FVendorTradeResult VendorTrade::BuyOffer(UWalletSubsystem& Wallet, UWeaponManagerComponent& Manager)
{
	FVendorTradeResult Result;
	const FVendorOffer& Offer = Wallet.GetOffer();
	if (!Offer.bRolled || !Offer.Gun.Definition)
	{
		return Result;
	}
	if (Offer.bSold)
	{
		Result.Result = EVendorTrade::SoldOut;
		return Result;
	}
	Result.Silver = LooterEconomy::GunBuyPrice(Offer.Gun);
	if (!HasRoomForGun(Manager))
	{
		Result.Result = EVendorTrade::NoRoom;
		return Result;
	}
	if (!Wallet.CanAfford(Result.Silver))
	{
		Result.Result = EVendorTrade::NotEnoughSilver;
		return Result;
	}
	// Into the backpack, where the inventory shows it to compare; a free equip slot when the backpack is full.
	const FWeaponInstanceData Gun = Offer.Gun;
	const bool bGiven = Manager.HasBackpackRoom() ? Manager.AddToBackpack(Gun) : Manager.GiveWeapon(Gun) != nullptr;
	if (!bGiven || !Wallet.Spend(Result.Silver))
	{
		Result.Result = EVendorTrade::NoRoom;
		return Result;
	}
	Wallet.MarkOfferSold();
	Result.Count = 1;
	Result.Result = EVendorTrade::Done;
	UE_LOG(LogLooter, Log, TEXT("Vendors: bought the gun of the day (%s %s, level %d) for %d silver."),
		*UEnum::GetDisplayValueAsText(Gun.Rarity).ToString(), *GetNameSafe(Gun.Definition), Gun.Level, Result.Silver);
	return Result;
}

FVendorTradeResult VendorTrade::SellGun(UWalletSubsystem& Wallet, UWeaponManagerComponent& Manager, int32 BackpackIndex)
{
	FVendorTradeResult Result;
	const TArray<FWeaponInstanceData>& Backpack = Manager.GetBackpack();
	if (!Backpack.IsValidIndex(BackpackIndex) || !Backpack[BackpackIndex].Definition)
	{
		return Result;
	}
	if (!LooterEconomy::CanSell(Backpack[BackpackIndex]))
	{
		Result.Result = EVendorTrade::NotForSale;
		return Result;
	}
	FWeaponInstanceData Sold;
	if (!Manager.TakeFromBackpack(BackpackIndex, Sold))
	{
		return Result;
	}
	Result.Silver = Wallet.Add(LooterEconomy::GunSellPrice(Sold), ESilverChange::Sold);
	Result.Count = 1;
	Result.Result = EVendorTrade::Done;
	UE_LOG(LogLooter, Log, TEXT("Vendors: sold a %s %s (level %d) for %d silver."), *UEnum::GetDisplayValueAsText(Sold.Rarity).ToString(),
		*GetNameSafe(Sold.Definition), Sold.Level, Result.Silver);
	return Result;
}

FVendorJunkTally VendorTrade::TallyJunk(const UWeaponManagerComponent& Manager)
{
	FVendorJunkTally Tally;
	for (const FWeaponInstanceData& Gun : Manager.GetBackpack())
	{
		if (Gun.bJunk && LooterEconomy::CanSell(Gun))
		{
			++Tally.Count;
			Tally.Silver += LooterEconomy::GunSellPrice(Gun);
			Tally.bAnyRare |= LooterEconomy::SaleNeedsConfirm(Gun);
		}
	}
	return Tally;
}

FVendorTradeResult VendorTrade::SellAllJunk(UWalletSubsystem& Wallet, UWeaponManagerComponent& Manager)
{
	FVendorTradeResult Result;
	// From the back, so the places of the guns still to sell don't move as each goes.
	for (int32 Index = Manager.GetBackpack().Num() - 1; Index >= 0; --Index)
	{
		const FWeaponInstanceData& Gun = Manager.GetBackpack()[Index];
		if (!Gun.bJunk || !LooterEconomy::CanSell(Gun))
		{
			continue;
		}
		const FVendorTradeResult One = SellGun(Wallet, Manager, Index);
		if (One.IsDone())
		{
			Result.Silver += One.Silver;
			++Result.Count;
		}
	}
	Result.Result = Result.Count > 0 ? EVendorTrade::Done : EVendorTrade::Nothing;
	return Result;
}

bool VendorTrade::ToggleJunk(UWeaponManagerComponent& Manager, int32 BackpackIndex)
{
	const TArray<FWeaponInstanceData>& Backpack = Manager.GetBackpack();
	return Backpack.IsValidIndex(BackpackIndex) && Manager.SetBackpackJunk(BackpackIndex, !Backpack[BackpackIndex].bJunk);
}
