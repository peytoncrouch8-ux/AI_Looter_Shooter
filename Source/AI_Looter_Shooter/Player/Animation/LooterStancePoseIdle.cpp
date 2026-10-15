#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "TwoBoneIK.h"

using namespace LooterStancePoseDetail;

// The weary revenant's idle, standing still unarmed (the user's picks, 2026-10-15: "Weary revenant" with breathing,
// glances around and hand fidgets; the armed stance kept as it was). Code on top of the graph's idle: a dead man who has
// been walking a long time, shoulders a little forward, head a little low, his weight shifting from leg to leg.

namespace
{
	/** A full shift of his weight, one leg to the other and back (s). */
	constexpr float ShiftPeriod = 18.f;
	/** One breath (s). */
	constexpr float BreathPeriod = 4.2f;
	/** The spells the glances, the finger flexes and the thumb in the belt are drawn in (s). */
	constexpr float GlanceSpell = 10.f;
	constexpr float FlexSpell = 6.f;
	constexpr float HookSpell = 24.f;

	/** The weight shift: the hips over the standing leg (cm), the free side's hip dropping (degrees), the chest's counter-tilt. */
	constexpr float HipShift = 2.5f;
	constexpr float HipRoll = 3.f;
	constexpr float ChestRoll = 2.f;
	/** The weary posture (degrees): the chest forward, the shoulders rounded, the head down (the neck down, the head back up a little). */
	constexpr float ChestSlump = 4.f;
	constexpr float ShoulderRound = 5.f;
	constexpr float NeckDown = 7.f;
	constexpr float HeadUp = 2.f;
	/** The breath in (degrees): the chest up and back, the shoulders lifting. */
	constexpr float BreathChest = 0.8f;
	constexpr float BreathShoulders = 1.3f;
	/** A glance's share in the neck (the head takes the rest). */
	constexpr float GlanceNeck = 0.4f;
	/** A finger flex at each joint (degrees), the thumb's. */
	constexpr float FlexDegrees = 22.f;
	/**
	 * The right wrist with the thumb hooked in the belt, from the hips (cm: out to his right, ahead, up), and where its elbow
	 * points from the shoulder.
	 */
	const FVector HookWrist(15.f, 12.f, 8.f);
	const FVector HookElbow(25.f, -10.f, -20.f);

	/** A number 0..1 for spell Index of a kind (Salt), the same every run. */
	float Draw(int32 Index, uint32 Salt)
	{
		uint32 Hash = static_cast<uint32>(Index) * 2654435761u ^ (Salt * 2246822519u + 374761393u);
		Hash ^= Hash >> 15;
		Hash *= 2246822519u;
		Hash ^= Hash >> 13;
		Hash *= 3266489917u;
		Hash ^= Hash >> 16;
		return static_cast<float>(Hash & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
	}

	/** 0, eased up to 1 over In from Start, held Hold, eased back to 0 over Out (all s, on T's clock). */
	float Bump(float T, float Start, float In, float Hold, float Out)
	{
		if (T <= Start || T >= Start + In + Hold + Out)
		{
			return 0.f;
		}
		if (T < Start + In)
		{
			return FMath::SmoothStep(0.f, 1.f, (T - Start) / In);
		}
		if (T <= Start + In + Hold)
		{
			return 1.f;
		}
		return 1.f - FMath::SmoothStep(0.f, 1.f, (T - Start - In - Hold) / Out);
	}

	/** A spell's number and how far into it Time is. */
	int32 SpellOf(float Time, float Spell, float& OutInto)
	{
		const int32 Index = FMath::FloorToInt(Time / Spell);
		OutInto = Time - static_cast<float>(Index) * Spell;
		return Index;
	}

	/** One hand's flex in its spell (Offset staggers the two hands, Salt draws their own). */
	float Flex(float Time, float Offset, uint32 Salt)
	{
		float Into = 0.f;
		const int32 Spell = SpellOf(Time + Offset, FlexSpell, Into);
		return Draw(Spell, Salt) > 0.4f ? Bump(Into, 0.5f + 3.f * Draw(Spell, Salt + 1), 0.4f, 0.5f, 0.6f) : 0.f;
	}

	/** Which way a hand's fingers point (the wrist to the middle knuckle) and which way they curl (toward the palm), in component space. */
	bool HandFrame(FCSPose<FCompactPose>& Pose, FCompactPoseBoneIndex Hand, FCompactPoseBoneIndex Knuckle, FCompactPoseBoneIndex Tip,
		FVector& OutFingers, FVector& OutPalm)
	{
		if (!Hand.IsValid() || !Knuckle.IsValid() || !Tip.IsValid())
		{
			return false;
		}
		const FVector Knuckles = Pose.GetComponentSpaceTransform(Knuckle).GetLocation();
		OutFingers = (Knuckles - Pose.GetComponentSpaceTransform(Hand).GetLocation()).GetSafeNormal();
		// The fingers rest a little curled: the way their tips lean off the hand's line is the palm's side.
		OutPalm = FVector::VectorPlaneProject(Pose.GetComponentSpaceTransform(Tip).GetLocation() - Knuckles, OutFingers);
		return !OutFingers.IsNearlyZero() && OutPalm.Normalize(0.25);
	}

	/** The fingers of one hand (Suffix "_l" or "_r") curled further toward the palm by Degrees at each joint. */
	void CurlFingers(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const TCHAR* Suffix, float Degrees)
	{
		if (Degrees <= UE_KINDA_SMALL_NUMBER)
		{
			return;
		}
		const FString Side(Suffix);
		FVector Fingers;
		FVector Palm;
		if (!HandFrame(Pose, FindBone(Bones, *(TEXT("hand") + Side)), FindBone(Bones, *(TEXT("middle_01") + Side)),
			FindBone(Bones, *(TEXT("middle_03") + Side)), Fingers, Palm))
		{
			return;
		}
		for (const TCHAR* Finger : { TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") })
		{
			FCompactPoseBoneIndex Joints[3];
			for (int32 Joint = 0; Joint < 3; ++Joint)
			{
				Joints[Joint] = FindBone(Bones, *FString::Printf(TEXT("%s_%02d%s"), Finger, Joint + 1, Suffix));
			}
			for (int32 Joint = 0; Joint < 3; ++Joint)
			{
				if (!Joints[Joint].IsValid())
				{
					continue;
				}
				// About the line square to the bone and the palm's side: turning about it carries the finger toward the palm.
				const FVector Here = Pose.GetComponentSpaceTransform(Joints[Joint]).GetLocation();
				FVector Along = Fingers;
				if (Joint + 1 < 3 && Joints[Joint + 1].IsValid())
				{
					Along = (Pose.GetComponentSpaceTransform(Joints[Joint + 1]).GetLocation() - Here).GetSafeNormal(UE_SMALL_NUMBER, Fingers);
				}
				const FVector Axis = FVector::CrossProduct(Along, Palm).GetSafeNormal();
				if (!Axis.IsNearlyZero())
				{
					RotateBone(Pose, Joints[Joint], FQuat(Axis, FMath::DegreesToRadians(Degrees)));
				}
			}
		}
	}
}

FLooterIdleStance LooterStancePose::DescribeIdle(float Time)
{
	FLooterIdleStance Idle;
	const float T = FMath::Max(Time, 0.f);

	// He leans on one leg a while, then the other: a sine held near its ends.
	const float Sway = FMath::Sin(UE_TWO_PI * T / ShiftPeriod);
	Idle.WeightShift = FMath::Sign(Sway) * FMath::Sqrt(FMath::Abs(Sway));
	Idle.Breath = 0.5f - 0.5f * FMath::Cos(UE_TWO_PI * T / BreathPeriod);

	// Most spells, a look off to one side and back.
	float Into = 0.f;
	const int32 Glance = SpellOf(T, GlanceSpell, Into);
	if (Draw(Glance, 1) > 0.25f)
	{
		const float Look = Bump(Into, 1.5f + 4.f * Draw(Glance, 2), 0.7f, 1.f + 1.2f * Draw(Glance, 3), 0.8f);
		const float Side = Draw(Glance, 4) > 0.5f ? 1.f : -1.f;
		Idle.GlanceYaw = Look * Side * (18.f + 17.f * Draw(Glance, 5));
		Idle.GlancePitch = Look * (-3.f + 6.f * Draw(Glance, 6));
	}

	// Some spells, the right thumb in the belt a few seconds.
	const int32 Hook = SpellOf(T, HookSpell, Into);
	if (Draw(Hook, 11) > 0.35f)
	{
		Idle.ThumbHook = Bump(Into, 2.f + 6.f * Draw(Hook, 12), 0.9f, 5.f + 4.f * Draw(Hook, 13), 0.9f);
	}

	// The fingers flex now and then, each hand on its own (the right not while it's on the belt).
	Idle.FlexLeft = Flex(T, 0.f, 7);
	Idle.FlexRight = Flex(T, 2.7f, 9) * (1.f - Idle.ThumbHook);
	return Idle;
}

void LooterStancePoseDetail::ApplyIdle(FCSPose<FCompactPose>& Pose, const FBoneContainer& Bones, const FLooterStanceInput& Stance,
	const FLegBones (&Legs)[2], FCompactPoseBoneIndex Pelvis, const FVector& Forward, const FVector& Right)
{
	const float Alpha = FMath::Clamp(Stance.IdleAlpha, 0.f, 1.f);
	if (Alpha <= UE_KINDA_SMALL_NUMBER || !Pelvis.IsValid())
	{
		return;
	}
	const FLooterIdleStance Idle = LooterStancePose::DescribeIdle(Stance.IdleTime);
	const FVector Up = FVector::UpVector;
	auto Radians = [Alpha](float Degrees) { return FMath::DegreesToRadians(Degrees * Alpha); };
	const FCompactPoseBoneIndex Spine3 = FindBone(Bones, TEXT("spine_03"));
	const FCompactPoseBoneIndex Spine5 = FindBone(Bones, TEXT("spine_05"));
	const FCompactPoseBoneIndex Neck = FindBone(Bones, TEXT("neck_01"));
	const FCompactPoseBoneIndex Head = FindBone(Bones, TEXT("head"));

	// The weight onto one leg: the hips over it, the free side's hip dropping, the legs bent back onto the planted feet.
	FTransform Feet[UE_ARRAY_COUNT(Legs)];
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Legs); ++Index)
	{
		Feet[Index] = Pose.GetComponentSpaceTransform(Legs[Index].Foot);
	}
	FTransform Hips = Pose.GetComponentSpaceTransform(Pelvis);
	Hips.AddToTranslation(Right * (Idle.WeightShift * HipShift * Alpha));
	// Turning about Forward lowers his left side: weight on the right (+) drops the left hip.
	Hips.SetRotation((FQuat(Forward, Radians(Idle.WeightShift * HipRoll)) * Hips.GetRotation()).GetNormalized());
	SetBone(Pose, Pelvis, Hips);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Legs); ++Index)
	{
		const FLegBones& Leg = Legs[Index];
		FTransform Thigh = Pose.GetComponentSpaceTransform(Leg.Thigh);
		FTransform Calf = Pose.GetComponentSpaceTransform(Leg.Calf);
		FTransform Foot = Pose.GetComponentSpaceTransform(Leg.Foot);
		AnimationCore::SolveTwoBoneIK(Thigh, Calf, Foot, Calf.GetLocation() + Forward * 60.f, Feet[Index].GetLocation(),
			/*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0, /*MaxStretchScale*/ 1.0);
		Foot.SetRotation(Feet[Index].GetRotation());
		const FBoneTransform Chain[] = { FBoneTransform(Leg.Thigh, Thigh), FBoneTransform(Leg.Calf, Calf), FBoneTransform(Leg.Foot, Foot) };
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
	}

	// The chest leans back the other way over the hips, forward a little (weary) and up with the breath; turning about
	// Right tips forward.
	RotateBone(Pose, Spine3, FQuat(Forward, Radians(-Idle.WeightShift * ChestRoll)) * FQuat(Right, Radians(-0.6f * BreathChest * Idle.Breath)));
	RotateBone(Pose, Spine5, FQuat(Right, Radians(ChestSlump - BreathChest * Idle.Breath)));

	// The shoulders rounded forward and lifting with the breath. About Up, a right shoulder comes forward the negative way and
	// a left one the positive; about Forward, a right one rises the positive way.
	for (const float Side : { -1.f, 1.f })
	{
		const FCompactPoseBoneIndex Clavicle = FindBone(Bones, Side > 0.f ? TEXT("clavicle_r") : TEXT("clavicle_l"));
		RotateBone(Pose, Clavicle, FQuat(Forward, Radians(Side * BreathShoulders * Idle.Breath)) * FQuat(Up, Radians(-Side * ShoulderRound)));
	}

	// The head a little low, and a glance aside (+ yaw to his right: about Up, Forward turns toward Right).
	RotateBone(Pose, Neck, FQuat(Up, Radians(Idle.GlanceYaw * GlanceNeck)) * FQuat(Right, Radians(NeckDown - Idle.GlancePitch * GlanceNeck)));
	RotateBone(Pose, Head, FQuat(Up, Radians(Idle.GlanceYaw * (1.f - GlanceNeck))) * FQuat(Right, Radians(-HeadUp - Idle.GlancePitch * (1.f - GlanceNeck))));

	// The right thumb in the belt: the wrist to the belt's front at the hip, the elbow out and back, the fingers hanging
	// down over the hip with the palm to the body.
	const float Hook = Idle.ThumbHook * Alpha;
	const FCompactPoseBoneIndex UpperArm = FindBone(Bones, TEXT("upperarm_r"));
	const FCompactPoseBoneIndex LowerArm = FindBone(Bones, TEXT("lowerarm_r"));
	const FCompactPoseBoneIndex Hand = FindBone(Bones, TEXT("hand_r"));
	if (Hook > UE_KINDA_SMALL_NUMBER && UpperArm.IsValid() && LowerArm.IsValid() && Hand.IsValid())
	{
		const FVector HipsNow = Pose.GetComponentSpaceTransform(Pelvis).GetLocation();
		const FVector Shoulder = Pose.GetComponentSpaceTransform(UpperArm).GetLocation();
		const FVector Wrist = HipsNow + Right * HookWrist.X + Forward * HookWrist.Y + Up * HookWrist.Z;
		const FVector Elbow = Shoulder + Right * HookElbow.X + Forward * HookElbow.Y + Up * HookElbow.Z;
		SolveArm(Pose, UpperArm, LowerArm, Hand, Wrist, Elbow, Hook, /*bTurnHand*/ false);

		FVector Fingers;
		FVector Palm;
		if (HandFrame(Pose, Hand, FindBone(Bones, TEXT("middle_01_r")), FindBone(Bones, TEXT("middle_03_r")), Fingers, Palm))
		{
			const FVector WantFingers = (-Up + Forward * 0.15f).GetSafeNormal();
			const FVector WantPalm = FVector::VectorPlaneProject(-Right * 0.75f - Forward * 0.35f, WantFingers).GetSafeNormal();
			const FQuat Point = FQuat::FindBetweenNormals(Fingers, WantFingers);
			const FVector Turned = FVector::VectorPlaneProject(Point.RotateVector(Palm), WantFingers).GetSafeNormal();
			const FQuat Roll = Turned.IsNearlyZero() || WantPalm.IsNearlyZero() ? FQuat::Identity : FQuat::FindBetweenNormals(Turned, WantPalm);
			FTransform HandNow = Pose.GetComponentSpaceTransform(Hand);
			const FQuat Hooked = (Roll * Point * HandNow.GetRotation()).GetNormalized();
			HandNow.SetRotation(FQuat::Slerp(HandNow.GetRotation(), Hooked, Hook).GetNormalized());
			SetBone(Pose, Hand, HandNow);
		}
	}

	// The fingers flexing now and then.
	CurlFingers(Pose, Bones, TEXT("_l"), FlexDegrees * Idle.FlexLeft * Alpha);
	CurlFingers(Pose, Bones, TEXT("_r"), FlexDegrees * Idle.FlexRight * Alpha);
}
