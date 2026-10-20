#include "Player/Animation/LooterStancePoseCarry.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Math/RotationMatrix.h"
#include "TwoBoneIK.h"

using namespace LooterStancePoseDetail;

// The third-person carry (LooterStancePoseCarry.h): when the gun goes low (FLooterReadyClock), how it's turned
// (LooterCarry::CarriedGun, which the view turns the gun by), the hands that carry it (ApplyCarry, in the stance layer),
// and the armed upper body's smoothed ride on the hips (FLooterBobFilter). The hands' spots are the full-size mannequin's
// (cm, its component space), in the body's frame.

namespace
{
	/**
	 * The share of the left arm's length its wrist may be from its shoulder: a carried gun whose foregrip would be further
	 * off is drawn in toward that shoulder (a shotgun's pump is twice as far out along it as a bullpup's foregrip), so the
	 * left hand never falls short of it.
	 */
	constexpr float SupportReach = 0.95f;

	/** A partial move takes this share of its time at the least (as the locomotion's alphas do). */
	constexpr float MinMoveShare = 0.35f;

	float MoveShare(const FViewEase& Ease, float Target)
	{
		return FMath::Clamp(FMath::Abs(Target - Ease.GetValue()), MinMoveShare, 1.f);
	}

	/**
	 * Heads Ease for Target in Seconds from where it is now, even when Target is the one it has: FViewEase keeps a plan whose
	 * target is asked for again, and planning for another target first starts from this same moment, speed and all.
	 */
	void HeadFor(FViewEase& Ease, float Target, float Seconds)
	{
		if (Ease.GetTarget() == Target)
		{
			Ease.SetTarget(Target + 1.f, Seconds);
		}
		Ease.SetTarget(Target, Seconds);
	}
}

// ---------------------------------------------------------------------------
// When and how far (game thread; pure)
// ---------------------------------------------------------------------------

void FLooterReadyClock::Update(float DeltaTime, bool bAlert, bool bMoving)
{
	SinceAlert = bAlert ? 0.f : SinceAlert + DeltaTime;
	StillFor = bMoving ? 0.f : StillFor + DeltaTime;
	if (bAlert)
	{
		// Aimed, shot or struck: back to the shoulder at once, even from a slow settle already on its way up.
		if (!bSnapping && (Ease.GetTarget() != 0.f || !Ease.IsSettled()))
		{
			HeadFor(Ease, 0.f, LooterCarry::RaiseSeconds * MoveShare(Ease, 0.f));
		}
		bSnapping = true;
	}
	else if (SinceAlert >= LooterCarry::ReadyDelay && StillFor < LooterCarry::StillGrace)
	{
		bSnapping = false;
		Ease.SetTarget(1.f, LooterCarry::LowerSeconds * MoveShare(Ease, 1.f));
	}
	else
	{
		// Stood still, or still within the delay after an alert: back up unhurried (a quick raise under way keeps its pace).
		Ease.SetTarget(0.f, LooterCarry::SettleSeconds * MoveShare(Ease, 0.f));
	}
	Ease.Advance(DeltaTime);
}

void FLooterReadyClock::Reset()
{
	Ease.Reset(0.f);
	SinceAlert = 0.f;
	StillFor = 0.f;
	bSnapping = false;
}

FVector FLooterBobFilter::Update(const FVector& Raw, float DeltaTime)
{
	if (!bStarted)
	{
		Smoothed = Raw;
		bStarted = true;
	}
	// Frame-rate free: the smoothed spot closes the same share of the gap per second at any frame rate.
	Smoothed += (Raw - Smoothed) * (1.f - FMath::Exp(-FMath::Max(DeltaTime, 0.f) / FollowSeconds));
	const FVector Lag = Smoothed - Raw;
	if (Lag.SizeSquared() > FMath::Square(MaxLag))
	{
		Smoothed = Raw + Lag.GetSafeNormal() * MaxLag;
	}
	return FMath::Lerp(Raw, Smoothed, SmoothShare);
}

FRotator LooterCarry::LowReadyTurn()
{
	return FRotator(-18.f, -6.f, 0.f);
}

FRotator LooterCarry::SprintHold(float Yaw, bool bShortGun)
{
	// The six-gun's muzzle a touch out, so it hangs clear of the leg it runs beside.
	return bShortGun ? FRotator(-70.f, Yaw + 4.f, 0.f) : FRotator(-20.f, Yaw - 35.f, -10.f);
}

FRotator LooterCarry::SwapHold(float Yaw, bool bShortGun)
{
	// A long gun angles further ahead and out than the six-gun, its longer barrel clear of the leg it hangs beside.
	return bShortGun ? FRotator(-75.f, Yaw + 6.f, 0.f) : FRotator(-62.f, Yaw + 14.f, 0.f);
}

FQuat LooterCarry::CarriedGun(const FQuat& Aiming, float Yaw, const FLooterCarry& Carry)
{
	FQuat Held = Aiming;
	if (Carry.LowReady > 0.f)
	{
		Held = FQuat::Slerp(Aiming, Aiming * LowReadyTurn().Quaternion(), FMath::Min(Carry.LowReady, 1.f));
	}
	if (Carry.Sprint > 0.f)
	{
		Held = FQuat::Slerp(Held, SprintHold(Yaw, Carry.bShortGun).Quaternion(), FMath::Min(Carry.Sprint, 1.f));
	}
	if (Carry.Swap > 0.f)
	{
		Held = FQuat::Slerp(Held, SwapHold(Yaw, Carry.bShortGun).Quaternion(), FMath::Min(Carry.Swap, 1.f));
	}
	return Held;
}

// ---------------------------------------------------------------------------
// The hands (animation thread)
// ---------------------------------------------------------------------------

FQuat LooterStancePoseDetail::ApplyCarry(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FQuat& ChestTurn, const FVector& Forward, const FVector& Right)
{
	const FLooterCarry& Carry = Stance.Carry;
	const float Weight = Carry.Weight();
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_r"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_r"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_r"));
	if (Weight <= UE_KINDA_SMALL_NUMBER || !UpperArm.IsValid() || !LowerArm.IsValid() || !Hand.IsValid())
	{
		return FQuat::Identity;
	}
	const FVector Up = FVector::UpVector;
	const float Low = FMath::Clamp(Carry.LowReady, 0.f, 1.f);
	const float Sprint = FMath::Clamp(Carry.Sprint, 0.f, 1.f);

	// The hands' turn. Shouldered, the chest (as the stance has leaned and aimed it) holds the gun along its own aim, and the
	// gun points along the player's; carried, the gun points where the carry puts it and the hands turn with it. A lean that
	// isn't the aim's (a crouch tips the chest, never the gun) is left between hand and gun as it always was while
	// shouldered and fades as the carry comes in, so the hands end up holding the carried gun just as the upright pose,
	// aiming level, holds it at the crosshair.
	const FQuat ChestAim = (ChestTurn * FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat()).GetNormalized();
	const FQuat Lean = FQuat::Slerp(Stance.AimRotation.Inverse() * ChestAim, FQuat::Identity, Weight);
	const FQuat HandTurn = (Stance.CarryRotation * Lean * ChestAim.Inverse()).GetNormalized();

	FTransform Shoulder = Pose.GetComponentSpaceTransform(UpperArm);
	FTransform Elbow = Pose.GetComponentSpaceTransform(LowerArm);
	FTransform Wrist = Pose.GetComponentSpaceTransform(Hand);
	const FVector WristAt = Wrist.GetLocation();
	const FVector Forearm = WristAt - Elbow.GetLocation();
	const FQuat HandRotation = (HandTurn * Wrist.GetRotation()).GetNormalized();

	// Where the wrist goes: lowered and drawn in a little for the low-ready; in a sprint low on the chest (a six-gun's low at
	// the side), from the shoulder as the run leans it; in a swap low at the side whatever the gun, the arm hanging.
	// The places off the shoulder are this body's; a hero worn over it with their own hold (ApplyHoldOffset, which already
	// moved the shouldered wrist) carries the gun as far off them, at their own chest and side.
	const FVector Lowered = WristAt - Up * LooterCarry::LowDrop - Forward * LooterCarry::LowBack - Right * LooterCarry::LowIn;
	const FVector HeroShoulder = Shoulder.GetLocation() + HoldOffsetVector(Stance, Forward, Right);
	const FVector AtSide = HeroShoulder + Forward * LooterCarry::SideAhead + Right * LooterCarry::SideOut - Up * LooterCarry::SideDown;
	const FVector Carried = Carry.bShortGun ? AtSide
		: HeroShoulder + Forward * LooterCarry::ChestAhead - Right * LooterCarry::ChestIn - Up * LooterCarry::ChestDown;
	FVector Target = FMath::Lerp(FMath::Lerp(FMath::Lerp(WristAt, Lowered, Low), Carried, Sprint), AtSide, FMath::Clamp(Carry.Swap, 0.f, 1.f));

	// Both hands on it: the gun is drawn in toward the left shoulder until its foregrip, where the turned left palm will hold
	// it, is within that arm's comfortable reach. A shouldered hold that already has it further off (a shotgun's pump, far
	// out along the gun, can be past the arm's whole length) keeps that much as the carry begins, so nothing pops, and is
	// drawn in to the comfortable reach as the carry comes in, so the carried gun is always in both hands.
	const float TwoHanded = 1.f - Carry.OneHanded();
	const FCompactPoseBoneIndex HoldBone = FindBone(Bones, *Stance.HoldBone.ToString());
	const FCompactPoseBoneIndex SupportUpper = FindBone(Bones, TEXT("upperarm_l"));
	const FCompactPoseBoneIndex SupportLower = FindBone(Bones, TEXT("lowerarm_l"));
	const FCompactPoseBoneIndex SupportHand = FindBone(Bones, TEXT("hand_l"));
	if (TwoHanded > UE_KINDA_SMALL_NUMBER && HoldBone == Hand && SupportUpper.IsValid() && SupportLower.IsValid() && SupportHand.IsValid())
	{
		const FVector OnGun = Stance.WeaponForegrip - Stance.WeaponGrip;
		const FTransform SupportWrist = Pose.GetComponentSpaceTransform(SupportHand);
		const FVector SupportShoulder = Pose.GetComponentSpaceTransform(SupportUpper).GetLocation();
		const FVector SupportElbow = Pose.GetComponentSpaceTransform(SupportLower).GetLocation();
		const float ArmLength = static_cast<float>(FVector::Dist(SupportShoulder, SupportElbow) + FVector::Dist(SupportElbow, SupportWrist.GetLocation()));

		// Shouldered: the hand where it is, the gun along the aim with whatever kick it has. The left hand is reckoned turned
		// as it will grip the gun (LeftHandOnGun), so its wrist is reckoned where it will be.
		const FQuat ShoulderedGun = (Stance.WeaponRotation * Stance.CarryRotation.Inverse() * Stance.AimRotation).GetNormalized();
		const FVector ShoulderedGrip = (Stance.HoldSocketLocal * Wrist).GetLocation();
		const FQuat ShoulderedPalm = LeftHandOnGun(Bones, Stance, ShoulderedGun, SupportWrist.GetRotation());
		const FVector ShoulderedReach = ShoulderedGrip + ShoulderedGun.RotateVector(OnGun) - ShoulderedPalm.RotateVector(LeftPalmFromWrist()) - SupportShoulder;
		const float Comfortable = SupportReach * ArmLength;
		const float Limit = FMath::Lerp(FMath::Max(Comfortable, static_cast<float>(ShoulderedReach.Size())), Comfortable, Weight);

		const FVector Grip = (Stance.HoldSocketLocal * FTransform(HandRotation, Target, Wrist.GetScale3D())).GetLocation();
		const FQuat Palm = LeftHandOnGun(Bones, Stance, Stance.WeaponRotation, (HandTurn * SupportWrist.GetRotation()).GetNormalized());
		const FVector Reach = Grip + Stance.WeaponRotation.RotateVector(OnGun) - Palm.RotateVector(LeftPalmFromWrist()) - SupportShoulder;
		const float Over = static_cast<float>(Reach.Size()) - Limit;
		if (Over > 0.f)
		{
			Target -= Reach.GetSafeNormal() * (Over * TwoHanded);
		}
	}

	// The elbow goes where the forearm, turned with the hand, would put it (setting out from where it is as the carry comes
	// in), so the wrist stays as straight as the arm's lengths allow.
	const FVector ElbowHint = FMath::Lerp(Elbow.GetLocation(), Target - HandTurn.RotateVector(Forearm), Weight);
	AnimationCore::SolveTwoBoneIK(Shoulder, Elbow, Wrist, ElbowHint, Target,
		/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
	Wrist.SetRotation(HandRotation);

	const FBoneTransform Chain[] = { FBoneTransform(UpperArm, Shoulder), FBoneTransform(LowerArm, Elbow), FBoneTransform(Hand, Wrist) };
	Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
	return HandTurn;
}
