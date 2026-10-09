#pragma once

#include "CoreMinimal.h"

/** The moves that carry the player over things (UPlayerLocomotionComponent, PlayerLocomotionTraversal.cpp). */
enum class ETraversalKind : uint8
{
	None,
	/** Up onto a ledge or a wall's top: the body rises, then comes forward over the edge onto it. */
	Mantle,
	/** Over a low, thin obstacle (a fence rail, a low wall) to the floor beyond, in one hop that keeps the run's speed. */
	Vault,
	/** A short glide out of a spot the player got wedged in (hung on rocks or props, neither falling nor walking). */
	Unstick,
};

/**
 * The traversal rules: how high, how far, how long. Heights are world cm at the player's size (0.85 of the mannequin),
 * measured from the feet. Pure numbers and formulas, so they are unit tested.
 */
namespace LooterTraversal
{
	/** An actor (or one of its components) tagged this is never climbed or vaulted: set dressing, fragile or story props. */
	inline FName NoClimbTag() { return FName(TEXT("NoClimb")); }

	/** The user's ask (2026-10-08): ledges and wall tops up to about 1.5 m over the feet are climbed. */
	inline constexpr float MaxMantleHeight = 150.f;
	/** In the air a ledge is caught from this far over the feet; anything lower the landing takes care of. */
	inline constexpr float MinAirHeight = 20.f;
	/** Obstacles up to this high are vaulted (a fence, a low wall, a grave)... */
	inline constexpr float MaxVaultHeight = 110.f;
	/** ...and up to this thick along the run. */
	inline constexpr float MaxVaultDepth = 90.f;
	/** A top at least this deep can be stood on (the body's middle over it); a rail or a plank is only vaulted. */
	inline constexpr float MinStandDepth = 25.f;
	/** A vault may come down this far below where it started (a fence on a slope); a bigger drop is a ledge, not a vault. */
	inline constexpr float MaxVaultDrop = 80.f;

	/**
	 * How far past the body's surface a ledge is reached for: a step's worth standing, more the faster the player goes. A
	 * vault reaches further, so one taken at a sprint is a hurdle with the fence in its middle, not a stop at the rail.
	 */
	inline constexpr float MantleReach = 35.f;
	inline constexpr float MantleReachPerSpeed = 0.05f;
	inline constexpr float AirReach = 30.f;
	inline constexpr float VaultReach = 35.f;
	inline constexpr float VaultReachPerSpeed = 0.12f;
	/** The ledge must face the player: within this many degrees of the way they go, and of the way they look. */
	inline constexpr float HeadingDegrees = 55.f;
	inline constexpr float LookDegrees = 70.f;
	/** A run at least this share of the walk (a sprint) is vaulted over a low wall rather than climbed onto it. */
	inline constexpr float VaultRunShare = 1.15f;

	/** Seconds after walking off an edge that the jump key still jumps, and before landing that a press still counts. */
	inline constexpr float CoyoteSeconds = 0.12f;
	inline constexpr float JumpBufferSeconds = 0.15f;
	/** Hung in the air this long within this far of one spot (cm) is wedged: the player is nudged free. */
	inline constexpr float StuckSeconds = 0.6f;
	inline constexpr float StuckRadius = 6.f;
	inline constexpr float UnstickSeconds = 0.25f;

	/** How far under the top the body's bottom may pass in a vault (the legs tuck); a mantle barely grazes the edge. */
	inline constexpr float VaultTuck = 30.f;
	inline constexpr float MantleTuck = 6.f;
	/** The eye stays this far over the obstacle's top while it's over it, so the view never dips into the wall. */
	inline constexpr float EyeClearance = 15.f;
	/** The eye ends this far under standing (a knee on the ledge; a vault's landing giving) and rises back after. */
	inline constexpr float MantleEyeDip = 10.f;
	inline constexpr float VaultEyeDip = 8.f;
	/** A vault lifts the eye this much over its path, the hop. */
	inline constexpr float VaultEyeHop = 6.f;

	/** A mantle's length for how far the feet rise (cm): 0.4 s for a 1.2 m ledge, quicker for a lip. */
	inline float MantleSeconds(float Rise) { return FMath::Clamp(0.18f + 0.0018f * Rise, 0.2f, 0.45f); }
	/** A vault's length for how far the feet rise to the top of the hop (cm): 0.4 s over a 1 m fence. */
	inline float VaultSeconds(float ApexRise) { return FMath::Clamp(0.24f + 0.0022f * ApexRise, 0.3f, 0.46f); }
}
