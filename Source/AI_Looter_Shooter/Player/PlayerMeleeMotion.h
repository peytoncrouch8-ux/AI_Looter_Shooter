#pragma once

#include "CoreMinimal.h"
#include "Player/ViewKick.h"

/** Where a swing has the gun: off its hold in its own frame (cm; X along the barrel, Y right, Z up) and turned (degrees). */
struct FMeleeSwingPose
{
	FVector Offset = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
};

/**
 * How a melee strike moves, done in code like recoil (no animation assets): the gun's swing in first person and the
 * view's lean into the blow, through the shared camera modifier (UCameraShakeModifier) so the player's camera shake
 * setting scales it and the aim never moves.
 *
 * The gun's stock strike, over FMeleeRules' 0.45 s: a quick cock back and to the right (0.05 s), the drive across (the
 * muzzle swung right and canted so the stock leads forward and left into the middle of the view, accelerating into the
 * blow at 0.12 s), then back to the hold. A blow that lands stops dead there (the striker's hit-stop holds the clock) and
 * comes straight back; a miss carries on past it a little before it recovers.
 *
 * A fist has no model yet (the first-person arms are hidden), so its strike is the view's alone: a short pull back, then
 * a punch in (the view narrowing) with a dip toward the blow.
 */
namespace MeleeMotion
{
	/** When the gun is fully cocked back, before it drives across (seconds into the swing). */
	inline constexpr float CockSeconds = 0.05f;
	/** How far a miss carries on past the strike pose (a share of it), and how soon it has (seconds after the blow). */
	inline constexpr float FollowThrough = 0.25f;
	inline constexpr float FollowThroughSeconds = 0.08f;

	/** The cocked pose and the strike pose (the moment of the blow), in the gun's frame. */
	AI_LOOTER_SHOOTER_API FMeleeSwingPose CockedPose();
	AI_LOOTER_SHOOTER_API FMeleeSwingPose StrikePose();

	/**
	 * The gun SwingTime seconds into a strike (the swing's own clock: it holds while the blow's hit-stop does). bLanded:
	 * the blow met a body (or a wall) and stopped dead, so there's no follow-through. Rest (zero) before and after.
	 */
	AI_LOOTER_SHOOTER_API FMeleeSwingPose GunPose(float SwingTime, bool bLanded);

	/** The view as the swing starts: drawn back and turned a little away from the blow to come (a fist's, a short pull back). */
	AI_LOOTER_SHOOTER_API FViewKick WindUp(bool bArmed);

	/** The view at the blow, hit or miss: leaning into the swing's way across (a fist's, a punch in with a dip). */
	AI_LOOTER_SHOOTER_API FViewKick Strike(bool bArmed);

	/**
	 * The blow landing on a body: a sharp jolt back off it and a punch in, rolled a little Lean's way (-1 or 1). The
	 * killing blow is 30% more.
	 */
	AI_LOOTER_SHOOTER_API FViewKick Impact(bool bKill, float Lean);

	/** The stock or the knuckles on a wall: a smaller jolt. */
	AI_LOOTER_SHOOTER_API FViewKick WallKnock();

	/** A landed blow's shake (UCameraShakeModifier::AddShake): its strength and seconds; a killing blow's is stronger. */
	inline constexpr float ImpactShake = 0.2f;
	inline constexpr float KillShake = 0.3f;
	inline constexpr float ShakeSeconds = 0.15f;

	/** A fist is lighter than a stock: its whoosh and thud play this much higher. */
	inline constexpr float FistSoundPitch = 1.1f;
	/** A knock on a wall is heard at this share of a blow's volume. */
	inline constexpr float WallVolume = 0.5f;
}
