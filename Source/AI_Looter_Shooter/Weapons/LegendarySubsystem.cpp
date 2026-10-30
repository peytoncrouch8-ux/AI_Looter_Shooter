#include "Weapons/LegendarySubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/CreatureBase.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/LegendaryEffectComponent.h"
#include "Weapons/LegendaryLantern.h"
#include "Weapons/LegendaryTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "Components/CapsuleComponent.h"
#include "CoreGlobals.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Silk as the light shows it: pale, a touch cool, never glowing white. */
	const FLinearColor SilkColor(0.78f, 0.79f, 0.84f);
	/** A Gravesilk round's tracer leans this far to the silk. */
	constexpr float SilkTracerTint = 0.45f;
	/** The strands pinning a rooted creature: how many, how far out round its feet (times its capsule's radius), how thick. */
	constexpr int32 StrandCount = 6;
	constexpr float StrandReach = 1.7f;
	constexpr float StrandWidth = 1.2f;
	constexpr float StrandIntensity = 0.7f;
	/** Company Scrip's pay at the body: gold glints rising. */
	const FLinearColor ScripGold(1.f, 0.76f, 0.3f);
	constexpr int32 ScripGlints = 6;
	/** Where a lantern looks for the ground under a body (cm, up and down from its middle). */
	constexpr float GroundProbe = 400.f;
	/** Silk records kept before the dead and the gone are swept out. */
	constexpr int32 SilkSweepAt = 64;

	bool IsDeadNow(const ACreatureBase* Creature)
	{
		const UHealthComponent* Health = Creature ? Creature->FindComponentByClass<UHealthComponent>() : nullptr;
		return !Creature || Creature->IsDead() || (Health && Health->IsDead());
	}
}

ULegendarySubsystem* ULegendarySubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<ULegendarySubsystem>() : nullptr;
}

bool ULegendarySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// As the bullets: editor preview worlds too, for the tests.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void ULegendarySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UBulletSubsystem* BulletSubsystem = Cast<UBulletSubsystem>(Collection.InitializeDependency(UBulletSubsystem::StaticClass()));
	if (!BulletSubsystem)
	{
		return;
	}
	Bullets = BulletSubsystem;
	FiringHandle = BulletSubsystem->OnBulletFiring.AddUObject(this, &ULegendarySubsystem::NoteFiring);
	LandingHandle = BulletSubsystem->OnBulletLanding.AddUObject(this, &ULegendarySubsystem::NoteLanding);
	StruckHandle = BulletSubsystem->OnBulletStruck.AddUObject(this, &ULegendarySubsystem::NoteStruck);
}

void ULegendarySubsystem::Deinitialize()
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

TStatId ULegendarySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULegendarySubsystem, STATGROUP_Tickables);
}

double ULegendarySubsystem::Now() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

// ---------------------------------------------------------------------------
// The bullets' hooks
// ---------------------------------------------------------------------------

void ULegendarySubsystem::NoteFiring(FBulletShot& Shot)
{
	AWeaponBase* Weapon = Shot.Weapon.Get();
	const ELegendarySignature Signature = Weapon ? LegendaryRules::SignatureOf(Weapon->GetInstance()) : ELegendarySignature::None;
	if (Signature == ELegendarySignature::None)
	{
		return;
	}
	// The sights as the round leaves (a sniper's round lands a beat later, when the scope may be down already).
	const AActor* Holder = Weapon->GetOwner();
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	ULegendaryEffectComponent::For(*Weapon)->NoteShot(Now(), View ? View->GetAimAlpha() : 0.f);
	if (Signature == ELegendarySignature::Gravesilk)
	{
		// Its rounds trail pale, so the silk is seen leaving the gun.
		Shot.TracerColor = FMath::Lerp(Shot.TracerColor, SilkColor, SilkTracerTint);
	}
}

void ULegendarySubsystem::NoteLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing)
{
	const AWeaponBase* Weapon = Shot.Weapon.Get();
	if (!Weapon || LegendaryRules::SignatureOf(Weapon->GetInstance()) == ELegendarySignature::None)
	{
		Landed.Reset();
		return;
	}
	// The damage is dealt between this and the struck hook, at once: alive now and dead then is this round's kill.
	ACreatureBase* Creature = Cast<ACreatureBase>(Hit.GetActor());
	Landed = Creature;
	bLandedAlive = Creature && !IsDeadNow(Creature);
}

void ULegendarySubsystem::NoteStruck(const FBulletShot& Shot, const FHitResult& Hit, FBulletStrike& Strike)
{
	AWeaponBase* Weapon = Shot.Weapon.Get();
	ACreatureBase* Creature = Cast<ACreatureBase>(Hit.GetActor());
	const bool bWasAlive = Creature && Landed.Get() == Creature && bLandedAlive;
	Landed.Reset();
	const ELegendarySignature Signature = Weapon ? LegendaryRules::SignatureOf(Weapon->GetInstance()) : ELegendarySignature::None;
	if (Signature == ELegendarySignature::None || Shot.bBlank || !Creature)
	{
		return;
	}
	const bool bKilled = bWasAlive && IsDeadNow(Creature);
	if (Signature == ELegendarySignature::Gravesilk && !bKilled && Strike.Damage > 0.f && !IsDeadNow(Creature))
	{
		const LegendaryRules::ESilk Laid = LaySilk(*Creature);
		UBulletSubsystem* BulletSubsystem = Bullets.Get();
		if (Laid != LegendaryRules::ESilk::None && BulletSubsystem)
		{
			// A tuft of silk caught where it struck, thrown back off the body a little.
			FWeaponFX& FX = BulletSubsystem->GetEffects();
			FX.Initialize(GetWorld());
			const FVector Out = Hit.ImpactNormal.IsNearlyZero() ? -Shot.Direction : FVector(Hit.ImpactNormal);
			for (int32 Tuft = 0; Tuft < 2; ++Tuft)
			{
				FX.SpawnDustPuff(Hit.ImpactPoint + Out * 4.f, (Out * 30.f + FMath::VRand() * 15.f), SilkColor, 0.55f, 5.f, 16.f, 0.7f);
			}
		}
	}
	if (bKilled && Weapon)
	{
		NoteKill(*Weapon, *Creature);
	}
}

// ---------------------------------------------------------------------------
// Gravesilk
// ---------------------------------------------------------------------------

LegendaryRules::ESilk ULegendarySubsystem::LaySilk(ACreatureBase& Creature)
{
	if (Silk.Num() >= SilkSweepAt)
	{
		for (auto It = Silk.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid() || IsDeadNow(It.Key().Get()))
			{
				It.RemoveCurrent();
			}
		}
	}
	const double When = Now();
	LegendaryRules::FSilk& On = Silk.FindOrAdd(&Creature);
	const LegendaryRules::ESilk Result = LegendaryRules::LaySilk(On, When, Creature.IsBossCreature(), FLegendaryTuning::Live());
	if (Result == LegendaryRules::ESilk::Rooted)
	{
		Root(Creature, On.RootEnds);
	}
	return Result;
}

int32 ULegendarySubsystem::SilkOn(const ACreatureBase* Creature) const
{
	const LegendaryRules::FSilk* On = Creature ? Silk.Find(const_cast<ACreatureBase*>(Creature)) : nullptr;
	return On ? LegendaryRules::StacksAt(*On, Now(), FLegendaryTuning::Live()) : 0;
}

bool ULegendarySubsystem::IsRooted(const ACreatureBase* Creature) const
{
	const LegendaryRules::FSilk* On = Creature ? Silk.Find(const_cast<ACreatureBase*>(Creature)) : nullptr;
	return On && LegendaryRules::IsRooted(*On, Now());
}

void ULegendarySubsystem::Root(ACreatureBase& Creature, double Ends)
{
	const FLegendaryTuning Tuning = FLegendaryTuning::Live();
	const double When = Now();
	// Held by the slow at its floor (the strongest slow wins, so nothing weaker lets it go early), its stride stopped dead.
	const bool bFirst = Creature.FindComponentByClass<UMovementSlowComponent>() == nullptr;
	UMovementSlowComponent* Slow = UMovementSlowComponent::Apply(&Creature, Tuning.GravesilkRootShare, static_cast<float>(Ends - When));
	if (Slow && bFirst)
	{
		// A creature's brain sets its walking speed after physics each step it steers: the slow has its say after that
		// (CreatureHitReactionComponent does the same with the first slow it makes).
		Slow->SetTickGroup(TG_PostPhysics);
		Slow->AddTickPrerequisiteActor(&Creature);
	}
	if (UCharacterMovementComponent* Movement = Creature.GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	Rooted.RemoveAll([&Creature](const FRooted& Each) { return Each.Creature.Get() == &Creature || !Each.Creature.IsValid(); });
	Rooted.Add({ &Creature, When, Ends });

	const UCapsuleComponent* Capsule = Creature.GetCapsuleComponent();
	const float Half = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 60.f;
	const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 40.f;
	const FVector Feet = Creature.GetActorLocation() - FVector(0.0, 0.0, Half - 2.f);
	if (UBulletSubsystem* BulletSubsystem = Bullets.Get())
	{
		// The web thrown down round it: a pale ring out over the ground and silk puffing up off its back.
		FWeaponFX& FX = BulletSubsystem->GetEffects();
		FX.Initialize(GetWorld());
		FX.SpawnShockRing(Feet, FVector::UpVector, Radius * 0.5f, Radius * StrandReach * 1.3f, 4.f, SilkColor, 1.2f, 0.35f);
		for (int32 Tuft = 0; Tuft < 5; ++Tuft)
		{
			const FVector Way = FMath::VRand();
			FX.SpawnDustPuff(Creature.GetActorLocation() + Way * Radius * 0.6f, FVector(Way.X * 40.0, Way.Y * 40.0, 25.0), SilkColor, 0.5f, 8.f, 26.f, 0.9f);
		}
	}
	LooterSound::PlayAt(this, LooterSoundCue::SpiderDrop, Creature.GetActorLocation(), 0.8f, 1.15f);
	LooterSound::PlayAt(this, LooterSoundCue::SinkCreak, Creature.GetActorLocation(), 0.7f);
	UE_LOG(LogLooter, Verbose, TEXT("Gravesilk rooted %s for %.1f s."), *Creature.GetName(), Ends - When);
}

void ULegendarySubsystem::Tick(float DeltaTime)
{
	if (Rooted.IsEmpty())
	{
		return;
	}
	const double When = Now();
	for (int32 Index = Rooted.Num() - 1; Index >= 0; --Index)
	{
		const FRooted& Each = Rooted[Index];
		const ACreatureBase* Creature = Each.Creature.Get();
		if (!Creature || IsDeadNow(Creature) || When >= Each.Ends)
		{
			Rooted.RemoveAtSwap(Index);
			continue;
		}
		// Pulled taut as it lands, slackening off over its last fifth of a second.
		const float In = FMath::Clamp(static_cast<float>((When - Each.Starts) / 0.08), 0.f, 1.f);
		const float Out = FMath::Clamp(static_cast<float>((Each.Ends - When) / 0.2), 0.f, 1.f);
		DrawStrands(*Creature, FMath::Min(In, Out));
	}
}

void ULegendarySubsystem::DrawStrands(const ACreatureBase& Creature, float Strength) const
{
	UBulletSubsystem* BulletSubsystem = Bullets.Get();
	if (!BulletSubsystem || Strength <= 0.f)
	{
		return;
	}
	FWeaponFX& FX = BulletSubsystem->GetEffects();
	FX.Initialize(GetWorld());
	const UCapsuleComponent* Capsule = Creature.GetCapsuleComponent();
	const float Half = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 60.f;
	const float Radius = Capsule ? Capsule->GetScaledCapsuleRadius() : 40.f;
	const FVector Middle = Creature.GetActorLocation();
	const FVector Feet = Middle - FVector(0.0, 0.0, Half - 2.f);
	// Strands from its back out to the ground all round, fixed to its heading so they don't swim.
	const float Yaw = static_cast<float>(Creature.GetActorRotation().Yaw);
	for (int32 Strand = 0; Strand < StrandCount; ++Strand)
	{
		const float Angle = FMath::DegreesToRadians(Yaw + 17.f + Strand * (360.f / StrandCount));
		const FVector Way(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
		const FVector Ground = Feet + Way * Radius * StrandReach;
		const FVector Back = Middle + Way * Radius * 0.5f + FVector(0.0, 0.0, Half * 0.35f);
		FX.AddStreak(Ground, Back, StrandWidth, SilkColor, StrandIntensity * Strength);
	}
}

// ---------------------------------------------------------------------------
// Kills: Company Scrip and Lamplighter
// ---------------------------------------------------------------------------

void ULegendarySubsystem::NoteKill(AWeaponBase& Gun, ACreatureBase& Victim)
{
	ULegendaryEffectComponent* Legendary = ULegendaryEffectComponent::For(Gun);
	UWorld* World = GetWorld();
	switch (Legendary->GetSignature())
	{
	case ELegendarySignature::CompanyScrip:
		if (Legendary->NoteKill(Now()))
		{
			if (UBulletSubsystem* BulletSubsystem = Bullets.Get())
			{
				// Paid: gold glints rising off the body.
				FWeaponFX& FX = BulletSubsystem->GetEffects();
				FX.Initialize(World);
				FX.SpawnLootMotes(Victim.GetActorLocation(), 40.f, ScripGold, ScripGlints);
			}
		}
		break;
	case ELegendarySignature::Lamplighter:
	{
		// The one holding it (only the player carries guns) is the one it heals.
		APawn* Healer = Cast<APawn>(Gun.GetOwner());
		if (!Legendary->WasLastShotScoped() || !Healer || !World)
		{
			break;
		}
		// At its feet: the ground under where it fell (world-static, skipping volumes), else its capsule's foot.
		const UCapsuleComponent* Capsule = Victim.GetCapsuleComponent();
		const float Half = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 60.f;
		FVector Ground = Victim.GetActorLocation() - FVector(0.0, 0.0, Half);
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("LamplighterGround"), &Victim);
		FHitResult Hit;
		const FVector Middle = Victim.GetActorLocation();
		if (World->LineTraceSingleByObjectType(Hit, Middle + FVector(0.0, 0.0, 50.0), Middle - FVector(0.0, 0.0, GroundProbe),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			Ground = Hit.ImpactPoint;
		}
		LightLantern(Ground, Healer);
		break;
	}
	default:
		break;
	}
}

ALegendaryLantern* ULegendarySubsystem::LightLantern(const FVector& Ground, APawn* Healer)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	const FLegendaryTuning Tuning = FLegendaryTuning::Live();
	Lanterns.RemoveAll([](const TWeakObjectPtr<ALegendaryLantern>& Each) { return !Each.IsValid() || Each->IsActorBeingDestroyed(); });
	// The oldest goes out to make room (a streak of scoped kills keeps the newest burning by the fight).
	while (Lanterns.Num() >= Tuning.LamplighterMaxLanterns)
	{
		if (ALegendaryLantern* Oldest = Lanterns[0].Get())
		{
			Oldest->Destroy();
		}
		Lanterns.RemoveAt(0);
	}
	ALegendaryLantern* Lantern = ALegendaryLantern::Light(World, Ground, Healer, Tuning.LamplighterSeconds, Tuning.LamplighterRadius);
	if (Lantern)
	{
		Lanterns.Add(Lantern);
		UE_LOG(LogLooter, Verbose, TEXT("Lamplighter: a lantern lit at %s for %s."), *Ground.ToCompactString(), *GetNameSafe(Healer));
	}
	return Lantern;
}

bool ULegendarySubsystem::ClaimHeal(const APawn* Pawn, const ALegendaryLantern* Lantern)
{
	if (!Pawn)
	{
		return false;
	}
	FHealClaim& Claim = HealClaims.FindOrAdd(Pawn);
	if (Claim.Frame == GFrameCounter && Claim.Lantern.IsValid() && Claim.Lantern.Get() != Lantern)
	{
		return false;
	}
	Claim.Frame = GFrameCounter;
	Claim.Lantern = Lantern;
	return true;
}

int32 ULegendarySubsystem::NumLanterns() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ALegendaryLantern>& Each : Lanterns)
	{
		Count += Each.IsValid() && !Each->IsActorBeingDestroyed() ? 1 : 0;
	}
	return Count;
}
