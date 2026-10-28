#pragma once

#include "CoreMinimal.h"

class AWeaponBase;

/**
 * How a gun maker's shot differs from its kind's (the user's pick, 2026-10-28: Roundhouse heavy, Saint Maud ringing,
 * Windfall rattly, Meridian clean). GunshotSound::PlayShot shapes the kind's layers by these, so a rifle still sounds like
 * a rifle and the player hears who made it. Ones and zeros change nothing (FMakerSoundFlavor{} is the plain kind).
 */
struct FMakerSoundFlavor
{
	/** The shot's pitch (Core, Mech, Low, Distant) times this: under 1 bigger and heavier, over 1 brighter. */
	float Pitch = 1.f;
	/** The tail's pitch, on top of Pitch (a brighter ring, a duller roll). */
	float TailPitch = 1.f;
	/** Each layer's volume times these: Core the punch, Mech the action, Low the chest thump, Tail the land's answer. */
	float Core = 1.f;
	float Mech = 1.f;
	float Low = 1.f;
	float Tail = 1.f;
	/** Mech's pitch moves up to this much either way each shot (0.07 = 7%): a loose action never sounds the same twice. */
	float MechJitter = 0.f;
	/** The maker's own layer (Saint Maud's barrel ring, Windfall's loose rattle) at this volume; 0 = none. */
	float Extra = 0.f;
};

/** The makers' own layers (Art/Sounds/recipes/guns_makers.py). The shooter's own gun only; nothing plays until the bank has them. */
namespace MakerSoundCue
{
	/** Saint Maud's cast barrel ringing on after the shot. */
	inline constexpr const TCHAR* SaintMaudRing = TEXT("Weapon.Maker.SaintMaud.Ring");
	/** Windfall's bolted-together bits rattling as the action cycles. */
	inline constexpr const TCHAR* WindfallRattle = TEXT("Weapon.Maker.Windfall.Rattle");
}

/**
 * The makers' sound flavours as shipped and as tuned live: cvars Looter.Gunfeel.Sound.Maker.<Key>.Pitch, .TailPitch,
 * .Core, .Mech, .Low, .Tail, .MechJitter, .Extra (made from the table when the module loads), and
 * Looter.Gunfeel.Sound.Maker.Scale [1] (0 = every maker plain, for comparisons; 2 = twice as far from plain). Read when
 * asked, so a change is heard on the next shot.
 */
namespace MakerSoundFlavor
{
	/** A maker's flavour as shipped (the cvars start from these) and its own layer's cue (null: none). */
	struct FShipped
	{
		const TCHAR* Key;
		FMakerSoundFlavor Flavor;
		const TCHAR* ExtraCue;
	};

	/** Every maker's shipped flavour, in WeaponMakers' order. */
	AI_LOOTER_SHOOTER_API TConstArrayView<FShipped> Shipped();
	/** The maker with this key's shipped entry, or null. */
	AI_LOOTER_SHOOTER_API const FShipped* ShippedFor(FName Key);
	/** The shipped flavour for Key: plain for None or a key the table doesn't know. */
	AI_LOOTER_SHOOTER_API FMakerSoundFlavor ShippedFlavor(FName Key);

	/** Looter.Gunfeel.Sound.Maker.Scale (never negative). */
	AI_LOOTER_SHOOTER_API float Scale();
	/** The live flavour for Key (its cvars) times Scale; plain for None or an unknown key. */
	AI_LOOTER_SHOOTER_API FMakerSoundFlavor For(FName Key);
	/** The live flavour of Weapon's maker: plain for a named gun or one without a maker. */
	AI_LOOTER_SHOOTER_API FMakerSoundFlavor Of(const AWeaponBase& Weapon);
	/** The maker's own layer's cue (NAME_None: none), for Key or Weapon's maker. */
	AI_LOOTER_SHOOTER_API FName ExtraCueFor(FName Key);
	AI_LOOTER_SHOOTER_API FName ExtraCueOf(const AWeaponBase& Weapon);

	// --- The rules (plain functions, for the tests) ---

	/**
	 * Flavor moved toward plain by Scale: 0 plain, 1 as it is, 2 twice as far. Volumes never go under silence, pitches
	 * stay between 0.5 and 2, the jitter between 0 and 0.3.
	 */
	AI_LOOTER_SHOOTER_API FMakerSoundFlavor Scaled(const FMakerSoundFlavor& Flavor, float InScale);
	/** Whether Flavor changes nothing. */
	AI_LOOTER_SHOOTER_API bool IsPlain(const FMakerSoundFlavor& Flavor);
	/** The Mech layer's pitch for a roll of 0..1 (0.5 = Pitch itself). */
	AI_LOOTER_SHOOTER_API float MechPitch(const FMakerSoundFlavor& Flavor, float Roll);
	/** The tail's pitch: Pitch x TailPitch. */
	AI_LOOTER_SHOOTER_API float TailPitchOf(const FMakerSoundFlavor& Flavor);
}
