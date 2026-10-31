// AUnpaidCreature's body language (2026-10-31; the user: "their animations need improved upon"): the rules are UnpaidGaitRules;
// this lays them on its rig over the pose the channels make (UnpaidCreatureRig.cpp's AnimateBody calls Step and LayBones after
// the trunk's own turns, UnpaidCreatureArms.cpp calls ArmPose before the keep-out sees the arms). What it adds:
//  - the lurch: drifting, each beat heaves its hips forward and up and throws the trunk onto that side, then it drags back and
//    sags, one side weaker (its limp, its own for its life); its head held level against it, its arms swung with the beat;
//  - at rest its breath (a slow, uneven draw it shouldn't have) and its head's tilt: lolled over and hung there a while, now
//    and then cracked over with a crackle of old bone; hunting, set at a predator's tilt;
//  - the coil of a lunge's, a feint's or a low swipe's wind-up (its hips drawn back and down, its head lowered onto its prey)
//    and the shiver as it's about to go, so the release reads; a rake's twist (the trunk wound away from the drawn arm, thrown
//    on through each swipe);
//  - a hit's loll (its head knocked over on a spring) and stumble (its beat held, its hips sagging);
//  - how a plain kill falls: a slump, a last reach at its killer, a twist away, a buckle.
// Every Unpaid on its rig but a boss (Abel's body is his fight's show); Looter.Unpaid.Gait 0 is the body as it was. Angles in
// the rig's degrees (Pitch nods forward, Roll tips left, Yaw turns right).

#include "Creatures/UnpaidLifeWork.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/UnpaidBodyParts.h"
#include "Creatures/UnpaidGaitRules.h"
#include "Creatures/UnpaidLifeTuning.h"

namespace
{
	FQuat Pitch(float Degrees)
	{
		return FQuat(FVector::RightVector, FMath::DegreesToRadians(Degrees));
	}

	FQuat Roll(float Degrees)
	{
		return FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Degrees));
	}

	FQuat Yaw(float Degrees)
	{
		return FQuat(FVector::UpVector, FMath::DegreesToRadians(Degrees));
	}

	/** Its lurch shows in this long as it sets off and goes as it stops (1/s). */
	constexpr float MoveRate = 4.f;
	/** The coil follows its wind-up this quickly (1/s). */
	constexpr float CoilRate = 14.f;
	/** At full coil: the hips drawn back and down (cm), the waist drawn in and the head lowered onto its prey (degrees). */
	constexpr float CoilBack = 9.f;
	constexpr float CoilSink = 5.f;
	constexpr float CoilWaist = 3.f;
	constexpr float CoilHead = 5.f;
	/** The shiver before the release (degrees of the chest's roll and yaw, and its rates). */
	constexpr float ShiverRoll = 1.6f;
	constexpr float ShiverYaw = 1.2f;
	/** A stumble sags its hips this far (cm) and folds its waist this far (degrees) at its start. */
	constexpr float StumbleSag = 4.f;
	constexpr float StumbleFold = 4.f;
	constexpr float StumbleLongest = 0.4f;
	/** A hit's loll: its spring (stiffness, fade) and how long its kick holds (s). */
	constexpr float LollOmega = 16.f;
	constexpr float LollFade = 5.f;
	constexpr float LollHold = 0.04f;
	/** The neck takes this share of the tilt and the loll, the head the rest. */
	constexpr float NeckTiltShare = 0.35f;
	constexpr float NeckLollShare = 0.3f;
	/** A fall's reaching arm: up and out at its killer, open-handed. */
	constexpr float ReachSwing = -115.f;
	constexpr float ReachOut = 18.f;
	constexpr float ReachBend = 12.f;
	constexpr float ReachWrist = -12.f;
	constexpr float ReachCurl = 8.f;
	/** Its neck's crack is heard this loud (near a player only). */
	constexpr float CrackVolume = 0.7f;

	/** The moves whose wind-up coils (the lunge's shriek pose and its feint, the low swipe's crouch); a rake twists instead. */
	bool CoilsFor(EUnpaidMove Move)
	{
		return Move == EUnpaidMove::Lunge || Move == EUnpaidMove::Feint || Move == EUnpaidMove::LowSwipe || Move == EUnpaidMove::None;
	}
}

bool FUnpaidGaitWork::IsOn(const AUnpaidCreature& Unpaid)
{
	return Unpaid.bRigReady && !Unpaid.IsBossCreature() && FUnpaidLifeTuning::Live().Gait != 0;
}

void FUnpaidGaitWork::Seed(AUnpaidCreature& Unpaid)
{
	// Each its own: a limp of its own and its own strong side, its beat and its head's first tilt at their own moments.
	FUnpaidGaitState& Gait = Unpaid.Kit.Gait;
	Gait.Limp = FMath::FRandRange(0.3f, 0.9f);
	Gait.Side = FMath::RandBool() ? 1.f : -1.f;
	Gait.Phase = FMath::FRand();
	Gait.Tilt.Next = FMath::FRandRange(0.5f, 3.f);
}

void FUnpaidGaitWork::Reset(AUnpaidCreature& Unpaid)
{
	Unpaid.Kit.Gait.Reset();
}

void FUnpaidGaitWork::Step(AUnpaidCreature& Unpaid, float DeltaSeconds, float Speed)
{
	FUnpaidGaitState& Gait = Unpaid.Kit.Gait;
	const FUnpaidLifeTuning Tuning = FUnpaidLifeTuning::Live();
	Gait.bOn = Unpaid.bRigReady && !Unpaid.IsBossCreature() && Tuning.Gait != 0;
	if (!Gait.bOn)
	{
		return;
	}
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	Gait.LurchScale = Tuning.LurchScale;
	Gait.TiltScale = Tuning.TiltScale;
	const ECreatureState State = Unpaid.GetCreatureState();
	const bool bAlive = !Unpaid.IsDead();
	const bool bBusy = State == ECreatureState::Attack || Unpaid.IsPhasing() || Unpaid.IsRising() || Unpaid.Kit.bLunging
		|| Unpaid.Kit.Action != EUnpaidAction::None;

	// It lurches when it drifts under its own steam; its moves (an attack, a flight, a blink, a scream) are their own.
	const float WantMove = bAlive && !bBusy ? FMath::Clamp(Speed, 0.f, 1.f) : 0.f;
	Gait.Move = Dt > 0.f ? FMath::FInterpTo(Gait.Move, WantMove, Dt, MoveRate) : WantMove;
	// Its beat runs with its drift; a stumble holds it a moment.
	if (Gait.Stumble > 0.f)
	{
		Gait.Stumble = FMath::Max(Gait.Stumble - Dt, 0.f);
	}
	else
	{
		Gait.Phase = FMath::Fmod(Gait.Phase + UnpaidGait::LurchHz(Speed) * Dt, 1000.f);
	}

	// Its head's tilt by its mood: hanging about, on its prey, or (busy, dying) level.
	const UnpaidGait::ETiltMood Mood = !bAlive || bBusy ? UnpaidGait::ETiltMood::Level
		: (Unpaid.GetTarget() ? UnpaidGait::ETiltMood::Hunting : UnpaidGait::ETiltMood::Calm);
	UCreatureReactComponent* Reacts = Unpaid.GetReact();
	const float A = Reacts ? Reacts->GetRandom().FRand() : FMath::FRand();
	const float B = Reacts ? Reacts->GetRandom().FRand() : FMath::FRand();
	const float C = Reacts ? Reacts->GetRandom().FRand() : FMath::FRand();
	if (UnpaidGait::StepTilt(Gait.Tilt, Dt, Mood, A, B, C) && Gait.TiltScale > 0.f)
	{
		// Its neck cracking over: a short dry crackle of old bone.
		UnpaidHurtPose::PlayCue(Unpaid, UnpaidLifeCue::NeckCrack, CrackVolume, /*bNearOnly*/ true);
	}
	Gait.HitLoll.Step(Dt, LollOmega, LollFade);

	// The wind-up's coil, eased; nothing outside a wind-up.
	float CoilGoal = 0.f;
	if (bAlive && State == ECreatureState::Attack && CoilsFor(Unpaid.Kit.Move) && Unpaid.GetStateTime() < Unpaid.AttackWindup)
	{
		CoilGoal = UnpaidGait::Coil(Unpaid.GetStateTime() / FMath::Max(Unpaid.AttackWindup, 0.01f));
	}
	Gait.Coil = Dt > 0.f ? FMath::FInterpTo(Gait.Coil, CoilGoal, Dt, CoilRate) : CoilGoal;
}

void FUnpaidGaitWork::LayBones(AUnpaidCreature& Unpaid)
{
	const FUnpaidGaitState& Gait = Unpaid.Kit.Gait;
	if (!Gait.bOn || !Unpaid.bRigReady)
	{
		return;
	}
	auto Turn = [&Unpaid](int32 Bone, const FQuat& Extra)
	{
		if (Unpaid.Bones.IsValidIndex(Bone))
		{
			Unpaid.Bones[Bone].Own = Unpaid.Bones[Bone].Own * Extra;
		}
	};
	auto Shift = [&Unpaid](int32 Bone, const FVector& By)
	{
		if (Unpaid.Bones.IsValidIndex(Bone))
		{
			Unpaid.Bones[Bone].Shift += By;
		}
	};
	const float Time = Unpaid.PoseTime + Unpaid.PoseSeed;
	const float Alive = 1.f - Unpaid.Pose.Death;
	const ECreatureState State = Unpaid.GetCreatureState();

	// The lurch: its hips heaved and dragged, the trunk thrown onto the heaving side, the chest driven round, the head held level.
	if (Gait.Move > 0.01f && Gait.LurchScale > 0.f)
	{
		const UnpaidGait::FLurch Beat = UnpaidGait::Lurch(Gait.Phase, Gait.Move, Gait.Limp, Gait.Side);
		const float Show = Gait.LurchScale * Alive;
		Turn(Unpaid.PelvisBone, Pitch(Beat.Pitch * Show) * Roll(Beat.Roll * Show));
		Shift(Unpaid.PelvisBone, FVector(Beat.Surge * Show, 0.f, Beat.Heave * Show));
		Turn(Unpaid.ChestBone, Yaw(Beat.Twist * Show));
		Turn(Unpaid.HeadBone, Pitch(Beat.HeadPitch * Show) * Roll(Beat.HeadRoll * Show));
	}
	// A hit's stumble: its hips sag and its waist folds a moment while its beat is held.
	if (Gait.Stumble > 0.f)
	{
		const float Sag = FMath::Clamp(Gait.Stumble / StumbleLongest, 0.f, 1.f) * Alive;
		Shift(Unpaid.PelvisBone, FVector(0.f, 0.f, -StumbleSag * Sag));
		Turn(Unpaid.SpineBone, Pitch(StumbleFold * Sag));
	}
	// Its breath at rest: hanging still, not attacking, not climbing out of the ground.
	const float Calm = Alive * (1.f - Gait.Move) * (State == ECreatureState::Attack || Unpaid.IsRising() ? 0.f : 1.f);
	if (Calm > 0.01f)
	{
		const FVector2f Draw = UnpaidGait::Breath(Time, Calm);
		Turn(Unpaid.ChestBone, Pitch(Draw.X));
		Shift(Unpaid.PelvisBone, FVector(0.f, 0.f, Draw.Y));
	}
	// Its head's tilt and a hit's loll (the neck takes a share), and a crack's jerk round.
	const float Tilt = Gait.Tilt.Roll * Gait.TiltScale * Alive;
	const float Loll = static_cast<float>(Gait.HitLoll.Offset.X) * Alive;
	Turn(Unpaid.NeckBone, Roll(NeckTiltShare * Tilt + NeckLollShare * Loll));
	Turn(Unpaid.HeadBone, Roll((1.f - NeckTiltShare) * Tilt + (1.f - NeckLollShare) * Loll) * Yaw(Gait.Tilt.CrackYaw * Gait.TiltScale * Alive));
	// The coil before its release, and the shiver as it's about to go.
	if (Gait.Coil > 0.01f)
	{
		Shift(Unpaid.PelvisBone, FVector(-CoilBack * Gait.Coil, 0.f, -CoilSink * Gait.Coil));
		Turn(Unpaid.SpineBone, Pitch(-CoilWaist * Gait.Coil));
		Turn(Unpaid.HeadBone, Pitch(CoilHead * Gait.Coil));
		if (State == ECreatureState::Attack && Unpaid.AttackWindup > 0.f)
		{
			const float Shiver = UnpaidGait::ReleaseShiver(Unpaid.GetStateTime() / Unpaid.AttackWindup);
			Turn(Unpaid.ChestBone, Roll(ShiverRoll * Shiver * FMath::Sin(Time * 41.f)) * Yaw(ShiverYaw * Shiver * FMath::Sin(Time * 37.f + 1.f)));
		}
	}
	// A rake (or a cinder's throw): the trunk wound away from the drawn arm and thrown on through the swipe, from the arms'
	// own channels as they ease, so it follows the claw exactly.
	if (Alive > 0.f)
	{
		const float Twist = UnpaidGait::SwipeTwist(Unpaid.Pose.Rake) * Alive;
		if (!FMath::IsNearlyZero(Twist, 0.05f))
		{
			Turn(Unpaid.SpineBone, Yaw(0.4f * Twist));
			Turn(Unpaid.ChestBone, Yaw(0.6f * Twist));
		}
	}
	// How a plain kill falls.
	if (Unpaid.IsDead() && Unpaid.Hurt.Kill == UnpaidBody::EKillStyle::Body)
	{
		const UnpaidGait::FFallPose Fall = UnpaidGait::FallPose(Gait.Fall, Unpaid.DeathTime, Gait.FallSide);
		Turn(Unpaid.PelvisBone, Yaw(Fall.Yaw) * Roll(Fall.Roll) * Pitch(Fall.Pitch));
		Shift(Unpaid.PelvisBone, FVector(0.f, 0.f, Fall.Sink));
		Turn(Unpaid.HeadBone, Roll(Fall.HeadRoll) * Pitch(Fall.HeadPitch));
	}
}

void FUnpaidGaitWork::ArmPose(const AUnpaidCreature& Unpaid, int32 Side, float& Swing, float& Out, float& Bend, float& Wrist, float& Curl)
{
	const FUnpaidGaitState& Gait = Unpaid.Kit.Gait;
	if (!Gait.bOn || Side < 0 || Side > 1)
	{
		return;
	}
	if (!Unpaid.IsDead())
	{
		// The lurch's swing: the heaving side's arm thrown forward, the other trailing; less while its arms claw or reach.
		if (Gait.Move > 0.01f && Gait.LurchScale > 0.f)
		{
			const UnpaidGait::FLurch Beat = UnpaidGait::Lurch(Gait.Phase, Gait.Move, Gait.Limp, Gait.Side);
			const float Free = (1.f - 0.5f * Unpaid.Pose.Hunt) * (1.f - Unpaid.Pose.Reach);
			Swing += Beat.ArmSwing[Side] * Gait.LurchScale * Free;
		}
		return;
	}
	if (Unpaid.Hurt.Kill != UnpaidBody::EKillStyle::Body)
	{
		return;
	}
	// A last reach at its killer as it sinks: up and out, open-handed, then dropping as it breaks up.
	const UnpaidGait::FFallPose Fall = UnpaidGait::FallPose(Gait.Fall, Unpaid.DeathTime, Gait.FallSide);
	if (Fall.Arm == Side && Fall.Reach > 0.f)
	{
		const float Reach = FMath::Clamp(Fall.Reach, 0.f, 1.f);
		Swing = FMath::Lerp(Swing, ReachSwing, Reach);
		Out = FMath::Lerp(Out, ReachOut, Reach);
		Bend = FMath::Lerp(Bend, ReachBend, Reach);
		Wrist = FMath::Lerp(Wrist, ReachWrist, Reach);
		Curl = FMath::Lerp(Curl, ReachCurl, Reach);
	}
}

void FUnpaidGaitWork::OnHurt(AUnpaidCreature& Unpaid, float Strength, const FVector& Away)
{
	// Its head knocked over the way it was pushed, on a spring (it overshoots and comes back to stare), and its lurch stumbles.
	FUnpaidGaitState& Gait = Unpaid.Kit.Gait;
	Gait.HitLoll.Add(FVector(UnpaidGait::HitHeadRoll(Away, Strength), 0.f, 0.f), LollHold);
	Gait.Stumble = FMath::Max(Gait.Stumble, UnpaidGait::HitStumble(Strength));
}

void FUnpaidGaitWork::OnDied(AUnpaidCreature& Unpaid)
{
	// A plain kill falls by where the killing shot came from (Hurt.KillAway: its push; pushed back, it was shot from the front);
	// toward its killer's side (pushed to its left, the killer stood on its right).
	FUnpaidGaitState& Gait = Unpaid.Kit.Gait;
	const FVector Away = Unpaid.Hurt.KillAway;
	Gait.Fall = Unpaid.Hurt.Kill == UnpaidBody::EKillStyle::Body && !Unpaid.IsBossCreature()
		? UnpaidGait::ChooseFall(static_cast<float>(-Away.X), FMath::FRand()) : UnpaidGait::EFall::Slump;
	Gait.FallSide = Away.Y <= 0.0 ? 1.f : -1.f;
	Gait.Stumble = 0.f;
	Gait.Coil = 0.f;
}
