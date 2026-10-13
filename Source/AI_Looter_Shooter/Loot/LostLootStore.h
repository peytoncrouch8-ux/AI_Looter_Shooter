#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "LostLootStore.generated.h"

class UNamedWeaponDefinition;

/**
 * Why a gun went to Lost Property, for its ticket's "found" line. Saved by value in every session that has a gun on the
 * shelves: add new reasons at the end only, never reorder them.
 */
UENUM()
enum class ELostLootReason : uint8
{
	/** Left lying on the ground until its five minutes ran out (LostLootRules::GroundLifeSeconds). */
	LeftLying,
	/** Left on the ground when the player travelled to another area. */
	LeftArea,
	/** Left on the ground when the game was quit (Save & Quit, closing the game), handed in as the session was loaded. */
	LeftAtQuit,
	/** Lying on the ground in a save from before Lost Property, handed in as it was loaded. */
	OldSave,
	/** Sent by the console (Looter.LostLoot.Send). */
	Console
};

/** One gun on Lost Property's shelves: the gun as it was left (its magazine too), its claim ticket, why and where it was found. */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FLostLootEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FWeaponInstanceData Weapon;

	/** Its claim ticket's number, counting up through the session and never reused: the stub's "No. 0042". */
	UPROPERTY()
	int32 Ticket = 0;

	UPROPERTY()
	ELostLootReason Reason = ELostLootReason::LeftLying;

	/** The level it was found in, by package name as sessions name maps: the ticket says where ("Ransom's Rest"). */
	UPROPERTY()
	FString Map;

	/** When it was handed in, in the session's time played (seconds); negative: not known. */
	UPROPERTY()
	double HandedInAt = -1.0;
};

/**
 * A gun on its way to the shelves: the gun, and the seconds its ground life had left (less is older: it lay longer).
 * Guns handed in together (a trip, a load) go on the shelves oldest first, so when they're more than the shelves hold,
 * the newest are the ones kept.
 */
struct FLostLootArrival
{
	FWeaponInstanceData Weapon;
	float LifeLeft = 0.f;
};

/**
 * Lost Property's shelves (the user's call, 2026-10-13: guns left on the ground pile up and cost frame time, so they go
 * here instead): the guns handed in, oldest first, the shelving bought, the ticket counter, and what the HUD hasn't told
 * of yet. The session keeps it (ULooterSessionSave::LostLoot); while a level plays it lives in ULostLootSubsystem.
 *
 * It holds LostLootRules::Capacity of them (30, and 10 more for each of the five upgrades bought at the window). When
 * a gun comes in and the shelves are full, the oldest goes out to make room: the oldest that isn't a named gun (Heirloom
 * is Pa's, one of a kind, and never given away), or the oldest of all when every one is named. Plain data with plain
 * rules, so the tests drive it without a level.
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FLostLootStore
{
	GENERATED_BODY()

	/** The guns on the shelves, oldest first (by ticket). */
	UPROPERTY()
	TArray<FLostLootEntry> Entries;

	/** Shelving bought at the window: each adds LostLootRules::CapacityPerUpgrade spaces, up to MaxUpgrades. */
	UPROPERTY()
	int32 Upgrades = 0;

	/** The next claim ticket's number. */
	UPROPERTY()
	int32 NextTicket = 1;

	/**
	 * Guns handed in by a trip or a load that the HUD hasn't told of yet ("3 guns sent to Lost Property"), and how many
	 * of the oldest went out to make room meanwhile. Kept with the session, so a trip's guns are told of on arrival.
	 */
	UPROPERTY()
	int32 Unannounced = 0;

	UPROPERTY()
	int32 UnannouncedGone = 0;

	/** Every gun that has gone out to make room, in the session. */
	UPROPERTY()
	int32 GivenAway = 0;

	/** The first gun left lying has been explained on the HUD (it went to Lost Property at the depot); once a session. */
	UPROPERTY()
	bool bExplained = false;

	/** How many guns the shelves hold now (LostLootRules::Capacity of the upgrades bought). */
	int32 GetCapacity() const;

	int32 Num() const { return Entries.Num(); }
	bool IsFull() const { return Entries.Num() >= GetCapacity(); }

	/** The entry that goes out when one more comes in while full: the oldest unnamed one, else the oldest. INDEX_NONE: empty. */
	int32 FindNextToGo() const;

	/**
	 * Puts Gun on the shelves as the newest, with the next ticket; when they're full, the next to go makes room first
	 * (OutGone gets it). False, nothing changed, for a gun with no kind (its data asset gone).
	 */
	bool Add(const FWeaponInstanceData& Gun, ELostLootReason Reason, const FString& Map, double HandedInAt, FLostLootEntry* OutGone = nullptr,
		bool* bOutGone = nullptr);

	/** Takes the entry at Index off the shelves (claimed); OutGun gets its gun. False when there's none there. */
	bool RemoveAt(int32 Index, FWeaponInstanceData* OutGun = nullptr);

	/** Where the gun with this ticket is, or INDEX_NONE. */
	int32 FindTicket(int32 Ticket) const;

	/** Where a named gun is on the shelves (Heirloom), or INDEX_NONE. */
	int32 FindNamed(const UNamedWeaponDefinition* Named) const;

	/** Empties the shelves (the console); the shelving and the ticket counter stay. How many guns went. */
	int32 Clear();

	/** Puts arrivals in the order they go on the shelves: the longest lain first (least life left), ties as they came. */
	static void SortOldestFirst(TArray<FLostLootArrival>& Arrivals);

	/** A ticket's number as its stub prints it: "0042" (more digits past 9999). */
	static FString TicketText(int32 Ticket);
};
