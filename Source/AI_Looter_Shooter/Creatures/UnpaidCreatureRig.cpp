// AUnpaidCreature's body: what it reads from SK_Unpaid's skeleton (the bones it poses, the shroud's chains, how the
// fingers curl) and the pose it works out every frame on top of the model's rest pose, the idle hang. Every turn is about
// a bone's own joint, in the rest pose's frame (X forward, Y right, Z up), and a bone takes its parent's turn on top of
// its own, as a skeleton does; the shroud's links take their chain's swing (FShroudChain) under what the chain hangs from.
// Turns by the rest pose's axes: Pitch nods forward (a hanging limb swings back), Roll tips the top to the left (a hanging
// limb swings out to the right), Yaw turns to the right.

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Pawn.h"
#include "ReferenceSkeleton.h"

namespace
{
	FQuat TurnBy(const FVector& Axis, float Degrees)
	{
		return FQuat(Axis, FMath::DegreesToRadians(Degrees));
	}

	FQuat PitchBy(float Degrees)
	{
		return TurnBy(FVector::RightVector, Degrees);
	}

	FQuat RollBy(float Degrees)
	{
		return TurnBy(FVector::ForwardVector, Degrees);
	}

	FQuat YawBy(float Degrees)
	{
		return TurnBy(FVector::UpVector, Degrees);
	}

	float Ease(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	/** The strips run out of phase with the shroud between them, and with each other (radians). */
	constexpr float ChainPhases[3] = { 0.f, 2.2f, -2.2f };

	/** A lone link with nothing to measure its direction by hangs down and a little back. */
	const FVector LoneLinkDirection = FVector(-0.3f, 0.f, -1.f).GetSafeNormal();

	/** The hover: how far it bobs (cm) and how often (Hz). */
	constexpr float BobHeight = 3.f;
	constexpr float BobHz = 0.45f;

	/** How far it looks round at its target before its body has to turn (degrees). */
	constexpr float LookYawLimit = 60.f;
	constexpr float LookPitchLimit = 35.f;
}

// ---------------------------------------------------------------------------
// The rig
// ---------------------------------------------------------------------------

int32 AUnpaidCreature::FindPosedBone(FName Name) const
{
	return Name.IsNone() ? INDEX_NONE : Bones.IndexOfByPredicate([Name](const FPosedBone& Bone) { return Bone.Name == Name; });
}

bool AUnpaidCreature::SetupRig()
{
	Bones.Reset();
	BonePose.Reset();
	PelvisBone = SpineBone = ChestBone = CoalBone = NeckBone = HeadBone = JawBone = INDEX_NONE;
	for (FArm& Arm : Arms)
	{
		Arm = FArm();
	}
	for (int32 Chain = 0; Chain < static_cast<int32>(UE_ARRAY_COUNT(Chains)); ++Chain)
	{
		Chains[Chain].Links.Reset();
		ChainAnchors[Chain] = INDEX_NONE;
		ChainRestDirections[Chain].Reset();
	}

	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	if (!Model)
	{
		UE_LOG(LogLooter, Log, TEXT("%s has no model yet (SK_Unpaid, from Art/Models/Creatures/Unpaid.py): it hunts unseen, coal and all."),
			*GetName());
		return false;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();

	// The bones it can't do without, and the ones it poses if the model has them.
	const FUnpaidArmBones* ArmNames[2] = { &Rig.LeftArm, &Rig.RightArm };
	TArray<FName> Needed = { Rig.Pelvis, Rig.Spine, Rig.Chest, Rig.Neck, Rig.Head };
	TArray<FName> Wanted = { Rig.Coal, Rig.Jaw };
	for (const FUnpaidArmBones* Arm : ArmNames)
	{
		Needed.Append({ Arm->UpperArm, Arm->LowerArm, Arm->Hand });
		Wanted.Append(Arm->Fingers);
	}
	Wanted.Append(Rig.Shroud);
	Wanted.Append(Rig.LeftStrip);
	Wanted.Append(Rig.RightStrip);
	TArray<FString> Missing;
	for (const FName Name : Needed)
	{
		if (Skeleton.FindBoneIndex(Name) == INDEX_NONE)
		{
			Missing.Add(Name.ToString());
		}
	}
	if (!Missing.IsEmpty())
	{
		UE_LOG(LogLooter, Error, TEXT("%s: %s has no %s, which the Unpaid poses (set its Rig to the model's bone names); it stays in its rest pose."),
			*GetName(), *Model->GetName(), *FString::Join(Missing, TEXT(", ")));
		return false;
	}

	// Parents first (a skeleton lists every bone after its parent), each with its rest pose and the nearest posed bone above
	// it: a bone between that it doesn't pose keeps its rest place on its parent, and so follows.
	TArray<int32> Indices;
	for (const FName Name : Needed)
	{
		Indices.AddUnique(Skeleton.FindBoneIndex(Name));
	}
	for (const FName Name : Wanted)
	{
		const int32 Index = Name.IsNone() ? INDEX_NONE : Skeleton.FindBoneIndex(Name);
		if (Index != INDEX_NONE)
		{
			Indices.AddUnique(Index);
		}
	}
	Indices.Sort();
	for (const int32 Index : Indices)
	{
		FPosedBone& Bone = Bones.AddDefaulted_GetRef();
		Bone.Name = Skeleton.GetBoneName(Index);
		Bone.SkeletonIndex = Index;
		Bone.Rest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index);
		Bone.Posed = Bone.Rest;
		for (int32 Above = Skeleton.GetParentIndex(Index); Above != INDEX_NONE && Bone.Parent == INDEX_NONE; Above = Skeleton.GetParentIndex(Above))
		{
			Bone.Parent = Bones.IndexOfByPredicate([Above](const FPosedBone& Each) { return Each.SkeletonIndex == Above; });
		}
	}
	PelvisBone = FindPosedBone(Rig.Pelvis);
	SpineBone = FindPosedBone(Rig.Spine);
	ChestBone = FindPosedBone(Rig.Chest);
	CoalBone = FindPosedBone(Rig.Coal);
	NeckBone = FindPosedBone(Rig.Neck);
	HeadBone = FindPosedBone(Rig.Head);
	JawBone = FindPosedBone(Rig.Jaw);

	// The arms. Each finger curls toward the palm, which faces the body in the idle hang: about the axis square to the way
	// it points from the wrist and to the arm's side. They fan out round the middle one, thumb first.
	for (int32 Side = 0; Side < 2; ++Side)
	{
		FArm& Arm = Arms[Side];
		const FUnpaidArmBones& Names = *ArmNames[Side];
		Arm.Side = Side == 0 ? -1.f : 1.f;
		Arm.UpperArm = FindPosedBone(Names.UpperArm);
		Arm.LowerArm = FindPosedBone(Names.LowerArm);
		Arm.Hand = FindPosedBone(Names.Hand);
		const FVector Out(0.f, Arm.Side, 0.f);
		const float Middle = (Names.Fingers.Num() - 1) * 0.5f;
		for (int32 Slot = 0; Slot < Names.Fingers.Num(); ++Slot)
		{
			const int32 Finger = FindPosedBone(Names.Fingers[Slot]);
			if (Finger == INDEX_NONE)
			{
				continue;
			}
			const FVector Pointing = Bones[Finger].Rest.GetLocation() - Bones[Arm.Hand].Rest.GetLocation();
			FVector Axis = FVector::CrossProduct(Pointing.GetSafeNormal(), Out);
			if (!Axis.Normalize())
			{
				Axis = FVector::ForwardVector;
			}
			Arm.Fingers.Add(Finger);
			Arm.CurlAxes.Add(Axis);
			Arm.FanOffsets.Add(Slot - Middle);
		}
	}

	// The shroud and its strips: each a chain from its first link down to its first missing one, hanging from what its
	// first link hangs from (the pelvis for the shroud; the shroud's second link, tail_02, for each strip). Each link
	// points at the next one at rest; the last keeps the one before's way.
	const TArray<FName>* ChainNames[3] = { &Rig.Shroud, &Rig.LeftStrip, &Rig.RightStrip };
	for (int32 Chain = 0; Chain < 3; ++Chain)
	{
		TArray<int32> Links;
		for (const FName Name : *ChainNames[Chain])
		{
			const int32 Link = FindPosedBone(Name);
			if (Link == INDEX_NONE)
			{
				break;
			}
			Links.Add(Link);
		}
		if (Links.IsEmpty())
		{
			continue;
		}
		ChainAnchors[Chain] = Bones[Links[0]].Parent;
		TArray<float> Angles;
		for (int32 Link = 0; Link < Links.Num(); ++Link)
		{
			const FVector Joint = Bones[Links[Link]].Rest.GetLocation();
			FVector Direction = Link + 1 < Links.Num() ? Bones[Links[Link + 1]].Rest.GetLocation() - Joint
				: (Link > 0 ? Joint - Bones[Links[Link - 1]].Rest.GetLocation() : LoneLinkDirection);
			if (!Direction.Normalize())
			{
				Direction = LoneLinkDirection;
			}
			ChainRestDirections[Chain].Add(Direction);
			Angles.Add(FShroudChain::HangAngle(Direction));
			Bones[Links[Link]].Chain = Chain;
			Bones[Links[Link]].Link = Link;
		}
		Chains[Chain].Init(Angles, ChainPhases[Chain]);
	}

	BonePose.Reset();
	for (const FPosedBone& Bone : Bones)
	{
		BonePose.Add({ Bone.Name, Bone.Rest });
	}
	bPoseStarted = false;
	UE_LOG(LogLooter, Verbose, TEXT("%s poses %d bones of %s (shroud %d links, strips %d and %d)."), *GetName(), Bones.Num(), *Model->GetName(),
		Chains[0].Links.Num(), Chains[1].Links.Num(), Chains[2].Links.Num());
	return true;
}

// ---------------------------------------------------------------------------
// The pose
// ---------------------------------------------------------------------------

AUnpaidCreature::FPoseChannels AUnpaidCreature::PoseTargets(float Speed, float Time) const
{
	FPoseChannels Goal;
	const float Moving = FMath::Min(Speed, 1.f);
	// Drifting or hanging still: a little hunched, leaning into its drift, the jaw slack and twitching now and then.
	Goal.Lean = 4.f + 10.f * Moving;
	Goal.Jaw = 6.f + 2.5f * FMath::Pow(FMath::Max(FMath::Sin(Time * 0.9f), 0.f), 4.f) * FMath::Sin(Time * 11.f);
	Goal.Curl = 25.f;
	Goal.Splay = 4.f;

	const float InState = GetStateTime();
	switch (GetCreatureState())
	{
	case ECreatureState::Chase:
		// Hungry: thrust forward, mouth open, hands half clawed.
		Goal.Lean = 6.f + 16.f * Moving;
		Goal.Jaw = 12.f;
		Goal.Curl = 30.f;
		Goal.Splay = 8.f;
		break;

	case ECreatureState::Attack:
		if (InState < AttackWindup)
		{
			// The shriek: it rears back with its arms flung wide and its fingers splayed, the jaw dropping as far as it goes.
			const float Rise = Ease(InState / FMath::Max(AttackWindup, 0.01f));
			Goal.Rear = Rise;
			Goal.Spread = Rise;
			Goal.Lean = -8.f * Rise;
			Goal.Jaw = FMath::Lerp(12.f, JawOpenDegrees, Rise);
			Goal.Curl = -5.f;
			Goal.Splay = 18.f;
		}
		else
		{
			// The lunge: thrown forward, arms reaching long and clawing, the shroud snapped straight; the recovery eases back.
			const float After = (InState - AttackWindup) / FMath::Max(AttackRecovery, 0.01f);
			const float Reach = bLunging ? 1.f : 1.f - Ease((After - 0.3f) / 0.7f);
			Goal.Reach = Reach;
			Goal.Spread = 0.15f * Reach;
			Goal.Lean = 6.f + 22.f * Reach;
			Goal.Jaw = FMath::Lerp(12.f, JawOpenDegrees * 0.75f, Reach);
			Goal.Curl = FMath::Lerp(30.f, 48.f, Reach);
			Goal.Splay = 12.f * Reach;
			Goal.Snap = bLunging ? 1.f : 0.f;
		}
		break;

	case ECreatureState::Dead:
		// It slumps as it goes: head down, arms hanging, hands curled, the jaw fallen open.
		Goal.Death = 1.f;
		Goal.Lean = 35.f;
		Goal.Jaw = 25.f;
		Goal.Curl = 50.f;
		Goal.Splay = 0.f;
		break;

	default:
		break;
	}

	// It looks at whoever it's after, as far as its neck turns; otherwise it looks about, slowly.
	if (const APawn* Victim = GetTarget(); Victim && !IsDead())
	{
		const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 0.7f * GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		const FVector To = FRotator(0.0, GetActorRotation().Yaw, 0.0).UnrotateVector(Victim->GetActorLocation() + FVector(0.f, 0.f, 60.f) - Eye);
		Goal.LookYaw = FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X))), -LookYawLimit, LookYawLimit);
		Goal.LookPitch = FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(To.Z, To.Size2D()))), -LookPitchLimit, LookPitchLimit);
	}
	else if (!IsDead())
	{
		Goal.LookYaw = 25.f * FMath::Sin(Time * 0.35f);
		Goal.LookPitch = -5.f + 4.f * FMath::Sin(Time * 0.23f);
	}
	return Goal;
}

void AUnpaidCreature::AnimateBody(float DeltaSeconds)
{
	if (!bRigReady)
	{
		return;
	}
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	PoseTime += Dt;
	const float Time = PoseTime + PoseSeed;

	// How it moves: its drift in its own frame, and how fast it turns.
	const FRotator Facing(0.0, GetActorRotation().Yaw, 0.0);
	const FVector Local = IsDead() ? FVector::ZeroVector : Facing.UnrotateVector(GetVelocity());
	const float Speed = FMath::Clamp(static_cast<float>(Local.Size2D()) / FMath::Max(ChaseSpeed, 1.f), 0.f, 1.3f);
	const float YawNow = static_cast<float>(GetActorRotation().Yaw);
	const float YawRate = Dt > 0.f ? FMath::FindDeltaAngleDegrees(LastYaw, YawNow) / Dt : 0.f;
	LastYaw = YawNow;

	// The channels ease toward where the brain puts them (quicker through an attack), or start there.
	const FPoseChannels Goal = PoseTargets(Speed, Time);
	if (!bPoseStarted || Dt <= 0.f)
	{
		Pose = Goal;
		bPoseStarted = true;
	}
	else
	{
		const float Rate = GetCreatureState() == ECreatureState::Attack ? 18.f : 9.f;
		auto Follow = [Dt](float& Value, float To, float Quickness) { Value = FMath::FInterpTo(Value, To, Dt, Quickness); };
		Follow(Pose.Lean, Goal.Lean, Rate);
		Follow(Pose.Rear, Goal.Rear, Rate);
		Follow(Pose.Reach, Goal.Reach, Rate);
		Follow(Pose.Spread, Goal.Spread, Rate);
		Follow(Pose.Jaw, Goal.Jaw, Rate);
		Follow(Pose.Curl, Goal.Curl, Rate);
		Follow(Pose.Splay, Goal.Splay, Rate);
		// The head turns, it doesn't snap; the strips fly straight with the lunge at once and drift back after it.
		Follow(Pose.LookYaw, Goal.LookYaw, 5.f);
		Follow(Pose.LookPitch, Goal.LookPitch, 5.f);
		Follow(Pose.Snap, Goal.Snap, Goal.Snap > Pose.Snap ? 25.f : 4.f);
		Follow(Pose.Death, Goal.Death, 3.f);
	}
	Flinch = FMath::FInterpTo(Flinch, 0.f, Dt, 6.f);
	const float Moving = FMath::Min(Speed, 1.f);
	const float Alive = 1.f - Pose.Death;
	// It banks into its turns, a little.
	const float Bank = FMath::Clamp(YawRate * 0.04f, -10.f, 10.f) * Alive;
	const float FlinchBack = 8.f * Flinch * static_cast<float>(FlinchAway.X);
	const float FlinchSide = 8.f * Flinch * static_cast<float>(FlinchAway.Y);

	// The body hovers, bobbing, and leans into its drift; the shriek rears it back and the lunge throws it forward. A hit
	// jolts it away.
	if (PelvisBone != INDEX_NONE)
	{
		FPosedBone& Pelvis = Bones[PelvisBone];
		const float Bob = BobHeight * FMath::Sin(2.f * UE_PI * BobHz * Time) * (1.f - 0.5f * Moving) * Alive;
		Pelvis.Own = PitchBy(0.4f * Pose.Lean + FlinchBack) * RollBy(-Bank - FlinchSide);
		Pelvis.Shift = FVector(0.f, 0.f, Bob - 4.f * Pose.Reach - 25.f * Pose.Death) + FlinchAway * (6.f * Flinch);
	}
	if (SpineBone != INDEX_NONE)
	{
		Bones[SpineBone].Own = PitchBy(0.3f * Pose.Lean + 1.5f * FMath::Sin(Time * 1.9f));
	}
	if (ChestBone != INDEX_NONE)
	{
		Bones[ChestBone].Own = PitchBy(0.3f * Pose.Lean - 8.f * Pose.Rear) * RollBy(-0.3f * Bank);
	}
	if (CoalBone != INDEX_NONE)
	{
		// The coal throbs, and swells as it flares.
		Bones[CoalBone].Scale = 1.f + 0.06f * FMath::Sin(2.f * UE_PI * 0.8f * Time) + 0.25f * FMath::Max(Heat, 0.f);
	}
	// Neck and head look at its target, holding most of the body's lean off so it keeps its eyes on them.
	if (NeckBone != INDEX_NONE)
	{
		Bones[NeckBone].Own = YawBy(0.4f * Pose.LookYaw) * PitchBy(-0.4f * Pose.LookPitch - 0.35f * Pose.Lean + 6.f * Pose.Reach);
	}
	if (HeadBone != INDEX_NONE)
	{
		Bones[HeadBone].Own = YawBy(0.6f * Pose.LookYaw)
			* PitchBy(-0.6f * Pose.LookPitch - 0.35f * Pose.Lean - 15.f * Pose.Rear + 8.f * Pose.Reach + 30.f * Pose.Death);
	}
	if (JawBone != INDEX_NONE)
	{
		Bones[JawBone].Own = PitchBy(Pose.Jaw);
	}

	// The arms hang and drift, trail a little as it glides, fling wide for the shriek and reach long in the lunge.
	for (const FArm& Arm : Arms)
	{
		const float Side = Arm.Side;
		const float Drift = 3.f * FMath::Sin(Time * 1.1f + 0.8f * Side);
		const float Swing = Drift + 10.f * Moving * (1.f - Pose.Reach) + 15.f * Pose.Spread - 80.f * Pose.Reach + 10.f * Pose.Death;
		const float Out = 6.f * Moving + 55.f * Pose.Spread + 12.f * Pose.Reach;
		const float Bend = 12.f + 18.f * Moving + 30.f * Pose.Spread - 6.f * Pose.Reach + 25.f * Pose.Death;
		if (Arm.UpperArm != INDEX_NONE)
		{
			Bones[Arm.UpperArm].Own = PitchBy(Swing) * RollBy(Side * Out);
		}
		if (Arm.LowerArm != INDEX_NONE)
		{
			Bones[Arm.LowerArm].Own = PitchBy(-Bend);
		}
		if (Arm.Hand != INDEX_NONE)
		{
			Bones[Arm.Hand].Own = PitchBy(15.f * Pose.Spread - 12.f * Pose.Reach);
		}
		for (int32 Finger = 0; Finger < Arm.Fingers.Num(); ++Finger)
		{
			Bones[Arm.Fingers[Finger]].Own = TurnBy(Arm.CurlAxes[Finger], -Pose.Curl) * PitchBy(Arm.FanOffsets[Finger] * Pose.Splay);
		}
	}

	// The shroud and its strips trail the drift, out of phase with each other, and snap straight with the lunge.
	for (int32 Chain = 0; Chain < static_cast<int32>(UE_ARRAY_COUNT(Chains)); ++Chain)
	{
		if (Chains[Chain].Links.Num() > 0)
		{
			Chains[Chain].Step(Shroud, Local, YawRate * Alive, Pose.Snap, Dt);
		}
	}

	SolvePose();
	for (int32 Index = 0; Index < Bones.Num() && Index < BonePose.Num(); ++Index)
	{
		BonePose[Index].Transform = Bones[Index].Posed;
	}
}

void AUnpaidCreature::SolvePose()
{
	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		FPosedBone& Bone = Bones[Index];
		FQuat Above = FQuat::Identity;
		FVector Joint = Bone.Rest.GetLocation();
		if (Bone.Parent != INDEX_NONE)
		{
			// Its joint rides on its parent: where the parent's turn carries it.
			const FPosedBone& Parent = Bones[Bone.Parent];
			Above = Parent.Turned;
			Joint = Parent.Posed.GetLocation() + Parent.Turned.RotateVector(Bone.Rest.GetLocation() - Parent.Rest.GetLocation());
		}
		Joint += Bone.Shift;
		if (Bone.Chain != INDEX_NONE)
		{
			// A shroud link swings as its chain says, under what the chain hangs from (the pelvis's lean carries it all).
			const int32 Anchor = ChainAnchors[Bone.Chain];
			const FQuat Hung = Anchor != INDEX_NONE ? Bones[Anchor].Turned : FQuat::Identity;
			Bone.Turned = Hung * Chains[Bone.Chain].LinkRotation(Bone.Link, ChainRestDirections[Bone.Chain][Bone.Link]);
		}
		else
		{
			Bone.Turned = Above * Bone.Own;
		}
		Bone.Posed = FTransform(Bone.Turned * Bone.Rest.GetRotation(), Joint, Bone.Rest.GetScale3D() * Bone.Scale);
	}
}
