// UMusicDirectorSubsystem's looking round: who hunts the local player, whether a boss's fight runs, the fight's kills,
// the elites' stings, and the boss's phases and win.

#include "Audio/MusicDirectorSubsystem.h"
#include "Audio/LooterSoundCues.h"
#include "Bosses/BossComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/UnpaidCreature.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UMusicDirectorSubsystem::FSenses UMusicDirectorSubsystem::Sense() const
{
	FSenses Seen;
	UWorld* World = GetWorld();
	if (!World)
	{
		return Seen;
	}
	const APawn* Player = UGameplayStatics::GetPlayerPawn(World, 0);
	// A few dozen creatures, four times a second: cheaper than a hook in every brain.
	for (TActorIterator<ACreatureBase> It(World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (!IsValid(Creature) || Creature->IsDead())
		{
			continue;
		}
		const bool bUnpaid = Creature->IsA<AUnpaidCreature>();
		if (UBossComponent* Boss = Creature->FindComponentByClass<UBossComponent>())
		{
			if (Boss->IsFighting())
			{
				Seen.bBossFight = true;
				Seen.Theme = MusicRules::ThemeFor(bUnpaid);
				Seen.Boss = Boss;
				Seen.BossFoe = Creature;
			}
		}
		if (Player && Creature->GetTarget() == Player)
		{
			++Seen.Hunters;
			Seen.HunterList.Add(Creature);
			// A Soulfed monster with a lair (the Gravemother) is a boss's fight, bar or no bar.
			if (Creature->GetRank() == ECreatureRank::Legendary && !Seen.bBossFight)
			{
				Seen.bBossFight = true;
				Seen.Theme = MusicRules::ThemeFor(bUnpaid);
				Seen.BossFoe = Creature;
			}
		}
	}
	return Seen;
}

void UMusicDirectorSubsystem::Notice(const FSenses& Seen, double At)
{
	// The fight's kills: those that hunted in it and have died since (or gone).
	for (const TWeakObjectPtr<ACreatureBase>& Hunter : Seen.HunterList)
	{
		FightHunters.Add(Hunter);
	}
	for (auto It = FightHunters.CreateIterator(); It; ++It)
	{
		const ACreatureBase* Creature = It->Get();
		if (!Creature || Creature->IsDead())
		{
			++FightKills;
			It.RemoveCurrent();
		}
	}
	// A creature of high rank is announced the first time it comes for the player (a boss has its own sting).
	for (const TWeakObjectPtr<ACreatureBase>& Hunter : Seen.HunterList)
	{
		const ACreatureBase* Creature = Hunter.Get();
		if (!Creature || !MusicRules::IsElite(Creature->GetRank()) || Announced.Contains(Hunter))
		{
			continue;
		}
		Announced.Add(Hunter);
		if (!Seen.bBossFight && Mood != EMusicMood::Boss && At >= NextElite)
		{
			PlaySting(LooterSoundCue::StingElite);
			NextElite = At + MusicRules::EliteStingCooldown;
		}
	}
	for (auto It = Announced.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void UMusicDirectorSubsystem::WatchBoss(UBossComponent* Boss)
{
	if (WatchedBoss.Get() == Boss)
	{
		return;
	}
	UnwatchBoss();
	if (!Boss)
	{
		return;
	}
	WatchedBoss = Boss;
	PhaseHandle = Boss->OnPhaseChanged.AddUObject(this, &UMusicDirectorSubsystem::HandleBossPhase);
	WonHandle = Boss->OnFightWon.AddUObject(this, &UMusicDirectorSubsystem::HandleBossWon);
}

void UMusicDirectorSubsystem::UnwatchBoss()
{
	if (UBossComponent* Boss = WatchedBoss.Get())
	{
		Boss->OnPhaseChanged.Remove(PhaseHandle);
		Boss->OnFightWon.Remove(WonHandle);
	}
	WatchedBoss.Reset();
	PhaseHandle.Reset();
	WonHandle.Reset();
}

void UMusicDirectorSubsystem::HandleBossPhase(int32 NewPhase, int32 OldPhase)
{
	// The fight's first phase came in with the opening sting; each later one hits it again over the theme.
	if (OldPhase != INDEX_NONE && NewPhase != OldPhase && Mood == EMusicMood::Boss)
	{
		PlaySting(LooterSoundCue::StingPhase);
		PhaseDuckUntil = Now() + MusicRules::PhaseDuckSeconds;
	}
}

void UMusicDirectorSubsystem::HandleBossWon()
{
	bBossWon = true;
}
