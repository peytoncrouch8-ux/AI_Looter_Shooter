#pragma once

#include "CoreMinimal.h"

/**
 * The brown spider's gait as plain numbers (the user, 2026-10-31: "their animations need improved upon"; ASpiderCreature walks
 * it in SpiderCreature.cpp and SpiderCreatureGait.cpp). Every distance is a full-size spider's (cm): the spider scales them by
 * its size. Kept apart from the actor so the tests (Looter.Creatures.Spider.Gait.*) check the rules without a world.
 *
 * What makes it read as weighty rather than skating (the big-studio creature look we're after):
 * - a foot is lifted off quickly, carried on an arc that peaks early, and set down firmly (StepAlong, StepLift);
 * - feet step before their leg straightens: a foot may stray at most MaxStrayShare of its leg's reach from its spot, a
 *   trailing one steps at OverreachShare, and no leg is ever posed past MaxReachShare (the old 0.92/0.98 and the solve's
 *   0.999 let legs go straight: long spikes at a run);
 * - the body rides the steps: it rises a little as a group swings and dips as it plants, yaws toward the side whose front
 *   leg is reaching, pitches into a speed-up and back on a stop, and banks into a turn (BodyMotion);
 * - turning on the spot steps the feet round as walking would (TurnSpeed), so a turn never twists the legs in place;
 * - a bite coils (body down, nose down), rears into a threat display (front legs high and spread, fangs wide), shivers just
 *   before it strikes (the tell), lunges and snaps shut, and pulls back (BitePose);
 * - a fall ends in a dip the size of the fall (LandingDip).
 */
/** A spider's gait as it goes (ASpiderCreature keeps one; SpiderCreatureGait.cpp moves it on). */
struct FSpiderGaitState
{
	/** Its turn rate (degrees a second) and speed-up along its facing (cm/s/s), and the frame's yaw and velocity they were read from. */
	float YawRate = 0.f;
	float ForwardAccel = 0.f;
	float LastYaw = 0.f;
	FVector LastVelocity = FVector::ZeroVector;
	bool bRead = false;
	/** The stepping group's progress (below 0: none) and its front leg's side; a plant's dip (cm at size 1, fading). */
	float GroupAlpha = -1.f;
	float GroupSide = 1.f;
	float PlantDip = 0.f;
	/** A landing's dip (cm at size 1) on its spring, the speed it last fell at, whether it was in the air, and since it landed (s). */
	float LandDip = 0.f;
	float LandDipSpeed = 0.f;
	float FallSpeed = 0.f;
	bool bWasAirborne = false;
	float SinceLanded = 10.f;
};

namespace SpiderGait
{
	/**
	 * No leg is ever posed past this share of its full length (the solve draws a farther foot short): a clear knee bend at
	 * all times (2026-10-31, Main and ART: SK_Spider's legs are the rig's 230 cm with Tripo's bends; a straight leg reads as a
	 * spike). A trailing foot this far into its reach steps with its group's next turn (OverreachShare); this far, at once
	 * (SevereOverreachShare), before the solve has to draw it short.
	 */
	constexpr float MaxReachShare = 0.9f;
	constexpr float OverreachShare = 0.82f;
	constexpr float SevereOverreachShare = 0.88f;
	/** A foot never strays farther from its spot than this share of its leg's reach before its group steps it back. */
	constexpr float MaxStrayShare = 0.25f;

	/** Seconds one step takes at Speed (cm/s): quicker at speed. */
	AI_LOOTER_SHOOTER_API float StepSeconds(float Speed);
	/** How far (cm) a foot may stray from its spot before its group steps it back at Speed, for a leg reaching LegReach. */
	AI_LOOTER_SHOOTER_API float StepStray(float Speed, float LegReach);
	/** How high a step lifts its foot (cm) at Speed. */
	AI_LOOTER_SHOOTER_API float StepHeight(float Speed);
	/** A step's share of its way along at Alpha (0 to 1): off its spot quickly, eased onto the new one. */
	AI_LOOTER_SHOOTER_API float StepAlong(float Alpha);
	/** A step's lift at Alpha (0 to 1 of its height): peaks early (about 0.4 in) and comes down firmly. */
	AI_LOOTER_SHOOTER_API float StepLift(float Alpha);
	/**
	 * The speed the gait walks at for a turn on the spot: a turn at YawRate (degrees a second) moves a foot RestRadius (cm)
	 * out from the middle this fast, and its steps are timed and sized as a walk that fast would be.
	 */
	AI_LOOTER_SHOOTER_API float TurnSpeed(float YawRate, float RestRadius);

	/** What the gait adds to the body this frame: its rise (cm), its yaw, roll and pitch (degrees). */
	struct FBodyMotion
	{
		float Rise = 0.f;
		float Yaw = 0.f;
		float Roll = 0.f;
		float Pitch = 0.f;
	};

	/**
	 * The body riding its steps. GroupAlpha: the stepping group's progress (below 0: all feet down); GroupSide: the side of
	 * that group's front leg (-1 left, +1 right); PaceShare: its speed against its run (0 to 1); YawRate (degrees a second,
	 * + to its right); ForwardAccel (cm/s/s along its facing); Speed (cm/s).
	 */
	AI_LOOTER_SHOOTER_API FBodyMotion BodyMotion(float GroupAlpha, float GroupSide, float PaceShare, float YawRate, float ForwardAccel,
		float Speed);

	/** A footfall's jolt: how far (cm) the body dips as a group plants at PaceShare of its run. */
	AI_LOOTER_SHOOTER_API float PlantDip(float PaceShare);
	/** A landing's dip (cm) from the speed it fell at (cm/s, down): bigger for a harder landing, never past LandingDipMost. */
	AI_LOOTER_SHOOTER_API float LandingDip(float FallSpeed);
	constexpr float LandingDipMost = 26.f;

	/**
	 * The bite's body (the body's frame, cm and degrees): Rise and Pitch on its stance, Lunge along its facing, Fang (the
	 * spread, degrees), FrontRaise (0 to 1: how far the front legs are up toward their raised spot), FrontSpread (how far out
	 * they are held, a share of their hip's side), Abdomen (its tilt, degrees: down as the front rears).
	 */
	struct FBitePose
	{
		float Rise = 0.f;
		float Pitch = 0.f;
		float Lunge = 0.f;
		float Fang = 0.f;
		float FrontRaise = 0.f;
		float FrontSpread = 1.f;
		float Abdomen = 0.f;
	};

	/** The coil's share of the wind-up (then it rears), and the tell's shiver over its last ShiverSeconds (s). */
	constexpr float CoilShare = 0.3f;
	constexpr float ShiverSeconds = 0.14f;

	/**
	 * The bite at WindupShare (0 to 1 of its wind-up) and StrikeShare (0 to 1 of its recovery, 0 until it strikes);
	 * ShiverClock: seconds into the wind-up's last ShiverSeconds (below 0: not yet), whose tremor it adds.
	 */
	AI_LOOTER_SHOOTER_API FBitePose BitePose(float WindupShare, float StrikeShare, float ShiverClock);
}
