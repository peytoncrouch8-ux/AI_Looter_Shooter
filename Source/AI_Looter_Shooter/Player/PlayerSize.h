#pragma once

#include "CoreMinimal.h"

/**
 * The player character's size and speed. The animations, the first-person rig, the cameras and the speeds were all made
 * for the full-size mannequin. The character is that mannequin scaled down as one actor (ALooterCharacter), and its
 * speeds are the full-size ones scaled the same way, so each step still covers the ground the animation's stride does.
 */
namespace LooterPlayerSize
{
	/** The user's call (2026-10-07): at full size the player looked too big for the world. */
	inline constexpr float Scale = 0.85f;

	/** Every movement speed against its full-size tuning. Kept equal to Scale so the strides match the animations. */
	inline constexpr float SpeedScale = Scale;

	/** Full-size walking (the jog the locomotion blend spaces top out at) and crouched walking, before SpeedScale (cm/s). */
	inline constexpr float FullSizeWalkSpeed = 600.f;
	inline constexpr float FullSizeCrouchSpeed = 300.f;
}
