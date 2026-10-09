#include "Combat/GraveSaltBurst.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossComponent.h"
#include "Combat/GraveSaltDamageType.h"
#include "Combat/HealthComponent.h"
#include "Combat/HitReaction.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/UnpaidCreature.h"
#include "Player/PlayerMeleeComponent.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

// The grave-salt burst: who it reaches, what it does to them and the knock. The look, the sound and the jolt are in
// GraveSaltBurstEffects.cpp.

namespace
{
	/** The search ball reaches this far past the radius: a big body's middle can be well past it while its side is in it. */
	constexpr float SearchMargin = 200.f;
	/** A body's top as the sight line looks for it: this far under its very top (the crown of a capsule is thin). */
	constexpr float TopInset = 10.f;
}

FGraveSaltBody GraveSaltBurst::BodyOf(const AActor& Actor)
{
	FGraveSaltBody Body;
	const ACharacter* Character = Cast<ACharacter>(&Actor);
	if (const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr)
	{
		Body.Center = Capsule->GetComponentLocation();
		Body.HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		Body.Radius = Capsule->GetScaledCapsuleRadius();
		return Body;
	}
	FVector Origin;
	FVector Extent;
	Actor.GetActorBounds(true, Origin, Extent);
	Body.Center = Origin;
	Body.HalfHeight = static_cast<float>(Extent.Z);
	Body.Radius = static_cast<float>(FMath::Max(Extent.X, Extent.Y));
	return Body;
}

bool GraveSaltBurst::CanHurt(const AActor* Actor, const AActor* Thrower)
{
	if (!Actor || Actor == Thrower)
	{
		return false;
	}
	// No self-damage, and never another player: the salt is the player's tool, not a hazard to them (fun first).
	const APawn* Pawn = Cast<APawn>(Actor);
	if (Pawn && Pawn->IsPlayerControlled())
	{
		return false;
	}
	const UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>();
	return Health && !Health->IsDead();
}

bool GraveSaltBurst::IsUnpaid(const AActor* Actor)
{
	return Actor && Actor->IsA<AUnpaidCreature>();
}

bool GraveSaltBurst::IsBoss(const AActor* Actor)
{
	const ACreatureBase* Creature = Cast<ACreatureBase>(Actor);
	return Creature && (Creature->GetRank() == ECreatureRank::Boss || Creature->FindComponentByClass<UBossComponent>() != nullptr);
}

bool GraveSaltBurst::CanSee(const UWorld& World, const FVector& Center, const FGraveSaltBody& Body, const AActor* Target, const AActor* Thrower)
{
	// The world's solid shapes only: walls, rocks, the ground. Volumes are skipped (StaticGeometryParams), and creatures
	// aren't world-static, so a body behind another is still burned.
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(&World, TEXT("GraveSaltSight"), Thrower, false);
	Params.AddIgnoredActor(Target);
	const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
	const FVector Top = Body.Center + FVector(0.f, 0.f, FMath::Max(Body.HalfHeight - TopInset, 0.f));
	for (const FVector& Point : { FGraveSaltRules::NearestPoint(Center, Body), Body.Center, Top })
	{
		if (Point.Equals(Center, 1.0))
		{
			return true;
		}
		FHitResult Block;
		if (!World.LineTraceSingleByObjectType(Block, Center, Point, Solid, Params))
		{
			return true;
		}
	}
	return false;
}

TArray<FGraveSaltTarget> GraveSaltBurst::FindTargets(const UWorld* World, const FVector& Center, const AActor* Thrower, float LevelScale,
	FRandomStream& Random)
{
	TArray<FGraveSaltTarget> Targets;
	if (!World)
	{
		return Targets;
	}
	// Everything with a body near enough: pawns (creatures) and dynamic things (dummies, egg sacs), as the strike looks.
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GraveSaltBurst), false, Thrower);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Objects, FCollisionShape::MakeSphere(FGraveSaltRules::Radius + SearchMargin), Params);

	TArray<const AActor*, TInlineAllocator<16>> Weighed;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Weighed.Contains(Actor))
		{
			continue;
		}
		Weighed.Add(Actor);
		if (!CanHurt(Actor, Thrower))
		{
			continue;
		}
		const FGraveSaltBody Body = BodyOf(*Actor);
		const float Distance = FGraveSaltRules::DistanceToBody(Center, Body);
		if (Distance > FGraveSaltRules::Radius || !CanSee(*World, Center, Body, Actor, Thrower))
		{
			continue;
		}
		FGraveSaltTarget& Target = Targets.AddDefaulted_GetRef();
		Target.Actor = Actor;
		Target.Point = FGraveSaltRules::NearestPoint(Center, Body);
		Target.Distance = Distance;
		Target.bUnpaid = IsUnpaid(Actor);
		Target.bBoss = IsBoss(Actor);
		Target.Damage = FGraveSaltRules::BurstDamage(Distance, LevelScale, Random.FRand(), Target.bUnpaid);
		if (Target.bBoss)
		{
			const UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>();
			Target.Damage = FGraveSaltRules::CapForBoss(Target.Damage, Health ? Health->GetMaxHealth() : 0.f);
		}
	}
	Targets.Sort([](const FGraveSaltTarget& A, const FGraveSaltTarget& B) { return A.Distance < B.Distance; });
	return Targets;
}

FVector GraveSaltBurst::KnockAway(AActor& Target, const FVector& Center)
{
	FVector Away = Target.GetActorLocation() - Center;
	Away.Z = 0.0;
	if (Away.IsNearlyZero(1.0))
	{
		// Burst right under it: any way out will do, the same one each time for the same body.
		Away = FRotator(0.f, static_cast<float>(GetTypeHash(Target.GetFName()) % 360u), 0.f).Vector();
	}
	return UPlayerMeleeComponent::KnockBack(Target, Away.GetSafeNormal());
}

FGraveSaltBurstResult GraveSaltBurst::Detonate(UWorld* World, const FVector& Center, const FVector& Up, APawn* Thrower, float LevelScale)
{
	FGraveSaltBurstResult Result;
	Result.Center = Center;
	if (!World)
	{
		return Result;
	}
	FRandomStream Random(static_cast<int32>(GetTypeHash(Center) ^ static_cast<uint32>(World->GetTimeSeconds() * 1000.0)));
	const TArray<FGraveSaltTarget> Targets = FindTargets(World, Center, Thrower, LevelScale, Random);

	// Seen and heard first, as a bullet's impact is: the damage below may kill, and the deaths' own bursts go over it.
	SpawnEffects(*World, Center, Up.IsNearlyZero() ? FVector::UpVector : Up.GetSafeNormal());
	PlaySounds(*World, Center, Targets);

	AController* Instigator = Thrower ? Thrower->GetController() : nullptr;
	for (const FGraveSaltTarget& Target : Targets)
	{
		AActor* Actor = Target.Actor.Get();
		UHealthComponent* Health = Actor ? Actor->FindComponentByClass<UHealthComponent>() : nullptr;
		// One body's death can take another with it (a pack's chain): only the living are hurt.
		if (!Health || Health->IsDead() || Target.Damage <= 0.f)
		{
			continue;
		}
		FVector Direction = (Target.Point - Center).GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector::UpVector;
		}
		const FHitResult Hit(Actor, Cast<UPrimitiveComponent>(Actor->GetRootComponent()), Target.Point, -Direction);
		if (Target.bUnpaid)
		{
			// Salt burns the dead: the creature's hit reactions hear it as a heavy blow (they aren't told what dealt it),
			// so it staggers whatever its share.
			FHitReaction::FMeleeScope SaltBurns;
			UGameplayStatics::ApplyPointDamage(Actor, Target.Damage, Direction, Hit, Instigator, Thrower, UGraveSaltDamageType::StaticClass());
		}
		else
		{
			UGameplayStatics::ApplyPointDamage(Actor, Target.Damage, Direction, Hit, Instigator, Thrower, UGraveSaltDamageType::StaticClass());
		}
		++Result.Hits;
		Result.HitResults.Add(Hit);
		Result.Damages.Add(Target.Damage);
		if (Health->IsDead())
		{
			++Result.Kills;
		}
		else if (Target.Distance <= FGraveSaltRules::Radius * FGraveSaltRules::KnockShare)
		{
			// A corpse goes with the kill's own knock; the living near the heart are thrown back out of it.
			KnockAway(*Actor, Center);
		}
	}
	KickViews(*World, Center, Random);
	return Result;
}
