#include "Creatures/SpiderCreature.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Slots in the pose: the bones the code moves. Each leg has two, femur then tibia. */
	namespace SpiderBones
	{
		constexpr int32 Body = 0;
		constexpr int32 FangLeft = 1;
		constexpr int32 FangRight = 2;
		constexpr int32 Abdomen = 3;
		constexpr int32 FirstLeg = 4;
	}

	/**
	 * Two-bone IK: places the knee so both segments keep their length, bending toward Pole.
	 * Unreachable targets are clamped along the hip-to-target line.
	 */
	void SolveTwoBone(const FVector& Hip, const FVector& Target, float Upper, float Lower, const FVector& Pole, FVector& OutKnee, FVector& OutFoot)
	{
		const FVector ToTarget = Target - Hip;
		float Distance = ToTarget.Size();
		const FVector Direction = Distance > KINDA_SMALL_NUMBER ? ToTarget / Distance : FVector::DownVector;
		Distance = FMath::Clamp(Distance, FMath::Abs(Upper - Lower) + 1.f, (Upper + Lower) * 0.999f);
		OutFoot = Hip + Direction * Distance;

		const float Along = (Upper * Upper - Lower * Lower + Distance * Distance) / (2.f * Distance);
		const float Height = FMath::Sqrt(FMath::Max(Upper * Upper - Along * Along, 0.f));
		FVector Bend = Pole - Direction * FVector::DotProduct(Pole, Direction);
		if (!Bend.Normalize())
		{
			Bend = FVector::UpVector;
		}
		OutKnee = Hip + Direction * Along + Bend * Height;
	}

	/** A leg segment's frame: at its root joint, X along the segment, Z toward the pole. */
	FTransform SegmentFrame(const FVector& From, const FVector& To, const FVector& Pole)
	{
		return FTransform(FRotationMatrix::MakeFromXZ(To - From, Pole).ToQuat(), From);
	}
}

ASpiderCreature::ASpiderCreature()
{
	DisplayName = FText::FromString(TEXT("Brown Spider"));
	Health->MaxHealth = 300.f;
	// The head is the critical spot; legs, thorax, abdomen, fangs and feelers take base damage.
	CriticalSpotBones = { TEXT("head") };

	WalkSpeed = 170.f;
	ChaseSpeed = 540.f;
	AttackRange = 220.f;
	AttackDamage = 12.f;
	HealthBarHeight = 120.f;

	GetCapsuleComponent()->InitCapsuleSize(62.f, 62.f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Model(TEXT("/Game/Art/Creatures/SK_Spider.SK_Spider"));
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSkeletalMeshAsset(Model.Object);
	// The model's origin is the ground under the thorax: the foot of the capsule.
	Body->SetRelativeLocation(FVector(0.f, 0.f, -62.f));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(USpiderAnimInstance::StaticClass());
	// The hit zones follow the bones even while the spider is off screen.
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	// Posed after this actor works out the frame's pose (see BeginPlay), from this frame's movement.
	Body->PrimaryComponentTick.TickGroup = TG_PostPhysics;

	// The physics asset's bodies are the hit zones. Only weapon and visibility traces see them; they never block
	// movement, loot, or anything else.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);
}

void ASpiderCreature::BeginPlay()
{
	Super::BeginPlay();

	GetMesh()->AddTickPrerequisiteActor(this);
	bRigReady = SetupRig();
	if (bRigReady)
	{
		PlantLegs();
		// Pose immediately so it never shows a frame in the resting pose.
		AnimateBody(0.f);
		AnimateLegs(0.f);
	}
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
			const FVector Pole = KneePole(FVector::UpVector, (Hip - Thorax.GetLocation()).GetSafeNormal2D());
			Leg.FemurInSegment = Femur.GetRelativeTransform(SegmentFrame(Hip, Knee, Pole));
			Leg.TibiaInSegment = Tibia.GetRelativeTransform(SegmentFrame(Knee, Foot, Pole));
			Legs.Add(Leg);
		}
	}
	if (!bComplete)
	{
		UE_LOG(LogLooter, Error, TEXT("%s: %s lacks bones the spider moves; it stays in its resting pose."), *GetName(), *Model->GetName());
		Legs.Reset();
		return false;
	}

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

// ---------------------------------------------------------------------------
// Procedural animation
// ---------------------------------------------------------------------------

void ASpiderCreature::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AnimTime += DeltaSeconds;
	if (bRigReady)
	{
		AnimateBody(DeltaSeconds);
		AnimateLegs(DeltaSeconds);
	}
}

void ASpiderCreature::SetBone(int32 Index, const FTransform& InSegment, const FTransform& SegmentToWorld)
{
	BonePose[Index].Transform = InSegment * SegmentToWorld.GetRelativeTransform(ComponentToWorld);
}

void ASpiderCreature::PosePivot(int32 Index, const FPivotBone& Pivot, const FRotator& Rotation, const FVector& Scale)
{
	SetBone(Index, Pivot.BoneInPivot, FTransform(Rotation, Pivot.Pivot, Scale) * BodyFrame);
}

FVector ASpiderCreature::GroundUnder(const FVector& Point) const
{
	// Search only a little above the body: feet find footing on bumps and steps, but never climb walls.
	FVector Ground;
	if (FindGround(Point, 60.f, 400.f, Ground))
	{
		return Ground;
	}
	const float GroundZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	return FVector(Point.X, Point.Y, GroundZ);
}

void ASpiderCreature::PlantLegs()
{
	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	for (FLeg& Leg : Legs)
	{
		Leg.Foot = GroundUnder(GetActorLocation() + Yaw.RotateVector(Leg.Rest));
		Leg.bStepping = false;
		Leg.bNeedsReset = false;
		Leg.StepAlpha = 1.f;
	}
	SteppingGroup = INDEX_NONE;
	bBodyInitialized = false;
}

void ASpiderCreature::AnimateBody(float DeltaSeconds)
{
	ComponentToWorld = GetMesh()->GetComponentTransform();
	const FVector ActorLocation = GetActorLocation();
	const float GroundZ = ActorLocation.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const ECreatureState CurrentState = GetCreatureState();
	const float Time = GetStateTime();

	// The body settles over its planted feet: their average sets the height, the differences tilt it.
	float Sum = 0.f, Front = 0.f, Back = 0.f, Left = 0.f, Right = 0.f;
	for (const FLeg& Leg : Legs)
	{
		Sum += Leg.Foot.Z;
		(Leg.Pair <= 1 ? Front : Back) += Leg.Foot.Z;
		(Leg.Side > 0.f ? Right : Left) += Leg.Foot.Z;
	}
	const float Half = FMath::Max(1.f, Legs.Num() * 0.5f);
	float TargetZ = FMath::Clamp(Sum / (Half * 2.f) + RideHeight, GroundZ + 25.f, GroundZ + RideHeight + 45.f);
	float TargetPitch = FMath::RadiansToDegrees(FMath::Atan2((Front - Back) / Half, 200.f));
	float TargetRoll = FMath::RadiansToDegrees(FMath::Atan2((Left - Right) / Half, 220.f));
	float Lunge = 0.f;
	float TargetFang = 3.f + FMath::Max(0.f, FMath::Sin(AnimTime * 3.1f)) * 4.f;

	if (CurrentState == ECreatureState::Attack)
	{
		// Rear up with fangs spread during the wind-up, then lunge and snap shut.
		const float Windup = FMath::Clamp(Time / AttackWindup, 0.f, 1.f);
		const float Strike = FMath::Clamp((Time - AttackWindup) / AttackRecovery, 0.f, 1.f);
		const float Rear = FMath::InterpEaseInOut(0.f, 1.f, Windup, 2.f) * (1.f - Strike);
		TargetPitch += 24.f * Rear - 12.f * FMath::Sin(Strike * UE_PI);
		TargetZ += 16.f * Rear;
		Lunge = -14.f * Rear + 30.f * FMath::Sin(Strike * UE_PI);
		TargetFang = Strike > 0.f ? 0.f : 30.f * Windup;
	}
	else if (CurrentState == ECreatureState::Dead)
	{
		// Collapse, then sink out of sight after the corpse time.
		const float Collapse = FMath::Clamp(Time / 0.6f, 0.f, 1.f);
		TargetZ = FMath::Lerp(TargetZ, GroundZ + 20.f, Collapse);
		TargetPitch = FMath::Lerp(TargetPitch, -5.f, Collapse);
		TargetRoll += 10.f * Collapse;
		TargetFang = 25.f;
		if (Time > CorpseTime)
		{
			TargetZ -= (Time - CorpseTime) * 45.f;
		}
	}
	else
	{
		TargetZ += FMath::Sin(AnimTime * 2.2f) * 1.5f; // breathing
	}

	if (!bBodyInitialized || DeltaSeconds <= 0.f)
	{
		BodyZ = TargetZ;
		BodyPitch = TargetPitch;
		BodyRoll = TargetRoll;
		FangOpen = TargetFang;
		bBodyInitialized = true;
	}
	BodyZ = FMath::FInterpTo(BodyZ, TargetZ, DeltaSeconds, CurrentState == ECreatureState::Dead ? 6.f : 10.f);
	BodyPitch = FMath::FInterpTo(BodyPitch, TargetPitch, DeltaSeconds, 8.f);
	BodyRoll = FMath::FInterpTo(BodyRoll, TargetRoll, DeltaSeconds, 8.f);
	FangOpen = FMath::FInterpTo(FangOpen, TargetFang, DeltaSeconds, 14.f);
	HurtOffset = FMath::VInterpTo(HurtOffset, FVector::ZeroVector, DeltaSeconds, 10.f);
	AbdomenKick = FMath::FInterpTo(AbdomenKick, 0.f, DeltaSeconds, 8.f);

	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const FVector Location = FVector(ActorLocation.X, ActorLocation.Y, BodyZ) + Yaw.RotateVector(FVector(Lunge, 0.f, 0.f)) + HurtOffset;
	BodyFrame = FTransform(Yaw * FRotator(BodyPitch, 0.f, BodyRoll).Quaternion(), Location);
	SetBone(SpiderBones::Body, BodyInFrame, BodyFrame);

	// Abdomen: slow breathing sway plus a jolt when hit.
	const float Breath = FMath::Sin(AnimTime * 1.7f);
	PosePivot(SpiderBones::Abdomen, Abdomen, FRotator(Breath * 2.5f + AbdomenKick, FMath::Sin(AnimTime * 0.9f) * 2.f, 0.f),
		FVector(1.f, 1.f + Breath * 0.015f, 1.f + Breath * 0.02f));

	PosePivot(SpiderBones::FangLeft, FangLeft, FRotator(FangOpen * 0.4f, -FangOpen, 0.f), FVector::OneVector);
	PosePivot(SpiderBones::FangRight, FangRight, FRotator(FangOpen * 0.4f, FangOpen, 0.f), FVector::OneVector);
}

void ASpiderCreature::AnimateLegs(float DeltaSeconds)
{
	const FTransform& Body = BodyFrame;
	const FVector BodyCenter = Body.GetLocation();
	const FVector Up = Body.GetRotation().GetUpVector();
	const FVector ActorLocation = GetActorLocation();
	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const FVector Velocity(GetVelocity().X, GetVelocity().Y, 0.f);
	const float Speed = Velocity.Size();
	const ECreatureState CurrentState = GetCreatureState();
	const float Time = GetStateTime();

	// Faster = quicker, longer, higher steps.
	const float StepDuration = FMath::Clamp(0.26f - Speed * 0.00018f, 0.12f, 0.26f);
	const float StepThreshold = 32.f + Speed * 0.07f;
	const float StepHeight = 18.f + Speed * 0.025f;

	// Feet mid-step carry on.
	for (FLeg& Leg : Legs)
	{
		if (Leg.bStepping)
		{
			Leg.StepAlpha = FMath::Min(1.f, Leg.StepAlpha + DeltaSeconds / StepDuration);
			const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Leg.StepAlpha, 2.f);
			Leg.Foot = FMath::Lerp(Leg.StepFrom, Leg.StepTo, Eased) + FVector(0.f, 0.f, FMath::Sin(Leg.StepAlpha * UE_PI) * StepHeight);
			if (Leg.StepAlpha >= 1.f)
			{
				Leg.Foot = Leg.StepTo;
				Leg.bStepping = false;
				Leg.LastStepTime = AnimTime;
			}
		}
	}

	// Legs the attack or death animation holds don't walk.
	auto IsHeld = [CurrentState](const FLeg& Leg)
	{
		return CurrentState == ECreatureState::Dead || (CurrentState == ECreatureState::Attack && Leg.Pair == 0);
	};

	// Where each foot wants to be: its rest spot, led by the velocity so it lands ahead of the body.
	TArray<FVector, TInlineAllocator<8>> RestSpots;
	bool bGroupWants[2] = { false, false };
	for (FLeg& Leg : Legs)
	{
		const FVector Rest = RestSpots.Add_GetRef(ActorLocation + Yaw.RotateVector(Leg.Rest) + Velocity * (StepDuration * 0.9f));
		if (IsHeld(Leg) || Leg.bStepping)
		{
			continue;
		}
		const float Offset = FVector::Dist2D(Leg.Foot, Rest);
		if (Offset > 450.f)
		{
			Leg.Foot = GroundUnder(Rest); // teleported (respawn)
			continue;
		}
		const bool bSettle = Speed < 10.f && Offset > 12.f && AnimTime - Leg.LastStepTime > 0.6f;
		bGroupWants[Leg.Group] |= Offset > StepThreshold || bSettle || Leg.bNeedsReset;
	}

	// Tetrapod gait: the two groups of alternate legs take turns. Once one group has all its feet down, the other lifts
	// every foot that is off its spot, together. (Letting each leg go whenever the other group was down let one group
	// step on and on while the other's feet dragged behind.)
	if (SteppingGroup != INDEX_NONE && !Legs.ContainsByPredicate([](const FLeg& Leg) { return Leg.bStepping; }))
	{
		LastGroup = SteppingGroup;
		SteppingGroup = INDEX_NONE;
	}
	if (SteppingGroup == INDEX_NONE && (bGroupWants[0] || bGroupWants[1]))
	{
		SteppingGroup = bGroupWants[0] && bGroupWants[1] ? 1 - LastGroup : (bGroupWants[0] ? 0 : 1);
		for (int32 Index = 0; Index < Legs.Num(); ++Index)
		{
			FLeg& Leg = Legs[Index];
			if (Leg.Group == SteppingGroup && !IsHeld(Leg) && (Leg.bNeedsReset || FVector::Dist2D(Leg.Foot, RestSpots[Index]) > 8.f))
			{
				Leg.StepFrom = Leg.Foot;
				Leg.StepTo = GroundUnder(RestSpots[Index]);
				Leg.StepAlpha = 0.f;
				Leg.bStepping = true;
				Leg.bNeedsReset = false;
			}
		}
	}

	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		FLeg& Leg = Legs[Index];
		const FVector Hip = Body.TransformPosition(Leg.Hip);
		const FVector Outward = (Hip - BodyCenter).GetSafeNormal2D();
		FVector Pole = KneePole(Up, Outward); // high, arched knees

		if (CurrentState == ECreatureState::Dead)
		{
			// Legs curl in under the body with the knees folded high.
			const FVector Curled = Body.TransformPosition(FVector(Leg.Hip.X * 0.6f + 10.f, Leg.Hip.Y * 1.6f, -8.f));
			Leg.Foot = FMath::VInterpTo(Leg.Foot, Curled, DeltaSeconds, 6.f);
			Leg.bStepping = false;
			Pole = Up * 1.5f + Outward * 0.2f;
		}
		else if (CurrentState == ECreatureState::Attack && Leg.Pair == 0)
		{
			// Front legs rise with the body, then slam down ahead of it on the strike.
			const float Strike = FMath::Clamp((Time - AttackWindup) / (AttackRecovery * 0.5f), 0.f, 1.f);
			const FVector Raised = Body.TransformPosition(FVector(Leg.Hip.X + 80.f, Leg.Hip.Y * 1.5f, 50.f));
			const FVector Slam = GroundUnder(ActorLocation + Yaw.RotateVector(FVector(150.f, Leg.Side * 50.f, 0.f)));
			Leg.Foot = DeltaSeconds > 0.f ? FMath::VInterpTo(Leg.Foot, FMath::Lerp(Raised, Slam, Strike), DeltaSeconds, 16.f) : Leg.Foot;
			Leg.bStepping = false;
			Leg.bNeedsReset = true;
		}

		FVector Knee;
		FVector Foot;
		SolveTwoBone(Hip, Leg.Foot, Leg.FemurLength, Leg.TibiaLength, Pole, Knee, Foot);
		SetBone(SpiderBones::FirstLeg + Index * 2, Leg.FemurInSegment, SegmentFrame(Hip, Knee, Pole));
		SetBone(SpiderBones::FirstLeg + Index * 2 + 1, Leg.TibiaInSegment, SegmentFrame(Knee, Foot, Pole));
	}
}

// ---------------------------------------------------------------------------
// Reactions
// ---------------------------------------------------------------------------

void ASpiderCreature::OnAttackStarted()
{
	AbdomenKick = -6.f; // abdomen tips down as the front rears up
}

void ASpiderCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	// Flinch away from the hit; headshots rock it harder.
	HurtOffset = (BodyFrame.GetLocation() - HitLocation).GetSafeNormal() * (bCritical ? 14.f : 7.f);
	AbdomenKick = bCritical ? 10.f : 6.f;
}

void ASpiderCreature::OnDied()
{
	for (FLeg& Leg : Legs)
	{
		Leg.bStepping = false;
	}
	SteppingGroup = INDEX_NONE;
}

void ASpiderCreature::OnRespawned()
{
	if (bRigReady)
	{
		PlantLegs();
		AnimateBody(0.f);
		AnimateLegs(0.f);
	}
}

void ASpiderCreature::SetHitVolumesEnabled(bool bEnabled)
{
	GetMesh()->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}
