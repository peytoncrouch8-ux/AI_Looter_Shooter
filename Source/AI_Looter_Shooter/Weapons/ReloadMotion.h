#pragma once

#include "CoreMinimal.h"

/** The part of a gun a reload visibly works on. */
enum class EWeaponReloadPart : uint8
{
	None,
	/** Rifles: the old magazine slides out and drops away, a fresh one goes in, then the charging handle. */
	Magazine,
	/** Shotguns: shells are pushed in one at a time, then the pump is racked. */
	Pump,
};

/**
 * One reload, choreographed over its progress (0 = started, 1 = done). The weapon moves its part from this and the
 * first-person view moves the whole gun from it, so the two always line up whatever the weapon's reload time.
 */
namespace LooterReload
{
	/**
	 * How far the magazine is out of its well (cm, along the well's axis), and whether it can be seen: the old one
	 * drops out of sight before the fresh one comes up from below.
	 */
	AI_LOOTER_SHOOTER_API float MagazineTravel(float Progress, bool& bOutVisible);

	/** How far the pump is pulled back (cm). */
	AI_LOOTER_SHOOTER_API float PumpTravel(float Progress);

	/** The first-person gun's motion on top of its hold pose, in camera space (X forward, Y right, Z up; degrees). */
	AI_LOOTER_SHOOTER_API void ViewModelPose(EWeaponReloadPart Part, float Progress, FVector& OutOffset, FRotator& OutRotation);
}
