#pragma once

#include "CoreMinimal.h"
#include "Player/ViewKick.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponTypes.h"

class AWeaponBase;

/** What a reload step is felt as: a tiny kick of the view and a tick of gamepad rumble. All zeros = nothing. */
struct FReloadBeat
{
	/** The view's jolt (only Pitch, Roll, FieldOfView, Frequency and Damping are used). */
	FViewKick Kick;
	/** Gamepad rumble strength (0-1, before the player's setting) and how long (s). */
	float Rumble = 0.f;
	float RumbleSeconds = 0.f;

	bool IsNone() const
	{
		return FMath::IsNearlyZero(Kick.Pitch) && FMath::IsNearlyZero(Kick.Roll) && FMath::IsNearlyZero(Kick.FieldOfView) && Rumble <= 0.f;
	}
};

/**
 * The felt beats of a reload (gun identity, 2026-10-28, Docs/Work/2026-10-28-Core/GUNS.md): the mag seating, the bolt
 * slamming, the cylinder snapping home are no longer sound only. AWeaponBase::PlayReloadSteps calls Play where a step's
 * sound plays; the local player's view takes a small kick (UCameraShakeModifier) and a gamepad a short tick. Each beat is a
 * fraction of a degree, so the reload reads as hands working, never as a second recoil. Nothing for other holders, through
 * the sight (a reload lowers it anyway), with Looter.Gunfeel.Identity 0, or Looter.Gunfeel.Reload.Beats 0.
 */
namespace ReloadBeats
{
	/** A beat plays only while the sight is lower than this (AimAlpha); aimed, it is none. */
	inline constexpr float AimedCutoff = 0.25f;

	/** The beat for a step of a gun of Kind at full strength (the shipped table; none for steps without one). */
	AI_LOOTER_SHOOTER_API FReloadBeat For(EWeaponKind Kind, EReloadStep Step);

	/** Looter.Gunfeel.Reload.Beats [1]: the beats' strength (kick and rumble); 0 = none. Never negative. */
	AI_LOOTER_SHOOTER_API float Scale();

	/** The plain rule: a beat plays with the identity switch on, a Scale above 0, and the sight down (AimAlpha under AimedCutoff). */
	AI_LOOTER_SHOOTER_API bool Plays(bool bIdentity, float BeatScale, float AimAlpha);

	/** Plays Weapon's beat for Step on its local player (view kick and rumble); logs it with Looter.Gunfeel.Reload.Debug 1. */
	AI_LOOTER_SHOOTER_API void Play(const AWeaponBase& Weapon, EReloadStep Step);
}
