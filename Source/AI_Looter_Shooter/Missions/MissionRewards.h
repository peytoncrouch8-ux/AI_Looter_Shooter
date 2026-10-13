#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

class AActor;
class AWeaponBase;
class UNamedWeaponDefinition;
class UWorld;
struct FMissionRewards;
struct FXPCurve;

/**
 * Where a named gun is in a level: carried by the player (in a slot, in hand or on their back, or in their backpack), and
 * the copies lying there as loot. A named gun is one of a kind: what a reward gives is checked against this.
 */
struct AI_LOOTER_SHOOTER_API FNamedGunWhereabouts
{
	bool bCarried = false;
	TArray<AWeaponBase*> Lying;

	bool IsAnywhere() const { return bCarried || !Lying.IsEmpty(); }
};

/**
 * What a mission's end actually gave, for whoever announces it (the HUD's mission-complete banner): the experience as
 * added, the reward gun's rolled rarity, the named gun and the areas opened, by name. Empty when it gave nothing (a
 * mission finished again, or one with no rewards).
 */
struct AI_LOOTER_SHOOTER_API FMissionRewardsGiven
{
	int64 Experience = 0;
	/** A reward gun was dropped, of this rarity (rolled, then raised to the mission's floor), and its rarity's colour. */
	bool bGun = false;
	EWeaponRarity GunRarity = EWeaponRarity::Common;
	FLinearColor GunColor = FLinearColor::White;
	/** The named gun's name ("Heirloom"), handed over in the story or dropped; empty for none. */
	FText NamedGun;
	/** The areas opened to travel, by their names ("The Gilded Lily"). */
	TArray<FText> AreasOpened;
	/** The silver its purse paid into the wallet (MissionRewards::GiveSilver). */
	int32 Silver = 0;

	bool IsEmpty() const { return Experience <= 0 && !bGun && NamedGun.IsEmpty() && AreasOpened.IsEmpty() && Silver <= 0; }
};

/** What finishing a mission gives, worked out and handed over (the runner grants them once per session, at the turn-in). */
namespace MissionRewards
{
	/**
	 * The experience a share of a level is worth to a player at Level: round(Share x what Level takes to finish), so a
	 * share of 0.3 is 30% of a level at any level. 0 at the maximum level, where experience stops counting.
	 */
	int64 ExperienceFor(float Share, int32 Level, const FXPCurve& Curve);

	/**
	 * The experience the rewards give a player at Level: their fixed Experience when set (Docs/Progression.md tunes each),
	 * else their share of the level (ExperienceFor). 0 at the maximum level.
	 */
	int64 ExperienceOf(const FMissionRewards& Rewards, int32 Level, const FXPCurve& Curve);

	/** The reward gun's rarity: as rolled, or Floor when that's rarer. */
	EWeaponRarity ApplyFloor(EWeaponRarity Rolled, EWeaponRarity Floor);

	/**
	 * Every reward gun's way into the world: Instance comes up a little ahead of Player (toward whoever they turned the
	 * mission in to) and is tossed up to land and settle as loot, its beam up and its rarity heard and seen as it lands
	 * (ULootFanfareSubsystem). Never through what stands between them: a shop window, a screen door or a fence ahead keeps
	 * it on the player's side, tossed straight up. Null when it couldn't be spawned.
	 */
	AWeaponBase* DropInFront(UWorld* World, const FWeaponInstanceData& Instance, const AActor& Player);

	/**
	 * Rolls the reward gun (its kind, or one from the default loot table) at Level, with the rarity floor, and drops it in
	 * front of Player (DropInFront). Null when the rewards have no gun, there's no player, or no kind could be found.
	 */
	AWeaponBase* DropGun(UWorld* World, const FMissionRewards& Rewards, int32 Level, const AActor* Player);

	/**
	 * Makes the reward's named gun (NamedGun, by its id: Heirloom) at Level and drops it in front of Player as the reward
	 * gun is. Null when the rewards name none, there's no player, or no named gun has that id.
	 */
	AWeaponBase* DropNamedGun(UWorld* World, const FMissionRewards& Rewards, int32 Level, const AActor* Player);

	/** Where Named is in World (FNamedGunWhereabouts): in Player's slots or backpack, lying as loot. Player may be null. */
	FNamedGunWhereabouts FindNamedGun(UWorld* World, const UNamedWeaponDefinition* Named, const AActor* Player);

	/**
	 * A named gun handed over in the story (bNamedGunByHand: Heirloom through Delia's door) made sure of as its mission is
	 * turned in, so the player never goes without it and never gets two: nothing when they carry it; one never taken (left
	 * on her porch, or dropped here) is brought to them, the very same gun; one never handed out (a save before it came
	 * out, or the turn-in before her words were done) is dropped in front of them. The gun dropped now, or null.
	 */
	AWeaponBase* EnsureNamedGun(UWorld* World, const FMissionRewards& Rewards, int32 Level, const AActor* Player);

	/** The named gun's name for people ("Heirloom"): its asset's display name, else its id as words. Empty for none. */
	FText NamedGunName(const FMissionRewards& Rewards);

	/** An area's name for people ("The Gilded Lily"): its asset's display name, else its id. */
	FText AreaName(FName AreaId);

	/**
	 * The silver a mission's turn-in pays (Docs/Progression.md, "Economy"): a fixed purse per mission, by its id, the same at
	 * any level. 0 for the ones that pay none: Sexton's (he pays in grave gold, and not to Ellis), the boss's and the
	 * Gravemother's (their loot is the reward), Skyreach's practice postings and the tutorial.
	 */
	int32 SilverFor(FName MissionId);

	/** Pays a mission's purse into World's wallet (UWalletSubsystem); what went in, 0 without a wallet or a purse. */
	int32 GiveSilver(UWorld* World, FName MissionId);

	/**
	 * The rewards in words, one line each, for the Missions page: "+40 XP" (or "+30% of a level's experience"), "+40 silver",
	 * "Gun: Rare or better", "Named gun: Heirloom", "Opens The Gilded Lily". The purse isn't part of FMissionRewards (it is
	 * kept by mission id, SilverFor), so the silver line comes only when MissionId is given and its purse isn't 0.
	 */
	TArray<FString> Describe(const FMissionRewards& Rewards, FName MissionId = NAME_None);
}
