// AUnpaidCreature's rise out of the ground, its pose (2026-10-31; the flow is UnpaidCreatureRise.cpp): how deep it is (RiseDrop:
// UnpaidRiseRules::Sink through its phases; killed mid-rise, sinking back into its grave), its channels (head down in the dirt,
// a hand grasping at the air, bent over its planted hands gasping, its head thrown back for its first breath), the trunk's turns
// laid on them (bending away from the arm it reaches up with, dragged forward over its hands, shaking the dirt off), and its
// arms (the lead hand thrust up out of the dirt, slammed down, the other up and planted beside it, both pushing, let go as it
// stands). Angles in the rig's degrees (UnpaidCreatureRig.cpp: Pitch nods forward, Roll tips left, Yaw turns right).

#include "Creatures/UnpaidLifeWork.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidBodyParts.h"
#include "Creatures/UnpaidRiseRules.h"

namespace
{
	float Ease(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	float EaseOut(float Alpha)
	{
		return FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	float EaseIn(float Alpha)
	{
		return FMath::InterpEaseIn(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	FQuat Pitch(float Degrees)
	{
		return FQuat(FVector::RightVector, FMath::DegreesToRadians(Degrees));
	}

	FQuat Roll(float Degrees)
	{
		return FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Degrees));
	}

	FQuat Yaw(float Degrees)
	{
		return FQuat(FVector::UpVector, FMath::DegreesToRadians(Degrees));
	}

	/** Killed mid-rise, it sinks this much further into its grave (cm) over this long (s) as it breaks up. */
	constexpr float SinkBack = 60.f;
	constexpr float SinkBackSeconds = 0.9f;
	/** Its arms' pose fades into the channels' over this share of its stand. */
	constexpr float ArmRelease = 0.6f;
}

// ---------------------------------------------------------------------------
// Its pose
// ---------------------------------------------------------------------------

float FUnpaidRiseWork::Drop(const AUnpaidCreature& Unpaid)
{
	const FUnpaidRiseState& Rise = Unpaid.Kit.Rise;
	if (Rise.IsRising())
	{
		return UnpaidRiseRules::Sink(Rise.Style, Rise.Phase, Rise.Into);
	}
	if (Rise.bDiedRising && Unpaid.IsDead())
	{
		return Rise.DiedAt - SinkBack * Ease(Unpaid.DeathTime / SinkBackSeconds);
	}
	return 0.f;
}

void FUnpaidRiseWork::Channels(const AUnpaidCreature& Unpaid, UnpaidBody::FPoseChannels& Goal)
{
	const FUnpaidRiseState& Rise = Unpaid.Kit.Rise;
	if (!Rise.IsRising() || Unpaid.IsDead())
	{
		return;
	}
	const float Into = Rise.Into;
	const float Time = Rise.Time;
	switch (Rise.Phase)
	{
	case EUnpaidRisePhase::Buried:
		// Curled under the dirt (unseen): head down, mouth open.
		Goal.Lean = 25.f;
		Goal.Jaw = 25.f;
		Goal.Hunt = 0.f;
		Goal.Reach = 0.f;
		Goal.Spread = 0.f;
		Goal.Rear = 0.f;
		Goal.Snap = 0.f;
		Goal.LookPitch = -20.f;
		break;
	case EUnpaidRisePhase::Breach:
		// Its head still in the dirt, its mouth wide, its hand grasping at the air.
		Goal.Lean = 20.f;
		Goal.Jaw = 28.f;
		Goal.Splay = 22.f;
		Goal.Hunt = 0.f;
		Goal.Reach = 0.f;
		Goal.Spread = 0.f;
		Goal.Rear = 0.f;
		Goal.LookPitch = -10.f;
		break;
	case EUnpaidRisePhase::Haul:
		// Bent over its planted hands, gasping, clawing at the dirt; its eyes come up as it gets out.
		Goal.Lean = 38.f;
		Goal.Jaw = 18.f + 6.f * FMath::Sin(Time * 9.f);
		Goal.Curl = 55.f;
		Goal.Splay = 10.f;
		Goal.Hunt = 0.f;
		Goal.Reach = 0.f;
		Goal.Spread = 0.f;
		Goal.Rear = 0.f;
		Goal.LookPitch = FMath::Lerp(-15.f, 10.f, Into);
		break;
	case EUnpaidRisePhase::Stand:
	{
		// It straightens and throws its head back for its first breath, arms flung out, then the hunger takes it.
		const float Up = Ease(Into);
		Goal.Lean = FMath::Lerp(30.f, Goal.Lean, Up);
		Goal.Rear = FMath::Max(Goal.Rear, 0.45f * FMath::Sin(UE_PI * FMath::Min(Into / 0.6f, 1.f)));
		Goal.Jaw = FMath::Lerp(30.f, Goal.Jaw, Ease((Into - 0.4f) / 0.6f));
		Goal.Spread = FMath::Max(Goal.Spread, 0.35f * (1.f - Up));
		Goal.Splay = FMath::Lerp(20.f, Goal.Splay, Up);
		Goal.Hunt *= Up;
		break;
	}
	default:
		break;
	}
}

void FUnpaidRiseWork::LayBones(AUnpaidCreature& Unpaid)
{
	const FUnpaidRiseState& Rise = Unpaid.Kit.Rise;
	if (!Rise.IsRising() || Unpaid.IsDead() || !Unpaid.bRigReady)
	{
		return;
	}
	auto Turn = [&Unpaid](int32 Bone, const FQuat& Extra)
	{
		if (Unpaid.Bones.IsValidIndex(Bone))
		{
			Unpaid.Bones[Bone].Own = Unpaid.Bones[Bone].Own * Extra;
		}
	};
	auto Shift = [&Unpaid](int32 Bone, const FVector& By)
	{
		if (Unpaid.Bones.IsValidIndex(Bone))
		{
			Unpaid.Bones[Bone].Shift += By;
		}
	};
	const float Into = Rise.Into;
	const float Time = Rise.Time;
	// The lead arm's side (+1 its right): reaching up with it, the trunk bends away from it.
	const float LeadSide = Rise.LeadArm == 1 ? 1.f : -1.f;
	switch (Rise.Phase)
	{
	case EUnpaidRisePhase::Breach:
	{
		const float Reach = EaseOut(Into / 0.4f);
		Turn(Unpaid.SpineBone, Roll(6.f * LeadSide * Reach));
		Turn(Unpaid.ChestBone, Roll(4.f * LeadSide * Reach) * Yaw(-6.f * LeadSide * Reach));
		// Its whole body jerking as it claws (the dirt fighting it).
		Shift(Unpaid.PelvisBone, FVector(0.f, 0.f, 3.f * FMath::Sin(Time * 23.f) * (1.f - Into)));
		break;
	}
	case EUnpaidRisePhase::Haul:
	{
		// Bent far over its hands, dragging itself forward over them, rocking as each heave takes it.
		const float Bent = 1.f - 0.4f * Ease((Into - 0.55f) / 0.45f);
		const float Rock = FMath::Sin(Into * UE_PI * 4.f);
		Turn(Unpaid.PelvisBone, Pitch(14.f * Bent) * Roll(4.f * Rock));
		Turn(Unpaid.SpineBone, Pitch(8.f * Bent));
		Turn(Unpaid.HeadBone, Pitch(-10.f * Ease(Into)));
		Shift(Unpaid.PelvisBone, FVector(18.f * EaseOut(Into), 0.f, 0.f));
		break;
	}
	case EUnpaidRisePhase::Stand:
	{
		// Its forward drag given back as it straightens; its head shaking the dirt off as it comes up.
		const float Up = Ease(Into);
		Turn(Unpaid.PelvisBone, Pitch(14.f * 0.6f * (1.f - Up)));
		Shift(Unpaid.PelvisBone, FVector(18.f * (1.f - Up), 0.f, 0.f));
		const float Shake = FMath::Max(1.f - Into / 0.5f, 0.f);
		Turn(Unpaid.HeadBone, Yaw(6.f * Shake * FMath::Sin(Time * 21.f)) * Roll(3.f * Shake * FMath::Sin(Time * 17.f + 1.f)));
		break;
	}
	default:
		break;
	}
}

void FUnpaidRiseWork::ArmPose(const AUnpaidCreature& Unpaid, int32 Side, float& Swing, float& Out, float& Bend, float& Wrist, float& Curl)
{
	const FUnpaidRiseState& Rise = Unpaid.Kit.Rise;
	if (!Rise.IsRising() || Unpaid.IsDead())
	{
		return;
	}
	const bool bLead = Side == FMath::Clamp(Rise.LeadArm, 0, 1);
	const float Into = Rise.Into;
	const float Time = Rise.Time;
	// The arm's own pose through the climb (degrees, the rig's: below 0 swings it forward and up), and how much of it shows.
	float ToSwing = -20.f;
	float ToOut = 5.f;
	float ToBend = 40.f;
	float ToWrist = 0.f;
	float ToCurl = 50.f;
	float Weight = 1.f;
	// Planted on the ground in front and pushing: further forward-down the higher it gets.
	auto Planted = [&](float From)
	{
		const float Push = Ease((Into - From) / FMath::Max(1.f - From, 0.01f));
		ToSwing = FMath::Lerp(-95.f, -40.f, Push);
		ToOut = 12.f;
		ToBend = 25.f + 15.f * FMath::Abs(FMath::Sin(Into * UE_PI * 2.f));
		ToWrist = -25.f;
		ToCurl = 55.f;
	};
	switch (Rise.Phase)
	{
	case EUnpaidRisePhase::Buried:
		break;
	case EUnpaidRisePhase::Breach:
		if (bLead)
		{
			// Thrust straight up out of the dirt, trembling, the hand grasping at the air.
			const float Up = EaseOut(Into / 0.4f);
			ToSwing = FMath::Lerp(-60.f, -168.f, Up) + 4.f * FMath::Sin(Time * 31.f);
			ToOut = 8.f;
			ToBend = FMath::Lerp(60.f, 8.f, Up);
			ToWrist = 15.f;
			ToCurl = 20.f + 35.f * (0.5f + 0.5f * FMath::Sin(2.f * UE_PI * 3.2f * Time));
		}
		else
		{
			ToSwing = -30.f;
			ToBend = 50.f;
		}
		break;
	case EUnpaidRisePhase::Haul:
		if (bLead)
		{
			if (Into < UnpaidRiseRules::LeadPlantAt)
			{
				// Slammed down from overhead onto the ground in front.
				ToSwing = FMath::Lerp(-168.f, -95.f, EaseIn(Into / UnpaidRiseRules::LeadPlantAt));
				ToBend = 15.f;
				ToCurl = 40.f;
			}
			else
			{
				Planted(UnpaidRiseRules::LeadPlantAt);
			}
		}
		else if (Into < 0.2f)
		{
			// The other comes up out of the dirt...
			ToSwing = FMath::Lerp(-30.f, -150.f, EaseOut(Into / 0.2f));
			ToBend = 20.f;
			ToCurl = 30.f;
		}
		else if (Into < UnpaidRiseRules::OtherPlantAt)
		{
			// ...and slams down beside the first.
			ToSwing = FMath::Lerp(-150.f, -95.f, EaseIn((Into - 0.2f) / (UnpaidRiseRules::OtherPlantAt - 0.2f)));
			ToBend = 15.f;
			ToCurl = 40.f;
		}
		else
		{
			Planted(UnpaidRiseRules::OtherPlantAt);
		}
		break;
	case EUnpaidRisePhase::Stand:
		// Let go of the ground, flung out for its first breath, and handed back to the channels' arms.
		ToSwing = -15.f;
		ToOut = 35.f;
		ToBend = 30.f;
		ToWrist = 10.f;
		ToCurl = 10.f;
		Weight = 1.f - Ease(Into / ArmRelease);
		break;
	default:
		return;
	}
	Swing = FMath::Lerp(Swing, ToSwing, Weight);
	Out = FMath::Lerp(Out, ToOut, Weight);
	Bend = FMath::Lerp(Bend, ToBend, Weight);
	Wrist = FMath::Lerp(Wrist, ToWrist, Weight);
	Curl = FMath::Lerp(Curl, ToCurl, Weight);
}
