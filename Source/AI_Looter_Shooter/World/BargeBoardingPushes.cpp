// ABargeBoarding: the deck fight's later pushes (BargeFightRules.h: the user's "Bigger", 2026-11-04). The first push is the
// deck's own encounter (BargeDeck), coming as the Lamplighter draws alongside; while the player fights across her deck
// (Main 6 step 3) she starts the second (the droppers off the house's roof and the bow's scoop: BargeDeckRoof and
// BargeDeckTower, rappel encounters) and then the third (up the walks from the stern: BargeDeckStern), each when the one
// before has brought all its waves and is down to a couple, a beat after, or when it has gone on long enough. The build
// script places the four spawners (their entrances only on her trigger); the mission's deck step clears all four.

#include "World/BargeBoarding.h"
#include "World/BargeRules.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

const FName ABargeBoarding::RoofFight(TEXT("BargeDeckRoof"));
const FName ABargeBoarding::TowerFight(TEXT("BargeDeckTower"));
const FName ABargeBoarding::SternFight(TEXT("BargeDeckStern"));

TArray<FName> ABargeBoarding::PushFights(int32 Push)
{
	switch (Push)
	{
	case 0: return { DeckFight };
	case 1: return { RoofFight, TowerFight };
	case 2: return { SternFight };
	default: return {};
	}
}

void ABargeBoarding::AdvancePushes(float DeltaSeconds)
{
	// Counted again from the first push whenever her step changes (a load mid-fight starts the step over).
	if (PushStep != StoryStep)
	{
		PushStep = StoryStep;
		PushesStarted = 1;
		PushClock = 0.f;
		MostlyDownClock = 0.f;
	}
	if (StoryStep != BargeRules::DeckStep || PushesStarted >= BargeRules::NumDeckPushes)
	{
		return;
	}
	UWorld* World = GetWorld();
	const UEncounterSubsystem* Encounters = World ? World->GetSubsystem<UEncounterSubsystem>() : nullptr;
	if (!Encounters)
	{
		return;
	}
	// Held while the player is down or away (a test level has no player: it runs on).
	const APawn* Player = FindPlayer();
	const UHealthComponent* Health = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
	if ((World->IsGameWorld() && !Player) || (Health && Health->IsDead())
		|| (Player && FVector::DistSquared(Player->GetActorLocation(), ToWorld(BargeRules::DeckMiddle())) > FMath::Square(BargeRules::PushPlayerReach)))
	{
		return;
	}
	PushClock += DeltaSeconds;
	// The push now: all its waves out (or cleared) and down to a couple, counting those still owed.
	bool bAny = false;
	bool bAllOut = true;
	int32 Left = 0;
	for (const FName Fight : PushFights(PushesStarted - 1))
	{
		if (const AEncounterSpawner* Spawner = Encounters->FindSpawner(Fight))
		{
			bAny = true;
			bAllOut &= Spawner->GetState() == EEncounterState::Cleared || Spawner->GetWavesStarted() >= Spawner->GetPlannedWaves();
			Left += Spawner->NumAlive() + Spawner->NumOwed();
		}
	}
	const bool bMostlyDown = !bAny || (bAllOut && Left <= BargeRules::PushNextAlive);
	MostlyDownClock = bMostlyDown ? MostlyDownClock + DeltaSeconds : 0.f;
	if (!BargeRules::IsNextPushDue(bMostlyDown, MostlyDownClock, PushClock))
	{
		return;
	}
	int32 Started = 0;
	for (const FName Fight : PushFights(PushesStarted))
	{
		if (AEncounterSpawner* Spawner = Encounters->FindSpawner(Fight))
		{
			Started += Spawner->TriggerWave() ? 1 : 0;
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the deck fight's push %d (%d of its encounters started, %d of the last left, %.0f s on)."), *GetActorNameOrLabel(),
		PushesStarted + 1, Started, Left, PushClock);
	++PushesStarted;
	PushClock = 0.f;
	MostlyDownClock = 0.f;
}
