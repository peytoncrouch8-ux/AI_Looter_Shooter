#pragma once

#include "CoreMinimal.h"

/**
 * The player character's size and speed. The animations, the first-person rig, the cameras and the speeds were all made
 * for the full-size mannequin. The character is that mannequin scaled down as one actor (ALooterCharacter), and its
 * speeds are the full-size ones scaled the same way, so each step still covers the ground the animation's stride does;
 * then the whole pace is slowed a little on top (PaceScale), which the blend spaces and the sprint's play rate absorb.
 */
namespace LooterPlayerSize
{
	/** The user's call (2026-10-07): at full size the player looked too big for the world. */
	inline constexpr float Scale = 0.85f;

	/** The speeds that go with the size alone: kept equal to Scale so the strides match the animations. */
	inline constexpr float SpeedScale = Scale;

	/**
	 * The user's call (2026-10-13, "pace of play is super fast"): walking, sprinting, crouching, sliding and walking with
	 * the sights up all 10% slower, apart from the size, so the size never has to change for it.
	 */
	inline constexpr float PaceScale = 0.9f;

	/**
	 * What every player speed is against its full-size tuning: the size and the pace together. Creatures whose chase was
	 * tuned against the player's speed use it too, so a chase keeps the same odds (a walking player still just outpaces a
	 * spider); the slimes keep their own pace.
	 */
	inline constexpr float MoveScale = SpeedScale * PaceScale;

	/** Full-size walking (the jog the locomotion blend spaces top out at) and crouched walking, before MoveScale (cm/s). */
	inline constexpr float FullSizeWalkSpeed = 600.f;
	inline constexpr float FullSizeCrouchSpeed = 300.f;
}
