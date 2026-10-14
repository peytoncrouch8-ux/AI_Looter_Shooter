#pragma once

#include "CoreMinimal.h"
#include "Loot/LostLootStore.h"

class UWalletSubsystem;
class UWeaponManagerComponent;

/**
 * Lost Property's rules (the user's call, 2026-10-13), as plain numbers and functions the tests read:
 *  - a gun lying on the ground goes to Lost Property after five minutes of play (GroundLifeSeconds), never from under the
 *    player's feet (KeepNearRadius) for more than a minute (NearGraceSeconds); leaving the area or quitting the game hands
 *    in every gun on the ground at once (ULostLootSubsystem; Session/SessionSubsystemTravel.cpp, SessionSubsystemWorld.cpp);
 *  - the shelves hold 30 to start and 10 more for each of five upgrades (80 at most), bought with silver at the window
 *    at rising prices that grow with the player's level as the vendors' do (LooterEconomy::AtLevel);
 *  - a gun is claimed into the backpack, else a free equip slot, else not at all ("no room").
 */
namespace LostLootRules
{
	/** A gun lying on the ground goes to Lost Property after this much play (s). */
	inline constexpr float GroundLifeSeconds = 300.f;

	/** A gun whose time is up isn't taken while the player is this close to it (cm): they may be walking to it... */
	inline constexpr float KeepNearRadius = 600.f;

	/** ...for at most this much longer (s), so standing among the guns doesn't keep them lying for good. */
	inline constexpr float NearGraceSeconds = 60.f;

	/** How often the ground's clock looks at the guns lying around (s). */
	inline constexpr float ClockSeconds = 1.f;

	inline constexpr int32 BaseCapacity = 30;
	inline constexpr int32 CapacityPerUpgrade = 10;
	inline constexpr int32 MaxUpgrades = 5;

	/** How many guns the shelves hold with Upgrades bought: 30, 40, ... 80. */
	AI_LOOTER_SHOOTER_API int32 Capacity(int32 Upgrades);

	/** The next upgrade's price at level 1 after Upgrades bought (300, 600, 1,000, 1,600, 2,400); 0 when all five are bought. */
	AI_LOOTER_SHOOTER_API int32 UpgradeBasePrice(int32 Upgrades);

	/** The next upgrade's price for a player at PlayerLevel: the base grown as the vendors' prices grow; 0 when none is left. */
	AI_LOOTER_SHOOTER_API int32 UpgradePrice(int32 Upgrades, int32 PlayerLevel);

	/** The next upgrade's name, as the station agent puts it ("A second shelf", "Racks along the wall"...); empty when none is left. */
	AI_LOOTER_SHOOTER_API FText UpgradeName(int32 Upgrades);

	/**
	 * Whether a gun lying on the ground with LifeLeft seconds of its five minutes (negative: overdue) goes now: once its time
	 * is up, unless the player is near it (bPlayerNear), and then once its grace is up too.
	 */
	AI_LOOTER_SHOOTER_API bool IsDue(float LifeLeft, bool bPlayerNear);

	/** Where and how a ticket's gun was found, for people: "Left lying in Ransom's Rest", "Left behind on leaving Skyreach". */
	AI_LOOTER_SHOOTER_API FText ReasonWords(ELostLootReason Reason, const FString& Map);

	/** The HUD's line for guns handed in by a trip or a load: "3 guns sent to Lost Property at the depot" (and the oldest gone). */
	AI_LOOTER_SHOOTER_API FText SentLine(int32 Count, int32 Gone);

	/** The HUD's line the first time a gun left lying is taken: "Left lying, the Rare Bullpup went to Lost Property at the depot". */
	AI_LOOTER_SHOOTER_API FText ExplainLine(const FWeaponInstanceData& Gun);

	// --- At the window ---

	enum class EUpgrade : uint8
	{
		/** Bought: the silver paid, the shelves bigger. */
		Done,
		NotEnoughSilver,
		/** All five are bought. */
		AtMost
	};

	struct FUpgradeResult
	{
		EUpgrade Result = EUpgrade::AtMost;
		/** The price asked (paid when Done). */
		int32 Silver = 0;
		/** The shelves' capacity after it. */
		int32 Capacity = 0;
	};

	/** Buys the next upgrade through the wallet, all or nothing. */
	AI_LOOTER_SHOOTER_API FUpgradeResult BuyUpgrade(UWalletSubsystem& Wallet, FLostLootStore& Store, int32 PlayerLevel);

	enum class ETake : uint8
	{
		/** Claimed into the backpack. */
		InBackpack,
		/** The backpack was full: claimed into a free equip slot. */
		InSlot,
		/** Nowhere to put it: the backpack and every equip slot are full. Nothing changed. */
		NoRoom,
		/** No gun there (or one whose kind is gone from the game). */
		Nothing
	};

	/** Somewhere to put a claimed gun: room in the backpack, or a free equip slot. */
	AI_LOOTER_SHOOTER_API bool HasRoom(const UWeaponManagerComponent& Manager);

	/** Where a claimed gun would go now, without claiming it (InBackpack, InSlot or NoRoom). */
	AI_LOOTER_SHOOTER_API ETake WhereItGoes(const UWeaponManagerComponent& Manager);

	/** Claims the shelves' gun at Index into Manager (WhereItGoes); off the shelves only once it's carried. OutGun: the gun. */
	AI_LOOTER_SHOOTER_API ETake Take(FLostLootStore& Store, int32 Index, UWeaponManagerComponent& Manager, FWeaponInstanceData* OutGun = nullptr);
}
