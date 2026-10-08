// AGravemotherCreature's charge (Docs/Areas/RansomsRest.md, "Enemies by rank": "a charge with a long telegraph and a ground
// crack"). It runs as her attack does (ACreatureBase's wind-up, strike and recovery), with its own timing: the wind-up is
// the telegraph (she rears and tracks her target, then holds her aim while the ground cracks open along her line), the
// strike starts the dash down the crack, and the recovery is the dash and her slam at its end. Her bite keeps its own.

#include "Creatures/GravemotherCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GroundCrack.h"
#include "Weapons/WeaponFX.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** Her way to her target is looked at this often while she could charge (s): sweeps and traces, not every frame. */
	constexpr float LaneCheckEvery = 0.25f;

	/** The lane's ground is looked at every this far (cm, at a spider's size): a step she can walk up or down. */
	constexpr float LaneStep = 100.f;

	/** Ground no steeper than this (the creatures' walkable floor angle, about 50 degrees) doesn't stop a charge. */
	constexpr double WalkableNormalZ = 0.65;

	/** A dash runs at most this long past what its length takes (s): against something the lane missed it gives up. */
	constexpr float DashGrace = 0.3f;

	/** A dash going slower than this share of its speed after this long (s) has run into something: she slams down there. */
	constexpr float BlockedShare = 0.25f;
	constexpr float BlockedAfter = 0.25f;

	/** The crack opens a little ahead of the wind-up's end, so it lies open all the way before she comes. */
	constexpr float CrackOpensAhead = 1.15f;

	/** After the charge the crack lies open this long, then closes (s). */
	constexpr float CrackLingers = 2.5f;

	/** Dust kicked up behind her as she runs, this often (s). */
	constexpr float TrailEvery = 0.18f;
}

// ---------------------------------------------------------------------------
// When she charges
// ---------------------------------------------------------------------------

void AGravemotherCreature::ReadyCharge()
{
	ChargeCooldownLeft = 0.f;
	LaneCheckedAt = -1.f;
}

bool AGravemotherCreature::WantsToCharge() const
{
	const APawn* Victim = GetTarget();
	if (ChargeState != EGravemotherCharge::None || ChargeCooldownLeft > 0.f || !Victim || IsDead())
	{
		return false;
	}
	// Far enough off for a charge to matter, near enough to reach, and nothing in the way.
	const float Scale = GetSizeScale();
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	return Distance >= ChargeMinDistance * Scale && Distance <= ChargeRange * Scale && IsLaneClear();
}

bool AGravemotherCreature::IsLaneClear() const
{
	if (LaneCheckedAt >= 0.f && ChargeClock - LaneCheckedAt < LaneCheckEvery)
	{
		return bLaneClear;
	}
	LaneCheckedAt = ChargeClock;
	const APawn* Victim = GetTarget();
	if (!Victim)
	{
		bLaneClear = false;
		return false;
	}
	// Clear when a charge would reach them: no wall, drop or rise in between, short of their side.
	FVector To = Victim->GetActorLocation() - GetActorLocation();
	To.Z = 0.0;
	const float Distance = static_cast<float>(To.Size());
	const float Short = GetCapsuleComponent()->GetScaledCapsuleRadius();
	bLaneClear = Distance > 1.f && MeasureLane(To / Distance, Distance) >= Distance - Short;
	return bLaneClear;
}

float AGravemotherCreature::MeasureLane(const FVector& Direction, float Wanted) const
{
	const UWorld* World = GetWorld();
	if (!World || Wanted <= 0.f)
	{
		return 0.f;
	}
	const float Scale = GetSizeScale();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const FVector Start = GetActorLocation();
	float Length = Wanted;

	// A wall, or rock too steep to walk up, ends it short of where her body would meet it. What she runs down (her target)
	// and her own brood don't; another creature does.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GravemotherCharge), false, this);
	if (const APawn* Victim = GetTarget())
	{
		Params.AddIgnoredActor(Victim);
	}
	for (const TWeakObjectPtr<ASpiderCreature>& Each : Brood)
	{
		if (const ASpiderCreature* Spiderling = Each.Get())
		{
			Params.AddIgnoredActor(Spiderling);
		}
	}
	FHitResult Hit;
	const FCollisionShape Body = FCollisionShape::MakeSphere(Capsule->GetScaledCapsuleRadius() * 0.8f);
	if (World->SweepSingleByChannel(Hit, Start, Start + Direction * Length, FQuat::Identity, ECC_Pawn, Body, Params)
		&& Hit.ImpactNormal.Z < WalkableNormalZ)
	{
		Length = FMath::Max(static_cast<float>(Hit.Distance) - Capsule->GetScaledCapsuleRadius() * 0.2f, 0.f);
	}

	// The ground all along it, on her level: it stops short of a drop, or of a rise she couldn't walk up. With no ground
	// under her at all (a test level) it's flat.
	const FVector Feet = GetFeet();
	FVector Under;
	if (!FindGround(Feet, 50.f * Scale, 100.f * Scale, Under))
	{
		return Length;
	}
	const float Step = LaneStep * Scale;
	double LastZ = Under.Z;
	float Reached = 0.f;
	while (Reached < Length)
	{
		const float At = FMath::Min(Reached + Step, Length);
		const FVector Point(Feet.X + Direction.X * At, Feet.Y + Direction.Y * At, LastZ);
		FVector Ground;
		if (!FindGround(Point, Step * 0.9f, Step * 0.9f, Ground))
		{
			return Reached;
		}
		LastZ = Ground.Z;
		Reached = At;
	}
	return Length;
}

float AGravemotherCreature::GetAttackStartRange() const
{
	// Her charge starts from well out; otherwise she closes in to bite.
	return WantsToCharge() ? ChargeRange * GetSizeScale() : Super::GetAttackStartRange();
}

bool AGravemotherCreature::CanStartAttack() const
{
	return ChargeState == EGravemotherCharge::None && Super::CanStartAttack();
}

bool AGravemotherCreature::TracksTargetInWindup() const
{
	// Once she has taken her aim the line is fixed: the crack shows where she'll run, and she runs there.
	return !(ChargeState == EGravemotherCharge::Telegraph && bAimTaken);
}

// ---------------------------------------------------------------------------
// The telegraph
// ---------------------------------------------------------------------------

void AGravemotherCreature::OnAttackStarted()
{
	// The same attack, two ways: a charge when one is wanted, else her bite with its own timing.
	if (WantsToCharge())
	{
		BeginTelegraph();
	}
	else
	{
		AttackWindup = BiteWindup;
		AttackRecovery = BiteRecovery;
	}
	Super::OnAttackStarted();
}

void AGravemotherCreature::BeginTelegraph()
{
	ChargeState = EGravemotherCharge::Telegraph;
	bAimTaken = false;
	bChargeHit = false;
	ChargeLength = 0.f;
	// The attack's wind-up is the whole telegraph (the spider's attack pose rears her up through it, forelegs high); its
	// recovery is set for this dash once she knows how long it runs (BeginDash), long enough for the longest till then.
	AttackWindup = ChargeTelegraph;
	AttackRecovery = (ChargeRange + ChargeOvershoot) / FMath::Max(ChargeSpeed, 1.f) + DashGrace + ChargeRecover;
	// She digs her back legs in as she rears.
	KickDirt(GetFeet() - GetActorForwardVector() * GetCapsuleComponent()->GetScaledCapsuleRadius(), 0.4f);
	UE_LOG(LogLooter, Verbose, TEXT("%s rears to charge %s."), *GetName(), *GetNameSafe(GetTarget()));
}

void AGravemotherCreature::TakeAim()
{
	bAimTaken = true;
	const float Scale = GetSizeScale();
	const APawn* Victim = GetTarget();
	FVector To = Victim ? Victim->GetActorLocation() - GetActorLocation() : GetActorForwardVector() * (ChargeRange * Scale);
	To.Z = 0.0;
	ChargeDirection = To.IsNearlyZero() ? GetActorForwardVector().GetSafeNormal2D() : To.GetSafeNormal();
	// Squared up to her line.
	SetActorRotation(FRotator(0.f, static_cast<float>(ChargeDirection.Rotation().Yaw), 0.f));
	const float Wanted = FMath::Clamp(static_cast<float>(To.Size()) + ChargeOvershoot * Scale, ChargeMinDistance * Scale,
		(ChargeRange + ChargeOvershoot) * Scale);
	ChargeStart = GetActorLocation();
	ChargeLength = MeasureLane(ChargeDirection, Wanted);

	// The ground cracks open along it, from under her forelegs to where she'll stop, glowing with her soul-light: the
	// crack is the line she runs.
	const FVector Feet = GetFeet();
	const float Front = GetCapsuleComponent()->GetScaledCapsuleRadius();
	if (AGroundCrack* Old = Crack.Get())
	{
		Old->Close(0.f);
	}
	Crack = AGroundCrack::Spawn(GetWorld(), Feet + ChargeDirection * Front, Feet + ChargeDirection * (ChargeLength + Front),
		ChargeCrackWidth * Scale, UCreatureRankSettings::Get(GetRank()).Color, FMath::Rand());
	KickDirt(Feet + ChargeDirection * Front, 0.6f);
	UE_LOG(LogLooter, Verbose, TEXT("%s takes aim: %.1f m toward yaw %.0f."), *GetName(), ChargeLength / 100.f,
		ChargeDirection.Rotation().Yaw);
}

// ---------------------------------------------------------------------------
// The dash
// ---------------------------------------------------------------------------

void AGravemotherCreature::Strike()
{
	if (ChargeState != EGravemotherCharge::Telegraph)
	{
		// A bite.
		Super::Strike();
		return;
	}
	// A wind-up shorter than her aim's time (its settings) aims as it ends.
	if (!bAimTaken)
	{
		TakeAim();
	}
	BeginDash();
}

void AGravemotherCreature::BeginDash()
{
	ChargeState = EGravemotherCharge::Dash;
	DashTime = 0.f;
	DashLast = GetActorLocation();
	TrailIn = 0.f;
	const float Speed = ChargeSpeed * GetSizeScale();
	DashSeconds = ChargeLength / FMath::Max(Speed, 1.f);
	// The attack lasts this dash and the slam's recovery after it.
	AttackRecovery = DashSeconds + DashGrace + ChargeRecover;
	if (AGroundCrack* Open = Crack.Get())
	{
		Open->SetOpen(1.f);
	}
	// She screams down the crack: the rear's cry was the warning, this is the charge.
	Voice->Play(LooterSoundCue::SpiderAlert);
	if (ChargeLength <= 1.f)
	{
		// Nowhere to run: she slams down where she stands.
		EndDash();
	}
}

void AGravemotherCreature::TickCharge(float DeltaSeconds)
{
	const bool bAttacking = GetCreatureState() == ECreatureState::Attack;
	switch (ChargeState)
	{
	case EGravemotherCharge::None:
		// Its cooldown runs while she's after someone, not while she dozes in her den.
		if (GetTarget() && ChargeCooldownLeft > 0.f)
		{
			ChargeCooldownLeft = FMath::Max(0.f, ChargeCooldownLeft - DeltaSeconds);
		}
		break;

	case EGravemotherCharge::Telegraph:
		if (!bAttacking)
		{
			// Cut short (a fight starting over).
			FinishCharge();
			break;
		}
		if (!bAimTaken && GetStateTime() >= ChargeAimSeconds)
		{
			TakeAim();
		}
		if (AGroundCrack* Open = bAimTaken ? Crack.Get() : nullptr)
		{
			const float CrackTime = FMath::Max(ChargeTelegraph - ChargeAimSeconds, 0.05f);
			Open->SetOpen((GetStateTime() - ChargeAimSeconds) / CrackTime * CrackOpensAhead);
		}
		break;

	case EGravemotherCharge::Dash:
		TickDash(DeltaSeconds);
		break;

	case EGravemotherCharge::Recover:
		if (!bAttacking)
		{
			FinishCharge();
		}
		break;
	}
}

void AGravemotherCreature::TickDash(float DeltaSeconds)
{
	if (GetCreatureState() != ECreatureState::Attack)
	{
		GetCharacterMovement()->StopMovementImmediately();
		FinishCharge();
		return;
	}
	DashTime += DeltaSeconds;
	const FVector Here = GetActorLocation();
	TryChargeHit(DashLast, Here);
	DashLast = Here;

	const float Speed = ChargeSpeed * GetSizeScale();
	const FVector Run = Here - ChargeStart;
	const float Covered = static_cast<float>(Run.X * ChargeDirection.X + Run.Y * ChargeDirection.Y);
	const bool bThere = Covered >= ChargeLength - 2.f;
	const bool bOutOfTime = DashTime >= DashSeconds + DashGrace;
	// Stopped by something the lane didn't see (a creature, a rock's corner): she slams down where she is.
	const bool bBlocked = DashTime > BlockedAfter && GetVelocity().Size2D() < Speed * BlockedShare;
	if (bThere || bOutOfTime || bBlocked)
	{
		EndDash();
		return;
	}

	// Run, not walked: full speed at once down the line (the movement keeps her on the ground and off the walls).
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = Speed;
	Movement->Velocity = ChargeDirection * Speed + FVector(0.0, 0.0, Movement->Velocity.Z);
	AddMovementInput(ChargeDirection);
	SetActorRotation(FRotator(0.f, static_cast<float>(ChargeDirection.Rotation().Yaw), 0.f));

	TrailIn -= DeltaSeconds;
	if (TrailIn <= 0.f)
	{
		TrailIn = TrailEvery;
		KickDirt(GetFeet() - ChargeDirection * (GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.5f), 0.3f);
	}
}

bool AGravemotherCreature::TryChargeHit(const FVector& From, const FVector& To)
{
	APawn* Victim = GetTarget();
	if (bChargeHit || !IsValidTarget(Victim))
	{
		return false;
	}
	// Her body's path from the last frame to this one, against theirs: a long frame can't step her over them.
	const FVector Where = Victim->GetActorLocation();
	const ACharacter* VictimBody = Cast<ACharacter>(Victim);
	const float VictimRadius = VictimBody ? VictimBody->GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f;
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Reach = Capsule->GetScaledCapsuleRadius() + VictimRadius + ChargeHitMargin * GetSizeScale();
	const FVector2D Flat(Where.X, Where.Y);
	const FVector2D Nearest = FMath::ClosestPointOnSegment2D(Flat, FVector2D(From.X, From.Y), FVector2D(To.X, To.Y));
	// Up and down: run down on her level; one up on a rock over her head is spared.
	const bool bLevel = FMath::Abs(Where.Z - To.Z) <= Capsule->GetScaledCapsuleHalfHeight() + 150.f;
	if (!bLevel || FVector2D::Distance(Flat, Nearest) > Reach)
	{
		return false;
	}
	bChargeHit = true;
	// Thrown off her line to whichever side they stood, and on along it.
	const FVector Side(-ChargeDirection.Y, ChargeDirection.X, 0.0);
	const double Way = FVector::DotProduct(Where - To, Side) >= 0.0 ? 1.0 : -1.0;
	HitWithAttack(Victim, (ChargeDirection * 0.6 + Side * (Way * 0.8)).GetSafeNormal(), ChargeStrength);
	UE_LOG(LogLooter, Log, TEXT("%s's charge ran down %s."), *GetName(), *GetNameSafe(Victim));
	return true;
}

void AGravemotherCreature::EndDash()
{
	ChargeState = EGravemotherCharge::Recover;
	GetCharacterMovement()->StopMovementImmediately();
	TryChargeHit(DashLast, GetActorLocation());
	// Her forelegs slam down at the crack's end (the spider's strike pose), and the ground bursts round them.
	const float Scale = GetSizeScale();
	const FVector Slam = GetFeet() + ChargeDirection * (GetCapsuleComponent()->GetScaledCapsuleRadius() * 1.2f);
	if (AGroundCrack* Open = Crack.Get())
	{
		Open->SetOpen(1.f);
		Open->Burst(Slam, ChargeBurstRadius * Scale);
	}
	KickDirt(Slam, 1.f);
	const FVector Side(-ChargeDirection.Y, ChargeDirection.X, 0.0);
	KickDirt(Slam + Side * (ChargeBurstRadius * Scale * 0.5f), 0.6f);
	KickDirt(Slam - Side * (ChargeBurstRadius * Scale * 0.5f), 0.6f);
}

void AGravemotherCreature::FinishCharge()
{
	const bool bCharged = ChargeState != EGravemotherCharge::None;
	ChargeState = EGravemotherCharge::None;
	bAimTaken = false;
	if (AGroundCrack* Open = Crack.Get())
	{
		Open->Close(CrackLingers);
	}
	Crack = nullptr;
	if (bCharged)
	{
		ChargeCooldownLeft = ChargeCooldown;
	}
	// Her next attack is a bite until the charge is ready again.
	AttackWindup = BiteWindup;
	AttackRecovery = BiteRecovery;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

FVector AGravemotherCreature::GetFeet() const
{
	return GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
}

void AGravemotherCreature::KickDirt(const FVector& Where, float Strength)
{
	UWorld* World = GetWorld();
	if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
	{
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnDirt(Where, FVector::UpVector, Strength);
	}
}
