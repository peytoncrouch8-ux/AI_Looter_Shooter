#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/WeaponInventorySave.h"
#include "Progression/PlayerProgressData.h"
#include "Session/CampaignRecord.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponTypes.h"
#include "SessionSave.generated.h"

/** A gun lying in the world. */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FSavedLootWeapon
{
	GENERATED_BODY()

	UPROPERTY()
	FWeaponInstanceData Weapon;

	UPROPERTY()
	FTransform Transform;
};

/** An ammo pickup lying in the world. */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FSavedAmmoPickup
{
	GENERATED_BODY()

	UPROPERTY()
	EAmmoType Type = EAmmoType::AssaultRifle;

	UPROPERTY()
	int32 Amount = 0;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;
};

/** What a weapon rack still offered. */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FSavedWeaponRack
{
	GENERATED_BODY()

	/** The rack actor's name in its level. */
	UPROPERTY()
	FName Rack;

	/** Its weapon was still lying on it. */
	UPROPERTY()
	bool bWeaponOffered = false;

	/** How many of the ammo pickups beside it were left. */
	UPROPERTY()
	int32 AmmoPickupsLeft = 0;
};

/**
 * What a session keeps of one map's world, so each map stays as it was left while the player is somewhere else: loot on
 * the ground, what the gun racks still offered, the wanted posters torn down, the hay bales loaded, the tutorial's step,
 * and when the map's creatures were last promoted and its Legendary monsters last beaten. Creatures themselves aren't
 * kept: they're all back whenever the map is played.
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FSavedMapWorld
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSavedLootWeapon> LootWeapons;

	UPROPERTY()
	TArray<FSavedAmmoPickup> AmmoPickups;

	UPROPERTY()
	TArray<FSavedWeaponRack> Racks;

	/**
	 * The wanted posters torn down (AWantedPoster, Side 1), by the poster actor's name in its level: they stay down, and
	 * Side 1's lasting objective counts them. New within version 2: a save from before it reads as none torn, which is
	 * right, so it needs no upgrade.
	 */
	UPROPERTY()
	TArray<FName> TornPosters;

	/**
	 * Amos's hay bales loaded into the stack by his barn (AHayBale, Side 2), by the bale actor's name in its level: they
	 * stay loaded, and Side 2's lasting objective counts them. New within version 2, as TornPosters: a save from before it
	 * reads as none loaded, which is right.
	 */
	UPROPERTY()
	TArray<FName> LoadedBales;

	/** The tutorial step on screen, or INDEX_NONE when it wasn't running (on every map but the tutorial's). */
	UPROPERTY()
	int32 TutorialStep = INDEX_NONE;

	/**
	 * When the map's creatures were last promoted, in the session's time played (PlayedSeconds); negative: never.
	 * Promotions roll again on arrival once enough play has passed (20 minutes). Kept here; nothing uses it yet.
	 */
	UPROPERTY()
	double PromotionsRolledAt = -1.0;

	/**
	 * When each of the map's Legendary monsters was last beaten, by its id (its lair's LegendaryId: "Gravemother"), in the
	 * session's time played: it's back on an arrival once 20 minutes of play have passed (USessionSubsystem::
	 * IsLegendaryBack, which its lair asks as the level begins; NoteLegendaryDefeat writes it).
	 */
	UPROPERTY()
	TMap<FName, double> LegendaryDefeatedAt;
};

/**
 * One of the three sessions (save slots "Session1" to "Session3", USessionSubsystem): the player as they left it (the
 * level, where they stood, health, level and experience, bestiary, guns and ammo), each map's world as it was left (loot
 * on the ground, what the gun racks still offered, the tutorial's step), and the story so far. A new game is a save
 * with nothing captured yet.
 *
 * A trip between maps (USessionSubsystem::TravelToMap) points the save at its destination with no player spot there
 * and the landing to arrive at, and saves it before the destination opens. Each map keeps its own world, so a trip
 * there and back leaves both as they were.
 *
 * Fields are found by name when a save is read, so a field is never renamed or retyped: an older save's would be lost.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULooterSessionSave : public USaveGame
{
	GENERATED_BODY()

public:
	/**
	 * 1: the first sessions, with one world (the level the session was in).
	 * 2: a world per map (Worlds), the campaign record (Campaign) and where a trip arrives (ArrivalTag). A version 1
	 *    save is brought up to date as it's read (Upgrade), before anything can save it again.
	 */
	static constexpr int32 CurrentVersion = 2;

	/** Every version 1 session was played on the tutorial island: a version 1 world with no map is filed under it. */
	static constexpr const TCHAR* Version1Map = TEXT("/Game/Maps/Lvl_TutorialIsland");

	/** New saves get CurrentVersion, for upgrading older ones as the save grows. */
	UPROPERTY()
	int32 Version = 0;

	/** When the session was started (its new game) and when it was last written, in local time. */
	UPROPERTY()
	FDateTime Created;

	UPROPERTY()
	FDateTime Saved;

	/** Seconds played in all (the game paused doesn't count). */
	UPROPERTY()
	double PlayedSeconds = 0.0;

	/**
	 * The level the player is in (its package name); the session continues there. Empty: the game's first level. A trip
	 * sets it to its destination before the destination opens.
	 */
	UPROPERTY()
	FString Map;

	// --- The player ---

	UPROPERTY()
	FPlayerProgressData Progress;

	/**
	 * Where the player stood and looked, in Map. Without it (a new game, saved while dead, or just after a trip) they
	 * start at the trip's landing, or else at the level's start.
	 */
	UPROPERTY()
	bool bHasPlayerSpot = false;

	UPROPERTY()
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator PlayerView = FRotator::ZeroRotator;

	/** Health left; 0 or less is full health (a new game, or saved while dead). */
	UPROPERTY()
	float Health = 0.f;

	/** The guns and ammo below were captured; without them the player starts with what the character is given. */
	UPROPERTY()
	bool bHasInventory = false;

	UPROPERTY()
	FWeaponInventorySave Inventory;

	/**
	 * The landing (AreaLandings) the player arrives at when the session next starts in Map without a spot there: a trip
	 * writes it, and it's cleared once they have arrived. None: the level's start.
	 */
	UPROPERTY()
	FName ArrivalTag;

	// --- The worlds ---

	/** Each map's world as it was left, by the map's package name. A map with none starts as it was built. */
	UPROPERTY()
	TMap<FString, FSavedMapWorld> Worlds;

	// --- The story ---

	UPROPERTY()
	FCampaignRecord Campaign;

	// --- Version 1's one world: only read to upgrade a version 1 save (Upgrade files it under its map), empty after ---

	UPROPERTY()
	bool bHasWorld = false;

	UPROPERTY()
	TArray<FSavedLootWeapon> LootWeapons;

	UPROPERTY()
	TArray<FSavedAmmoPickup> AmmoPickups;

	UPROPERTY()
	TArray<FSavedWeaponRack> Racks;

	UPROPERTY()
	int32 TutorialStep = INDEX_NONE;

	// --- Bookkeeping (SessionSave.cpp) ---

	/**
	 * Brings a save just read up to CurrentVersion: version 1's world is filed under the map it was saved in (the
	 * tutorial island when it has none). Guns, ammo, health, progress and the player's spot don't move. A save written by
	 * a newer game is left as it is. Returns the version it was read as.
	 */
	int32 Upgrade();

	/** The world kept for this map, or null when there's none (the map starts as it was built). */
	const FSavedMapWorld* FindWorld(const FString& MapPackage) const;

	/** The world kept for this map, made empty when there was none. */
	FSavedMapWorld& FindOrAddWorld(const FString& MapPackage);

	/**
	 * The saved spot is on this map. A spot is only good in the level it was saved in (Map): a session that can't
	 * continue there (the level renamed or not in the game) starts at the level's start instead.
	 */
	bool HasPlayerSpotOn(const FString& MapPackage) const;

	/** Where the player arrives when the session starts on this map: the trip's landing, unless they have a spot here. */
	FName GetArrivalOn(const FString& MapPackage) const;

	/**
	 * A trip's bookkeeping, with no level opened: the session continues in Destination, with no spot there yet,
	 * arriving at Landing (None: the level's start). Every map's world and the player's guns, health and progress stay.
	 */
	void PrepareTrip(const FString& Destination, FName Landing);

	/** Guns carried: the equipped ones and the backpack's. */
	int32 CountGuns() const;
};
