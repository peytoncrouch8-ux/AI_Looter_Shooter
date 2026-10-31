// The Stone Sentinel's poses as plain curves (StoneSentinelRules.h, StoneSentinelMoveRules.h): standing, the slam, its
// waking, its stride, the throw, the shockwave and its reel. Each move's keys are what its rigid Tripo pieces bend to cleanly
// (arms to about 128 / 70, a bow to about 28, a twist of 14, a spread of 0.42; Docs/Pipeline/Enemies.md "The Stone Sentinel"):
// past them its shoulders split into shards. The weight is in the timing: slow climbs that gather speed, fast falls, holds.

#include "Creatures/StoneSentinelMoveRules.h"
#include "Creatures/StoneSentinelRules.h"

namespace
{
	FStoneSentinelPose MakePose(float Arm, float Elbow, float Bow, float Twist = 0.f, float Spread = 0.f, float Lean = 0.f, float Crouch = 0.f,
		float HeadPitch = 0.f)
	{
		FStoneSentinelPose Pose;
		Pose.ArmSwing = Arm;
		Pose.ElbowBend = Elbow;
		Pose.Bow = Bow;
		Pose.Twist = Twist;
		Pose.Spread = Spread;
		Pose.Lean = Lean;
		Pose.Crouch = Crouch;
		Pose.HeadPitch = HeadPitch;
		return Pose;
	}

	// Standing awake, and the slam (fists overhead, fists in the dirt: Raised and Down as cut for the Tripo pieces, 2026-10-30).
	const FStoneSentinelPose Idle = MakePose(8.f, 12.f, 4.f);
	const FStoneSentinelPose Raised = MakePose(128.f, 70.f, -8.f, 0.f, 0.f, 0.f, 0.f, 8.f);
	const FStoneSentinelPose Down = MakePose(45.f, 20.f, 22.f, 0.f, 0.f, 0.f, 4.f, -6.f);

	// Waking: a stretch free of the years, then its drop into its stance.
	const FStoneSentinelPose Stretch = MakePose(24.f, 6.f, -9.f, 0.f, 0.35f, 0.f, 0.f, 12.f);
	const FStoneSentinelPose Settle = MakePose(10.f, 16.f, 9.f, 0.f, 0.f, 0.f, 4.f, -3.f);
	constexpr float StretchFrom = 0.5f;
	constexpr float StretchAt = 0.82f;

	// The throw: hands to the ground, the rock overhead, drawn back wound round, the release, the follow-through.
	const FStoneSentinelPose Reach = MakePose(40.f, 18.f, 26.f, 0.f, 0.f, 0.f, 5.f, -8.f);
	const FStoneSentinelPose Overhead = MakePose(124.f, 62.f, -6.f, 0.f, 0.f, 0.f, 0.f, 10.f);
	const FStoneSentinelPose Drawn = MakePose(128.f, 70.f, -12.f, 14.f, 0.f, -3.f, 0.f, 12.f);
	const FStoneSentinelPose Release = MakePose(70.f, 14.f, 18.f, -10.f, 0.f, 2.f, 2.f, 0.f);
	const FStoneSentinelPose Follow = MakePose(52.f, 12.f, 20.f, -8.f, 0.f, 0.f, 3.f, -2.f);
	/** It grips the rock this long at the bottom before it heaves (s). */
	constexpr float GripSeconds = 0.1f;
	/** The follow-through's share of the recovery. */
	constexpr float FollowShare = 0.3f;

	// The shockwave: reared back with its arms spread wide, quivering at the top, both fists in the ground.
	const FStoneSentinelPose Rear = MakePose(118.f, 38.f, -12.f, 0.f, 0.42f, 0.f, 0.f, 14.f);
	const FStoneSentinelPose RearTop = MakePose(122.f, 40.f, -14.f, 0.f, 0.42f, 0.f, 0.f, 15.f);
	const FStoneSentinelPose Pound = MakePose(34.f, 10.f, 28.f, 0.f, 0.3f, 0.f, 6.f, -6.f);
	/** The share of the rise it takes to rear up; the rest it holds at the top, swelling a little. */
	constexpr float RearShare = 0.75f;

	// The reel: from the rock's grip, arms flung up round its head as it bursts, sagging, then standing.
	const FStoneSentinelPose Flung = MakePose(96.f, 54.f, -14.f, 6.f, 0.1f, 5.f, 0.f, -14.f);
	const FStoneSentinelPose Sag = MakePose(30.f, 22.f, 16.f, -4.f, 0.f, -3.f, 4.f, -10.f);
	constexpr float FlungShare = 0.15f;
	constexpr float SagShare = 0.55f;

	// The stride (degrees and cm at a full walk; StoneSentinelRules::GaitAt).
	/** The share of a cycle each foot stays planted: stone doesn't glide, it plants and heaves the other leg through. */
	constexpr float StanceShare = 0.62f;
	constexpr float ThighSwing = 16.f;
	constexpr float KneeBend = 22.f;
	/** A planted knee gives a little under the weight. */
	constexpr float StanceKnee = 0.15f;
	constexpr float ArmSwingDegrees = 7.f;
	constexpr float HipDrop = 3.f;
	constexpr float SwayDegrees = 3.5f;
	constexpr float TwistDegrees = 5.f;
	constexpr float LurchDegrees = 2.5f;
	/** How long a strike's drop and lurch last (a share of the cycle, their curve's width). */
	constexpr float StrikeWidth = 0.09f;
	constexpr float LurchWidth = 0.12f;

	float Smooth(float Alpha)
	{
		return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
	}

	/** Share of the way from From to To that Time is (0-1). */
	float Along(float Time, float From, float To)
	{
		return FMath::Clamp((Time - From) / FMath::Max(To - From, 0.001f), 0.f, 1.f);
	}

	/** A strike's bump: 1 at the foot's strike (Q 0, its own phase), falling off over Width either side. */
	float StrikeBump(float Q, float Width)
	{
		const float Off = FMath::Min(Q, 1.f - Q);
		return FMath::Exp(-FMath::Square(Off / FMath::Max(Width, 0.001f)));
	}
}

namespace StoneSentinelRules
{
	FStoneSentinelPose BlendPose(const FStoneSentinelPose& A, const FStoneSentinelPose& B, float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.f, 1.f);
		FStoneSentinelPose Out;
		Out.ArmSwing = FMath::Lerp(A.ArmSwing, B.ArmSwing, T);
		Out.ElbowBend = FMath::Lerp(A.ElbowBend, B.ElbowBend, T);
		Out.Bow = FMath::Lerp(A.Bow, B.Bow, T);
		Out.Twist = FMath::Lerp(A.Twist, B.Twist, T);
		Out.Spread = FMath::Lerp(A.Spread, B.Spread, T);
		Out.Lean = FMath::Lerp(A.Lean, B.Lean, T);
		Out.Crouch = FMath::Lerp(A.Crouch, B.Crouch, T);
		Out.HeadPitch = FMath::Lerp(A.HeadPitch, B.HeadPitch, T);
		return Out;
	}

	FStoneSentinelPose IdlePose()
	{
		return Idle;
	}

	FStoneSentinelPose SlamPoseAt(float AttackTime, const FStoneSentinelTuning& Tuning)
	{
		const float Windup = FMath::Max(Tuning.SlamWindup, 0.05f);
		const float Fall = FMath::Clamp(Tuning.SlamFallSeconds, 0.01f, Windup * 0.5f);
		const float Heave = Windup - Fall;
		if (AttackTime < Heave)
		{
			// Up slowly, faster at the top: the tell is the fists climbing.
			return BlendPose(Idle, Raised, Smooth(AttackTime / Heave));
		}
		if (AttackTime < Windup)
		{
			// Down hard: it gathers speed all the way to the ground.
			return BlendPose(Raised, Down, FMath::Square((AttackTime - Heave) / Fall));
		}
		const float Recovery = FMath::Max(Tuning.SlamRecovery, 0.05f);
		const float Hold = Recovery * FMath::Clamp(Tuning.SlamHoldShare, 0.f, 1.f);
		const float After = AttackTime - Windup;
		if (After < Hold)
		{
			return Down;
		}
		return BlendPose(Down, Idle, Smooth((After - Hold) / FMath::Max(Recovery - Hold, 0.01f)));
	}

	FStoneSentinelPose WakePoseAt(float WakeTime, float WakeSeconds)
	{
		const float S = FMath::Clamp(WakeTime / FMath::Max(WakeSeconds, 0.01f), 0.f, 1.f);
		if (S < StretchFrom)
		{
			return Idle;
		}
		if (S < StretchAt)
		{
			return BlendPose(Idle, Stretch, Smooth(Along(S, StretchFrom, StretchAt)));
		}
		// The drop into its stance comes fast, as its weight is its own again.
		return BlendPose(Stretch, Settle, FMath::Square(Along(S, StretchAt, 1.f)));
	}

	FStoneSentinelGait GaitAt(float Phase)
	{
		FStoneSentinelGait Gait;
		const float P = FMath::Frac(Phase);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			// Each foot's own phase: 0 as it strikes (the left at 0, the right half a cycle on).
			const float Q = FMath::Frac(P - (Side == 0 ? 0.f : 0.5f));
			float Swing;
			float Knee;
			if (Q < StanceShare)
			{
				// Planted: the body walks over it, so it sweeps from in front to behind; the knee gives a little mid-way.
				const float U = Q / StanceShare;
				Swing = FMath::Lerp(1.f, -1.f, U);
				Knee = StanceKnee * FMath::Sin(PI * U);
			}
			else
			{
				// Heaved through: quick, the knee bent most mid-swing, the foot set down ahead.
				const float U = (Q - StanceShare) / (1.f - StanceShare);
				Swing = FMath::Lerp(-1.f, 1.f, Smooth(U));
				Knee = FMath::Sin(PI * U);
			}
			Gait.Thigh[Side] = Swing * ThighSwing;
			Gait.Knee[Side] = Knee * KneeBend;
			// Its arm swings against its own leg.
			Gait.Arm[Side] = -Swing * ArmSwingDegrees;
			Gait.Drop = FMath::Max(Gait.Drop, HipDrop * StrikeBump(Q, StrikeWidth));
			Gait.Lurch = FMath::Max(Gait.Lurch, LurchDegrees * StrikeBump(Q, LurchWidth));
		}
		// Its weight over the planted foot (the left planted through the cycle's first half: a lean to its left), its shoulders
		// turning against its legs (the left foot forward at 0 brings the right shoulder forward).
		const float Cycle = 2.f * PI * P;
		Gait.Roll = -SwayDegrees * FMath::Sin(Cycle);
		Gait.Twist = -TwistDegrees * FMath::Cos(Cycle);
		return Gait;
	}
}

namespace StoneSentinelMoves
{
	FStoneSentinelPose ThrowPoseAt(float AttackTime, const FSentinelBoulderTuning& Boulder)
	{
		// Its keys in order, each after the last whatever the tuning says.
		const float Windup = FMath::Max(Boulder.Windup, 0.5f);
		const float TearTime = FMath::Clamp(Boulder.TearAt, 0.05f, Windup * 0.6f);
		const float GripTime = FMath::Min(TearTime + GripSeconds, Windup * 0.65f);
		const float LiftedTime = FMath::Clamp(Boulder.LiftedAt, GripTime + 0.05f, Windup * 0.8f);
		const float DrawnTime = FMath::Clamp(Boulder.DrawnAt, LiftedTime + 0.05f, Windup - 0.05f);
		const float T = AttackTime;
		if (T < TearTime)
		{
			return StoneSentinelRules::BlendPose(Idle, Reach, Smooth(T / TearTime));
		}
		if (T < GripTime)
		{
			return Reach;
		}
		if (T < LiftedTime)
		{
			// The heave: slow off the ground under the rock's weight, faster as it comes up past its chest.
			return StoneSentinelRules::BlendPose(Reach, Overhead, Smooth(FMath::Pow(Along(T, GripTime, LiftedTime), 1.3f)));
		}
		if (T < DrawnTime)
		{
			return StoneSentinelRules::BlendPose(Overhead, Drawn, Smooth(Along(T, LiftedTime, DrawnTime)));
		}
		if (T < Windup)
		{
			// The throw gathers speed all the way to the release.
			return StoneSentinelRules::BlendPose(Drawn, Release, FMath::Square(Along(T, DrawnTime, Windup)));
		}
		const float Recovery = FMath::Max(Boulder.Recovery, 0.1f);
		const float FollowEnd = Windup + Recovery * FollowShare;
		if (T < FollowEnd)
		{
			return StoneSentinelRules::BlendPose(Release, Follow, Smooth(Along(T, Windup, FollowEnd)));
		}
		return StoneSentinelRules::BlendPose(Follow, Idle, Smooth(Along(T, FollowEnd, Windup + Recovery)));
	}

	FStoneSentinelPose ShockPoseAt(float AttackTime, const FSentinelShockTuning& Shock)
	{
		const float Windup = FMath::Max(Shock.Windup, 0.3f);
		const float Fall = FMath::Clamp(Shock.FallSeconds, 0.02f, Windup * 0.5f);
		const float Rise = Windup - Fall;
		const float Reared = Rise * RearShare;
		const float T = AttackTime;
		if (T < Reared)
		{
			return StoneSentinelRules::BlendPose(Idle, Rear, Smooth(T / Reared));
		}
		if (T < Rise)
		{
			return StoneSentinelRules::BlendPose(Rear, RearTop, Smooth(Along(T, Reared, Rise)));
		}
		if (T < Windup)
		{
			return StoneSentinelRules::BlendPose(RearTop, Pound, FMath::Square(Along(T, Rise, Windup)));
		}
		const float Recovery = FMath::Max(Shock.Recovery, 0.1f);
		const float Hold = Recovery * FMath::Clamp(Shock.HoldShare, 0.f, 1.f);
		const float After = T - Windup;
		if (After < Hold)
		{
			return Pound;
		}
		// Prising its fists free and standing: slow, heavy.
		return StoneSentinelRules::BlendPose(Pound, Idle, Smooth(Along(After, Hold, Recovery)));
	}

	FStoneSentinelPose ReelPoseAt(float Time, float Seconds)
	{
		const float S = FMath::Clamp(Time / FMath::Max(Seconds, 0.1f), 0.f, 1.f);
		if (S < FlungShare)
		{
			return StoneSentinelRules::BlendPose(Overhead, Flung, Smooth(S / FlungShare));
		}
		if (S < SagShare)
		{
			return StoneSentinelRules::BlendPose(Flung, Sag, Smooth(Along(S, FlungShare, SagShare)));
		}
		return StoneSentinelRules::BlendPose(Sag, Idle, Smooth(Along(S, SagShare, 1.f)));
	}
}
