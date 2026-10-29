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
	// size): at the same speed, a big one strides long and slow, a small one scurries. (How far a foot strays before it
	// steps is ASpiderCreature::StepThresholdAt, which its thaw uses too.)

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
	 * flinch held it at 0.83 of its own. (The kick's hold and share are OnHurt's: SpiderCreatureReact.cpp.) A hit's tilt
	 * (pitch and roll away from the shot) rides the same spring.
	 */
	constexpr float HurtOmega = 40.f;
	constexpr float HurtKickFade = 10.f;
	constexpr float SpringStep = 1.f / 120.f;

	/** How quickly a knee's spread follows its foot (1/s): well inside a step, smooth across the bite's hand-back. */
	constexpr float KneeSpreadRate = 15.f;
}

float ASpiderCreature::StepThresholdAt(float Speed)
{
	// How far (cm, full size) a foot may get from its spot before its group steps it back: farther at speed (longer strides).
	return 32.f + Speed * 0.07f;
}

ASpiderCreature::ASpiderCreature()
{
	DisplayName = FText::FromString(TEXT("Brown Spider"));
	// 230, was 300; its bite 11, was 12 (the spitter's spit stays softer at 10) (the user, 2026-10-22: "soften the spiders"): six at level 2 killed a player with a
	// Common rifle and ward at any damage scale (three biting at once, 324 health each). The Gravemother keeps 300 and 12.
	Health->MaxHealth = 230.f;
	// The head is the critical spot; legs, thorax, abdomen, fangs and feelers take base damage.
	CriticalSpotBones = { TEXT("head") };

	WalkSpeed = 170.f;
	// Tuned against the full-size player's 600 walk (90% of it, so a walking player just outpaces one) and scaled with the
	// player's speed, size and pace: at 540 against the smaller player's 510 a spider caught anyone walking (the user's
	// call, 2026-10-08), and the slower pace (2026-10-13) keeps the same odds.
	ChaseSpeed = 540.f * LooterPlayerSize::MoveScale;
	AttackRange = 220.f;
	AttackDamage = 11.f;
	HealthBarHeight = 120.f;
	// Its body lies curled about ten seconds (a twitch or two), then sinks away (the polish's "deaths with weight").
	CorpseTime = 10.f;
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
	// Its idle life, its entrance and its death's last twitches move on (SpiderCreatureReact.cpp).
	TickReactions(DeltaSeconds);
	// Its hit feel's clocks: buckles, the stagger's cry, its wounds (SpiderCreatureHurt.cpp).
	TickHitFeel(DeltaSeconds);
	// Far away out of view it walks on unposed (OnPoseThawed puts its feet back under it); a corpse lying still keeps its pose.
	if (bRigReady && !IsPoseFrozen() && !IsCorpseSettled())
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
	// A subclass's own pose (the Gravemother's dig, lift and plough), alive: on the spider's, or in place of its attack's.
	PoseOverride = FSpiderPoseOverride();
	bPoseOverride = CurrentState != ECreatureState::Dead && GetPoseOverride(PoseOverride);
	const float OwnAttack = bPoseOverride ? FMath::Clamp(PoseOverride.OwnShare, 0.f, 1.f) : 1.f;

	if (CurrentState == ECreatureState::Attack)
	{
		// Rear up with fangs spread during the wind-up, then lunge and snap shut.
		const float Windup = FMath::Clamp(Time / AttackWindup, 0.f, 1.f);
		const float Strike = FMath::Clamp((Time - AttackWindup) / AttackRecovery, 0.f, 1.f);
		const float Rear = FMath::InterpEaseInOut(0.f, 1.f, Windup, 2.f) * (1.f - Strike);
		TargetPitch += (24.f * Rear - 12.f * FMath::Sin(Strike * UE_PI)) * OwnAttack;
		TargetZ += 16.f * Scale * Rear * OwnAttack;
		Lunge = (-14.f * Rear + 30.f * FMath::Sin(Strike * UE_PI)) * Scale * OwnAttack;
		TargetFang = Strike > 0.f ? 0.f : 30.f * Windup;
	}
	else if (CurrentState == ECreatureState::Dead)
	{
		// Its throe, the drop onto its folding legs and the bounce, lying still on the ground's slope, its last shudder and
		// the sink (SpiderCreatureDeath.cpp). A lunge it died in eases off over the rear.
		DeadBody(Time, GroundZ, TargetZ, TargetPitch, TargetRoll, TargetFang);
		Lunge = DeathLunge * (1.f - FMath::SmoothStep(0.f, 0.25f * DeathTimeScale(Scale), Time));
	}
	else
	{
		TargetZ += FMath::Sin(AnimTime * 2.2f) * 1.5f * Scale * WoundBreath(); // breathing, heavier badly hurt
		// Its notice beat (reared, fangs flared) and idle life (grooming), SpiderCreatureReact.cpp.
		ReactBody(TargetZ, TargetPitch, TargetFang);
	}
	// Wounded, it leans onto the leg it limps on (SpiderCreatureLegs.cpp); not the ground's tilt its death keeps.
	TargetRoll += LimpRoll();
	if (bPoseOverride)
	{
		TargetZ += PoseOverride.Rise * Scale;
		TargetPitch += PoseOverride.Pitch;
		Lunge += PoseOverride.Lunge * Scale;
		if (PoseOverride.Fang.IsSet())
		{
			TargetFang = PoseOverride.Fang.GetValue();
		}
	}

	if (!bBodyInitialized || DeltaSeconds <= 0.f)
	{
		BodyZ = TargetZ;
		BodyPitch = TargetPitch;
		BodyRoll = TargetRoll;
		FangOpen = TargetFang;
		bBodyInitialized = true;
	}
	if (CurrentState == ECreatureState::Dead)
	{
		// Dead, the body follows its death's own curves, which start where it was as it died: nothing to smooth, and the drop
		// lands on its moment rather than easing in late.
		BodyZ = TargetZ;
		BodyPitch = TargetPitch;
		BodyRoll = TargetRoll;
	}
	else
	{
		BodyZ = FMath::FInterpTo(BodyZ, TargetZ, DeltaSeconds, 10.f);
		BodyPitch = FMath::FInterpTo(BodyPitch, TargetPitch, DeltaSeconds, 8.f);
		BodyRoll = FMath::FInterpTo(BodyRoll, TargetRoll, DeltaSeconds, 8.f);
		LungeNow = Lunge;
	}
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
			HurtTiltKick -= HurtHold > 0.f ? FVector::ZeroVector : HurtTiltKick * FMath::Min(HurtKickFade * Step, 1.f);
			HurtTiltVelocity += (HurtOmega * HurtOmega * (HurtTiltKick - HurtTilt) - 2.f * HurtOmega * HurtTiltVelocity) * Step;
			HurtTilt += HurtTiltVelocity * Step;
			// The hit feel's twist, a shot leg's roll and dip (SpiderCreatureHurt.cpp), on a spring as stiff.
			if (!HitTwist.IsSettled())
			{
				HitTwist.Step(Step, HurtOmega, HurtKickFade);
			}
		}
	}
	AbdomenKick = FMath::FInterpTo(AbdomenKick, 0.f, DeltaSeconds, 8.f);
	// The hit feel on top (SpiderCreatureHurt.cpp): the twist, a shot leg's roll and dip, the stagger's crouch, fangs, abdomen.
	// Laid on after the body's smoothing: each is its own curve already, and smoothing would only blunt a stagger's drop.
	const FHitFeelPose Feel = HitFeelPose();

	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	// Climbing out of the dirt as it comes into the world, the body starts under the ground (EmergeDrop).
	const FVector Location = FVector(ActorLocation.X, ActorLocation.Y, BodyZ + EmergeDrop() - Feel.Drop)
		+ Yaw.RotateVector(FVector(Lunge, 0.f, 0.f)) + HurtOffset;
	// The frame carries the size like the mesh does: hips, pivots and poses set in it scale with the spider. A hit tilts it
	// away from the shot (HurtTilt: pitch, roll) and twists it (Feel.Yaw).
	const FQuat Twisted = FRotator(0.f, GetActorRotation().Yaw + Feel.Yaw, 0.f).Quaternion();
	BodyFrame = FTransform(Twisted * FRotator(BodyPitch + HurtTilt.X + Feel.Pitch, 0.f, BodyRoll + HurtTilt.Y + Feel.Roll).Quaternion(),
		Location, FVector(Scale));
	SetBone(SpiderBones::Body, BodyInFrame, BodyFrame);

	// Abdomen: slow breathing sway plus a jolt when hit. Dead, the breath goes out of it, and the throe lifts it and the drop
	// slams it down (SpiderCreatureDeath.cpp).
	const bool bDead = CurrentState == ECreatureState::Dead;
	const float Breathing = bDead ? DeathBreath(Time) : 1.f;
	const float Breath = FMath::Sin(AnimTime * 1.7f) * Breathing;
	const float DeadTilt = bDead ? DeadAbdomenTilt(Time) : 0.f;
	PosePivot(SpiderBones::Abdomen, Abdomen, FRotator(Breath * 2.5f + AbdomenKick + Feel.Abdomen + DeadTilt, FMath::Sin(AnimTime * 0.9f) * 2.f * Breathing, 0.f),
		FVector(1.f, 1.f + Breath * 0.015f, 1.f + Breath * 0.02f));

	const float Fang = FMath::Max(FangOpen, Feel.FangAtLeast) + Feel.FangAdd;
	PosePivot(SpiderBones::FangLeft, FangLeft, FRotator(Fang * 0.4f, -Fang, 0.f), FVector::OneVector);
	PosePivot(SpiderBones::FangRight, FangRight, FRotator(Fang * 0.4f, Fang, 0.f), FVector::OneVector);
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

	// A Big hit's skids slide their planted feet on (SpiderCreatureLegs.cpp).
	SlideSkids(DeltaSeconds);
	// Feet mid-step carry on; a limping leg's (badly hurt: SpiderCreatureHurt.cpp) lower and slower.
	for (FLeg& Leg : Legs)
	{
		if (Leg.bStepping)
		{
			Leg.StepAlpha = FMath::Min(1.f, Leg.StepAlpha + DeltaSeconds / (StepTime * LimpTimeScale(Leg)));
			const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Leg.StepAlpha, 2.f);
			Leg.Foot = FMath::Lerp(Leg.StepFrom, Leg.StepTo, Eased)
				+ FVector(0.f, 0.f, FMath::Sin(Leg.StepAlpha * UE_PI) * StepHeight * LimpLiftShare(Leg));
			if (Leg.StepAlpha >= 1.f)
			{
				Leg.Foot = Leg.StepTo;
				Leg.bStepping = false;
				Leg.LastStepTime = AnimTime;
				// Running, its feet kick up dust as they land (SpiderCreatureReact.cpp).
				NoteFootLanded(Leg, Speed);
			}
		}
	}

	// Legs the attack, death or a subclass's pose holds don't walk.
	const bool bFrontHeld = bPoseOverride && PoseOverride.bFrontFeet;
	auto IsHeld = [CurrentState, bFrontHeld](const FLeg& Leg)
	{
		return CurrentState == ECreatureState::Dead || ((CurrentState == ECreatureState::Attack || bFrontHeld) && Leg.Pair == 0);
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
			// Its throe's kicks, then the curl under the body, knees low and drawn in, tips under the thorax and abdomen; once
			// curled, a leg or two twitches its last (SpiderCreatureDeath.cpp). The bones keep the walking legs' roll.
			Pole = KneePole(Up, Outward) + Outward * Leg.KneeSpread;
			PoseDeadLeg(Leg, Index, Time, Hip, Outward, Up, Pole, Knee, Foot);
			Leg.Foot = Foot;
			Leg.bStepping = false;
		}
		else
		{
			if (bFrontHeld && Leg.Pair == 0)
			{
				// Where the subclass's pose holds it (dug into the floor, raised with a clod), on its spring.
				StepHeldFoot(Leg, OverrideFootTarget(Leg, Body), PoseOverride.FrontOmega, DeltaSeconds);
				Leg.bStepping = false;
				Leg.bNeedsReset = true;
			}
			else if (CurrentState == ECreatureState::Attack && Leg.Pair == 0)
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
			// Its planted foot stays put for the gait; a front leg the notice beat raises or idle life taps or grooms with is
			// posed off it (SpiderCreatureReact.cpp); a shot leg buckles and a stagger splays the feet (SpiderCreatureLegs.cpp).
			PoseGaitLeg(Leg, Hip, Leg.Foot + ReactFootOffset(Leg, Body), Outward, IsHeld(Leg), Pole, Knee, Foot);
		}
		Leg.PosedFoot = Foot;
		SetBone(SpiderBones::FirstLeg + Index * 2, Leg.FemurInSegment, SegmentFrame(Hip, Knee, Pole, Scale));
		SetBone(SpiderBones::FirstLeg + Index * 2 + 1, Leg.TibiaInSegment, SegmentFrame(Knee, Foot, Pole, Scale));
	}
}

// Its reactions (hits, death, coming back, the notice beat, idle life, footfalls): SpiderCreatureReact.cpp.
