#pragma once

#include "CoreMinimal.h"

class APlayerController;

/**
 * The gamepad's look (the audit's change 7, 2026-11-03): the right stick as a turn in degrees per second, and the aim
 * assist that goes with it. Plain math first, so the tests drive every rule without a world; the one part that reads the
 * world (ForPlayer) only gathers the numbers and calls the math.
 *
 * The stick: a dead zone and a power curve on its length (small pushes aim finely, a full push turns fast), times a rate in
 * degrees per second and the frame's time. Before, ALooterCharacter::Look added the stick's raw value to the view each frame,
 * so the turn doubled at 120 fps. The mouse is none of this: its deltas are already a distance, not a speed.
 *
 * Aim assist, pad only (the mouse is never assisted): friction slows the look while the crosshair is within a creature's
 * capsule plus a margin, and magnetism pulls the view a little toward a creature the crosshair is nearly on, only while the
 * stick moves. Neither touches a dead, passive or hidden creature, nor one behind a wall (one sight line is traced for the
 * creature that would be assisted). Tunables are the Looter.Look.Pad* and Looter.AimAssist* cvars, read live.
 */
namespace AimAssist
{
	/** Every tunable's default, in one place (the cvars in AimAssist.cpp and FTuning start from these). */
	namespace Default
	{
		/** The stick's dead zone (its length, 0-1) and the curve's power past it (1 is linear). */
		inline constexpr float StickDeadZone = 0.18f;
		inline constexpr float StickCurve = 1.6f;
		/** A full push turns this many degrees a second, round and up/down; 0 gives the old per-frame stick back. */
		inline constexpr float YawDegreesPerSecond = 220.f;
		inline constexpr float PitchDegreesPerSecond = 160.f;
		/** The longest frame the stick turns for (s): a hitch must not spin the view. */
		inline constexpr float MaxStepSeconds = 0.1f;
		/** The share of the look kept while the crosshair is on a creature (the sensitivity times this). */
		inline constexpr float Friction = 0.6f;
		/** The magnet's pull (degrees a second, at most), the cone round the crosshair it works in (degrees), and the angle within which it eases off. */
		inline constexpr float MagnetDegreesPerSecond = 2.f;
		inline constexpr float MagnetConeDegrees = 2.f;
		inline constexpr float MagnetSettleDegrees = 0.25f;
		/** How far past a creature's capsule (cm) the crosshair still counts as on it, and how far away (cm) a creature is assisted. */
		inline constexpr float MarginCm = 20.f;
		inline constexpr float RangeCm = 4000.f;
		/** The creature list is found again at most this often (s). */
		inline constexpr float RefreshSeconds = 0.1f;
	}

	/** The tunables as they are now (the cvars, each held to a range that can't break the look). */
	struct FTuning
	{
		bool bEnabled = true;
		float StickDeadZone = Default::StickDeadZone;
		float StickCurve = Default::StickCurve;
		float YawDegreesPerSecond = Default::YawDegreesPerSecond;
		float PitchDegreesPerSecond = Default::PitchDegreesPerSecond;
		float MaxStepSeconds = Default::MaxStepSeconds;
		float Friction = Default::Friction;
		float MagnetDegreesPerSecond = Default::MagnetDegreesPerSecond;
		float MagnetConeDegrees = Default::MagnetConeDegrees;
		float MagnetSettleDegrees = Default::MagnetSettleDegrees;
		float MarginCm = Default::MarginCm;
		float RangeCm = Default::RangeCm;
		float RefreshSeconds = Default::RefreshSeconds;
	};

	/** The cvars now, clamped. */
	AI_LOOTER_SHOOTER_API FTuning LiveTuning();

	// --- The stick (plain math) ---

	/**
	 * The stick as a turn between 0 and 1 long: its length past DeadZone, rescaled to start at 0, then raised to Curve; its
	 * direction kept (the dead zone is round, so a diagonal keeps its angle). Inside the dead zone nothing; a push of 1 or
	 * more is 1. NaN gives nothing.
	 */
	AI_LOOTER_SHOOTER_API FVector2D ShapeStick(const FVector2D& Stick, float DeadZone, float Curve);

	/**
	 * Degrees a shaped stick turns in DeltaSeconds: its value times the per-second rates times the time, which is held to
	 * MaxStepSeconds. The signs are the stick's (the input mapping's), so they feed AddControllerYawInput / PitchInput as the
	 * stick's value did.
	 */
	AI_LOOTER_SHOOTER_API FVector2D StickTurn(const FVector2D& Shaped, float YawPerSecond, float PitchPerSecond, float DeltaSeconds, float MaxStepSeconds);

	/** Looking up and down reversed: Input with its Y flipped when bInvert (the mouse and the stick both pass through this). */
	AI_LOOTER_SHOOTER_API FVector2D InvertY(const FVector2D& Input, bool bInvert);

	/**
	 * The player controller multiplies every AddControllerYawInput / PitchInput by its legacy input scales (InputYawScale 2.5,
	 * InputPitchScale -2.5, switched on by DefaultInput.ini's bEnableLegacyInputScales); those are 1 and 1 with the setting
	 * off. The pair is what ForPlayer's callers divide out so that a number of degrees is that many degrees.
	 */
	AI_LOOTER_SHOOTER_API FVector2D LegacyInputScales(const APlayerController& Player);

	/** A turn in degrees with the stick's signs, as the input to hand AddController*Input: divided by the scales' size (the engine applies their signs). */
	AI_LOOTER_SHOOTER_API FVector2D DegreesToInput(const FVector2D& StickDegrees, const FVector2D& Scales);

	/** A turn as the view rotates (yaw right, pitch up) as the input to hand AddController*Input: divided by the scales, signs and all. */
	AI_LOOTER_SHOOTER_API FVector2D RotationToInput(const FVector2D& Rotation, const FVector2D& Scales);

	// --- Aim assist (plain math) ---

	/** A creature's capsule as the math sees it (world cm, upright). */
	struct FTarget
	{
		FVector Center = FVector::ZeroVector;
		float Radius = 0.f;
		/** Half the capsule's height, ends included. */
		float HalfHeight = 0.f;
	};

	/** Whether a creature may be assisted at all: alive, not held back (passive), not hidden, and solid (a corpse, a blinked-away Unpaid or one still in the ground is none). */
	AI_LOOTER_SHOOTER_API bool CanAssist(bool bDead, bool bPassive, bool bHidden, bool bSolid);

	/**
	 * How far (cm) the aim ray, from Eye along Aim for RangeCm, passes from the capsule's surface; 0 or less is through it. A
	 * large number for a capsule behind the eye or past the range.
	 */
	AI_LOOTER_SHOOTER_API float RayGap(const FVector& Eye, const FVector& Aim, const FTarget& Target, float RangeCm);

	/** The crosshair is within the capsule plus MarginCm. */
	AI_LOOTER_SHOOTER_API bool OnTarget(const FVector& Eye, const FVector& Aim, const FTarget& Target, float MarginCm, float RangeCm);

	/** The share of the look kept: Friction while on a creature, 1 off it. Held to 0.05-1, so a stick can't be frozen. */
	AI_LOOTER_SHOOTER_API float FrictionScale(bool bOnTarget, float Friction);

	/** The angle (degrees) between Aim and the way from Eye to Point. */
	AI_LOOTER_SHOOTER_API float AngleTo(const FVector& Eye, const FVector& Aim, const FVector& Point);

	/** The turn (degrees; yaw right, pitch up) that takes Aim onto Point. */
	AI_LOOTER_SHOOTER_API FVector2D TurnTo(const FVector& Eye, const FVector& Aim, const FVector& Point);

	/**
	 * The creature the magnet would pull toward: of those whose middle is within ConeDegrees of the crosshair and RangeCm of the
	 * eye, the one the crosshair is nearest to (the first listed on a tie). INDEX_NONE for none.
	 */
	AI_LOOTER_SHOOTER_API int32 PickMagnetTarget(const FVector& Eye, const FVector& Aim, TArrayView<const FTarget> Targets, float ConeDegrees, float RangeCm);

	/**
	 * This frame's pull toward Point (degrees; yaw right, pitch up): at most MagnetDps, only while the stick moves and Point is
	 * within ConeDegrees of the crosshair, easing to nothing as it nears the middle (inside SettleDegrees) so the crosshair
	 * arrives and stays instead of hunting, and never more than the way left (no overshoot).
	 */
	AI_LOOTER_SHOOTER_API FVector2D MagnetPull(const FVector& Eye, const FVector& Aim, const FVector& Point, bool bStickMoving,
		float MagnetDps, float ConeDegrees, float SettleDegrees, float DeltaSeconds);

	/** The creature list is due again: it has never been found (Last far in the past) or IntervalSeconds have passed. */
	AI_LOOTER_SHOOTER_API bool RefreshDue(double Now, double LastRefresh, float IntervalSeconds);

	// --- The player's frame ---

	/** What the assist does to this frame's pad turn. */
	struct FFrame
	{
		/** Times the look sensitivity: Friction on a creature, 1 elsewhere. */
		float FrictionScale = 1.f;
		/** The magnet's pull (degrees; yaw right, pitch up). */
		FVector2D Pull = FVector2D::ZeroVector;
		bool bOnTarget = false;
	};

	/**
	 * The assist for Player's look now. Nothing (scale 1, no pull) with Looter.AimAssist 0, the Aim assist setting off, no
	 * creature near the crosshair, or one the sight line to is blocked. Finds the creatures through TActorIterator at most
	 * every FTuning::RefreshSeconds and reads them from that list between; traces at most two sight lines.
	 */
	AI_LOOTER_SHOOTER_API FFrame ForPlayer(const APlayerController& Player, bool bStickMoving, float DeltaSeconds);
}
