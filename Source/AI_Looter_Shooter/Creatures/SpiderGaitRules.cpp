#include "Creatures/SpiderGaitRules.h"

// The spider's gait rules (SpiderGaitRules.h says why each is so). Numbers are a full-size spider's.

namespace
{
	/** A step's time: this long standing, this much less per cm/s, never shorter than StepSecondsLeast. */
	constexpr float StepSecondsStill = 0.27f;
	constexpr float StepSecondsPerSpeed = 0.0002f;
	constexpr float StepSecondsLeast = 0.11f;
	/** How far a foot strays before it steps (cm): this standing, and this much farther per cm/s. */
	constexpr float StrayStill = 32.f;
	constexpr float StrayPerSpeed = 0.07f;
	/** A step's lift (cm): this standing, this much higher per cm/s. */
	constexpr float LiftStill = 18.f;
	constexpr float LiftPerSpeed = 0.028f;
	/** The foot rises this share of the step before it starts across, and is down this share before the step ends. */
	constexpr float LiftLead = 0.08f;
	/** The lift's arc is bent by this power so it peaks early (0.75: at about 0.4 of the step). */
	constexpr float LiftBend = 0.75f;

	/** The body riding its steps at a full run: its rise (cm) and yaw (degrees) over a group's swing. */
	constexpr float RideRise = 2.5f;
	constexpr float RideYaw = 3.f;
	/** Banking into a turn: degrees of roll per (degree a second x cm/s), never past BankMost. */
	constexpr float BankPerTurn = 0.00006f;
	constexpr float BankMost = 6.f;
	/** Leaning into a speed-up (nose down) and back on a stop: degrees per cm/s/s, never past LeanMost. */
	constexpr float LeanPerAccel = 0.0025f;
	constexpr float LeanMost = 5.f;
	/** A group planting: the body's dip (cm) standing and at a full run. */
	constexpr float PlantDipStill = 1.f;
	constexpr float PlantDipRun = 2.5f;
	/** A landing: no dip under this fall speed (cm/s), then this much per cm/s. */
	constexpr float LandingDipFrom = 150.f;
	constexpr float LandingDipPerSpeed = 0.022f;

	// The bite (cm and degrees at size 1).
	/** Coiled: the body this much down, its nose this far down, drawn back this far, its abdomen up, fangs parting. */
	constexpr float CoilRise = -9.f;
	constexpr float CoilPitch = -7.f;
	constexpr float CoilLunge = -6.f;
	constexpr float CoilAbdomen = 5.f;
	constexpr float CoilFang = 10.f;
	constexpr float CoilFrontRaise = 0.15f;
	/** Reared (the threat display): up, nose high, drawn back, abdomen down, fangs wide, front legs high and spread. */
	constexpr float RearRise = 20.f;
	constexpr float RearPitch = 30.f;
	constexpr float RearLunge = -16.f;
	constexpr float RearAbdomen = -8.f;
	constexpr float RearFang = 36.f;
	constexpr float RearFrontSpread = 1.55f;
	/** The tell's shiver: this fast (Hz), this much pitch (degrees), lift (cm) and fang flutter (degrees). */
	constexpr float ShiverHz = 22.f;
	constexpr float ShiverPitch = 1.6f;
	constexpr float ShiverRise = 0.8f;
	constexpr float ShiverFang = 4.f;
	/** The strike: out over StrikeOut of the recovery, back from StrikeBackFrom; reaching this far, nose this far down. */
	constexpr float StrikeOut = 0.28f;
	constexpr float StrikeBackFrom = 0.4f;
	constexpr float StrikeLunge = 42.f;
	constexpr float StrikePitch = -14.f;
	constexpr float StrikeRise = -6.f;
	constexpr float StrikeAbdomen = 4.f;
	/** The fangs snap shut over this much of the recovery. */
	constexpr float FangSnap = 0.12f;

	float Smooth01(float X)
	{
		const float T = FMath::Clamp(X, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

namespace SpiderGait
{
	float StepSeconds(float Speed)
	{
		return FMath::Clamp(StepSecondsStill - FMath::Max(Speed, 0.f) * StepSecondsPerSpeed, StepSecondsLeast, StepSecondsStill);
	}

	float StepStray(float Speed, float LegReach)
	{
		return FMath::Min(StrayStill + FMath::Max(Speed, 0.f) * StrayPerSpeed, MaxStrayShare * FMath::Max(LegReach, 1.f));
	}

	float StepHeight(float Speed)
	{
		return LiftStill + FMath::Max(Speed, 0.f) * LiftPerSpeed;
	}

	float StepAlong(float Alpha)
	{
		// Off the ground first, then across; down onto its spot just before the step ends (a smootherstep in between).
		const float T = FMath::Clamp((Alpha - LiftLead) / (1.f - 2.f * LiftLead), 0.f, 1.f);
		return T * T * T * (T * (T * 6.f - 15.f) + 10.f);
	}

	float StepLift(float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.f, 1.f);
		return FMath::Sin(UE_PI * FMath::Pow(T, LiftBend));
	}

	float TurnSpeed(float YawRate, float RestRadius)
	{
		return FMath::Abs(FMath::DegreesToRadians(YawRate)) * FMath::Max(RestRadius, 0.f);
	}

	FBodyMotion BodyMotion(float GroupAlpha, float GroupSide, float PaceShare, float YawRate, float ForwardAccel, float Speed)
	{
		FBodyMotion Motion;
		const float Pace = FMath::Clamp(PaceShare, 0.f, 1.f);
		if (GroupAlpha >= 0.f)
		{
			// Over a group's swing the body is carried up a little and turned toward the reaching front leg.
			const float Swing = FMath::Sin(UE_PI * FMath::Clamp(GroupAlpha, 0.f, 1.f));
			Motion.Rise = RideRise * Pace * Swing;
			Motion.Yaw = RideYaw * Pace * Swing * (GroupSide < 0.f ? -1.f : 1.f);
		}
		Motion.Roll = FMath::Clamp(YawRate * FMath::Max(Speed, 0.f) * BankPerTurn, -BankMost, BankMost);
		Motion.Pitch = FMath::Clamp(-ForwardAccel * LeanPerAccel, -LeanMost, LeanMost);
		return Motion;
	}

	float PlantDip(float PaceShare)
	{
		return FMath::Lerp(PlantDipStill, PlantDipRun, FMath::Clamp(PaceShare, 0.f, 1.f));
	}

	float LandingDip(float FallSpeed)
	{
		return FMath::Clamp((FallSpeed - LandingDipFrom) * LandingDipPerSpeed, 0.f, LandingDipMost);
	}

	FBitePose BitePose(float WindupShare, float StrikeShare, float ShiverClock)
	{
		FBitePose Pose;
		const float Windup = FMath::Clamp(WindupShare, 0.f, 1.f);
		const float Strike = FMath::Clamp(StrikeShare, 0.f, 1.f);
		if (Strike <= 0.f)
		{
			if (Windup < CoilShare)
			{
				// Coiling: down and back, nose down, the abdomen lifting behind, the fangs parting.
				const float Coil = Smooth01(Windup / CoilShare);
				Pose.Rise = CoilRise * Coil;
				Pose.Pitch = CoilPitch * Coil;
				Pose.Lunge = CoilLunge * Coil;
				Pose.Abdomen = CoilAbdomen * Coil;
				Pose.Fang = CoilFang * Coil;
				Pose.FrontRaise = CoilFrontRaise * Coil;
			}
			else
			{
				// Rearing into its threat display: up and back, nose high, the front legs raised and spread wide, fangs open.
				const float Rear = FMath::InterpEaseInOut(0.f, 1.f, (Windup - CoilShare) / (1.f - CoilShare), 2.f);
				Pose.Rise = FMath::Lerp(CoilRise, RearRise, Rear);
				Pose.Pitch = FMath::Lerp(CoilPitch, RearPitch, Rear);
				Pose.Lunge = FMath::Lerp(CoilLunge, RearLunge, Rear);
				Pose.Abdomen = FMath::Lerp(CoilAbdomen, RearAbdomen, Rear);
				Pose.Fang = FMath::Lerp(CoilFang, RearFang, Rear);
				Pose.FrontRaise = FMath::Lerp(CoilFrontRaise, 1.f, Rear);
				Pose.FrontSpread = FMath::Lerp(1.f, RearFrontSpread, Rear);
			}
			if (ShiverClock >= 0.f)
			{
				// The tell: a quick tremor through it, the fangs fluttering, just before it goes.
				const float Envelope = Smooth01(ShiverClock / (ShiverSeconds * 0.3f));
				const float Tremor = FMath::Sin(2.f * UE_PI * ShiverHz * ShiverClock) * Envelope;
				Pose.Pitch += ShiverPitch * Tremor;
				Pose.Rise += ShiverRise * Tremor;
				Pose.Fang += ShiverFang * FMath::Abs(Tremor);
			}
			return Pose;
		}
		// The strike: thrown forward fast, nose down, fangs snapping shut; then drawn back to its stance.
		const float Out = 1.f - FMath::Pow(1.f - FMath::Clamp(Strike / StrikeOut, 0.f, 1.f), 3.f);
		const float Hold = 1.f - Smooth01((Strike - StrikeBackFrom) / (1.f - StrikeBackFrom));
		Pose.Lunge = FMath::Lerp(RearLunge, StrikeLunge, Out) * Hold;
		Pose.Pitch = FMath::Lerp(RearPitch, StrikePitch, Out) * Hold;
		Pose.Rise = FMath::Lerp(RearRise, StrikeRise, Out) * Hold;
		Pose.Abdomen = FMath::Lerp(RearAbdomen, StrikeAbdomen, Out) * Hold;
		Pose.Fang = RearFang * (1.f - Smooth01(Strike / FangSnap));
		Pose.FrontRaise = 1.f - Out;
		Pose.FrontSpread = FMath::Lerp(RearFrontSpread, 1.f, Out);
		return Pose;
	}
}
