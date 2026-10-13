#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "Weapons/ReloadMotion.h"
#include "BoneContainer.h"
#include "BonePose.h"

using namespace LooterStancePoseDetail;

// The stance pose's revolver reload (the Drover's): a code pose, since the mannequin's reload animation is a rifle's
// magazine swap and the left hand went to a magazine well the six-gun hasn't got. It runs on the gun's own reload steps
// (LooterReload::Steps for a cylinder, and its CylinderSwing), so the hand meets the cylinder as the gun's model swings it
// and the sounds land on the hand's work. The gun's grip stays in the right hand, which brings it in front of the chest;
// the view keeps it tipped and rolled for the reload (UPlayerViewComponent), its left side up, where the left hand works.

namespace
{
	// The Drover's cylinder in the gun's own space from the shooting hand's grip point (cm; X along the barrel, Y right,
	// Z up), as Art/Models/Weapons/Drover.py lays it out: the grip point raked back 2 cm under the frame, the cylinder's
	// middle 6.3 ahead of it and 6.2 above, swinging out to the gun's left on the crane's hinge, low on that side.
	constexpr float CylinderAhead = 6.3f;
	constexpr float CylinderUp = 6.2f;
	constexpr float HingeLeft = 1.4f;
	constexpr float HingeUp = 3.8f;
	constexpr float CylinderHalfLength = 2.2f;
	/** The ejector rod's tip ahead of the cylinder's middle, along the bore (it swings out with the cylinder). */
	constexpr float RodAhead = 6.75f;

	/** The left palm on the cylinder: a little to its left and under its middle, cradling it (the hand is palm up). */
	FVector PalmOnCylinder()
	{
		return FVector(0.f, -1.5f, -0.5f);
	}
	/** Out past the swung-out cylinder's left, to bat it home from there. */
	constexpr float BatFrom = 3.5f;
	/** The palm's punch on the ejector rod: from just ahead of its tip, this far back along the bore. */
	constexpr float RodClear = 2.f;
	constexpr float PunchDepth = 3.5f;
	/** The speedloader in the palm, behind the open cylinder's rear face, and how far it's pushed in. */
	constexpr float LoaderBehind = 4.f;
	constexpr float LoaderPush = 2.f;

	/** The belt pouch the speedloader comes from, from the hips (cm, the body's frame): the front of the left hip. */
	constexpr float PouchAhead = 10.f;
	constexpr float PouchOut = 15.f;
	constexpr float PouchUp = 3.f;

	/** The gun brought in to be worked on (cm, the body's frame, at the reload's full weight): toward the middle, up, out. */
	constexpr float InAhead = 2.f;
	constexpr float InToMiddle = 5.f;
	constexpr float InUp = 3.f;
	/** The flick that shuts the cylinder: the gun hand jolts right and up as the left hand bats it home. */
	constexpr float FlickRight = 2.5f;
	constexpr float FlickUp = 1.5f;

	/** Where in a cylinder's reload a step is (its progress), or Fallback if the choreography has no such step. */
	float StepAt(EReloadStep Step, float Fallback)
	{
		for (const FReloadStepAt& At : LooterReload::Steps(EWeaponReloadPart::Cylinder))
		{
			if (At.Step == Step)
			{
				return At.Progress;
			}
		}
		return Fallback;
	}

	/** 0 before A, eases up to 1 at B. */
	float Rise(float P, float A, float B)
	{
		return FMath::SmoothStep(A, B, P);
	}
}

FVector LooterStancePose::CylinderCenter(float SwingDegrees)
{
	// Swung about the crane's hinge, an axis along the bore, its top going over to the gun's left (-Y).
	const FVector Hinge(CylinderAhead, -HingeLeft, HingeUp);
	const FVector Closed(CylinderAhead, 0.f, CylinderUp);
	return Hinge + FQuat(FVector::ForwardVector, FMath::DegreesToRadians(SwingDegrees)).RotateVector(Closed - Hinge);
}

FLooterCylinderReload LooterStancePose::DescribeCylinderReload(float Progress)
{
	const float P = FMath::Clamp(Progress, 0.f, 1.f);
	const float Eject = StepAt(EReloadStep::Eject, 0.30f);
	const float RoundsIn = StepAt(EReloadStep::RoundsIn, 0.56f);
	const float In = StepAt(EReloadStep::CylinderIn, 0.72f);

	const float Open = LooterReload::CylinderOpenDegrees;
	const float Swing = LooterReload::CylinderSwing(P);
	const FVector Along = FVector::ForwardVector;
	const FVector OpenAt = CylinderCenter(Open);
	const FVector RodFront = OpenAt + Along * (RodAhead + RodClear);
	const FVector RodPunched = RodFront - Along * PunchDepth;
	const FVector LoaderFrom = OpenAt - Along * (CylinderHalfLength + LoaderBehind);
	const FVector LoaderIn = LoaderFrom + Along * LoaderPush;

	// The moments between the gun's own steps: to the rod and punched by the Eject, off to the belt and back with the
	// speedloader in by RoundsIn, out to the cylinder's left to bat it home by CylinderIn.
	const float ToRod = Eject - 0.08f;
	const float AtRod = Eject - 0.02f;
	const float Away = Eject + 0.03f;
	const float AtPouch = Away + 0.09f;
	const float LeavePouch = AtPouch + 0.04f;
	const float AtLoader = RoundsIn - 0.04f;
	const float ToBat = RoundsIn + 0.04f;
	const float AtBat = In - 0.07f;

	FLooterCylinderReload Reload;
	if (P < ToRod)
	{
		// On the cylinder as the latch clicks, riding it out as it swings (the reload's weight brings the hand there first).
		Reload.OnGun = CylinderCenter(Swing) + PalmOnCylinder();
	}
	else if (P < AtRod)
	{
		Reload.OnGun = FMath::Lerp(OpenAt + PalmOnCylinder(), RodFront, Rise(P, ToRod, AtRod));
	}
	else if (P < Eject)
	{
		// The palm punches the rod, speeding into it: the empties go as it bottoms.
		Reload.OnGun = FMath::Lerp(RodFront, RodPunched, FMath::InterpEaseIn(0.f, 1.f, (P - AtRod) / (Eject - AtRod), 2.f));
	}
	else if (P < Away)
	{
		Reload.OnGun = RodPunched;
	}
	else if (P < AtLoader)
	{
		// Down to the belt for the speedloader and back up behind the cylinder with it.
		Reload.OnGun = FMath::Lerp(RodPunched, LoaderFrom, Rise(P, Away, AtPouch));
		Reload.Pouch = Rise(P, Away, AtPouch) * (1.f - Rise(P, LeavePouch, AtLoader));
	}
	else if (P < RoundsIn)
	{
		Reload.OnGun = FMath::Lerp(LoaderFrom, LoaderIn, Rise(P, AtLoader, RoundsIn));
	}
	else if (P < ToBat)
	{
		// The loader let go of, its rounds in the chambers.
		Reload.OnGun = LoaderIn;
	}
	else
	{
		// Out past the cylinder's left, then riding it home as it's batted shut, and on it until the reload lets go.
		const float StillOut = FMath::Clamp(Swing / Open, 0.f, 1.f);
		const FVector Batting = CylinderCenter(Swing) + FMath::Lerp(PalmOnCylinder(), FVector(0.f, -BatFrom, 0.f), StillOut);
		Reload.OnGun = FMath::Lerp(LoaderIn, Batting, Rise(P, ToBat, AtBat));
	}
	// The gun hand's flick, peaking as the cylinder latches.
	Reload.Flick = Rise(P, In - 0.05f, In) * (1.f - Rise(P, In, In + 0.06f));
	return Reload;
}

void LooterStancePoseDetail::ApplyCylinderReloadGun(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FLooterCylinderReload& Reload, const FVector& Forward, const FVector& Right)
{
	// The six-gun's reach is gone by now (ApplyHoldReach eases it off through a reload); it comes in front of the chest,
	// toward the middle, where both hands can work it, and jolts as the cylinder shuts.
	const FVector Up = FVector::UpVector;
	const FVector Move = (Forward * InAhead - Right * InToMiddle + Up * InUp) * Stance.ReloadWeight + (Right * FlickRight + Up * FlickUp) * Reload.Flick;
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_r"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_r"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_r"));
	if (Move.IsNearlyZero(0.01) || !UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
	{
		return;
	}
	const FVector Wrist = Pose.GetComponentSpaceTransform(Hand).GetLocation();
	const FVector Elbow = Pose.GetComponentSpaceTransform(LowerArm).GetLocation();
	SolveArm(Pose, UpperArm, LowerArm, Hand, Wrist + Move, Elbow - Up * 15.f + Right * 10.f, 1.f, /*bTurnHand*/ false);
}

void LooterStancePoseDetail::ApplyCylinderReloadHand(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FLooterCylinderReload& Reload, const FVector& Forward, const FVector& Right)
{
	const FCompactPoseBoneIndex HoldBone = FindBone(Bones, *Stance.HoldBone.ToString());
	const FCompactPoseBoneIndex Pelvis = FindBone(Bones, TEXT("pelvis"));
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_l"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_l"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_l"));
	if (!HoldBone.IsValid() || !Pelvis.IsValid() || !UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
	{
		return;
	}
	// The gun's grip is in the right hand, the gun turned as the view holds it for the reload: the spot on it is here.
	const FVector Up = FVector::UpVector;
	const FVector Grip = (Stance.HoldSocketLocal * Pose.GetComponentSpaceTransform(HoldBone)).GetLocation();
	const FVector OnGun = Grip + Stance.WeaponRotation.RotateVector(Reload.OnGun);
	const FVector Pouch = Pose.GetComponentSpaceTransform(Pelvis).GetLocation() + Forward * PouchAhead - Right * PouchOut + Up * PouchUp;
	const FVector Palm = FMath::Lerp(OnGun, Pouch, Reload.Pouch);

	// The palm (not the wrist) on the spot, the hand as the pose holds it; the elbow down and out, as on the foregrip.
	const FQuat HandRotation = Pose.GetComponentSpaceTransform(Hand).GetRotation();
	const FVector WristTarget = Palm - HandRotation.RotateVector(LeftPalmFromWrist());
	const FVector ElbowHint = Pose.GetComponentSpaceTransform(LowerArm).GetLocation() - Up * 20.f - Right * 10.f;
	SolveArm(Pose, UpperArm, LowerArm, Hand, WristTarget, ElbowHint, Stance.ReloadWeight, /*bTurnHand*/ false);
}
