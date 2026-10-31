#pragma once

#include "CoreMinimal.h"
#include "Combat/HitPose.h"

/**
 * The Unpaid's body language as plain rules (2026-10-31, the user: "their animations need improved upon"; UnpaidCreatureGait.cpp
 * lays them on the rig). The dead drift with no legs, so their "walk" is a lurch: each beat the hips heave forward and up and the
 * trunk is thrown onto that side, then it drags back and sags, one side weaker than the other (a limp), the head held level
 * against it the way a hunter keeps its eyes still. At rest they breathe they shouldn't, and their heads loll over and hang
 * there, now and then cracking over to one side; hunting, the head sets at a predator's tilt. A wind-up coils before its
 * release (the hips drawn back and down, a shiver as it's about to go), a rake throws the trunk through each swipe, a hit lolls
 * the head on a spring and stumbles the lurch, and a plain kill falls one of four ways (a slump, a last reach at its killer, a
 * twist away, a buckle). Angles in the rig's degrees (UnpaidCreatureRig.cpp: Pitch nods forward, Roll tips left, Yaw turns
 * right); lengths in cm at size 1. Plain functions and structs, so the tests check them without a world.
 */
namespace UnpaidGait
{
	/** One moment of the lurch, laid over the rig's pose. */
	struct AI_LOOTER_SHOOTER_API FLurch
	{
		/** The hips' surge forward over the capsule and their heave up (cm; below 0: the drag back and the sag). */
		float Surge = 0.f;
		float Heave = 0.f;
		/** The hips' pitch into the heave, the trunk's roll onto the heaving side, the chest's twist (the shoulder driving on). */
		float Pitch = 0.f;
		float Roll = 0.f;
		float Twist = 0.f;
		/** The head's counter (it keeps its eyes level): pitch against the heave, roll back against the trunk's. */
		float HeadPitch = 0.f;
		float HeadRoll = 0.f;
		/** Each arm's swing with the beat (degrees; below 0 forward): the heaving side's arm thrown forward, the other trailing. */
		float ArmSwing[2] = { 0.f, 0.f };
	};

	/** How fast its lurch cycles (Hz; two beats a cycle) at Speed (its drift against its chase speed, 0 to 1.3). */
	AI_LOOTER_SHOOTER_API float LurchHz(float Speed);

	/** One beat's push Into (0-1) through it: a quick heave up to 1 (by its first quarter or so), then a slow drag back to 0. */
	AI_LOOTER_SHOOTER_API float BeatShape(float Into);

	/**
	 * Its lurch at Phase (cycles; two beats a cycle, the first its strong side's) and Moving (0-1), with its Limp (0 even, 1 the
	 * weak beat half the strong one) and Side (+1: its right side heaves strong, -1 its left).
	 */
	AI_LOOTER_SHOOTER_API FLurch Lurch(float Phase, float Moving, float Limp, float Side);

	/** Its breath at rest Time seconds in: the chest's pitch (degrees; below 0 drawn back, an inhale) and the hips' rise (cm). */
	AI_LOOTER_SHOOTER_API FVector2f Breath(float Time, float Calm);

	// --- The head's tilt ---

	/** What its head's tilt is asked for. */
	enum class ETiltMood : uint8
	{
		/** Hanging about: the head lolls over and hangs a while, now and then cracking over. */
		Calm,
		/** On its prey: set at a predator's tilt, held, changing sides now and then. */
		Hunting,
		/** Attacking, rising, dying: level. */
		Level
	};

	/** Its head's tilt as it goes (StepTilt). */
	struct AI_LOOTER_SHOOTER_API FTilt
	{
		/** Its roll now and where it heads (degrees), its spring's speed (degrees a second). */
		float Roll = 0.f;
		float Goal = 0.f;
		float Speed = 0.f;
		/** Seconds until it picks a new tilt. */
		float Next = 1.5f;
		/** The goal is a neck's crack: snapped over on a stiff spring, with a jerk of the head's yaw (degrees, fading). */
		bool bCrack = false;
		float CrackYaw = 0.f;
		/** The mood its last goal was picked in (a new mood picks afresh). */
		ETiltMood Mood = ETiltMood::Level;
	};

	/** A calm loll's reach (degrees), how long it hangs (s), and the chance a new tilt is a neck's crack instead. */
	inline constexpr float CalmRoll[2] = { 6.f, 16.f };
	inline constexpr float CalmHold[2] = { 1.4f, 4.f };
	inline constexpr float CrackChance = 0.2f;
	inline constexpr float CrackRoll[2] = { 18.f, 26.f };
	inline constexpr float CrackJerk = 12.f;
	/** A hunting tilt's reach (degrees) and how long it holds one side (s). */
	inline constexpr float HuntRoll[2] = { 6.f, 11.f };
	inline constexpr float HuntHold[2] = { 2.5f, 5.f };

	/**
	 * Moves Tilt on by Dt in Mood. Three dice (0-1) settle its next goal when one is due: the side and how far, how long, and
	 * (calm) whether it's a crack. True on the frame a crack begins (its sound).
	 */
	AI_LOOTER_SHOOTER_API bool StepTilt(FTilt& Tilt, float Dt, ETiltMood Mood, float RollA, float RollB, float RollC);

	// --- Wind-ups and swipes ---

	/** A strike's coil Into (0-1) through its wind-up: drawn in over its first third, held, a last tighten before the release. */
	AI_LOOTER_SHOOTER_API float Coil(float Into);

	/** The shiver as it is about to go (0-1 of its strength): only through the wind-up's last fifth. */
	AI_LOOTER_SHOOTER_API float ReleaseShiver(float Into);

	/**
	 * The trunk's twist with a rake (degrees of yaw; above 0 to its right) from the rig's rake channels (each arm -1 drawn back
	 * and out, 1 swept across its front): wound away from the arm that's drawn, thrown on through each swipe.
	 */
	AI_LOOTER_SHOOTER_API float SwipeTwist(const float Rake[2]);

	// --- Hits ---

	/** A hit's head loll (degrees of roll, its own frame's push Away): the head tips the way it was pushed, harder for a harder hit. */
	AI_LOOTER_SHOOTER_API float HitHeadRoll(const FVector& Away, float Strength);

	/** How long (s) a hit of Strength stumbles its lurch: its beat held and its hips sagging. */
	AI_LOOTER_SHOOTER_API float HitStumble(float Strength);

	// --- Deaths ---

	/** How a plain kill falls (a kill style other than the plain one keeps its own look: UnpaidCreatureHurt.cpp). */
	enum class EFall : uint8
	{
		/** The slump and crumple it always had. */
		Slump,
		/** A last reach at its killer as it sinks, the arm on their side lifting, fingers opening, then dropping. */
		Reach,
		/** Twisted away from the shot as it crumples, one shoulder thrown round. */
		Twist,
		/** Its strings cut: it drops straight down, head falling over, arms hanging. */
		Buckle
	};

	/** The fall a plain kill takes: from behind it slumps or buckles; from the front any of the four (FromFront: -1 behind to 1 in front). */
	AI_LOOTER_SHOOTER_API EFall ChooseFall(float FromFront, float Roll);

	/** A fall's pose DeathTime seconds in, on top of the slump's (Side: +1 toward its right, -1 its left). */
	struct AI_LOOTER_SHOOTER_API FFallPose
	{
		/** The hips' turn (degrees) and their extra sink (cm). */
		float Yaw = 0.f;
		float Roll = 0.f;
		float Pitch = 0.f;
		float Sink = 0.f;
		/** The head's loll. */
		float HeadRoll = 0.f;
		float HeadPitch = 0.f;
		/** The reaching arm (0 left, 1 right, -1 none) and how far it reaches (0-1). */
		int32 Arm = -1;
		float Reach = 0.f;
	};

	AI_LOOTER_SHOOTER_API FFallPose FallPose(EFall Fall, float DeathTime, float Side);
}

/** What AUnpaidCreature keeps of its body language as it goes (its Kit.Gait; UnpaidCreatureGait.cpp). */
struct AI_LOOTER_SHOOTER_API FUnpaidGaitState
{
	/** Whether its body language shows this frame (Looter.Unpaid.Gait, its rig, no boss) and how much (the cvars, read once a frame). */
	bool bOn = false;
	float LurchScale = 1.f;
	float TiltScale = 1.f;
	/** Its lurch's clock (cycles), how much it shows (eased with its drift), its limp and its strong side. */
	float Phase = 0.f;
	float Move = 0.f;
	float Limp = 0.5f;
	float Side = 1.f;
	/** A hit's stumble: seconds left of its beat held. */
	float Stumble = 0.f;
	/** Its head's tilt, and a hit's loll on a spring (X: roll). */
	UnpaidGait::FTilt Tilt;
	FHitSpring HitLoll;
	/** The wind-up's coil as shown (eased). */
	float Coil = 0.f;
	/** How it falls (chosen as it dies), and to which side. */
	UnpaidGait::EFall Fall = UnpaidGait::EFall::Slump;
	float FallSide = 1.f;

	/** Back as it was made (a respawn); its limp and side are kept. */
	void Reset();
};
