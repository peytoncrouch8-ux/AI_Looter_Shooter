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
	/**
	 * How thick the thing is round its line (cm): more room kept from the limbs than cloth gets (the lantern's 4.3 cm body
	 * sank 1.5 cm into the crouched thigh while its line kept clear).
	 */
	float Bulk = 0.f;
	/**
	 * May rise as high as its animated target, not only as high as the point it hangs from: the cape's chains ride the
	 * arms, and held level with their anchors they couldn't follow an arm raised to aim (the arm came up through them).
	 */
	bool bRisesToTarget = false;
	/**
	 * The share of the anchor's own move carried straight to the points each step (0: all of it left to their inertia, as
	 * free cloth hangs). A thing lying against the body goes with it, so only the rest of a start or a stop flings it: the
	 * body starts and stops at about 3 g, which swung Crane's hook out level on every step off (2026-10-20).
	 */
	float Carry = 0.f;
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
	/**
	 * Pushed off the shortest way, never across the limb to the chain's outward side (a hand or forearm: it hangs outside
	 * the coat as often as in it, and taken for come through, the cloth was flipped out over the hand).
	 */
	bool bShortestWay = false;
};

/** The ground a chain lies on. */
struct AI_LOOTER_SHOOTER_API FBodySwingFloor
{
	/** World Z no point goes under (the ground at his feet). */
	double Z = TNumericLimits<double>::Lowest();
	/**
	 * How far out from its head the chain's last point may lie on the floor, a share of its length (1: as far as its
	 * length reaches). Short of that its bone is shorter: the hem gathers on the ground instead of jutting out stiffly.
	 */
	float LastReach = 1.f;
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
	/** Where the anchor was at the last fixed step (its move since is what Carry hands on). */
	FVector LastStepAnchor = FVector::ZeroVector;
	/** Time not yet stepped (less than one fixed step). */
	float Accumulator = 0.f;
	bool bStarted = false;
	/**
	 * The way out of the body the chain hangs (world space, set by the caller each frame; zero: unknown): a limb pushes
	 * the bones out this side, the cloth staying outside the legs.
	 */
	FVector Outward = FVector::ZeroVector;

	/** Puts every point on its target, at rest (the first frame, after a teleport). */
	void Reset(TConstArrayView<FVector> Targets);

	/**
	 * Moves the points one step of DeltaSeconds toward Targets (where the animation has them), with inertia, damping,
	 * gravity (world -Z, cm/s^2 scaled by Settings.Gravity), each kept Lengths[i] from the one before (the first from
	 * Anchor) and out of the capsules: for each point in turn the same number of limbs, each with that point's own
	 * clearance as its radius (all in world space; the caller converts). The anchor jumping over 1.5 m starts it over.
	 * It steps in fixed 1/120 s steps (the leftover carried), so every frame rate swings alike; Shown is what to draw.
	 * No point goes under the floor (so a crouch lays the hem on the ground), and the last may then lie nearer its head
	 * than its length (Floor.LastReach). A point a limb (or the floor, or the ceiling over its head) pushes keeps no speed
	 * into it, and the push gives it none. Settings.Carry of the anchor's move is handed straight on to the points.
	 */
	void Step(const FVector& Anchor, TConstArrayView<FVector> Targets, TConstArrayView<float> Lengths,
		TConstArrayView<FBodySwingCapsule> Capsules, const FBodySwingSettings& Settings, float DeltaSeconds,
		const FBodySwingFloor& Floor = FBodySwingFloor());

	/** P pushed out of Capsule to at least its radius from its segment. */
	static FVector PushOut(const FVector& P, const FBodySwingCapsule& Capsule);

	/**
	 * Tip moved so the bone from Head keeps Capsule.BoneRadius from the limb (the bone's length kept by the caller):
	 * out Outward's side of the limb (square to the limb and the bone), back across it when the limb has come through;
	 * with no Outward, straight away from the limb.
	 */
	static FVector PushBoneOut(const FVector& Head, const FVector& Tip, const FBodySwingCapsule& Capsule,
		const FVector& Outward = FVector::ZeroVector);

	/**
	 * How far to move a place on the cloth (S) to keep Capsule.Radius from the limb: straight away from it, or out
	 * Outward's side (square to the limb) when the limb has come through. Zero when clear.
	 */
	static FVector PushPlaceOut(const FVector& S, const FBodySwingCapsule& Capsule, const FVector& Outward);

	/**
	 * Tip (Length from Head) kept no higher than CeilingZ (Head's height for hanging cloth: it never rises over what it
	 * hangs from, a crouch's thigh once swung a coat flap up to his shoulder; a riding chain's target height).
	 */
	static FVector KeepUnder(const FVector& Head, const FVector& Tip, double Length, double CeilingZ);

	/**
	 * Tip (Length from Head) kept on or over FloorZ: under it, it lies on it out the way it leans (Lean's when it hangs
	 * straight down), at most MaxReach from Head across the ground (so then nearer Head than Length).
	 */
	static FVector KeepAbove(const FVector& Head, const FVector& Tip, double Length, double FloorZ, const FVector& Lean,
		double MaxReach);
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

	/** Chain kind's settings, by its name's prefix (skirt, cape, bandana, hair, lantern, hook, longhair, tails): the tuning table. */
	static FBodySwingSettings SettingsFor(const FString& ChainName);

	/**
	 * The kind (SettingsFor's name) of the chain ChainName hanging from ParentBone: a `lantern_` chain hung from the spine is
	 * a hook (Crane's toll-chain hook on his chest: heavier and shorter than Ellis's lantern on her thigh, and lying against
	 * him), hair hung from the neck is long hair (Crane's, down his back and in front of his shoulders; Ellis's hangs from
	 * her head under the hat), a skirt chain on a tailcoat (bTailcoat: the body's skirt has no front chains, skirt_f*:
	 * Odessa's tails) is tails; else the chain's own name.
	 */
	static FString KindOf(const FString& ChainName, const FString& ParentBone, bool bTailcoat = false);

	/** Neighbouring skirt chains whose cloth between them keeps off the limbs too (the coat's ring). */
	int32 NumRingPairs() const { return Ring.Num(); }

	/** The cloth at rest keeps this share of its distance from a limb (a little give, so it isn't shoved at rest). */
	static constexpr float ClearanceShare = 0.95f;

private:
	struct FLimb
	{
		FBoneReference From;
		FBoneReference To;
		/** The most a point is kept from it (cm): about the limb's own radius with the clothes on it. */
		float MaxClearance = 0.f;
		/**
		 * A forearm or a hand: it pushes the cloth at most HandGive past where the animation has it (each frame), however
		 * far its rest clearance asks. A chain hung right where the idle hand rests (Crane's front-left skirt, Pike's sides)
		 * was thrown 18 to 29 cm out and up, folded over the hip, by a hand measured against the bind pose's A.
		 */
		bool bHand = false;
	};

	struct FChain
	{
		FString Name;
		FBodySwingSettings Settings;
		TArray<FBoneReference> Bones;
		/**
		 * Each bone's tip in its own space (its next bone's head; the last one's `<chain>_end` bone, where its cloth ends,
		 * or past it by the bone before's length), and the head-to-tip length.
		 */
		TArray<FVector> TipInBone;
		TArray<float> Lengths;
		/** The bone's own axis along its length (0 X, 1 Y, 2 Z; INDEX_NONE when it lies along none), to shorten it on. */
		TArray<int32> LengthAxes;
		/** The way out of the body the chain hangs (level, from the pelvis to its first tip at rest), in its first bone's space. */
		FVector OutwardInBone = FVector::ZeroVector;
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
		/** The followed bone on the chain's first bone, eased (the cape takes up the arm's slow moves, not a run's swing). */
		FTransform FollowEased = FTransform::Identity;
		bool bFollowStarted = false;
		FBodySwingPoints Sim;

		/** This frame's: whether the pose has the chain, its bones' indices and animated transforms, its anchor (world). */
		bool bLive = false;
		TArray<FCompactPoseBoneIndex> Indices;
		TArray<FTransform> Animated;
		FVector Anchor = FVector::ZeroVector;
	};

	/**
	 * Two neighbouring skirt chains (round the coat, not across its open front): the cloth between them keeps off the
	 * limbs too. Without it a shin slipped between two chains' points and showed through the coat in a stride.
	 */
	struct FRingPair
	{
		int32 A = INDEX_NONE;
		int32 B = INDEX_NONE;
		/**
		 * Per level and limb, what the cloth keeps from the limb (rest distance x ClearanceShare, at most the limb's):
		 * halfway between the two chains' points (Mid) and in the middle of the patch down to them from the level above
		 * (Center).
		 */
		TArray<float> MidClearances;
		TArray<float> CenterClearances;
	};

	void InitBoneReferences(const FBoneContainer& Bones);
	/** The ring's pairs and their clearances, from the rest pose (Rest: Skeleton's, component space) and the limbs at rest. */
	void BindRing(const FReferenceSkeleton& Skeleton, TConstArrayView<FTransform> Rest, TConstArrayView<FVector> LimbA,
		TConstArrayView<FVector> LimbB, TConstArrayView<float> LimbMax);
	/** The cloth between neighbouring skirt chains pushed off the limbs (this frame's shown points; world space). */
	void ApplyRing(TConstArrayView<FBodySwingCapsule> LimbsNow, double Scale);

	TArray<FChain> Chains;
	TArray<FLimb> Limbs;
	TArray<FRingPair> Ring;
	/** The skeleton bound to (its bone count and first name, to notice another mesh) and the bone container's serial. */
	int32 BoundBoneCount = 0;
	FName BoundFirstBone;
	uint16 BoundSerial = 0;
	bool bReferencesValid = false;
};
