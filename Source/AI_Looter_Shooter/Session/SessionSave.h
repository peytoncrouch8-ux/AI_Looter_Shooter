#pragma once

#include "CoreMinimal.h"
#include "Economy/VendorOffer.h"
#include "Embers/EmberTypes.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/WeaponInventorySave.h"
#include "Loot/LostLootStore.h"
#include "Player/PlayerThrowSave.h"
#include "Progression/PlayerProgressData.h"
#include "Session/BossRecallRecord.h"
#include "Session/CampaignRecord.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponTypes.h"
#include "SessionSave.generated.h"

/**
 * A gun lying in the world when the session was saved. Since Lost Property (2026-10-13) a load never puts it back on the
 * ground: it's handed in to the shelves (ULostLootSubsystem, from USessionSubsystem::RestoreWorld), the longest lain first.
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FSavedLootWeapon
{
	GENERATED_BODY()

	UPROPERTY()
	FWeaponInstanceData Weapon;

	UPROPERTY()
	FTransform Transform;

	/**
	 * Seconds of its five minutes on the ground it had left (LostLootRules::GroundLifeSeconds; negative: overdue, waiting by
	 * the player), as ammo keeps its life: the guns handed in at the next load go on the shelves oldest first by it. 0: not
	 * kept (a save from before Lost Property), counted as the oldest.
	 */
	UPROPERTY()
	float LifeLeft = 0.f;
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

	/**
	 * Seconds it had left before it would have gone (AActor::GetLifeSpan), so a Save & Quit doesn't give it a whole new
	 * life: loaded every few minutes, ammo never ran out and piled up round the fights. 0: not kept (an older save), a
	 * whole life.
	 */
	UPROPERTY()
	float LifeLeft = 0.f;
};

/** Silver lying in the world (ASilverPickup). */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FSavedSilverPickup
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Amount = 0;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	/** Seconds it had left before it would have gone, as an ammo pickup keeps them (0: a whole life). */
	UPROPERTY()
	float LifeLeft = 0.f;
};

/** A grave-salt grenade tin lying in the world (AGrenadePickup). */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FSavedGrenadePickup
{
	GENERATED_BODY()

	/** How many grenades the tin holds. */
	UPROPERTY()
	int32 Amount = 0;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	/**
	 * Seconds it had left before it would have gone, as an ammo pickup keeps them: put back, the tin goes on with these, and
	 * vanishes when they run out (0: a whole life).
	 */
	UPROPERTY()
	float LifeLeft = 0.f;
};

/** A ward (the player's shield as loot) lying on the ground. Put back where it lay on load, unlike a gun (wards have no Lost Property yet). */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FSavedShieldPickup
{
	GENERATED_BODY()

	UPROPERTY()
	FShieldInstanceData Ward;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	/** Seconds it had left on the ground. */
	UPROPERTY()
	float LifeLeft = 0.f;
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
 * the ground (guns, ammo, silver, grenade tins), what the gun racks still offered, the wanted posters torn down, the hay
 * bales loaded, the chests opened and the ones found, the tutorial's step, and when the map's creatures were last promoted and its Legendary monsters last beaten.
 * Creatures themselves aren't kept: they're all back whenever the map is played.
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FSavedMapWorld
{
	GENERATED_BODY()

	/**
	 * The guns lying loose here when the session was saved (never a rack's). Handed in to Lost Property as the session is
	 * loaded, never put back on the ground; a trip leaves none (TravelToMap hands them in as it goes).
	 */
	UPROPERTY()
	TArray<FSavedLootWeapon> LootWeapons;

	UPROPERTY()
	TArray<FSavedAmmoPickup> AmmoPickups;

	/** Wards lying here (2026-10-22): they go back on the ground with the life they had left. */
	UPROPERTY()
	TArray<FSavedShieldPickup> ShieldPickups;

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

	/**
	 * The loot chests opened (AChest: the Ranger caches, the gang's Strongbox), by the chest's id (AChest::GetSaveKey):
	 * they stay open and empty, and give nothing again. Only chests whose loot is out count, so a save mid-swing keeps the
	 * chest closed and full. New within version 2, as TornPosters: a save from before it reads as none opened.
	 */
	UPROPERTY()
	TArray<FName> OpenedChests;

	/**
	 * The crates and barrels broken (ABreakableProp), by the prop's id (ABreakableProp::GetSaveKey): they stay broken, as
	 * stumps, and give nothing again. New within version 2, as TornPosters: a save from before it reads as none broken.
	 */
	UPROPERTY()
	TArray<FName> BrokenProps;

	/**
	 * Silver lying around (ASilverPickup), as ammo is kept: it comes back where it lay. New within version 2, as TornPosters:
	 * a save from before it reads as none, which is right.
	 */
	UPROPERTY()
	TArray<FSavedSilverPickup> SilverPickups;

	/**
	 * Grave-salt grenade tins lying around (AGrenadePickup), kept as ammo and silver are, with the seconds each had left: a
	 * Save & Quit gives no new life, and a tin whose time runs out is gone. Never Lost Property's (only guns go there). New
	 * within version 2, as TornPosters: a save from before it reads as none, which is right.
	 */
	UPROPERTY()
	TArray<FSavedGrenadePickup> GrenadePickups;

	/**
	 * The loot chests the player has found but not opened (AChest::GetSaveKey; UMapDiscoverySubsystem), so their pins on the
	 * map page survive a Save & Quit and Continue. An opened chest is kept in OpenedChests instead and pinned as looted. New
	 * within version 2, as TornPosters: a save from before it reads as none found, which is right.
	 */
	UPROPERTY()
	TArray<FName> FoundChests;

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
	 * 2: a world per map (Worlds), the campaign record (Campaign) and where a trip arrives (ArrivalTag).
	 * 3: the same fields; the campaign record is on 2026-10-13's missions (CampaignStepMigration: Main 4's steps, the side
	 *    missions that used to start by themselves). An older save is brought up to date as it's read (Upgrade), before
	 *    anything can save it again.
	 * 4: the same fields; the campaign record is on 2026-10-21's missions (CampaignStepMigration::UpgradeTalkCut: Side 2's
	 *    and Knocking Below's steps, Dead Lift's retired crank and coal).
	 */
	static constexpr int32 CurrentVersion = 4;

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

	/**
	 * Level and experience, the tutorial, and the bestiary's kinds met and defeated. Its DefeatedByTier (the defeat counts per
	 * enemy tier, 2026-10-20) is new within version 3: a save from before it reads as tiers unknown, with every total right,
	 * so it needs no upgrade and no new version.
	 */
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
	 * The grave-salt grenades below were captured. A save from before grenades reads as none captured, so the player is
	 * given their starting two as their first gun comes into hand, as in a new game.
	 */
	UPROPERTY()
	bool bHasThrowables = false;

	UPROPERTY()
	FThrowablesSave Throwables;

	/**
	 * The ember powers below were captured (UEmberComponent::SaveEmbers: the equipped one and its cooldown left). A save
	 * from before the powers reads as none captured: the hero's first power equipped, ready.
	 */
	UPROPERTY()
	bool bHasEmbers = false;

	UPROPERTY()
	FEmberSave Embers;

	/**
	 * The hero this session plays (UHeroDefinition::Id: "Ellis", "Odessa", "Crane", "Gauge", "Pike"), picked on the hero
	 * select before its new game and kept for good: another hero means another session. New within version 3: a save
	 * from before the pick has none and reads as Ellis, the only hero there was (GetHeroId), so it needs no upgrade.
	 */
	UPROPERTY()
	FName HeroId;

	/**
	 * The silver carried (UWalletSubsystem; Docs/Progression.md, "Economy"). New within version 2: a save from before money
	 * reads as none carried, which is right, so it needs no upgrade.
	 */
	UPROPERTY()
	int32 Silver = 0;

	/** The vendors' gun of the day as it stood (FVendorOffer). A save from before it reads as none rolled: the first vendor opened rolls one. */
	UPROPERTY()
	FVendorOffer VendorOffer;

	/**
	 * Lost Property's shelves (FLostLootStore: the guns handed in, the shelving bought, the ticket counter, what the HUD has
	 * still to tell), one for the whole session like the purse: ULostLootSubsystem has them while a level plays. New within
	 * version 2: a save from before it reads as empty shelves of 30, which is right; its guns on the ground are handed in as
	 * it's loaded (RestoreWorld).
	 */
	UPROPERTY()
	FLostLootStore LostLoot;

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

	// --- The vigil stones (World/BossShrine; SessionSubsystemBossShrines.cpp) ---

	/**
	 * Each vigil stone's boss by its boss key ("Gravemother", "Abel"): its last defeat, for its call back (five minutes of
	 * play after it, or at once after a Save & Quit), and whether its stone has risen. New within version 2: a save from
	 * before it has none, which is right (a boss beaten then is ready at once), so it needs no upgrade.
	 */
	UPROPERTY()
	TMap<FName, FBossRecallRecord> BossRecalls;

	/**
	 * How many times the session has been loaded from its slot (Continue, or a new game's first save read back; never a
	 * trip's arrival): a boss beaten in an earlier load has had a Save & Quit since. New within version 2: a save from
	 * before it reads as 0, which is right.
	 */
	UPROPERTY()
	int32 SessionLoads = 0;

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
	 * tutorial island when it has none); version 2's campaign record is moved onto 2026-10-13's missions when it was written
	 * before them (CampaignStepMigration::Upgrade). Guns, ammo, health, progress and the player's spot don't move. A save
	 * written by a newer game is left as it is. Returns the version it was read as.
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

	/** The hero played: HeroId, or Ellis for a save from before the pick (UHeroDefinition::DefaultId). */
	FName GetHeroId() const;
};
