#pragma once

#include "CoreMinimal.h"
#include "Combat/HitPose.h"
#include "Combat/HitReaction.h"
#include "Creatures/UnpaidHat.h"

class AUnpaidCreature;
struct FRandomStream;
struct FUnpaidRigBones;

/**
 * The Unpaid's body parts as its rig and arms track them (AUnpaidCreature uses them by these names: UnpaidCreatureRig.cpp,
 * UnpaidCreatureArms.cpp, UnpaidCreatureArmGuard.cpp). Split from UnpaidCreature.h (2026-10-28, the ~500-line rule). Its
 * hits as they show on the body (FHurtChannels, HITS 2026-10-28) are here too: UnpaidCreatureHurt.cpp.
 */
namespace UnpaidBody
{
	struct FArm
	{
		float Side = 1.f;
		int32 UpperArm = INDEX_NONE;
		int32 LowerArm = INDEX_NONE;
		int32 Hand = INDEX_NONE;
		TArray<int32> Fingers;
		/** The axis each finger curls about, in the rest pose's frame, and how far it fans from the middle one (in fingers). */
		TArray<FVector> CurlAxes;
		TArray<float> FanOffsets;
		/** The idle curl and fan each finger rests in (the model bakes them in), taken back off as it's posed. */
		TArray<FQuat> IdleTurns;
	};

	/** The pose's smoothed channels (degrees, or 0 to 1). */
	struct FPoseChannels
	{
		float Lean = 0.f;
		float Rear = 0.f;
		float Reach = 0.f;
		float Spread = 0.f;
		float Jaw = 0.f;
		float Curl = 0.f;
		float Splay = 0.f;
		float Snap = 0.f;
		float LookYaw = 0.f;
		float LookPitch = 0.f;
		float Death = 0.f;
		/** Hunting (0 to 1): its arms come up and claw at the air in turn while it chases. */
		float Hunt = 0.f;
		/** Dying, crumpling to the ground (0 to 1; not a boss); calm, its slow sway where it hangs (degrees of roll). */
		float Crumple = 0.f;
		float Sway = 0.f;
		/** Each arm's rake (0 left, 1 right): -1 drawn back and out, 0 as it is, 1 swept forward across its front. */
		float Rake[2] = { 0.f, 0.f };
	};

	/** One arm's lag behind the body's moves, on a spring: degrees swung back and out from the body, and their speeds. */
	struct FArmSway
	{
		float Swing = 0.f;
		float Out = 0.f;
		float SwingSpeed = 0.f;
		float OutSpeed = 0.f;
		/** How far the keep-out has turned it away from the body and swung it back (degrees), easing off once it needn't. */
		float KeepOut = 0.f;
		float KeepSwing = 0.f;
	};

	/** A point on an arm's hit hull (in its arm bone's space) that must keep out of the torso, and how deep it sits in each torso shape at rest. */
	struct FArmGuardPoint
	{
		/** Its arm bone (an index into Bones). */
		int32 Bone = INDEX_NONE;
		FVector InBone = FVector::ZeroVector;
		/** Per TorsoGuards entry: the rest pose's depth (a shoulder's sleeve sits in the chest's hull by design). */
		TArray<float> RestDepth;
		/** On the upper arm by the shoulder: the chest it hangs from may take it in (the armpit's fold), nothing else. */
		bool bNearShoulder = false;
	};

	/** A torso shape the arms keep out of: its bone (an index into Bones) and its shape in TorsoHulls. */
	struct FTorsoGuard
	{
		int32 Bone = INDEX_NONE;
		int32 Shape = INDEX_NONE;
	};

	/** How a killed Unpaid goes (HITS kill styles; a boss's, or with Looter.Hits.KillStyles 0, always Body: today's crumple). */
	enum class EKillStyle : uint8
	{
		/** Today's slump, crumple and ash. */
		Body,
		/** Killed by a shot on its coal: the coal snuffed out, a moment stiff, then a quick crumple. */
		Snuffed,
		/** Killed by a blast (a shotgun's, a rocket's): thrown back along the shot, embers torn out of it. */
		Torn,
		/** Overkilled: gone in a breath through its phase fade, its soul torn into tatters along the shot. */
		Scattered
	};

	/**
	 * Its hits as they show on the body (HITS, 2026-10-28; UnpaidCreatureHurt.cpp): what each part of the last hit asks of the
	 * flinch (the body's share, each arm's fling, each chain's jerk), the springs the rig lays over its pose (the spine's
	 * twist, the head's knock and whip, the jaw, the coal's pulse), its stagger's fold, its wound stage, how it died and its
	 * hat's flight. Angles in the rig's degrees (UnpaidCreatureRig.cpp: Pitch nods forward, Roll tips left, Yaw turns right).
	 * Plain numbers, so the tests drive it without a world.
	 */
	struct AI_LOOTER_SHOOTER_API FHurtChannels
	{
		// --- The last hit's shares (OnHit; ReactToHit and OnHurt read them; 1 each for a hit that isn't fresh) ---
		float BodyShare = 1.f;
		float ArmShare[2] = { 1.f, 1.f };
		/** The shroud, its left strip, its right strip. */
		float ChainJerk[3] = { 1.f, 1.f, 1.f };
		/** The pelvis's flinch, degrees per unit of flinch (8 before HITS, 11 at Looter.Hits.UnpaidScale 1). */
		float PelvisGain = 11.f;
		/** How much of HITS' new poses it shows (Looter.Hits.UnpaidScale, as of its last hit; 0 on a boss: its flinch is its own). */
		float NewShare = 1.f;
		/** This hit asks for its hat off, and for sparks spat out of its back from the coal. */
		bool bHatOff = false;
		bool bCoalSparks = false;
		/** The last hit's push in its own frame (as FlinchAway), and its stagger's, caught as the stagger began. */
		FVector Away = FVector::ZeroVector;
		FVector FoldAway = FVector::ZeroVector;

		// --- The springs (UnpaidCreatureRig.cpp lays them on) ---
		/** The spine's twist (Z: yaw) by how far off its axis the wound is. */
		FHitSpring Twist;
		/** The head knocked (X: pitch, a head hit's; Z: yaw, turned toward a shooter behind it), and its whip (X: the snap after its lag). */
		FHitSpring HeadKnock;
		FHitSpring Whip;
		/** The jaw dropped (X, degrees) and the coal swelled (X, its scale's share). */
		FHitSpring Jaw;
		FHitSpring Coal;
		/** The head's lag behind the body's jolt (1 held, easing to 0), seconds left of it, and the whip that comes after it (degrees). */
		float HeadHold = 0.f;
		float HeadLagLeft = 0.f;
		float WhipDegrees = 0.f;
		/** Seconds into each hand's splay (an arm hit; below 0: none), and the strips' snap left (s, a Big hit from the front). */
		float SplayTime[2] = { -1.f, -1.f };
		float SnapLeft = 0.f;

		// --- Its stagger, its wounds, its death, its hat ---
		/** How far its stagger pose shows (ACreatureBase::GetStaggerAlpha, kept from its last frame), and whether one is running. */
		float Stagger = 0.f;
		bool bStaggered = false;
		/** Its wound stage (WoundStage: 0, 1, 2), the coal's flicker as last added to its heat, its gutter's and its smoke's clocks. */
		int32 Wound = 0;
		float Flicker = 0.f;
		float GutterClock = 0.f;
		float PuffClock = 0.f;
		EKillStyle Kill = EKillStyle::Body;
		/** The killing shot's push in its own frame. */
		FVector KillAway = FVector::ZeroVector;
		FUnpaidHatFlight Hat;

		/**
		 * A fresh hit (its OnHurt): its shares, kicks and asks from where it landed (Hit.Zone), how hard (Strength: its
		 * flinch's) and which way it pushed (InAway, its own frame); ToShooter: toward whoever shot it (its own frame, unit).
		 * Arm and Chain: the arm (0 left, 1 right) and chain the bone is of, -1 none. bHatOn: a hat on its head to lose.
		 * bBoss: a boss's flinch is its own: nothing new but the pelvis's bigger jolt.
		 */
		void OnHit(const FCreatureHit& Hit, float Strength, int32 Arm, int32 Chain, const FVector& InAway, const FVector& ToShooter,
			bool bHatOn, bool bBoss);
		/** A flinch that isn't a fresh hit's (a boss's own jolt): the shares back to 1, nothing asked. */
		void ClearShares();
		/** How it dies by its killing hit (OnDied), pushed InAway (its own frame). */
		EKillStyle OnKilled(const FCreatureHit& Hit, const FVector& InAway, bool bBoss);
		/** The springs and pulses on by Dt (with the pose: AnimateBody). */
		void Step(float Dt);
		/** Its stagger pose as the hit reactions show it now (Alpha, 0 to 1): +1 as one begins, -1 as it ends, else 0. */
		int32 TickStagger(float Alpha);
		/**
		 * Its wounds at its health's share, alive (Time: its pose's clock): the change to its coal's heat for the flicker now
		 * (returned), and whether a puff of ember smoke (bOutPuff) or its gutter's sound (bOutGutter) is due.
		 */
		float TickWound(float HealthShare, float Dt, float Time, FRandomStream& Dice, bool& bOutPuff, bool& bOutGutter);
		/** Back as it was made (a respawn), the hat's flight with it (UnpaidHat::PutBack it first). */
		void Reset() { *this = FHurtChannels(); }

		// --- What the rig lays on (UnpaidCreatureRig.cpp); each 0 (or 1 for a scale) at rest ---
		float TwistYaw() const { return static_cast<float>(Twist.Offset.Z); }
		/** The chest's lag behind the pelvis's flinch pitch (PelvisFlinch, degrees). */
		float ChestPitch(float PelvisFlinch) const;
		float NeckPitch() const;
		/** The head's: its knock, its lag behind the pelvis's flinch and the whip after it. */
		float HeadPitch(float PelvisFlinch) const;
		float HeadYaw() const { return static_cast<float>(HeadKnock.Offset.Z); }
		float JawOpen() const { return static_cast<float>(Jaw.Offset.X); }
		/** A hand's fingers' extra fan (degrees per finger; Side 0 left, 1 right). */
		float SplayFor(int32 Side) const;
		/** The coal's extra size alive (a share of it), and its size dying, DeathTime in (times; 1 unless snuffed). */
		float CoalPulse() const { return static_cast<float>(Coal.Offset.X); }
		float CoalDeathScale(float DeathTime) const;
		/** Its stagger's fold at the waist (X pitch, Y roll; degrees) and its sink (cm). */
		FVector2D StaggerFold() const;
		float StaggerSink() const;
		/** How much of its bob it keeps (half at wound stage 2). */
		float BobShare() const;
		/** Its kill style's throw DeathTime in: the pelvis's tilt (X pitch, Y roll; degrees) and shift (cm, its own frame). */
		FVector2D KillTilt(float DeathTime) const;
		FVector KillShift(float DeathTime) const;
		/** How much quicker it slumps and crumples than it used to (2 snuffed), and whether it stands stiff now (snuffed, its first moment). */
		float CrumpleRate() const { return Kill == EKillStyle::Snuffed ? 2.f : 1.f; }
		bool IsStiff(float DeathTime) const;
		/** Its phase fade DeathTime in by its kill style (Scattered: gone within 0.3 s); below 0 for the usual fade. */
		float KillPhase(float DeathTime) const;

		/**
		 * Both arms' flings (degrees a second) for a fling of Fling at each arm's Share: each at most MaxSpeed, and the arm
		 * flung less kept to at most half the other's, so a hit arm always flies at least twice as far.
		 */
		static void ArmFlings(float Fling, const float Share[2], float MaxSpeed, float OutFling[2]);
	};
}

/**
 * The world side of the Unpaid's hits (UnpaidHurtEffects.cpp), on its public parts: the arm and chain a bone is of, its
 * hat knocked off, sparks and smoke from its coal, its kill style's effects and sounds.
 */
namespace UnpaidHurtPose
{
	/** Which arm Bone is of (0 left, 1 right: upper arm down to the fingers), else -1. */
	int32 ArmOf(const FUnpaidRigBones& Rig, FName Bone);
	/** Which chain Bone is a link of (0 the shroud, 1 the left strip, 2 the right), else -1. */
	int32 ChainOf(const FUnpaidRigBones& Rig, FName Bone);
	/** Its last hit (fresh) read into Hurt, and what it asks done: its hat knocked off, the coal's sparks. Away: its push, its own frame. */
	void OnHit(AUnpaidCreature& Unpaid, UnpaidBody::FHurtChannels& Hurt, float Strength, const FVector& Away);
	/** Its killing hit: its kill style, that style's effects and sounds, its hat off when its head (or an overkill) killed it. */
	void OnKilled(AUnpaidCreature& Unpaid, UnpaidBody::FHurtChannels& Hurt, const FVector& Away);
	/** One puff of ember smoke from its wounded coal (near a player, a few at a time world-wide). */
	void PuffWound(const AUnpaidCreature& Unpaid);
	/** A cue of CreatureHitCue's at its coal (near a player only when bNearOnly). */
	void PlayCue(const AUnpaidCreature& Unpaid, const TCHAR* Cue, float Volume = 1.f, bool bNearOnly = false);
}
