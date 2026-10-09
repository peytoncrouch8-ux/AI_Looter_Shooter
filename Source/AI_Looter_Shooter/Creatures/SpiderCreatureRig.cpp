// ASpiderCreature's rig: what it reads from SK_Spider's skeleton (the bones the code moves and the legs' layout), the
// leg segments' frames, where its feet find the ground, and the ground its body covers (its footprint, which packmates
// keep clear of). SpiderCreature.cpp poses the rig every frame; SpiderCreatureLegs.cpp has the attack's and death's leg
// poses and the knee's spread.

#include "Creatures/SpiderCreature.h"
#include "AI_Looter_Shooter.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"

FTransform ASpiderCreature::SegmentFrame(const FVector& From, const FVector& To, const FVector& Pole, float Scale)
{
	return FTransform(FRotationMatrix::MakeFromXZ(To - From, Pole).ToQuat(), From, FVector(Scale));
}

ACreatureBase::FFootprint ASpiderCreature::GetFootprint() const
{
	// Spider.py: the chelicerae reach about 65 cm ahead of the thorax, the abdomen's tip 134 behind, 44 across.
	FFootprint Footprint;
	Footprint.Front = 28.f;
	Footprint.Back = -95.f;
	Footprint.Radius = 40.f;
	return Footprint;
}

void ASpiderCreature::PlantLegs()
{
	// Every foot down on its resting spot, the body to settle over them at once, no jolt left over.
	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const float Scale = GetSizeScale();
	for (FLeg& Leg : Legs)
	{
		Leg.Foot = GroundUnder(GetActorLocation() + Yaw.RotateVector(Leg.Rest * Scale));
		Leg.bStepping = false;
		Leg.bNeedsReset = false;
		Leg.StepAlpha = 1.f;
		Leg.HeldVelocity = FVector::ZeroVector;
		Leg.KneeSpread = 0.f;
		Leg.PosedFoot = Leg.Foot;
		Leg.DeathFoot = FVector::ZeroVector;
	}
	SteppingGroup = INDEX_NONE;
	bBodyInitialized = false;
	HurtKick = HurtOffset = HurtVelocity = FVector::ZeroVector;
	HurtHold = 0.f;
}

FVector ASpiderCreature::GroundUnder(const FVector& Point) const
{
	const float Scale = GetSizeScale();
	const float Search = FootSearchAbove * Scale;
	const FVector Here = GetActorLocation();
	const float GroundZ = static_cast<float>(Here.Z) - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	// Where the ground it stands on puts the ground at Point: the floor under its capsule, as a plane. A foot looks from a
	// little above that. (It used to look from a fixed height over the body's middle, which started inside slopes steeper
	// than about 26 degrees under a running spider's leading feet: they sank into the hill.)
	float Expected = GroundZ;
	const FFindFloorResult& Floor = GetCharacterMovement()->CurrentFloor;
	if (Floor.bBlockingHit && Floor.HitResult.ImpactNormal.Z > 0.5)
	{
		const FVector Normal = Floor.HitResult.ImpactNormal;
		Expected = GroundZ - static_cast<float>((Normal.X * (Point.X - Here.X) + Normal.Y * (Point.Y - Here.Y)) / Normal.Z);
	}
	const float Above = FMath::Max(Search, Expected - static_cast<float>(Point.Z) + Search);
	const float Below = FMath::Max(400.f * Scale, static_cast<float>(Point.Z) - Expected + 150.f * Scale);
	FVector Ground;
	if (FindGround(Point, Above, Below, Ground))
	{
		// A look from inside a rock finds the rock where it started: look again from just over the body.
		const bool bStartedInside = Above > Search && FMath::IsNearlyEqual(Ground.Z, Point.Z + Above, 1.0);
		if (!bStartedInside || FindGround(Point, Search, Below, Ground))
		{
			return Ground;
		}
	}
	return FVector(Point.X, Point.Y, Expected);
}

bool ASpiderCreature::SetupRig()
{
	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	if (!Model)
	{
		UE_LOG(LogLooter, Error, TEXT("%s has no model; import SK_Spider (Art/Models/Creatures/Spider.py)."), *GetName());
		return false;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	bool bComplete = true;
	auto RestPose = [&Skeleton, &bComplete](FName Bone)
	{
		const int32 Index = Skeleton.FindBoneIndex(Bone);
		bComplete &= Index != INDEX_NONE;
		return Index != INDEX_NONE ? FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Index) : FTransform::Identity;
	};

	// The body's frame at rest is level, at the thorax. The code places everything from it, like the old spider's
	// BodyRoot, so each bone is kept relative to the frame it moves with.
	const FTransform Thorax = RestPose(TEXT("body"));
	const FTransform FrameAtRest(Thorax.GetLocation());
	BodyInFrame = Thorax.GetRelativeTransform(FrameAtRest);
	RideHeight = Thorax.GetLocation().Z;

	auto MakePivot = [&RestPose, &FrameAtRest](FName Bone)
	{
		const FTransform Rest = RestPose(Bone);
		FPivotBone Pivot;
		Pivot.Bone = Bone;
		Pivot.Pivot = FrameAtRest.InverseTransformPosition(Rest.GetLocation());
		Pivot.BoneInPivot = Rest.GetRelativeTransform(FTransform(Rest.GetLocation()));
		return Pivot;
	};
	FangLeft = MakePivot(TEXT("fang_l"));
	FangRight = MakePivot(TEXT("fang_r"));
	Abdomen = MakePivot(TEXT("abdomen"));

	// The legs as the model stands: each foot resting on the ground, knees bent toward the pole.
	Legs.Reset();
	for (int32 Pair = 0; Pair < 4; ++Pair)
	{
		for (const float Side : { -1.f, 1.f })
		{
			const FString Suffix = FString::Printf(TEXT("%d_%s"), Pair, Side < 0.f ? TEXT("l") : TEXT("r"));
			FLeg Leg;
			Leg.Side = Side;
			Leg.Pair = Pair;
			// Alternating tetrapod: L0 R1 L2 R3 move together, then R0 L1 R2 L3.
			Leg.Group = (Pair + (Side > 0.f ? 1 : 0)) % 2;
			Leg.Femur = *(TEXT("femur_") + Suffix);
			Leg.Tibia = *(TEXT("tibia_") + Suffix);
			const FTransform Femur = RestPose(Leg.Femur);
			const FTransform Tibia = RestPose(Leg.Tibia);
			const FVector Hip = Femur.GetLocation();
			const FVector Knee = Tibia.GetLocation();
			const FVector Foot = RestPose(*(TEXT("foot_") + Suffix)).GetLocation();
			Leg.Hip = FrameAtRest.InverseTransformPosition(Hip);
			// The model's origin is under the capsule's center, so its resting feet are around the actor too.
			Leg.Rest = FVector(Foot.X, Foot.Y, 0.f);
			Leg.FemurLength = FVector::Dist(Hip, Knee);
			Leg.TibiaLength = FVector::Dist(Knee, Foot);
			Leg.RestStretch = FMath::Max(FVector::Dist(Hip, Foot), 1.f);
			const FVector Pole = KneePole(FVector::UpVector, (Hip - Thorax.GetLocation()).GetSafeNormal2D());
			Leg.FemurInSegment = Femur.GetRelativeTransform(SegmentFrame(Hip, Knee, Pole, 1.f));
			Leg.TibiaInSegment = Tibia.GetRelativeTransform(SegmentFrame(Knee, Foot, Pole, 1.f));
			Legs.Add(Leg);
		}
	}
	if (!bComplete)
	{
		UE_LOG(LogLooter, Error, TEXT("%s: %s lacks bones the spider moves; it stays in its resting pose."), *GetName(), *Model->GetName());
		Legs.Reset();
		return false;
	}

	// The pose's slots, in the order SpiderCreature.cpp numbers them (SpiderBones): the body, the fangs and the abdomen,
	// then each leg's femur and tibia.
	BonePose.Reset();
	for (const FName Bone : { FName(TEXT("body")), FangLeft.Bone, FangRight.Bone, Abdomen.Bone })
	{
		BonePose.Add({ Bone, FTransform::Identity });
	}
	for (const FLeg& Leg : Legs)
	{
		BonePose.Add({ Leg.Femur, FTransform::Identity });
		BonePose.Add({ Leg.Tibia, FTransform::Identity });
	}
	return true;
}
