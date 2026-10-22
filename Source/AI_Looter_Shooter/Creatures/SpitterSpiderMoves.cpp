// ASpitterSpider's attacks: which one it makes (its one attack, wound up, struck and recovered, each with its own timing),
// the spit (its tell, the lob's clear way, the globs thrown, a spit choked by a shot to its sac) and the bite it keeps for
// when it's caught or cornered. SpitterSpiderKeepAway.cpp is how it keeps its distance.

#include "Creatures/SpitterSpider.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Creatures/FightTuning.h"
#include "Creatures/SpitterGlob.h"
#include "Weapons/WeaponFX.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	/** The tell's glow grows at its fangs to this many times a glob's radius. */
	constexpr float TellGlowSize = 1.8f;
	/** Where that glow sits on its body (its own frame, at size 1): ahead of and over its fangs, where its rear lifts its head. */
	const FVector TellGlowSpot(80.0, 0.0, 30.0);
	/** Its lob's way is looked at this often (s) while its spit is ready, and up to this share of the flight (the rest comes down onto its target). */
	constexpr float LobCheckEvery = 0.25f;
	constexpr float LobCheckShare = 0.85f;
	constexpr int32 LobCheckSegments = 6;
	/** A choked spit's flinch (s): the window it leaves. */
	constexpr float ChokedRecovery = 0.7f;
	/** Its next spit comes this much sooner or later than its interval says, so a pair of Spitters falls out of step. */
	constexpr float SpitJitter = 0.25f;
	/** A bite reaches this far past where it starts, and up and down this far (at its size, at least size 1). */
	constexpr float BiteOvershoot = 70.f;
	constexpr float BiteHeight = 220.f;

	/** Where a pawn's feet are. */
	FVector FeetOf(const APawn& Pawn)
	{
		const ACharacter* Character = Cast<ACharacter>(&Pawn);
		const float Half = Character ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 88.f;
		return Pawn.GetActorLocation() - FVector(0.0, 0.0, Half);
	}
}

// ---------------------------------------------------------------------------
// Which attack
// ---------------------------------------------------------------------------

float ASpitterSpider::GetAttackStartRange() const
{
	// Caught or cornered, it bites what's in reach; otherwise venom at whoever is in its range, the moment its spit is
	// ready and its lob's way clear. Nothing else starts an attack: it keeps its distance.
	if (WantsToBite())
	{
		return BiteFrom * GetSizeScale();
	}
	if (WantsToSpit())
	{
		return SpitMaxDistance;
	}
	return 0.f;
}

EFightRole ASpitterSpider::GetAttackTokenRole() const
{
	// A bite up close is a melee turn like any spider's; a spit is one of the ranged turns, which keep spitters' shots apart.
	return WantsToBite() ? EFightRole::Melee : EFightRole::Ranged;
}

bool ASpitterSpider::GetFightBand(float& OutNear, float& OutFar) const
{
	OutNear = GetBackOffDistance();
	OutFar = FMath::Max(CloseInTo, OutNear + 100.f);
	return true;
}

float ASpitterSpider::GetBackOffDistance() const
{
	const float FromFight = FFightTuning::LiveRangedBackOff();
	return FromFight > 0.f ? FromFight : BackOffWithin;
}

void ASpitterSpider::OnAttackCalledOff()
{
	// The tell's glow at its fangs goes out with nothing thrown; its spit comes round again from its interval.
	if (UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
	{
		Shots->ClearShotsFrom(this);
	}
	if (Move == ESpitterMove::Spit)
	{
		SpitReadyIn = FMath::Max(SpitReadyIn, GetSpitInterval() * 0.5f);
	}
	AttackWindup = OwnBiteWindup;
	AttackRecovery = OwnBiteRecovery;
}

void ASpitterSpider::OnAttackStarted()
{
	if (WantsToBite())
	{
		BeginBite();
	}
	else
	{
		BeginSpit();
	}
	Super::OnAttackStarted();
}

void ASpitterSpider::Strike()
{
	if (Move == ESpitterMove::Bite)
	{
		BiteNow();
		return;
	}
	// A spit whose sac a shot burst throws nothing.
	if (!bSpitChoked)
	{
		ReleaseSpit();
	}
}

bool ASpitterSpider::WantsToSpit() const
{
	const APawn* Victim = GetTarget();
	if (!Victim || IsDead() || !IsValidTarget(Victim))
	{
		return false;
	}
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	return SpitterRules::CanSpit(Distance, SpitMinDistance * GetSizeScale(), SpitMaxDistance, SpitReadyIn <= 0.f) && IsLobClear(*Victim);
}

bool ASpitterSpider::WantsToBite() const
{
	const APawn* Victim = GetTarget();
	if (!Victim || IsDead() || !IsValidTarget(Victim))
	{
		return false;
	}
	const float Scale = GetSizeScale();
	const FVector To = Victim->GetActorLocation() - GetActorLocation();
	return FMath::Abs(To.Z) < BiteHeight * FMath::Max(Scale, 1.f)
		&& SpitterRules::WantsBite(static_cast<float>(To.Size2D()), BiteFrom * Scale, CaughtWithin * Scale, IsCornered(), BiteReadyIn <= 0.f);
}

bool ASpitterSpider::IsLobClear(const APawn& Victim) const
{
	if (LobCheckedAt >= 0.f && Clock - LobCheckedAt < LobCheckEvery)
	{
		return bLobClear;
	}
	LobCheckedAt = Clock;
	const UWorld* World = GetWorld();
	if (!World)
	{
		bLobClear = false;
		return false;
	}
	// The arc it would throw now, looked along in a few straight pieces up to where it comes down onto its target: a lob
	// clears low cover, but not a wall or a roof. Blocked, it keeps strafing for a clear way (SpitterSpiderKeepAway.cpp).
	const FVector Start = GetMuzzle();
	const FVector LandAt = FeetOf(Victim);
	const float Seconds = SpitterRules::FlightSeconds(static_cast<float>(FVector::Dist2D(Start, LandAt)));
	const FVector Velocity = SpitterRules::LobVelocity(Start, LandAt, Seconds);
	const FCollisionQueryParams Query = ASpitterGlob::MakeWorldQuery(World, this);
	FVector From = Start;
	for (int32 Piece = 1; Piece <= LobCheckSegments; ++Piece)
	{
		const FVector To = SpitterRules::LobPoint(Start, Velocity, Seconds * LobCheckShare * Piece / LobCheckSegments);
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), Query))
		{
			bLobClear = false;
			return false;
		}
		From = To;
	}
	bLobClear = true;
	return true;
}

// ---------------------------------------------------------------------------
// The spit
// ---------------------------------------------------------------------------

void ASpitterSpider::BeginSpit()
{
	// The tell: it rears up (the spider's attack pose through the wind-up), its sac swells and throbs brighter (TickSac), a
	// glow grows at its fangs and it hisses. Time enough to step aside or get behind something.
	Move = ESpitterMove::Spit;
	bSpitChoked = false;
	AttackWindup = SpitTell;
	AttackRecovery = SpitRecovery;
	if (UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
	{
		Shots->ShowCharge(this, TellGlowSpot, SpitTell, GlobRadius * TellGlowSize, SpitterLook::Venom);
	}
	LooterSound::PlayAttached(LooterSoundCue::Spitter::Tell, GetRootComponent());
	UE_LOG(LogLooter, Verbose, TEXT("%s rears to spit."), *GetName());
}

void ASpitterSpider::ReleaseSpit()
{
	// The next one comes a whole interval after this one (its tell included), a little sooner or later.
	SpitReadyIn = FMath::Max(GetSpitInterval() - SpitTell + FMath::FRandRange(-SpitJitter, SpitJitter), 0.5f);
	SacSpent = 1.f;
	LastGlobs.Reset();
	UWorld* World = GetWorld();
	APawn* Victim = GetTarget();
	if (!World || !IsValidTarget(Victim))
	{
		return;
	}
	const float Scale = GetSizeScale();
	const FVector Start = GetMuzzle();
	// Its target's feet, led a little along their way, so standing still or running straight gets hit and a turn or a strafe
	// sees it land behind.
	const FVector Feet = FeetOf(*Victim);
	const float Guess = SpitterRules::FlightSeconds(static_cast<float>(FVector::Dist2D(Start, Feet)));
	const FVector Aim = SpitterRules::AimPoint(Feet, Victim->GetVelocity(), Guess, LeadShare);
	const float Seconds = SpitterRules::FlightSeconds(static_cast<float>(FVector::Dist2D(Start, Aim)));
	FVector Across = FVector::CrossProduct(Aim - Start, FVector::UpVector).GetSafeNormal();
	if (Across.IsNearlyZero())
	{
		Across = GetActorRightVector();
	}

	FSpitterGlobLaunch Launch;
	Launch.Start = Start;
	Launch.Radius = GlobRadius * FMath::Sqrt(Scale);
	// Its venom hits as hard as its bite does against the fight's difficulty (Looter.Fight.DamageScale).
	const float Hurt = AttackDamage * GetFightDamageScale();
	Launch.Damage = Hurt * GlobDamageShare;
	Launch.SplashDamage = Hurt * SplashDamageShare;
	Launch.SplashRadius = SplashRadius;
	Launch.Size = Scale;
	Launch.Color = SpitterLook::Venom;
	Launch.Spitter = this;
	Launch.Instigator = GetController();
	Launch.Puddle.Radius = PuddleRadius * FMath::Sqrt(Scale);
	Launch.Puddle.Seconds = PuddleSeconds;
	Launch.Puddle.Damage = Hurt * PuddleDamageShare;
	Launch.Puddle.Every = PuddleEvery;
	Launch.Puddle.Color = SpitterLook::Venom;
	// One glob, or a Gravebound one's three side by side across its line (each lands as far from the middle one).
	for (int32 Index = 0; Index < GlobsPerSpit; ++Index)
	{
		const float Offset = (static_cast<float>(Index) - 0.5f * static_cast<float>(GlobsPerSpit - 1)) * GlobSpacing;
		Launch.Velocity = SpitterRules::LobVelocity(Start, Aim + Across * Offset, Seconds);
		if (ASpitterGlob* Glob = ASpitterGlob::Launch(World, Launch))
		{
			LastGlobs.Add(Glob);
		}
	}

	// A spray of venom off its fangs as it spits, and the wet spit itself.
	if (World->IsGameWorld())
	{
		if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			Effects.SpawnDeathBurst(EDeathBurst::Gel, Start, (Aim - Start).GetSafeNormal(), 30.f * Scale, SpitterLook::Venom);
		}
	}
	LooterSound::PlayAt(this, LooterSoundCue::Spitter::Spit, Start);
	UE_LOG(LogLooter, Verbose, TEXT("%s spat %d glob(s) at %s (%.1f s flight)."), *GetName(), LastGlobs.Num(), *GetNameSafe(Victim), Seconds);
}

void ASpitterSpider::ChokeSpit(const FVector& Where)
{
	if (!IsTelling())
	{
		return;
	}
	// Its tell is cut short: the strike comes at once and throws nothing (Strike sees it choked), and it flinches a while,
	// the venom bursting out of the sac where the shot went in. Its spit starts over from a full interval.
	bSpitChoked = true;
	AttackWindup = FMath::Max(GetStateTime(), 0.05f);
	AttackRecovery = ChokedRecovery;
	if (UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
	{
		Shots->ClearShotsFrom(this);
	}
	BurstSac(Where, 40.f);
	SacSpent = 1.f;
	SacFlash = 1.f;
	SpitReadyIn = GetSpitInterval();
	UE_LOG(LogLooter, Log, TEXT("%s's spit choked: a shot burst its sac."), *GetName());
}

// ---------------------------------------------------------------------------
// The bite
// ---------------------------------------------------------------------------

void ASpitterSpider::BeginBite()
{
	Move = ESpitterMove::Bite;
	AttackWindup = OwnBiteWindup;
	AttackRecovery = OwnBiteRecovery;
	BiteReadyIn = BiteCooldown + OwnBiteWindup + OwnBiteRecovery;
}

void ASpitterSpider::BiteNow()
{
	// As a spider's bite lands (ACreatureBase::Strike), at its own short reach: its attack range is its spit's hold.
	bool bConnected = false;
	APawn* Victim = GetTarget();
	if (IsValidTarget(Victim))
	{
		const float Scale = GetSizeScale();
		const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
		const bool bInReach = ToVictim.Size2D() <= (BiteFrom + BiteOvershoot) * Scale && FMath::Abs(ToVictim.Z) < BiteHeight * FMath::Max(Scale, 1.f);
		const bool bInFront = FVector::DotProduct(GetActorForwardVector(), ToVictim.GetSafeNormal2D()) > 0.4f;
		if (bInReach && bInFront)
		{
			HitWithAttack(Victim, ToVictim.GetSafeNormal2D());
			bConnected = true;
		}
	}
	OnAttackStrike(bConnected);
}

// ---------------------------------------------------------------------------
// Its clocks
// ---------------------------------------------------------------------------

void ASpitterSpider::TickMoves(float DeltaSeconds)
{
	// Its spit's cooldown runs while it's after someone; a target lost (or not yet seen) leaves it the first spit's delay,
	// so it sizes up whoever it turns on next before it spits.
	const bool bHasTarget = GetTarget() != nullptr && !IsDead();
	if (bHasTarget)
	{
		SpitReadyIn = FMath::Max(0.f, SpitReadyIn - DeltaSeconds);
	}
	else if (bHadTarget)
	{
		SpitReadyIn = FMath::Max(SpitReadyIn, FirstSpitDelay);
	}
	bHadTarget = bHasTarget;
	BiteReadyIn = FMath::Max(0.f, BiteReadyIn - DeltaSeconds);
}
