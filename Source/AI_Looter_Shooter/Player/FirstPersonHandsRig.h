#pragma once

#include "CoreMinimal.h"
#include "Player/FirstPersonHandsTypes.h"

class USkeletalMesh;
struct FReferenceSkeleton;

/** What the rig poses the arms toward this frame, all in the arms mesh's component space. */
struct FHandsRigTargets
{
	/** Each hand's placement (by FirstPersonHands::Index). */
	FHandPlacement Hands[2];
	/** Each hand's usual way out of the wrist for its forearm (back, out and down), where a placement doesn't give one. */
	FVector DefaultForearm[2] = { FVector(-0.5, -0.5, -0.7), FVector(-0.7, 0.45, -0.55) };
};

/**
 * The first-person arms' skeleton posed by code, as plain math (no component, so the tests pose the mannequin's rest
 * pose with it): each hand's grip point and frame put on its placement, the forearm leaving the wrist the way the
 * placement says (two-bone IK, the elbow hinging the way its rest pose bends, the forearm taking most of the wrist's
 * twist), the fingers curled from straight. Works from the bones' places only, never their axes (an import turns those).
 *
 * Bones by the mannequin's names: clavicle, upperarm, lowerarm, hand _l/_r and thumb/index/middle/ring/pinky _01.._03.
 * A hand's grip point is the mesh's HandGrip_L / HandGrip_R socket where it has one (the mannequin's), else SK_FP_Arms's
 * spot worked out from the hand's bones (FirstPersonHands::GripOffPalm). A hand whose elbow the upper arm can't reach
 * (view-model guns sit far out) brings its shoulder to it, off the view; the arms are only ever seen from the elbow down.
 *
 * bArmsOnly (the mannequin stand-in): every bone that isn't an arm's (the body, the head, the legs) collapses to a point
 * between the shoulders, so only the arms show.
 *
 * A placement with a contact (FHandContact) has its palm seated on its handle before the arm is solved, and its fingers
 * closed till they touch it after (the trigger finger onto the trigger).
 *
 * FirstPersonHandsRig.cpp: binding and the arms; FirstPersonHandsFingers.cpp: the hands' frames, grip points and fingers;
 * FirstPersonHandsContact.cpp: handles, seating and closing on them.
 */
class AI_LOOTER_SHOOTER_API FFirstPersonHandsRig
{
public:
	/** Reads Mesh's skeleton; false (and unbound) without both arms' four bones. */
	bool Bind(const USkeletalMesh* Mesh, bool bArmsOnly);

	bool IsBound() const { return bBound; }

	/**
	 * The pose for Targets: every bone's transform against its parent (OutLocal, what a poseable mesh takes) and in the
	 * component's space (OutSpace). Unbound: the rest pose.
	 */
	void Pose(const FHandsRigTargets& Targets, TArray<FTransform>& OutLocal, TArray<FTransform>& OutSpace) const;

	/** The body's facing and up at rest, and the shoulders' middle (component space). */
	FVector GetForward() const { return Forward; }
	FVector GetUp() const { return Up; }
	FVector GetShoulderMiddle() const { return ShoulderMiddle; }

	/** A hand's bone, and its grip point in that bone's space. */
	int32 GetHandBone(EFirstPersonHand Hand) const { return Arms[FirstPersonHands::Index(Hand)].Hand; }
	FVector GetHoldLocal(EFirstPersonHand Hand) const { return Arms[FirstPersonHands::Index(Hand)].HoldLocal; }

	/** A forearm's length, elbow to wrist (the mesh's own cm: the component's scale on top). The framing keeps its elbow out of view. */
	float GetForearmLength(EFirstPersonHand Hand) const { return bBound ? Arms[FirstPersonHands::Index(Hand)].LowerLength : 0.f; }

	/** A finger joint's bone (INDEX_NONE where the skeleton has none). */
	int32 GetFingerBone(EFirstPersonHand Hand, int32 Finger, int32 Joint) const;

	/** At rest: a hand's grip point and palm facing (component space), and how bent a finger joint is (degrees). */
	FVector GetRestHoldPoint(EFirstPersonHand Hand) const
	{
		const FArm& Arm = Arms[FirstPersonHands::Index(Hand)];
		return bBound ? RestSpace[Arm.Hand].TransformPosition(Arm.HoldLocal) : FVector::ZeroVector;
	}
	FVector GetRestPalm(EFirstPersonHand Hand) const { return Arms[FirstPersonHands::Index(Hand)].HandFrame.GetAxisY(); }
	float GetRestCurl(EFirstPersonHand Hand, int32 Finger, int32 Joint) const
	{
		return bBound ? Arms[FirstPersonHands::Index(Hand)].RestCurls[FMath::Clamp(Finger, 0, FHandFingers::Count - 1)][FMath::Clamp(Joint, 0, FHandFingers::Joints - 1)] : 0.f;
	}

	/** Where a hand's grip point is, and which way its knuckles and palm face, in a posed component space. */
	FVector HoldPointIn(EFirstPersonHand Hand, const TArray<FTransform>& Space) const;
	FVector FingersDirIn(EFirstPersonHand Hand, const TArray<FTransform>& Space) const;
	FVector PalmDirIn(EFirstPersonHand Hand, const TArray<FTransform>& Space) const;

	/** A bone squeezed to nothing (bArmsOnly). */
	bool IsCollapsed(int32 Bone) const { return Kept.IsValidIndex(Bone) && !Kept[Bone]; }

	/**
	 * A finger's joints and its tip (Out[0..2] the three joints, Out[3] the tip; component space) with the hand's bone at
	 * HandSpace and the finger bent Curl (degrees from straight, knuckle out) and spread Spread (FirstPersonHandsContact.cpp).
	 * False where the skeleton lacks the finger.
	 */
	bool FingerPoints(EFirstPersonHand Hand, const FTransform& HandSpace, int32 Finger, const float Curl[FHandFingers::Joints], float Spread,
		FVector Out[4]) const;

	/**
	 * The fingers a placement ends with once its contact is worked (FirstPersonHandsContact.cpp): each finger closed till it
	 * touches the handle, the trigger finger on the trigger, blended by the contact's shares onto the placement's own curls.
	 * The hand's bone at HandSpace (component space, as Pose solves it). Cached: a hand held still on a gun reuses its answer.
	 */
	FHandFingers ContactFingers(EFirstPersonHand Hand, const FTransform& HandSpace, const FHandPlacement& Target) const;

	/** A finger's tip in a posed component space (past its end joint, as the touch solve takes it). */
	FVector FingerTipIn(EFirstPersonHand Hand, int32 Finger, const TArray<FTransform>& Space) const;

	/** Which fingers touched their handle in the last solve (a bit per finger, thumb first), and their curls then. */
	uint8 GetLastTouched(EFirstPersonHand Hand) const { return ContactCache[FirstPersonHands::Index(Hand)].Found; }
	const FHandFingers& GetLastSolved(EFirstPersonHand Hand) const { return ContactCache[FirstPersonHands::Index(Hand)].Solved; }

	/** The placement with its palm seated on its handle (Contact.Seat of the way, at most MaxSeat cm). */
	FHandPlacement Seated(const FHandPlacement& Target) const;

	/** The most a palm is slid to seat it (cm): past it the hold itself is wrong, and the table wants fixing. */
	static constexpr float MaxSeat = 2.5f;
	/** A finger's tip past its end joint, as a share of the joint before it (the skeleton has no tip bone). */
	static constexpr float TipShare = 0.85f;

	/**
	 * The furthest a shoulder comes forward for a far hand (cm): a shotgun's pump is the longest reach, about 65 (more with
	 * smaller arms). The arms never stretch (a stretched sleeve shows), so a hand further out than this falls short of its
	 * spot.
	 */
	static constexpr float MaxShoulderShift = 90.f;
	/** The share of the wrist's twist the forearm takes (the rest stays at the wrist, as a real forearm turns). */
	static constexpr float ForearmTwistShare = 0.6f;
	/** How small a collapsed bone is (zero would lose the arms parented to it). */
	static constexpr float CollapsedScale = 0.001f;

private:
	struct FArm
	{
		int32 Clavicle = INDEX_NONE;
		int32 UpperArm = INDEX_NONE;
		int32 LowerArm = INDEX_NONE;
		int32 Hand = INDEX_NONE;
		float UpperLength = 0.f;
		float LowerLength = 0.f;
		/** At rest: the upper arm's and forearm's directions and the elbow's hinge (the axis it bends about). */
		FVector UpperDir = FVector::ForwardVector;
		FVector LowerDir = FVector::ForwardVector;
		FVector Hinge = FVector::RightVector;
		/** The hand's frame at rest (X wrist to knuckles, Y out of the palm), component space. */
		FQuat HandFrame = FQuat::Identity;
		/** The hand's grip point in its bone's space. */
		FVector HoldLocal = FVector::ZeroVector;
		/** Each finger joint's bone, and the axis it curls about toward the palm, in that bone's own space. */
		int32 Fingers[FHandFingers::Count][FHandFingers::Joints];
		FVector CurlAxes[FHandFingers::Count][FHandFingers::Joints];
		/** How bent each joint already is at rest (degrees): the curls asked for are from straight. */
		float RestCurls[FHandFingers::Count][FHandFingers::Joints];
		/** Each finger's spread axis (the palm's facing, signed: index-ward for a finger, the fingers' way for the thumb), in its first joint's space. */
		FVector SpreadAxes[FHandFingers::Count];
		/** Each finger's first joint's parent (its metacarpal, or the hand) at rest, in the hand bone's space. */
		FTransform FingerBase[FHandFingers::Count];
		/** Each finger's tip in its end joint's space. */
		FVector TipLocal[FHandFingers::Count];
	};

	/** A finger joint's transform against its parent, bent Degrees from straight (and, the first joint, spread Spread). */
	FTransform JointLocal(const FArm& Arm, int32 Finger, int32 Joint, float Degrees, float Spread) const;

	/** What ContactFingers last worked out for a hand, and from what (a still hand on a gun reuses it). */
	struct FContactCache
	{
		FHandleShape Handle;
		FVector TriggerPoint = FVector::ZeroVector;
		FHandFingers From;
		FHandFingers Solved;
		uint8 Found = 0;
		bool bTrigger = false;
		bool bValid = false;
	};
	/** The touch-closed fingers, uncached: Found has a bit set per finger that touched (else the placement's curls stand). */
	FHandFingers SolveContact(EFirstPersonHand Hand, const FTransform& HandSpace, const FHandPlacement& Target, uint8& OutFound) const;
	mutable FContactCache ContactCache[2];

	/** Finds an arm's four bones; false without them. */
	bool FindArm(FArm& Arm, EFirstPersonHand Side, const FReferenceSkeleton& Skeleton) const;
	/** The arm's lengths and elbow hinge at rest (needs the body's facing). */
	void MeasureArm(FArm& Arm) const;
	/** The hand's frame, grip point and fingers (FirstPersonHandsFingers.cpp). */
	void BindHand(FArm& Arm, EFirstPersonHand Side, const FReferenceSkeleton& Skeleton, const USkeletalMesh& Mesh);

	/** Solves one arm to its placement: the clavicle, upper arm, forearm and hand, in component space. */
	void SolveArm(const FArm& Arm, const FHandPlacement& Target, const FVector& DefaultForearm, TArray<int32, TInlineAllocator<8>>& OutBones,
		TArray<FTransform, TInlineAllocator<8>>& OutSpaces) const;

	/** A finger joint's transform against its parent, curled as Fingers (by hand) say (FirstPersonHandsFingers.cpp). */
	FTransform CurledLocal(int32 Bone, const FHandFingers (&Fingers)[2]) const;

	TArray<int32> Parents;
	TArray<FTransform> RestLocal;
	TArray<FTransform> RestSpace;
	TArray<bool> Kept;
	/** For a finger joint's bone: its hand, finger and joint packed (hand * 15 + finger * 3 + joint); -1 for other bones. */
	TArray<int16> FingerSlot;
	FArm Arms[2];
	FVector Forward = FVector::ForwardVector;
	FVector Up = FVector::UpVector;
	FVector ShoulderMiddle = FVector::ZeroVector;
	bool bBound = false;
};
