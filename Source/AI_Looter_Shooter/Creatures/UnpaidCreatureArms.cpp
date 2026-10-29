// AUnpaidCreature's arms, over the trunk's pose (UnpaidCreatureRig.cpp). They used to hang almost still, moving as one
// piece with the chest (stiff, Main's tour 2026-10-08). Now they drift loose at rest (a twitch now and then), hang against
// the body's lean, come up and claw in turn as it hunts, fling wide and shake for the shriek, reach long and a little wide
// in the lunge, and lag its moves and hits on springs; a hit's flinch rises over a few frames instead of one. They keep
// out of the torso: the model's own hit hulls (PA_Unpaid) measure how deep each arm sits in the pelvis, waist, chest and
// the shroud's top links past where it sits at rest, and when a pose would push it deeper the upper arm moves (out, back or
// forward, whichever clears it) and eases back after; near the camera only. (Watching only the arm's inner side and the
// upper arm's elbow half let the shriek's rear press an armpit 3 cm into the waist.)
// Every angle is the rig's (UnpaidCreatureRig.cpp): Pitch swings a hanging arm back, Roll swings it out (by the arm's side).

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "ReferenceSkeleton.h"

namespace
{
	FQuat TurnBy(const FVector& Axis, float Degrees)
	{
		return FQuat(Axis, FMath::DegreesToRadians(Degrees));
	}

	FQuat PitchBy(float Degrees)
	{
		return TurnBy(FVector::RightVector, Degrees);
	}

	FQuat RollBy(float Degrees)
	{
		return TurnBy(FVector::ForwardVector, Degrees);
	}

	/** The forearm's bend baked into the rest pose (Unpaid.py's IDLE_BEND): the channels' bend comes back off it. */
	constexpr float RestBend = 12.f;
	/** Out from the body at rest beyond the model's hang (degrees): a little air between the sleeves and the vest. */
	constexpr float RestOut = 2.f;
	/** How much of the body's forward lean the arms hang back against (0 moves them with the chest). */
	constexpr float HangShare = 0.4f;
	/** The left arm runs this far out of step with the right (radians). */
	constexpr float LeftArmPhase = 2.1f;

	/** Hunting: the arms swing this far forward, and claw this much farther in turn, this many times a second. */
	constexpr float HuntReach = 24.f;
	constexpr float HuntClaw = 18.f;
	constexpr float ClawHz = 0.7f;

	/**
	 * The sway: a spring a little over once a second, under-damped so the arms float on. Acceleration swings them this many
	 * degrees per cm/s^2 (a 1200 cm/s^2 start trails them 10), a turn this many per degree a second; at most this far.
	 */
	constexpr float SwayHz = 1.1f;
	constexpr float SwayDamping = 0.45f;
	constexpr float SwayStep = 1.f / 120.f;
	constexpr float SwingPerAcceleration = 0.0085f;
	constexpr float OutPerAcceleration = 0.0085f;
	constexpr float OutPerTurn = 0.035f;
	constexpr float MaxSwaySwing = 20.f;
	constexpr float MaxSwayOut = 14.f;
	constexpr float MinSwayOut = -5.f;
	/** A lunge's launch or a phase-step is no shove: the acceleration the sway takes is capped (cm/s^2). */
	constexpr float MaxSwayAcceleration = 2400.f;

	/** A twitch: how hard (degrees a second) and how often (seconds between, at random). */
	constexpr float TwitchSwing = 150.f;
	constexpr float TwitchOut = 90.f;
	constexpr float TwitchEvery[2] = { 3.5f, 8.f };

	/** A hit flings the arms this hard (degrees a second, along the jolt), a crit wider too; kicks together go no faster than this. */
	constexpr float HitFling = 220.f;
	constexpr float CritStartle = 70.f;
	constexpr float MaxKickSpeed = 320.f;

	/** The flinch: its kick fades, and it follows on a critically damped spring, peaking 90 ms on at 0.65 of the kick (the gain makes up the rest). */
	constexpr float FlinchOmega = 40.f;
	constexpr float FlinchKickFade = 6.f;
	constexpr float FlinchKickGain = 1.54f;
	/** A hit's flinch kick tops out here (the pelvis bends 8 degrees per unit of flinch), and its arm fling at this many times HitFling. */
	constexpr float MaxFlinchKick = 3.8f;
	constexpr float MaxFlingStrength = 2.f;

	/**
	 * The guard: points spread evenly all round each arm bone's hull (upper arm, forearm, hand); an upper arm's within this
	 * share of its length of the shoulder sit in the chest it hangs from by design (the armpit's sleeve folds into it), so
	 * only the chest lets them in. A point may go this far (cm) past its rest depth before the arm moves; it moves out, back
	 * or forward (whichever clears it most), at most this far a pass and this far in all (degrees), easing back after.
	 */
	// The upper arm's hull has the most corners near the torso (its armpit against the waist in the shriek): with 28 points a
	// corner between them sat 2-3 cm deeper than any the guard watched, so it watches nearly all of them.
	constexpr float KeepOutTolerance = 0.5f;
	constexpr float MaxKeepTurn = 12.f;
	constexpr float MaxKeepOut = 35.f;
	constexpr float MaxKeepSwing = 25.f;
	constexpr float KeepOutRelax = 0.8f;
	constexpr int32 KeepOutPasses = 4;
}

// ---------------------------------------------------------------------------
// The pose
// ---------------------------------------------------------------------------

void AUnpaidCreature::KickArm(int32 Side, float SwingSpeed, float OutSpeed)
{
	if (Side >= 0 && Side < 2)
	{
		// A shotgun's pellets land together: their kicks add up only so far.
		ArmSway[Side].SwingSpeed = FMath::Clamp(ArmSway[Side].SwingSpeed + SwingSpeed, -MaxKickSpeed, MaxKickSpeed);
		ArmSway[Side].OutSpeed = FMath::Clamp(ArmSway[Side].OutSpeed + OutSpeed, -MaxKickSpeed, MaxKickSpeed);
	}
}

void AUnpaidCreature::ReactToHit(bool bCritical, float Strength)
{
	// The flinch: its kick (the spring's peak comes to about the old flinch's 0.6, 1 on a crit), sized by the hit (a
	// revolver round jolts it hard, a rifle round less; never past MaxFlinchKick: about 20 degrees at the peak). A shot
	// through its shroud's cloth jolts it less (Hurt.BodyShare, HITS: UnpaidCreatureHurt.cpp).
	FlinchKick = FMath::Max(FlinchKick, FMath::Min((bCritical ? 1.f : 0.6f) * FlinchKickGain * Strength * Hurt.BodyShare, MaxFlinchKick));
	// The body jolts along FlinchAway; the arms lag the other way: pushed back, they fly forward; pushed right, they swing left.
	// A shot on an arm flings that one hard and the other hardly (Hurt.ArmShare): the hit one always at least twice as far.
	float Flings[2];
	UnpaidBody::FHurtChannels::ArmFlings(HitFling * FMath::Min(Strength, MaxFlingStrength), Hurt.ArmShare, MaxKickSpeed, Flings);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const float ArmSide = Arms[Side].Side;
		KickArm(Side, Flings[Side] * static_cast<float>(FlinchAway.X),
			-ArmSide * Flings[Side] * static_cast<float>(FlinchAway.Y) + (bCritical ? CritStartle : 0.f));
	}
}

void AUnpaidCreature::StepFlinch(float DeltaSeconds)
{
	const float Seconds = FMath::Min(DeltaSeconds, FCreatureUpdateRate::MaxInterval);
	if (Seconds <= 0.f)
	{
		return;
	}
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / SwayStep - 0.01f));
	const float Step = Seconds / Steps;
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		FlinchKick -= FlinchKick * FMath::Min(FlinchKickFade * Step, 1.f);
		FlinchSpeed += (FlinchOmega * FlinchOmega * (FlinchKick - Flinch) - 2.f * FlinchOmega * FlinchSpeed) * Step;
		Flinch += FlinchSpeed * Step;
	}
}

void AUnpaidCreature::StepArmSway(float DeltaSeconds, const FVector& LocalAcceleration, float YawRate)
{
	const float Seconds = FMath::Min(DeltaSeconds, FCreatureUpdateRate::MaxInterval);
	if (Seconds <= 0.f)
	{
		return;
	}
	const float Omega = 2.f * UE_PI * SwayHz;
	const float Stiffness = Omega * Omega;
	const float Damping = 2.f * SwayDamping * Omega;
	// Short even steps: a distant one updating a few times a second sways as a near one does.
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / SwayStep - 0.01f));
	const float Step = Seconds / Steps;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		FArmSway& Sway = ArmSway[Side];
		const float ArmSide = Arms[Side].Side;
		// Setting off forward, they trail back; moved or turned to one side, they swing the other way.
		const float SwingTo = FMath::Clamp(static_cast<float>(LocalAcceleration.X) * SwingPerAcceleration, -MaxSwaySwing, MaxSwaySwing);
		const float OutTo = FMath::Clamp(-ArmSide * (static_cast<float>(LocalAcceleration.Y) * OutPerAcceleration + YawRate * OutPerTurn),
			MinSwayOut, MaxSwayOut);
		for (int32 Index = 0; Index < Steps; ++Index)
		{
			Sway.SwingSpeed += (Stiffness * (SwingTo - Sway.Swing) - Damping * Sway.SwingSpeed) * Step;
			Sway.Swing += Sway.SwingSpeed * Step;
			Sway.OutSpeed += (Stiffness * (OutTo - Sway.Out) - Damping * Sway.OutSpeed) * Step;
			Sway.Out += Sway.OutSpeed * Step;
		}
		Sway.Swing = FMath::Clamp(Sway.Swing, -MaxSwaySwing, MaxSwaySwing);
		Sway.Out = FMath::Clamp(Sway.Out, MinSwayOut, MaxSwayOut);
	}
}

void AUnpaidCreature::TurnArm(const FArm& Arm, float Swing, float Out, float Bend, float Wrist, float Curl)
{
	if (Arm.UpperArm != INDEX_NONE)
	{
		Bones[Arm.UpperArm].Own = PitchBy(Swing) * RollBy(Arm.Side * Out);
	}
	if (Arm.LowerArm != INDEX_NONE)
	{
		// The elbow bends forward only: a straight arm is as far as it goes.
		Bones[Arm.LowerArm].Own = PitchBy(-(FMath::Clamp(Bend, 0.f, 130.f) - RestBend));
	}
	if (Arm.Hand != INDEX_NONE)
	{
		Bones[Arm.Hand].Own = PitchBy(Wrist);
	}
	// A shot on this arm fans its fingers out for a moment (HITS).
	const float Splay = Pose.Splay + Hurt.SplayFor(Arm.Side < 0.f ? 0 : 1);
	for (int32 Finger = 0; Finger < Arm.Fingers.Num(); ++Finger)
	{
		// The curl and fan are about axes that don't commute, so the rest's idle comes off as one turn, after them.
		Bones[Arm.Fingers[Finger]].Own = TurnBy(Arm.CurlAxes[Finger], -Curl) * PitchBy(Arm.FanOffsets[Finger] * Splay)
			* Arm.IdleTurns[Finger].Inverse();
	}
}

void AUnpaidCreature::PoseArms(float DeltaSeconds, float Time, float Speed, const FVector& LocalVelocity, float YawRate)
{
	const float Moving = FMath::Min(Speed, 1.f);
	const float Alive = 1.f - Pose.Death;

	// The body's acceleration in its own frame swings the arms on their springs (none on a first or thawed frame).
	FVector Acceleration = FVector::ZeroVector;
	if (DeltaSeconds > 0.f)
	{
		Acceleration = ((LocalVelocity - LastLocalVelocity) / DeltaSeconds).GetClampedToMaxSize(MaxSwayAcceleration);
	}
	LastLocalVelocity = LocalVelocity;
	StepArmSway(DeltaSeconds, Acceleration * Alive, YawRate * Alive);

	// Now and then, while it hangs about, one arm twitches and settles.
	const bool bCalm = Alive > 0.99f && Pose.Hunt < 0.1f && Pose.Spread < 0.05f && Pose.Reach < 0.05f;
	if (bCalm && DeltaSeconds > 0.f)
	{
		NextTwitch -= DeltaSeconds;
		if (NextTwitch <= 0.f)
		{
			const float Strength = FMath::FRandRange(0.6f, 1.f);
			KickArm(FMath::RandRange(0, 1), -TwitchSwing * Strength, TwitchOut * Strength);
			NextTwitch = FMath::FRandRange(TwitchEvery[0], TwitchEvery[1]);
		}
	}
	// The keep-out eases off when it's no longer needed (and comes back at once when it is).
	if (DeltaSeconds > 0.f)
	{
		for (FArmSway& Sway : ArmSway)
		{
			Sway.KeepOut = FMath::FInterpTo(Sway.KeepOut, 0.f, DeltaSeconds, KeepOutRelax);
			Sway.KeepSwing = FMath::FInterpTo(Sway.KeepSwing, 0.f, DeltaSeconds, KeepOutRelax);
		}
	}

	const float Lean = FMath::Max(Pose.Lean, 0.f);
	const float Calm = (1.f - Pose.Hunt) * (1.f - Pose.Spread) * (1.f - Pose.Reach) * Alive;
	const float Hunt = Pose.Hunt * Alive;
	const float Tremble = Pose.Spread * Alive;
	float Swing[2];
	float OutBeforeKeepOut[2];
	float Bend[2];
	float Wrist[2];
	float Curl[2];
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FArmSway& Sway = ArmSway[Side];
		const float Phase = Side == 0 ? LeftArmPhase : 0.f;
		// Adrift: a slow, uneven sway out of step with the other arm, the elbow, wrist and fingers each in their own time.
		const float DriftSwing = 6.f * FMath::Sin(Time * 0.9f + Phase) + 2.5f * FMath::Sin(Time * 2.3f + 1.7f * Phase + 0.5f);
		const float DriftOut = 2.5f * FMath::Sin(Time * 0.7f + Phase + 0.6f);
		const float DriftBend = 5.f * FMath::Sin(Time * 1.1f + Phase + 0.4f);
		const float DriftWrist = 5.f * FMath::Sin(Time * 1.3f + Phase + 1.1f);
		const float DriftCurl = 6.f * FMath::Sin(Time * 0.8f + Phase + 2.f);
		// Hunting: up and reaching, each arm clawing in turn, half a beat apart.
		const float Claw = 0.5f + 0.5f * FMath::Sin(2.f * UE_PI * ClawHz * Time + (Side == 0 ? UE_PI : 0.f));
		// The shriek shakes the flung arms.
		const float Shake = Tremble * FMath::Sin(Time * 47.f + 3.f * Phase);

		Swing[Side] = Calm * DriftSwing
			+ HangShare * Lean * (1.f - Pose.Reach)
			+ 8.f * Moving * (1.f - Pose.Hunt) * (1.f - Pose.Reach)
			- Hunt * (HuntReach + HuntClaw * Claw)
			+ 15.f * Pose.Spread + 2.5f * Shake
			- 80.f * Pose.Reach + (Side == 0 ? -5.f : 5.f) * Pose.Reach
			+ 10.f * Pose.Death
			+ Sway.Swing;
		// Swung back past the hips, an arm swings a little wide of them.
		OutBeforeKeepOut[Side] = RestOut + Calm * DriftOut + 6.f * Moving + Hunt * (4.f + 5.f * Claw) + 55.f * Pose.Spread + 2.f * Shake
			+ 18.f * Pose.Reach + 0.25f * FMath::Max(Swing[Side], 0.f) + Sway.Out;
		Bend[Side] = RestBend + Calm * DriftBend + 14.f * Moving * (1.f - Pose.Hunt) * (1.f - Pose.Reach) + Hunt * (20.f + 18.f * Claw)
			+ 30.f * Pose.Spread - 6.f * Pose.Reach + 25.f * Pose.Death;
		Wrist[Side] = Calm * DriftWrist + 15.f * Pose.Spread - 12.f * Pose.Reach - Hunt * 12.f * Claw;
		Curl[Side] = Pose.Curl + Calm * DriftCurl + Hunt * 14.f * (Claw - 0.3f);
		// Its kit's rake (and the cinder's throw): drawn back and out wide, then swept forward from out there, so the claw
		// comes round across its front without crossing its own chest (the keep-out below holds it off anyway).
		const float Cocked = FMath::Max(-Pose.Rake[Side], 0.f) * Alive;
		const float Swept = FMath::Max(Pose.Rake[Side], 0.f) * Alive;
		Swing[Side] += 45.f * Cocked - 90.f * Swept;
		OutBeforeKeepOut[Side] += 45.f * Cocked;
		Bend[Side] += 25.f * Cocked - 10.f * Swept;
		Wrist[Side] += 20.f * Cocked - 18.f * Swept;
		Curl[Side] += 20.f * Swept - 25.f * Cocked;
		TurnArm(Arms[Side], Swing[Side] + Sway.KeepSwing, OutBeforeKeepOut[Side] + Sway.KeepOut, Bend[Side], Wrist[Side], Curl[Side]);
	}

	// The keep-out, near the camera: an arm the pose would push into the torso moves until it doesn't. Out from the body
	// clears a hand or an elbow at the hips; an armpit pressed into the waist (the shriek rears the chest back over it) may
	// clear only by swinging, so each pass tries out, back and forward and keeps whichever leaves it shallowest.
	if (TorsoGuards.IsEmpty() || GetUpdateInterval() > 0.f)
	{
		return;
	}
	for (int32 Pass = 0; Pass < KeepOutPasses; ++Pass)
	{
		SolvePose();
		bool bMoved = false;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			float Lever = 0.f;
			const float Past = ArmIntrusion(Side, &Lever);
			if (Past <= KeepOutTolerance)
			{
				continue;
			}
			FArmSway& Sway = ArmSway[Side];
			const float Turn = FMath::Clamp(1.1f * FMath::RadiansToDegrees(FMath::Atan2(Past, FMath::Max(Lever, 10.f))), 0.5f, MaxKeepTurn);
			const float Tries[3][2] = { { Turn, 0.f }, { 0.f, Turn }, { 0.f, -Turn } };
			float Best = Past;
			float BestOut = Sway.KeepOut;
			float BestSwing = Sway.KeepSwing;
			for (const float* Try : Tries)
			{
				const float Out = FMath::Min(Sway.KeepOut + Try[0], MaxKeepOut);
				const float Back = FMath::Clamp(Sway.KeepSwing + Try[1], -MaxKeepSwing, MaxKeepSwing);
				if (Out == Sway.KeepOut && Back == Sway.KeepSwing)
				{
					continue;
				}
				TurnArm(Arms[Side], Swing[Side] + Back, OutBeforeKeepOut[Side] + Out, Bend[Side], Wrist[Side], Curl[Side]);
				SolvePose();
				const float Now = ArmIntrusion(Side);
				if (Now < Best)
				{
					Best = Now;
					BestOut = Out;
					BestSwing = Back;
				}
			}
			bMoved |= Best < Past;
			Sway.KeepOut = BestOut;
			Sway.KeepSwing = BestSwing;
			TurnArm(Arms[Side], Swing[Side] + Sway.KeepSwing, OutBeforeKeepOut[Side] + Sway.KeepOut, Bend[Side], Wrist[Side], Curl[Side]);
		}
		if (!bMoved)
		{
			break;
		}
	}
}
