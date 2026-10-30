// AStoneSentinelCreature's pose, all code (UCreaturePoseAnimInstance): the model's rest pose is the statue (the mourner on
// its plinth, hood bowed), and everything it does is laid over it by how awake it is: its waking shudder and lifted head, its
// arms hanging heavy, the slam's heave and blow (StoneSentinelRules::SlamPoseAt), its lumbering stride, a hit's small jolt,
// the crumble's sag. Worked out in component space down the rig (each bone after its parent), so the arms aim where they're
// meant to whatever the trunk's bow. Without the model, the stand-in post leans with the same bow.

#include "Creatures/StoneSentinelCreature.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "ReferenceSkeleton.h"

namespace
{
	const FName PelvisBone(TEXT("pelvis"));
	const FName Spine1Bone(TEXT("spine_01"));
	const FName Spine2Bone(TEXT("spine_02"));
	const FName HeadBone(TEXT("head"));
	/** Left, then right. */
	const FName UpperBones[2] = { FName(TEXT("upperarm_l")), FName(TEXT("upperarm_r")) };
	const FName LowerBones[2] = { FName(TEXT("lowerarm_l")), FName(TEXT("lowerarm_r")) };
	const FName HandBones[2] = { FName(TEXT("hand_l")), FName(TEXT("hand_r")) };
	const FName ThighBones[2] = { FName(TEXT("thigh_l")), FName(TEXT("thigh_r")) };
	const FName CalfBones[2] = { FName(TEXT("calf_l")), FName(TEXT("calf_r")) };
	const FName FootBones[2] = { FName(TEXT("foot_l")), FName(TEXT("foot_r")) };
	const FName RootBone(TEXT("root"));

	/** The arms come out from the body this much (the Y of their way, -1 left): the upper arms a little, the fists in to meet. */
	constexpr float UpperSpread = 0.2f;
	constexpr float ForeSpread = -0.12f;
	/** Its trunk's bow shared down the spine. */
	constexpr float LowerSpineShare = 0.6f;
	constexpr float UpperSpineShare = 0.4f;
	/** Its stride (degrees each way at the thigh) and the knee's bend as the foot comes through. */
	constexpr float ThighSwing = 15.f;
	constexpr float KneeBend = 16.f;
	/** A heavy sway side to side as it steps (degrees at the trunk). */
	constexpr float StepSway = 2.5f;
	/** Awake, its bowed head lifts this much (degrees) and turns after its target up to HeadTurn either way. */
	constexpr float HeadLift = 12.f;
	constexpr float HeadTurn = 40.f;
	/** The waking's grinding shudder at its start, and the crumble's at its worst (degrees). */
	constexpr float WakeShudder = 2.5f;
	constexpr float CrumbleShudder = 6.f;
	/** How fast its pose eases to a new one outside the slam (1/s): stone, so not quick. */
	constexpr float PoseEase = 5.f;

	/** The bone's turn by Turn in component space, round its own head. */
	void TurnBone(FTransform& Bone, const FQuat& Turn)
	{
		Bone.SetRotation((Turn * Bone.GetRotation()).GetNormalized());
	}

	/** The way down a limb from its swing (degrees forward of hanging, round the body's side axis) and how far it comes out. */
	FVector LimbWay(float SwingDegrees, float Out)
	{
		const float Radians = FMath::DegreesToRadians(SwingDegrees);
		return FVector(FMath::Sin(Radians), Out, -FMath::Cos(Radians)).GetSafeNormal();
	}

	/**
	 * Aims a limb bone: its rest way (LocalWay, in its own axes) is swung round the body's side axis to WantedSwing, then
	 * turned the little rest of the way onto Wanted (if any), by Weight (0 leaves it as its parents put it). The swing goes
	 * first so a raised arm comes up in front, never round the side.
	 */
	void AimLimb(FTransform& Bone, const FVector& LocalWay, float WantedSwing, const FVector& Wanted, float Weight)
	{
		if (Weight <= 0.f)
		{
			return;
		}
		const FVector Now = Bone.GetRotation().RotateVector(LocalWay).GetSafeNormal();
		const float NowSwing = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Now.X), static_cast<float>(-Now.Z)));
		// A turn round +Y by a negative angle brings a hanging limb forward.
		const FQuat Swing(FVector::RightVector, -FMath::DegreesToRadians(FMath::UnwindDegrees(WantedSwing - NowSwing)));
		FQuat Full = Swing;
		if (!Wanted.IsNearlyZero())
		{
			Full = FQuat::FindBetweenNormals(Swing.RotateVector(Now), Wanted.GetSafeNormal()) * Swing;
		}
		TurnBone(Bone, FQuat::Slerp(FQuat::Identity, Full, FMath::Clamp(Weight, 0.f, 1.f)));
	}

	FStoneSentinelPose EasePose(const FStoneSentinelPose& Now, const FStoneSentinelPose& Wanted, float DeltaSeconds)
	{
		FStoneSentinelPose Out;
		Out.ArmSwing = FMath::FInterpTo(Now.ArmSwing, Wanted.ArmSwing, DeltaSeconds, PoseEase);
		Out.ElbowBend = FMath::FInterpTo(Now.ElbowBend, Wanted.ElbowBend, DeltaSeconds, PoseEase);
		Out.Bow = FMath::FInterpTo(Now.Bow, Wanted.Bow, DeltaSeconds, PoseEase);
		return Out;
	}
}

bool AStoneSentinelCreature::SetupRig()
{
	Rig = FRig();
	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	if (!Model)
	{
		return false;
	}
	const FReferenceSkeleton& Ref = Model->GetRefSkeleton();
	const int32 Num = Ref.GetNum();
	if (Num <= 0)
	{
		return false;
	}
	Rig.RefLocal = Ref.GetRefBonePose();
	Rig.Parents.SetNum(Num);
	Rig.Posed.SetNum(Num);
	// The rest pose in component space (parents come first in the skeleton).
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const int32 Parent = Ref.GetParentIndex(Index);
		Rig.Parents[Index] = Parent;
		Rig.Posed[Index] = Parent >= 0 ? Rig.RefLocal[Index] * Rig.Posed[Parent] : Rig.RefLocal[Index];
	}
	const TArray<FTransform>& Rest = Rig.Posed;
	auto Find = [&Ref, this](FName Bone)
	{
		const int32 Index = Ref.FindBoneIndex(Bone);
		if (Index != INDEX_NONE)
		{
			Rig.Emitted.Emplace(Bone, Index);
		}
		return Index;
	};
	// A limb bone's way to the next bone down, in its own axes; hanging straight down where the next is missing.
	auto WayTo = [&Rest](int32 From, int32 To)
	{
		const FVector Way = To != INDEX_NONE ? (Rest[To].GetLocation() - Rest[From].GetLocation()).GetSafeNormal() : FVector::DownVector;
		return Rest[From].GetRotation().UnrotateVector(Way.IsNearlyZero() ? FVector::DownVector : Way);
	};
	Find(RootBone);
	Rig.Pelvis = Find(PelvisBone);
	Rig.Spine1 = Find(Spine1Bone);
	Rig.Spine2 = Find(Spine2Bone);
	Rig.Head = Find(HeadBone);
	bool bAnyLimb = false;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		Rig.Upper[Side] = Find(UpperBones[Side]);
		Rig.Lower[Side] = Find(LowerBones[Side]);
		Rig.Hand[Side] = Find(HandBones[Side]);
		Rig.Thigh[Side] = Find(ThighBones[Side]);
		Rig.Calf[Side] = Find(CalfBones[Side]);
		Rig.Foot[Side] = Find(FootBones[Side]);
		Rig.UpperWay[Side] = Rig.Upper[Side] != INDEX_NONE ? WayTo(Rig.Upper[Side], Rig.Lower[Side]) : FVector::DownVector;
		Rig.LowerWay[Side] = Rig.Lower[Side] != INDEX_NONE ? WayTo(Rig.Lower[Side], Rig.Hand[Side]) : FVector::DownVector;
		Rig.ThighWay[Side] = Rig.Thigh[Side] != INDEX_NONE ? WayTo(Rig.Thigh[Side], Rig.Calf[Side]) : FVector::DownVector;
		Rig.CalfWay[Side] = Rig.Calf[Side] != INDEX_NONE ? WayTo(Rig.Calf[Side], Rig.Foot[Side]) : FVector::DownVector;
		bAnyLimb |= Rig.Upper[Side] != INDEX_NONE || Rig.Thigh[Side] != INDEX_NONE;
	}
	Rig.bReady = bAnyLimb || Rig.Spine1 != INDEX_NONE;
	return Rig.bReady;
}

FStoneSentinelPose AStoneSentinelCreature::WantedPose() const
{
	if (Phase == EStoneSentinelPhase::Crumbling)
	{
		// It sags: arms dropping, bowing over as the cracks run through it.
		const float T = FMath::Clamp(PhaseTime / FMath::Max(Tuning.CrumbleSeconds, 0.05f), 0.f, 1.f);
		return FStoneSentinelPose{ 0.f, 5.f, FMath::Lerp(20.f, 45.f, T) };
	}
	if (GetCreatureState() == ECreatureState::Attack)
	{
		return StoneSentinelRules::SlamPoseAt(GetStateTime(), Tuning);
	}
	return StoneSentinelRules::IdlePose();
}

void AStoneSentinelCreature::AnimateBody(float DeltaSeconds)
{
	// The slam's curve is the pose itself (its timing is the tell); anything else eases there, so a called-off heave or a
	// stagger's end never snaps.
	const FStoneSentinelPose Wanted = WantedPose();
	PoseNow = GetCreatureState() == ECreatureState::Attack && Phase == EStoneSentinelPhase::Awake ? Wanted
		: EasePose(PoseNow, Wanted, DeltaSeconds);

	// Its head after its target, while it's awake and has one.
	float WantedYaw = 0.f;
	if (const APawn* Prey = GetTarget(); Prey && Phase != EStoneSentinelPhase::Crumbling)
	{
		const FVector Local = GetActorQuat().UnrotateVector(Prey->GetActorLocation() - GetActorLocation());
		WantedYaw = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Local.Y), static_cast<float>(Local.X))), -HeadTurn, HeadTurn);
	}
	HeadYaw = FMath::FInterpTo(HeadYaw, WantedYaw, DeltaSeconds, 3.f);

	// The shudder: grinding loose as it wakes (strongest at first), cracking through as it crumbles.
	float Shudder = 0.f;
	if (Phase == EStoneSentinelPhase::Waking)
	{
		Shudder = WakeShudder * (1.f - AwakeWeight);
	}
	else if (Phase == EStoneSentinelPhase::Crumbling)
	{
		Shudder = WakeShudder + (CrumbleShudder - WakeShudder) * FMath::Clamp(PhaseTime / FMath::Max(Tuning.CrumbleSeconds, 0.05f), 0.f, 1.f);
	}
	const FVector2D Quake = Shudder > 0.f
		? FVector2D(FMath::Sin(Clock * 37.f), FMath::Sin(Clock * 29.f + 1.3f)) * Shudder : FVector2D::ZeroVector;
	const float Weight = FMath::Clamp(AwakeWeight, 0.f, 1.f);
	const float Cycle = GaitPhase * 2.f * PI;

	// Without the model: the stand-in post leans with its bow and jolts.
	if (bStandIn)
	{
		// A pitch below 0 tips its top forward (its bow); a shot from the front (Jolt.X above 0) rocks it back.
		const FRotator Lean(-PoseNow.Bow * 0.5f * Weight + Jolt.X + Quake.X, 0.f, Jolt.Y + Quake.Y);
		StandInBody->SetRelativeRotation(Lean);
		StandInHead->SetRelativeLocation(FVector(0.f, 0.f, -18.f) + Lean.RotateVector(FVector(0.f, 0.f, 116.f)));
		return;
	}
	if (!Rig.bReady)
	{
		return;
	}
	// A still statue is its rest pose: nothing to hand the anim instance.
	if (Weight <= 0.f && Shudder <= 0.f && Jolt.IsNearlyZero(0.05f))
	{
		BonePose.Reset();
		return;
	}

	const float Sway = StepSway * Walking * FMath::Sin(Cycle);
	const FQuat Bow1(FVector::RightVector, FMath::DegreesToRadians((PoseNow.Bow * Weight) * LowerSpineShare - Jolt.X));
	const FQuat Bow2(FVector::RightVector, FMath::DegreesToRadians((PoseNow.Bow * Weight) * UpperSpineShare + Quake.X));
	const FQuat Roll1(FVector::ForwardVector, FMath::DegreesToRadians(Jolt.Y + Sway * Weight));
	const FQuat Roll2(FVector::ForwardVector, FMath::DegreesToRadians(Quake.Y));
	// Lifting the bowed head: a turn round +Y by a negative angle tips the face up.
	const FQuat Head = FQuat(FVector::UpVector, FMath::DegreesToRadians(HeadYaw * Weight))
		* FQuat(FVector::RightVector, -FMath::DegreesToRadians(HeadLift * Weight));

	const int32 Num = Rig.RefLocal.Num();
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const int32 Parent = Rig.Parents[Index];
		FTransform& Bone = Rig.Posed[Index];
		Bone = Parent >= 0 ? Rig.RefLocal[Index] * Rig.Posed[Parent] : Rig.RefLocal[Index];
		if (Index == Rig.Spine1)
		{
			TurnBone(Bone, Roll1 * Bow1);
		}
		else if (Index == Rig.Spine2)
		{
			TurnBone(Bone, Roll2 * Bow2);
		}
		else if (Index == Rig.Head)
		{
			TurnBone(Bone, Head);
		}
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float Out = Side == 0 ? -1.f : 1.f;
			if (Index == Rig.Upper[Side])
			{
				AimLimb(Bone, Rig.UpperWay[Side], PoseNow.ArmSwing, LimbWay(PoseNow.ArmSwing, Out * UpperSpread), Weight);
			}
			else if (Index == Rig.Lower[Side])
			{
				const float Swing = PoseNow.ArmSwing + PoseNow.ElbowBend;
				AimLimb(Bone, Rig.LowerWay[Side], Swing, LimbWay(Swing, Out * ForeSpread), Weight);
			}
			else if (Index == Rig.Thigh[Side])
			{
				// The legs stride opposite each other; only the swing, so its stance keeps its own width.
				const float Leg = -ThighSwing * Walking * FMath::Sin(Cycle + (Side == 0 ? 0.f : PI));
				AimLimb(Bone, Rig.ThighWay[Side], Leg, FVector::ZeroVector, Weight);
			}
			else if (Index == Rig.Calf[Side])
			{
				// The knee bends (the shin back) as the foot comes through.
				const float Leg = -ThighSwing * Walking * FMath::Sin(Cycle + (Side == 0 ? 0.f : PI));
				const float Bend = KneeBend * Walking * FMath::Max(0.f, FMath::Cos(Cycle + (Side == 0 ? 0.f : PI)));
				AimLimb(Bone, Rig.CalfWay[Side], Leg - Bend, FVector::ZeroVector, Weight);
			}
		}
	}

	BonePose.Reset();
	for (const TPair<FName, int32>& Each : Rig.Emitted)
	{
		BonePose.Add(FCreatureBonePose{ Each.Key, Rig.Posed[Each.Value] });
	}
}
