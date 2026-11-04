#pragma once

#include "CoreMinimal.h"
#include "Player/ViewKick.h"

/**
 * The camera answering the body's movement (core-loop audit change 6, 2026-11-03: "the camera is dead", only the gun bobbed):
 * a landing dips the view and the eye by how hard it came down (nothing under a step), a jump kicks it up, every footfall
 * of the stride nods it (and, in a sprint, rolls it a touch to that foot's side, in phase with the gun's own bob), a strafe
 * leans it into the move, and a mantle or vault lands with a dip. All plain math over numbers, so the tests drive it with
 * no world; UPlayerLocomotionComponent plays it (PlayerLocomotionCameraMotion.cpp): the turns as FViewKick on the camera
 * modifier (UCameraShakeModifier, which scales them by the Camera shake setting), the eye dip and the lean read per frame
 * (scaled by the setting there).
 *
 * Every number is a Looter.Camera.Motion.* variable read when asked (Live); Looter.Camera.Motion 0 is today's dead camera.
 * Degrees are the view's turn (pitch up, roll clockwise), centimetres the eye's drop.
 */
namespace CameraMotion
{
	namespace Default
	{
		inline constexpr float Master = 1.f;

		// A landing: under LandMinSpeed (cm/s, UPlayerFootfallComponent::SoftLandSpeed: a curb's hop, a step down) nothing; from
		// there the view dips LandPitchMin degrees and the eye LandDropMin cm, rising to the max at LandFullSpeed
		// (HardLandSpeed) and held beyond.
		inline constexpr float LandMinSpeed = 300.f;
		inline constexpr float LandFullSpeed = 1000.f;
		inline constexpr float LandPitchMin = 0.4f;
		inline constexpr float LandPitchMax = 1.6f;
		inline constexpr float LandDropMin = 1.f;
		inline constexpr float LandDropMax = 4.f;
		/** The view's dip is a spring (Hz, damping 0-1: snaps down in two frames, settles in ~0.3 s). */
		inline constexpr float LandHertz = 4.5f;
		inline constexpr float LandDamping = 0.55f;
		/** The eye's dip reaches its depth this long (s) after the landing, on a curve that starts at rest, and is gone in ~8x that. */
		inline constexpr float LandDropSeconds = 0.06f;

		// The jump's kick up (degrees) and its spring.
		inline constexpr float JumpPitch = 0.3f;
		inline constexpr float JumpHertz = 6.f;
		inline constexpr float JumpDamping = 0.55f;

		// The footfall's nod (degrees): walking and at a full sprint, its spring (settled before the next step, ~0.3 s apart),
		// and the sprint's roll to the foot's side.
		inline constexpr float StepBobWalk = 0.15f;
		inline constexpr float StepBobSprint = 0.35f;
		inline constexpr float StepHertz = 5.5f;
		inline constexpr float StepDamping = 0.6f;
		inline constexpr float SprintSway = 0.2f;

		// The strafe's lean (degrees at a full-speed strafe, into the move) and how fast it comes and goes (1/s).
		inline constexpr float StrafeRoll = 0.6f;
		inline constexpr float StrafeRate = 8.f;

		// A mantle's and a vault's landing dip (degrees); the eye already gives through a move (FPlayerTraversal), so the view only nods.
		inline constexpr float MantleDip = 0.5f;
		inline constexpr float VaultDip = 0.8f;

		/** Through the sights all of it keeps this share, so the reticle stays on what it is on (the gun bob keeps 15%). */
		inline constexpr float AimedShare = 0.25f;

		/** Never, whatever the variables say: the eye drops this far at most (cm), with this many dips at once. */
		inline constexpr float MaxEyeDrop = 6.f;
		inline constexpr int32 MaxEyeDips = 4;
	}

	/** Every number the rules read: the shipped defaults, or Live(), the variables as set now. */
	struct FTuning
	{
		/** 0: no camera motion at all (today's camera). */
		float Master = Default::Master;
		float LandMinSpeed = Default::LandMinSpeed;
		float LandFullSpeed = Default::LandFullSpeed;
		float LandPitchMin = Default::LandPitchMin;
		float LandPitchMax = Default::LandPitchMax;
		float LandDropMin = Default::LandDropMin;
		float LandDropMax = Default::LandDropMax;
		float LandHertz = Default::LandHertz;
		float LandDamping = Default::LandDamping;
		float LandDropSeconds = Default::LandDropSeconds;
		float JumpPitch = Default::JumpPitch;
		float JumpHertz = Default::JumpHertz;
		float JumpDamping = Default::JumpDamping;
		float StepBobWalk = Default::StepBobWalk;
		float StepBobSprint = Default::StepBobSprint;
		float StepHertz = Default::StepHertz;
		float StepDamping = Default::StepDamping;
		float SprintSway = Default::SprintSway;
		float StrafeRoll = Default::StrafeRoll;
		float StrafeRate = Default::StrafeRate;
		float MantleDip = Default::MantleDip;
		float VaultDip = Default::VaultDip;
		float AimedShare = Default::AimedShare;
	};

	/** The variables as set now (read each time, never cached, so a change in play shows on the next frame). */
	AI_LOOTER_SHOOTER_API FTuning Live();

	// --- The rules (plain functions; a kick with nothing in it, IsNone, when the master is off or the case calls for none) ---

	/** A landing from FallSpeed (cm/s, the fastest the fall went) is felt: the master is on and it is not a step's drop. */
	AI_LOOTER_SHOOTER_API bool LandingFelt(float FallSpeed, const FTuning& Tuning);
	/** The dip's size in degrees (0 when not felt): LandPitchMin at LandMinSpeed up to LandPitchMax at LandFullSpeed. */
	AI_LOOTER_SHOOTER_API float LandingPitch(float FallSpeed, const FTuning& Tuning);
	/** The eye's drop in cm (0 when not felt), scaled the same way. */
	AI_LOOTER_SHOOTER_API float LandingDrop(float FallSpeed, const FTuning& Tuning);
	/** The view's dip as a kick: pitch down by LandingPitch. */
	AI_LOOTER_SHOOTER_API FViewKick LandingKick(float FallSpeed, const FTuning& Tuning);
	/** Leaving the ground: the view tips up JumpPitch. */
	AI_LOOTER_SHOOTER_API FViewKick JumpKick(const FTuning& Tuning);
	/** A mantle's or vault's landing: the view dips (a vault, which lands on the far side, more). */
	AI_LOOTER_SHOOTER_API FViewKick TraversalKick(bool bVault, const FTuning& Tuning);
	/**
	 * One footfall: the view nods down StepBobWalk degrees walking and StepBobSprint at a full sprint (SprintAlpha 0-1), and in
	 * a sprint rolls SprintSway toward Side (+1 right foot, -1 left: the side the gun's bob sways to at that moment). MoveWeight
	 * is the view model's own measure of how fast the feet go (0 standing .. 1 a walk, held at 1 here), so a creep nods less.
	 */
	AI_LOOTER_SHOOTER_API FViewKick StepKick(float SprintAlpha, float MoveWeight, float Side, const FTuning& Tuning);
	/** The lean (degrees, signed: + into a right strafe) for LateralSpeed (cm/s, + to the right) against the walk's speed. */
	AI_LOOTER_SHOOTER_API float StrafeTarget(float LateralSpeed, float WalkSpeed, const FTuning& Tuning);
	/** How much of the motion shows at AimAlpha (0 hip .. 1 sights up): 1 at the hip, AimedShare through the sights. */
	AI_LOOTER_SHOOTER_API float AimedShare(float AimAlpha, const FTuning& Tuning);
	/** Seconds after the spring (Hz, damping) is kicked that its first swing peaks: a footfall's kick is given this early. */
	AI_LOOTER_SHOOTER_API float PeakSeconds(float Hertz, float Damping);
	/** The eye dip's curve: 0 at Age 0 and at rest there, 1 at AttackSeconds, back to ~0 in eight times that. Never below 0. */
	AI_LOOTER_SHOOTER_API float DipShape(float Age, float AttackSeconds);
}

/** The eye's drop and the view's roll this frame, already scaled (the Camera shake setting, the sights). */
struct FCameraMotionPose
{
	/** Cm the eye is lower than its curve says. */
	float EyeDrop = 0.f;
	/** Degrees of roll on top of the slide's. */
	float Roll = 0.f;
};

/**
 * The state behind the camera's motion that the rules above can't hold: the eye's dips under way, the lean it has eased to,
 * and the last reading of the gun bob's stride clock, so footfalls are found in the same clock the gun bobs by.
 */
class AI_LOOTER_SHOOTER_API FCameraMotion
{
public:
	/** Forgets everything under way (the lean too). */
	void Reset();
	/** Drops the dips under way, keeps the lean and the stride clock (the master switched off in play). */
	void ClearDips() { Dips.Reset(); }

	/** A landing's dip of the eye: Depth cm, reached AttackSeconds after (CameraMotion::DipShape). Nothing for a depth under 0. */
	void AddEyeDip(float Depth, float AttackSeconds);
	/** Ages the dips and drops those that are over. */
	void Advance(float DeltaSeconds);
	/** The eye's drop now (cm), the dips together, held within Default::MaxEyeDrop. */
	float GetEyeDrop() const;

	/** Eases the lean toward Target (degrees) at Rate (1/s) and returns it. */
	float EaseLean(float Target, float Rate, float DeltaSeconds);
	float GetLean() const { return Lean; }

	/**
	 * Feeds the gun bob's stride clock (StepPhase, 0 to 2 pi: each pi is one step, its lowest dip at pi/2 into the step, the
	 * sway to the right in the first half of the stride) once a frame, DeltaSeconds after the last. True on the frame to give
	 * the footfall's kick: PeakSeconds before the dip's lowest point, so the view's nod peaks as the gun's bob does. OutSide
	 * is +1 for the right foot (the gun is swaying right then) and -1 for the left. False on the first reading, when the
	 * clock stood still or jumped.
	 */
	bool StepFootfall(float StepPhase, float DeltaSeconds, float PeakSeconds, float& OutSide);

private:
	struct FDip
	{
		float Depth = 0.f;
		float Age = 0.f;
		float Attack = 0.06f;
	};
	TArray<FDip, TInlineAllocator<CameraMotion::Default::MaxEyeDips>> Dips;
	float Lean = 0.f;
	/** The stride clock as last read (negative: none yet). */
	float LastPhase = -1.f;
};
