#include "Creatures/SlimeCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreaturePoseAnimInstance.h"
#include "Creatures/CreatureUpdateRate.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The capsule: the slime is about 1.5 m wide at its foot and 0.6 m tall. */
	constexpr float CapsuleRadius = 50.f;

	// Hops (cm and seconds). Strolling hops are short and low with a breather between them; chasing hops are longer
	// and come one after another.
	constexpr float CrouchTime = 0.12f;
	constexpr float LaunchTime = 0.08f;
	constexpr float LandTime = 0.06f;
	constexpr float WanderHopLength[2] = { 150.f, 220.f };
	constexpr float WanderHopHeight[2] = { 60.f, 75.f };
	constexpr float WanderHopPause[2] = { 0.3f, 0.6f };
	constexpr float ChaseHopLength[2] = { 200.f, 260.f };
	constexpr float ChaseHopHeight[2] = { 70.f, 90.f };
	constexpr float ChaseHopPause[2] = { 0.12f, 0.25f };
	// A standing slime nudged off a neighbour's footprint (CreatureBaseSpacing.cpp) shuffles a little, not a stroll's hop.
	constexpr float ShuffleHopLength[2] = { 70.f, 90.f };
	constexpr float ShuffleHopHeight[2] = { 25.f, 35.f };

	// The leap attack: from a deep squash, up to LeapLength at the player, hurting it if it lands within LeapHitRadius.
	constexpr float LeapLength = 500.f;
	constexpr float LeapHeight = 120.f;
	constexpr float LeapHitRadius = 140.f;
	/** Lands this far short of the player's middle, against its side rather than on its head. */
	constexpr float LeapShortOf = 70.f;

	// Squash heights (1 = rest).
	constexpr float CrouchSquash = 0.7f;
	constexpr float LaunchStretch = 1.3f;
	constexpr float ApexSquash = 1.05f;
	constexpr float FallStretch = 1.15f;
	constexpr float LandSquash = 0.6f;
	constexpr float TelegraphSquash = 0.55f;
	constexpr float LeapLandSquash = 0.45f;
	constexpr float HurtSquash = 0.82f;
	constexpr float IdleWobble = 0.04f;
	constexpr float IdleWobbleHz = 1.2f;
	// The squash and the core's springs are SlimeCreatureBody.cpp's.

	// Death: it flattens into a puddle, lies there for the creature's CorpseTime, then dries up.
	constexpr float FlattenTime = 0.4f;
	constexpr float PuddleHeight = 0.15f;
	constexpr float PuddleWidth = 1.6f;
	constexpr float DryUpTime = 0.8f;

	/** A scale along the mesh's axes, as the bone's own (its rest rotation maps its axes onto the mesh's). */
	FVector BoneScale(const FQuat& Rotation, const FVector& MeshScale)
	{
		FVector Result;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			FVector Unit = FVector::ZeroVector;
			Unit[Axis] = 1.f;
			const FVector Along = Rotation.RotateVector(Unit);
			Result[Axis] = Along.X * Along.X * MeshScale.X + Along.Y * Along.Y * MeshScale.Y + Along.Z * Along.Z * MeshScale.Z;
		}
		return Result;
	}

	float RandomIn(const float (&Range)[2])
	{
		return FMath::FRandRange(Range[0], Range[1]);
	}
}

ASlimeCreature::ASlimeCreature()
{
	DisplayName = FText::FromString(TEXT("Meadow Slime"));
	Health->MaxHealth = 120.f;
	CriticalSpotBones = { CoreBone };

	// Slow and weak alone; dangerous as a group, which turns on you together.
	WalkSpeed = 170.f;
	ChaseSpeed = 220.f;
	AggroRadius = 1800.f;
	WanderRadius = 600.f;
	PackAlertRadius = 2500.f;
	AttackRange = 450.f;
	AttackDamage = 6.f;
	AttackWindup = 0.5f;
	AttackRecovery = 1.4f;
	AttackCooldown = 1.6f;
	CorpseTime = 2.5f;
	HealthBarHeight = 75.f;
	// A hurt slime's call reaches every slime near it (PackAlertRadius), whatever its class.
	PackTag = TEXT("Slime");

	GetCapsuleComponent()->InitCapsuleSize(CapsuleRadius, CapsuleRadius);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	// Hops are ballistic: no steering in the air, and a landing stops it dead (the splat) instead of sliding on.
	Movement->AirControl = 0.f;
	Movement->GroundFriction = 12.f;
	Movement->BrakingDecelerationWalking = 3000.f;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Model(TEXT("/Game/Art/Creatures/SK_Slime.SK_Slime"));
	USkeletalMeshComponent* Body = GetMesh();
	Body->SetSkeletalMeshAsset(Model.Object);
	// The model's origin is the ground under its middle: the foot of the capsule.
	Body->SetRelativeLocation(FVector(0.f, 0.f, -CapsuleRadius));
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(UCreaturePoseAnimInstance::StaticClass());
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->PrimaryComponentTick.TickGroup = TG_PostPhysics;

	// The physics asset's two hulls (gel and core) are the hit zones, seen only by weapon and visibility traces.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionObjectType(ECC_WorldDynamic);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Body->SetGenerateOverlapEvents(false);
	Body->SetCanEverAffectNavigation(false);
}

void ASlimeCreature::BeginPlay()
{
	Super::BeginPlay();

	GetMesh()->AddTickPrerequisiteActor(this);
	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	const FReferenceSkeleton* Skeleton = Model ? &Model->GetRefSkeleton() : nullptr;
	const int32 BodyIndex = Skeleton ? Skeleton->FindBoneIndex(BodyBone) : INDEX_NONE;
	const int32 CoreIndex = Skeleton ? Skeleton->FindBoneIndex(CoreBone) : INDEX_NONE;
	bRigReady = BodyIndex != INDEX_NONE && CoreIndex != INDEX_NONE;
	if (!bRigReady)
	{
		UE_LOG(LogLooter, Error, TEXT("%s needs SK_Slime with body and core bones (Art/Models/Creatures/Slime.py)."), *GetName());
		return;
	}
	BodyRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(*Skeleton, BodyIndex);
	CoreRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(*Skeleton, CoreIndex);
	// Each slime wobbles in its own time, so a group doesn't breathe in step.
	IdlePhase = FMath::FRandRange(0.f, 2.f * UE_PI);
	NextHopDelay = FMath::FRandRange(0.f, 0.6f);
	AnimateBody(0.f);
}

FVector ASlimeCreature::HopVelocity(const FVector& Direction, float Length, float Height, float Gravity)
{
	const float Up = FMath::Sqrt(2.f * Gravity * FMath::Max(Height, 1.f));
	const float AirTime = 2.f * Up / Gravity;
	return Direction.GetSafeNormal2D() * (Length / AirTime) + FVector(0.f, 0.f, Up);
}

FVector ASlimeCreature::SquashScale(float Z)
{
	const float Side = 1.f / FMath::Sqrt(FMath::Max(Z, 0.05f));
	return FVector(Side, Side, Z);
}

bool ASlimeCreature::IsCriticalSpot(const FHitResult& Hit) const
{
	if (Super::IsCriticalSpot(Hit))
	{
		return true;
	}
	if (Hit.GetComponent() != GetMesh() || GetMesh()->GetBoneIndex(CoreBone) == INDEX_NONE)
	{
		return false;
	}
	// A shot stops at the gel's surface: it's a crit when the line it was on would have gone on through the core (which
	// is as much bigger as the slime is).
	const FVector Direction = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
	const FVector Core = GetMesh()->GetBoneLocation(CoreBone);
	const float Reach = CapsuleRadius * 4.f * GetSizeScale();
	return !Direction.IsZero()
		&& FMath::PointDistToSegment(Core, Hit.ImpactPoint, Hit.ImpactPoint + Direction * Reach) <= CoreRadius * GetSizeScale();
}

// ---------------------------------------------------------------------------
// Hopping
// ---------------------------------------------------------------------------

void ASlimeCreature::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The brain steers like any creature's, but a slime never slides: it takes the way it was told to go and hops it.
	const FVector Wanted = ConsumeMovementInputVector();
	if (!IsDead())
	{
		TickHops(Wanted, DeltaSeconds);
	}
	// Far away out of view it hops on unposed (OnPoseThawed settles its springs again).
	if (!IsPoseFrozen())
	{
		AnimateBody(DeltaSeconds);
	}
}

void ASlimeCreature::TickHops(const FVector& Wanted, float DeltaSeconds)
{
	HopTime += DeltaSeconds;
	switch (Hop)
	{
	case EHop::Ground:
		if (!Wanted.IsNearlyZero() && HopTime >= NextHopDelay && GetCharacterMovement()->IsMovingOnGround()
			&& GetCreatureState() != ECreatureState::Attack)
		{
			Hop = EHop::Crouch;
			HopTime = 0.f;
			HopDirection = Wanted.GetSafeNormal2D();
			// An idle brain never steers, so the only thing that moves an idle slime is a neighbour's crowding: that's a
			// shuffle aside, not the stroll's 1.5 to 2 m hop.
			bShuffleHop = GetCreatureState() == ECreatureState::Idle;
			Squash.Ramp(CrouchSquash, CrouchTime);
		}
		break;
	case EHop::Crouch:
		if (HopTime >= CrouchTime)
		{
			Launch();
		}
		break;
	default:
		break;
	}
}

void ASlimeCreature::Launch()
{
	// Hops are the full-size slime's, times its size: a big one bounds farther and higher (and so a little slower).
	const float Scale = GetSizeScale();
	const bool bHunting = GetCreatureState() == ECreatureState::Chase || GetCreatureState() == ECreatureState::Return;
	float Length = RandomIn(bShuffleHop ? ShuffleHopLength : bHunting ? ChaseHopLength : WanderHopLength) * Scale;
	const float Height = RandomIn(bShuffleHop ? ShuffleHopHeight : bHunting ? ChaseHopHeight : WanderHopHeight) * Scale;
	// Never hop off an edge: shorten the hop until it lands on ground, or stay put.
	FVector Ground;
	while (Length > 60.f * Scale && !FindGround(GetActorLocation() + HopDirection * Length, 300.f * Scale, 600.f * Scale, Ground))
	{
		Length *= 0.5f;
	}
	if (Length <= 60.f * Scale)
	{
		Hop = EHop::Ground;
		HopTime = 0.f;
		return;
	}
	LaunchCharacter(HopVelocity(HopDirection, Length, Height, -GetCharacterMovement()->GetGravityZ()), true, true);
	Hop = EHop::Air;
	HopTime = 0.f;
	LaunchedFrom = GetActorLocation();
	PlannedLength = Length;
	Squash.Ramp(LaunchStretch, LaunchTime);
	// The core is left behind for a moment: down, and back from the way it jumps.
	CoreVelocity += FVector(0.f, 0.f, -90.f) - GetActorRotation().UnrotateVector(HopDirection) * 60.f;
	NextHopDelay = RandomIn(bHunting ? ChaseHopPause : WanderHopPause);
}

void ASlimeCreature::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (IsDead())
	{
		return;
	}
	Hop = EHop::Ground;
	HopTime = 0.f;
	CoreVelocity += FVector(0.f, 0.f, 110.f);
	Voice->Play(LooterSoundCue::SlimeHop, bLeaping ? 1.f : (GetTarget() ? 0.8f : 0.5f));

	if (bLeaping)
	{
		bLeaping = false;
		Squash.Ramp(LeapLandSquash, LandTime);
		// The leap hurts whoever it lands on (up and down, a small slime still reaches a player, as a full-size one does).
		APawn* Victim = GetTarget();
		if (IsValidTarget(Victim))
		{
			const float Scale = GetSizeScale();
			const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
			if (ToVictim.Size2D() <= LeapHitRadius * Scale && FMath::Abs(ToVictim.Z) < 200.f * FMath::Max(Scale, 1.f))
			{
				HitWithAttack(Victim, ToVictim.GetSafeNormal2D());
			}
		}
		return;
	}

	Squash.Ramp(LandSquash, LandTime);
	// A hop that got under a third of the way hit something; a few in a row and the brain takes a detour.
	const float Progress = PlannedLength > 0.f ? FVector::DotProduct(GetActorLocation() - LaunchedFrom, HopDirection) / PlannedLength : 1.f;
	BlockedHops = Progress < 0.35f ? BlockedHops + 1 : 0;
}

bool ASlimeCreature::IsStuck(float Speed) const
{
	return BlockedHops >= 2;
}

// ---------------------------------------------------------------------------
// The leap attack
// ---------------------------------------------------------------------------

void ASlimeCreature::OnAttackStarted()
{
	// The telegraph: a deep squash, held until it leaps.
	Hop = EHop::Ground;
	bTelegraph = true;
	Squash.Ramp(TelegraphSquash, 0.15f);
	Squash.Target = TelegraphSquash;
}

void ASlimeCreature::Strike()
{
	bTelegraph = false;
	const float Scale = GetSizeScale();
	const APawn* Victim = GetTarget();
	FVector ToVictim = Victim ? Victim->GetActorLocation() - GetActorLocation() : GetActorForwardVector() * (LeapLength * Scale);
	ToVictim.Z = 0.f;
	const float Length = FMath::Clamp(static_cast<float>(ToVictim.Size()) - LeapShortOf * Scale, 80.f * Scale, LeapLength * Scale);
	HopDirection = ToVictim.GetSafeNormal();
	LaunchCharacter(HopVelocity(HopDirection, Length, LeapHeight * Scale, -GetCharacterMovement()->GetGravityZ()), true, true);
	Hop = EHop::Air;
	HopTime = 0.f;
	bLeaping = true;
	LaunchedFrom = GetActorLocation();
	PlannedLength = Length;
	Squash.Ramp(LaunchStretch + 0.05f, LaunchTime);
	CoreVelocity += FVector(0.f, 0.f, -120.f);
	OnAttackStrike(false);
}

// ---------------------------------------------------------------------------
// Hits, death and respawn
// ---------------------------------------------------------------------------

void ASlimeCreature::OnHurt(bool bCritical, const FVector& HitLocation)
{
	if (IsDead())
	{
		return;
	}
	// A squash and a wobble; the core jiggles away from the hit (harder on a crit).
	if (Hop == EHop::Ground && !bTelegraph && !Squash.IsRamping())
	{
		Squash.Ramp(HurtSquash, 0.05f);
	}
	const FVector Away = GetActorTransform().InverseTransformVector(GetActorLocation() - HitLocation).GetSafeNormal();
	CoreVelocity += (Away + FMath::VRand() * 0.5f) * (bCritical ? 220.f : 130.f);
}

void ASlimeCreature::OnDied()
{
	Hop = EHop::Ground;
	bLeaping = false;
	bTelegraph = false;
	// Death stops all movement, mid-hop too: drop it onto the ground so the puddle lies there, not in the air.
	FVector Ground;
	if (FindGround(GetActorLocation(), 0.f, 1000.f, Ground))
	{
		SetActorLocation(Ground + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), false, nullptr,
			ETeleportType::TeleportPhysics);
	}
}

bool ASlimeCreature::CanStartAttack() const
{
	// Leaps start from the ground, never mid-hop.
	return Hop == EHop::Ground && GetCharacterMovement()->IsMovingOnGround();
}

void ASlimeCreature::OnRespawned()
{
	Hop = EHop::Ground;
	HopTime = 0.f;
	BlockedHops = 0;
	Squash = FSquashSpring();
	CoreOffset = CoreVelocity = FVector::ZeroVector;
	AnimateBody(0.f);
}

void ASlimeCreature::OnPoseThawed()
{
	// The springs stood still while it hopped on unposed, but every hop still kicked the core and started a squash. Carry on
	// from the shape it was last drawn in, with the springs at rest, so it doesn't pop if it's in view: the squashes and
	// kicks of hops long over are dropped, and the springs ease it back to shape. (It's never frozen mid-attack, so no
	// telegraph squash is lost.)
	Squash.Velocity = 0.f;
	Squash.RampTime = Squash.RampLength = 0.f;
	CoreVelocity = FVector::ZeroVector;
	AnimateBody(0.f);
}

void ASlimeCreature::SetHitVolumesEnabled(bool bEnabled)
{
	GetMesh()->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

// ---------------------------------------------------------------------------
// The body (its springs and its fit to the ground: SlimeCreatureBody.cpp)
// ---------------------------------------------------------------------------

void ASlimeCreature::AnimateBody(float DeltaSeconds)
{
	if (!bRigReady)
	{
		return;
	}
	AnimTime += DeltaSeconds;
	// The body bone's scale (the mesh's, against the model; the actor's size is on top).
	FVector Shape;
	if (IsDead())
	{
		// A puddle, then nothing.
		const float Time = GetStateTime();
		const float Flat = FMath::InterpEaseOut(0.f, 1.f, FMath::Min(Time / FlattenTime, 1.f), 2.f);
		const float Dry = FMath::Clamp((Time - FlattenTime - CorpseTime) / DryUpTime, 0.f, 1.f);
		Shape = FMath::Lerp(SquashScale(Squash.Value), FVector(PuddleWidth, PuddleWidth, PuddleHeight), Flat) * FMath::Max(1.f - Dry, 0.01f);
		Shape.Z = FMath::Max(Shape.Z, 0.01f);
	}
	else
	{
		// What the spring heads for between the hop's forced beats: a slow wobble at rest; in the air, stretched on the
		// way up, round at the top and a little stretched again coming down. (A hop's speed grows with the root of its
		// height, so a bigger slime's launch counts as the same stretch.)
		if (Hop == EHop::Air)
		{
			const float Vertical = GetVelocity().Z / (400.f * FMath::Sqrt(GetSizeScale()));
			Squash.Target = Vertical > 0.f ? ApexSquash + (LaunchStretch - ApexSquash) * FMath::Min(Vertical, 1.f)
				: ApexSquash + (FallStretch - ApexSquash) * FMath::Min(-Vertical, 1.f);
		}
		else if (!bTelegraph)
		{
			Squash.Target = 1.f + IdleWobble * FMath::Sin(2.f * UE_PI * IdleWobbleHz * AnimTime + IdlePhase);
		}
		StepSprings(Squash, CoreOffset, CoreVelocity, DeltaSeconds);
		Shape = SquashScale(Squash.Value);
	}

	// The body bone carries everything but the core: scaling it squashes the slime against the ground. The core rides
	// with it (half as squashed: it's firmer), offset by its lag. All of it is in the mesh's space, so a slime of another
	// size squashes and wobbles the same, scaled. Then the whole body lies along the ground under it, turned about its foot's
	// middle (the mesh's origin) and set down onto the ground there (FitToGround): the squash follows the slope, and the foot
	// neither cuts into the hill on one side nor hangs over it on the other.
	FitToGround(DeltaSeconds);
	const FVector Seat(0.f, 0.f, -GroundDrop);
	const FVector CoreScale = FMath::Lerp(FVector::OneVector, Shape, 0.5f);
	const FVector CoreLocation = CoreRest.GetLocation() * Shape + CoreOffset;
	BonePose.SetNum(2);
	BonePose[0] = { BodyBone, FTransform(GroundTilt * BodyRest.GetRotation(), Seat + GroundTilt.RotateVector(BodyRest.GetLocation()),
		BoneScale(BodyRest.GetRotation(), Shape)) };
	BonePose[1] = { CoreBone, FTransform(GroundTilt * CoreRest.GetRotation(), Seat + GroundTilt.RotateVector(CoreLocation),
		BoneScale(CoreRest.GetRotation(), CoreScale)) };
}
