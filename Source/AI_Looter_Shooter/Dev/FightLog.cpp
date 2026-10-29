// UFightLogSubsystem (FightLog.h): Looter.Fight.Log watches a fight against the player and counts it. The report is in
// FightLogReport.cpp.

#include "Dev/FightLog.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Shields/PlayerShieldComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The peak damage a second is taken over this many seconds. */
	constexpr double PeakWindowSeconds = 5.0;
}

UFightLogSubsystem* UFightLogSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UFightLogSubsystem>() : nullptr;
}

bool UFightLogSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return Super::ShouldCreateSubsystem(Outer);
#endif
}

bool UFightLogSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UFightLogSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFightLogSubsystem, STATGROUP_Tickables);
}

void UFightLogSubsystem::Deinitialize()
{
	UnbindPlayer();
	UnbindDirector();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// On, off, clear
// ---------------------------------------------------------------------------

void UFightLogSubsystem::Start()
{
	Reset();
	bOn = true;
	BindDirector();
	UE_LOG(LogLooter, Display, TEXT("FIGHT log: on."));
}

void UFightLogSubsystem::Stop()
{
	bOn = false;
	UnbindPlayer();
	UnbindDirector();
	UE_LOG(LogLooter, Display, TEXT("FIGHT log: off."));
}

void UFightLogSubsystem::Reset()
{
	Watched.Reset();
	DamageByKind.Reset();
	DamageTimes.Reset();
	EventsByName.Reset();
	StartedByKind.Reset();
	LandedByKind.Reset();
	FightStart = -1.0;
	LastDeath = -1.0;
	LastSeen = -1.0;
	LowestShare = 1.f;
	PeakDps = 0.f;
	TotalDamage = 0.f;
	TotalToHealth = 0.f;
	PlayerDeaths = 0;
	PeakFighting = 0;
	PeakMeleeTokens = 0;
	PeakRangedTokens = 0;
	if (bOn)
	{
		// The director may have been made since the last start (a new level): listen again.
		BindDirector();
	}
}

void UFightLogSubsystem::BindPlayer(UHealthComponent* Health)
{
	UnbindPlayer();
	PlayerHealth = Health;
	if (Health)
	{
		Health->OnDamaged.AddUniqueDynamic(this, &UFightLogSubsystem::HandlePlayerDamaged);
		Health->OnDeath.AddUniqueDynamic(this, &UFightLogSubsystem::HandlePlayerDeath);
	}
}

void UFightLogSubsystem::UnbindPlayer()
{
	if (UHealthComponent* Health = PlayerHealth.Get())
	{
		Health->OnDamaged.RemoveDynamic(this, &UFightLogSubsystem::HandlePlayerDamaged);
		Health->OnDeath.RemoveDynamic(this, &UFightLogSubsystem::HandlePlayerDeath);
	}
	PlayerHealth = nullptr;
}

void UFightLogSubsystem::BindDirector()
{
	UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(this);
	if (Director == BoundDirector.Get())
	{
		return;
	}
	UnbindDirector();
	if (Director)
	{
		EventHandle = Director->OnFightEvent.AddUObject(this, &UFightLogSubsystem::HandleFightEvent);
		BoundDirector = Director;
	}
	else
	{
		UE_LOG(LogLooter, Warning, TEXT("FIGHT log: no fight director in this world (Looter.Fight.Director 0?): damage and kills are logged, the turns and moves are not."));
	}
}

void UFightLogSubsystem::UnbindDirector()
{
	if (UFightDirectorSubsystem* Director = BoundDirector.Get())
	{
		Director->OnFightEvent.Remove(EventHandle);
	}
	EventHandle.Reset();
	BoundDirector = nullptr;
}

// ---------------------------------------------------------------------------
// What it sees
// ---------------------------------------------------------------------------

FString UFightLogSubsystem::KindOf(const AActor& Actor)
{
	// "BP_SpiderCreature_C" and "SpiderCreature" both read "Spider".
	FString Name = Actor.GetClass()->GetName();
	Name.RemoveFromStart(TEXT("BP_"));
	Name.RemoveFromEnd(TEXT("_C"));
	Name.RemoveFromEnd(TEXT("Creature"));
	return Name;
}

void UFightLogSubsystem::HandlePlayerDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!bOn)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (FightStart < 0.0)
	{
		FightStart = Now;
	}
	LastSeen = Now;
	// The ward may have taken it all: the health's own loss is what counts as a wound.
	const UHealthComponent* Health = PlayerHealth.Get();
	const float ToHealth = Health ? Health->GetLastHealthLoss() : Damage;
	FDamageIn& In = DamageByKind.FindOrAdd(DamageCauser ? KindOf(*DamageCauser) : FString(TEXT("Unknown")));
	In.Total += Damage;
	In.ToHealth += ToHealth;
	++In.Hits;
	TotalDamage += Damage;
	TotalToHealth += ToHealth;
	DamageTimes.Emplace(Now, Damage);
	// The worst stretch so far: the last few seconds' damage over the time the fight has run, at most the window.
	const double Span = FMath::Clamp(Now - FightStart, 1.0, PeakWindowSeconds);
	PeakDps = FMath::Max(PeakDps, static_cast<float>(WindowDamage(Now) / Span));
}

void UFightLogSubsystem::HandlePlayerDeath(AController* Killer)
{
	if (bOn)
	{
		++PlayerDeaths;
		UE_LOG(LogLooter, Display, TEXT("FIGHT the player died at %.1f s into the fight."), GetFightSeconds());
	}
}

void UFightLogSubsystem::HandleFightEvent(const ACreatureBase& Creature, EFightEvent Event, float Value)
{
	if (!bOn)
	{
		return;
	}
	++EventsByName.FindOrAdd(EventName(Event));
	if (Event == EFightEvent::AttackStarted)
	{
		++StartedByKind.FindOrAdd(KindOf(Creature));
	}
	else if (Event == EFightEvent::AttackLanded)
	{
		++LandedByKind.FindOrAdd(KindOf(Creature));
	}
}

float UFightLogSubsystem::WindowDamage(double Now) const
{
	float Sum = 0.f;
	for (const TPair<double, float>& Entry : DamageTimes)
	{
		if (Entry.Key >= Now - PeakWindowSeconds)
		{
			Sum += Entry.Value;
		}
	}
	return Sum;
}

void UFightLogSubsystem::Tick(float DeltaTime)
{
	if (bOn && GetWorld())
	{
		Poll(GetWorld()->GetTimeSeconds());
	}
}

void UFightLogSubsystem::Poll(double Now)
{
	UWorld* World = GetWorld();
	const APlayerController* Controller = World->GetFirstPlayerController();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
	if (Health != PlayerHealth.Get())
	{
		BindPlayer(Health);
	}
	if (!BoundDirector.IsValid())
	{
		BindDirector();
	}

	int32 Fighting = 0;
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		const ACreatureBase* Creature = *It;
		const UHealthComponent* CreatureHealth = Creature->FindComponentByClass<UHealthComponent>();
		FWatch* Watch = Watched.Find(Creature->GetUniqueID());
		if (!Watch)
		{
			if (Creature->IsDead())
			{
				continue;
			}
			Watch = &Watched.Add(Creature->GetUniqueID());
			Watch->Creature = Creature;
			Watch->Kind = KindOf(*Creature);
			Watch->LastHealth = CreatureHealth ? CreatureHealth->GetHealth() : 0.f;
		}
		if (Watch->Died >= 0.0)
		{
			continue;
		}
		// A promotion or a level set after it was seen changes what it is: the report names what it was when it died.
		Watch->Rank = Creature->GetRank();
		Watch->Level = Creature->Level;
		const float HealthNow = CreatureHealth ? CreatureHealth->GetHealth() : 0.f;
		if (Watch->FirstHit < 0.0 && (HealthNow < Watch->LastHealth - KINDA_SMALL_NUMBER || Creature->IsDead()))
		{
			Watch->FirstHit = Now;
		}
		Watch->LastHealth = HealthNow;
		if (Creature->IsDead())
		{
			Watch->Died = Now;
			LastDeath = Now;
			continue;
		}
		if (Creature->IsFightingWith(Pawn))
		{
			++Fighting;
		}
	}

	if (Fighting > 0)
	{
		if (FightStart < 0.0)
		{
			FightStart = Now;
		}
		LastSeen = Now;
	}
	PeakFighting = FMath::Max(PeakFighting, Fighting);
	if (const UFightDirectorSubsystem* Director = BoundDirector.Get())
	{
		PeakMeleeTokens = FMath::Max(PeakMeleeTokens, Director->CountTokens(*Pawn, EFightRole::Melee));
		PeakRangedTokens = FMath::Max(PeakRangedTokens, Director->CountTokens(*Pawn, EFightRole::Ranged));
	}

	if (FightStart >= 0.0 && Health)
	{
		// Health and ward together, as the player's last stand is (a ward is health that comes back).
		float Current = Health->GetHealth();
		float Most = Health->GetMaxHealth();
		if (const UPlayerShieldComponent* Ward = UPlayerShieldComponent::Find(Pawn))
		{
			if (Ward->HasWard())
			{
				Current += Ward->GetCharge();
				Most += Ward->GetCapacity();
			}
		}
		if (Most > KINDA_SMALL_NUMBER)
		{
			LowestShare = FMath::Min(LowestShare, Current / Most);
		}
	}

	// Entries older than the window are no use to the peak.
	DamageTimes.RemoveAll([Now](const TPair<double, float>& Entry) { return Entry.Key < Now - PeakWindowSeconds; });
}

double UFightLogSubsystem::GetFightSeconds() const
{
	if (FightStart < 0.0)
	{
		return 0.0;
	}
	return FMath::Max(FMath::Max(LastDeath, LastSeen) - FightStart, 0.0);
}

int32 UFightLogSubsystem::GetKills() const
{
	int32 Kills = 0;
	for (const TPair<uint32, FWatch>& Pair : Watched)
	{
		Kills += Pair.Value.Died >= 0.0 ? 1 : 0;
	}
	return Kills;
}

int32 UFightLogSubsystem::CountEvents(EFightEvent Event) const
{
	const int32* Count = EventsByName.Find(EventName(Event));
	return Count ? *Count : 0;
}

FString UFightLogSubsystem::EventName(EFightEvent Event)
{
	return FightEventName(Event);
}

// ---------------------------------------------------------------------------
// The console command
// ---------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING

namespace
{
	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void FightLogCommand(const TArray<FString>& Args, UWorld* World)
	{
		UFightLogSubsystem* Log = UFightLogSubsystem::Get(FindGameWorld(World));
		if (!Log)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Fight.Log: no game running."));
			return;
		}
		const FString What = Args.Num() > 0 ? Args[0] : FString(TEXT("report"));
		if (What == TEXT("1") || What.Equals(TEXT("on"), ESearchCase::IgnoreCase))
		{
			Log->Start();
		}
		else if (What == TEXT("0") || What.Equals(TEXT("off"), ESearchCase::IgnoreCase))
		{
			Log->Stop();
		}
		else if (What.Equals(TEXT("reset"), ESearchCase::IgnoreCase))
		{
			Log->Reset();
			UE_LOG(LogLooter, Display, TEXT("FIGHT log: cleared."));
		}
		else if (What.Equals(TEXT("report"), ESearchCase::IgnoreCase))
		{
			Log->Report(true);
		}
		else
		{
			UE_LOG(LogLooter, Warning, TEXT("Usage: Looter.Fight.Log 0|1|report|reset"));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs FightLogConsoleCommand(
		TEXT("Looter.Fight.Log"),
		TEXT("Looter.Fight.Log 0|1|report|reset: watches a fight against the player. 1 starts it (clearing what it saw), 0 stops it, report logs how ")
		TEXT("the fight went (length, damage in by kind and its worst 5 s, the lowest health, deaths, attacks landed, kill times by kind and rank, ")
		TEXT("the new moves) and writes Saved/FightLog/<time>.csv, reset clears it. Looter.Fight.Sim starts it for a measured fight."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&FightLogCommand));
}

#endif
