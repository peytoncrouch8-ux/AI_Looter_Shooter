#include "Creatures/SpiderCreature.h"
#include "Creatures/CreaturePoseAnimInstance.h"
#include "Combat/HealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Player/PlayerSize.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/**
	 * Slots in the pose: the bones the code moves. Each leg has two, femur then tibia. SetupRig (SpiderCreatureRig.cpp)
	 * lays the pose out in this order.
	 */
	namespace SpiderBones
	{
		constexpr int32 Body = 0;
		constexpr int32 FangLeft = 1;
		constexpr int32 FangRight = 2;
		constexpr int32 Abdomen = 3;
		constexpr int32 FirstLeg = 4;
	}

	// The gait at full size. A spider of another size takes the same gait scaled in space and in time (multiply both by its
	// size): at the same speed, a big one strides long and slow, a small one scurries.

	/** How far (cm) a foot may get from its spot before its group steps it back: farther at speed (longer strides). */
	float StepThresholdAt(float Speed)
	{
		return 32.f + Speed * 0.07f;
	}

	/** Seconds one step takes: quicker at speed. */
	float StepDurationAt(float Speed)
	{
		return FMath::Clamp(0.26f - Speed * 0.00018f, 0.12f, 0.26f);
	}

	/** A trailing foot this far into its leg's reach steps with its group's next turn, and this far at once (past full reach it slides). */
	constexpr float OverreachShare = 0.92f;
	constexpr float SevereOverreachShare = 0.98f;

	/**
	 * A hit's jolt: its kick holds HurtKickHold seconds, then fades at HurtKickFade, and the body follows on a critically
	 * damped spring (HurtOmega, steps of SpringStep): one hit peaks about 90 ms on at 0.84 of its kick, under a quarter of it
	 * in any frame; a kick renewed every frame (the Gravemother sinking as she reels) holds the body at it, as the old
	 * flinch held it at 0.83 of its own.
	 */
	constexpr float HurtOmega = 40.f;
	constexpr float HurtKickHold = 0.06f;
	constexpr float HurtKickFade = 10.f;
	constexpr float HurtKickShare = 0.85f;
	constexpr float SpringStep = 1.f / 120.f;

	/** How quickly a knee's spread follows its foot (1/s): well inside a step, smooth across the bite's hand-back. */
	constexpr float KneeSpreadRate = 15.f;
}

ASpiderCreature::ASpiderCreature()
{
	DisplayName = FText::FromString(TEXT("Brown Spider"));
	Health->MaxHealth = 300.f;
	// The head is the critical spot; legs, thorax, abdomen, fangs and feelers take base damage.
	CriticalSpotBones = { TEXT("head") };

	WalkSpeed = 170.f;
	// Tuned against the full-size player's 600 walk (90% of it, so a walking player just outpaces one) and scaled with the
	// player's speed: at 540 against the smaller player's 510 a spider caught anyone walking (the user's call, 2026-10-08).
	ChaseSpeed = 540.f * LooterPlayerSize::SpeedScale;
	AttackRange = 220.f;
	AttackDamage = 12.f;
	HealthBarHeight = 120.f;
	// Every spider answers every other's call (spiderlings, the Gravemother), whatever its class.
	PackTag = TEXT("Spider");

	GetCapsuleComponent()->InitCapsuleSize(62.f, 62.f);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Model(TEXT("/Game/Art/Creatures/SK_Spider.SK_Spider"));
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSkeletalMeshAsset(Model.Object);
	// The model's origin is the ground under the thorax: the foot of the capsule.
	Body->SetRelativeLocation(FVector(0.f, 0.f, -62.f));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(UCreaturePoseAnimInstance::StaticClass());
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

// ---------------------------------------------------------------------------
// Procedural animation
// ---------------------------------------------------------------------------

void ASpiderCreature::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AnimTime += DeltaSeconds;
	// Far away out of view it walks on unposed (OnPoseThawed puts its feet back under it).
	if (bRigReady && !IsPoseFrozen())
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

void ASpiderCreature::AnimateBody(float DeltaSeconds)
{
	ComponentToWorld = GetMesh()->GetComponentTransform();
	const FVector ActorLocation = GetActorLocation();
	const float GroundZ = ActorLocation.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const ECreatureState CurrentState = GetCreatureState();
	const float Time = GetStateTime();
	// Every distance below is the full-size spider's, times its size.
	const float Scale = GetSizeScale();
	const float Ride = RideHeight * Scale;

	// The body settles over its planted feet: their average sets the height, the differences tilt it.
	float Sum = 0.f, Front = 0.f, Back = 0.f, Left = 0.f, Right = 0.f;
	for (const FLeg& Leg : Legs)
	{
		Sum += Leg.Foot.Z;
		(Leg.Pair <= 1 ? Front : Back) += Leg.Foot.Z;
		(Leg.Side > 0.f ? Right : Left) += Leg.Foot.Z;
	}
	const float Half = FMath::Max(1.f, Legs.Num() * 0.5f);
	float TargetZ = FMath::Clamp(Sum / (Half * 2.f) + Ride, GroundZ + 25.f * Scale, GroundZ + Ride + 45.f * Scale);
	float TargetPitch = FMath::RadiansToDegrees(FMath::Atan2((Front - Back) / Half, 200.f * Scale));
	float TargetRoll = FMath::RadiansToDegrees(FMath::Atan2((Left - Right) / Half, 220.f * Scale));
	float Lunge = 0.f;
	float TargetFang = 3.f + FMath::Max(0.f, FMath::Sin(AnimTime * 3.1f)) * 4.f;
	// The ground's tilt as its feet give it, kept for its death: dead, its feet fold in with the body and say nothing of the
	// ground (and a raised front pair would tip the body back).
	if (CurrentState != ECreatureState::Dead && CurrentState != ECreatureState::Attack)
	{
		GroundPitch = TargetPitch;
		GroundRoll = TargetRoll;
	}

	if (CurrentState == ECreatureState::Attack)
	{
		// Rear up with fangs spread during the wind-up, then lunge and snap shut.
		const float Windup = FMath::Clamp(Time / AttackWindup, 0.f, 1.f);
		const float Strike = FMath::Clamp((Time - AttackWindup) / AttackRecovery, 0.f, 1.f);
		const float Rear = FMath::InterpEaseInOut(0.f, 1.f, Windup, 2.f) * (1.f - Strike);
		TargetPitch += 24.f * Rear - 12.f * FMath::Sin(Strike * UE_PI);
		TargetZ += 16.f * Scale * Rear;
		Lunge = (-14.f * Rear + 30.f * FMath::Sin(Strike * UE_PI)) * Scale;
		TargetFang = Strike > 0.f ? 0.f : 30.f * Windup;
	}
	else if (CurrentState == ECreatureState::Dead)
	{
		// Collapse onto its folding legs, then sink out of sight after the corpse time (a big body sinks as much faster as it
		// is bigger, so it's gone by the time it's hidden). It keeps the ground's slope, slumped a little nose down and over.
		const float Collapse = FMath::Clamp(Time / CurlSeconds, 0.f, 1.f);
		TargetZ = FMath::Lerp(TargetZ, GroundZ + DeadRide * Scale, Collapse);
		TargetPitch = GroundPitch - 5.f * Collapse;
		TargetRoll = GroundRoll + 10.f * Collapse;
		TargetFang = 25.f;
		if (Time > CorpseTime)
		{
			TargetZ -= (Time - CorpseTime) * 45.f * Scale;
		}
	}
	else
	{
		TargetZ += FMath::Sin(AnimTime * 2.2f) * 1.5f * Scale; // breathing
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
	// The hit's jolt follows its fading kick on a stiff spring: no one-frame jump.
	const float SpringSeconds = FMath::Min(DeltaSeconds, 0.5f);
	if (SpringSeconds > 0.f)
	{
		const int32 Steps = FMath::Max(1, FMath::CeilToInt32(SpringSeconds / SpringStep - 0.01f));
		const float Step = SpringSeconds / Steps;
		for (int32 Index = 0; Index < Steps; ++Index)
		{
			HurtHold -= Step;
			HurtKick -= HurtHold > 0.f ? FVector::ZeroVector : HurtKick * FMath::Min(HurtKickFade * Step, 1.f);
			HurtVelocity += (HurtOmega * HurtOmega * (HurtKick - HurtOffset) - 2.f * HurtOmega * HurtVelocity) * Step;
			HurtOffset += HurtVelocity * Step;
		}
	}
	AbdomenKick = FMath::FInterpTo(AbdomenKick, 0.f, DeltaSeconds, 8.f);

	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const FVector Location = FVector(ActorLocation.X, ActorLocation.Y, BodyZ) + Yaw.RotateVector(FVector(Lunge, 0.f, 0.f)) + HurtOffset;
	// The frame carries the size like the mesh does: hips, pivots and poses set in it scale with the spider.
	BodyFrame = FTransform(Yaw * FRotator(BodyPitch, 0.f, BodyRoll).Quaternion(), Location, FVector(Scale));
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
	const float Scale = GetSizeScale();

	// Faster = quicker, longer, higher steps. At another size the full-size gait is scaled in space and time alike.
	const float StepDuration = StepDurationAt(Speed) * Scale;
	const float StepThreshold = StepThresholdAt(Speed) * Scale;
	const float StepHeight = (18.f + Speed * 0.025f) * Scale;
	// A distant spider updates only a few times a second. When an update is over half a step long, a step lasts just one
	// update: rounded up to two, each step would take twice as long while the body walks on, and the feet would trail.
	const float StepTime = DeltaSeconds > StepDuration * 0.5f ? DeltaSeconds : StepDuration;

	// Feet mid-step carry on.
	for (FLeg& Leg : Legs)
	{
		if (Leg.bStepping)
		{
			Leg.StepAlpha = FMath::Min(1.f, Leg.StepAlpha + DeltaSeconds / StepTime);
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
		const FVector Rest = RestSpots.Add_GetRef(ActorLocation + Yaw.RotateVector(Leg.Rest * Scale) + Velocity * (StepDuration * 0.9f));
		if (IsHeld(Leg) || Leg.bStepping)
		{
			continue;
		}
		const float Offset = FVector::Dist2D(Leg.Foot, Rest);
		if (Offset > 450.f * Scale)
		{
			Leg.Foot = GroundUnder(Rest); // teleported (respawn)
			continue;
		}
		const bool bSettle = Speed < 10.f && Offset > 12.f * Scale && AnimTime - Leg.LastStepTime > 0.6f;
		// A foot left trailing near the end of its leg's reach (its group waited on the other's step while the body ran on)
		// steps with its group's next turn, and at the very end steps at once: past full reach the IK would pull it off its
		// spot, sliding.
		const FVector Hip = Body.TransformPosition(Leg.Hip);
		const float Reach = (Leg.FemurLength + Leg.TibiaLength) * Scale;
		const float Stretch = static_cast<float>(FVector::Dist(Hip, Leg.Foot));
		const bool bTrailing = FVector::DotProduct(Leg.Foot - Hip, Velocity) < 0.0;
		// Standing, a foot the body has pulled toward the end of its reach (rearing for a bite, settling on a slope) steps
		// back under it the same way: no velocity says it trails, and it would only slide. One on its spot already stays.
		const bool bPulled = bTrailing || (Speed < 10.f && Offset > 8.f * Scale);
		if (bPulled && Stretch > SevereOverreachShare * Reach)
		{
			Leg.StepFrom = Leg.Foot;
			Leg.StepTo = GroundUnder(Rest);
			Leg.StepAlpha = 0.f;
			Leg.bStepping = true;
			Leg.bNeedsReset = false;
			continue;
		}
		Leg.bNeedsReset |= bPulled && Stretch > OverreachShare * Reach;
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
			if (Leg.Group == SteppingGroup && !IsHeld(Leg) && !Leg.bStepping
				&& (Leg.bNeedsReset || FVector::Dist2D(Leg.Foot, RestSpots[Index]) > 8.f * Scale))
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
		FVector Knee;
		FVector Foot;

		if (CurrentState == ECreatureState::Dead)
		{
			// The legs fold in from where they were, knees up and out, tips under the body (SpiderCreatureLegs.cpp).
			CurlDeadLeg(Leg, Time, Knee, Foot, Pole);
			Leg.Foot = Foot;
			Leg.bStepping = false;
		}
		else
		{
			if (CurrentState == ECreatureState::Attack && Leg.Pair == 0)
			{
				// Front legs rise with the body, then slam down ahead of it on the strike (the body's frame carries the size).
				// A spring carries them, so they lift off from rest instead of leaping up at full speed, and the slam's
				// stiffer spring keeps it quick.
				const float Strike = FMath::Clamp((Time - AttackWindup) / (AttackRecovery * 0.5f), 0.f, 1.f);
				const FVector Raised = Body.TransformPosition(FVector(Leg.Hip.X + 80.f, Leg.Hip.Y * 1.5f, 50.f));
				// The ground is looked for only once the strike starts: until then the foot is all Raised, and a probe per front
				// leg per frame through the whole windup bought nothing.
				const FVector Slam = Strike > 0.f
					? GroundUnder(ActorLocation + Yaw.RotateVector(FVector(150.f, Leg.Side * 50.f, 0.f) * Scale)) : Raised;
				StepHeldFoot(Leg, FMath::Lerp(Raised, Slam, Strike), Strike > 0.f ? 32.f : 14.f, DeltaSeconds);
				Leg.bStepping = false;
				Leg.bNeedsReset = true;
			}
			else
			{
				Leg.HeldVelocity = FVector::ZeroVector;
			}
			// A walking knee spreads out as its foot comes in toward the hip; a held one arches as the bite has it. The spread
			// eases, so a leg the bite hands back to the gait doesn't swing its knee over in a frame.
			const float WantedSpread = IsHeld(Leg) ? 0.f : WantedKneeSpread(Leg, static_cast<float>(FVector::Dist(Hip, Leg.Foot)) / Scale);
			Leg.KneeSpread = DeltaSeconds > 0.f ? FMath::FInterpTo(Leg.KneeSpread, WantedSpread, DeltaSeconds, KneeSpreadRate) : WantedSpread;
			Pole = KneePole(Up, Outward) + Outward * Leg.KneeSpread;
			SolveTwoBone(Hip, Leg.Foot, Leg.FemurLength * Scale, Leg.TibiaLength * Scale, Pole, Knee, Foot);
		}
		Leg.PosedFoot = Foot;
		SetBone(SpiderBones::FirstLeg + Index * 2, Leg.FemurInSegment, SegmentFrame(Hip, Knee, Pole, Scale));
		SetBone(SpiderBones::FirstLeg + Index * 2 + 1, Leg.TibiaInSegment, SegmentFrame(Knee, Foot, Pole, Scale));
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
	// Flinch away from the hit; headshots rock it harder. A shotgun's pellets land together: the strongest kick stands, they
	// don't add up.
	const float Peak = (bCritical ? 14.f : 7.f) * GetSizeScale();
	const FVector Kick = (BodyFrame.GetLocation() - HitLocation).GetSafeNormal() * (Peak * HurtKickShare);
	if (Kick.SizeSquared() >= HurtKick.SizeSquared())
	{
		HurtKick = Kick;
		HurtHold = HurtKickHold;
	}
	AbdomenKick = bCritical ? 10.f : 6.f;
}

void ASpiderCreature::OnDied()
{
	for (FLeg& Leg : Legs)
	{
		Leg.bStepping = false;
	}
	SteppingGroup = INDEX_NONE;
	// The curl starts from where each foot is now.
	RecordDeathPose();
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

void ASpiderCreature::OnPoseThawed()
{
	if (!bRigReady)
	{
		return;
	}
	// Its feet stayed where they last stood while it walked on unposed. This usually happens just before a view reaches
	// it, but a quick turn can show it, so only what's out of place moves: a step half taken lands, feet it walked away
	// from are put down under it afresh, and feet still near their spots stay down for the gait to step in as it walks.
	const FVector ActorLocation = GetActorLocation();
	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	const float Scale = GetSizeScale();
	const float StepThreshold = StepThresholdAt(static_cast<float>(GetVelocity().Size2D())) * Scale;
	for (FLeg& Leg : Legs)
	{
		if (Leg.bStepping)
		{
			Leg.Foot = Leg.StepTo;
			Leg.bStepping = false;
			Leg.StepAlpha = 1.f;
		}
		const FVector Rest = ActorLocation + Yaw.RotateVector(Leg.Rest * Scale);
		if (Leg.bNeedsReset || FVector::Dist2D(Leg.Foot, Rest) > StepThreshold)
		{
			Leg.Foot = GroundUnder(Rest);
			Leg.bNeedsReset = false;
		}
	}
	SteppingGroup = INDEX_NONE;
	// The body settles over its feet at once: it may have walked up or down a slope since it was last posed.
	bBodyInitialized = false;
	AnimateBody(0.f);
	AnimateLegs(0.f);
}

void ASpiderCreature::OnSizeChanged()
{
	// Promoted (or resized) in play: its feet go down where the new size puts them, and the body settles over them at once.
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
