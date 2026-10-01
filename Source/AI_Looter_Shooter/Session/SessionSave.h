#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/WeaponInventorySave.h"
#include "Progression/PlayerProgressData.h"
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
 * One of the three sessions (save slots "Session1" to "Session3", USessionSubsystem): the player as they left it (where
 * they stood, health, level and experience, bestiary, guns and ammo) and the world (loot on the ground, what the gun
 * racks still offered, the tutorial's step). Creatures aren't kept: they're all back when a session is loaded, as they
 * come back after a respawn. A new game is a save with nothing captured yet.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULooterSessionSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** 1: the first sessions. */
	static constexpr int32 CurrentVersion = 1;

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

	/** The level the player was in (its package name); the session continues there. Empty: the game's first level. */
	UPROPERTY()
	FString Map;

	// --- The player ---

	UPROPERTY()
	FPlayerProgressData Progress;

	/** Where the player stood and looked. Without it (a new game, or saved while dead) they start at the level's start. */
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

	// --- The world ---

	/** The world below was captured; without it (a new game) the level starts as it was built. */
	UPROPERTY()
	bool bHasWorld = false;

	UPROPERTY()
	TArray<FSavedLootWeapon> LootWeapons;

	UPROPERTY()
	TArray<FSavedAmmoPickup> AmmoPickups;

	UPROPERTY()
	TArray<FSavedWeaponRack> Racks;

	/** The tutorial step on screen, or INDEX_NONE when it wasn't running. */
	UPROPERTY()
	int32 TutorialStep = INDEX_NONE;
};
