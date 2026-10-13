#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "VendorOffer.generated.h"

struct FRandomStream;

/**
 * The vendors' gun of the day (Docs/Progression.md, "Economy"): one gun rolled at the player's level, Uncommon or better,
 * never cursed, the same at every vendor. A level-up or a day's trade (an hour of play, LooterEconomy::OfferDaySeconds)
 * brings the next; bought, it's sold out until then. The session saves it (ULooterSessionSave::VendorOffer), so loading
 * a session never rolls a new one. New in version 2: a save from before it reads as none rolled, which is right.
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FVendorOffer
{
	GENERATED_BODY()

	/** A gun has been rolled (none until a vendor is first opened). */
	UPROPERTY()
	bool bRolled = false;

	UPROPERTY()
	FWeaponInstanceData Gun;

	/** Bought: sold out until the next one. */
	UPROPERTY()
	bool bSold = false;

	/** The player's level it was rolled for: a higher level brings a new one. */
	UPROPERTY()
	int32 RolledAtLevel = 0;

	/** When it was rolled, in the time the wallet keeps (UWalletSubsystem::GetClock: the session's time played). */
	UPROPERTY()
	double RolledAt = 0.0;

	/** A new one is due for a player at PlayerLevel at Now: none rolled, or none of its kind, a level gained, a day gone by. */
	bool IsDue(int32 PlayerLevel, double Now) const;

	/** Seconds of play until a new one comes by time alone (0 when due; a clock that went back, a new level's, counts as none gone). */
	double SecondsToNext(double Now) const;

	/**
	 * Rolls a new one at PlayerLevel, Now: its kind by the default loot table's weights (ULootLibrary::PickWeaponWith), its
	 * rarity by LooterEconomy::RollOfferRarity, not sold. False, leaving it as it was, without a kind to roll.
	 */
	bool Roll(int32 PlayerLevel, double Now, FRandomStream& Random);
};
