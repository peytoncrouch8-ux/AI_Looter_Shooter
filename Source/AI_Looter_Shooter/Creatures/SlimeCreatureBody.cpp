// ASlimeCreature's body: the squash and core springs, and its fit to the ground. SlimeCreature.cpp works out the shape
// each frame (AnimateBody) and hops.
//
// The fit: the slime stands upright on its capsule, whose foot touches a slope uphill of its middle, so its flat, wide foot
// (78 cm across at rest, more squashed) cut into the hill on one side and hung over it on the other, and a squash pressed
// the gel through the ground (the user's "models clip"). On the ground it now measures the ground under its foot (a plane
// through four points round it, only after it has moved: it stands still between hops), lays the body along it, turned
// about the foot's middle, and sets it down onto the ground under its middle. In the air it eases back upright.

#include "Creatures/SlimeCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/HitFeelTuning.h"
#include "Combat/HitPose.h"
#include "Creatures/CreatureHitCues.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureKillLook.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/CreatureUpdateRate.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/**
	 * The squash spring: about 3.2 wobbles a second, lightly damped, so a landing at 0.6 overshoots to about 1.1, dips
	 * to 0.95 and settles in about a third of a second.
	 */
	constexpr float SquashStiffness = 404.f;
	constexpr float SquashDamping = 14.f;
	/** The core's spring: a little quicker and looser, so it jiggles inside the gel. */
	constexpr float CoreStiffness = 632.f;
	constexpr float CoreDamping = 12.6f;
	/** The springs' longest step (s): small enough for both to stay steady. */
	constexpr float SpringStep = 1.f / 120.f;

	/** The ground is measured again once it has moved this far (cm), at this share of its foot's radius round its middle. */
	constexpr float RefitDistance = 4.f;
	constexpr float ProbeShare = 0.7f;
	/** The farthest it sets itself down (or up) onto the ground under its middle (cm at size 1): no reaching into a hole. */
	constexpr float MaxDrop = 30.f;
	constexpr float MaxRise = 10.f;
	/** How quickly it settles onto the ground's tilt and seat, and rights itself in the air (1/s). */
	constexpr float FitRate = 14.f;
	constexpr float UprightRate = 8.f;
}

void ASlimeCreature::FSquashSpring::Ramp(float NewTo, float Seconds)
{
	From = Value;
	To = NewTo;
	RampTime = 0.f;
	RampLength = FMath::Max(Seconds, KINDA_SMALL_NUMBER);
	Velocity = 0.f;
}

void ASlimeCreature::FSquashSpring::Tick(float DeltaSeconds)
{
	if (IsRamping())
	{
		RampTime += DeltaSeconds;
		Value = FMath::Lerp(From, To, FMath::InterpEaseOut(0.f, 1.f, FMath::Min(RampTime / RampLength, 1.f), 2.f));
		// It then springs from rest at the ramp's end, which gives the overshoot after a landing.
		return;
	}
	Velocity += (SquashStiffness * (Target - Value) - SquashDamping * Velocity) * DeltaSeconds;
	Value += Velocity * DeltaSeconds;
}

void ASlimeCreature::StepSprings(FSquashSpring& SquashSpring, FVector& CoreLag, FVector& CoreLagSpeed, float DeltaSeconds)
{
	const float Seconds = FMath::Min(DeltaSeconds, FCreatureUpdateRate::MaxInterval);
	if (Seconds <= 0.f)
	{
		return;
	}
	// Even steps no longer than SpringStep: short enough for both springs to stay steady, and a long update takes the
	// same steps as the short ones it stands in for (a slow-ticking slime wobbles like a near one).
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Seconds / SpringStep - 0.01f));
	const float Step = Seconds / Steps;
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		SquashSpring.Tick(Step);
		CoreLagSpeed += (-CoreStiffness * CoreLag - CoreDamping * CoreLagSpeed) * Step;
		CoreLag += CoreLagSpeed * Step;
	}
}

ACreatureBase::FFootprint ASlimeCreature::GetFootprint() const
{
	FFootprint Footprint;
	Footprint.Radius = FootRadius - 4.f;
	return Footprint;
}

FQuat ASlimeCreature::TiltForGround(const FVector& GroundNormal)
{
	const FVector Normal = GroundNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	if (Normal.Z <= 0.0)
	{
		return FQuat::Identity;
	}
	const FQuat Full = FQuat::FindBetweenNormals(FVector::UpVector, Normal);
	const float Degrees = FMath::RadiansToDegrees(Full.GetAngle());
	return Degrees <= MaxGroundTilt ? Full : FQuat::Slerp(FQuat::Identity, Full, MaxGroundTilt / Degrees).GetNormalized();
}

void ASlimeCreature::FitToGround(float DeltaSeconds)
{
	const bool bOnGround = IsDead() || Hop != EHop::Air;
	const float Scale = GetSizeScale();
	if (!bOnGround)
	{
		// Airborne: upright over the capsule's foot, as it hops.
		GroundTiltTarget = FQuat::Identity;
		GroundDropTarget = 0.f;
		GroundMeasuredAt = FVector(1.0e9);
	}
	else if (FVector::DistSquared(GetActorLocation(), GroundMeasuredAt) > FMath::Square(RefitDistance * Scale))
	{
		// The ground under its foot: four points round its middle (a plane through them), and the point under its middle.
		GroundMeasuredAt = GetActorLocation();
		const USkeletalMeshComponent* Body = GetMesh();
		const FTransform& MeshXf = Body->GetComponentTransform();
		const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const float Reach = FootRadius * ProbeShare * Scale;
		const FVector Middle = GetActorLocation();
		FVector Points[4];
		bool bFound = true;
		for (int32 Index = 0; Index < 4 && bFound; ++Index)
		{
			const FVector Offset = MeshXf.TransformVectorNoScale(FVector(Index == 0 ? Reach : (Index == 1 ? -Reach : 0.f),
				Index == 2 ? Reach : (Index == 3 ? -Reach : 0.f), 0.f));
			bFound = FindGround(Middle + Offset, HalfHeight + Reach, HalfHeight + 2.f * Reach, Points[Index]);
		}
		FVector Under;
		if (bFound && FindGround(Middle, HalfHeight, HalfHeight + MaxDrop * Scale, Under))
		{
			FVector Normal = FVector::CrossProduct(Points[0] - Points[1], Points[2] - Points[3]);
			Normal *= Normal.Z < 0.0 ? -1.0 : 1.0;
			GroundTiltTarget = TiltForGround(MeshXf.InverseTransformVectorNoScale(Normal));
			// The mesh's origin is the capsule's foot; the ground under its middle is a little below it on a slope (the capsule
			// touches uphill), or under the floor's float. In the mesh's space (the actor's size scales it).
			const float FootZ = static_cast<float>(Middle.Z) - HalfHeight;
			GroundDropTarget = FMath::Clamp((FootZ - static_cast<float>(Under.Z)) / Scale, -MaxRise, MaxDrop);
		}
		else
		{
			GroundTiltTarget = FQuat::Identity;
			GroundDropTarget = 0.f;
		}
	}
	if (!bGroundFitStarted || DeltaSeconds <= 0.f)
	{
		GroundTilt = GroundTiltTarget;
		GroundDrop = GroundDropTarget;
		bGroundFitStarted = true;
		return;
	}
	const float Rate = bOnGround ? FitRate : UprightRate;
	GroundTilt = FQuat::Slerp(GroundTilt, GroundTiltTarget, FMath::Min(DeltaSeconds * Rate, 1.f)).GetNormalized();
	GroundDrop = FMath::FInterpTo(GroundDrop, GroundDropTarget, DeltaSeconds, Rate);
}

// ---------------------------------------------------------------------------
// The hit-feel pass (HITS lane D, 2026-10-28): a hop in the air knocked off its line, the wounded gel's shrink and drips, the
// stagger's cue and the core sinking, and an overkill held inflated through its hit-stop until it bursts.
// ---------------------------------------------------------------------------

namespace
{
	/** The wounded gel is this share of its size at no health left, easing to it as its health goes. */
	constexpr float WoundShrinkMost = 0.88f;
	constexpr float WoundShrinkRate = 8.f;
	/** A stagger sinks its core this far (cm, the mesh's space). */
	constexpr float StaggerCoreSink = 10.f;
	/** A hop pushed in the air may land this far off its ground's height (cm at size 1): no knocking one off a ledge. */
	constexpr float KnockLedge = 250.f;
	/** An overkill's wait for its hit-stop to end is at most this long on its own clock (a test world never ends one). */
	constexpr float OverkillWaitMost = 0.15f;
	/** Wounded slimes drip within this far of a viewer (cm), at most this many at once in the world, one gel bit this often (s). */
	constexpr float DripSeenWithin = 2500.f;
	constexpr int32 DripMost = 6;
	constexpr float DripEvery = 0.8f;

	/** The slimes dripping now, so no more than DripMost drip at once. */
	TArray<TWeakObjectPtr<const AActor>>& Drippers()
	{
		static TArray<TWeakObjectPtr<const AActor>> Active;
		return Active;
	}

	/** Whether Slime may drip now: it already is, or a place is free. Notes it. */
	bool TakeDripPlace(const AActor* Slime)
	{
		TArray<TWeakObjectPtr<const AActor>>& Active = Drippers();
		Active.RemoveAll([](const TWeakObjectPtr<const AActor>& Each) { return !Each.IsValid(); });
		if (Active.Contains(TWeakObjectPtr<const AActor>(Slime)))
		{
			return true;
		}
		if (Active.Num() >= DripMost)
		{
			return false;
		}
		Active.Add(TWeakObjectPtr<const AActor>(Slime));
		return true;
	}
}

FVector ASlimeCreature::AirKnockFor(const FVector& ShotWay, float BurstShare, float SizeScale)
{
	const FVector Flat = ShotWay.GetSafeNormal2D();
	if (Flat.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}
	const float Speed = FMath::Min(150.f + 400.f * FMath::Max(BurstShare, 0.f), MaxAirKnock);
	return Flat * (Speed * FMath::Sqrt(FMath::Max(SizeScale, 0.1f)));
}

FVector ASlimeCreature::KnockInAir(const FVector& ShotWay, float BurstShare)
{
	FVector Push = AirKnockFor(ShotWay, BurstShare, GetSizeScale());
	if (Hop != EHop::Air || IsDead() || Push.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	// Its flight so far: a hop launched this frame hasn't been taken up by its movement yet.
	const bool bPending = Movement->Velocity.IsNearlyZero();
	const FVector Flight = bPending ? HopLaunchVelocity : FVector(Movement->Velocity);
	const FVector Here = GetActorLocation();
	const float Scale = GetSizeScale();
	FVector GroundHere;
	if (!FindGround(Here, 0.f, 3000.f * Scale, GroundHere))
	{
		return FVector::ZeroVector;
	}
	// When it comes down: the rest of its arc over the ground under it.
	const float Gravity = FMath::Max(-static_cast<float>(Movement->GetGravityZ()), 1.f);
	const float Height = FMath::Max(static_cast<float>(Here.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - GroundHere.Z), 0.f);
	const float Rise = static_cast<float>(Flight.Z);
	const float AirTime = (Rise + FMath::Sqrt(Rise * Rise + 2.f * Gravity * Height)) / Gravity;
	// The hop's own ground rules (Launch): it lands on ground, not past a ledge. Halve the push until it does, else none.
	for (int32 Try = 0; Try < 4; ++Try)
	{
		const FVector Land = Here + FVector((Flight.X + Push.X) * AirTime, (Flight.Y + Push.Y) * AirTime, 0.0);
		FVector Ground;
		if (FindGround(Land, 300.f * Scale, 600.f * Scale, Ground) && FMath::Abs(Ground.Z - GroundHere.Z) <= KnockLedge * Scale)
		{
			if (bPending)
			{
				HopLaunchVelocity += Push;
				LaunchCharacter(HopLaunchVelocity, true, true);
			}
			else
			{
				Movement->Velocity += Push;
			}
			// Its progress along its hop (Landed's blocked-hop count) is measured from where the push alone would have taken it.
			LaunchedFrom += Push * AirTime;
			return Push;
		}
		Push *= 0.5f;
	}
	return FVector::ZeroVector;
}

void ASlimeCreature::HopNow(const FVector& Direction)
{
	const float Scale = GetSizeScale();
	HopDirection = Direction.GetSafeNormal2D();
	if (HopDirection.IsNearlyZero())
	{
		HopDirection = FVector::ForwardVector;
	}
	bShuffleHop = false;
	Hop = EHop::Air;
	HopTime = 0.f;
	LaunchedFrom = GetActorLocation();
	PlannedLength = 200.f * Scale;
	HopLaunchVelocity = HopVelocity(HopDirection, PlannedLength, 70.f * Scale, -GetCharacterMovement()->GetGravityZ());
	LaunchCharacter(HopLaunchVelocity, true, true);
	Squash.Ramp(1.3f, 0.08f);
}

float ASlimeCreature::HitHeightShare(const FVector& HitLocation) const
{
	const FBoxSphereBounds& Bounds = GetMesh()->Bounds;
	const float Tall = static_cast<float>(Bounds.BoxExtent.Z) * 2.f;
	if (Tall <= 1.f)
	{
		return 0.5f;
	}
	return FMath::Clamp(static_cast<float>(HitLocation.Z - (Bounds.Origin.Z - Bounds.BoxExtent.Z)) / Tall, 0.f, 1.f);
}

ASlimeCreature::EDeathStyle ASlimeCreature::DeathStyleFor(const FCreatureHit& Hit)
{
	switch (CreatureKillLook::StyleOf(Hit))
	{
	case CreatureKillLook::EStyle::Overkill: return EDeathStyle::Overkill;
	case CreatureKillLook::EStyle::Blast: return EDeathStyle::Splattered;
	case CreatureKillLook::EStyle::Crit: return EDeathStyle::Popped;
	default: return EDeathStyle::Body;
	}
}

float ASlimeCreature::GetStaggerShare() const
{
	return !IsDead() && FHitFeelTuning::Live().StaggerPose ? FMath::Clamp(GetStaggerAlpha(), 0.f, 1.f) : 0.f;
}

void ASlimeCreature::ReleaseOverkill()
{
	bOverkillHeld = false;
	CreatureKillLook::Play(*this, OverkillHit, /*bSplat*/ true);
}

void ASlimeCreature::TickWounds(float DeltaSeconds)
{
	// An overkill's gel is held swollen through its hit-stop (its own clock all but stops), then bursts, its layers and splat with it.
	if (bOverkillHeld && IsDead())
	{
		const bool bStopped = HitReaction && HitReaction->IsHitStopped();
		if (!bStopped || GetStateTime() >= OverkillWaitMost)
		{
			ReleaseOverkill();
		}
	}
	if (IsDead())
	{
		return;
	}
	const FHitFeelTuning Feel = FHitFeelTuning::Live();
	const float Max = Health->GetMaxHealth();
	const float Share = Max > 0.f ? FMath::Clamp(Health->GetHealth() / Max, 0.f, 1.f) : 1.f;
	const bool bWounds = Feel.Wounds && Feel.SlimeScale > 0.f;

	// The wounded gel shrinks as its health goes (its hulls follow: the whole mesh is scaled about the ground it stands on).
	const float Goal = bWounds ? FMath::Lerp(WoundShrinkMost, 1.f, Share) : 1.f;
	WoundScale = DeltaSeconds > 0.f ? FMath::FInterpTo(WoundScale, Goal, DeltaSeconds, WoundShrinkRate) : Goal;
	if (FMath::Abs(GetMesh()->GetRelativeScale3D().X - WoundScale) > 0.002)
	{
		GetMesh()->SetRelativeScale3D(FVector(WoundScale));
	}

	// Staggered: a wet wobble once, its core sunk (the squash's flatten and quiver are ReactSquash's).
	const float Stagger = GetStaggerShare();
	if (Stagger > 0.f)
	{
		if (!bStaggerCued)
		{
			bStaggerCued = true;
			Voice->Play(FName(CreatureHitCue::SlimeStagger));
		}
		CoreOffset.Z = FMath::Min(CoreOffset.Z, static_cast<double>(-StaggerCoreSink * Stagger));
		CoreVelocity.Z = FMath::Min(CoreVelocity.Z, 0.0);
	}
	else
	{
		bStaggerCued = false;
	}

	// Under a quarter of its health it drips gel, one bit about every 0.8 s, only near a viewer and at most six slimes at once.
	if (!bWounds || WoundStage(Share) < 2 || IsPoseFrozen() || DeltaSeconds <= 0.f)
	{
		Drippers().Remove(TWeakObjectPtr<const AActor>(this));
		return;
	}
	DripClock -= DeltaSeconds;
	if (DripClock > 0.f)
	{
		return;
	}
	DripClock = DripEvery * FMath::FRandRange(0.8f, 1.2f);
	UWorld* World = GetWorld();
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this);
	UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
	const float Scale = GetSizeScale();
	const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
	const FVector Drip = GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (40.f * Scale) + FVector(0.0, 0.0, 10.0 * Scale);
	if (!Bullets || (Reacts && !Reacts->IsNearAViewer(Drip, DripSeenWithin)) || !TakeDripPlace(this))
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(World);
	Effects.SpawnGooDrip(Drip, CreatureBlood::TintOf(this), 3.f * Scale);
}
