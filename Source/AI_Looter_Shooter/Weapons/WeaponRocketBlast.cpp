#include "Weapons/WeaponRocketBlast.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/GraveSaltRules.h"
#include "Combat/HealthComponent.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/LooterDamageTypes.h"
#include "Elements/ElementCallout.h"
#include "Elements/ElementStatusComponent.h"
#include "Weapons/GunshotSound.h"
#include "Weapons/PartEffectComponent.h"
#include "Weapons/WeaponBase.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

// The launcher's rocket bursting: who it reaches, what it does to them, the knock and the shove. The look is in
// WeaponRocketBlastEffects.cpp. It measures and sees as the grave-salt burst does (GraveSaltBurst's BodyOf and CanSee).

namespace
{
	/** The search ball reaches this far past the burst's reach: a big body's middle can be well past it while its side is in. */
	constexpr float SearchMargin = 200.f;
	/** The burst's middle is lifted this far off what it met along its normal, so the sight lines don't start in the wall. */
	constexpr float SurfaceLift = 8.f;
}

float WeaponRocketBlast::DamageAt(float Damage, float Distance, const GunKinds::FBlastRules& Rules, bool bShooter)
{
	const float Share = GunKinds::BlastFalloff(FMath::Max(Distance, 0.f), Rules.Core, Rules.Radius, Rules.EdgeShare);
	return FMath::Max(Damage, 0.f) * Share * (bShooter ? Rules.SelfScale : 1.f);
}

bool WeaponRocketBlast::CanHurt(const AActor* Actor, const AActor* Shooter)
{
	if (!Actor)
	{
		return false;
	}
	// The shooter feels their own rocket (a share of it); no other player is ever hurt by one.
	const APawn* Pawn = Cast<APawn>(Actor);
	if (Pawn && Pawn->IsPlayerControlled() && Actor != Shooter)
	{
		return false;
	}
	const UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>();
	return Health && !Health->IsDead();
}

TArray<FRocketBlastTarget> WeaponRocketBlast::FindTargets(const UWorld* World, const FVector& Center, const AActor* Shooter,
	const AActor* Struck, float Damage, const GunKinds::FBlastRules& Rules)
{
	TArray<FRocketBlastTarget> Targets;
	if (!World)
	{
		return Targets;
	}
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RocketBlast), false);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Rules.Radius + SearchMargin), Params);

	TArray<const AActor*, TInlineAllocator<16>> Weighed;
	auto Consider = [&](AActor* Actor)
	{
		if (!Actor || Weighed.Contains(Actor))
		{
			return;
		}
		Weighed.Add(Actor);
		if (!CanHurt(Actor, Shooter))
		{
			return;
		}
		const bool bStruck = Actor == Struck;
		const FGraveSaltBody Body = GraveSaltBurst::BodyOf(*Actor);
		const float Distance = bStruck ? 0.f : FGraveSaltRules::DistanceToBody(Center, Body);
		// What the rocket hit takes it whatever stands between; the rest only past nothing solid.
		if (!bStruck && (Distance > Rules.Radius || !GraveSaltBurst::CanSee(*World, Center, Body, Actor, nullptr)))
		{
			return;
		}
		FRocketBlastTarget& Target = Targets.AddDefaulted_GetRef();
		Target.Actor = Actor;
		Target.Point = bStruck ? Center : FGraveSaltRules::NearestPoint(Center, Body);
		Target.Distance = Distance;
		Target.bShooter = Actor == Shooter;
		Target.Damage = DamageAt(Damage, Distance, Rules, Target.bShooter);
		if (Target.bShooter)
		{
			// A rocket at the shooter's feet hurts, but never takes more than its share of their health (it never kills from full).
			const UHealthComponent* Health = Actor->FindComponentByClass<UHealthComponent>();
			Target.Damage = FMath::Min(Target.Damage, Rules.SelfCap * (Health ? Health->GetMaxHealth() : 0.f));
		}
	};
	Consider(const_cast<AActor*>(Struck));
	for (const FOverlapResult& Overlap : Overlaps)
	{
		Consider(Overlap.GetActor());
	}
	// The shooter's capsule may not be in the overlap (a player's pawn answers on its own channel): look at them too.
	Consider(const_cast<AActor*>(Shooter));
	Targets.Sort([](const FRocketBlastTarget& A, const FRocketBlastTarget& B) { return A.Distance < B.Distance; });
	return Targets;
}

FRocketBlastResult WeaponRocketBlast::Detonate(UWorld* World, const FVector& Center, const FVector& Up, AWeaponBase* Weapon, AActor* Shooter,
	AController* Instigator, float Damage, float Impulse, const FHitResult* Hit, AActor* BurstSource)
{
	FRocketBlastResult Result;
	if (!World)
	{
		return Result;
	}
	const FVector Normal = Up.IsNearlyZero() ? FVector::UpVector : Up.GetSafeNormal();
	const FVector Heart = Center + Normal * SurfaceLift;
	Result.Center = Heart;
	const GunKinds::FBlastRules Rules = GunKinds::BlastRules();
	AActor* Struck = Hit ? Hit->GetActor() : nullptr;
	const TArray<FRocketBlastTarget> Targets = FindTargets(World, Heart, Shooter, Struck, Damage, Rules);

	// Seen and heard first, as a bullet's impact is: the damage below may kill, and the deaths' own bursts go over it.
	SpawnEffects(*World, Heart, Normal, Hit);
	// Near or far by the listener (GunshotSound: the near crack and debris, or past ExplosionDistantFrom the far boom).
	GunshotSound::PlayExplosion(World, Heart);

	for (const FRocketBlastTarget& Target : Targets)
	{
		AActor* Actor = Target.Actor.Get();
		UHealthComponent* Health = Actor ? Actor->FindComponentByClass<UHealthComponent>() : nullptr;
		// One body's death can take another with it: only the living are hurt.
		if (!Health || Health->IsDead() || Target.Damage <= 0.f)
		{
			continue;
		}
		FVector Direction = (Target.Point - Heart).GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = Weapon ? (Actor->GetActorLocation() - Weapon->GetActorLocation()).GetSafeNormal() : Normal;
		}
		const FHitResult BodyHit(Actor, Cast<UPrimitiveComponent>(Actor->GetRootComponent()), Target.Point, -Direction);
		if (Target.bShooter)
		{
			// The shooter's own share: from the burst itself (no one credited, no damage number over their own head).
			UGameplayStatics::ApplyPointDamage(Actor, Target.Damage, Direction, BodyHit, nullptr, BurstSource, UWeaponDamageType::StaticClass());
			Result.ShooterDamage += Target.Damage;
			continue;
		}
		// The gun's damage, the launcher its causer: the kill is its notch, and the creature's reaction knows the launcher.
		AActor* Causer = Weapon ? static_cast<AActor*>(Weapon) : BurstSource;
		UPartEffectComponent* Parts = Weapon ? UPartEffectComponent::For(*Weapon) : nullptr;
		const ELooterElement Element = Parts ? Parts->GetElement() : ELooterElement::None;
		{
			// The number over it knows the rocket's element, as a bullet's does (ElementCallout).
			const FHitElementScope ElementOfThisHit(Element);
			UGameplayStatics::ApplyPointDamage(Actor, Target.Damage, Direction, BodyHit, Instigator, Causer, UWeaponDamageType::StaticClass());
		}
		// The launcher's element (its rocket's kind: Incendiary, Saint's, Salt) on each body it hurt, as a bullet's hit sets
		// it (UPartEffectSubsystem): rockets never pass the bullets' hooks.
		if (Weapon && !Health->IsDead())
		{
			if (Element != ELooterElement::None
				&& UElementStatusComponent::TryApply(*Actor, Element, Parts->GetElementChance(), Target.Damage, false, FMath::FRand(), Target.Point,
					Weapon, Instigator))
			{
				if (UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>())
				{
					Bullets->GetEffects().Initialize(World);
					Bullets->GetEffects().SpawnElementHit(Element, Target.Point, -Direction, Direction, 1.5f, true, false);
				}
			}
		}
		++Result.Hits;
		if (Weapon)
		{
			Weapon->NotifyBulletHit(BodyHit, Target.Damage, false);
		}
		if (Health->IsDead())
		{
			++Result.Kills;
		}
		else if (Target.Distance <= Rules.Radius * Rules.KnockShare)
		{
			// A corpse goes with the kill's own knock (CreatureHitReactionComponent); the living near the heart are thrown.
			GraveSaltBurst::KnockAway(*Actor, Heart);
		}
	}

	// Loose things shoved out of it: crates, bottles, the dead's bones.
	if (Impulse > 0.f)
	{
		FCollisionObjectQueryParams Loose;
		Loose.AddObjectTypesToQuery(ECC_PhysicsBody);
		Loose.AddObjectTypesToQuery(ECC_WorldDynamic);
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByObjectType(Overlaps, Heart, FQuat::Identity, Loose, FCollisionShape::MakeSphere(Rules.Radius),
			FCollisionQueryParams(SCENE_QUERY_STAT(RocketShove), false));
		for (const FOverlapResult& Overlap : Overlaps)
		{
			UPrimitiveComponent* Component = Overlap.GetComponent();
			if (Component && Component->IsSimulatingPhysics())
			{
				Component->AddRadialImpulse(Heart, Rules.Radius, Impulse, ERadialImpulseFalloff::RIF_Linear, true);
			}
		}
	}

	FRandomStream Random(static_cast<int32>(GetTypeHash(Heart)));
	GraveSaltBurst::KickViews(*World, Heart, Random);
	UE_LOG(LogLooter, Verbose, TEXT("Rocket burst at %s: %d hurt, %d killed, %.0f to its shooter"), *Heart.ToCompactString(), Result.Hits,
		Result.Kills, Result.ShooterDamage);
	return Result;
}

FName WeaponRocketBlast::ExplosionCue()
{
	return LooterSoundCue::Launcher::Explosion;
}
