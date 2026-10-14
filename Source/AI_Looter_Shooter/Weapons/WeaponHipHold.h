#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

/**
 * Where each gun family sits at the hip in first person (the user's call, 2026-10-14: raised like Borderlands', so both
 * gloves and the support hand's reload work stay in view): its grip's spot off the eye (camera space: X ahead, Y right,
 * Z up; cm) and its turn. The first-person view model draws at its own fixed field of view, so the field of view setting
 * never moves it. Aiming down the sights doesn't use it (PlayerLocomotionViewModel.cpp slerps to the sight line).
 *
 * A definition's own FirstPersonGrip (the Drover's, from Tools/Unreal/create_revolver.py) wins over this table.
 */
namespace WeaponHipHold
{
	/** The hip hold for a gun family; false for a kind the table doesn't have (the weapon manager's AttachOffset then). */
	AI_LOOTER_SHOOTER_API bool For(EWeaponKind Kind, FVector& OutGrip, FRotator& OutTurn);

	/**
	 * Development only, for tuning from screenshots without a build: the Looter.HipHold.Ahead / .Right / .Up (cm) and
	 * .Pitch / .Yaw / .Roll (degrees) console variables, added to the hip hold of the kind Looter.HipHold.Kind picks (0
	 * every gun, 1 rifle, 2 shotgun, 3 revolver). Nothing in a shipping build. Looter.Hands.Dump prints them.
	 */
	AI_LOOTER_SHOOTER_API FVector TuningNudge(EWeaponKind Kind);
	AI_LOOTER_SHOOTER_API FRotator TuningTurn(EWeaponKind Kind);
}
