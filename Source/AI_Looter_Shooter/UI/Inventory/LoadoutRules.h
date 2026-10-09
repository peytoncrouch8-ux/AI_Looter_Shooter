#pragma once

#include "CoreMinimal.h"

class UWeaponDefinition;
struct FWeaponInstanceData;
struct FWeaponPartTotals;
struct FWeaponStats;

/**
 * Rules behind the loadout screen (the backpack list's order, its compare arrows, what a gun's card shows at first glance
 * and on Inspect), kept apart from the widget so they can be tested.
 */
namespace LoadoutRules
{
	enum class EVerdict : uint8
	{
		/** Not compared (a different kind of gun, or nothing to compare with). */
		None,
		Upgrade,
		Similar,
		Weaker
	};

	/** Damage per second (every pellet of every shot): the number verdicts compare. */
	AI_LOOTER_SHOOTER_API float DamagePerSecond(const FWeaponStats& Stats);

	/**
	 * How a backpack gun compares with the gun it would replace. Only guns of the same kind are compared, on damage per
	 * second: more than 3% better is an upgrade, more than 3% worse is weaker, in between similar.
	 */
	AI_LOOTER_SHOOTER_API EVerdict Compare(const FWeaponInstanceData& Candidate, const FWeaponInstanceData* Current);

	/** How the backpack list is ordered; R (or the list's sort label) steps through them. */
	enum class ESort : uint8
	{
		/** The swap target's kind of gun first, best damage per second first; then the rest by rarity and level. */
		Match,
		/** Rarest first, then the higher level. */
		Rarity,
		/** Highest level first, then the rarer. */
		Level,
		/** The latest found first. */
		Newest,
		Count
	};

	/** "Best for slot", "Rarity", "Level", "Newest". */
	AI_LOOTER_SHOOTER_API const TCHAR* SortName(ESort Sort);
	AI_LOOTER_SHOOTER_API ESort NextSort(ESort Sort);

	/**
	 * The backpack in the order the list shows it (indices into Backpack). Ties keep backpack order, so the list never
	 * shuffles guns that are alike. Returns how many lead the list as the target's kind of gun: only for Match with a
	 * target gun, else 0.
	 */
	AI_LOOTER_SHOOTER_API int32 SortBackpack(const TArray<FWeaponInstanceData>& Backpack, const FWeaponInstanceData* Target, ESort Sort,
		TArray<int32>& OutOrder);

	/** The numbers a gun's card can show. */
	enum class EStat : uint8
	{
		Damage,
		FireRate,
		Magazine,
		Reload,
		Accuracy,
		Range,
		Recoil,
		Handling,
		Zoom
	};

	/**
	 * What a card shows at first glance: damage (the big number) and the four that decide how a gun fights, fire rate,
	 * magazine, reload and accuracy. The rest wait behind Inspect.
	 */
	AI_LOOTER_SHOOTER_API TConstArrayView<EStat> FirstGlanceStats();

	/** Every stat, in the order Inspect lists them (the first glance's first). */
	AI_LOOTER_SHOOTER_API TConstArrayView<EStat> AllStats();

	AI_LOOTER_SHOOTER_API const TCHAR* StatName(EStat Stat);

	/** The number compared: damage is the whole shot's (pellets too), range is in metres, recoil and handling in percent. */
	AI_LOOTER_SHOOTER_API float StatValue(EStat Stat, const FWeaponStats& Stats);

	/** The number as the card writes it: "7 x9", "600", "2.10s", "1.2°", "40 m", "85%", "4x". */
	AI_LOOTER_SHOOTER_API FString StatText(EStat Stat, const FWeaponStats& Stats);

	/** 0-1 for its bar (rough scale across every kind of gun). */
	AI_LOOTER_SHOOTER_API float StatRating(EStat Stat, const FWeaponStats& Stats);

	/** False for reload time, spread and recoil, where less is better. */
	AI_LOOTER_SHOOTER_API bool HigherIsBetter(EStat Stat);

	/** How a stat moved against the gun it would replace: the card's arrow (up green for better, down red for worse). */
	enum class EChange : uint8
	{
		Same,
		Better,
		Worse
	};
	AI_LOOTER_SHOOTER_API EChange CompareStat(EStat Stat, const FWeaponStats& New, const FWeaponStats& Old);

	/** The signed change written beside the arrow ("+6", "-0.25s"); empty when the same. */
	AI_LOOTER_SHOOTER_API FString DeltaText(EStat Stat, const FWeaponStats& New, const FWeaponStats& Old);

	/** One of a gun's parts' bonuses as a percentage the parts add up to (the card's special line). */
	struct FBonus
	{
		EStat Stat = EStat::Damage;
		/** As the parts write it: +12 is 12% more; for reload and recoil, less (negative) is better. */
		float Percent = 0.f;
		/** How good it is: Percent, turned round for reload and recoil, so above 0 always helps. */
		float Goodness = 0.f;
	};

	/** Every bonus the parts give (zero ones left out), the most helpful first and the most harmful last. */
	AI_LOOTER_SHOOTER_API TArray<FBonus> Bonuses(const FWeaponPartTotals& Totals);

	/** The best few that help, for the card's first glance (none when no part helps). */
	AI_LOOTER_SHOOTER_API TArray<FBonus> TopBonuses(const FWeaponPartTotals& Totals, int32 MaxCount = 2);

	/** "+24% ACCURACY", "-30% RELOAD TIME". */
	AI_LOOTER_SHOOTER_API FString BonusText(const FBonus& Bonus);

	/** The gun's parts' bonuses added up, as its stats were worked out (a named gun's at its fixed quality). */
	AI_LOOTER_SHOOTER_API FWeaponPartTotals PartTotals(const FWeaponInstanceData& Gun);

	/**
	 * A gun's identity for the screen's NEW marks: its kind, named gun, seed, rarity and level. What happens to it after
	 * (kills, parts fitted at the bench, its magazine) doesn't change it, so a gun is new once, when it's found.
	 */
	AI_LOOTER_SHOOTER_API uint32 GunIdentity(const FWeaponInstanceData& Gun);
}
