#include "Creatures/SpiderCreature.h"
#include "Creatures/CreaturePoseAnimInstance.h"
#include "Combat/HealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Player/PlayerSize.h"
#include "UObject/ConstructorHelpers.h"

// The spider's body and legs each frame: the stepping gait (SpiderGaitRules: its numbers; SpiderCreatureGait.cpp: its motion,
// the air and the landing), the bite, the death's body (SpiderCreatureDeath.cpp). On a wall, ledge or tree in an ambush
// it's posed by SpiderCreatureCling.cpp instead.

namespace
{
	// The gait at full size. A spider of another size takes the same gait scaled in space and in time (multiply both by its
	// size): at the same speed, a big one strides long and slow, a small one scurries.

	/** How quickly a knee's spread follows its foot (1/s): well inside a step, smooth across the bite's hand-back. */
	constexpr float KneeSpreadRate = 15.f;
	/** A turn on the spot is stepped as a walk of the speed its feet this far out (cm at size 1) turn at. */
	constexpr float TurnRestRadius = 150.f;
	/** Its rest spots lead its turn by this share of a step's turning, so feet land where the turn is taking it. */
	constexpr float TurnLead = 0.6f;
	/** The body follows its target height this fast (1/s); in the air and just after a landing, close behind its capsule. */
	constexpr float BodyFollow = 10.f;
	constexpr float BodyFollowAir = 40.f;
	constexpr float BodyFollowLanded = 0.25f;
	/** A group's plant's dip fades this fast (1/s). */
	constexpr float PlantDipFade = 12.f;
	/** The bite's raised front legs: their knees this far out past KneePole (a share of the outward way). */
	constexpr float BiteKneeSpread = 0.8f;
}

ASpiderCreature::ASpiderCreature()
{
	DisplayName = FText::FromString(TEXT("Brown Spider"));
	ElementBody = EElementBody::Chitin;
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

	// A boss built on the spider (the Gravemother) keeps the look her fight was made round: the old bite and a level ride.
	bWeightyGait = !IsBossCreature();
	bWeightyBite = bWeightyGait && GetFightRole() == EFightRole::Melee;
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
	// Its ambush (SpiderCreatureLurk.cpp) and a skitter back (SpiderCreatureHunt.cpp) hold its brain, so they go first.
	TickLurk(DeltaSeconds);
	HoldForHunt(DeltaSeconds);
	Super::Tick(DeltaSeconds);
	AnimTime += DeltaSeconds;
	// Its circle's facing and its skitter's steps, after its brain's turn (SpiderCreatureHunt.cpp).
	TickHunt(DeltaSeconds);
	// Its idle life, its entrance and its death's last twitches move on (SpiderCreatureReact.cpp).
	TickReactions(DeltaSeconds);
	// Its hit feel's clocks: buckles, the stagger's cry, its wounds (SpiderCreatureHurt.cpp).
	TickHitFeel(DeltaSeconds);
	// Far away out of view it walks on unposed (OnPoseThawed puts its feet back under it); a corpse lying still keeps its pose.
	if (bRigReady && !IsPoseFrozen() && !IsCorpseSettled())
	{
		if (IsInDen())
		{
			// Coming out of its den in a wall (SpiderCreatureDen.cpp).
			AnimateDen(DeltaSeconds);
			return;
		}
		if (IsClinging())
		{
			// On its wall, ledge or tree (SpiderCreatureCling.cpp).
			AnimateCling(DeltaSeconds);
			return;
		}
		ReadGaitMotion(DeltaSeconds);
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
	const bool bDead = CurrentState == ECreatureState::Dead;
	const bool bAir = IsAirborne();

	// A subclass's own pose (the Gravemother's dig, lift and plough), alive: on the spider's, or in place of its attack's.
	PoseOverride = FSpiderPoseOverride();
	bPoseOverride = !bDead && GetPoseOverride(PoseOverride);
	const float OwnAttack = bPoseOverride ? FMath::Clamp(PoseOverride.OwnShare, 0.f, 1.f) : 1.f;

	// The body settles over its planted feet: the ground under each (a stepping foot's, under its arc), their average sets
	// the height and their differences tilt it. A front leg the bite holds up says nothing of the ground (its raised foot
	// used to lift and tip the body on top of the bite's own rear). A boss (the Gravemother: her dig and her clod's lift
	// were made round it) keeps every foot as it is, as it always did.
	const bool bFrontHeld = bWeightyGait && (CurrentState == ECreatureState::Attack || (bPoseOverride && PoseOverride.bFrontFeet));
	float Sum = 0.f, Front = 0.f, Back = 0.f, Left = 0.f, Right = 0.f;
	int32 Count = 0, Fronts = 0, Backs = 0, Lefts = 0, Rights = 0;
	for (const FLeg& Leg : Legs)
	{
		if (bFrontHeld && Leg.Pair == 0 && !bDead)
		{
			continue;
		}
		const float FootZ = static_cast<float>(Leg.bStepping && bWeightyGait
			? FMath::Lerp(Leg.StepFrom.Z, Leg.StepTo.Z, static_cast<double>(SpiderGait::StepAlong(Leg.StepAlpha))) : Leg.Foot.Z);
		Sum += FootZ;
		++Count;
		(Leg.Pair <= 1 ? Front : Back) += FootZ;
		++(Leg.Pair <= 1 ? Fronts : Backs);
		(Leg.Side > 0.f ? Right : Left) += FootZ;
		++(Leg.Side > 0.f ? Rights : Lefts);
	}
	const float Mean = Count > 0 ? Sum / Count : GroundZ;
	auto MeanOf = [Mean](float Total, int32 Number) { return Number > 0 ? Total / Number : Mean; };
	float TargetZ = FMath::Clamp(Mean + Ride, GroundZ + 25.f * Scale, GroundZ + Ride + 45.f * Scale);
	float TargetPitch = FMath::RadiansToDegrees(FMath::Atan2(MeanOf(Front, Fronts) - MeanOf(Back, Backs), 200.f * Scale));
	float TargetRoll = FMath::RadiansToDegrees(FMath::Atan2(MeanOf(Left, Lefts) - MeanOf(Right, Rights), 220.f * Scale));
	if (bAir)
	{
		// In the air: the body rides its capsule, its nose following its flight (down as it falls), level across.
		const FVector Flight = GetVelocity();
		TargetZ = GroundZ + Ride;
		TargetPitch = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Flight.Z), FMath::Max(static_cast<float>(Flight.Size2D()), 200.f)))
			* 0.6f, -35.f, 20.f);
		TargetRoll = 0.f;
	}
	float Lunge = 0.f;
	float TargetFang = 3.f + FMath::Max(0.f, FMath::Sin(AnimTime * 3.1f)) * 4.f;
	float AbdomenTilt = 0.f;
	float SwayYaw = 0.f;
	// The ground's tilt as its feet give it, kept for its death: dead, its feet fold in with the body and say nothing of the
	// ground (and a raised front pair would tip the body back).
	if (!bDead && CurrentState != ECreatureState::Attack && !bAir)
	{
		GroundPitch = TargetPitch;
		GroundRoll = TargetRoll;
	}

	BiteNow = SpiderGait::FBitePose();
	if (CurrentState == ECreatureState::Attack)
	{
		const float Windup = FMath::Clamp(Time / FMath::Max(AttackWindup, 0.05f), 0.f, 1.f);
		const float Strike = FMath::Clamp((Time - AttackWindup) / FMath::Max(AttackRecovery, 0.05f), 0.f, 1.f);
		if (bWeightyBite)
		{
			// Coiled, then reared into its threat display, a shiver its tell; the lunge and the snap (SpiderGait::BitePose).
			const float Shiver = Time < AttackWindup ? Time - (AttackWindup - SpiderGait::ShiverSeconds) : -1.f;
			BiteNow = SpiderGait::BitePose(Windup, Strike, Shiver >= 0.f ? Shiver : -1.f);
			TargetPitch += BiteNow.Pitch * OwnAttack;
			TargetZ += BiteNow.Rise * Scale * OwnAttack;
			Lunge = BiteNow.Lunge * Scale * OwnAttack;
			TargetFang = BiteNow.Fang;
			AbdomenTilt = BiteNow.Abdomen * OwnAttack;
		}
		else
		{
			// The boss's and the Spitter's attack as it was: rear up with fangs spread during the wind-up, then lunge and snap shut.
			const float Rear = FMath::InterpEaseInOut(0.f, 1.f, Windup, 2.f) * (1.f - Strike);
			TargetPitch += (24.f * Rear - 12.f * FMath::Sin(Strike * UE_PI)) * OwnAttack;
			TargetZ += 16.f * Scale * Rear * OwnAttack;
			Lunge = (-14.f * Rear + 30.f * FMath::Sin(Strike * UE_PI)) * Scale * OwnAttack;
			TargetFang = Strike > 0.f ? 0.f : 30.f * Windup;
		}
	}
	else if (bDead)
	{
		// Its throe, the drop onto its folding legs and the bounce, lying still on the ground's slope, its last shudder and
		// the sink (SpiderCreatureDeath.cpp). A lunge it died in eases off over the rear.
		DeadBody(Time, GroundZ, TargetZ, TargetPitch, TargetRoll, TargetFang);
		Lunge = DeathLunge * (1.f - FMath::SmoothStep(0.f, 0.25f * DeathTimeScale(Scale), Time));
	}
	else
	{
		TargetZ += FMath::Sin(AnimTime * 2.2f) * 1.5f * Scale * WoundBreath(); // breathing, heavier badly hurt
		// Its notice beat (reared, fangs flared), idle life (grooming, a weight shift), SpiderCreatureReact.cpp.
		ReactBody(TargetZ, TargetPitch, TargetRoll, TargetFang);
		if (bWeightyGait && !bAir)
		{
			// Riding its steps: up a little as a group swings, down as it plants, turned toward the reaching front leg, leaning
			// into a speed-up and banking into a turn (SpiderGait::BodyMotion).
			const SpiderGait::FBodyMotion Motion = GaitMotion();
			TargetZ += (Motion.Rise - Gait.PlantDip) * Scale;
			TargetPitch += Motion.Pitch;
			TargetRoll += Motion.Roll;
			SwayYaw = Motion.Yaw;
		}
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
	if (bDead)
	{
		// Dead, the body follows its death's own curves, which start where it was as it died: nothing to smooth, and the drop
		// lands on its moment rather than easing in late.
		BodyZ = TargetZ;
		BodyPitch = TargetPitch;
		BodyRoll = TargetRoll;
	}
	else
	{
		// In the air (and just down) the body keeps close to its capsule: a leap's arc would leave it trailing a metre behind.
		const float Follow = bAir || Gait.SinceLanded < BodyFollowLanded ? BodyFollowAir : BodyFollow;
		BodyZ = FMath::FInterpTo(BodyZ, TargetZ, DeltaSeconds, Follow);
		BodyPitch = FMath::FInterpTo(BodyPitch, TargetPitch, DeltaSeconds, 8.f);
		BodyRoll = FMath::FInterpTo(BodyRoll, TargetRoll, DeltaSeconds, 8.f);
		LungeNow = Lunge;
	}
	FangOpen = FMath::FInterpTo(FangOpen, TargetFang, DeltaSeconds, 14.f);
	// The jolts' springs (a hit's kick and tilt, the hit feel's twist, a landing's dip): SpiderCreatureGait.cpp.
	StepBodySprings(DeltaSeconds);
	Gait.PlantDip *= FMath::Exp(-PlantDipFade * FMath::Max(DeltaSeconds, 0.f));
	AbdomenKick = FMath::FInterpTo(AbdomenKick, 0.f, DeltaSeconds, 8.f);
	// The hit feel on top (SpiderCreatureHurt.cpp): the twist, a shot leg's roll and dip, the stagger's crouch, fangs, abdomen.
	// Laid on after the body's smoothing: each is its own curve already, and smoothing would only blunt a stagger's drop.
	const FHitFeelPose Feel = HitFeelPose();

	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw, 0.f).Quaternion();
	// Climbing out of the dirt as it comes into the world, the body starts under the ground (EmergeDrop); a landing presses it
	// down by its dip.
	const FVector Location = FVector(ActorLocation.X, ActorLocation.Y, BodyZ + EmergeDrop() - Feel.Drop - (bDead ? 0.f : Gait.LandDip * Scale))
		+ Yaw.RotateVector(FVector(Lunge, 0.f, 0.f)) + HurtOffset;
	// The frame carries the size like the mesh does: hips, pivots and poses set in it scale with the spider. A hit tilts it
	// away from the shot (HurtTilt: pitch, roll) and twists it (Feel.Yaw); its steps sway it (SwayYaw).
	const FQuat Twisted = FRotator(0.f, GetActorRotation().Yaw + Feel.Yaw + SwayYaw, 0.f).Quaternion();
	BodyFrame = FTransform(Twisted * FRotator(BodyPitch + HurtTilt.X + Feel.Pitch, 0.f, BodyRoll + HurtTilt.Y + Feel.Roll).Quaternion(),
		Location, FVector(Scale));
	// Just off its wall in a leap, the body turns from how it clung to how it flies (SpiderCreatureLurk.cpp keeps where it left).
	if (Lurk.Phase == ESpiderLurkPhase::Leaping)
	{
		const float Turned = FMath::SmoothStep(0.f, 0.22f, Lurk.Clock);
		if (Turned < 1.f)
		{
			BodyFrame.SetRotation(FQuat::Slerp(Lurk.LeftFrom, BodyFrame.GetRotation(), Turned));
			BodyFrame.SetLocation(FMath::Lerp(Lurk.LeftAt, BodyFrame.GetLocation(), static_cast<double>(Turned)));
		}
	}
	SetBone(SlotBody, BodyInFrame, BodyFrame);

	// Abdomen: slow breathing sway plus a jolt when hit and its tilt in a bite. Dead, the breath goes out of it, and the throe
	// lifts it and the drop slams it down (SpiderCreatureDeath.cpp). Fangs on top.
	const float Breathing = bDead ? DeathBreath(Time) : 1.f;
	PoseHeadAndAbdomen(Breathing, (bDead ? DeadAbdomenTilt(Time) : 0.f) + AbdomenTilt, Feel);
}

void ASpiderCreature::AnimateLegs(float DeltaSeconds)
{
	const FTransform& Body = BodyFrame;
	const FVector BodyCenter = Body.GetLocation();
	const FVector Up = Body.GetRotation().GetUpVector();
	const FVector ActorLocation = GetActorLocation();
	const FVector Velocity(GetVelocity().X, GetVelocity().Y, 0.f);
	const float Speed = Velocity.Size();
	const ECreatureState CurrentState = GetCreatureState();
	const bool bDead = CurrentState == ECreatureState::Dead;
	const bool bAir = IsAirborne();
	const float Time = GetStateTime();
	const float Scale = GetSizeScale();

	// Faster = quicker, longer, higher steps (SpiderGait); a turn on the spot is stepped as a walk its feet's speed. At another
	// size the full-size gait is scaled in space and time alike. A boss keeps the old gait its fight was made round.
	const float GaitSpeed = bWeightyGait ? FMath::Max(Speed, SpiderGait::TurnSpeed(Gait.YawRate, TurnRestRadius * Scale)) : Speed;
	const float StepDuration = (bWeightyGait ? SpiderGait::StepSeconds(GaitSpeed) : FMath::Clamp(0.26f - Speed * 0.00018f, 0.12f, 0.26f)) * Scale;
	const float StepHeight = (bWeightyGait ? SpiderGait::StepHeight(GaitSpeed) : 18.f + Speed * 0.025f) * Scale;
	// A distant spider updates only a few times a second. When an update is over half a step long, a step lasts just one
	// update: rounded up to two, each step would take twice as long while the body walks on, and the feet would trail.
	const float StepTime = DeltaSeconds > StepDuration * 0.5f ? DeltaSeconds : StepDuration;
	// Its rest spots lead its turn as they lead its walk, so a turning foot lands where the turn is taking it.
	const float LeadYaw = bWeightyGait ? FMath::Clamp(Gait.YawRate * StepDuration * TurnLead, -40.f, 40.f) : 0.f;
	const FQuat Yaw = FRotator(0.f, GetActorRotation().Yaw + LeadYaw, 0.f).Quaternion();

	// A Big hit's skids slide their planted feet on (SpiderCreatureLegs.cpp).
	SlideSkids(DeltaSeconds);
	// Feet mid-step carry on: off the ground first, across on an arc that peaks early, set down firmly (SpiderGait); a limping
	// leg's (badly hurt: SpiderCreatureHurt.cpp) lower and slower.
	for (FLeg& Leg : Legs)
	{
		if (Leg.bStepping)
		{
			Leg.StepAlpha = FMath::Min(1.f, Leg.StepAlpha + DeltaSeconds / (StepTime * LimpTimeScale(Leg)));
			const float Along = bWeightyGait ? SpiderGait::StepAlong(Leg.StepAlpha) : FMath::InterpEaseInOut(0.f, 1.f, Leg.StepAlpha, 2.f);
			const float Lift = bWeightyGait ? SpiderGait::StepLift(Leg.StepAlpha) : FMath::Sin(Leg.StepAlpha * UE_PI);
			Leg.Foot = FMath::Lerp(Leg.StepFrom, Leg.StepTo, static_cast<double>(Along)) + FVector(0.f, 0.f, Lift * StepHeight * LimpLiftShare(Leg));
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

	// Legs the attack, death, the air or a subclass's pose holds don't walk.
	const bool bFrontHeld = bPoseOverride && PoseOverride.bFrontFeet;
	auto IsHeld = [this, bFrontHeld, bAir](const FLeg& Leg) { return IsLegHeld(Leg, bFrontHeld, bAir); };

	// Where each foot wants to be: its rest spot, led by the velocity so it lands ahead of the body.
	TArray<FVector, TInlineAllocator<8>> RestSpots;
	for (const FLeg& Leg : Legs)
	{
		RestSpots.Add(ActorLocation + Yaw.RotateVector(Leg.Rest * Scale) + Velocity * (StepDuration * 0.9f));
	}
	// Down from the air: every foot reaches for the ground at once, and the body dips by the fall (SpiderCreatureGait.cpp).
	if (Gait.bWasAirborne && !bAir && !bDead)
	{
		PlantAfterFall(RestSpots, StepHeight);
	}
	Gait.bWasAirborne = bAir && !bDead;

	// Which feet step now: strayed, trailing or pulled out toward the end of their reach, an idle weight shift; the
	// alternating groups (SpiderCreatureGait.cpp).
	PlanSteps(RestSpots, GaitSpeed, Speed, bFrontHeld, bAir);

	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		FLeg& Leg = Legs[Index];
		const FVector Hip = Body.TransformPosition(Leg.Hip);
		const FVector Outward = (Hip - BodyCenter).GetSafeNormal2D();
		FVector Pole = KneePole(Up, Outward); // high, arched knees
		FVector Knee;
		FVector Foot;

		if (bDead)
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
			if (bAir)
			{
				// In the air: reaching out round the body for the ground, the front pair ahead (SpiderCreatureGait.cpp).
				PoseAirFoot(Leg, Body, DeltaSeconds);
				Leg.bBiteHeld = false;
			}
			else if (bFrontHeld && Leg.Pair == 0)
			{
				// Where the subclass's pose holds it (dug into the floor, raised with a clod), on its spring.
				StepHeldFoot(Leg, OverrideFootTarget(Leg, Body), PoseOverride.FrontOmega, DeltaSeconds);
				Leg.bStepping = false;
				Leg.bNeedsReset = true;
			}
			else if (CurrentState == ECreatureState::Attack && Leg.Pair == 0)
			{
				// Front legs rise from where they stood (coiled, then high and spread in its threat display), then slam down
				// ahead of it on the strike (the body's frame carries the size). A spring carries them, so they lift off from
				// rest instead of leaping up at full speed, and the slam's stiffer spring keeps it quick.
				if (!Leg.bBiteHeld)
				{
					Leg.bBiteHeld = true;
					Leg.HeldFrom = Leg.Foot;
				}
				const float Strike = FMath::Clamp((Time - AttackWindup) / (AttackRecovery * 0.5f), 0.f, 1.f);
				FVector Raised;
				if (bWeightyBite)
				{
					// Raised forward and out, knees out to the side and bent (BiteKneeSpread), tips held high: never straight up.
					const FVector High = Body.TransformPosition(FVector(Leg.Hip.X + 120.f, Leg.Hip.Y * 1.6f * BiteNow.FrontSpread, 60.f));
					Raised = FMath::Lerp(Leg.HeldFrom, High, static_cast<double>(BiteNow.FrontRaise));
				}
				else
				{
					Raised = Body.TransformPosition(FVector(Leg.Hip.X + 80.f, Leg.Hip.Y * 1.5f, 50.f));
				}
				// The ground is looked for only once the strike starts: until then the foot is all Raised, and a probe per front
				// leg per frame through the whole windup bought nothing.
				const FVector Slam = Strike > 0.f
					? GroundUnder(ActorLocation + Yaw.RotateVector(FVector(150.f, Leg.Side * 50.f, 0.f) * Scale)) : Raised;
				StepHeldFoot(Leg, FMath::Lerp(Raised, Slam, static_cast<double>(Strike)), Strike > 0.f ? 32.f : (bWeightyBite ? 16.f : 14.f),
					DeltaSeconds);
				Leg.bStepping = false;
				Leg.bNeedsReset = true;
			}
			else
			{
				Leg.HeldVelocity = FVector::ZeroVector;
				Leg.bBiteHeld = false;
			}
			// A walking knee spreads out as its foot comes in toward the hip; a held one arches as the bite has it. The spread
			// eases, so a leg the bite hands back to the gait doesn't swing its knee over in a frame.
			// The bite's raised front legs hold their knees out to the side as they rise (a bent, spread threat display, not a leg
			// pointing up), easing out with the raise so the knee never swings round in a frame.
			const bool bBiteLeg = bWeightyBite && CurrentState == ECreatureState::Attack && Leg.Pair == 0 && !bAir;
			const float WantedSpread = bBiteLeg ? BiteKneeSpread * BiteNow.FrontRaise
				: (IsHeld(Leg) ? 0.f : WantedKneeSpread(Leg, static_cast<float>(FVector::Dist(Hip, Leg.Foot)) / Scale));
			Leg.KneeSpread = DeltaSeconds > 0.f ? FMath::FInterpTo(Leg.KneeSpread, WantedSpread, DeltaSeconds, KneeSpreadRate) : WantedSpread;
			Pole = KneePole(Up, Outward) + Outward * Leg.KneeSpread;
			// Its planted foot stays put for the gait; a front leg the notice beat raises or idle life taps or grooms with is
			// posed off it (SpiderCreatureReact.cpp); a shot leg buckles and a stagger splays the feet (SpiderCreatureLegs.cpp).
			PoseGaitLeg(Leg, Hip, Leg.Foot + ReactFootOffset(Leg, Body), Outward, IsHeld(Leg), Pole, Knee, Foot);
		}
		Leg.PosedFoot = Foot;
		SetBone(SlotFirstLeg + Index * 2, Leg.FemurInSegment, SegmentFrame(Hip, Knee, Pole, Scale));
		SetBone(SlotFirstLeg + Index * 2 + 1, Leg.TibiaInSegment, SegmentFrame(Knee, Foot, Pole, Scale));
	}
}

// Its reactions (hits, death, coming back, the notice beat, idle life, footfalls): SpiderCreatureReact.cpp.
