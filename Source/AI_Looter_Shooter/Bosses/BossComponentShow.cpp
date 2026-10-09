#include "Bosses/BossComponent.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Bosses/BossLootShower.h"
#include "Bosses/BossRules.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Loot/LootDropComponent.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

// UBossComponent's show (the bar sweeping in, the stings and cries, the camera's shakes, the slow beat at the death), its
// weak spot's stagger and its loot shower. The fight's course is in BossComponent.cpp.

#define LOCTEXT_NAMESPACE "LooterBoss"

namespace
{
	/** How long each moment's shake dies away over (s). */
	constexpr float IntroShakeSeconds = 0.7f;
	constexpr float PhaseShakeSeconds = 0.8f;
	constexpr float StaggerShakeSeconds = 0.45f;
	constexpr float DeathShakeSeconds = 1.4f;

	/** A crit that would stagger a boss busy with something it can't break off leaves it this near: one more does it. */
	constexpr float BusyBuildUp = 0.95f;

	/**
	 * The world's clock at a boss's death belongs to the world, not to a boss: two bosses dying together would otherwise each
	 * set it and each put it back to full speed under the other. A dying boss claims a speed; the world runs at the slowest
	 * claimed, and at full speed again once the last claim is let go.
	 */
	TMap<TWeakObjectPtr<UWorld>, TMap<const UBossComponent*, float>> DeathSlowClaims;

	void ApplyDeathSlow(UWorld* World)
	{
		const TMap<const UBossComponent*, float>* Claims = DeathSlowClaims.Find(World);
		float Dilation = 1.f;
		if (Claims)
		{
			for (const TPair<const UBossComponent*, float>& Claim : *Claims)
			{
				Dilation = FMath::Min(Dilation, Claim.Value);
			}
		}
		if (World->GetWorldSettings())
		{
			UGameplayStatics::SetGlobalTimeDilation(World, Dilation);
		}
	}

	/** Claims the clock at this speed for Boss (a claim it already has changes to it). */
	void ClaimDeathSlow(UWorld* World, const UBossComponent* Boss, float Dilation)
	{
		// Worlds that have gone (a play session ended) don't stay in the list.
		for (auto It = DeathSlowClaims.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid())
			{
				It.RemoveCurrent();
			}
		}
		DeathSlowClaims.FindOrAdd(World).Add(Boss, Dilation);
		ApplyDeathSlow(World);
	}

	/** Lets Boss's claim go: the world speeds up to the slowest still claimed, or to full speed when none is. */
	void ReleaseDeathSlow(UWorld* World, const UBossComponent* Boss)
	{
		TMap<const UBossComponent*, float>* Claims = DeathSlowClaims.Find(World);
		if (!Claims)
		{
			return;
		}
		Claims->Remove(Boss);
		if (Claims->IsEmpty())
		{
			DeathSlowClaims.Remove(World);
		}
		ApplyDeathSlow(World);
	}
}

// ---------------------------------------------------------------------------
// The show
// ---------------------------------------------------------------------------

void UBossComponent::PlayCue(FName Cue) const
{
	const ACreatureBase* Creature = GetCreature();
	if (!Cue.IsNone() && Creature)
	{
		LooterSound::PlayAttached(Cue, Creature->GetRootComponent());
	}
}

void UBossComponent::Shake(float Strength, float Seconds) const
{
	if (const ACreatureBase* Creature = GetCreature())
	{
		BossCameraShake::Kick(this, Creature->GetActorLocation(), Strength, Seconds);
	}
}

void UBossComponent::PlayIntro()
{
	// The bar sweeps in, its fill running up from empty, the title spelled out under it before the first phase's name.
	if (Bar)
	{
		Bar->SetTitle(Show.Title);
		Bar->PlayIntro();
	}
	LooterSound::Play2D(this, LooterSoundCue::BossIntro);
	PlayCue(Show.IntroCue);
	Shake(Show.IntroShake, IntroShakeSeconds);
}

void UBossComponent::PlayPhaseTell()
{
	LooterSound::Play2D(this, LooterSoundCue::BossPhase);
	PlayCue(Show.PhaseCue);
	Shake(Show.PhaseShake, PhaseShakeSeconds);
}

void UBossComponent::PlayDeath(bool bWasFighting)
{
	if (bWasFighting)
	{
		// The killing blow lands slow, the body falls, the bar empties with its last word.
		StartDeathSlow();
		Shake(Show.DeathShake, DeathShakeSeconds);
		LooterSound::Play2D(this, LooterSoundCue::BossDeath);
		if (Bar && bBarWanted)
		{
			Bar->PlayDefeated(Show.DefeatedLine);
		}
	}
	PlayCue(Show.DeathCue);
	ThrowLoot();
}

// ---------------------------------------------------------------------------
// The slow beat at its death
// ---------------------------------------------------------------------------

void UBossComponent::StartDeathSlow()
{
	UWorld* World = GetWorld();
	// Game worlds only (a test level's clock is the tests'), once, and only with a beat to play.
	if (!World || !World->IsGameWorld() || !World->GetWorldSettings() || bDeathSlowOn || Show.DeathSlowSeconds <= 0.f
		|| Show.DeathSlowMo >= 0.999f)
	{
		return;
	}
	const float Slow = FMath::Clamp(Show.DeathSlowMo, 0.05f, 1.f);
	ClaimDeathSlow(World, this, Slow);
	bDeathSlowOn = true;
	// Timers run on the slowed clock: two thirds of the beat (in real seconds) held slow, the rest half the way back.
	World->GetTimerManager().SetTimer(DeathSlowTimer, FTimerDelegate::CreateUObject(this, &UBossComponent::EaseDeathSlow),
		FMath::Max(Show.DeathSlowSeconds * (2.f / 3.f) * Slow, 0.01f), false);
	UE_LOG(LogLooter, Log, TEXT("Boss %s: the world slows to %.2f for its death."), *GetLabel(), Slow);
}

void UBossComponent::EaseDeathSlow()
{
	UWorld* World = GetWorld();
	if (!bDeathSlowOn || !World || !World->GetWorldSettings())
	{
		EndDeathSlow();
		return;
	}
	const float Half = FMath::Lerp(FMath::Clamp(Show.DeathSlowMo, 0.05f, 1.f), 1.f, 0.5f);
	ClaimDeathSlow(World, this, Half);
	World->GetTimerManager().SetTimer(DeathSlowTimer, FTimerDelegate::CreateUObject(this, &UBossComponent::EndDeathSlow),
		FMath::Max(Show.DeathSlowSeconds * (1.f / 3.f) * Half, 0.01f), false);
}

void UBossComponent::EndDeathSlow()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(DeathSlowTimer);
	}
	if (!bDeathSlowOn)
	{
		return;
	}
	bDeathSlowOn = false;
	// Back to full speed unless another dying boss still holds the clock slow (nothing else in the game holds the world's clock
	// slowed for long; a hit's stop is shorter).
	if (World)
	{
		ReleaseDeathSlow(World, this);
	}
}

// ---------------------------------------------------------------------------
// The loot shower
// ---------------------------------------------------------------------------

void UBossComponent::TakeOverLoot()
{
	ULootDropComponent* Loot = GetOwner() ? GetOwner()->FindComponentByClass<ULootDropComponent>() : nullptr;
	if (bLootTakenOver || !LootShower.bEnabled || !Loot || !Loot->bDropOnDeath)
	{
		return;
	}
	// Its table, level and luck stay the loot drop component's; only the throw is the shower's.
	Loot->bDropOnDeath = false;
	bLootTakenOver = true;
}

void UBossComponent::ThrowLoot()
{
	ACreatureBase* Creature = GetCreature();
	if (!bLootTakenOver || !Creature)
	{
		return;
	}
	bLootTakenOver = false;
	// Turned off after play began (the tests): nothing drops.
	if (!LootShower.bEnabled)
	{
		return;
	}
	const float Half = Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Middle = LootShower.bFromSpot ? Creature->GetHome().GetLocation() : Creature->GetActorLocation();
	ABossLootShower::Throw(*Creature, Middle - FVector(0.0, 0.0, Half), LootShower);
}

// ---------------------------------------------------------------------------
// The weak spot
// ---------------------------------------------------------------------------

void UBossComponent::AddCritDamage(float Damage)
{
	const UHealthComponent* Health = GetBossHealth();
	if (!bFighting || bUntargetable || bStaggered || StaggerCooldownLeft > 0.f || Stagger.CritShare <= 0.f || !Health
		|| Health->IsDead() || Health->GetHealth() <= 0.f)
	{
		return;
	}
	StaggerBuildUp = BossRules::AddCritToStagger(StaggerBuildUp, Damage, Health->GetMaxHealth(), Stagger);
	if (StaggerBuildUp >= 1.f && !BeginStagger())
	{
		StaggerBuildUp = BusyBuildUp;
	}
}

void UBossComponent::TickStagger(float DeltaSeconds)
{
	if (bStaggered)
	{
		StaggerTimeLeft -= DeltaSeconds;
		if (StaggerTimeLeft <= 0.f)
		{
			EndStagger();
		}
		return;
	}
	if (StaggerCooldownLeft > 0.f)
	{
		StaggerCooldownLeft = FMath::Max(0.f, StaggerCooldownLeft - DeltaSeconds);
		return;
	}
	StaggerBuildUp = BossRules::DrainStagger(StaggerBuildUp, DeltaSeconds, Stagger);
}

bool UBossComponent::BeginStagger()
{
	const ACreatureBase* Creature = GetCreature();
	const UHealthComponent* Health = GetBossHealth();
	if (!bFighting || bStaggered || bUntargetable || !Creature || Creature->IsDead() || !Health || Health->GetHealth() <= 0.f)
	{
		return false;
	}
	if (CanStagger.IsBound() && !CanStagger.Execute())
	{
		return false;
	}
	bStaggered = true;
	StaggerTimeLeft = FMath::Max(Stagger.Seconds, 0.2f);
	StaggerBuildUp = 0.f;
	UE_LOG(LogLooter, Log, TEXT("Boss %s: staggered for %.1f s."), *GetLabel(), StaggerTimeLeft);
	LooterSound::PlayAttached(LooterSoundCue::BossStagger, Creature->GetRootComponent());
	PlayCue(Show.StaggerCue);
	Shake(Show.StaggerShake, StaggerShakeSeconds);
	if (Bar && bBarWanted)
	{
		Bar->ShowCallout(Show.StaggerCallout.IsEmpty() ? LOCTEXT("Staggered", "Staggered") : Show.StaggerCallout, StaggerTimeLeft);
	}
	OnStaggered.Broadcast(true);
	return true;
}

void UBossComponent::EndStagger()
{
	if (!bStaggered)
	{
		return;
	}
	bStaggered = false;
	StaggerTimeLeft = 0.f;
	StaggerBuildUp = 0.f;
	// It can't be kept down: nothing builds up for a while.
	StaggerCooldownLeft = Stagger.Cooldown;
	OnStaggered.Broadcast(false);
}

// ---------------------------------------------------------------------------
// Adds the boss's own code raised, and adds dying with it
// ---------------------------------------------------------------------------

void UBossComponent::AdoptAdd(ACreatureBase* Add)
{
	if (Add && Add != GetOwner() && !Adds.Contains(Add))
	{
		Adds.Add(Add);
	}
}

void UBossComponent::KillAdds()
{
	// As a death, not a fade: they fall and sink as any creature does (no one's kill: no experience).
	ClearShots();
	for (ACreatureBase* Add : GetAliveAdds())
	{
		if (const UHealthComponent* AddHealth = Add->FindComponentByClass<UHealthComponent>())
		{
			UGameplayStatics::ApplyDamage(Add, AddHealth->GetMaxHealth() * 10.f + 1.f, nullptr, GetOwner(), UDamageType::StaticClass());
		}
	}
	Adds.Reset();
}

#undef LOCTEXT_NAMESPACE
