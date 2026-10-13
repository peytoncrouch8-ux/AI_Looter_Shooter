#include "Economy/VendorOffer.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Economy/EconomyRules.h"
#include "Loot/LootLibrary.h"
#include "Weapons/WeaponDefinition.h"
#include "Math/RandomStream.h"

bool FVendorOffer::IsDue(int32 PlayerLevel, double Now) const
{
	if (!bRolled || !Gun.Definition)
	{
		return true;
	}
	if (PlayerLevel > RolledAtLevel)
	{
		return true;
	}
	return SecondsToNext(Now) <= 0.0;
}

double FVendorOffer::SecondsToNext(double Now) const
{
	if (!bRolled)
	{
		return 0.0;
	}
	// A clock that reads earlier than the roll is a new level's (a trip played without a session): none of the day gone by.
	const double Gone = FMath::Max(0.0, Now - RolledAt);
	return FMath::Max(0.0, LooterEconomy::OfferDaySeconds - Gone);
}

bool FVendorOffer::Roll(int32 PlayerLevel, double Now, FRandomStream& Random)
{
	UWeaponDefinition* Kind = ULootLibrary::PickWeaponWith(ULootLibrary::GetDefaultLootTable(), Random);
	if (!Kind)
	{
		UE_LOG(LogLooter, Warning, TEXT("Vendors: no gun of the day (the default loot table, DA_LootTable_Default, lists no guns)."));
		return false;
	}
	const EWeaponRarity Rarity = LooterEconomy::RollOfferRarity(Random);
	const int32 Level = FMath::Max(PlayerLevel, 1);
	Gun = UWeaponRollLibrary::RollWeaponWithRarity(Kind, Rarity, Level);
	bRolled = true;
	bSold = false;
	RolledAtLevel = Level;
	RolledAt = Now;
	UE_LOG(LogLooter, Log, TEXT("Vendors: the gun of the day is a %s %s (level %d), %d silver."),
		*UEnum::GetDisplayValueAsText(Rarity).ToString(), *Kind->GetName(), Level, LooterEconomy::GunBuyPrice(Gun));
	return true;
}
