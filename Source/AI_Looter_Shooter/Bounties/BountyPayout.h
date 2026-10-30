#pragma once

#include "CoreMinimal.h"
#include "Bounties/BountyTypes.h"
#include "Weapons/WeaponTypes.h"

class ABountyBoard;
class APawn;
class ULootTable;
class UWorld;
struct FBountyTuning;
struct FRandomStream;

/** What the board handed over for a bounty (BountyPayout::HandOver): how many pickups, the rarest gun among them, the silver. */
struct AI_LOOTER_SHOOTER_API FBountyHaul
{
	/** Pickups thrown out (guns, wards, ammo, a grenade; the silver's coins not counted). */
	int32 Items = 0;
	/** A gun came with it, of this rarity at best, in its rarity's colour (the mission banner's "RARE GUN"). */
	bool bGun = false;
	EWeaponRarity BestGun = EWeaponRarity::Common;
	FLinearColor BestGunColor = FLinearColor::White;
	/** Silver spilled with it (a Strongbox's own roll; the purse is paid into the wallet apart). */
	int32 Silver = 0;
};

/**
 * A bounty's loot handed over at the board as it's paid (the user's playtest, 2026-10-27, note 40: no chest at the fight;
 * the reward comes when the bounty is turned in). The rolls are the old strongbox's, so nothing about the reward's worth
 * changed: the tier's guns (BountyRules::RewardLoot) from the default loot table at a Strongbox's luck, two ammo pickups
 * of a chest's 1.5 boxes, a grave-salt grenade at a Strongbox's chance, a Strongbox's silver. They come out of the board's
 * face toward the player, fanned so none lands on another, and land as any loot does (their beams, the rarity's sting).
 */
namespace BountyPayout
{
	/** The loot table its guns and ammo are rolled from: the default table's guns at the tier's count and the tuning's luck. */
	AI_LOOTER_SHOOTER_API ULootTable* MakeTable(EBountyTier Tier, const FBountyTuning& Tuning, bool bGunsDropHere);

	/**
	 * The Index-th of Count pickups' throw out of the board toward Toward (flat): fanned across HandOverFan degrees, each a
	 * little off its share, up and out so it lands between the board and the player.
	 */
	AI_LOOTER_SHOOTER_API FVector TossVelocity(int32 Index, int32 Count, const FVector& Toward, FRandomStream& Random);

	/**
	 * Hands a Tier bounty's loot to Player at Board: rolled at the player's level kept inside the area's band (as a chest
	 * rolls), thrown out of the board's face toward them. Without a board, from just in front of the player. What it gave.
	 */
	AI_LOOTER_SHOOTER_API FBountyHaul HandOver(UWorld* World, const ABountyBoard* Board, const APawn* Player, EBountyTier Tier,
		const FBountyTuning& Tuning);
}
