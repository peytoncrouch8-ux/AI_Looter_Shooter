#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponTypes.h"

class UPrimitiveComponent;
class UWeaponDefinition;

/**
 * A maker's handling trade, in percent of the finished stat (+10 = 10% more; Reload is reload time, so + is slower;
 * Recoil + kicks harder), and its leanings. Zeros and ones change nothing. The shipped numbers are the table's; the live
 * ones come from the Looter.Makers.<Key>.* cvars (WeaponMakerTuning).
 */
struct FWeaponMakerQuirk
{
	float Damage = 0.f;
	float Accuracy = 0.f;
	float Range = 0.f;
	float FireRate = 0.f;
	float Reload = 0.f;
	float Recoil = 0.f;
	float Handling = 0.f;
	/** Never on the kinds whose rounds show (a revolver's cylinder, a launcher's rocket): WeaponMakers::ChangesMagazine. */
	float Magazine = 0.f;
	/** Added to the gun's wear (0 fresh to 1 battered). */
	float Wear = 0.f;
	/** An element part's chance (its weight) times this when the gun's parts are picked. */
	float ElementPartBias = 1.f;
	/** An effect part's chance times this. */
	float EffectPartBias = 1.f;
};

/**
 * One of the Reaches' gun makers (the user's call, 2026-10-22; Docs/Work/2026-10-22-Next/MAKERS.md). Every rolled gun
 * has one, by its seed: it adds a handling trade (Quirk), repaints the gun (Paints over the definition's), puts its mark
 * on the body (MarkCell of T_GunMakerMarks, in MarkColor) and names a gun no part names. The table is in WeaponMakers.cpp,
 * keyed by names that never change once guns drop (a gun saves its maker's key).
 */
struct FWeaponMaker
{
	FName Key;
	/** "Meridian": the card's maker line and the word a gun no part names takes. */
	FText Name;
	/** "Meridian Instrument Co." */
	FText FullName;
	/** "Highwater Landing" */
	FText Place;
	/** The one-line fiction, for the inspect card. */
	FText Fiction;
	/** "True Glass": the quirk's name on the inspect card. */
	FText QuirkName;
	/** Its numbers as made (the cvars start from these). */
	FWeaponMakerQuirk Quirk;
	/** "4,3,2,2,2": its weight at each rarity, Common to Legendary (the cvar Looter.Makers.<Key>.Odds starts from it). */
	const TCHAR* Odds = TEXT("1,1,1,1,1");
	/** "LMG=1.5,Launcher=1.5": how much more or less it makes a kind (1 unless named; Looter.Makers.<Key>.Kinds). */
	const TCHAR* Kinds = TEXT("");
	/** Material slot -> weighted colours (linear), over the definition's paints. */
	TArray<FWeaponPaint> Paints;
	/** Its cell in T_GunMakerMarks, 1-4 (Art/Icons/MakerMarks.py MARK_ORDER). */
	int32 MarkCell = 0;
	/** The mark's paint on the gun (linear). */
	FLinearColor MarkColor = FLinearColor::White;
	/** Its Inked icon (InkedIconData: "MakerMeridian"), for cards and labels. */
	FName MarkIcon;
};

/** The mark a gun's part wears: which part (its slot index in the look) and the gun master's custom primitive data. */
struct FWeaponMarkPaint
{
	int32 PartIndex = INDEX_NONE;
	float Cell = 0.f;
	FWeaponMarkPlace Place;
	FLinearColor Color = FLinearColor::White;

	bool IsSet() const { return PartIndex != INDEX_NONE && Cell > 0.f; }
};

namespace WeaponMakers
{
	/**
	 * Where the gun master (M_Gun) reads the mark from the part's custom primitive data: MarkDataIndex the cell (0 = none),
	 * +1 X, +2 Z, +3 size, +4 depth (cm, the part's space), +5..+7 the colour. After the notches' 1-7 and soul-light's 8-11.
	 */
	inline constexpr int32 MarkDataIndex = 12;
	inline constexpr int32 MarkDataCount = 8;

	AI_LOOTER_SHOOTER_API TConstArrayView<FWeaponMaker> All();
	/** The maker with this key, or null. */
	AI_LOOTER_SHOOTER_API const FWeaponMaker* Find(FName Key);
	/** The maker an old saved gun takes (Looter.Makers.Default, Roundhouse). */
	AI_LOOTER_SHOOTER_API FName DefaultKey();
	/** The gun's maker: null for a named gun, a gun without one (None) or a key that no longer exists. */
	AI_LOOTER_SHOOTER_API const FWeaponMaker* Of(const FWeaponInstanceData& Gun);

	/** A maker's weight for a gun of this kind and rarity (its odds times its lean to the kind; live). */
	AI_LOOTER_SHOOTER_API float Weight(const FWeaponMaker& Maker, EWeaponKind Kind, EWeaponRarity Rarity);
	/** The maker a new gun of this kind and rarity rolls from its seed (its own stream: no other roll moves). */
	AI_LOOTER_SHOOTER_API FName Roll(EWeaponKind Kind, int32 Seed, EWeaponRarity Rarity);
	AI_LOOTER_SHOOTER_API FName Roll(const UWeaponDefinition* Definition, int32 Seed, EWeaponRarity Rarity);

	/** The maker's live quirk (its cvars) times Looter.Makers.QuirkScale. */
	AI_LOOTER_SHOOTER_API FWeaponMakerQuirk QuirkOf(const FWeaponMaker& Maker);
	/** The maker's magazine change applies to this kind (not a revolver's cylinder or a launcher's rocket: their rounds show). */
	AI_LOOTER_SHOOTER_API bool ChangesMagazine(EWeaponKind Kind);
	/** The quirk on finished stats (pure: the rule the tests check). */
	AI_LOOTER_SHOOTER_API void ApplyQuirk(const FWeaponMakerQuirk& Quirk, EWeaponKind Kind, FWeaponStats& InOutStats);
	/** The gun's maker's quirk on its stats (nothing without one): UWeaponRollLibrary::ComputeInstanceStats. */
	AI_LOOTER_SHOOTER_API void ApplyToStats(const FWeaponInstanceData& Gun, FWeaponStats& InOutStats);

	/** An option's chance when a gun's parts are picked: its weight, times the maker's leaning to its element or effect. */
	AI_LOOTER_SHOOTER_API float PartWeight(const FWeaponMaker* Maker, const FWeaponPartOption& Option);
	/** Recolours the look with the maker's paints, from its own stream of the gun's seed. */
	AI_LOOTER_SHOOTER_API void PaintLook(const FWeaponMaker& Maker, int32 Seed, FWeaponLook& InOutLook);
	/** The maker's wear on top of the gun's own (0 without one). */
	AI_LOOTER_SHOOTER_API float ExtraWear(const FWeaponInstanceData& Gun);
	/** The word a gun no part names takes ("Meridian"); empty without a maker. */
	AI_LOOTER_SHOOTER_API FText NameWord(const FWeaponInstanceData& Gun);

	/** The mark: on the first part (slot order) with a mark place; not set without a maker or such a part. */
	AI_LOOTER_SHOOTER_API FWeaponMarkPaint MarkFor(const FWeaponInstanceData& Gun, const FWeaponLook& Look);
	/** Writes the mark into a part's custom primitive data (an unset mark clears it). */
	AI_LOOTER_SHOOTER_API void PaintMark(UPrimitiveComponent& Part, const FWeaponMarkPaint& Mark);
}
