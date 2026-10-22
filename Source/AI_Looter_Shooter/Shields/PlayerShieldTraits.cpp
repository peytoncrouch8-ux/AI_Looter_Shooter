// UPlayerShieldComponent's traits (Rare and up): Last Breath's burst on a break, Barbwire's cut on a biter, Lodestone's
// rounds from a hit, Saint's Grace's flare when broken and low. (Harvest's kill is HandleKillXP, in
// PlayerShieldComponent.cpp.) What deals damage waits for the tick: the hit that set it off is a creature's attack still
// running its damage call, and a death inside it would be its own.

#include "Shields/PlayerShieldComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/GraveSaltRules.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementStatusComponent.h"
#include "Embers/Powers/EmberVfx.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Shields/ShieldDamageType.h"
#include "Shields/ShieldSounds.h"
#include "Shields/ShieldTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponFX.h"
#include "CollisionQueryParams.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** The overlap reaches this much past the burst so a big body's middle outside it is still weighed by its side. */
	constexpr float NovaSearchMargin = 200.f;
	/** Within this share of its reach the living are thrown back out of the burst (the grenade's rule). */
	constexpr float NovaKnockShare = 0.6f;
	/** Barbwire's spark on the biter, and Saint's Grace's ring, in the kit's cyan and the saints' gold. */
	const FLinearColor BarbSpark(0.35f, 0.85f, 1.f);

	FWeaponFX* EffectsIn(UWorld* World)
	{
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& FX = Bullets->GetEffects();
		FX.Initialize(World);
		return &FX;
	}
}

float UPlayerShieldComponent::Strength() const
{
	return ShieldRules::TraitStrength(Ward.Definition, Ward.Rarity);
}

void UPlayerShieldComponent::TraitsOnHit(float ChargeBefore)
{
	const UHealthComponent* Life = Health.Get();
	const UHealthComponent::FHealthHit* Hit = Life ? Life->GetHitBeingTaken() : nullptr;
	if (!Hit || ChargeBefore <= 0.f)
	{
		return;
	}
	const FShieldTuning Tuning = FShieldTuning::Live();
	if (Ward.Trait == EShieldTrait::Barbwire && Hit->DamageType && Hit->DamageType->IsA<UCreatureAttackDamageType>() && Hit->DamageCauser)
	{
		// A bite or claw: the creature that made it (its attack's causer is itself).
		PendingBarbs.Add(TPair<TWeakObjectPtr<AActor>, float>(TWeakObjectPtr<AActor>(Hit->DamageCauser),
			ShieldRules::BarbDamage(GetCapacity(), Strength(), Tuning)));
	}
	else if (Ward.Trait == EShieldTrait::Lodestone && FMath::FRand() < ShieldRules::LodestoneOdds(Strength(), Tuning))
	{
		// The ward draws the lead out of what struck it: rounds for the gun in hand, a tenth of its magazine.
		UWeaponManagerComponent* Manager = BoundManager.Get();
		const AWeaponBase* Gun = Manager ? Manager->GetActiveWeapon() : nullptr;
		const UWeaponDefinition* Kind = Gun ? Gun->GetInstance().Definition.Get() : nullptr;
		if (Kind)
		{
			const int32 Rounds = ShieldRules::LodestoneRounds(Gun->GetInstance().Stats.MagazineSize, Strength(), Tuning);
			if (Manager->AddAmmo(Kind->AmmoType, Rounds) > 0)
			{
				LooterSound::Play2D(this, FName(LooterSoundCue::AmmoPickup), 0.6f, 1.15f);
			}
		}
	}
}

void UPlayerShieldComponent::TraitsOnBreak()
{
	if (Ward.Trait == EShieldTrait::LastBreath && bNovaReady)
	{
		bNovaReady = false;
		bNovaPending = true;
	}
}

void UPlayerShieldComponent::TickTraits()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Instigator = Pawn ? Pawn->GetController() : nullptr;
	if (!PendingBarbs.IsEmpty())
	{
		TArray<TPair<TWeakObjectPtr<AActor>, float>> Cuts = MoveTemp(PendingBarbs);
		PendingBarbs.Reset();
		FWeaponFX* FX = EffectsIn(GetWorld());
		for (const TPair<TWeakObjectPtr<AActor>, float>& Cut : Cuts)
		{
			AActor* Biter = Cut.Key.Get();
			const UHealthComponent* Life = Biter ? Biter->FindComponentByClass<UHealthComponent>() : nullptr;
			if (!Life || Life->IsDead() || Cut.Value <= 0.f)
			{
				continue;
			}
			UGameplayStatics::ApplyDamage(Biter, Cut.Value, Instigator, nullptr, UShieldDamageType::StaticClass());
			if (FX)
			{
				EmberVfx::Strike(*FX, Biter->GetActorLocation() + FVector(0.f, 0.f, 40.f), BarbSpark, 0.8f);
			}
		}
	}
	if (bNovaPending)
	{
		bNovaPending = false;
		LooseNova();
	}

	// Saint's Grace: broken and low, the ward flares back once a full charge.
	const UHealthComponent* Life = Health.Get();
	const FShieldTuning Tuning = FShieldTuning::Live();
	if (Ward.Trait == EShieldTrait::SaintsGrace && Life && !Life->IsDead()
		&& ShieldRules::GraceAnswers(bGraceReady, Charge.Charge, Life->GetHealthPercent(), Tuning))
	{
		bGraceReady = false;
		bBroken = false;
		Charge.Charge = ShieldRules::GraceCharge(GetCapacity(), Strength(), Tuning);
		++GraceCount;
		PlayCue(ShieldCue::Grace);
		if (FWeaponFX* FX = EffectsIn(GetWorld()); FX && Pawn)
		{
			EmberVfx::Activation(*FX, Pawn->GetActorLocation() - FVector(0.f, 0.f, 80.f), ElementRules::FxColor(ELooterElement::SoulLight), 0.8f);
		}
		UE_LOG(LogLooter, Log, TEXT("Shield: Saint's Grace, the ward back to %.0f"), Charge.Charge);
	}
}

void UPlayerShieldComponent::LooseNova()
{
	UWorld* World = GetWorld();
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!World || !Pawn)
	{
		return;
	}
	const FShieldTuning Tuning = FShieldTuning::Live();
	const FVector Center = Pawn->GetActorLocation();
	AController* Instigator = Pawn->GetController();
	// As the enemies and the grenade grow with the player's level, so the burst takes the same share at equal levels.
	const UPlayerProgressionSubsystem* Kills = Progression.Get();
	const int32 Level = Kills ? FMath::Max(Kills->GetLevel(), 1) : FMath::Max(Ward.Level, 1);
	const float LevelScale = UPlayerProgressionSubsystem::GetLevelRules().EnemyScale(Level);
	const float NovaStrength = Strength();
	const ELooterElement Element = Ward.Element != ELooterElement::None ? Ward.Element : ELooterElement::Ember;

	// Seen and heard first: the deaths' own bursts go over it.
	if (FWeaponFX* FX = EffectsIn(World))
	{
		EmberVfx::Blast(*FX, this, Center - FVector(0.f, 0.f, 60.f), FVector::UpVector, Tuning.NovaRadius, ElementRules::FxColor(Element),
			Element == ELooterElement::SoulLight);
		EmberVfx::Activation(*FX, Center - FVector(0.f, 0.f, 80.f), ElementRules::FxColor(Element), 1.2f);
	}
	LooterSound::PlayAt(this, FName(ShieldCue::Nova), Center);

	// Every living thing that can be hurt in its reach and its sight (the grenade's rules), never the player.
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ShieldNova), false, Pawn);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Tuning.NovaRadius + NovaSearchMargin), Params);
	TArray<AActor*, TInlineAllocator<16>> Weighed;
	int32 Hits = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Weighed.Contains(Actor))
		{
			continue;
		}
		Weighed.Add(Actor);
		if (!GraveSaltBurst::CanHurt(Actor, Pawn))
		{
			continue;
		}
		const FGraveSaltBody Body = GraveSaltBurst::BodyOf(*Actor);
		const float Distance = FGraveSaltRules::DistanceToBody(Center, Body);
		if (Distance > Tuning.NovaRadius || !GraveSaltBurst::CanSee(*World, Center, Body, Actor, Pawn))
		{
			continue;
		}
		UHealthComponent* Life = Actor->FindComponentByClass<UHealthComponent>();
		if (!Life || Life->IsDead())
		{
			continue;
		}
		float Damage = ShieldRules::NovaDamageAt(Distance, NovaStrength, LevelScale, Tuning);
		if (GraveSaltBurst::IsBoss(Actor))
		{
			Damage = FMath::Min(Damage, Life->GetMaxHealth() * Tuning.NovaBossCap);
		}
		if (Damage <= 0.f)
		{
			continue;
		}
		const FVector Point = FGraveSaltRules::NearestPoint(Center, Body);
		FVector Direction = (Point - Center).GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector::UpVector;
		}
		const FHitResult Hit(Actor, Cast<UPrimitiveComponent>(Actor->GetRootComponent()), Point, -Direction);
		UGameplayStatics::ApplyPointDamage(Actor, Damage, Direction, Hit, Instigator, nullptr, UShieldDamageType::StaticClass());
		++Hits;
		if (Life->IsDead())
		{
			continue;
		}
		// The ward's element takes hold on what it struck (no roll: the burst is the element), at its weakness.
		UElementStatusComponent::TryApply(*Actor, Element, 1.f, Damage, false, 0.f, Point, nullptr, Instigator);
		if (Distance <= Tuning.NovaRadius * NovaKnockShare)
		{
			GraveSaltBurst::KnockAway(*Actor, Center);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Shield: Last Breath (%s), %d struck"), *ElementRules::Word(Element), Hits);
}
