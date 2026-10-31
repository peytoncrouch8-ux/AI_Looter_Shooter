#include "Creatures/UnpaidGaitRules.h"

// The Unpaid's body language as plain rules (2026-10-31): the lurch it drifts with, its breath and head's tilt at rest, the
// coil of a wind-up, a rake's twist, a hit's loll and stumble, and how a plain kill falls. UnpaidCreatureGait.cpp lays them on.

namespace
{
	float EaseOut(float Alpha)
	{
		return FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	float EaseIn(float Alpha)
	{
		return FMath::InterpEaseIn(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	float EaseInOut(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	float Between(const float Range[2], float Roll)
	{
		return FMath::Lerp(Range[0], Range[1], FMath::Clamp(Roll, 0.f, 1.f));
	}

	/** The lurch's walk and chase rates (Hz, two beats a cycle): about 0.9 beats a second drifting, 1.7 at its chase. */
	constexpr float WalkHz = 0.42f;
	constexpr float ChaseHz = 0.85f;
	/** A beat heaves up over this share of it, then drags back over the rest. */
	constexpr float HeaveShare = 0.28f;
	/** A beat's mean push (BeatShape over a beat): the surge and heave swing about it, so the lurch doesn't creep forward. */
	constexpr float MeanBeat = 0.55f;
	/** At full drift: the hips' surge and heave (cm), pitch, roll and the chest's twist (degrees), the arms' swing (degrees). */
	constexpr float SurgeCm = 9.f;
	constexpr float HeaveCm = 3.5f;
	constexpr float LurchPitch = 4.f;
	constexpr float LurchRoll = 5.f;
	constexpr float LurchTwist = 7.f;
	constexpr float HeadCounter = 0.6f;
	constexpr float ArmLead = 10.f;
	constexpr float ArmTrail = 5.f;

	/** Its breath: about one in four seconds, the chest drawn back this far (degrees), the hips lifted this far (cm). */
	constexpr float BreathHz = 0.27f;
	constexpr float BreathPitch = 1.8f;
	constexpr float BreathRise = 0.8f;

	/** The tilt's springs: a calm loll slow and soft, a hunting tilt firmer, a crack stiff and under-damped (it snaps over). */
	constexpr float CalmOmega = 3.2f;
	constexpr float HuntOmega = 7.f;
	constexpr float CrackOmega = 28.f;
	constexpr float SoftZeta = 0.9f;
	constexpr float CrackZeta = 0.42f;
	constexpr float TiltStep = 1.f / 120.f;
	/** A crack's yaw jerk fades at this rate (1/s). */
	constexpr float CrackYawFade = 2.f;

	/** The coil is drawn in over this share of the wind-up, tightened again over its last share. */
	constexpr float CoilIn = 0.35f;
	constexpr float CoilTighten = 0.85f;
	constexpr float CoilExtra = 0.25f;
	constexpr float ShiverFrom = 0.8f;

	/** A rake's twist: this many degrees per unit of the rake channels, at most this far. */
	constexpr float SwipeDegrees = 24.f;
	constexpr float MaxSwipe = 30.f;

	/** A hit's loll: degrees per unit of push sideways and of strength (strength counted to 2), at most this far. */
	constexpr float LollPerUnit = 14.f;
	constexpr float MaxLoll = 24.f;
}

// ---------------------------------------------------------------------------
// The lurch
// ---------------------------------------------------------------------------

float UnpaidGait::LurchHz(float Speed)
{
	return FMath::Lerp(WalkHz, ChaseHz, FMath::Clamp(Speed, 0.f, 1.f));
}

float UnpaidGait::BeatShape(float Into)
{
	const float At = FMath::Clamp(Into, 0.f, 1.f);
	if (At < HeaveShare)
	{
		return EaseOut(At / HeaveShare);
	}
	return 1.f - EaseInOut((At - HeaveShare) / (1.f - HeaveShare));
}

UnpaidGait::FLurch UnpaidGait::Lurch(float Phase, float Moving, float Limp, float Side)
{
	FLurch Out;
	const float Drift = FMath::Clamp(Moving, 0.f, 1.f);
	if (Drift <= 0.f)
	{
		return Out;
	}
	const float Cycle = Phase - FMath::FloorToFloat(Phase);
	const bool bStrong = Cycle < 0.5f;
	const float Into = (bStrong ? Cycle : Cycle - 0.5f) * 2.f;
	// The weak beat drags: up to half the strong one.
	const float Strength = bStrong ? 1.f : 1.f - 0.5f * FMath::Clamp(Limp, 0.f, 1.f);
	const float Beat = BeatShape(Into) * Strength * Drift;
	// About the cycle's mean (both beats', so nothing jumps where one beat hands over to the next).
	const float Mean = MeanBeat * (1.f - 0.25f * FMath::Clamp(Limp, 0.f, 1.f)) * Drift;
	// The heaving side: its strong side on the first beat, the other on the second.
	const float Heaving = (bStrong ? 1.f : -1.f) * (Side >= 0.f ? 1.f : -1.f);
	Out.Surge = SurgeCm * (Beat - Mean);
	Out.Heave = HeaveCm * (Beat - Mean);
	Out.Pitch = LurchPitch * Beat;
	// Thrown onto the heaving side: its top tips that way (a roll to the left is positive), and that shoulder drives on (the
	// chest turns away from it).
	Out.Roll = -LurchRoll * Beat * Heaving;
	Out.Twist = -LurchTwist * Beat * Heaving;
	Out.HeadPitch = -HeadCounter * Out.Pitch;
	Out.HeadRoll = -HeadCounter * Out.Roll;
	for (int32 Arm = 0; Arm < 2; ++Arm)
	{
		const float ArmSide = Arm == 0 ? -1.f : 1.f;
		Out.ArmSwing[Arm] = ArmSide == Heaving ? -ArmLead * Beat : ArmTrail * Beat;
	}
	return Out;
}

FVector2f UnpaidGait::Breath(float Time, float Calm)
{
	const float Share = FMath::Clamp(Calm, 0.f, 1.f);
	// Uneven: a slow draw with a catch in it, as no living breath goes.
	const float Draw = FMath::Sin(2.f * UE_PI * BreathHz * Time) + 0.3f * FMath::Sin(2.f * UE_PI * BreathHz * 3.1f * Time + 0.7f);
	return FVector2f(-BreathPitch * Draw * Share, BreathRise * FMath::Sin(2.f * UE_PI * BreathHz * Time) * Share);
}

// ---------------------------------------------------------------------------
// The head's tilt
// ---------------------------------------------------------------------------

bool UnpaidGait::StepTilt(FTilt& Tilt, float Dt, ETiltMood Mood, float RollA, float RollB, float RollC)
{
	bool bCracked = false;
	if (Mood != Tilt.Mood)
	{
		// A new mood picks afresh at once (straightening as an attack begins, setting its tilt as the hunt does).
		Tilt.Mood = Mood;
		Tilt.Next = 0.f;
	}
	Tilt.Next -= FMath::Max(Dt, 0.f);
	if (Mood == ETiltMood::Level)
	{
		Tilt.Goal = 0.f;
		Tilt.bCrack = false;
	}
	else if (Tilt.Next <= 0.f)
	{
		const float Way = RollA < 0.5f ? -1.f : 1.f;
		const float Reach = FMath::Frac(FMath::Clamp(RollA, 0.f, 0.9999f) * 2.f);
		if (Mood == ETiltMood::Calm)
		{
			Tilt.bCrack = RollC < CrackChance;
			if (Tilt.bCrack)
			{
				Tilt.Goal = Way * Between(CrackRoll, Reach);
				Tilt.CrackYaw = Way * CrackJerk;
				bCracked = true;
			}
			else
			{
				// Now and then it straightens a moment before it lolls again.
				Tilt.Goal = Way * Between(CalmRoll, Reach) * (RollC > 0.8f ? 0.25f : 1.f);
			}
			Tilt.Next = Between(CalmHold, RollB);
		}
		else
		{
			Tilt.bCrack = false;
			Tilt.Goal = Way * Between(HuntRoll, Reach);
			Tilt.Next = Between(HuntHold, RollB);
		}
	}
	// On its spring, in even steps so it acts the same at any frame rate (a hitch takes at most half a second).
	const float Omega = Tilt.bCrack ? CrackOmega : (Mood == ETiltMood::Hunting ? HuntOmega : CalmOmega);
	const float Zeta = Tilt.bCrack ? CrackZeta : SoftZeta;
	const float Seconds = FMath::Clamp(Dt, 0.f, 0.5f);
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / TiltStep - 0.01f));
	const float Step = Seconds / Steps;
	for (int32 Index = 0; Index < Steps && Step > 0.f; ++Index)
	{
		Tilt.Speed += (Omega * Omega * (Tilt.Goal - Tilt.Roll) - 2.f * Zeta * Omega * Tilt.Speed) * Step;
		Tilt.Roll += Tilt.Speed * Step;
	}
	Tilt.CrackYaw = FMath::FInterpTo(Tilt.CrackYaw, 0.f, Seconds, CrackYawFade);
	return bCracked;
}

// ---------------------------------------------------------------------------
// Wind-ups and swipes
// ---------------------------------------------------------------------------

float UnpaidGait::Coil(float Into)
{
	const float At = FMath::Clamp(Into, 0.f, 1.f);
	if (At < CoilIn)
	{
		return EaseOut(At / CoilIn);
	}
	if (At < CoilTighten)
	{
		return 1.f;
	}
	return 1.f + CoilExtra * EaseIn((At - CoilTighten) / (1.f - CoilTighten));
}

float UnpaidGait::ReleaseShiver(float Into)
{
	return Into < ShiverFrom ? 0.f : EaseIn((FMath::Min(Into, 1.f) - ShiverFrom) / (1.f - ShiverFrom));
}

float UnpaidGait::SwipeTwist(const float Rake[2])
{
	// The left arm sweeps across to its right (the trunk follows it round to the right), the right arm to its left; an arm
	// drawn back winds the trunk the other way first.
	return FMath::Clamp(SwipeDegrees * (Rake[0] - Rake[1]), -MaxSwipe, MaxSwipe);
}

// ---------------------------------------------------------------------------
// Hits
// ---------------------------------------------------------------------------

float UnpaidGait::HitHeadRoll(const FVector& Away, float Strength)
{
	// Pushed to its right (Away.Y above 0), its head tips right: a roll to the right is negative.
	return FMath::Clamp(-LollPerUnit * static_cast<float>(Away.Y) * FMath::Clamp(Strength, 0.f, 2.f), -MaxLoll, MaxLoll);
}

float UnpaidGait::HitStumble(float Strength)
{
	return Strength < 0.3f ? 0.f : FMath::Clamp(0.12f + 0.12f * Strength, 0.f, 0.4f);
}

// ---------------------------------------------------------------------------
// Deaths
// ---------------------------------------------------------------------------

UnpaidGait::EFall UnpaidGait::ChooseFall(float FromFront, float Roll)
{
	if (FromFront < -0.3f)
	{
		// Shot from behind: it falls away from the shooter, never reaching back at one it can't see.
		return Roll < 0.55f ? EFall::Slump : EFall::Buckle;
	}
	if (Roll < 0.38f)
	{
		return EFall::Reach;
	}
	if (Roll < 0.62f)
	{
		return EFall::Twist;
	}
	return Roll < 0.84f ? EFall::Buckle : EFall::Slump;
}

UnpaidGait::FFallPose UnpaidGait::FallPose(EFall Fall, float DeathTime, float Side)
{
	FFallPose Pose;
	const float T = FMath::Max(DeathTime, 0.f);
	const float Way = Side >= 0.f ? 1.f : -1.f;
	switch (Fall)
	{
	case EFall::Reach:
	{
		// Up at its killer in a quarter of a second, held, then the arm drops as it breaks up; its head lolls the other way.
		Pose.Arm = Way > 0.f ? 1 : 0;
		Pose.Reach = EaseOut(T / 0.3f) * (1.f - 0.75f * EaseInOut((T - 0.65f) / 0.45f));
		Pose.Pitch = 6.f * Pose.Reach;
		Pose.HeadPitch = -12.f * Pose.Reach;
		Pose.HeadRoll = -Way * 14.f * EaseOut(T / 0.5f);
		Pose.Sink = -10.f * EaseInOut((T - 0.4f) / 0.8f);
		break;
	}
	case EFall::Twist:
		Pose.Yaw = Way * 55.f * EaseOut(T / 0.55f);
		Pose.Roll = -Way * 12.f * EaseOut(T / 0.6f);
		Pose.HeadRoll = Way * 16.f * EaseOut(T / 0.45f);
		break;
	case EFall::Buckle:
		// Dropped as if its strings were cut: falling faster and faster, then folding over.
		Pose.Sink = -28.f * EaseIn(T / 0.28f);
		Pose.Pitch = 14.f * EaseOut((T - 0.15f) / 0.5f);
		Pose.HeadRoll = Way * 26.f * EaseOut((T - 0.05f) / 0.35f);
		Pose.HeadPitch = 22.f * EaseOut(T / 0.4f);
		break;
	default:
		break;
	}
	return Pose;
}

void FUnpaidGaitState::Reset()
{
	const float KeptLimp = Limp;
	const float KeptSide = Side;
	*this = FUnpaidGaitState();
	Limp = KeptLimp;
	Side = KeptSide;
}
