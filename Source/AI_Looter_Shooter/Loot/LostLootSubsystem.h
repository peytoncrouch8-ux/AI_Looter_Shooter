#pragma once

#include "CoreMinimal.h"
#include "Loot/LostLootStore.h"
#include "Subsystems/WorldSubsystem.h"
#include "LostLootSubsystem.generated.h"

class AWeaponBase;
class AWeaponRack;
class UNamedWeaponDefinition;

/** The shelves changed (a gun handed in or claimed, shelving bought, put back from a save): an open screen reads them again. */
DECLARE_MULTICAST_DELEGATE(FOnLostLootChanged);

/**
 * Lost Property in the level being played (the user's call, 2026-10-13): guns lying on the ground don't pile up any more.
 *
 * The shelves (FLostLootStore) live here while a level plays, as the purse lives in the wallet: the session puts them
 * back as the level starts and keeps them as it ends or a trip leaves (USessionSubsystem::RestoreWorld, CaptureWorld;
 * ULooterSessionSave::LostLoot), so they're the same at every window in every area. Without a session they start empty,
 * kept for that play only.
 *
 * Every gun lying loose on the ground (any rarity, the player's own drops, named guns and mission rewards too; never one
 * on a weapon rack, which is the rack's) has five minutes of play (LostLootRules::GroundLifeSeconds), counted by the ground's
 * clock (LostLootGround.cpp, once a second in a played level), and then goes to the shelves. A gun nobody can see yet (a
 * scene keeps it hidden) doesn't age; one under the player's feet waits a little (KeepNearRadius, NearGraceSeconds).
 * Leaving the area hands in every gun on the ground at once (SendAllGroundWeapons, from TravelToMap), and so does a load:
 * the guns a save kept lying are never put back on the ground, they're handed in (every load follows a quit: Save & Quit,
 * closing the game, a stopped play session). Each goes off the ground as it goes on the shelves, so it's never in both,
 * and never in the save twice.
 *
 * Guns handed in by a trip or a load are told of once the player can see the HUD ("3 guns sent to Lost Property at the
 * depot", on the weapon manager's message line), and the first gun ever left lying is explained the same way; after that
 * the ground's guns go quietly.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULostLootSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Lost Property in WorldContextObject's world, or null (no world, a world kind without one). */
	static ULostLootSubsystem* Get(const UObject* WorldContextObject);

	/** Played levels, play in the editor and the tests' levels. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	/** A played level (the game, play in the editor): the ground's clock starts. Tests' levels run it by hand (Advance). */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// --- The shelves ---

	const FLostLootStore& GetStore() const { return Store; }

	/** The shelves to change in place (the window's screen claims and buys through LostLootRules); call NotifyChanged after. */
	FLostLootStore& GetMutableStore() { return Store; }

	/** Puts back the shelves a session kept (guns whose kind is gone from the game are left out). */
	void SetStore(const FLostLootStore& InStore);

	/** Tells whoever shows the shelves that they changed. */
	void NotifyChanged();

	int32 GetCapacity() const { return Store.GetCapacity(); }

	/** A named gun (Heirloom) is on the shelves: one of a kind, so nothing makes another while it waits there. */
	bool HoldsNamed(const UNamedWeaponDefinition* Named) const;

	/** Takes a named gun off the shelves (a mission's turn-in brings the very same gun to the player). False when it isn't there. */
	bool TakeNamed(const UNamedWeaponDefinition* Named, FWeaponInstanceData& OutGun);

	/**
	 * Puts guns on the shelves, the longest lain first (FLostLootStore::SortOldestFirst), found in Map (its package name)
	 * for Reason. With bAnnounce the HUD tells of them once the player can see it. How many went on.
	 */
	int32 Receive(TArray<FLostLootArrival> Arrivals, ELostLootReason Reason, const FString& Map, bool bAnnounce);

	/** Empties the shelves (the console). How many went. */
	int32 ClearStore();

	/** Now, as the shelves' tickets count it: the session's time played, else this level's play (s). */
	double GetClock() const;

	FOnLostLootChanged OnChanged;

	// --- Guns on the ground (LostLootGround.cpp) ---

	/** A gun lying loose in the level: loot (no holder), not on its way out, not lying on a weapon rack. */
	static bool IsGroundWeapon(const AWeaponBase& Weapon, const TArray<AWeaponRack*>& Racks);

	/** Every gun lying loose in the level now. */
	TArray<AWeaponBase*> FindGroundWeapons() const;

	/** Seconds of its five minutes Weapon has left (negative: overdue, waiting by the player); a whole life when not yet counted. */
	float GetLifeLeft(const AWeaponBase& Weapon) const;

	/** Sets what's left of Weapon's five minutes (the console, the tests). */
	void SetLifeLeft(AWeaponBase& Weapon, float Seconds);

	/** The ground's clock: DeltaSeconds more for every gun lying in sight, and the ones whose time is up handed in. */
	void Advance(float DeltaSeconds);

	/** Advance as if the player stood at PlayerAt (unset: nowhere, as in the tests' levels without one). */
	void AdvanceAt(float DeltaSeconds, const TOptional<FVector>& PlayerAt);

	/** Hands Weapon in now, off the ground and onto the shelves. False when it isn't a gun lying loose. */
	bool Send(AWeaponBase& Weapon, ELostLootReason Reason);

	/**
	 * Hands in every gun lying loose in the level at once, the longest lain first (leaving the area, the console). Told of
	 * on the HUD after the trip (bAnnounce), as the next level begins. How many went.
	 */
	int32 SendAllGroundWeapons(ELostLootReason Reason, bool bAnnounce = true);

	/** How many guns handed in are waiting to be told of on the HUD. */
	int32 GetUnannounced() const { return Store.Unannounced; }

	/** The first gun left lying is waiting to be explained on the HUD. */
	bool IsExplainWaiting() const { return bExplainWaiting; }

	/** Says what's waiting on the HUD when the player can see it (no scene, no menu, the level a moment old). Whether it said. */
	bool TryAnnounce();

private:
	/** The clock's beat in a played level. */
	void HandleClock();

	/** The player's pawn's place in a played level, or unset. */
	TOptional<FVector> FindPlayerPlace() const;

	/** Puts a line on the player's HUD with Lost Property's sound. False when there's no player to tell. */
	bool SayToPlayer(const FText& Line) const;

	/** The shelves: the guns' kinds are assets held here for the garbage collector. */
	UPROPERTY(Transient)
	FLostLootStore Store;

	/** The guns lying loose that the clock has seen, and the seconds each has left. */
	TMap<TWeakObjectPtr<AWeaponBase>, float> GroundLife;

	FTimerHandle ClockTimer;

	/** The level's time at the clock's last beat, and when play began (s). */
	double LastClock = 0.0;
	double BeganAt = 0.0;

	/** When the HUD last said something of Lost Property's (the level's time), so two lines don't land on each other. */
	double LastSaidAt = -1000.0;

	/** The first gun ever left lying went in this play and waits to be explained (ExplainGun). */
	bool bExplainWaiting = false;

	UPROPERTY(Transient)
	FWeaponInstanceData ExplainGun;
};
