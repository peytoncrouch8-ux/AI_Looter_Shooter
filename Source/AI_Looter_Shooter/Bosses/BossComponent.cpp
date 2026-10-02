#include "Bosses/BossComponent.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/BossRules.h"
#include "Bosses/BossSeal.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

// UBossComponent: how a fight starts, ends, starts over and is won, frame by frame. Its wall and bar are in
// BossComponentArena.cpp, phases, spells and volleys in BossComponentPhases.cpp, the adds in BossComponentAdds.cpp.

namespace
{
	/** Someone a boss fights: a living character that isn't a creature (the player, or a test's stand-in). */
	bool IsLivingPlayer(const APawn* Pawn)
	{
		if (!Pawn || !Pawn->IsA<ACharacter>() || Pawn->IsA<ACreatureBase>())
		{
			return false;
		}
		const UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
		return Health && !Health->IsDead();
	}
}

FString UBossComponent::GetLabel() const
{
	return BossName.IsEmpty() ? GetNameSafe(GetOwner()) : BossName.ToString();
}

UBossComponent::UBossComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

ACreatureBase* UBossComponent::GetCreature() const
{
	return Cast<ACreatureBase>(GetOwner());
}

UHealthComponent* UBossComponent::GetBossHealth() const
{
	const AActor* Boss = GetOwner();
	return Boss ? Boss->FindComponentByClass<UHealthComponent>() : nullptr;
}

APawn* UBossComponent::GetFightPlayer() const
{
	return FightPlayer.Get();
}

UHudBossBarWidget* UBossComponent::GetBar() const
{
	return Bar;
}

void UBossComponent::BeginPlay()
{
	Super::BeginPlay();
	UHealthComponent* BossHealth = GetBossHealth();
	if (!GetCreature() || !BossHealth)
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: a boss component only works on a creature with health; it does nothing here."), *GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}
	BossHealth->OnDamaged.AddUniqueDynamic(this, &UBossComponent::HandleBossDamaged);
	BossHealth->OnHealthChanged.AddUniqueDynamic(this, &UBossComponent::HandleBossHealthChanged);
	BossHealth->OnDeath.AddUniqueDynamic(this, &UBossComponent::HandleBossDeath);
	FightPhases = BossRules::Ordered(Phases);
	// Until its fight starts it waits at its spot and hunts nobody: a boss never fights without its bar and its wall.
	if (!bFighting)
	{
		GetCreature()->SetPassive(true);
	}
}

void UBossComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UHealthComponent* BossHealth = GetBossHealth())
	{
		BossHealth->OnDamaged.RemoveDynamic(this, &UBossComponent::HandleBossDamaged);
		BossHealth->OnHealthChanged.RemoveDynamic(this, &UBossComponent::HandleBossHealthChanged);
		BossHealth->OnDeath.RemoveDynamic(this, &UBossComponent::HandleBossDeath);
	}
	UnbindPlayer();
	// The boss removed from a level that goes on (a dev command replacing it): its fight goes with it, so no adds or wall
	// are left behind. A level that's ending takes everything with it anyway.
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		DespawnAdds();
		ClearShots();
		DropSeal();
		if (SpawnedSeal)
		{
			SpawnedSeal->Destroy();
		}
	}
	SpawnedSeal = nullptr;
	if (Bar)
	{
		Bar->RemoveFromParent();
		Bar = nullptr;
	}
	bFighting = false;
	Super::EndPlay(EndPlayReason);
}

void UBossComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickFight(DeltaTime);
}

// ---------------------------------------------------------------------------
// Starting, starting over, winning
// ---------------------------------------------------------------------------

void UBossComponent::StartFight(APawn* Player)
{
	ACreatureBase* Creature = GetCreature();
	if (bFighting || bWon || !Creature || Creature->IsDead())
	{
		return;
	}
	APawn* Fighter = Player;
	if (!Fighter)
	{
		const UWorld* World = GetWorld();
		const APlayerController* First = World ? World->GetFirstPlayerController() : nullptr;
		Fighter = First ? First->GetPawn() : nullptr;
	}
	if (!IsLivingPlayer(Fighter))
	{
		return;
	}

	bFighting = true;
	BarHideTime = 0.f;
	PlayerOutsideTime = 0.f;
	FightPlayer = Fighter;
	// Its arena centers on its home, wherever the fight found it; its phases as the data has them now.
	Spot = Creature->GetHome().GetLocation();
	FightPhases = BossRules::Ordered(Phases);
	Phase = INDEX_NONE;
	PhaseTime = 0.f;
	Scheduled.Reset();
	BindPlayer(Fighter);
	// Its own tag steps aside for the bar at the top of the screen.
	bSavedShowsTag = Creature->bShowsHealthTag;
	if (bShowBar)
	{
		Creature->bShowsHealthTag = false;
	}
	// No longer waiting: it fights.
	Creature->SetPassive(false);
	Creature->AlertTo(Fighter);
	ShowBar();
	TryRaiseSeal();
	UE_LOG(LogLooter, Log, TEXT("Boss %s: the fight starts (%d phases)."), *GetLabel(), FightPhases.Num());
	OnFightStarted.Broadcast();
	EnterPhase(0);
	// Started by a hit, it may be past its first lines already.
	EvaluatePhase();
}

void UBossComponent::ResetFight()
{
	if (!bFighting)
	{
		return;
	}
	UE_LOG(LogLooter, Log, TEXT("Boss %s: the fight starts over."), *GetLabel());
	// Not fighting before it heals: the health it gets back starts no phase.
	ClearFight();
	HideBar();
	Phase = INDEX_NONE;
	if (ACreatureBase* Creature = GetCreature())
	{
		Creature->ResetToHome();
		// Waiting again, hunting nobody until something starts the fight.
		Creature->SetPassive(true);
	}
	OnFightReset.Broadcast();
}

void UBossComponent::ClearFight()
{
	bFighting = false;
	StopUntargetable(/*bRejoin*/ false);
	bVolleyPending = false;
	ClearShots();
	DespawnAdds();
	DropSeal();
	UnbindPlayer();
	Scheduled.Reset();
	FightPlayer.Reset();
	if (ACreatureBase* Creature = GetCreature())
	{
		Creature->bShowsHealthTag = bSavedShowsTag;
	}
}

void UBossComponent::HandleBossDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser)
{
	// A player's hit starts the fight (before the health event that follows, so its phases see the hit).
	if (bFighting || bWon || !bStartWhenHurt)
	{
		return;
	}
	APawn* Attacker = InstigatedBy ? InstigatedBy->GetPawn() : nullptr;
	if (IsLivingPlayer(Attacker))
	{
		StartFight(Attacker);
	}
}

void UBossComponent::HandleBossHealthChanged(float NewHealth, float MaxHealth)
{
	EvaluatePhase();
}

void UBossComponent::HandleBossDeath(AController* Killer)
{
	if (bWon)
	{
		return;
	}
	bWon = true;
	const bool bWasFighting = bFighting;
	ClearFight();
	// The bar shows it empty a moment, then fades.
	if (bWasFighting && bBarWanted)
	{
		UpdateBar();
		BarHideTime = BarHideDelay;
	}
	else
	{
		HideBar();
	}
	UE_LOG(LogLooter, Log, TEXT("Boss %s: beaten."), *GetLabel());

	// The campaign remembers a story boss beaten (its first defeat's scene and reward come once).
	if (!BossId.IsNone())
	{
		USessionSubsystem* Sessions = USessionSubsystem::Get(this);
		FCampaignRecord* Campaign = Sessions ? Sessions->GetCampaign() : nullptr;
		if (Campaign && Campaign->RecordBossDefeat(BossId))
		{
			UE_LOG(LogLooter, Log, TEXT("Boss %s: first defeat recorded as %s."), *GetLabel(), *BossId.ToString());
			Sessions->SaveSoon();
		}
	}
	OnFightWon.Broadcast();
}

void UBossComponent::HandlePlayerDeath(AController* Killer)
{
	// The player's death starts the fight over; the player's own death and respawn go on as ever (UPlayerVitalsSubsystem).
	ResetFight();
}

void UBossComponent::BindPlayer(APawn* Player)
{
	UnbindPlayer();
	if (UHealthComponent* PlayerHealth = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		PlayerHealth->OnDeath.AddUniqueDynamic(this, &UBossComponent::HandlePlayerDeath);
		BoundPlayerHealth = PlayerHealth;
	}
}

void UBossComponent::UnbindPlayer()
{
	if (UHealthComponent* PlayerHealth = BoundPlayerHealth.Get())
	{
		PlayerHealth->OnDeath.RemoveDynamic(this, &UBossComponent::HandlePlayerDeath);
	}
	BoundPlayerHealth.Reset();
}

APawn* UBossComponent::FindPlayerNear(const FVector& Center, float Radius) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* Controller = It->Get();
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (IsLivingPlayer(Pawn) && FVector::Dist2D(Pawn->GetActorLocation(), Center) <= Radius)
		{
			return Pawn;
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// The fight, frame by frame
// ---------------------------------------------------------------------------

void UBossComponent::TickFight(float DeltaSeconds)
{
	if (BarHideTime > 0.f)
	{
		BarHideTime -= DeltaSeconds;
		if (BarHideTime <= 0.f)
		{
			HideBar();
		}
	}

	ACreatureBase* Creature = GetCreature();
	if (!bFighting)
	{
		if (!bWon && EngageRadius > 0.f && Creature && !Creature->IsDead())
		{
			if (APawn* Near = FindPlayerNear(Creature->GetHome().GetLocation(), EngageRadius))
			{
				StartFight(Near);
			}
		}
		return;
	}

	// The player's death hook starts it over as they fall; this covers a player who's simply gone.
	if (!Creature || !IsLivingPlayer(FightPlayer.Get()))
	{
		ResetFight();
		return;
	}
	// Before the wall closes, a boss led off its spot goes home and the fight starts over.
	if (LeashRadius > 0.f && !IsSealRaised() && FVector::Dist2D(Creature->GetActorLocation(), Spot) > LeashRadius)
	{
		ResetFight();
		return;
	}
	// Once it's closed, a player put back outside it (fall recovery's safe spot) would be shut out of their own fight: after
	// a moment it starts over.
	const ABossSeal* ActiveSeal = GetActiveSeal();
	PlayerOutsideTime = ActiveSeal && ActiveSeal->IsRaised() && !ActiveSeal->IsInside(FightPlayer->GetActorLocation(), -SealMargin)
		? PlayerOutsideTime + DeltaSeconds : 0.f;
	if (PlayerOutsideTime > ShutOutSeconds)
	{
		ResetFight();
		return;
	}

	PruneAdds();
	TryRaiseSeal();
	// What's under way moves on first, so a spell or a volley that this frame's events begin starts its full time now.
	TickUntargetable(DeltaSeconds);
	TickVolley(DeltaSeconds);
	PhaseTime += DeltaSeconds;
	RunDueEvents();
	UpdateBar();
}
