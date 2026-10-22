#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

class AWeaponBase;

/** One layer of a gunshot as heard (Gunfeel step 2; Saved/Gunfeel/CONTRACT.md). */
enum class EGunshotLayer : uint8
{
	/** The whole shot as one sound: today's Weapon.<Kind>.Fire. */
	Fire,
	/** The action cycling, close to the ear: the shooter's own gun only. */
	Mech,
	/** The low thump in the chest: the shooter's own gun only. */
	Low,
	/** What the land answers with, by the shooter's surroundings. */
	TailOpen,
	TailTown,
	TailInside,
	/** A shot heard from far away (another shooter's). */
	Distant,
	/** The rising click of the magazine's last quarter. */
	LowAmmo,
};

/**
 * What a gun's shot sounds like: AWeaponBase::FireShot calls PlayShot for every shot. One place, so the shot's layers
 * (the near mechanism, the punch, the low thump, a tail by the surroundings, a distant version) are chosen together.
 * Until GUNFEEL-SOUND fills the layers in, PlayShot plays the kind's one Fire cue (or the definition's own FireSound), as
 * before. A layer's cue is "Weapon.<Kind>.<Layer>" (Weapon.Rifle.Mech, Weapon.Shotgun.TailTown, ...); a cue with no
 * sounds plays nothing.
 */
namespace GunshotSound
{
	/** The cue for Kind's Layer (Fire: WeaponSounds::Fire). */
	AI_LOOTER_SHOOTER_API FName CueFor(EWeaponKind Kind, EGunshotLayer Layer);

	/** Plays Weapon's shot. */
	AI_LOOTER_SHOOTER_API void PlayShot(const AWeaponBase& Weapon);

	/**
	 * After a shot left RoundsLeft of Magazine: the last quarter's click, rising as it empties (none above the quarter, none
	 * for magazines under 4). The share and pitch come from GunfeelKick's cvars (Looter.Gunfeel.LowAmmo.*).
	 */
	AI_LOOTER_SHOOTER_API void PlayLowAmmo(const AWeaponBase& Weapon, int32 RoundsLeft, int32 Magazine);

	/** A revolver's critical hit landing: a bright ring for its shooter (2D). */
	AI_LOOTER_SHOOTER_API void PlayCritRing(const UObject* WorldContext);

	/**
	 * A rocket bursting at Location (the Powderkeg's rocket calls this): the near explosion (crack, blast, debris, the
	 * land's roll), or past Looter.Gunfeel.Sound.ExplosionDistantFrom [6000 cm] from the listener the far boom alone.
	 */
	AI_LOOTER_SHOOTER_API void PlayExplosion(const UObject* WorldContext, const FVector& Location);

	/** Looter.Gunfeel.Sound.ExplosionDistantFrom (cm, never negative). */
	AI_LOOTER_SHOOTER_API float ExplosionDistantFrom();

	/** The plain rule: an explosion Distance cm from the listener plays far past DistantFrom (0 = always near). */
	AI_LOOTER_SHOOTER_API bool IsExplosionDistant(float Distance, float DistantFrom);
}
