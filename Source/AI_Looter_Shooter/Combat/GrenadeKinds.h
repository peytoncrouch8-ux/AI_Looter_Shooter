#pragma once

#include "CoreMinimal.h"
#include "Combat/GraveSaltTuning.h"
#include "Combat/GrenadeKindsTuning.h"
#include "Elements/LooterElement.h"

/**
 * The grenade kinds (round 4, 2026-10-27, the user's pick: "a few kinds found as loot"). Every jar is grave salt packed
 * in a tin (Docs/Story.md: salt burns the dead); the three others are packed with something more by whoever filled them:
 *  - Salt (the grave-salt jar, as before): the widest burst (4 m), knocks bodies out of it, burns the Unpaid x1.5.
 *  - Pitch: salt in hot pitch. It sticks to the first thing it touches (a creature carries it), bursts in 3 m with a
 *    1.5 m full-damage core, and tars what it doesn't kill (walking at half speed for 3 s; never a boss). No knock.
 *  - Scatter: a cartridge maker's trick: a smaller burst that flings a brood of four small jars, popping round it one
 *    after another (each 2 m). Covers a spread pack; each piece is weak.
 *  - Cinder: salt over forge coals. A smaller burst that leaves a bed of burning cinders (2.6 m, 5 s) burning what walks
 *    in it as Ember (spiders and slimes are weak to it, the Unpaid resist it). No knock: they stay in the fire.
 * Numbers: FGrenadeKindTuning (shares of the salt jar's FGraveSaltTuning, so the early ramp holds for every kind).
 *
 * Found as loot: a dropped tin is a special kind at Looter.Grenade.Kinds.Share (35%) from player level 2 (RollDrop).
 * The carry rule (Collect): the player carries one kind. A special tin found swaps the kind carried, keeping the count
 * and adding its jar; a plain salt tin (and the vendor's jar, the porch plate's, the first gun's) is salt for any jar, so
 * it adds to the count and never changes the kind. So a found kind stays until another is found, and the common salt
 * drops refill it.
 */
enum class EGrenadeKind : uint8
{
	Salt,
	Pitch,
	Scatter,
	Cinder
};

/** How a jar's burst looks and sounds on top of the salt jar's explosion (GraveSaltBurst::DrawBlast, PlaySounds). */
struct FGrenadeBurstLook
{
	/** The fire's colour. */
	FLinearColor Fire = FLinearColor(1.f, 0.55f, 0.22f);
	/** The white salt flung on top: times the tuning's Look.Salt. */
	float Salt = 1.f;
	/** Globs flung out of it (the pitch's tar): their tint and size (cm; 0 none). */
	FLinearColor GlobTint = FLinearColor::Black;
	float GlobSize = 0.f;
	/** An element's burst of its own on top (the cinder's embers; None for none). */
	ELooterElement Element = ELooterElement::None;
	/** A small jar's pop (the scatter's brood): a quick flash and puff, not the launcher's whole blast. */
	bool bSmall = false;
	/** The scorch left: times the tuning's Look.Scorch. */
	float Scorch = 1.f;
	/** The point light's flash and the views' jolt: times the salt jar's. */
	float Flash = 1.f;
	float Kick = 1.f;
	/** The salt jar's burst sound at this volume (0: not played), and the kind's own layer over it (null: none). */
	float SaltVolume = 1.f;
	const TCHAR* LayerCue = nullptr;
};

/** What finding a tin does to what the player carries (GrenadeKinds::Collect). */
struct FGrenadeCollect
{
	EGrenadeKind Kind = EGrenadeKind::Salt;
	int32 Count = 0;
	/** Jars of the tin's that went in (0 when the count was full). */
	int32 Taken = 0;
	/** The kind carried changed. */
	bool bSwapped = false;
};

/** The kinds' own sounds (Art/Sounds/recipes/throwables_kinds.py). They play over the salt jar's (ThrowCue) where a kind has one. */
namespace GrenadeKindCue
{
	/** The pitch jar slapping onto something and holding: a wet, heavy smack and a sticky pull. */
	inline constexpr const TCHAR* PitchStick = TEXT("Throwable.PitchJar.Stick");
	/** Over the salt burst: hot pitch thrown about, a wet tearing splatter and its sizzle. */
	inline constexpr const TCHAR* PitchBurst = TEXT("Throwable.PitchJar.Burst");
	/** Over the salt burst: the tin splitting and the small jars whirring away with a clatter. */
	inline constexpr const TCHAR* ScatterBurst = TEXT("Throwable.ScatterJar.Burst");
	/** One small jar popping: a short, sharp crack with a puff of salt. */
	inline constexpr const TCHAR* ScatterPop = TEXT("Throwable.ScatterJar.Pop");
	/** Over the salt burst: the coals catching with a deep whoomp and a roar of flame. */
	inline constexpr const TCHAR* CinderBurst = TEXT("Throwable.CinderJar.Burst");
	/** The cinder bed burning (a loop): crackling coals, a low flutter of flame, pops of sap and grit. */
	inline constexpr const TCHAR* CinderBed = TEXT("Throwable.CinderJar.Bed");
	/** 2D: a new kind of jar taken up in place of the old: a tin set in a bandolier loop and its lid twisted shut. */
	inline constexpr const TCHAR* Swap = TEXT("Throwable.Jar.Swap");
}

/** The kinds' plain rules (numbers in, numbers out): names, looks, tunings, the drop roll, the carry rule. */
namespace GrenadeKinds
{
	inline constexpr int32 Num = 4;

	/** Pitch, scatter or cinder: a kind found as loot, which the carry rule swaps to. */
	AI_LOOTER_SHOOTER_API bool IsSpecial(EGrenadeKind Kind);

	/** A saved byte as a kind (anything unknown, an old save's 0 too: salt). */
	AI_LOOTER_SHOOTER_API EGrenadeKind FromSaved(uint8 Saved);

	/** "Grave salt", "Pitch jar", "Scatter jar", "Cinder jar". */
	AI_LOOTER_SHOOTER_API FText Name(EGrenadeKind Kind);

	/** The HUD's word for it: "SALT", "PITCH", "SCATTER", "CINDER". */
	AI_LOOTER_SHOOTER_API FText Tag(EGrenadeKind Kind);

	/** What it does, in a line (the vendor's row, a pickup's line). */
	AI_LOOTER_SHOOTER_API FText Blurb(EGrenadeKind Kind);

	/** Its interface colour (LooterUI::Color): salt's pale cyan-white, pitch's amber, scatter's verdigris green, cinder's ember. */
	AI_LOOTER_SHOOTER_API FLinearColor UiColor(EGrenadeKind Kind);

	/** Its model (/Game/Art/Throwables/SM_<SaltGrenade|PitchJar|ScatterJar|CinderJar>, from Art/Models/Throwables). */
	AI_LOOTER_SHOOTER_API const TCHAR* MeshPath(EGrenadeKind Kind);

	/** The kind's burst: Base (the salt jar's) times its shares; its share of a boss's cap (a jar that bursts again splits it). */
	AI_LOOTER_SHOOTER_API FGraveSaltTuning BurstTuning(EGrenadeKind Kind, const FGraveSaltTuning& Base, const FGrenadeKindTuning& Kinds);

	/** One of the scatter jar's small jars' bursts: weaker, smaller, no knock, a share of the cap. */
	AI_LOOTER_SHOOTER_API FGraveSaltTuning BroodTuning(const FGraveSaltTuning& Base, const FGrenadeKindTuning& Kinds);

	/** The cinder bed's share of a boss's health, all its burning together (the jar's burst takes the other half). */
	AI_LOOTER_SHOOTER_API float BedBossCap(const FGraveSaltTuning& Base);

	/** How the kind's burst (or a small jar's, bBrood) looks and sounds. */
	AI_LOOTER_SHOOTER_API FGrenadeBurstLook LookOf(EGrenadeKind Kind, bool bBrood = false);

	/**
	 * A dropped tin's kind for a player of PlayerLevel: salt below the tuning's FromLevel; then a special one when
	 * RollSpecial (0-1) is under its SpecialShare, which one by RollWhich (0-1) over the kinds' weights; else salt.
	 */
	AI_LOOTER_SHOOTER_API EGrenadeKind RollDrop(int32 PlayerLevel, float RollSpecial, float RollWhich, const FGrenadeKindTuning& Kinds);

	/**
	 * A tin of Found holding Amount jars taken by a player carrying Count of Carried (at most Max): a special kind becomes
	 * the kind carried (the count kept), salt keeps the kind; as many of its jars as fit go in.
	 */
	AI_LOOTER_SHOOTER_API FGrenadeCollect Collect(EGrenadeKind Carried, int32 Count, EGrenadeKind Found, int32 Amount, int32 Max);

	/**
	 * The scatter's Count small jars' launches off a burst whose surface faces Up: spread evenly round (a random start,
	 * a little jitter each), Speed across and Lift up it, each between 85% and 115%.
	 */
	AI_LOOTER_SHOOTER_API void BroodVelocities(int32 Count, const FVector& Up, float Speed, float Lift, FRandomStream& Random, TArray<FVector>& OutVelocities);

	/** Small jar Index of Count's fuse: spread from FuseMin to FuseMax so they pop one after another (Roll 0-1 jitters it a little). */
	AI_LOOTER_SHOOTER_API float BroodFuse(int32 Index, int32 Count, float Roll, const FGrenadeKindTuning& Kinds);

	/**
	 * The cinder bed's burn over Seconds on a body at LevelScale (the thrower's), times Affinity (the Ember weakness
	 * table's: 1.75 weak, 0.5 resists), at Roll (0-1) in the +/-10% spread.
	 */
	AI_LOOTER_SHOOTER_API float BedDamage(float Seconds, float LevelScale, float Affinity, float Roll, const FGraveSaltTuning& Base,
		const FGrenadeKindTuning& Kinds);

	/** Damage cut to what's left of a boss's cap (CapShare of MaxHealth) after AlreadyDealt. */
	AI_LOOTER_SHOOTER_API float CapLeft(float Damage, float AlreadyDealt, float MaxHealth, float CapShare);
}
