#pragma once

#include "CoreMinimal.h"
#include "Player/ViewKick.h"

/** Something held off its rest by a throw: moved (cm) and turned (degrees), in its own frame. */
struct FThrowPose
{
	FVector Offset = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
};

/**
 * How a grenade throw moves, done in code like the melee strike (no animation assets): the jar in the off hand, the gun
 * dipping out of its way, and the view's lean, through the shared camera modifier (UCameraShakeModifier) so the camera
 * shake setting scales it and the aim never moves.
 *
 * Over FThrowRules' 0.5 s: the jar comes up from under the view on the left (the off hand; the first-person arms are
 * hidden, so the jar is what's seen), draws back by the cheek (0.1 s), and is slung forward and in, leaving the hand at
 * 0.16 s just where the real grenade starts its flight (FThrowRules::ReleasePoint), which carries it on up toward the
 * middle. Meanwhile the gun dips down and to the right with a cant, out of the throw's way: all the way down (0.08 s)
 * before the jar is up, held through the release and back up to its hold by the end. The third-person body follows the
 * same clock (LooterStancePose::DescribeThrow, UHandJarComponent).
 */
namespace ThrowMotion
{
	/** The jar fully drawn back, before it's slung (seconds into the throw). */
	inline constexpr float DrawSeconds = 0.1f;
	/** The gun is all the way down from here to here, then comes back up by the throw's end. */
	inline constexpr float DipInSeconds = 0.08f;
	inline constexpr float DipHoldSeconds = 0.26f;

	/** The jar's key poses off the first-person camera (X ahead, Y right, Z up; cm and degrees). */
	AI_LOOTER_SHOOTER_API FThrowPose JarStartPose();
	AI_LOOTER_SHOOTER_API FThrowPose JarDrawnPose();
	AI_LOOTER_SHOOTER_API FThrowPose JarReleasePose();

	/**
	 * The jar is in the off hand ThrowTime seconds in: from the press until it leaves the hand at the release (the first
	 * person's jar under the view and the third person's in the body's hand alike).
	 */
	AI_LOOTER_SHOOTER_API bool IsJarInHand(float ThrowTime);

	/**
	 * The jar off the first-person camera ThrowTime seconds in, and whether it shows (it rises from out of sight below
	 * the view and leaves the hand at the release).
	 */
	AI_LOOTER_SHOOTER_API FThrowPose JarPose(float ThrowTime, bool& bOutShown);

	/** The gun's deepest dip, in its own frame (X along the barrel, Y right, Z up), turned about its hands. */
	AI_LOOTER_SHOOTER_API FThrowPose GunDipPose();

	/** How far into its dip the gun is ThrowTime seconds in (0 in its hold, 1 all the way down): easing down, held, easing back up. */
	AI_LOOTER_SHOOTER_API float GunDipShare(float ThrowTime);

	/** The gun ThrowTime seconds into a throw: easing down, held, easing back up; rest (zero) before and after. */
	AI_LOOTER_SHOOTER_API FThrowPose GunPose(float ThrowTime);

	/** The view as the jar is drawn back: a little up and toward the throwing side. */
	AI_LOOTER_SHOOTER_API FViewKick WindUp();

	/** The view as the jar leaves the hand: leaning into the throw, a touch narrower. */
	AI_LOOTER_SHOOTER_API FViewKick Release();

	/**
	 * A burst felt Share (0-1, FGraveSaltRules::KickShare: by distance) of its full weight: a hard jolt up and a punch in,
	 * rolled Lean's way (-1 or 1). Nothing at 0.
	 */
	AI_LOOTER_SHOOTER_API FViewKick Burst(float Share, float Lean);

	/** A burst's shake (UCameraShakeModifier::AddShake) at full weight, and its seconds. */
	inline constexpr float BurstShake = 0.55f;
	inline constexpr float BurstShakeSeconds = 0.4f;
}
