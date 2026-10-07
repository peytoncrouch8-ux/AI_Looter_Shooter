// AHobBird's rig: what he reads from SK_Hob's skeleton and the pose he works out every frame on its rest pose (perched,
// wings folded, head level, front +X). As with the Unpaid (UnpaidCreatureRig.cpp), each turn is about a bone's own joint
// in the model's frame (X forward, Y right, Z up), and a bone takes its parent's turn on top of its own. The wings don't
// open by turning the folded wing: each wing bone blends from its rest to its open place and turn (Art/Models/Creatures/
// Hob.py's open_pose(), printed in Unreal's frame by its unreal_open_table()), and the whole wing then beats about the
// shoulder.

#include "Story/HobBird.h"
#include "AI_Looter_Shooter.h"
#include "Story/SpeakerPointComponent.h"
#include "AnimationRuntime.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

namespace
{
	/**
	 * A turn as Hob.py writes one (degrees: yaw about Z, pitch about Y with the nose going down, roll about X, in his
	 * own frame with Y to his left), in the model's frame here, where Y runs to his right: yaw and roll change sign.
	 */
	FQuat HobTurn(float Yaw, float Pitch, float Roll)
	{
		return FQuat(FVector::UpVector, FMath::DegreesToRadians(-Yaw)) * FQuat(FVector::RightVector, FMath::DegreesToRadians(Pitch))
			* FQuat(FVector::ForwardVector, FMath::DegreesToRadians(-Roll));
	}

	/** A head and neck he holds for a while: Hob.py's (yaw, pitch, roll) for each. */
	struct FHeadStep
	{
		float Head[3];
		float Neck[3];
	};

	/**
	 * The poses he steps between, perched: level; the art session's tilt (the approved renders), and its mirror; a
	 * glance; a look down at whoever's below; a look up.
	 */
	const FHeadStep HeadSteps[] = {
		{ { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f } },
		{ { -28.f, 4.f, -16.f }, { -8.f, 0.f, -4.f } },
		{ { 28.f, 4.f, 16.f }, { 8.f, 0.f, 4.f } },
		{ { -14.f, 4.f, -6.f }, { 0.f, 0.f, 0.f } },
		{ { 12.f, 12.f, 8.f }, { 4.f, 5.f, 0.f } },
		{ { 0.f, -10.f, 0.f }, { 0.f, -4.f, 0.f } },
	};
	/** The art session's tilt: the one he holds while he talks. */
	constexpr int32 TalkStep = 1;

	/** A crow's head moves in small sudden steps: snapped over this long, then held this long (seconds). */
	constexpr float StepSeconds = 0.09f;
	constexpr float HoldMin = 0.9f;
	constexpr float HoldMax = 3.2f;

	/** A ruffle now and then: its length and the gap between (seconds), how far it shakes his body (degrees). */
	constexpr float RuffleSeconds = 0.6f;
	constexpr float RuffleMin = 7.f;
	constexpr float RuffleMax = 14.f;
	constexpr float RuffleShake = 3.f;

	/** Breathing: how far his body rises and settles (cm) and how often (Hz). */
	constexpr float BreathCm = 0.12f;
	constexpr float BreathHz = 1.3f;

	/** In flight: the beat's middle and reach about the shoulder, and its pace; the flare over the drop at the end. */
	constexpr float FlapMiddle = 12.f;
	constexpr float FlapReach = 30.f;
	constexpr float FlapHz = 4.5f;
	constexpr float FlareFlap = 42.f;

	/** One wing bone's open pose (the left wing's; the right mirrors it across his middle). */
	struct FWingOpen
	{
		const TCHAR* Bone;
		FVector RestHead;
		FVector OpenHead;
		FVector Axis;
		float Degrees;
	};

	/** Hob.py's unreal_open_table(), as the art session gave it (cm; X forward, Y right, Z up). */
	const FWingOpen LeftWing[4] = {
		{ TEXT("wing_upper_l"), FVector(1.1, -2.8, 16.5), FVector(6.7, -6.5, 15.4), FVector(-0.77, 0.58, -0.28), 60.f },
		{ TEXT("wing_fore_l"), FVector(-5.2, -3.0, 11.5), FVector(0.8, -8.6, 15.9), FVector(-0.84, 0.53, -0.10), 73.f },
		{ TEXT("wing_hand_l"), FVector(3.5, -5.8, 13.7), FVector(2.6, -16.9, 17.2), FVector(-0.99, 0.11, -0.09), 82.f },
		{ TEXT("wing_fan_l"), FVector(-2.6, -5.2, 9.1), FVector(1.2, -24.5, 18.1), FVector(-0.87, -0.23, 0.44), 115.f },
	};

	/** A wing bone's name on Side (0 left, 1 right). */
	FName WingBoneName(int32 Side, int32 Part)
	{
		FString Name = LeftWing[Part].Bone;
		if (Side == 1)
		{
			Name.RemoveFromEnd(TEXT("_l"));
			Name += TEXT("_r");
		}
		return FName(*Name);
	}

	/** Its head's move from rest to open, and its open turn, on Side: the right wing mirrors the left across Y = 0. */
	FVector WingMove(int32 Side, int32 Part)
	{
		const FVector Move = LeftWing[Part].OpenHead - LeftWing[Part].RestHead;
		return Side == 0 ? Move : FVector(Move.X, -Move.Y, Move.Z);
	}

	FQuat WingTurn(int32 Side, int32 Part)
	{
		const FVector& Axis = LeftWing[Part].Axis;
		const FVector Mirrored = Side == 0 ? Axis : FVector(-Axis.X, Axis.Y, -Axis.Z);
		return FQuat(Mirrored.GetSafeNormal(), FMath::DegreesToRadians(LeftWing[Part].Degrees));
	}

	// Named apart from the story character's Body, which a member function would find first.
	const FName BodyName(TEXT("body"));
	const FName NeckName(TEXT("neck"));
	const FName HeadName(TEXT("head"));
	const FName TailName(TEXT("tail"));
}

int32 AHobBird::FindRigBone(FName Name) const
{
	return RigBones.IndexOfByPredicate([Name](const FHobBone& Bone) { return Bone.Name == Name; });
}

bool AHobBird::SetupRig()
{
	RigBones.Reset();
	BodyBone = NeckBone = HeadBone = TailBone = INDEX_NONE;
	const USkeletalMesh* Model = Bird ? Cast<USkeletalMesh>(Bird->GetSkinnedAsset()) : nullptr;
	if (!Model)
	{
		return false;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	TArray<FName> Wanted = { BodyName, NeckName, HeadName, TailName };
	for (int32 Side = 0; Side < 2; ++Side)
	{
		for (int32 Part = 0; Part < 4; ++Part)
		{
			Wanted.Add(WingBoneName(Side, Part));
		}
	}
	TArray<int32> Indices;
	for (const FName Name : Wanted)
	{
		const int32 Index = Skeleton.FindBoneIndex(Name);
		if (Index == INDEX_NONE)
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: %s has no bone %s, which Hob poses; he stays in his rest pose."), *GetName(), *Model->GetName(),
				*Name.ToString());
			return false;
		}
		Indices.AddUnique(Index);
	}
	// Parents first (a skeleton lists every bone after its parent), each with its rest pose and the nearest posed bone
	// above it. The bones he doesn't pose (the jaw, the beak, the tail's fans, the legs) keep their rest on their parents.
	Indices.Sort();
	for (const int32 Index : Indices)
	{
		FHobBone& Bone = RigBones.AddDefaulted_GetRef();
		Bone.Name = Skeleton.GetBoneName(Index);
		Bone.Rest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index);
		Bone.Posed = Bone.Rest;
		for (int32 Above = Skeleton.GetParentIndex(Index); Above != INDEX_NONE && Bone.Parent == INDEX_NONE; Above = Skeleton.GetParentIndex(Above))
		{
			const FName AboveName = Skeleton.GetBoneName(Above);
			Bone.Parent = RigBones.IndexOfByPredicate([AboveName](const FHobBone& Each) { return Each.Name == AboveName; });
		}
		for (int32 Side = 0; Side < 2; ++Side)
		{
			for (int32 Part = 0; Part < 4; ++Part)
			{
				if (Bone.Name == WingBoneName(Side, Part))
				{
					Bone.Wing = Side;
					Bone.Part = Part;
				}
			}
		}
	}
	BodyBone = FindRigBone(BodyName);
	NeckBone = FindRigBone(NeckName);
	HeadBone = FindRigBone(HeadName);
	TailBone = FindRigBone(TailName);
	// No two Hobs (should there ever be) step their heads in time.
	Moods.Initialize(static_cast<int32>(GetUniqueID()));
	NextStepAt = Moods.FRandRange(HoldMin, HoldMax);
	NextRuffleAt = Moods.FRandRange(RuffleMin, RuffleMax);
	return true;
}

void AHobBird::PoseRig(float DeltaSeconds)
{
	if (!bRigReady)
	{
		return;
	}
	const float Dt = FMath::Max(DeltaSeconds, 0.f);
	RigClock += Dt;
	const bool bFlying = FlightLeft > 0.f;
	const bool bTalking = SpeakerPoint && SpeakerPoint->IsTalking();

	// The head: held, then snapped to another of his poses; while he talks, the art session's sidelong tilt.
	StepClock += Dt;
	const int32 Wanted = bTalking ? TalkStep : HeadStep;
	if ((!bTalking && StepClock >= NextStepAt) || Wanted != HeadStep)
	{
		HeadFrom = HeadBone != INDEX_NONE ? RigBones[HeadBone].Own : FQuat::Identity;
		NeckFrom = NeckBone != INDEX_NONE ? RigBones[NeckBone].Own : FQuat::Identity;
		if (Wanted != HeadStep)
		{
			HeadStep = Wanted;
		}
		else
		{
			// Never the same one twice running.
			const int32 Count = static_cast<int32>(UE_ARRAY_COUNT(HeadSteps));
			HeadStep = (HeadStep + 1 + Moods.RandRange(0, Count - 2)) % Count;
		}
		StepClock = 0.f;
		NextStepAt = Moods.FRandRange(HoldMin, HoldMax);
	}
	const float Snap = FMath::SmoothStep(0.f, 1.f, StepClock / StepSeconds);
	const FHeadStep& Step = bFlying ? HeadSteps[0] : HeadSteps[HeadStep];
	if (HeadBone != INDEX_NONE)
	{
		RigBones[HeadBone].Own = FQuat::Slerp(HeadFrom, HobTurn(Step.Head[0], Step.Head[1], Step.Head[2]), Snap);
	}
	if (NeckBone != INDEX_NONE)
	{
		RigBones[NeckBone].Own = FQuat::Slerp(NeckFrom, HobTurn(Step.Neck[0], Step.Neck[1], Step.Neck[2]), Snap);
	}

	// A ruffle now and then: a quick shake through his body, his wings lifting off his sides and settling.
	RuffleClock += Dt;
	if (!bFlying && RuffleClock >= NextRuffleAt)
	{
		RuffleClock = 0.f;
		NextRuffleAt = Moods.FRandRange(RuffleMin, RuffleMax);
	}
	const float Ruffle = !bFlying && RuffleClock < RuffleSeconds ? FMath::Sin(UE_PI * RuffleClock / RuffleSeconds) : 0.f;
	if (BodyBone != INDEX_NONE)
	{
		const float Shake = RuffleShake * Ruffle * FMath::Sin(2.f * UE_PI * 11.f * RuffleClock);
		RigBones[BodyBone].Own = HobTurn(0.f, 0.f, Shake);
		RigBones[BodyBone].Shift = FVector(0.0, 0.0, BreathCm * FMath::Sin(2.f * UE_PI * BreathHz * RigClock));
	}
	if (TailBone != INDEX_NONE)
	{
		// The tail flicks with a ruffle.
		RigBones[TailBone].Own = HobTurn(0.f, -6.f * Ruffle, 0.f);
	}

	// The wings: folded, lifted a little in a ruffle, open and beating in flight, flared over the drop and folded as he lands.
	if (bFlying)
	{
		const float Alpha = 1.f - FlightLeft / FMath::Max(FlightTotal, 0.01f);
		WingOpen = 1.f - FMath::SmoothStep(0.88f, 1.f, Alpha);
		WingFlap = Alpha < 0.7f ? FlapMiddle + FlapReach * FMath::Sin(2.f * UE_PI * FlapHz * RigClock)
			: FMath::Lerp(FlapMiddle, FlareFlap, FMath::SmoothStep(0.7f, 0.85f, Alpha));
	}
	else
	{
		WingOpen = 0.12f * Ruffle;
		WingFlap = 0.f;
	}

	SolveRig();
	ApplyRig();
}

void AHobBird::SolveRig()
{
	// Where each wing's shoulder is this frame: its upper bone's head, blended open (the beat swings the wing about it).
	FVector Shoulders[2] = { FVector::ZeroVector, FVector::ZeroVector };
	for (const FHobBone& Bone : RigBones)
	{
		if (Bone.Wing != INDEX_NONE && Bone.Part == 0)
		{
			Shoulders[Bone.Wing] = Bone.Rest.GetLocation() + WingMove(Bone.Wing, 0) * WingOpen;
		}
	}
	for (FHobBone& Bone : RigBones)
	{
		const FHobBone* Parent = Bone.Parent != INDEX_NONE ? &RigBones[Bone.Parent] : nullptr;
		if (Bone.Wing != INDEX_NONE && Parent)
		{
			// Blended toward open: its head along a line, its turn from none toward the open one; then the beat about the
			// shoulder (raising the wing's tip: about +X on the right, -X on the left); then the body carries it all.
			FQuat Turn = FQuat::Slerp(FQuat::Identity, WingTurn(Bone.Wing, Bone.Part), WingOpen);
			FVector Joint = Bone.Rest.GetLocation() + WingMove(Bone.Wing, Bone.Part) * WingOpen;
			const float Side = Bone.Wing == 0 ? -1.f : 1.f;
			const FQuat Beat(FVector::ForwardVector, FMath::DegreesToRadians(Side * WingFlap));
			const FVector& Shoulder = Shoulders[Bone.Wing];
			Joint = Shoulder + Beat.RotateVector(Joint - Shoulder);
			Turn = Beat * Turn;
			// The wings hang from the body (Hob.py: body, then the beat, then the blend).
			const FHobBone& Carrier = BodyBone != INDEX_NONE ? RigBones[BodyBone] : *Parent;
			Joint = Carrier.Posed.GetLocation() + Carrier.Turned.RotateVector(Joint - Carrier.Rest.GetLocation());
			Bone.Turned = Carrier.Turned * Turn;
			Bone.Posed = FTransform(Bone.Turned * Bone.Rest.GetRotation(), Joint, Bone.Rest.GetScale3D());
			continue;
		}
		FQuat Above = FQuat::Identity;
		FVector Joint = Bone.Rest.GetLocation();
		if (Parent)
		{
			// Its joint rides on its parent: where the parent's turn carries it.
			Above = Parent->Turned;
			Joint = Parent->Posed.GetLocation() + Parent->Turned.RotateVector(Bone.Rest.GetLocation() - Parent->Rest.GetLocation());
		}
		Joint += Bone.Shift;
		Bone.Turned = Above * Bone.Own;
		Bone.Posed = FTransform(Bone.Turned * Bone.Rest.GetRotation(), Joint, Bone.Rest.GetScale3D());
	}
}

void AHobBird::ApplyRig()
{
	if (!Bird || !Bird->GetSkinnedAsset())
	{
		return;
	}
	// Parents first: each is placed against where its parent has just gone. Then drawn this frame, not the next.
	for (const FHobBone& Bone : RigBones)
	{
		Bird->SetBoneTransformByName(Bone.Name, Bone.Posed, EBoneSpaces::ComponentSpace);
	}
	Bird->RefreshBoneTransforms();
}
