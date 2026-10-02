#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

class AActor;
class AWeaponBase;
class UWorld;
struct FMissionRewards;
struct FXPCurve;

/** What finishing a mission gives, worked out and handed over (the runner grants them once per session). */
namespace MissionRewards
{
	/**
	 * The experience a share of a level is worth to a player at Level: round(Share x what Level takes to finish), so a
	 * main mission's 0.3 is 30% of a level at any level. 0 at the maximum level, where experience stops counting.
	 */
	int64 ExperienceFor(float Share, int32 Level, const FXPCurve& Curve);

	/** The reward gun's rarity: as rolled, or Floor when that's rarer. */
	EWeaponRarity ApplyFloor(EWeaponRarity Rolled, EWeaponRarity Floor);

	/**
	 * Rolls the reward gun (its kind, or one from the default loot table) at Level, with the rarity floor, and drops it in
	 * front of Player like loot. Null when the rewards have no gun, there's no player, or no kind could be found.
	 */
	AWeaponBase* DropGun(UWorld* World, const FMissionRewards& Rewards, int32 Level, const AActor* Player);

	/** The rewards in words, one line each, for the Missions page: "+30% of a level's experience", "Gun: Rare or better". */
	TArray<FString> Describe(const FMissionRewards& Rewards);
}
