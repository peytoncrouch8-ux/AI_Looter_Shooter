// LostLootRules: Lost Property's numbers (the ground's five minutes, the shelves and their upgrades' prices), its words,
// and the two things done at the window: buying shelving and claiming a gun.

#include "Loot/LostLootRules.h"
#include "AI_Looter_Shooter.h"
#include "Economy/EconomyRules.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Session/SessionSubsystem.h"
#include "UI/Style/WeaponText.h"

#define LOCTEXT_NAMESPACE "LooterLostLoot"

namespace
{
	/**
	 * Each upgrade's price at level 1, in the order they're bought. A playthrough of Ransom's Rest earns about 1,270
	 * (Looter.Economy.Balance), so the first is within its reach beside the supplies; the rest are goals for the areas after
	 * it, which pay more, as the shelves fill with a campaign's guns (the user's call, 2026-10-14: dearer, since the world
	 * grows and earns more; twice the first prices).
	 */
	constexpr int32 UpgradeBases[] = { 300, 600, 1000, 1600, 2400 };
	static_assert(UE_ARRAY_COUNT(UpgradeBases) == LostLootRules::MaxUpgrades, "One price per upgrade");
}

int32 LostLootRules::Capacity(int32 Upgrades)
{
	return BaseCapacity + CapacityPerUpgrade * FMath::Clamp(Upgrades, 0, MaxUpgrades);
}

int32 LostLootRules::UpgradeBasePrice(int32 Upgrades)
{
	return Upgrades >= 0 && Upgrades < MaxUpgrades ? UpgradeBases[Upgrades] : 0;
}

int32 LostLootRules::UpgradePrice(int32 Upgrades, int32 PlayerLevel)
{
	return LooterEconomy::AtLevel(UpgradeBasePrice(Upgrades), PlayerLevel);
}

FText LostLootRules::UpgradeName(int32 Upgrades)
{
	// How the station agent makes room, one step at a time: the baggage room's shelves, then the depot's other corners.
	switch (Upgrades)
	{
	case 0: return LOCTEXT("Upgrade1", "A second shelf");
	case 1: return LOCTEXT("Upgrade2", "Racks along the wall");
	case 2: return LOCTEXT("Upgrade3", "The back room cleared");
	case 3: return LOCTEXT("Upgrade4", "The loft over the baggage room");
	case 4: return LOCTEXT("Upgrade5", "The old freight shed");
	default: return FText::GetEmpty();
	}
}

bool LostLootRules::IsDue(float LifeLeft, bool bPlayerNear)
{
	if (LifeLeft > 0.f)
	{
		return false;
	}
	return !bPlayerNear || LifeLeft <= -NearGraceSeconds;
}

FText LostLootRules::ReasonWords(ELostLootReason Reason, const FString& Map)
{
	const FText Place = FText::FromString(Map.IsEmpty() ? FString() : USessionSubsystem::AreaName(Map));
	const bool bPlace = !Map.IsEmpty();
	switch (Reason)
	{
	case ELostLootReason::LeftLying:
		return bPlace ? FText::Format(LOCTEXT("LeftLyingIn", "Left lying in {0}"), Place) : LOCTEXT("LeftLying", "Left lying on the ground");
	case ELostLootReason::LeftArea:
		return bPlace ? FText::Format(LOCTEXT("LeftAreaIn", "Left behind on leaving {0}"), Place) : LOCTEXT("LeftArea", "Left behind on leaving");
	case ELostLootReason::LeftAtQuit:
		return bPlace ? FText::Format(LOCTEXT("LeftAtQuitIn", "Left on the ground in {0} at day's end"), Place)
			: LOCTEXT("LeftAtQuit", "Left on the ground at day's end");
	case ELostLootReason::OldSave:
		return bPlace ? FText::Format(LOCTEXT("OldSaveIn", "Found lying in {0}"), Place) : LOCTEXT("OldSave", "Found lying about");
	case ELostLootReason::Console:
		return LOCTEXT("Console", "Brought in by the railway");
	}
	return FText::GetEmpty();
}

FText LostLootRules::SentLine(int32 Count, int32 Gone)
{
	if (Gone > 0)
	{
		return FText::Format(LOCTEXT("SentGone", "{0} {0}|plural(one=gun,other=guns) sent to Lost Property at the depot · the {1} oldest went to make room"),
			FText::AsNumber(Count), FText::AsNumber(Gone));
	}
	return FText::Format(LOCTEXT("Sent", "{0} {0}|plural(one=gun,other=guns) sent to Lost Property at the depot"), FText::AsNumber(Count));
}

FText LostLootRules::ExplainLine(const FWeaponInstanceData& Gun)
{
	return FText::Format(LOCTEXT("Explain", "Left lying, your {0} went to Lost Property at the depot"),
		FText::FromString(LooterWeaponText::Name(Gun)));
}

// ---------------------------------------------------------------------------
// At the window
// ---------------------------------------------------------------------------

LostLootRules::FUpgradeResult LostLootRules::BuyUpgrade(UWalletSubsystem& Wallet, FLostLootStore& Store, int32 PlayerLevel)
{
	FUpgradeResult Result;
	Result.Capacity = Store.GetCapacity();
	if (Store.Upgrades >= MaxUpgrades)
	{
		Result.Result = EUpgrade::AtMost;
		return Result;
	}
	Result.Silver = UpgradePrice(Store.Upgrades, PlayerLevel);
	if (!Wallet.Spend(Result.Silver))
	{
		Result.Result = EUpgrade::NotEnoughSilver;
		return Result;
	}
	++Store.Upgrades;
	Result.Capacity = Store.GetCapacity();
	Result.Result = EUpgrade::Done;
	UE_LOG(LogLooter, Log, TEXT("Lost Property: shelving bought for %d silver, room for %d guns now."), Result.Silver, Result.Capacity);
	return Result;
}

bool LostLootRules::HasRoom(const UWeaponManagerComponent& Manager)
{
	return WhereItGoes(Manager) != ETake::NoRoom;
}

LostLootRules::ETake LostLootRules::WhereItGoes(const UWeaponManagerComponent& Manager)
{
	// Into the backpack, where the inventory shows it to compare (as a vendor's gun of the day goes); a free equip slot when
	// the backpack is full (its room shared with the spare wards). A shut slot never takes one.
	if (Manager.HasBackpackRoom())
	{
		return ETake::InBackpack;
	}
	return Manager.HasFreeSlot() ? ETake::InSlot : ETake::NoRoom;
}

LostLootRules::ETake LostLootRules::Take(FLostLootStore& Store, int32 Index, UWeaponManagerComponent& Manager, FWeaponInstanceData* OutGun)
{
	if (!Store.Entries.IsValidIndex(Index) || !Store.Entries[Index].Weapon.Definition)
	{
		return ETake::Nothing;
	}
	const ETake Where = WhereItGoes(Manager);
	if (Where == ETake::NoRoom)
	{
		return ETake::NoRoom;
	}
	const FWeaponInstanceData Gun = Store.Entries[Index].Weapon;
	const bool bCarried = Where == ETake::InBackpack ? Manager.AddToBackpack(Gun) : Manager.GiveWeapon(Gun) != nullptr;
	if (!bCarried)
	{
		return ETake::NoRoom;
	}
	// Off the shelves only now that it's carried: one gun, never two, never none.
	Store.RemoveAt(Index);
	if (OutGun)
	{
		*OutGun = Gun;
	}
	UE_LOG(LogLooter, Log, TEXT("Lost Property: claimed %s (level %d) into the %s; %d left on the shelves."), *LooterWeaponText::Name(Gun),
		Gun.Level, Where == ETake::InBackpack ? TEXT("backpack") : TEXT("equip slots"), Store.Num());
	return Where;
}

#undef LOCTEXT_NAMESPACE
