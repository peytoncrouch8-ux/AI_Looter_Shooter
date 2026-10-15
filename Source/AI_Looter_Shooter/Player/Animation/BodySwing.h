#pragma once

#include "CoreMinimal.h"
#include "Animation/BoneReference.h"
#include "BoneContainer.h"
#include "BonePose.h"

struct FReferenceSkeleton;

/**
 * How one kind of hanging thing swings (the duster's skirt, the cape, the bandana's point, the hair, the lantern):
 * pulled back toward its animated pose, its swing dying away, sagging a little more when flung. Rates are per second.
 */
struct AI_LOOTER_SHOOTER_API FBodySwingSettings
{
	/** The spring toward its animated pose: its natural frequency (rad/s), high is stiff. */
	float Stiffness = 8.f;
	/** How quickly its swing dies away (1/s). */
	float Damping = 4.f;
	/** Gravity's share (1 full). Its animated pose already hangs, so a small share, but for a pendulum's. */
	float Gravity = 0.05f;
};

/** A limb the swinging points keep clear of: a segment between two points (component space) and the distance kept. */
struct AI_LOOTER_SHOOTER_API FBodySwingCapsule
{
	FVector A = FVector::ZeroVector;
	FVector B = FVector::ZeroVector;
	/** Kept between the limb and the point. */
	float Radius = 0.f;
	/**
	 * Kept between the limb and the bone ending at the point (from the point before; 0: the point only). A shin swung
	 * back in a stride passed between two of the skirt's points and showed through the cloth between them.
	 */
	float BoneRadius = 0.f;
};

/**
 * One swinging chain's points (each bone's tip, in world space) and the pure step that moves them, kept apart from the
 * pose so the tests can drive it. The chain hangs from Anchor (its first bone's head, which the body moves).
 */
struct AI_LOOTER_SHOOTER_API FBodySwingPoints
{
	/** The points after the last fixed step, and the step before (Verlet's velocity). */
	TArray<FVector> Points;
	TArray<FVector> Previous;
	/** What the bones show this frame: between the last two steps by the time left over, hung from the anchor now. */
	TArray<FVector> Shown;
	/** Where the anchor was last frame: it moves smoothly across a long frame's steps, and a jump is a teleport. */
	FVector LastAnchor = FVector::ZeroVector;
	/** Time not yet stepped (less than one fixed step). */
	float Accumulator = 0.f;
	bool bStarted = false;

	/** Puts every point on its target, at rest (the first frame, after a teleport). */
	void Reset(TConstArrayView<FVector> Targets);

	/**
	 * Moves the points one step of DeltaSeconds toward Targets (where the animation has them), with inertia, damping,
	 * gravity (world -Z, cm/s^2 scaled by Settings.Gravity), each kept Lengths[i] from the one before (the first from
	 * Anchor) and out of the capsules: for each point in turn the same number of limbs, each with that point's own
	 * clearance as its radius (all in world space; the caller converts). The anchor jumping over 1.5 m starts it over.
	 * It steps in fixed 1/120 s steps (the leftover carried), so every frame rate swings alike; Shown is what to draw.
	 * No point goes under FloorZ (world Z: the ground at his feet, so a crouch lays the hem on it).
	 */
	void Step(const FVector& Anchor, TConstArrayView<FVector> Targets, TConstArrayView<float> Lengths,
		TConstArrayView<FBodySwingCapsule> Capsules, const FBodySwingSettings& Settings, float DeltaSeconds,
		double FloorZ = TNumericLimits<double>::Lowest());

	/** P pushed out of Capsule to at least its radius from its segment. */
	static FVector PushOut(const FVector& P, const FBodySwingCapsule& Capsule);

	/** Tip turned about Head so the bone between them keeps Capsule.BoneRadius from the limb (the bone's length kept by the caller). */
	static FVector PushBoneOut(const FVector& Head, const FVector& Tip, const FBodySwingCapsule& Capsule);

	/** Tip (Length from Head) kept on or over FloorZ: swung up about Head, its lean kept (Lean's when it hangs straight down). */
	static FVector KeepAbove(const FVector& Head, const FVector& Tip, double Length, double FloorZ, const FVector& Lean);
};

/**
 * Ellis's swing bones (Art/Models/Characters/Ellis.py: skirt_*, cape_*, bandana_*, hair_*, lantern_*), moved after the
 * retarget by springs (UBodyRetargetAnimInstance): each chain lags, swings and settles as the body moves, kept out of
 * the legs and body. Chains are found by name; a mesh without them swings nothing.
 */
struct AI_LOOTER_SHOOTER_API FBodySwing
{
	/** Finds the chains and their rest lengths and clearances in Skeleton (once per mesh). */
	void Bind(const FReferenceSkeleton& Skeleton);

	/** Moves the swing bones of Pose (component space) by DeltaSeconds; ComponentToWorld places the body in the world. */
	void Apply(FCSPose<FCompactPose>& Pose, const FTransform& ComponentToWorld, float DeltaSeconds);

	int32 NumChains() const { return Chains.Num(); }
	bool IsBoundTo(const FReferenceSkeleton& Skeleton) const;

	/** Chain kind's settings, by its name's prefix (skirt, cape, bandana, hair, lantern): the tuning table. */
	static FBodySwingSettings SettingsFor(const FString& ChainName);

private:
	struct FLimb
	{
		FBoneReference From;
		FBoneReference To;
		/** The most a point is kept from it (cm): about the limb's own radius with the clothes on it. */
		float MaxClearance = 0.f;
	};

	struct FChain
	{
		FString Name;
		FBodySwingSettings Settings;
		TArray<FBoneReference> Bones;
		/** Each bone's tip in its own space (its next bone's head, or past the last one), and the head-to-tip length. */
		TArray<FVector> TipInBone;
		TArray<float> Lengths;
		/** Per point and limb: the distance kept from the limb (its rest clearance, at most the limb's MaxClearance). */
		TArray<float> Clearances;
		/** Per bone and limb: the same for the bone ending at the point (its nearest at rest). */
		TArray<float> BoneClearances;
		/**
		 * A bone the chain's rest also follows, by FollowWeight (the cape's corners: the upper arm under them; the cape
		 * hangs from the spine, so without it it kept the arms-out shape it was modelled over), and each tip in its space.
		 */
		FBoneReference Follow;
		float FollowWeight = 0.f;
		TArray<FVector> TipInFollow;
		FBodySwingPoints Sim;
	};

	void InitBoneReferences(const FBoneContainer& Bones);

	TArray<FChain> Chains;
	TArray<FLimb> Limbs;
	/** The skeleton bound to (its bone count and first name, to notice another mesh) and the bone container's serial. */
	int32 BoundBoneCount = 0;
	FName BoundFirstBone;
	uint16 BoundSerial = 0;
	bool bReferencesValid = false;
};
