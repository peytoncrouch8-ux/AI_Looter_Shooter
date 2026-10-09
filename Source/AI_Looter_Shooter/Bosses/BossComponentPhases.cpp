#include "Bosses/BossComponent.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossRules.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

// UBossComponent's phases, their events, untargetable spells and volleys. The fight's course is in BossComponent.cpp.

namespace
{
	/** A repeating event fires at most this often (seconds), whatever its data says, so a mistyped 0.01 can't flood the frame. */
	constexpr float ShortestRepeat = 0.25f;
	/** A volley's tell glows this much bigger than one of its pellets. */
	constexpr float ChargeSize = 1.8f;
}

// ---------------------------------------------------------------------------
// Phases
// ---------------------------------------------------------------------------

void UBossComponent::EvaluatePhase()
{
	if (!bFighting)
	{
		return;
	}
	const UHealthComponent* BossHealth = GetBossHealth();
	// A killing blow wins the fight (HandleBossDeath); the phases it passed on the way down never start.
	if (!BossHealth || BossHealth->IsDead() || BossHealth->GetHealth() <= 0.f)
	{
		return;
	}
	const int32 Wanted = BossRules::PhaseAt(FightPhases, BossHealth->GetHealthPercent());
	// One at a time, in order: a big hit across two lines still starts the phase between (its spell, its adds).
	while (bFighting && Phase < Wanted)
	{
		EnterPhase(Phase + 1);
	}
}

void UBossComponent::EnterPhase(int32 NewPhase)
{
	if (!FightPhases.IsValidIndex(NewPhase))
	{
		return;
	}
	const int32 OldPhase = Phase;
	Phase = NewPhase;
	PhaseTime = 0.f;
	// The last phase's events end with it: repeats stop, and a volley still winding up fizzles. A spell already cast plays
	// out (its adds are still there to kill).
	bVolleyPending = false;
	Scheduled.Reset();
	const TArray<FBossPhaseEvent>& Events = FightPhases[Phase].Events;
	for (int32 Index = 0; Index < Events.Num(); ++Index)
	{
		FScheduledEvent& Entry = Scheduled.AddDefaulted_GetRef();
		Entry.Event = Index;
		Entry.NextTime = FMath::Max(Events[Index].Delay, 0.f);
	}
	const UHealthComponent* BossHealth = GetBossHealth();
	UE_LOG(LogLooter, Log, TEXT("Boss %s: phase %d of %d \"%s\" at %.0f%% health."), *GetLabel(), Phase + 1, FightPhases.Num(),
		*FightPhases[Phase].Name.ToString(), BossHealth ? BossHealth->GetHealthPercent() * 100.f : 0.f);
	OnPhaseChanged.Broadcast(Phase, OldPhase);
	UpdateBar();
	// A later phase is announced: the sting, the boss's cry, the shake (the bar flashes as its phase moves on).
	if (OldPhase != INDEX_NONE && Phase > OldPhase)
	{
		PlayPhaseTell();
	}
	// What starts with the phase happens at once.
	RunDueEvents();
}

void UBossComponent::RunDueEvents()
{
	if (!FightPhases.IsValidIndex(Phase))
	{
		return;
	}
	const int32 RunningPhase = Phase;
	for (int32 Index = 0; Index < Scheduled.Num(); ++Index)
	{
		if (Scheduled[Index].bDone || PhaseTime < Scheduled[Index].NextTime)
		{
			continue;
		}
		// Copied, and its next time set before it runs: what it does may start the next phase (which clears the list) or end
		// the fight.
		const FBossPhaseEvent Event = FightPhases[Phase].Events[Scheduled[Index].Event];
		Scheduled[Index].bFired = true;
		if (Event.RepeatEvery > 0.f)
		{
			// After a hitch it picks up from now rather than firing every repeat it missed.
			Scheduled[Index].NextTime = FMath::Max(Scheduled[Index].NextTime + FMath::Max(Event.RepeatEvery, ShortestRepeat), PhaseTime);
		}
		else
		{
			Scheduled[Index].bDone = true;
		}
		RunEvent(Event);
		if (!bFighting || Phase != RunningPhase)
		{
			return;
		}
	}
}

void UBossComponent::RunEvent(const FBossPhaseEvent& Event)
{
	switch (Event.Kind)
	{
	case EBossEventKind::AddWave:
		SpawnWave(Event.Wave);
		break;
	case EBossEventKind::Untargetable:
		BeginUntargetable(Event.Untargetable);
		break;
	case EBossEventKind::Volley:
		FireVolley(Event.Volley);
		break;
	case EBossEventKind::Custom:
		UE_LOG(LogLooter, Verbose, TEXT("Boss %s: %s."), *GetLabel(), *Event.Name.ToString());
		OnCustomEvent.Broadcast(Event.Name);
		break;
	}
}

bool UBossComponent::HasPendingWaves() const
{
	if (!FightPhases.IsValidIndex(Phase))
	{
		return false;
	}
	for (const FScheduledEvent& Entry : Scheduled)
	{
		const TArray<FBossPhaseEvent>& Events = FightPhases[Phase].Events;
		if (!Entry.bFired && Events.IsValidIndex(Entry.Event) && Events[Entry.Event].Kind == EBossEventKind::AddWave)
		{
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// Untargetable
// ---------------------------------------------------------------------------

void UBossComponent::BeginUntargetable(const FBossUntargetable& Spell)
{
	ACreatureBase* Creature = GetCreature();
	UHealthComponent* BossHealth = GetBossHealth();
	if (!Creature || !BossHealth || Creature->IsDead())
	{
		return;
	}
	const bool bWasUntargetable = bUntargetable;
	if (!bUntargetable)
	{
		bUntargetable = true;
		bSavedInvulnerable = BossHealth->bInvulnerable;
		bSavedShowNumbers = BossHealth->bShowDamageNumbers;
	}
	ActiveSpell = Spell;
	SpellTime = 0.f;
	// Hits land and take nothing, with no numbers to say otherwise: the grey bar says it.
	BossHealth->bInvulnerable = true;
	BossHealth->bShowDamageNumbers = false;
	if (Spell.bWithdraw && !bWithdrawn)
	{
		bWithdrawn = true;
		bSavedPassive = Creature->IsPassive();
		Creature->SetPassive(true);
	}
	UE_LOG(LogLooter, Log, TEXT("Boss %s: can't be hurt (%s)."), *GetLabel(),
		Spell.bUntilAddsDie ? TEXT("until its adds are dead") : *FString::Printf(TEXT("%.0f s"), Spell.Seconds));
	UpdateBar();
	if (!bWasUntargetable)
	{
		OnUntargetableChanged.Broadcast(true);
	}
}

void UBossComponent::EndUntargetable()
{
	StopUntargetable(/*bRejoin*/ true);
}

void UBossComponent::StopUntargetable(bool bRejoin)
{
	if (!bUntargetable)
	{
		return;
	}
	bUntargetable = false;
	if (UHealthComponent* BossHealth = GetBossHealth())
	{
		BossHealth->bInvulnerable = bSavedInvulnerable;
		BossHealth->bShowDamageNumbers = bSavedShowNumbers;
	}
	ACreatureBase* Creature = GetCreature();
	if (bWithdrawn && Creature)
	{
		bWithdrawn = false;
		Creature->SetPassive(bSavedPassive);
		// Back into the fight at once, rather than when it next happens to see the player.
		if (bRejoin && !bSavedPassive)
		{
			Creature->AlertTo(FightPlayer.Get());
		}
	}
	ActiveSpell = FBossUntargetable();
	SpellTime = 0.f;
	UE_LOG(LogLooter, Log, TEXT("Boss %s: can be hurt again."), *GetLabel());
	UpdateBar();
	OnUntargetableChanged.Broadcast(false);
}

void UBossComponent::TickUntargetable(float DeltaSeconds)
{
	if (!bUntargetable)
	{
		return;
	}
	SpellTime += DeltaSeconds;
	if (BossRules::UntargetableEnds(ActiveSpell, SpellTime, NumAliveAdds(), HasPendingWaves()))
	{
		EndUntargetable();
	}
}

// ---------------------------------------------------------------------------
// Volleys
// ---------------------------------------------------------------------------

void UBossComponent::FireVolley(const FBossVolley& Volley)
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature || Creature->IsDead() || bVolleyPending)
	{
		return;
	}
	PendingVolley = Volley;
	bVolleyPending = true;
	VolleyTimeLeft = Volley.WindupSeconds;
	if (Volley.WindupSeconds <= 0.f)
	{
		ReleaseVolley();
		return;
	}
	// The tell: a glow swelling at its muzzle, following it as it moves.
	UWorld* World = GetWorld();
	if (UEnemyProjectileSubsystem* Shots = World ? World->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr)
	{
		Shots->ShowCharge(Creature, Volley.Muzzle, Volley.WindupSeconds, Volley.Radius * ChargeSize, Volley.Color);
	}
}

void UBossComponent::TickVolley(float DeltaSeconds)
{
	if (!bVolleyPending)
	{
		return;
	}
	VolleyTimeLeft -= DeltaSeconds;
	if (VolleyTimeLeft <= 0.f)
	{
		ReleaseVolley();
	}
}

void UBossComponent::ReleaseVolley()
{
	bVolleyPending = false;
	ACreatureBase* Creature = GetCreature();
	const APawn* Fighter = FightPlayer.Get();
	UWorld* World = GetWorld();
	UEnemyProjectileSubsystem* Shots = World ? World->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr;
	if (!Creature || Creature->IsDead() || !Fighter || !Shots)
	{
		return;
	}
	// From its muzzle (its own frame, so it scales with it) at where the player is now: slow enough to step out of.
	const FVector Muzzle = Creature->GetActorTransform().TransformPosition(PendingVolley.Muzzle);
	FEnemyShot Pellet;
	Pellet.Start = Muzzle;
	Pellet.Speed = PendingVolley.Speed;
	Pellet.Range = PendingVolley.Range;
	Pellet.Radius = PendingVolley.Radius;
	// A share of its bite, which its level and rank have grown already.
	Pellet.Damage = Creature->AttackDamage * PendingVolley.DamageShare;
	Pellet.Color = PendingVolley.Color;
	Pellet.Shooter = Creature;
	Pellet.Instigator = Creature->GetController();
	Shots->FireVolley(Pellet, Fighter->GetActorLocation() - Muzzle, PendingVolley.Pellets, PendingVolley.SpreadDegrees);
}

void UBossComponent::ClearShots()
{
	UWorld* World = GetWorld();
	UEnemyProjectileSubsystem* Shots = World ? World->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr;
	if (!Shots)
	{
		return;
	}
	Shots->ClearShotsFrom(GetOwner());
	for (const TWeakObjectPtr<ACreatureBase>& Add : Adds)
	{
		Shots->ClearShotsFrom(Add.Get());
	}
}
