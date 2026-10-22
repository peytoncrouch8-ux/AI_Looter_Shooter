#pragma once

#include "CoreMinimal.h"
#include "Player/Animation/HeroPoseTypes.h"
#include "Weapons/WeaponTypes.h"

/**
 * The heroes' stances and third-person aim as the stance layer takes them (ULooterCharacterAnimInstance reads these on the
 * game thread and copies the answers into FLooterStanceInput): the hero's data (FHeroIdle, FHeroAim) with the
 * Looter.Pose.* console variables on it in a development build, and the numbers the aim shares between kinds of gun.
 * Looter.Pose.Set and Looter.Pose.Dump (Dev/PoseDevCommands.cpp) change and print a hero's data live.
 */
namespace HeroPoseTuning
{
	/**
	 * The idle the stance layer plays for Style: Looter.Pose.On 0 gives Ellis's weary idle with the animation's open hands
	 * (every hero as before 2026-10-22, to compare); Looter.Pose.Curl scales the hands' rest curl.
	 */
	AI_LOOTER_SHOOTER_API FHeroIdle Idle(const FHeroIdle& Style);

	/** The aim for Style: Looter.Pose.Aim 0 keeps the hip hold through a sight (Raise 0); Looter.Pose.Aim.Head scales the head's part. */
	AI_LOOTER_SHOOTER_API FHeroAim Aim(const FHeroAim& Style);

	/**
	 * How far ahead of the eye the gun's grip sits along the aim when sighted (cm on the full-size body; the stock in the
	 * shoulder, the six-gun out in both hands): DefaultGripAhead, or Looter.Pose.Aim.Grip.<Kind> when set (not 0).
	 */
	AI_LOOTER_SHOOTER_API float GripAhead(EWeaponKind Kind);
	AI_LOOTER_SHOOTER_API float DefaultGripAhead(EWeaponKind Kind);

	/**
	 * The mannequin's right eye from its head bone (cm: ahead, right, up, in the body's frame as the aim pitches it), where
	 * the sight lines up: DefaultEyeFromHead, or the Looter.Pose.Aim.Eye* tuning on it.
	 */
	AI_LOOTER_SHOOTER_API FVector EyeFromHead();
	AI_LOOTER_SHOOTER_API FVector DefaultEyeFromHead();
}
