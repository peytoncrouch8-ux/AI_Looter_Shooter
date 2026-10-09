#pragma once

#include "CoreMinimal.h"
#include "Player/TraversalRules.h"

struct FPlayerTraversal;

/**
 * The body's pose for a mantle or a vault, as the stance layer poses it: how strongly each part comes in, and where the
 * hands reach, in the mesh's component space (the body's own size, so a smaller player's reach scales with it). Worked out
 * from the move's plan on the game thread (LooterClimbPose::Describe), so the animation thread only has to pose.
 */
struct FLooterClimbInput
{
	/** The legs tucked up and the hands reaching for the obstacle, each 0..1; they come in and go out as the move goes. */
	float Legs = 0.f;
	float Hands = 0.f;
	/** 0..1: the body leaning into the move (full lean is TorsoLean), the locomotion's own eased pose alpha. */
	float Lean = 0.f;
	/** The spine's lean at full pose (degrees, forward), by move; the Anim Blueprint's property sets it. */
	float TorsoLean = 0.f;
	bool bVault = false;
	/**
	 * The way the move goes (horizontal, unit length, in the mesh's component space): the ledge lies along it, which isn't
	 * always the way the body faces (a ledge met on a diagonal). Zero means the body's facing.
	 */
	FVector Heading = FVector::ZeroVector;
	/**
	 * Where the hand takes the obstacle: ahead of the body's axis along Heading, and over its feet (a mantle: the edge
	 * and the top; a vault: the rail or wall's top, a little before its middle).
	 */
	float LedgeAhead = 0.f;
	float LedgeUp = 0.f;

	bool IsActive() const { return Legs > UE_KINDA_SMALL_NUMBER || Hands > UE_KINDA_SMALL_NUMBER || Lean > UE_KINDA_SMALL_NUMBER; }
};

/**
 * The climbing pose's rules: a pure function of the move's plan and the pose alpha the locomotion eases, so it is unit
 * tested. The pose rides the plan, not the clock: the hands hold a point fixed in the world (on the ledge), so as the body
 * rises and comes over, they stay on it, then let go.
 */
namespace LooterClimbPose
{
	/**
	 * The most the locomotion's pose alpha (UPlayerLocomotionComponent::GetTraversalAlpha) reaches in a vault: a vault takes
	 * 0.6 of it (VaultPoseShare in PlayerLocomotionComponent.cpp, which the first-person gun reads). The body's tuck counts
	 * that as full, or a vault would only half tuck its legs. A test holds the two together.
	 */
	inline constexpr float VaultAlphaShare = 0.6f;

	/** How far past a ledge's edge the hands go onto the top (cm), and how far before a vaulted rail's middle (cm). */
	inline constexpr float MantleHandDepth = 10.f;
	inline constexpr float VaultHandBefore = 12.f;
	/** TopOverFeet when the move's start wasn't heard (the anim instance was made mid-move): Describe estimates it from the plan. */
	inline constexpr float UnknownTop = -1.f;

	/**
	 * The pose for the move Plan is under way (or was last) at Alpha, the locomotion's pose alpha. TopOverFeet is the
	 * obstacle's top over the feet as the move started (world cm, or UnknownTop), CapsuleRadius the body's (world cm),
	 * BodyScale the mesh's scale against the full-size mannequin and MeshRotation its component-to-world rotation (the
	 * move's heading goes into its space). Nothing active (a default input) for no move, an unstick glide, or a zero alpha.
	 */
	FLooterClimbInput Describe(const FPlayerTraversal& Plan, float Alpha, float TopOverFeet, float CapsuleRadius, float BodyScale, const FQuat& MeshRotation);

	/**
	 * The obstacle's top over the feet at the move's start, worked out from the plan alone: a mantle ends standing on it; a
	 * vault's hop passes a tuck's worth under it at the top.
	 */
	float EstimateTop(const FPlayerTraversal& Plan);

	/** When into the move (s) the body is highest: a vault's hop over the middle of the obstacle. */
	float ApexTime(const FPlayerTraversal& Plan);
}
