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
#include "GameFramework/CharacterMovementComponent.h"
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

	/** The strips run out of phase with the shroud between them, and with each other (radians). */
	constexpr float ChainPhases[3] = { 0.f, 2.2f, -2.2f };

	/** A lone link with nothing to measure its direction by hangs down and a little back. */
	const FVector LoneLinkDirection = FVector(-0.3f, 0.f, -1.f).GetSafeNormal();

	/** The hover: how far it bobs (cm) and how often (Hz). */
	constexpr float BobHeight = 3.f;
	constexpr float BobHz = 0.45f;

	/** How quickly its bank follows its turn (1/s): a full bank builds over about a quarter of a second. */
	constexpr float BankRate = 8.f;

	/**
	 * Its crumple as it dies (Crumple, 0 to 1; not a boss's): the hips sink this far (cm) and the trunk folds forward this
	 * much at the hips, waist and head (degrees), on top of the Unpaid's slump (Death).
	 */
	constexpr float CrumpleSink = 60.f;
	constexpr float CrumpleHips = 35.f;
	constexpr float CrumpleWaist = 12.f;
	constexpr float CrumpleHead = 20.f;
	/** Its look snaps round this much quicker in its notice beat (1/s) than its usual turn of the head. */
	constexpr float LookRate = 5.f;
	constexpr float NoticeLookRate = 14.f;

	/** The most the shroud lifts off a slope rising behind it (degrees): the steepest ground it drifts over. */
	constexpr float MaxShroudLift = 40.f;

	/**
	 * The idle the model rests in (Unpaid.py bakes it into SK_Unpaid's rest pose, so the bestiary's stand and anything
	 * else that shows it unposed show the approved idle): the fingers curled 25 degrees and fanned 4 per finger, the jaw 6
	 * past a mouth 10 open, each forearm bent 12 (that one is UnpaidCreatureArms.cpp's). The pose's channels keep meaning
	 * what they did on the old rest pose, and these come back off at the bones.
	 */
	constexpr float RestCurl = 25.f;
	constexpr float RestSplay = 4.f;
	constexpr float RestJaw = 6.f;
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
	// A body built on this rig poses bones of its own too (Abel's lantern, gun and skirts).
	GetExtraRigBones(Wanted);
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
			Arm.IdleTurns.Add(TurnBy(Axis, -RestCurl) * PitchBy((Slot - Middle) * RestSplay));
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
	// The arms keep out of the torso by the model's own hit hulls (UnpaidCreatureArms.cpp).
	SetupArmGuard();
	bPoseStarted = false;
	UE_LOG(LogLooter, Verbose, TEXT("%s poses %d bones of %s (shroud %d links, strips %d and %d)."), *GetName(), Bones.Num(), *Model->GetName(),
		Chains[0].Links.Num(), Chains[1].Links.Num(), Chains[2].Links.Num());
	return true;
}

// ---------------------------------------------------------------------------
// The pose (what each moment asks of it, PoseTargets: UnpaidCreatureMoods.cpp)
// ---------------------------------------------------------------------------

void AUnpaidCreature::AnimateBody(float DeltaSeconds)
{
	if (!bRigReady)
	{
		return;
	}
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	// Its element statuses' pose this frame (UnpaidCreatureStatus.cpp); salted, its clock runs slow.
	StatusShown = IsDead() ? FCreatureStatusPose() : CreatureStatusPose::Of(this);
	PoseTime += Dt * StatusShown.ClockScale;
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
		// The head turns, it doesn't snap (but in its notice beat, when it has just seen them); the strips fly straight with
		// the lunge at once and drift back after it.
		const float Look = IsNoticing() ? NoticeLookRate : LookRate;
		Follow(Pose.LookYaw, Goal.LookYaw, Look);
		Follow(Pose.LookPitch, Goal.LookPitch, Look);
		Follow(Pose.Snap, Goal.Snap, Goal.Snap > Pose.Snap ? 25.f : 4.f);
		// Dying, it crumples, quicker than it slumps (twice as quick again snuffed: HITS); calm, it sways slowly where it hangs.
		Follow(Pose.Death, Goal.Death, 3.f * Hurt.CrumpleRate());
		Follow(Pose.Crumple, Goal.Crumple, 4.5f * Hurt.CrumpleRate());
		Follow(Pose.Sway, Goal.Sway, 2.f);
		// The arms come up as the hunt starts, not at once; the shriek's own channels take over quickly.
		Follow(Pose.Hunt, Goal.Hunt, GetCreatureState() == ECreatureState::Attack ? Rate : 4.f);
		// A rake's swipe snaps across quickly (about a third of the way a frame at 60: a blur, yet no bone leaps 45 cm in a
		// frame, Looter.Creatures.Anim.UnpaidArms) and drifts back after.
		for (int32 Side = 0; Side < 2; ++Side)
		{
			Follow(Pose.Rake[Side], Goal.Rake[Side], Goal.Rake[Side] > Pose.Rake[Side] + 0.3f ? 16.f : 8.f);
		}
	}
	// A hit's flinch rises over a few frames and eases off (UnpaidCreatureArms.cpp).
	StepFlinch(Dt);
	// Its hits' own channels (HITS, UnpaidCreatureHurt.cpp): the twist, the head's knock and whip, the jaw, the coal.
	Hurt.Step(Dt);
	const float Moving = FMath::Min(Speed, 1.f);
	const float Alive = 1.f - Pose.Death;
	// It banks into its turns a little, eased: followed straight, a turn's start threw the head and hat 30-50 cm in a frame.
	const float WantedBank = FMath::Clamp(YawRate * 0.04f, -10.f, 10.f) * Alive;
	Bank = Dt > 0.f ? FMath::FInterpTo(Bank, WantedBank, Dt, BankRate) : WantedBank;
	const float FlinchBack = Hurt.PelvisGain * Flinch * static_cast<float>(FlinchAway.X);
	const float FlinchSide = Hurt.PelvisGain * Flinch * static_cast<float>(FlinchAway.Y);
	// A stagger folds it at the waist away from the hit and sinks it; a blast's kill throws it back along the shot (HITS).
	const FVector2D Fold = Hurt.StaggerFold();
	const FVector2D Thrown = Hurt.KillTilt(DeathTime);

	// The body hovers, bobbing, and leans into its drift; the shriek rears it back and the lunge throws it forward. A hit
	// jolts it away.
	if (PelvisBone != INDEX_NONE)
	{
		FPosedBone& Pelvis = Bones[PelvisBone];
		// Badly hurt it hangs heavier: half its bob.
		const float Bob = BobHeight * FMath::Sin(2.f * UE_PI * BobHz * Time) * (1.f - 0.5f * Moving) * Alive * Hurt.BobShare();
		// Rising out of the ground it comes up from under it (RiseDrop); crumpling it sinks and folds over.
		Pelvis.Own = PitchBy(0.4f * Pose.Lean + FlinchBack + CrumpleHips * Pose.Crumple + Thrown.X)
			* RollBy(-Bank - FlinchSide + Pose.Sway + Thrown.Y);
		Pelvis.Shift = FVector(0.f, 0.f, Bob - 4.f * Pose.Reach - 25.f * Pose.Death - CrumpleSink * Pose.Crumple + RiseDrop() - Hurt.StaggerSink())
			+ FlinchAway * (6.f * Flinch) + Hurt.KillShift(DeathTime);
	}
	if (SpineBone != INDEX_NONE)
	{
		// The shriek's rear bends mostly at the waist: the chest folding back over it presses the armpits into the waist
		// (all at the chest it did badly, an even split still 2-3 cm). The same 8 degrees back in all. A wound off its middle
		// twists it here, and a stagger folds it here.
		Bones[SpineBone].Own = YawBy(Hurt.TwistYaw())
			* PitchBy(0.3f * Pose.Lean + 1.5f * FMath::Sin(Time * 1.9f) - 6.f * Pose.Rear + CrumpleWaist * Pose.Crumple + Fold.X)
			* RollBy(Fold.Y);
	}
	if (ChestBone != INDEX_NONE)
	{
		// The chest lags the hips' jolt a little (the whiplash's start).
		Bones[ChestBone].Own = PitchBy(0.3f * Pose.Lean - 2.f * Pose.Rear + Hurt.ChestPitch(FlinchBack)) * RollBy(-0.3f * Bank);
	}
	if (CoalBone != INDEX_NONE)
	{
		// The coal throbs, and swells as it flares (and as a crit lands on it); snuffed, it swells once and goes out.
		Bones[CoalBone].Scale = (1.f + 0.06f * FMath::Sin(2.f * UE_PI * 0.8f * Time) + 0.25f * FMath::Max(Heat, 0.f) + Hurt.CoalPulse())
			* Hurt.CoalDeathScale(DeathTime);
	}
	// Neck and head look at its target, holding most of the body's lean off so it keeps its eyes on them. A hit knocks the
	// head, lags it behind the body's jolt and whips it on after; from behind it whips round toward the shooter.
	if (NeckBone != INDEX_NONE)
	{
		Bones[NeckBone].Own = YawBy(0.4f * Pose.LookYaw)
			* PitchBy(-0.4f * Pose.LookPitch - 0.35f * Pose.Lean + 6.f * Pose.Reach + Hurt.NeckPitch());
	}
	if (HeadBone != INDEX_NONE)
	{
		Bones[HeadBone].Own = YawBy(0.6f * Pose.LookYaw + Hurt.HeadYaw())
			* PitchBy(-0.6f * Pose.LookPitch - 0.35f * Pose.Lean - 15.f * Pose.Rear + 8.f * Pose.Reach + 30.f * Pose.Death
				+ CrumpleHead * Pose.Crumple + Hurt.HeadPitch(FlinchBack));
	}
	if (JawBone != INDEX_NONE)
	{
		Bones[JawBone].Own = PitchBy(Pose.Jaw - RestJaw + Hurt.JawOpen());
	}
	// Its statuses' tremors, twists, jerk and cough on the trunk (UnpaidCreatureStatus.cpp).
	LayStatusBones();

	// The shroud lifts off ground that rises behind it (it trails a meter back, low): the floor's slope along its facing,
	// eased so bumps don't jerk it. Read off the movement's floor, so it costs no trace.
	float WantedLift = 0.f;
	const FFindFloorResult& Floor = GetCharacterMovement()->CurrentFloor;
	if (!IsDead() && Floor.bBlockingHit && Floor.HitResult.ImpactNormal.Z > 0.5)
	{
		const FVector Normal = Floor.HitResult.ImpactNormal;
		const double Rise = FVector::DotProduct(Normal, Facing.Vector());
		WantedLift = FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Rise, Normal.Z))), 0.f, MaxShroudLift);
	}
	ShroudLift = Dt > 0.f ? FMath::FInterpTo(ShroudLift, WantedLift, Dt, 4.f) : WantedLift;

	// The shroud and its strips trail the drift, out of phase with each other, and snap straight with the lunge; its
	// statuses shake it on top (jittering jolted, flapping burning).
	const FVector ShroudDrift = Local + StatusShroudShake();
	for (int32 Chain = 0; Chain < static_cast<int32>(UE_ARRAY_COUNT(Chains)); ++Chain)
	{
		if (Chains[Chain].Links.Num() > 0)
		{
			Chains[Chain].Step(Shroud, ShroudDrift, YawRate * Alive, Pose.Snap, Dt);
		}
	}

	// The arms (UnpaidCreatureArms.cpp), last: adrift at rest, clawing as it hunts, flung wide for the shriek, long in the
	// lunge, lagging its moves, and kept out of its torso as this frame poses it, shroud and all.
	PoseArms(Dt, Time, Speed, Local, YawRate);

	// A body built on this rig lays its own pose over the channels' (Abel's pose table).
	LayerPose(Dt);
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
			// A shroud link swings as its chain says, under what the chain hangs from (the pelvis's lean carries it all). A
			// chain hung from the body lifts with the ground rising behind it (ShroudLift, about the body's right-hand axis); a
			// strip hung from the shroud has that lift from its link already.
			const int32 Anchor = ChainAnchors[Bone.Chain];
			const bool bFromBody = Anchor == INDEX_NONE || Bones[Anchor].Chain == INDEX_NONE;
			const FQuat Lift = bFromBody && ShroudLift > 0.f ? PitchBy(ShroudLift) : FQuat::Identity;
			const FQuat Hung = Lift * (Anchor != INDEX_NONE ? Bones[Anchor].Turned : FQuat::Identity);
			const FQuat Swung = Hung * Chains[Bone.Chain].LinkRotation(Bone.Link, ChainRestDirections[Bone.Chain][Bone.Link]);
			// Knelt or sat (Abel), the links lie as their own turns lay them, the chain's swing blended out.
			Bone.Turned = ChainSwing >= 1.f ? Swung : FQuat::Slerp(Above * Bone.Own, Swung, FMath::Clamp(ChainSwing, 0.f, 1.f)).GetNormalized();
		}
		else
		{
			Bone.Turned = Above * Bone.Own;
		}
		Bone.Posed = FTransform(Bone.Turned * Bone.Rest.GetRotation(), Joint, Bone.Rest.GetScale3D() * Bone.Scale);
	}
}
