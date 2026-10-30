#include "Weapons/PartEffectSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementStatusComponent.h"
#include "Elements/ElementTuning.h"
#include "Weapons/PartEffectComponent.h"
#include "Weapons/PartEffectRules.h"
#include "Weapons/PartEffectTuning.h"
#include "Weapons/WeaponBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** A splash's pop, and a bounce's whine, heard at most this often (s): a fast gun's stream of them sounds as a few. */
	constexpr double PopSoundGap = 0.08;
	constexpr double BounceSoundGap = 0.1;
	/** An Explosive round's pop: the rocket's blast, small and high. */
	const FName PopCue(TEXT("Weapon.Launcher.Explosion"));
	constexpr float PopVolume = 0.3f;
	constexpr float PopPitch = 1.6f;
	/** A Ricochet round's bounce looks for this many of the nearest creatures before giving up on a clear line. */
	constexpr int32 BounceLooks = 3;
	/** How far a bounce may fly past its target's distance (it flies at its target, not forever). */
	constexpr float BounceRangeScale = 1.5f;
}

bool UPartEffectSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// As the bullets: editor preview worlds too, for the tests.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UPartEffectSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UBulletSubsystem* BulletSubsystem = Cast<UBulletSubsystem>(Collection.InitializeDependency(UBulletSubsystem::StaticClass()));
	if (!BulletSubsystem)
	{
		return;
	}
	Bullets = BulletSubsystem;
	FiringHandle = BulletSubsystem->OnBulletFiring.AddUObject(this, &UPartEffectSubsystem::NoteFiring);
	LandingHandle = BulletSubsystem->OnBulletLanding.AddUObject(this, &UPartEffectSubsystem::NoteLanding);
	StruckHandle = BulletSubsystem->OnBulletStruck.AddUObject(this, &UPartEffectSubsystem::NoteStruck);
}

void UPartEffectSubsystem::Deinitialize()
{
	if (UBulletSubsystem* BulletSubsystem = Bullets.Get())
	{
		BulletSubsystem->OnBulletFiring.Remove(FiringHandle);
		BulletSubsystem->OnBulletLanding.Remove(LandingHandle);
		BulletSubsystem->OnBulletStruck.Remove(StruckHandle);
	}
	Bullets.Reset();
	Super::Deinitialize();
}

void UPartEffectSubsystem::NoteFiring(FBulletShot& Shot)
{
	AWeaponBase* Weapon = Shot.Weapon.Get();
	// A round an effect made carries its maker's stamp already, and makes no more.
	if (!Weapon || Shot.EffectGeneration > 0)
	{
		return;
	}
	UPartEffectComponent* Parts = UPartEffectComponent::For(*Weapon);
	Shot.Element = Parts->GetElement();
	Shot.ElementChance = Parts->GetElementChance();
	Shot.Effect = Parts->GetEffect();
	if (Shot.Element != ELooterElement::None)
	{
		// The element shows from the muzzle: its tracer runs toward the element's colour.
		Shot.TracerColor = FMath::Lerp(Shot.TracerColor, ElementRules::FxColor(Shot.Element), FElementTuning::Live().TracerTint);
	}
	const FPartEffectTuning Tuning = FPartEffectTuning::Live();
	switch (Shot.Effect)
	{
	case EPartEffect::Piercing:
		Shot.PierceLeft = Tuning.PiercingCount;
		break;
	case EPartEffect::Overcharge:
	{
		// The wound-up round: its charge's damage, a heavier tracer to show it.
		const float Scale = Parts->TakeChargeScale();
		Shot.Damage *= Scale;
		Shot.TracerWidth *= FMath::Lerp(1.f, 2.f, FMath::Clamp((Scale - 1.f) / FMath::Max(Tuning.OverchargeDamage - 1.f, 0.01f), 0.f, 1.f));
		break;
	}
	case EPartEffect::Twin:
	{
		// Two bullets side by side for the one round: this one a little left, its twin a little right.
		FVector Left;
		FVector Right;
		PartEffectRules::TwinDirections(Shot.Direction, FVector::UpVector, Tuning, Left, Right);
		Shot.Damage *= Tuning.TwinShare;
		FBulletShot Second = Shot;
		Shot.Direction = Left;
		Second.Direction = Right;
		Second.EffectGeneration = 1;
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Right).GetSafeNormal();
		Second.VisualStart -= Side * 2.f;
		if (UBulletSubsystem* BulletSubsystem = Bullets.Get())
		{
			BulletSubsystem->FireSoon(Second);
		}
		break;
	}
	default:
		break;
	}
}

void UPartEffectSubsystem::NoteLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing)
{
	if (Shot.bBlank)
	{
		return;
	}
	if (Shot.Effect == EPartEffect::Explosive)
	{
		// What it hit takes Direct of it; the rest splashes (NoteStruck), what it hit included.
		Landing.DamageScale *= FPartEffectTuning::Live().ExplosiveDirect;
	}
	// The old salted-ward bonus is gone (the user, 2026-10-29 night): wards are typed now and Wardbreak itself is the
	// breaker (UCreatureTraitComponent's ward).
}

void UPartEffectSubsystem::NoteStruck(const FBulletShot& Shot, const FHitResult& Hit, FBulletStrike& Strike)
{
	if (Shot.bBlank || (Shot.Element == ELooterElement::None && Shot.Effect == EPartEffect::None))
	{
		return;
	}
	AActor* Struck = Hit.GetActor();
	ACreatureBase* Creature = Cast<ACreatureBase>(Struck);
	UBulletSubsystem* BulletSubsystem = Bullets.Get();

	if (Shot.Element != ELooterElement::None && BulletSubsystem)
	{
		// Every hit shows its element; one whose roll comes up sets its status, fuller, and fuller still on a weak creature.
		bool bSet = false;
		bool bWeak = false;
		if (Creature && Strike.Damage > 0.f)
		{
			bWeak = ElementRules::Affinity(Shot.Element, ElementRules::TargetOf(Creature), Strike.bCritical) == EElementAffinity::Weak;
			bSet = UElementStatusComponent::TryApply(*Creature, Shot.Element, Shot.ElementChance, Strike.Damage, Strike.bCritical,
				FMath::FRand(), Hit.ImpactPoint, Shot.Weapon.Get(), Shot.Instigator.Get());
			StatusesSet += bSet ? 1 : 0;
		}
		if (FElementTuning::Live().LookScale > 0.f)
		{
			FWeaponFX& FX = BulletSubsystem->GetEffects();
			FX.Initialize(GetWorld());
			const float Scale = Creature ? 1.f : 0.5f;
			FX.SpawnElementHit(Shot.Element, Hit.ImpactPoint, Hit.ImpactNormal, Shot.Direction, Scale, bSet, bWeak);
		}
	}

	switch (Shot.Effect)
	{
	case EPartEffect::Piercing:
		if (Creature && Shot.PierceLeft > 0)
		{
			Strike.bPassThrough = true;
			Strike.PassDamageScale = FPartEffectTuning::Live().PiercingShare;
		}
		break;
	case EPartEffect::Ricochet:
		if (Shot.EffectGeneration == 0)
		{
			Bounce(Shot, Hit, Strike.Damage);
		}
		break;
	case EPartEffect::Explosive:
		Splash(Shot, Hit, Strike.Damage);
		break;
	default:
		break;
	}
}

AActor* UPartEffectSubsystem::FindBounceTarget(const FVector& From, float Radius, const AActor* Except) const
{
	const UWorld* World = GetWorld();
	if (!World || Radius <= 0.f)
	{
		return nullptr;
	}
	TArray<ACreatureBase*, TInlineAllocator<16>> Near;
	TArray<FVector, TInlineAllocator<16>> Spots;
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (Creature == Except || Creature->IsDead() || Creature->IsActorBeingDestroyed()
			|| FVector::DistSquared(Creature->GetActorLocation(), From) > FMath::Square(static_cast<double>(Radius)))
		{
			continue;
		}
		Near.Add(Creature);
		Spots.Add(Creature->GetActorLocation());
	}
	// Nearest first, a clear line to it or the next: a bounce never goes through a wall.
	for (int32 Look = 0; Look < BounceLooks && !Near.IsEmpty(); ++Look)
	{
		const int32 Pick = PartEffectRules::PickNearest(From, Spots, Radius);
		if (Pick == INDEX_NONE)
		{
			break;
		}
		ACreatureBase* Candidate = Near[Pick];
		FCollisionQueryParams Params(SCENE_QUERY_STAT(RicochetSight), false);
		Params.AddIgnoredActor(Candidate);
		if (Except)
		{
			Params.AddIgnoredActor(Except);
		}
		if (!World->LineTraceTestByChannel(From, Candidate->GetActorLocation(), ECC_Visibility, Params))
		{
			return Candidate;
		}
		Near.RemoveAtSwap(Pick);
		Spots.RemoveAtSwap(Pick);
	}
	return nullptr;
}

void UPartEffectSubsystem::Bounce(const FBulletShot& Shot, const FHitResult& Hit, float Damage)
{
	UBulletSubsystem* BulletSubsystem = Bullets.Get();
	if (!BulletSubsystem || Damage <= 0.f)
	{
		return;
	}
	const FPartEffectTuning Tuning = FPartEffectTuning::Live();
	// Off the surface it struck (a creature: back the way it came), toward the nearest creature it can see.
	const FVector Out = Hit.ImpactNormal.IsNearlyZero() ? -Shot.Direction : Hit.ImpactNormal.GetSafeNormal();
	const FVector From = Hit.ImpactPoint + Out * 5.f;
	AActor* Target = FindBounceTarget(From, Tuning.RicochetRadius, Hit.GetActor());
	if (!Target)
	{
		return;
	}
	const FVector Aim = Target->GetActorLocation();
	FBulletShot Bounced = Shot;
	Bounced.Start = From;
	Bounced.VisualStart = From;
	Bounced.Direction = (Aim - From).GetSafeNormal();
	Bounced.Range = FMath::Max(FVector::Dist(From, Aim) * BounceRangeScale, 200.f);
	// Its share of what the round would have done, without the round's falloff (it's a fresh flight).
	Bounced.Damage = Shot.Damage * Tuning.RicochetShare;
	Bounced.FalloffStats.Reset();
	Bounced.EffectGeneration = 1;
	Bounced.IgnoreActor = Hit.GetActor();
	Bounced.TracerColor = FMath::Lerp(Shot.TracerColor, FLinearColor(1.f, 0.95f, 0.85f), 0.4f);
	BulletSubsystem->FireSoon(Bounced);
	++Ricochets;

	FWeaponFX& FX = BulletSubsystem->GetEffects();
	FX.Initialize(GetWorld());
	FX.SpawnRicochet(Hit.ImpactPoint, Bounced.Direction, 0.8f);
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Now - LastBounceSound >= BounceSoundGap || Now < LastBounceSound)
	{
		LastBounceSound = Now;
		LooterSound::PlayAt(this, LooterSoundCue::Impact::Ricochet, Hit.ImpactPoint);
	}
	UE_LOG(LogLooter, Verbose, TEXT("Ricochet off %s to %s for %.1f"), *GetNameSafe(Hit.GetActor()), *GetNameSafe(Target), Bounced.Damage);
}

void UPartEffectSubsystem::Splash(const FBulletShot& Shot, const FHitResult& Hit, float Damage)
{
	UWorld* World = GetWorld();
	UBulletSubsystem* BulletSubsystem = Bullets.Get();
	const FPartEffectTuning Tuning = FPartEffectTuning::Live();
	const float SplashDamage = PartEffectRules::SplashDamage(Damage, Tuning);
	if (!World || !BulletSubsystem || SplashDamage <= 0.f || Tuning.ExplosiveRadius <= 0.f)
	{
		return;
	}
	const FVector Out = Hit.ImpactNormal.IsNearlyZero() ? -Shot.Direction : Hit.ImpactNormal.GetSafeNormal();
	const FVector Origin = Hit.ImpactPoint + Out * 5.f;
	// Everything round it (what it hit too: its 60% and this make the round whole), never through a wall (visibility),
	// never the shooter or their gun.
	TArray<AActor*> Spared;
	if (AActor* Shooter = Shot.Shooter.Get())
	{
		Spared.Add(Shooter);
	}
	if (AActor* Weapon = Shot.Weapon.Get())
	{
		Spared.Add(Weapon);
	}
	UGameplayStatics::ApplyRadialDamageWithFalloff(World, SplashDamage, SplashDamage * Tuning.ExplosiveEdgeShare, Origin, 0.f,
		Tuning.ExplosiveRadius, 1.f, UWeaponDamageType::StaticClass(), Spared, Shot.Weapon.Get(), Shot.Instigator.Get(), ECC_Visibility);
	++Splashes;

	FWeaponFX& FX = BulletSubsystem->GetEffects();
	FX.Initialize(World);
	FX.SpawnSplashPop(Hit.ImpactPoint, Out, Tuning.ExplosiveRadius);
	const double Now = World->GetTimeSeconds();
	if (Now - LastPopSound >= PopSoundGap || Now < LastPopSound)
	{
		LastPopSound = Now;
		LooterSound::PlayAt(this, PopCue, Hit.ImpactPoint, PopVolume, PopPitch);
	}
}
