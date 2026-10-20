// AGravemotherCreature's Furrow (the user's call, 2026-10-19; her old charge upgraded): it runs as her attack does
// (ACreatureBase's wind-up, strike and recovery) with its own timing. The wind-up is the heave, the user's 2 s tell: she digs
// in head-low and tracks her target until AimAt, then holds her aim while the ground heaves and splits along her line
// toward them (AGravemotherFurrow), all of it open by CrawlEnd; the strike starts the plough straight down the line, berms
// rising each side behind her; the recovery is the plough and her slam at its end. Never off a drop or through a wall. Her
// bite keeps its own timing.

#include "Creatures/GravemotherCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Creatures/GravemotherFurrow.h"
#include "Creatures/GravemotherFX.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** Her way to her target is looked at this often while she could plough (s): sweeps and traces, not every frame. */
	constexpr float LaneCheckEvery = 0.25f;
	/** The lane's ground is looked at every this far (cm, at a spider's size): a step she can walk up or down. */
	constexpr float LaneStep = 100.f;
	/** Ground no steeper than this (the creatures' walkable floor angle, about 50 degrees) doesn't stop a furrow. */
	constexpr double WalkableNormalZ = 0.65;
	/** A plough runs at most this long past what its length takes (s): against something the lane missed it gives up. */
	constexpr float PloughGrace = 0.3f;
	/** A plough going slower than this share of its speed after this long (s) has run into something: she slams down there. */
	constexpr float BlockedShare = 0.25f;
	constexpr float BlockedAfter = 0.25f;
	/** Her plough shakes a player near her (within ShakeWithin cm) this often (s). */
	constexpr float ShakeEvery = 0.3f;
	constexpr float ShakeWithin = 900.f;
	/** Her front legs tear at the floor through the heave: dirt at them this often (s). */
	constexpr float HeaveDirtEvery = 0.2f;
}

// ---------------------------------------------------------------------------
// When she ploughs
// ---------------------------------------------------------------------------

void AGravemotherCreature::ReadyFurrow()
{
	FurrowCooldownLeft = 0.f;
	LaneCheckedAt = -1.f;
}

float AGravemotherCreature::GetPloughSpeed() const
{
	return Furrows.Speed * GetSizeScale() * Pace.SpeedScale;
}

bool AGravemotherCreature::WantsToFurrow() const
{
	const APawn* Victim = GetTarget();
	if (FurrowState != EGravemotherFurrow::None || FurrowCooldownLeft > 0.f || !Victim || IsDead())
	{
		return false;
	}
	// Far enough off for a furrow to matter, near enough to reach, and nothing in the way.
	const float Scale = GetSizeScale();
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	return Distance >= Furrows.MinDistance * Scale && Distance <= Furrows.Range * Scale && IsLaneClear();
}

bool AGravemotherCreature::IsLaneClear() const
{
	if (LaneCheckedAt >= 0.f && Clock - LaneCheckedAt < LaneCheckEvery)
	{
		return bLaneClear;
	}
	LaneCheckedAt = Clock;
	const APawn* Victim = GetTarget();
	if (!Victim)
	{
		bLaneClear = false;
		return false;
	}
	// Clear when a plough would reach them: no wall, drop, rise, berm or rubble in between, short of their side.
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
	const FVector From = GetActorLocation();
	float Length = Wanted;

	// A wall, rock too steep to walk up, a berm or rubble ends it short of where her body would meet it. What she runs down
	// (her target) and her own brood don't; another creature does.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GravemotherFurrow), false, this);
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
	if (World->SweepSingleByChannel(Hit, From, From + Direction * Length, FQuat::Identity, ECC_Pawn, Body, Params)
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

bool AGravemotherCreature::TracksTargetInWindup() const
{
	// Once she has taken her aim the line is fixed: the split shows where she'll run, and she runs there.
	return !(FurrowState == EGravemotherFurrow::Heave && bAimTaken);
}

// ---------------------------------------------------------------------------
// The heave (her attack's choice of move is GravemotherCreatureMoves.cpp's)
// ---------------------------------------------------------------------------

void AGravemotherCreature::BeginFurrow()
{
	Move = EGravemotherMove::Furrow;
	FurrowState = EGravemotherFurrow::Heave;
	bAimTaken = false;
	bFurrowHit = false;
	bPloughRan = false;
	FurrowLength = 0.f;
	HeaveFront = 0.f;
	// The attack's wind-up is the whole tell (the user's 2 s); its recovery is set for this plough once she knows how long it
	// runs (BeginPlough), long enough for the longest till then.
	AttackWindup = Furrows.Tell;
	AttackRecovery = (Furrows.Range + Furrows.Overshoot) * GetSizeScale() / FMath::Max(GetPloughSpeed(), 1.f) + PloughGrace + Furrows.Recover;
	// Head low, her front legs dig into the floor (GravemotherCreaturePose.cpp), and the ground starts to heave.
	KickDirt(GetFeet() + GetActorForwardVector() * GetCapsuleComponent()->GetScaledCapsuleRadius(), 0.5f);
	HeaveSound = LooterSound::Start(this, LooterSoundCue::GravemotherHeave, GetRootComponent());
	UE_LOG(LogLooter, Verbose, TEXT("%s digs in to plough at %s."), *GetName(), *GetNameSafe(GetTarget()));
}

void AGravemotherCreature::TakeAim()
{
	bAimTaken = true;
	const float Scale = GetSizeScale();
	const APawn* Victim = GetTarget();
	FVector To = Victim ? Victim->GetActorLocation() - GetActorLocation() : GetActorForwardVector() * (Furrows.Range * Scale);
	To.Z = 0.0;
	FurrowDirection = To.IsNearlyZero() ? GetActorForwardVector().GetSafeNormal2D() : To.GetSafeNormal();
	// Squared up to her line.
	SetActorRotation(FRotator(0.f, static_cast<float>(FurrowDirection.Rotation().Yaw), 0.f));
	const float Wanted = FMath::Clamp(static_cast<float>(To.Size()) + Furrows.Overshoot * Scale, Furrows.MinDistance * Scale,
		(Furrows.Range + Furrows.Overshoot) * Scale);
	FurrowStart = GetActorLocation();
	FurrowLength = MeasureLane(FurrowDirection, Wanted);

	// The ground heaves along it, from under her forelegs to where she'll stop: the split is the line she runs.
	const FVector Feet = GetFeet();
	const float Front = GetCapsuleComponent()->GetScaledCapsuleRadius();
	if (AGravemotherFurrow* Old = Furrow.Get(); Old && Old->IsCancelled() == false && !Old->IsPloughed())
	{
		Old->Cancel();
	}
	Furrow = AGravemotherFurrow::Spawn(GetWorld(), Feet + FurrowDirection * Front, FurrowDirection, FMath::Max(FurrowLength, 1.f), Furrows);
	KickDirt(Feet + FurrowDirection * Front, 0.7f);
	UE_LOG(LogLooter, Verbose, TEXT("%s takes aim: %.1f m toward yaw %.0f."), *GetName(), FurrowLength / 100.f, FurrowDirection.Rotation().Yaw);
}

// ---------------------------------------------------------------------------
// The plough (her attack's strike is GravemotherCreatureMoves.cpp's: a furrow's starts this)
// ---------------------------------------------------------------------------

void AGravemotherCreature::BeginPlough()
{
	// A wind-up shorter than her aim's time (its settings) aims as it ends.
	if (!bAimTaken)
	{
		TakeAim();
	}
	FurrowState = EGravemotherFurrow::Plough;
	bPloughRan = true;
	bPloughSetOff = false;
	PloughTime = 0.f;
	PloughLast = GetActorLocation();
	ShakeIn = 0.f;
	PloughSeconds = FurrowLength / FMath::Max(GetPloughSpeed(), 1.f);
	// The attack lasts this plough and the slam's recovery after it.
	AttackRecovery = PloughSeconds + PloughGrace + Furrows.Recover;
	if (AGravemotherFurrow* Ground = Furrow.Get())
	{
		Ground->BeginPlough();
	}
	LooterSound::Stop(HeaveSound.Get(), 0.1f);
	HeaveSound = nullptr;
	PloughSound = LooterSound::Start(this, LooterSoundCue::GravemotherPlough, GetRootComponent());
	// She screams down the line: the heave was the warning, this is the plough.
	Voice->Play(LooterSoundCue::SpiderAlert);
	UE_LOG(LogLooter, Verbose, TEXT("%s ploughs %.1f m at %.0f cm/s (%.2f s)."), *GetName(), FurrowLength / 100.f, GetPloughSpeed(), PloughSeconds);
	if (FurrowLength <= 1.f)
	{
		// Nowhere to run: she slams down where she stands.
		EndPlough();
	}
}

void AGravemotherCreature::TickFurrow(float DeltaSeconds)
{
	const bool bAttacking = GetCreatureState() == ECreatureState::Attack;
	switch (FurrowState)
	{
	case EGravemotherFurrow::None:
		// Its cooldown runs while she's after someone, not while she dozes in her den.
		if (GetTarget() && FurrowCooldownLeft > 0.f)
		{
			FurrowCooldownLeft = FMath::Max(0.f, FurrowCooldownLeft - DeltaSeconds);
		}
		break;

	case EGravemotherFurrow::Heave:
	{
		if (!bAttacking)
		{
			// Cut short (a stagger, a fight starting over).
			FinishFurrow();
			break;
		}
		const float Time = GetStateTime();
		if (!bAimTaken && Time >= Furrows.AimAt)
		{
			TakeAim();
		}
		if (AGravemotherFurrow* Ground = bAimTaken ? Furrow.Get() : nullptr)
		{
			HeaveFront = GravemotherFight::HeaveFront(Time, Furrows.AimAt, Furrows.CrawlEnd, FurrowLength);
			Ground->SetHeave(HeaveFront);
		}
		// Her front legs tear at the floor.
		DigDirtIn -= DeltaSeconds;
		if (DigDirtIn <= 0.f)
		{
			DigDirtIn = HeaveDirtEvery;
			const float Front = GetCapsuleComponent()->GetScaledCapsuleRadius() * 1.3f;
			KickDirt(GetFeet() + GetActorForwardVector() * Front, 0.25f);
		}
		break;
	}

	case EGravemotherFurrow::Plough:
		TickPlough(DeltaSeconds);
		break;

	case EGravemotherFurrow::Recover:
		if (!bAttacking)
		{
			FinishFurrow();
		}
		break;
	}
}

void AGravemotherCreature::TickPlough(float DeltaSeconds)
{
	if (GetCreatureState() != ECreatureState::Attack)
	{
		GetCharacterMovement()->StopMovementImmediately();
		FinishFurrow();
		return;
	}
	// Her first frame only sets her going: she stood still through the heave, and the movement runs her line from the next. Its
	// time isn't plough time, so a long frame there (a hitch) neither times the plough out nor reads as running into something
	// (it did: one 0.4 s frame and she slammed down where she stood).
	if (bPloughSetOff)
	{
		PloughTime += DeltaSeconds;
	}
	bPloughSetOff = true;
	const FVector Here = GetActorLocation();
	TryFurrowHit(PloughLast, Here);
	PloughLast = Here;

	const float Speed = GetPloughSpeed();
	const FVector Run = Here - FurrowStart;
	const float Covered = static_cast<float>(Run.X * FurrowDirection.X + Run.Y * FurrowDirection.Y);
	// The berms rise and the strip is laid as she passes (the furrow's line starts at her front as she took aim).
	if (AGravemotherFurrow* Ground = Furrow.Get())
	{
		Ground->SetPlough(Covered - GetCapsuleComponent()->GetScaledCapsuleRadius());
	}
	const bool bThere = Covered >= FurrowLength - 2.f;
	const bool bOutOfTime = PloughTime >= PloughSeconds + PloughGrace;
	// Stopped by something the lane didn't see (a creature, a rock's corner): she slams down where she is.
	const bool bBlocked = PloughTime > BlockedAfter && GetVelocity().Size2D() < Speed * BlockedShare;
	if (bThere || bOutOfTime || bBlocked)
	{
		UE_LOG(LogLooter, Verbose, TEXT("%s's plough ends %s after %.2f s, %.1f of %.1f m."), *GetName(),
			bThere ? TEXT("there") : (bOutOfTime ? TEXT("out of time") : TEXT("blocked")), PloughTime, Covered / 100.f, FurrowLength / 100.f);
		EndPlough();
		return;
	}

	// Ploughed, not walked: full speed at once down the line (the movement keeps her on the ground and off the walls).
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = Speed;
	Movement->Velocity = FurrowDirection * Speed + FVector(0.0, 0.0, Movement->Velocity.Z);
	AddMovementInput(FurrowDirection);
	SetActorRotation(FRotator(0.f, static_cast<float>(FurrowDirection.Rotation().Yaw), 0.f));

	// The ground shudders under a player near her.
	ShakeIn -= DeltaSeconds;
	const APawn* Victim = GetTarget();
	if (ShakeIn <= 0.f && Victim && FVector::Dist2D(Victim->GetActorLocation(), Here) <= ShakeWithin * GetSizeScale() * 0.6f)
	{
		ShakeIn = ShakeEvery;
		ShakeFrom(Here, 0.2f, 0.35f);
	}
}

bool AGravemotherCreature::TryFurrowHit(const FVector& From, const FVector& To)
{
	APawn* Victim = GetTarget();
	if (bFurrowHit || !IsValidTarget(Victim))
	{
		return false;
	}
	// Her body's path from the last frame to this one, against theirs: a long frame can't step her over them.
	const FVector Where = Victim->GetActorLocation();
	const ACharacter* VictimBody = Cast<ACharacter>(Victim);
	const float VictimRadius = VictimBody ? VictimBody->GetCapsuleComponent()->GetScaledCapsuleRadius() : 40.f;
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float Reach = Capsule->GetScaledCapsuleRadius() + VictimRadius + Furrows.HitMargin * GetSizeScale();
	const FVector2D Flat(Where.X, Where.Y);
	const FVector2D Nearest = FMath::ClosestPointOnSegment2D(Flat, FVector2D(From.X, From.Y), FVector2D(To.X, To.Y));
	// Up and down: run down on her level; one up on a rock over her head is spared.
	const bool bLevel = FMath::Abs(Where.Z - To.Z) <= Capsule->GetScaledCapsuleHalfHeight() + 150.f;
	if (!bLevel || FVector2D::Distance(Flat, Nearest) > Reach)
	{
		return false;
	}
	bFurrowHit = true;
	// Thrown off her line to whichever side they stood, and on along it; the player feels it.
	const FVector Side(-FurrowDirection.Y, FurrowDirection.X, 0.0);
	const double Way = FVector::DotProduct(Where - To, Side) >= 0.0 ? 1.0 : -1.0;
	HitWithAttack(Victim, (FurrowDirection * 0.6 + Side * (Way * 0.8)).GetSafeNormal(), Furrows.Strength);
	ShakeFrom(Where, 0.7f, 0.6f);
	UE_LOG(LogLooter, Log, TEXT("%s's plough ran down %s."), *GetName(), *GetNameSafe(Victim));
	return true;
}

void AGravemotherCreature::EndPlough()
{
	FurrowState = EGravemotherFurrow::Recover;
	GetCharacterMovement()->StopMovementImmediately();
	TryFurrowHit(PloughLast, GetActorLocation());
	LooterSound::Stop(PloughSound.Get(), 0.2f);
	PloughSound = nullptr;
	// Her forelegs slam down at the line's end, the ground bursts round them and scars, and the berms stand their time.
	const float Scale = GetSizeScale();
	const FVector Slam = GetFeet() + FurrowDirection * (GetCapsuleComponent()->GetScaledCapsuleRadius() * 1.2f);
	if (AGravemotherFurrow* Ground = Furrow.Get())
	{
		Ground->EndPlough(Slam, Furrows.BurstRadius * Scale);
	}
	KickDirt(Slam, 1.f);
	const FVector Side(-FurrowDirection.Y, FurrowDirection.X, 0.0);
	KickDirt(Slam + Side * (Furrows.BurstRadius * Scale * 0.5f), 0.6f);
	KickDirt(Slam - Side * (Furrows.BurstRadius * Scale * 0.5f), 0.6f);
	LooterSound::PlayAt(this, LooterSoundCue::GravemotherSlam, Slam);
	ShakeFrom(Slam, 0.4f, 0.5f);
	// Dodged, her forelegs stay in the dirt a while (the window to punish her: she sinks, open); having run her target
	// down, she's up sooner.
	const float SinceStrike = FMath::Max(GetStateTime() - AttackWindup, 0.f);
	AttackRecovery = SinceStrike + (bFurrowHit ? Furrows.Recover : Furrows.MissRecover);
}

void AGravemotherCreature::FinishFurrow()
{
	const bool bRan = FurrowState != EGravemotherFurrow::None;
	UE_LOG(LogLooter, Verbose, TEXT("%s's furrow is over (from %s, %s)."), *GetName(), *UEnum::GetValueAsString(FurrowState),
		*UEnum::GetValueAsString(GetCreatureState()));
	if (AGravemotherFurrow* Ground = Furrow.Get())
	{
		if (!bPloughRan)
		{
			// A tell broken off: the split fades, no berms.
			Ground->Cancel();
		}
		else if (FurrowState == EGravemotherFurrow::Plough)
		{
			// Stopped mid-plough (a stagger, her death): what she passed stands.
			Ground->EndPlough(GetFeet(), Furrows.BurstRadius * GetSizeScale() * 0.6f);
		}
	}
	FurrowState = EGravemotherFurrow::None;
	bAimTaken = false;
	Furrow = nullptr;
	bPloughRan = false;
	LooterSound::Stop(HeaveSound.Get(), 0.1f);
	LooterSound::Stop(PloughSound.Get(), 0.2f);
	HeaveSound = nullptr;
	PloughSound = nullptr;
	if (bRan)
	{
		FurrowCooldownLeft = Furrows.Cooldown;
	}
	// Her next attack is a bite until something else is ready.
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
	UGravemotherFX* Effects = UGravemotherFX::Get(this);
	if (!Effects)
	{
		return;
	}
	// Fresh grave earth thrown up: puffs and clumps, more and farther for a stronger kick.
	const float Amount = FMath::Clamp(Strength, 0.f, 1.f);
	FGraveDustBurst Dirt;
	Dirt.Count = FMath::Max(1, FMath::RoundToInt32(2.f + 8.f * Amount));
	Dirt.Spread = 30.f + 50.f * Amount;
	Dirt.SpreadUp = 15.f;
	Dirt.StartSize = 45.f + 30.f * Amount;
	Dirt.EndSize = 110.f + 90.f * Amount;
	Dirt.Life = 1.f + 0.5f * Amount;
	Dirt.Opacity = 0.3f;
	Dirt.Drift = FVector(0.0, 0.0, 40.0);
	Dirt.Outward = 80.f + 160.f * Amount;
	Effects->Puffs(Where, Dirt);
	Effects->Chunks(EGraveChunk::Debris, Where + FVector(0.0, 0.0, 10.0), FVector::UpVector, FMath::RoundToInt32(1.f + 5.f * Amount),
		300.f + 350.f * Amount);
}
