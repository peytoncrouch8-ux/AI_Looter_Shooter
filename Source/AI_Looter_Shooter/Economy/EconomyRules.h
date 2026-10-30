#pragma once

#include "CoreMinimal.h"
#include "Weapons/AmmoTypes.h"
#include "Weapons/WeaponTypes.h"

enum class ECreatureRank : uint8;
enum class EChestKind : uint8;
struct FRandomStream;

/**
 * The game's money: silver (Docs/Progression.md, "Economy"). Silver is the living's coin on the frontier; grave gold, the
 * old fare laid on dead men's eyes and Sexton's pay to the gang, stays rare and the story's. What drops it, what the
 * vendors ask and pay, and the gun of the day's odds, as plain numbers and functions the tests read.
 *
 * Every amount grows with level as enemies do (LevelScale: 8% of its level 1 value a level), so what the world gives and
 * what the vendors ask keep their ratio at every level. The amounts below are level 1's.
 */
namespace LooterEconomy
{
	/** The most silver the player carries. */
	inline constexpr int32 MaxSilver = 9999999;

	/** How amounts grow with level: 8% of level 1's a level (FLevelRules' enemy growth). */
	inline constexpr float LevelGrowth = 0.08f;

	/** 1 + LevelGrowth x (Level - 1); 1 at level 1 and below. */
	AI_LOOTER_SHOOTER_API float LevelScale(int32 Level);

	/** Base at Level: Base x LevelScale, rounded, never under Base's own sign (0 stays 0, 1 stays at least 1). */
	AI_LOOTER_SHOOTER_API int32 AtLevel(int32 Base, int32 Level);

	/** An amount for people, grouped: "1,240". The coin glyph beside it says silver. */
	AI_LOOTER_SHOOTER_API FString Format(int32 Silver);

	/** An amount in words: "1,240 silver". */
	AI_LOOTER_SHOOTER_API FText Words(int32 Silver);

	// --- What drops it ---

	/** One drop's odds at level 1: the chance it drops at all, then an amount from Min to Max (both included). */
	struct FDropOdds
	{
		float Chance = 0.f;
		int32 Min = 0;
		int32 Max = 0;

		/** What it gives on average at level 1. */
		float Average() const { return Chance * 0.5f * static_cast<float>(Min + Max); }
	};

	/**
	 * A kill's silver by the creature's rank: the Unpaid were townsfolk once and still have their pockets, so they drop it
	 * more often and more of it than beasts (spiders, slimes), whose silver is whatever they swallowed. Regular, Restless,
	 * Gravebound, a Soulfed monster and a boss pay amounts of their own (as before the top tiers); Hallowed and Ethereal pay
	 * Gravebound's times their SilverMultiplier (x1.6, x2.5: ScaledKillOdds).
	 */
	AI_LOOTER_SHOOTER_API FDropOdds KillOdds(ECreatureRank Rank, bool bUnpaid);

	/**
	 * Odds worth Multiplier times as much on average: the chance it drops at all as if it had Multiplier tries
	 * (1 - (1 - Chance)^Multiplier), the amount grown to make up the rest.
	 */
	AI_LOOTER_SHOOTER_API FDropOdds ScaledKillOdds(const FDropOdds& Regular, float Multiplier);

	/** A container's silver (a Ranger cache, the gang's Strongbox, a coffin, a grave, a mailbox, a footlocker). */
	AI_LOOTER_SHOOTER_API FDropOdds ContainerOdds(EChestKind Kind);

	/** A crate's or a barrel's, now and then. */
	AI_LOOTER_SHOOTER_API FDropOdds BreakableOdds();

	/**
	 * Rolls Odds at Level from Random: the amount, or 0 when the chance says none. It always draws the same two numbers
	 * (the chance, then the amount), so what's rolled after it doesn't move.
	 */
	AI_LOOTER_SHOOTER_API int32 Roll(const FDropOdds& Odds, int32 Level, FRandomStream& Random);

	/** What Odds give on average at Level. */
	AI_LOOTER_SHOOTER_API float Expected(const FDropOdds& Odds, int32 Level);

	// --- What the vendors ask and pay ---

	/** A vendor's box of Type: a chest pickup's 36 rounds, at most half of what the player can carry (24 shells, 18 sniper rounds). */
	AI_LOOTER_SHOOTER_API int32 AmmoBoxRounds(EAmmoType Type);

	/** A full box's price for a player at PlayerLevel (15 at level 1, every type). */
	AI_LOOTER_SHOOTER_API int32 AmmoBoxPrice(EAmmoType Type, int32 PlayerLevel);

	/** Rounds of Type, part of a box when the player has less room: the box's share, rounded up, at least 1. */
	AI_LOOTER_SHOOTER_API int32 AmmoPrice(EAmmoType Type, int32 Rounds, int32 PlayerLevel);

	/** One jar of grave salt for a player at PlayerLevel (25 at level 1). */
	AI_LOOTER_SHOOTER_API int32 GrenadePrice(int32 PlayerLevel);

	/** What a vendor pays and asks for a gun of a rarity at level 1. */
	AI_LOOTER_SHOOTER_API int32 SellBase(EWeaponRarity Rarity);
	AI_LOOTER_SHOOTER_API int32 BuyBase(EWeaponRarity Rarity);

	/** A cursed iron fetches this share of its price: a storekeeper pays less for a gun that bites. */
	inline constexpr float CursedSellShare = 0.75f;

	/** What a vendor asks for Gun (the gun of the day): by its rarity, at its level. */
	AI_LOOTER_SHOOTER_API int32 GunBuyPrice(const FWeaponInstanceData& Gun);

	/** What a vendor pays for Gun: by its rarity, at its level, a cursed iron for less; 0 for one a vendor won't buy. */
	AI_LOOTER_SHOOTER_API int32 GunSellPrice(const FWeaponInstanceData& Gun);

	/** Whether a vendor buys Gun at all: a story heirloom (a named gun without a signature: Heirloom, Pa's) stays with the
	 *  player; a boss's named legendary sells (behind the confirm: SaleNeedsConfirm). OutWhy says why not. */
	AI_LOOTER_SHOOTER_API bool CanSell(const FWeaponInstanceData& Gun, FText* OutWhy = nullptr);

	/** Selling Gun asks first: Rare and better. */
	AI_LOOTER_SHOOTER_API bool SaleNeedsConfirm(const FWeaponInstanceData& Gun);

	// --- The gun of the day ---

	/** Play between two guns of the day when no level-up brings one sooner: half an hour (was an hour; 2026-10-30, an
	 *  explorer's silver piled up with too little to buy). */
	inline constexpr double OfferDaySeconds = 1800.0;

	/** The gun of the day's chance of each rarity, in EWeaponRarity order: never Common or Uncommon, Rare most days. Adds to 1. */
	AI_LOOTER_SHOOTER_API float OfferRarityChance(EWeaponRarity Rarity);

	/** Rolls the gun of the day's rarity by OfferRarityChance (one number from Random). */
	AI_LOOTER_SHOOTER_API EWeaponRarity RollOfferRarity(FRandomStream& Random);
}
