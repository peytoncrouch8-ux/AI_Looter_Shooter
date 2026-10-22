// APlaceholderCreature: its attacks by role and a ranged kind's distance. A melee, swarm or big kind strikes in front with
// its strength; a flyer dives on its strike (its pose: PlaceholderRules::PoseAt); a ranged kind keeps its band as the Venom
// Spitter does (its brain brings it in to CloseInTo; inside that it backs off, walks round to the fight director's spot for
// it, or holds facing its target) and shoots pellets through UEnemyProjectileSubsystem on a ranged turn, a glow growing at
// its muzzle through the wind-up as the tell; caught close it bites instead (a melee turn).

#include "Creatures/PlaceholderCreature.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightRules.h"
#include "Creatures/FightTuning.h"
#include "Creatures/PlaceholderCreatureDefinition.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Its shot's line is looked at this often (s), and blocked this long at its spot it asks for another. */
	constexpr double ShotCheckEvery = 0.25;
	constexpr float BlockedMove = 1.5f;
	/** Its ways are looked for again this often (s); at its fight spot within this (cm, at size 1). */
	constexpr float KeepLookEvery = 0.2f;
	constexpr float SpotArrive = 120.f;
	/** Its share of its chase backing away. */
	constexpr float BackOffShare = 0.8f;
	/** The nearest it shoots from (cm): nearer, it bites or backs off. */
	constexpr float ShotMinDistance = 250.f;
	/** A flyer's strike reaches this far up or down (cm at size 1); a walker's as ACreatureBase's. */
	constexpr float FlyerStrikeHeight = 400.f;
	constexpr float WalkerStrikeHeight = 220.f;
	/** The tell's glow at its muzzle against its pellet's size. */
	constexpr float ChargeGrowth = 1.8f;

	/** Nothing world-static between From and Victim's middle or feet (ACreatureBase::Strike's rule, here for its own blow). */
	bool IsLineClear(const AActor& Creature, const APawn& Victim, const FVector& From)
	{
		const UWorld* World = Creature.GetWorld();
		if (!World)
		{
			return true;
		}
		FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("PlaceholderStrike"), &Creature, false);
		Params.AddIgnoredActor(&Victim);
		const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
		const float Half = Victim.GetSimpleCollisionHalfHeight();
		for (const FVector& To : { Victim.GetActorLocation(), Victim.GetActorLocation() - FVector(0.f, 0.f, Half * 0.7f) })
		{
			if (!World->LineTraceTestByObjectType(From, To, Solid, Params))
			{
				return true;
			}
		}
		return false;
	}
}

// ---------------------------------------------------------------------------
// Its place in a fight
// ---------------------------------------------------------------------------

EFightRole APlaceholderCreature::GetFightRole() const
{
	return GetRole() == EPlaceholderRole::Ranged ? EFightRole::Ranged : EFightRole::Melee;
}

EFightRole APlaceholderCreature::GetAttackTokenRole() const
{
	return GetRole() == EPlaceholderRole::Ranged && !WantsBite() ? EFightRole::Ranged : EFightRole::Melee;
}

bool APlaceholderCreature::GetFightBand(float& OutNear, float& OutFar) const
{
	if (GetRole() != EPlaceholderRole::Ranged || !Definition)
	{
		return false;
	}
	const float FromFight = FFightTuning::LiveRangedBackOff();
	OutNear = FromFight > 0.f ? FMath::Min(FromFight, Definition->CloseInTo - 100.f) : Definition->BackOffWithin;
	OutFar = FMath::Max(Definition->CloseInTo, OutNear + 100.f);
	return true;
}

bool APlaceholderCreature::WantsBite() const
{
	const APawn* Victim = GetTarget();
	return Victim && FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()) <= PlaceholderRules::BiteReach * GetSizeScale();
}

float APlaceholderCreature::GetAttackStartRange() const
{
	if (GetRole() != EPlaceholderRole::Ranged || !Definition)
	{
		return Super::GetAttackStartRange();
	}
	// Caught close it bites; otherwise a shot at whoever is in its range with a clear line. Nothing else starts an attack.
	if (WantsBite())
	{
		return PlaceholderRules::BiteReach * GetSizeScale();
	}
	const APawn* Victim = GetTarget();
	const float Distance = Victim ? static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation())) : 0.f;
	return Distance >= ShotMinDistance && IsShotClear() ? Definition->ShotRange : 0.f;
}

bool APlaceholderCreature::CanReachHeight(float Rise) const
{
	// A shot or a flyer reaches a player up on a rock or a roof.
	const EPlaceholderRole Kind = GetRole();
	return Kind == EPlaceholderRole::Ranged || Kind == EPlaceholderRole::Flyer || Super::CanReachHeight(Rise);
}

bool APlaceholderCreature::IsShotClear() const
{
	const UWorld* World = GetWorld();
	const APawn* Victim = GetTarget();
	if (!World || !Victim)
	{
		return false;
	}
	const double Now = World->GetTimeSeconds();
	if (Now - ShotCheckedAt >= ShotCheckEvery)
	{
		ShotCheckedAt = Now;
		bShotClear = IsLineClear(*this, *Victim, GetMuzzle());
	}
	return bShotClear;
}

// ---------------------------------------------------------------------------
// Its attacks
// ---------------------------------------------------------------------------

void APlaceholderCreature::OnAttackStarted()
{
	bShotAttack = GetRole() == EPlaceholderRole::Ranged && !WantsBite();
	// The tell: its cry, and for a shot a glow swelling at its muzzle until the pellets fly.
	PlayCue(CueFor(1));
	if (bShotAttack && Definition)
	{
		if (UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
		{
			const FVector Local = GetActorTransform().InverseTransformPosition(GetMuzzle());
			Shots->ShowCharge(this, Local, AttackWindup, Definition->ShotRadius * ChargeGrowth, Definition->ShotColor);
		}
	}
}

void APlaceholderCreature::Strike()
{
	if (bShotAttack)
	{
		FireShots();
		return;
	}
	StrikeInFront();
}

void APlaceholderCreature::OnAttackCalledOff()
{
	// A shot called off before it flew (hit off its turn): its tell's glow goes out with nothing thrown.
	if (bShotAttack)
	{
		if (UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
		{
			Shots->ClearShotsFrom(this);
		}
	}
	bShotAttack = false;
}

void APlaceholderCreature::FireShots()
{
	APawn* Victim = GetTarget();
	UEnemyProjectileSubsystem* Shots = GetWorld() ? GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr;
	if (!Definition || !Shots || !IsValidTarget(Victim))
	{
		return;
	}
	const UPlaceholderCreatureDefinition& Kind = *Definition;
	FEnemyShot Shot;
	Shot.Start = GetMuzzle();
	// At its middle, led a little for a moving target; the pellets stop on the world, so cover still saves them.
	const FVector Aim = PlaceholderRules::LeadAim(Shot.Start, Victim->GetActorLocation(), Victim->GetVelocity(), Kind.ShotSpeed, Kind.LeadShare);
	Shot.Direction = (Aim - Shot.Start).GetSafeNormal();
	Shot.Speed = Kind.ShotSpeed;
	Shot.Range = Kind.ShotRange * 1.3f;
	Shot.Radius = Kind.ShotRadius;
	// A pellet's damage as its blow's (its level and rank in AttackDamage already), times the fight's difficulty.
	Shot.Damage = AttackDamage * Kind.ShotDamageShare * GetFightDamageScale();
	Shot.Color = Kind.ShotColor;
	Shot.Shooter = this;
	Shot.Instigator = GetController();
	if (Kind.ShotCount > 1)
	{
		Shots->FireVolley(Shot, Shot.Direction, Kind.ShotCount, Kind.ShotSpread);
	}
	else
	{
		Shots->Fire(Shot);
	}
	ShotsFired += Kind.ShotCount;
	if (UBulletSubsystem* Bullets = GetWorld()->GetSubsystem<UBulletSubsystem>())
	{
		// A puff at its muzzle as they leave: seen from the side as well as in front.
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(GetWorld());
		Effects.SpawnFlash(Shot.Start, Shot.Direction, 0.6f);
	}
}

void APlaceholderCreature::StrikeInFront()
{
	APawn* Victim = GetTarget();
	if (!IsValidTarget(Victim))
	{
		return;
	}
	const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
	const float Reach = GetStrikeReach();
	const float Height = (GetRole() == EPlaceholderRole::Flyer ? FlyerStrikeHeight : WalkerStrikeHeight) * FMath::Max(GetSizeScale(), 1.f);
	const bool bInReach = ToVictim.Size2D() <= Reach && FMath::Abs(static_cast<float>(ToVictim.Z)) < Height;
	const bool bInFront = FVector::DotProduct(GetActorForwardVector(), ToVictim.GetSafeNormal2D()) > 0.4f;
	if (bInReach && bInFront && IsLineClear(*this, *Victim, GetActorLocation()))
	{
		HitWithAttack(Victim, ToVictim.GetSafeNormal2D(), Definition ? Definition->StrikeStrength : 1.f);
	}
	// A big kind's blow lands in the ground in front of it, hit or miss: dirt thrown up where it struck.
	if (GetRole() == EPlaceholderRole::Big)
	{
		if (UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr)
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(GetWorld());
			const FVector Feet = GetMesh()->GetComponentLocation() + GetActorForwardVector() * Reach * 0.6f;
			Effects.SpawnDirt(Feet, FVector::UpVector, 0.9f);
		}
	}
}

// ---------------------------------------------------------------------------
// A ranged kind keeping its distance (as ASpitterSpider's, without its strafe)
// ---------------------------------------------------------------------------

void APlaceholderCreature::TickKeepAway(float DeltaSeconds)
{
	const APawn* Victim = GetTarget();
	const bool bHunting = Victim && Definition && !IsDead() && GetCreatureState() == ECreatureState::Chase && !GetPack()->IsRetreating()
		&& !IsNoticing();
	if (!bHunting)
	{
		SetKeepingDistance(false);
		Keep = EPlaceholderKeep::None;
		BlockedFor = 0.f;
		return;
	}
	const FVector Here = GetActorLocation();
	const FVector Prey = Victim->GetActorLocation();
	const float Distance = static_cast<float>(FVector::Dist2D(Here, Prey));
	const float Hold = GetAttackRange() * 0.75f;
	float Near = 0.f;
	float Far = 0.f;
	GetFightBand(Near, Far);

	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this);
	FVector Spot = Here;
	const bool bHasSpot = Distance >= Near && Director && Director->GetWaitSpot(*this, *Victim, Spot);
	const float Gap = static_cast<float>(FVector::Dist2D(Here, Spot));
	const EPlaceholderKeep Was = Keep;
	Keep = PlaceholderRules::ChooseKeep(Distance, Near, Hold, bHasSpot, Gap, SpotArrive * GetSizeScale());
	if (Keep == EPlaceholderKeep::None)
	{
		SetKeepingDistance(false);
		return;
	}
	SetKeepingDistance(true);

	if (Keep == EPlaceholderKeep::Hold)
	{
		// There: it stands and watches. Its shot blocked from here a while, it asks for another spot.
		KeepWay = FVector::ZeroVector;
		BlockedFor = IsShotClear() ? 0.f : BlockedFor + DeltaSeconds;
		if (BlockedFor >= BlockedMove && Director)
		{
			BlockedFor = 0.f;
			Director->RequestNewSpot(*this);
		}
		return;
	}
	BlockedFor = 0.f;
	KeepLookIn -= DeltaSeconds;
	if (KeepLookIn <= 0.f || Keep != Was)
	{
		KeepLookIn = KeepLookEvery;
		if (Keep == EPlaceholderKeep::BackOff)
		{
			// Straight back first, then back and to a side, then out sideways: whichever is clear (none: it stands and bites).
			FVector Away = Here - Prey;
			Away.Z = 0.0;
			Away = Away.IsNearlyZero() ? -GetActorForwardVector().GetSafeNormal2D() : Away.GetSafeNormal();
			const FVector Side(-Away.Y, Away.X, 0.0);
			const FVector Ways[] = { Away, Away.RotateAngleAxis(35.f, FVector::UpVector), Away.RotateAngleAxis(-35.f, FVector::UpVector),
				Away.RotateAngleAxis(70.f, FVector::UpVector), Away.RotateAngleAxis(-70.f, FVector::UpVector), Side, -Side };
			KeepWay = FirstClearWay(Ways);
		}
		else
		{
			// Round its target to its spot, never across them.
			const FVector Way = FightRules::OrbitWay(Here, Prey, Spot);
			const FVector Ways[] = { Way, Way.RotateAngleAxis(30.f, FVector::UpVector), Way.RotateAngleAxis(-30.f, FVector::UpVector) };
			KeepWay = FirstClearWay(Ways);
		}
	}
	if (KeepWay.IsNearlyZero())
	{
		return;
	}
	GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed * (Keep == EPlaceholderKeep::BackOff ? BackOffShare : FFightTuning::LiveRangedMoveShare());
	AddMovementInput(KeepWay);
}

FVector APlaceholderCreature::FirstClearWay(TConstArrayView<FVector> Ways) const
{
	const float Look = GetSteerProbes().SweepLength;
	const FVector Here = GetActorLocation();
	const FVector Middle = GetHome().GetLocation();
	for (const FVector& Way : Ways)
	{
		if (Way.IsNearlyZero())
		{
			continue;
		}
		// Never off its hunting ground (its spawner's), whose edge it would only turn back from.
		if (HuntingGround.IsSet() && !HuntingGround.Contains(Here + Way * Look, Middle))
		{
			continue;
		}
		if (LookAlong(Way, Look).bClear)
		{
			return Way;
		}
	}
	return FVector::ZeroVector;
}

void APlaceholderCreature::SetKeepingDistance(bool bKeeping)
{
	if (bKeepingDistance == bKeeping)
	{
		return;
	}
	bKeepingDistance = bKeeping;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	// Keeping its distance it faces its target (its brain turns it) whichever way it walks, and walks off no ledge.
	Movement->bOrientRotationToMovement = !bKeeping;
	Movement->bCanWalkOffLedges = !bKeeping;
}
