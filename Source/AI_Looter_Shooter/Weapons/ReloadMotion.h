#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

/** A moment of a reload that makes a sound. */
enum class EReloadStep : uint8
{
	/** The old magazine comes free. */
	MagOut,
	/** The fresh one is slapped home. */
	MagIn,
	/** The charging handle. */
	Bolt,
	/** A shell pushed into the tube. */
	ShellIn,
	/** The pump racked. */
	Pump
};

/** Where in a reload (its progress, 0 to 1) a step happens. */
struct FReloadStepAt
{
	float Progress;
	EReloadStep Step;
};

/**
 * One reload, choreographed over its progress (0 = started, 1 = done). The weapon moves its part from this and the
 * first-person view moves the whole gun from it, so the two always line up whatever the weapon's reload time.
 */
namespace LooterReload
{
	/**
	 * The reload's sounds where its motion makes them, in order: the magazine out, the fresh one seated, the charging
	 * handle; or each shell pushed in, then the pump. Empty for None.
	 */
	AI_LOOTER_SHOOTER_API TConstArrayView<FReloadStepAt> Steps(EWeaponReloadPart Part);

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
