#pragma once

#include "CoreMinimal.h"
#include "Player/FirstPersonHandsTypes.h"

/**
 * The support hand's work away from the foregrip, in camera space (X ahead, Y right, Z up; cm), each on the clock of
 * what it works so it can never drift off it: the gun's reload choreography (LooterReload: its progress, its parts'
 * travel, its sound steps) and the grenade throw's (ThrowMotion: the jar's pose).
 *
 * Reloads (FirstPersonHandsReload.cpp):
 *  - Magazine (the Bullpup): onto the magazine, riding it out of the well, letting it fall, down to the pouch for a
 *    fresh one, riding that up into the well, then the charging handle (a yank back) and home to the handguard.
 *  - Pump (the Ranchhand): down to the shell belt (low on the right, off the view) and up to the loading port, a shell
 *    thumbed in on each of the reload's shell steps, then onto the pump as it's racked back and forward.
 *  - Cylinder (the Drover): cradling the cylinder as it swings out, the palm punching the ejector rod, down to the
 *    pouch for the speedloader and in behind the open chambers, then batting the cylinder home from its left.
 *
 * The throw (FirstPersonHandsThrow.cpp): off the gun and round the jar (ThrowMotion::JarPose) through the draw and the
 * sling, open past the release, back to the gun as it comes up.
 */
namespace FirstPersonHandsMotion
{
	/** The belt's pouches (camera space): low on the left for magazines and speedloaders, low on the right for shells. */
	AI_LOOTER_SHOOTER_API FVector MagazinePouch();
	AI_LOOTER_SHOOTER_API FVector ShellBelt();

	/** The support hand at Gun.ReloadProgress (on the foregrip when it isn't reloading). */
	AI_LOOTER_SHOOTER_API FHandPlacement SupportInReload(const FHandsGun& Gun);

	/** The spots the reloads work, for the paths and the tests (camera space). Travel: the magazine's, as LooterReload has it. */
	AI_LOOTER_SHOOTER_API FHandPlacement OnMagazine(const FHandsGun& Gun, float Travel);
	/** On the charging handle, yanked back Yank (0-1) of its pull. */
	AI_LOOTER_SHOOTER_API FHandPlacement OnHandle(const FHandsGun& Gun, float Yank);
	/** Under the loading port with a shell, pushed Push (0-1) of the way in. */
	AI_LOOTER_SHOOTER_API FHandPlacement AtLoadingPort(const FHandsGun& Gun, float Push);
	/** Cradling the cylinder from its left, as it is now. */
	AI_LOOTER_SHOOTER_API FHandPlacement OnCylinder(const FHandsGun& Gun);
	/** The palm on the ejector rod's tip, Punch (0-1) of the way through its stroke. */
	AI_LOOTER_SHOOTER_API FHandPlacement OnEjectorRod(const FHandsGun& Gun, float Punch);
	/** The speedloader behind the open cylinder, pushed Push (0-1) of the way into the chambers. */
	AI_LOOTER_SHOOTER_API FHandPlacement WithSpeedloader(const FHandsGun& Gun, float Push);
	/** The flat of the hand on the cylinder's left, batting it home. */
	AI_LOOTER_SHOOTER_API FHandPlacement BattingCylinder(const FHandsGun& Gun);

	/** The support hand ThrowTime seconds into a throw; OnGun: where it holds the gun now (its dip included). */
	AI_LOOTER_SHOOTER_API FHandPlacement SupportInThrow(const FHandPlacement& OnGun, float ThrowTime);

	/** The hand round the jar ThrowTime seconds into a throw, where ThrowMotion::JarPose has the jar (camera space). */
	AI_LOOTER_SHOOTER_API FHandPlacement OnJar(float ThrowTime);
}
