// ADredgerHoseMan: his tank, the weak spot on his back. Every round on it is a crit; once those have taken a share of his
// health (Looter.Dredger.Tank.Share, 30%) it bursts: a big soul-light pop with a shock ring, smog and motes, the view
// shaken, a crack heard across the fight; the creatures round him hurt by it, him staggered and hurt again, the hose torn
// off. He fights on with the nozzle as a club, quicker and angrier.

#include "Creatures/DredgerHoseMan.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/DredgerHoseComponent.h"
#include "Creatures/DredgerLook.h"
#include "Creatures/DredgerRules.h"
#include "Creatures/DredgerTuning.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/VfxTypes.h"
#include "Weapons/WeaponFX.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	/** Burst, he chases this much faster. */
	constexpr float BurstChaseScale = 1.25f;
	/** His club's start range once the tank is gone (cm, full size). */
	constexpr float BurstAttackRange = 220.f;
	/** The view's shake at the burst: strength, length (s), reach (cm). */
	constexpr float BurstShake = 0.45f;
	constexpr float BurstShakeSeconds = 0.5f;
	constexpr float BurstShakeReach = 2500.f;
}

float ADredgerHoseMan::NoteTankHit(float Lost, AController* By)
{
	if (bTankBurst || IsDead())
	{
		return TankDamage;
	}
	TankDamage += FMath::Max(Lost, 0.f);
	if (DredgerRules::TankBursts(TankDamage, Health->GetMaxHealth(), FDredgerTuning::Live()))
	{
		BurstTank(By);
	}
	return TankDamage;
}

void ADredgerHoseMan::BurstTank(AController* By)
{
	if (bTankBurst || IsDead())
	{
		return;
	}
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	PlayTankBurst(By);
	bTankBurst = true;
	Tank->SetVisibility(false);
	TankGauge->SetVisibility(false);
	Tank->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TankGauge->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// The line goes with it, and the feed hose with nothing to feed.
	Hose->SetTorn(true);
	// Thrown off his feet by his own tank: staggered (his rank's share of it), and his turn given up.
	HitReaction->Jolt(Tuning.TankStagger, GetActorForwardVector());
	// He fights on with the nozzle: nearer, quicker.
	AttackRange = BurstAttackRange;
	ChaseSpeed = ClassChaseSpeed * BurstChaseScale;
	UE_LOG(LogLooter, Log, TEXT("%s's tank burst (%.0f of %.0f health on it)."), *GetName(), TankDamage, Health->GetMaxHealth());

	// What the burst does to him and to those round him lands a frame on, never inside the hit that burst it (its damage is
	// still being handed out).
	const TWeakObjectPtr<AController> Blame(By);
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, Blame]()
	{
		const FDredgerTuning Later = FDredgerTuning::Live();
		AController* Who = Blame.Get();
		const FVector Center = GetActorLocation();
		if (!IsDead() && Later.TankSelfShare > 0.f)
		{
			UGameplayStatics::ApplyDamage(this, Health->GetMaxHealth() * Later.TankSelfShare, Who, this, UDamageType::StaticClass());
		}
		if (Later.TankRadius <= 0.f || Later.TankSplashShare <= 0.f)
		{
			return;
		}
		for (TActorIterator<ACreatureBase> It(GetWorld()); It; ++It)
		{
			ACreatureBase* Other = *It;
			if (!Other || Other == this || Other->IsDead())
			{
				continue;
			}
			const float Distance = static_cast<float>(FVector::Dist(Other->GetActorLocation(), Center));
			const UHealthComponent* OtherHealth = Other->FindComponentByClass<UHealthComponent>();
			if (Distance > Later.TankRadius || !OtherHealth)
			{
				continue;
			}
			// Full at its heart, half at its edge.
			const float Falloff = FMath::Lerp(1.f, 0.5f, Distance / Later.TankRadius);
			UGameplayStatics::ApplyDamage(Other, OtherHealth->GetMaxHealth() * Later.TankSplashShare * Falloff, Who, this, UDamageType::StaticClass());
		}
	}));
}

void ADredgerHoseMan::PlayTankBurst(AController* By)
{
	const FDredgerTuning Tuning = FDredgerTuning::Live();
	const FVector Center = Tank->Bounds.Origin;
	LooterSound::PlayAt(this, LooterSoundCue::DredgerTankBurst, Center);
	BossCameraShake::Kick(this, Center, BurstShake, BurstShakeSeconds, BurstShakeReach);
	UBulletSubsystem* Bullets = GetWorld() ? GetWorld()->GetSubsystem<UBulletSubsystem>() : nullptr;
	if (!Bullets)
	{
		return;
	}
	FWeaponFX& Effects = Bullets->GetEffects();
	Effects.Initialize(GetWorld());
	// The soul in the tank let go at once: a cyan blast, a ring across the ground, motes and torn souls, smog rolling off.
	FVfxExplosion Blast;
	Blast.Center = Center;
	Blast.Up = FVector::UpVector;
	Blast.Radius = FMath::Max(Tuning.TankRadius, 150.f);
	Blast.Fire = DredgerLook::SoulCyan;
	Blast.bSoul = true;
	Blast.Ground = EImpactMaterial::Unknown;
	Blast.bOnGround = false;
	Effects.SpawnExplosion(Blast);
	Effects.SpawnShockRing(Center, FVector::UpVector, 30.f, Blast.Radius * 1.2f, 18.f, DredgerLook::SoulCyan, 6.f, 0.45f);
	Effects.SpawnSoulTatters(Center, FVector::UpVector, DredgerLook::SoulCyan, 90.f, 10);
	for (int32 Puff = 0; Puff < 5; ++Puff)
	{
		const FVector Drift(FMath::FRandRange(-120.f, 120.f), FMath::FRandRange(-120.f, 120.f), FMath::FRandRange(60.f, 160.f));
		Effects.SpawnDustPuff(Center, Drift, DredgerLook::Smog, 0.8f, 40.f, 160.f, 2.2f);
	}
}
