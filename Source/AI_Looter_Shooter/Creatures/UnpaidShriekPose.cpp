// AUnpaidCreature's failed-lunge shriek on its body (2026-10-29; the shriek itself is UnpaidCreatureShriek.cpp). Its channels
// (PoseTargets' goals, eased by the rig): the tell rears it back, tips its head up, drops its jaw wide, spreads its arms and
// curls its claws; the scream thrusts its head forward, jaw at its widest; the recovery hunches it, panting. On top, after
// the channels (UnpaidCreatureRig.cpp's AnimateBody, as its statuses' quick motions are): the head's violent shake through the
// scream, the body's shudder, the panting chest. Its shroud flares in the tell and shudders through the scream (a shake on the
// drift its chains swing to, and pushes on the side strips). Its hat lifts off its head through the scream and settles back.
// Angles in the rig's degrees (Pitch nods forward, Roll tips left, Yaw turns right; UnpaidCreatureRig.cpp).

#include "Creatures/UnpaidKitWork.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidBodyParts.h"
#include "Creatures/UnpaidMoveRules.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	float Ease(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

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

	/** A shake's two uneven wobbles (about -1.5 to 1.5): never a clean sine, so it reads as violent, not as a wobble. */
	float Shake(float Time, float Hz, float Seed)
	{
		return FMath::Sin(2.f * UE_PI * Hz * Time + Seed) + 0.5f * FMath::Sin(2.f * UE_PI * Hz * 1.55f * Time + 2.3f * Seed);
	}

	/** Where in the shriek it is: the phase, and how far into it (0-1). */
	EUnpaidShriekPhase PhaseOf(const FUnpaidMoveState& State, const FUnpaidMoveTuning& Tuning, float& OutInto)
	{
		const float Time = State.ShriekTime;
		OutInto = 0.f;
		switch (State.Shriek)
		{
		case EUnpaidShriekPhase::Tell:
			OutInto = Time / FMath::Max(Tuning.ShriekTell, 0.01f);
			break;
		case EUnpaidShriekPhase::Scream:
			OutInto = (Time - Tuning.ShriekTell) / FMath::Max(Tuning.ShriekScream, 0.01f);
			break;
		case EUnpaidShriekPhase::Recover:
			OutInto = (Time - Tuning.ShriekTell - Tuning.ShriekScream) / FMath::Max(Tuning.ShriekRecover, 0.01f);
			break;
		default:
			break;
		}
		OutInto = FMath::Clamp(OutInto, 0.f, 1.f);
		return State.Shriek;
	}

	/** The scream's head thrust: snapped forward over its first tenth, held, easing back over its last quarter. */
	float Thrust(float Into)
	{
		return Ease(Into / 0.1f) * (1.f - Ease((Into - 0.75f) / 0.25f));
	}

	/** How high its hat lifts (cm, in the hat's own size): off its head as the scream hits, trembling, settling with a bounce. */
	constexpr float HatLift = 9.f;
	constexpr float HatTilt = 8.f;
}

void FUnpaidShriekWork::Channels(const AUnpaidCreature& Unpaid, UnpaidBody::FPoseChannels& Goal)
{
	if (Unpaid.Kit.Action != EUnpaidAction::Shriek || Unpaid.IsDead())
	{
		return;
	}
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	float Into = 0.f;
	const float Jaw = Unpaid.JawOpenDegrees;
	switch (PhaseOf(Unpaid.Kit.Moves, Tuning, Into))
	{
	case EUnpaidShriekPhase::Tell:
	{
		// It rears back, its head tipping up, its jaw dropping wide, its arms spreading and its claws curling.
		const float Rise = Ease(Into);
		Goal.Rear = Rise;
		Goal.Spread = Rise;
		Goal.Lean = FMath::Lerp(Goal.Lean, -10.f, Rise);
		Goal.Jaw = FMath::Lerp(12.f, Jaw * 1.15f, Rise);
		Goal.Curl = FMath::Lerp(30.f, 58.f, Rise);
		Goal.Splay = FMath::Lerp(8.f, 3.f, Rise);
		Goal.LookPitch += 22.f * Rise;
		Goal.Hunt = 1.f - Rise;
		Goal.Reach = 0.f;
		Goal.Rake[0] = -0.25f * Rise;
		Goal.Rake[1] = -0.25f * Rise;
		break;
	}
	case EUnpaidShriekPhase::Scream:
	{
		// Its head thrust at them, jaw at its widest, arms flung back and wide, claws locked, its shroud snapped out.
		const float Push = Thrust(Into);
		const float Fade = 1.f - Ease((Into - 0.8f) / 0.2f);
		Goal.Rear = FMath::Lerp(1.f, 0.15f, Push);
		Goal.Lean = FMath::Lerp(-10.f, 16.f, Push);
		Goal.Reach = 0.35f * Push;
		Goal.Spread = 1.f;
		Goal.Jaw = FMath::Lerp(12.f, Jaw * 1.3f, Fade);
		Goal.Curl = 62.f;
		Goal.Splay = 2.f;
		Goal.Hunt = 0.f;
		Goal.Snap = 0.55f * Fade;
		Goal.LookPitch += 4.f;
		Goal.Rake[0] = -0.4f * Fade;
		Goal.Rake[1] = -0.4f * Fade;
		break;
	}
	case EUnpaidShriekPhase::Recover:
	{
		// Hunched over, panting through its open mouth, arms hanging, before the hunger takes it again.
		const float Back = Ease(Into);
		Goal.Rear = 0.f;
		Goal.Lean = FMath::Lerp(24.f, 10.f, Back);
		Goal.Spread = 0.1f;
		Goal.Reach = 0.f;
		Goal.Jaw = 14.f + 4.f * FMath::Sin(2.f * UE_PI * 3.2f * Unpaid.Kit.Moves.ShriekTime);
		Goal.Curl = 35.f;
		Goal.Hunt = 0.4f * Back;
		break;
	}
	default:
		break;
	}
}

void FUnpaidShriekWork::LayBones(AUnpaidCreature& Unpaid)
{
	if (Unpaid.Kit.Action != EUnpaidAction::Shriek || Unpaid.IsDead() || !Unpaid.bRigReady)
	{
		return;
	}
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	float Into = 0.f;
	const EUnpaidShriekPhase Phase = PhaseOf(Unpaid.Kit.Moves, Tuning, Into);
	const float Time = Unpaid.Kit.Moves.ShriekTime;
	const float Seed = Unpaid.PoseSeed;
	auto Turn = [&Unpaid](int32 Bone, const FQuat& Extra)
	{
		if (Bone != INDEX_NONE && Unpaid.Bones.IsValidIndex(Bone))
		{
			Unpaid.Bones[Bone].Own = Unpaid.Bones[Bone].Own * Extra;
		}
	};
	auto Shift = [&Unpaid](int32 Bone, const FVector& By)
	{
		if (Bone != INDEX_NONE && Unpaid.Bones.IsValidIndex(Bone))
		{
			Unpaid.Bones[Bone].Shift += By;
		}
	};
	switch (Phase)
	{
	case EUnpaidShriekPhase::Tell:
	{
		// Rearing up: the head tipped further back than the channels' rear, a tremble building in it, the body rising.
		const float Rise = Ease(Into);
		Turn(Unpaid.HeadBone, Pitch(-10.f * Rise) * Yaw(1.5f * Rise * Shake(Time, 11.f, Seed)));
		Shift(Unpaid.PelvisBone, FVector(-5.f * Rise, 0.f, 4.f * Rise));
		break;
	}
	case EUnpaidShriekPhase::Scream:
	{
		// The scream: the head thrust forward and shaking violently, the jaw juddering, the trunk shuddering under it.
		const float Push = Thrust(Into);
		const float Violence = 1.f - 0.5f * Ease((Into - 0.6f) / 0.4f);
		Turn(Unpaid.NeckBone, Pitch(8.f * Push));
		Turn(Unpaid.HeadBone, Pitch(12.f * Push + 2.f * Violence * Shake(Time, 9.f, Seed + 1.f))
			* Yaw(7.f * Violence * Shake(Time, 15.3f, Seed)) * Roll(5.f * Violence * Shake(Time, 12.1f, Seed + 1.3f)));
		Turn(Unpaid.JawBone, Pitch(3.f * Violence * Shake(Time, 19.f, Seed + 0.7f)));
		Turn(Unpaid.SpineBone, Roll(2.5f * Violence * Shake(Time, 10.7f, Seed + 2.f)));
		Turn(Unpaid.ChestBone, Pitch(2.f * Violence * Shake(Time, 13.9f, Seed + 3.f)));
		Shift(Unpaid.PelvisBone, FVector(5.f * Push, 0.f, 1.2f * Violence * Shake(Time, 17.f, Seed)));
		break;
	}
	case EUnpaidShriekPhase::Recover:
	{
		// Panting: the chest heaving, the body bobbing with it, the head hung low.
		const float Pant = FMath::Sin(2.f * UE_PI * 2.4f * Time);
		const float Fade = 1.f - Ease(Into);
		Turn(Unpaid.ChestBone, Pitch(3.5f * Pant * Fade));
		Turn(Unpaid.HeadBone, Pitch(6.f * Fade));
		Shift(Unpaid.PelvisBone, FVector(0.f, 0.f, 1.5f * Pant * Fade));
		break;
	}
	default:
		break;
	}
}

FVector FUnpaidShriekWork::ShroudShake(const AUnpaidCreature& Unpaid)
{
	if (Unpaid.Kit.Action != EUnpaidAction::Shriek || Unpaid.IsDead())
	{
		return FVector::ZeroVector;
	}
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	float Into = 0.f;
	const float Time = Unpaid.Kit.Moves.ShriekTime;
	const float Seed = Unpaid.PoseSeed;
	switch (PhaseOf(Unpaid.Kit.Moves, Tuning, Into))
	{
	case EUnpaidShriekPhase::Tell:
		// Flaring: the chains swing as to a rush forward, so the shroud lifts back and out behind it.
		return FVector(350.f * Ease(Into), 40.f * Shake(Time, 6.f, Seed), 0.f);
	case EUnpaidShriekPhase::Scream:
	{
		// Shuddering with the scream: out and juddering side to side.
		const float Violence = 1.f - 0.6f * Ease((Into - 0.6f) / 0.4f);
		return FVector((450.f + 150.f * Shake(Time, 8.3f, Seed)) * Violence, 260.f * Violence * Shake(Time, 11.1f, Seed + 1.f), 0.f);
	}
	case EUnpaidShriekPhase::Recover:
		return FVector(80.f * (1.f - Into), 0.f, 0.f);
	default:
		return FVector::ZeroVector;
	}
}

void FUnpaidShriekWork::TickBody(AUnpaidCreature& Unpaid, float DeltaSeconds)
{
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();
	float Into = 0.f;
	const EUnpaidShriekPhase Phase = PhaseOf(State, Tuning, Into);
	// The side strips flung outward as it rears and screams (pushes on their links: each side away from the other).
	const float Flare = Phase == EUnpaidShriekPhase::Tell ? 220.f * Ease(Into) : (Phase == EUnpaidShriekPhase::Scream ? 300.f : 0.f);
	if (Flare > 0.f)
	{
		for (int32 Strip = 1; Strip < static_cast<int32>(UE_ARRAY_COUNT(Unpaid.Chains)); ++Strip)
		{
			const float Out = Strip == 1 ? -1.f : 1.f;
			for (FShroudChain::FLink& Link : Unpaid.Chains[Strip].Links)
			{
				Link.SideSpeed += Out * Flare * DeltaSeconds;
				Link.BackSpeed += 0.5f * Flare * DeltaSeconds;
			}
		}
	}
	// Its hat lifted off its head by the scream, trembling, then settling back with a little bounce.
	UStaticMeshComponent* HatMesh = Unpaid.GetHat();
	if (!State.bHatLifted || !HatMesh || Unpaid.Hurt.Hat.IsOff())
	{
		return;
	}
	float Lift = 0.f;
	if (Phase == EUnpaidShriekPhase::Scream)
	{
		Lift = HatLift * Ease(Into / 0.1f) * (1.f - Ease((Into - 0.7f) / 0.3f))
			+ (Into < 0.8f ? 1.5f * Shake(State.ShriekTime, 16.f, Unpaid.PoseSeed) * Ease(Into / 0.1f) : 0.f);
	}
	else if (Phase == EUnpaidShriekPhase::Recover)
	{
		Lift = 2.5f * FMath::Max(FMath::Sin(2.f * UE_PI * Into), 0.f) * (1.f - Into);
	}
	// In the hat bone's own space (the hat is attached there): straight up in the world, and tipped back about its side.
	const FTransform Socket = Unpaid.GetMesh()->GetSocketTransform(Unpaid.Rig.Hat);
	const FVector LocalUp = Socket.InverseTransformVectorNoScale(FVector::UpVector);
	const FVector LocalSide = Socket.InverseTransformVectorNoScale(Unpaid.GetActorRightVector());
	const FQuat Tip(LocalSide.GetSafeNormal(), FMath::DegreesToRadians(-HatTilt * Lift / HatLift));
	HatMesh->SetRelativeLocationAndRotation(State.HatRestLocation + LocalUp * Lift, Tip * State.HatRestTurn);
}

void FUnpaidShriekWork::SettleHat(AUnpaidCreature& Unpaid)
{
	FUnpaidMoveState& State = Unpaid.Kit.Moves;
	if (!State.bHatLifted)
	{
		return;
	}
	State.bHatLifted = false;
	UStaticMeshComponent* HatMesh = Unpaid.GetHat();
	if (HatMesh && !Unpaid.Hurt.Hat.IsOff())
	{
		HatMesh->SetRelativeLocationAndRotation(State.HatRestLocation, State.HatRestTurn);
	}
}
