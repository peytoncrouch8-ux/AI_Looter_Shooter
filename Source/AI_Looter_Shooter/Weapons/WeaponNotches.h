#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponNotches.generated.h"

/** How far a gun has come by its kills: it wakes at each milestone. */
UENUM()
enum class ENotchTier : uint8
{
	None,
	/** 50 kills: +3% damage. */
	Blooded,
	/** 250 kills: +6% damage in all, and a nickname in quotes after its name. */
	Named,
	/** 1,000 kills: +10% damage in all, and a faint soul-light in its rarity's color in the hand. */
	SoulForged
};

/**
 * Notches: every gun counts the creatures it has killed (FWeaponInstanceData::Kills), shows them as tally marks cut in
 * its stock, and wakes at milestones. AWeaponBase::AddKill counts them; UWeaponRollLibrary::ComputeInstanceStats adds
 * the damage, so a gun rebuilt from its instance has it.
 */
namespace WeaponNotches
{
	inline constexpr int32 BloodedKills = 50;
	inline constexpr int32 NamedKills = 250;
	inline constexpr int32 SoulForgedKills = 1000;
	/** One tally mark per 5 kills, four cuts and a slash per group, until the row is full at 25 marks. */
	inline constexpr int32 KillsPerMark = 5;
	inline constexpr int32 MaxMarks = 25;
	/** A cursed iron's drawback lifts by itself at this many notches. */
	inline constexpr int32 CurseLiftKills = 100;

	AI_LOOTER_SHOOTER_API ENotchTier TierFor(int32 Kills);
	/** 1, 1.03 (Blooded), 1.06 (Named), 1.10 (Soul-forged). */
	AI_LOOTER_SHOOTER_API float DamageMultiplier(int32 Kills);
	/** Tally marks cut in the stock: Kills / KillsPerMark, at most MaxMarks. */
	AI_LOOTER_SHOOTER_API int32 Marks(int32 Kills);

	/** The tier's word as the cards and messages say it: "Blooded", "Named", "Soul-forged"; empty for None. */
	AI_LOOTER_SHOOTER_API FText TierName(ENotchTier Tier);

	/** What one more kill did to a gun. */
	struct FKillResult
	{
		/** The milestone it just reached (None if none). */
		ENotchTier Reached = ENotchTier::None;
		/** Its curse's drawback lifted with this kill (CurseLiftKills). */
		bool bCurseLifted = false;
	};
	/**
	 * Counts one creature kill on the gun (Kills, and bCurseLifted at CurseLiftKills). The caller recomputes its stats when
	 * Reached isn't None, and when bCurseLifted (a lifted Restless curse stops doubling the recoil).
	 */
	AI_LOOTER_SHOOTER_API FKillResult AddKill(FWeaponInstanceData& Gun);

	/** Its nickname from its kind's list by its seed once Named ("Lantern Jaw"); empty below NamedKills and for a named gun. */
	AI_LOOTER_SHOOTER_API FString Nickname(const FWeaponInstanceData& Gun);
	/** The nicknames a kind of gun can earn (a kind without a list of its own uses the rifles'). */
	AI_LOOTER_SHOOTER_API TConstArrayView<const TCHAR*> Nicknames(EWeaponKind Kind);

	/** "WHISPER BULLPUP IS BLOODED · +3% DAMAGE" and the like, for the HUD's message line. */
	AI_LOOTER_SHOOTER_API FText MilestoneMessage(const FWeaponInstanceData& Gun, ENotchTier Tier);
	/** "WHISPER BULLPUP'S CURSE IS LIFTED" */
	AI_LOOTER_SHOOTER_API FText CurseLiftedMessage(const FWeaponInstanceData& Gun);
}
