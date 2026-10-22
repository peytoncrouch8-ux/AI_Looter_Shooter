#pragma once

#include "CoreMinimal.h"
#include "Affixes/WeaponMakers.h"
#include "Weapons/WeaponTypes.h"

/**
 * The gun makers' numbers as shipped and as tuned live. Each maker's quirk, odds and kind leanings are cvars
 * (Looter.Makers.<Key>.Damage/Accuracy/Range/FireRate/Reload/Recoil/Handling/Magazine in percent, .Wear, .ElementPartBias,
 * .EffectPartBias, .Odds "C,U,R,E,L", .Kinds "LMG=1.5,Launcher=1.5"), plus Looter.Makers.QuirkScale [1] and
 * Looter.Makers.Default [Roundhouse]. They're read when asked, so a change shows on the next gun made or rebuilt.
 */
namespace WeaponMakerTuning
{
	/** A maker's numbers as shipped (the cvars start from these). */
	struct FShipped
	{
		const TCHAR* Key;
		FWeaponMakerQuirk Quirk;
		const TCHAR* Odds;
		const TCHAR* Kinds;
	};

	/** Every maker's shipped numbers, in the table's order. */
	AI_LOOTER_SHOOTER_API TConstArrayView<FShipped> Shipped();
	AI_LOOTER_SHOOTER_API const FShipped* ShippedFor(FName Key);

	/** Every maker's quirk is scaled by this (0 turns them all off, for comparisons). */
	AI_LOOTER_SHOOTER_API float QuirkScale();
	AI_LOOTER_SHOOTER_API FName DefaultKey();

	/** The maker's live quirk (unscaled), its odds string and its kinds string. */
	AI_LOOTER_SHOOTER_API FWeaponMakerQuirk Quirk(FName Key);
	AI_LOOTER_SHOOTER_API FString Odds(FName Key);
	AI_LOOTER_SHOOTER_API FString Kinds(FName Key);

	/** "4,3,2,2,2" at a rarity (Common first); a missing or bad entry is 0, negatives 0. */
	AI_LOOTER_SHOOTER_API float OddsAt(const FString& Odds, EWeaponRarity Rarity);
	/** "LMG=1.5,Launcher=0.4" for a kind (its EWeaponKind name, any case); 1 when it isn't named, negatives 0. */
	AI_LOOTER_SHOOTER_API float LeanFor(const FString& Kinds, EWeaponKind Kind);
}
