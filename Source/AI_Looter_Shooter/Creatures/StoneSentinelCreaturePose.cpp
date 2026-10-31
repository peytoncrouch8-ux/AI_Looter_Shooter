// AStoneSentinelCreature's pose, all code (UCreaturePoseAnimInstance): the model's rest pose is the statue (the mourner on
// its plinth, hood bowed), and everything it does is laid over it by how awake it is: its waking (a grinding shudder, a
// stretch free of the years, its drop into its stance), its arms hanging heavy and its weight shifting as stone settles,
// its moves' curves (StoneSentinelPoses.cpp: the slam, the throw with its trunk wound round, the shockwave's rear and pound,
// the reel), its lumbering stride (StoneSentinelRules::GaitAt: feet planted most of the cycle, hips dropping at each strike,
// the trunk rolling over the planted foot and twisting against the legs, the arms swinging a little), a hit's small jolt,
// the strain of a heave, the crumble's sag. Worked out in component space down the rig (each bone after its parent), so the
// arms aim where they're meant to whatever the trunk's bow and turn. Without the model, the stand-in post leans the same.

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
	/** The forearms take this share of a pose's spread. */
	constexpr float ForeSpreadShare = 0.6f;
	/** Its trunk's bow, lean and turn shared down the spine. */
	constexpr float LowerSpineShare = 0.6f;
	constexpr float UpperSpineShare = 0.4f;
	/** Its head turns back against this share of its trunk's turn, so its eyes stay on its target as it winds round. */
	constexpr float HeadCounterTwist = 0.6f;
	/** Awake, its bowed head lifts this much (degrees) and turns after its target up to HeadTurn either way. */
	constexpr float HeadLift = 12.f;
	constexpr float HeadTurn = 40.f;
	/** Its hips sinking bends its knees this much a cm (degrees: the thigh forward, the shin back twice that). */
	constexpr float CrouchKnee = 1.5f;
	/** Stone settling as it stands: a slow shift of its weight side to side and a breath of a bow (degrees, rad/s). */
	constexpr float SettleRoll = 0.8f;
	constexpr float SettleRollRate = 0.9f;
	constexpr float SettleBow = 0.6f;
	constexpr float SettleBowRate = 0.6f;
	/** The shudders (degrees): grinding loose as it wakes, straining as it tears the rock up or rears at its top, crumbling. */
	constexpr float WakeShudder = 2.5f;
	constexpr float StrainShudder = 1.4f;
	constexpr float CrumbleShudder = 6.f;
	/** The tear's strain lasts this long after TearAt (s); the shockwave's quiver is the rear's last part (a share of it). */
	constexpr float StrainSeconds = 0.3f;
	constexpr float QuiverFrom = 0.75f;
	/** How fast its pose eases to a new one outside a move's curve (1/s): stone, so not quick; a reel's quicker. */
	constexpr float PoseEase = 5.f;
	constexpr float ReelEase = 10.f;

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
	 * first so a raised arm comes up in front, never round the side. Frame is the trunk's turn: swing and way are taken in
	 * it, so the arms come round with the shoulders as it winds.
	 */
	void AimLimb(FTransform& Bone, const FVector& LocalWay, float WantedSwing, const FVector& Wanted, float Weight, const FQuat& Frame)
	{
		if (Weight <= 0.f)
		{
			return;
		}
		const FVector Now = Frame.UnrotateVector(Bone.GetRotation().RotateVector(LocalWay).GetSafeNormal());
		const float NowSwing = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Now.X), static_cast<float>(-Now.Z)));
		// A turn round +Y by a negative angle brings a hanging limb forward.
		const FQuat Swing(FVector::RightVector, -FMath::DegreesToRadians(FMath::UnwindDegrees(WantedSwing - NowSwing)));
		FQuat Full = Swing;
		if (!Wanted.IsNearlyZero())
		{
			Full = FQuat::FindBetweenNormals(Swing.RotateVector(Now), Wanted.GetSafeNormal()) * Swing;
		}
		const FQuat InComponent = Frame * Full * Frame.Inverse();
		TurnBone(Bone, FQuat::Slerp(FQuat::Identity, InComponent, FMath::Clamp(Weight, 0.f, 1.f)));
	}

	FStoneSentinelPose EasePose(const FStoneSentinelPose& Now, const FStoneSentinelPose& Wanted, float DeltaSeconds, float Rate)
	{
		return StoneSentinelRules::BlendPose(Now, Wanted, 1.f - FMath::Exp(-Rate * FMath::Max(DeltaSeconds, 0.f)));
	}

	FQuat Degrees(const FVector& Axis, float Angle)
	{
		return FQuat(Axis, FMath::DegreesToRadians(Angle));
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
	Rig.Root = Find(RootBone);
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
		// It sags: arms dropping, bowing over and sinking as the cracks run through it.
		const float T = FMath::Clamp(PhaseTime / FMath::Max(Tuning.CrumbleSeconds, 0.05f), 0.f, 1.f);
		FStoneSentinelPose Sag;
		Sag.ArmSwing = 0.f;
		Sag.ElbowBend = 5.f;
		Sag.Bow = FMath::Lerp(20.f, 40.f, T);
		Sag.Lean = FMath::Lerp(0.f, 6.f, T);
		Sag.Crouch = FMath::Lerp(0.f, 6.f, T);
		Sag.HeadPitch = -14.f;
		return Sag;
	}
	if (Phase == EStoneSentinelPhase::Waking)
	{
		return StoneSentinelRules::WakePoseAt(PhaseTime, Tuning.WakeSeconds);
	}
	if (GetCreatureState() == ECreatureState::Attack)
	{
		const float Time = GetStateTime();
		switch (Move)
		{
		case EStoneSentinelMove::Shockwave:
			return StoneSentinelMoves::ShockPoseAt(Time, ShockTuning);
		case EStoneSentinelMove::Boulder:
			return bBoulderCountered && Time >= AttackWindup
				? StoneSentinelMoves::ReelPoseAt(Time - AttackWindup, AttackRecovery)
				: StoneSentinelMoves::ThrowPoseAt(Time, BoulderTuning);
		default:
			return StoneSentinelRules::SlamPoseAt(Time, Tuning);
		}
	}
	return StoneSentinelRules::IdlePose();
}

void AStoneSentinelCreature::AnimateBody(float DeltaSeconds)
{
	// A move's curve is the pose itself (its timing is the tell), and so is its waking; anything else eases there, so a
	// called-off heave, a reel or a stagger's end never snaps.
	const FStoneSentinelPose Wanted = WantedPose();
	const bool bAttacking = GetCreatureState() == ECreatureState::Attack && Phase == EStoneSentinelPhase::Awake;
	const bool bReeling = bAttacking && Move == EStoneSentinelMove::Boulder && bBoulderCountered;
	if ((bAttacking && !bReeling) || Phase == EStoneSentinelPhase::Waking)
	{
		PoseNow = Wanted;
	}
	else
	{
		PoseNow = EasePose(PoseNow, Wanted, DeltaSeconds, bReeling ? ReelEase : PoseEase);
	}

	// Its head after its target, while it's awake and has one.
	float WantedYaw = 0.f;
	if (const APawn* Prey = GetTarget(); Prey && Phase != EStoneSentinelPhase::Crumbling)
	{
		const FVector Local = GetActorQuat().UnrotateVector(Prey->GetActorLocation() - GetActorLocation());
		WantedYaw = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Local.Y), static_cast<float>(Local.X))), -HeadTurn, HeadTurn);
	}
	HeadYaw = FMath::FInterpTo(HeadYaw, WantedYaw, DeltaSeconds, 3.f);

	// The shudder: grinding loose as it wakes (strongest at first), straining under the rock or at the top of its rear,
	// cracking through as it crumbles.
	float Shudder = 0.f;
	const float MoveTime = GetStateTime();
	if (Phase == EStoneSentinelPhase::Waking)
	{
		Shudder = WakeShudder * (1.f - AwakeWeight);
	}
	else if (Phase == EStoneSentinelPhase::Crumbling)
	{
		Shudder = WakeShudder + (CrumbleShudder - WakeShudder) * FMath::Clamp(PhaseTime / FMath::Max(Tuning.CrumbleSeconds, 0.05f), 0.f, 1.f);
	}
	else if (bAttacking && Move == EStoneSentinelMove::Boulder && !bBoulderCountered
		&& MoveTime >= BoulderTuning.TearAt && MoveTime < BoulderTuning.TearAt + StrainSeconds)
	{
		Shudder = StrainShudder;
	}
	else if (bAttacking && Move == EStoneSentinelMove::Shockwave)
	{
		const float Rise = ShockTuning.Windup - ShockTuning.FallSeconds;
		Shudder = MoveTime >= Rise * QuiverFrom && MoveTime < Rise ? StrainShudder * 0.8f : 0.f;
	}
	const FVector2D Quake = Shudder > 0.f
		? FVector2D(FMath::Sin(Clock * 37.f), FMath::Sin(Clock * 29.f + 1.3f)) * Shudder : FVector2D::ZeroVector;
	const float Weight = FMath::Clamp(AwakeWeight, 0.f, 1.f);

	// Without the model: the stand-in post leans with its bow, turn and jolts.
	if (bStandIn)
	{
		// A pitch below 0 tips its top forward (its bow); a shot from the front (Jolt.X above 0) rocks it back.
		const FRotator Lean(-PoseNow.Bow * 0.5f * Weight + Jolt.X + Quake.X, PoseNow.Twist * 0.5f * Weight, Jolt.Y + Quake.Y + PoseNow.Lean * Weight);
		StandInBody->SetRelativeRotation(Lean);
		StandInHead->SetRelativeLocation(FVector(0.f, 0.f, -18.f - PoseNow.Crouch * Weight) + Lean.RotateVector(FVector(0.f, 0.f, 116.f)));
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

	// The stride (eased in and out with Walking), and stone settling as it stands (fading as it walks or attacks).
	const FStoneSentinelGait Gait = StoneSentinelRules::GaitAt(GaitPhase);
	const float Stride = Walking * Weight;
	const float Settling = Weight * (1.f - Walking) * (bAttacking ? 0.f : 1.f);
	const float SettleRollNow = SettleRoll * Settling * FMath::Sin(Clock * SettleRollRate);
	const float SettleBowNow = SettleBow * Settling * FMath::Sin(Clock * SettleBowRate + 1.f);

	const float Bow = PoseNow.Bow * Weight + Gait.Lurch * Stride + SettleBowNow;
	const float Roll = PoseNow.Lean * Weight + Gait.Roll * Stride + SettleRollNow;
	const float Twist = PoseNow.Twist * Weight + Gait.Twist * Stride;
	const float Sink = PoseNow.Crouch * Weight + Gait.Drop * Stride;
	const FQuat Spine1Turn = Degrees(FVector::UpVector, Twist * 0.5f) * Degrees(FVector::ForwardVector, Jolt.Y + Roll * LowerSpineShare)
		* Degrees(FVector::RightVector, Bow * LowerSpineShare - Jolt.X);
	const FQuat Spine2Turn = Degrees(FVector::UpVector, Twist * 0.5f) * Degrees(FVector::ForwardVector, Quake.Y + Roll * UpperSpineShare)
		* Degrees(FVector::RightVector, Bow * UpperSpineShare + Quake.X);
	// The arms come round with the shoulders.
	const FQuat ArmFrame = Degrees(FVector::UpVector, Twist);
	// Lifting the bowed head (a turn round +Y by a negative angle tips the face up), its eyes kept on its target as it winds.
	const FQuat HeadTurnNow = Degrees(FVector::UpVector, (HeadYaw - Twist * HeadCounterTwist) * Weight)
		* Degrees(FVector::RightVector, -(HeadLift + PoseNow.HeadPitch) * Weight);

	const int32 Num = Rig.RefLocal.Num();
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const int32 Parent = Rig.Parents[Index];
		FTransform& Bone = Rig.Posed[Index];
		Bone = Parent >= 0 ? Rig.RefLocal[Index] * Rig.Posed[Parent] : Rig.RefLocal[Index];
		if (Index == Rig.Pelvis)
		{
			// Its weight dropping (a blow, a strike of its foot): everything above and below comes with it.
			Bone.AddToTranslation(FVector(0.0, 0.0, -Sink));
		}
		else if (Index == Rig.Spine1)
		{
			TurnBone(Bone, Spine1Turn);
		}
		else if (Index == Rig.Spine2)
		{
			TurnBone(Bone, Spine2Turn);
		}
		else if (Index == Rig.Head)
		{
			TurnBone(Bone, HeadTurnNow);
		}
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const float Out = Side == 0 ? -1.f : 1.f;
			const float ArmSwing = PoseNow.ArmSwing + Gait.Arm[Side] * Walking;
			const float Knees = PoseNow.Crouch * CrouchKnee;
			if (Index == Rig.Upper[Side])
			{
				AimLimb(Bone, Rig.UpperWay[Side], ArmSwing, LimbWay(ArmSwing, Out * (UpperSpread + PoseNow.Spread)), Weight, ArmFrame);
			}
			else if (Index == Rig.Lower[Side])
			{
				const float Swing = ArmSwing + PoseNow.ElbowBend;
				AimLimb(Bone, Rig.LowerWay[Side], Swing, LimbWay(Swing, Out * (ForeSpread + PoseNow.Spread * ForeSpreadShare)), Weight, ArmFrame);
			}
			else if (Index == Rig.Thigh[Side])
			{
				// The legs stride opposite each other; only the swing, so its stance keeps its own width.
				AimLimb(Bone, Rig.ThighWay[Side], Gait.Thigh[Side] * Walking + Knees, FVector::ZeroVector, Weight, FQuat::Identity);
			}
			else if (Index == Rig.Calf[Side])
			{
				// The knee bends (the shin back) as the foot comes through, and as its hips sink.
				const float Leg = Gait.Thigh[Side] * Walking + Knees;
				AimLimb(Bone, Rig.CalfWay[Side], Leg - Gait.Knee[Side] * Walking - Knees * 2.f, FVector::ZeroVector, Weight, FQuat::Identity);
			}
		}
	}

	BonePose.Reset();
	for (const TPair<FName, int32>& Each : Rig.Emitted)
	{
		BonePose.Add(FCreatureBonePose{ Each.Key, Rig.Posed[Each.Value] });
	}
}
