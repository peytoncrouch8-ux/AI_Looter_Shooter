#pragma once

#include "CoreMinimal.h"
#include "Player/PlayerMeleeToolTypes.h"
#include "Player/ViewKick.h"

/** Where a swing has the gun: off its hold in its own frame (cm; X along the barrel, Y right, Z up) and turned (degrees). */
struct FMeleeSwingPose
{
	FVector Offset = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
};

/**
 * One swing style's arc (MeleeMotion::ArcFor), in the first-person camera's space (X ahead, Y right, Z up; cm): the right
 * arm turns about Axis through the shoulder, the hand Reach out along ContactDir at the blow. The wind-up has the arm turned
 * CockDegrees round (its sign says which side), and after the blow it goes on to ThroughDegrees: past the blow (the other
 * sign) for a follow-through, or back the cock's way for a yank (the gaff). The wrist lays the head back behind the hand
 * by WristDegrees at the wind-up's top and snaps it through at the blow (the whip that reads as weight), rolling over by
 * half as much after. The reach's share at the wind-up and after (CockReach, ThroughReach): a jab draws the hand in and
 * drives it out, a yank pulls it back. The handle points HandleAtContact at the blow; the striking side is the way the head
 * moves then, or StrikeHint when that runs along the handle (a jab's does).
 */
struct FMeleeArc
{
	FVector Shoulder = FVector::ZeroVector;
	FVector Axis = FVector::UpVector;
	FVector ContactDir = FVector::ForwardVector;
	float Reach = 50.f;
	float CockDegrees = 70.f;
	float ThroughDegrees = -50.f;
	float WristDegrees = 35.f;
	float CockReach = 1.f;
	float ThroughReach = 1.f;
	FVector HandleAtContact = FVector::UpVector;
	FVector StrikeHint = FVector(0.f, -1.f, 0.f);
	/** The striking side is StrikeHint always (a punch: the back of the fist up, whichever way the arm turns). */
	bool bFaceHint = false;
	/**
	 * How much the tool is rolled about its handle to show its face to the eye through the arc (0: its striking side leads
	 * the motion; 1: faces the eye): a flat blade swung flat reads edge on, a spear point (DEMO, round 4: Ellis's spade).
	 */
	float FaceView = 0.f;
};

/** The tool on the right arm at a moment of the swing (MeleeMotion::ToolPose). */
struct FMeleeToolPose
{
	/**
	 * Its frame in the camera's space: the origin the grip point (a forearm tool's: the fist's), X up the handle (a forearm
	 * tool's: the punch's way), Z its striking side (the back of the fist), Y the palm's way onto the handle.
	 */
	FTransform Grip = FTransform::Identity;
	/** How much of the right hand is on the tool (1) rather than on the gun's grip (0). */
	float HandShare = 0.f;
	/** A forearm tool's piston driven out past the fist (cm along X): it kicks at the blow. */
	float Push = 0.f;
	/** The tool is drawn. */
	bool bShown = false;
};

/**
 * How a melee strike moves, done in code like recoil (no animation assets): the hero's tool swung on the right arm (the
 * user's note 5 and picks, 2026-10-27), the gun dropped into the left hand meanwhile, and the view's lean into the blow,
 * through the shared camera modifier (UCameraShakeModifier) so the player's camera shake setting scales it and the aim
 * never moves.
 *
 * Over FMeleeRules' 0.5 s, the blow at 0.15 s:
 *  - The draw and wind-up (to CockSeconds): the right hand lets go of the grip and comes up from under the frame with the
 *    tool, cocked back on the swing's far side, the head laid back behind the hand.
 *  - The strike (to the blow): the arm swung through, accelerating, the wrist snapping the head through at the blow.
 *  - The blow: one that lands stops dead there (the striker's hit-stop holds the clock) and bounces back a little; a miss
 *    goes on to the arc's through (past the blow, or a gaff's yank back).
 *  - The return (ReturnFrom to the end): the tool goes back down under the frame and the hand back to the grip.
 *  - The gun meanwhile: dropped out of the frame in 0.08 s (AwayPose) and hidden there, back up over the last 0.08 s; the
 *    left hand rests under the frame (the first-person hands with no gun shown).
 * A heft (quick, steady, heavy) scales the wind-up and the jolts, never the clock. The timings are named constants here (the
 * third-person jab reads CockSeconds too: LooterStancePose::MeleeJab); the sizes and shares are Looter.Melee.* console
 * variables (PlayerMeleeToolMotion.cpp), the arcs named constants there.
 */
namespace MeleeMotion
{
	/**
	 * When the tool is fully wound up (seconds into the swing): the draw up from under the frame and the coil. Most of the
	 * 0.15 s before the blow, so the wind-up is seen; the drive after it is short, so the head is at its fastest as it lands.
	 */
	inline constexpr float CockSeconds = 0.09f;
	/** How soon a miss has gone through (seconds after the blow), and a landed blow's bounce back off the body. */
	inline constexpr float FollowThroughSeconds = 0.1f;
	inline constexpr float RecoilSeconds = 0.07f;
	/** When the tool starts back down under the frame (seconds into the swing): after the follow-through has been seen. */
	inline constexpr float ReturnFrom = 0.3f;
	/** How long the right hand takes to leave the grip for the tool, and to come back to it at the end (seconds). */
	inline constexpr float HandSeconds = 0.04f;

	/** The right shoulder every arc swings from (camera space): under the eye, a little back and out to the right. */
	AI_LOOTER_SHOOTER_API FVector ShoulderPoint();

	/** A heft's scale on the wind-up's size and the view's jolts: quick 0.8, steady 1, heavy 1.25 (Looter.Melee.Heft.*). */
	AI_LOOTER_SHOOTER_API float HeftScale(EMeleeHeft Heft);

	/** Each style's arc (named constants with their reasons, PlayerMeleeToolMotion.cpp), sized by Looter.Melee.ArcScale and the heft. */
	AI_LOOTER_SHOOTER_API FMeleeArc ArcFor(EMeleeSwingStyle Style, EMeleeHeft Heft = EMeleeHeft::Steady);

	/**
	 * The tool's frame on Arc with the arm turned Degrees from the blow, the reach at ReachShare of the arc's, the wrist laid
	 * back Wrist degrees about the arc's axis, and the whole pose moved by Drop (camera space: under the frame).
	 */
	AI_LOOTER_SHOOTER_API FTransform ArcPose(const FMeleeArc& Arc, float Degrees, float ReachShare, float Wrist, const FVector& Drop = FVector::ZeroVector);

	/** The tool SwingTime seconds into a strike on Arc (the swing's own clock); bLanded: the blow met a body or a wall. */
	AI_LOOTER_SHOOTER_API FMeleeToolPose ToolPoseOnArc(const FMeleeArc& Arc, float SwingTime, bool bLanded);
	/** TipLength: the tool's striking end from the hand (cm, drawn; AimAtTip), 0 for none known. */
	AI_LOOTER_SHOOTER_API FMeleeToolPose ToolPose(EMeleeSwingStyle Style, EMeleeHeft Heft, float SwingTime, bool bLanded, float TipLength = 0.f);

	/**
	 * A long tool's arc aimed so its striking end (TipLength from the hand) crosses the middle of the view at the blow, a
	 * little under the crosshair, rather than its handle (tools shorter than Looter.Melee.AimTipFrom keep their arc's).
	 */
	AI_LOOTER_SHOOTER_API void AimAtTip(FMeleeArc& Arc, float TipLength);

	/** The gun dropped down out of the frame while the tool swings (its own frame, about the middle of the hands). */
	AI_LOOTER_SHOOTER_API FMeleeSwingPose AwayPose();

	/**
	 * How far the gun is toward AwayPose SwingTime seconds into a strike: down over Looter.Melee.GunLowerSeconds (0.08),
	 * held, back up over the swing's last Looter.Melee.GunRaiseSeconds (0.08); 0 outside a swing.
	 */
	AI_LOOTER_SHOOTER_API float GunAwayShare(float SwingTime);

	/**
	 * The tool's head lays down its smear SwingTime seconds into a strike: from late in the wind-up (as the drive starts)
	 * through the follow-through, the arc's fast part (round 5, 2026-10-27).
	 */
	AI_LOOTER_SHOOTER_API bool IsSmearing(float SwingTime);

	/** The gun is all the way down: hidden (with its attachments) until it comes back up (UPlayerMeleeComponent). */
	AI_LOOTER_SHOOTER_API bool IsGunAway(float SwingTime);

	/** The gun SwingTime seconds into a strike: AwayPose by GunAwayShare. */
	AI_LOOTER_SHOOTER_API FMeleeSwingPose GunPose(float SwingTime, bool bLanded);

	/** The view as the swing starts: drawn back and turned a little away from the blow to come. */
	AI_LOOTER_SHOOTER_API FViewKick WindUp(EMeleeSwingStyle Style, EMeleeHeft Heft = EMeleeHeft::Steady);

	/** The view at the blow, hit or miss: leaning into the swing's way (a chop dips, a jab punches in). */
	AI_LOOTER_SHOOTER_API FViewKick Strike(EMeleeSwingStyle Style, EMeleeHeft Heft = EMeleeHeft::Steady);

	/**
	 * The blow landing on a body: a sharp jolt back off it and a punch in, rolled a little Lean's way (-1 or 1). The
	 * killing blow is 30% more.
	 */
	AI_LOOTER_SHOOTER_API FViewKick Impact(bool bKill, float Lean);

	/** The tool on a wall: a smaller jolt. */
	AI_LOOTER_SHOOTER_API FViewKick WallKnock();

	/** A landed blow's shake (UCameraShakeModifier::AddShake): its strength and seconds; a killing blow's is stronger. */
	inline constexpr float ImpactShake = 0.2f;
	inline constexpr float KillShake = 0.3f;
	inline constexpr float ShakeSeconds = 0.15f;

	/** A knock on a wall is heard at this share of a blow's volume. */
	inline constexpr float WallVolume = 0.5f;
}
