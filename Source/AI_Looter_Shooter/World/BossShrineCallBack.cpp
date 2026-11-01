// ABossShrine: bringing its boss back. A lair brings its boss anew; a placed boss is readied at its spot; Abel, whose fight
// ends in a last call, a scene and a story that must not replay, has his own way: his stone calls back "the smog's echo of
// him" (the user's call, 2026-10-31 night: the furnace's shadow of Abel, fought again for his loot, while the real Abel keeps
// Saint Ada at the chapel). The echo has no Abel in it to call back: no call-backs, no pips, no last call; beaten, it boils
// away into the fog before his kneel could ask for the last call.

#include "World/BossShrine.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSpawner.h"
#include "Engine/World.h"

namespace
{
	/**
	 * The echo, beaten, kneels this long (s) before it boils away: well inside his kneel's settling (2 s,
	 * AbelKeeperMoves.cpp), after which his own code would open the last call.
	 */
	constexpr float RematchKneelSeconds = 1.25f;
}

bool ABossShrine::CallFromLair()
{
	AEncounterSpawner* Den = Lair.Get();
	if (!Den)
	{
		return false;
	}
	// As the console's Looter.Legendary.Return does: whatever its story and its away-this-visit say, it starts over and
	// brings its boss now (a lurker waits in its den until it notices someone, and its fight starts as it turns on them).
	if (!Den->TriggerWave(/*bForce*/ true))
	{
		return false;
	}
	bLairCallOn = true;
	MarkLairBosses();
	return true;
}

void ABossShrine::MarkLairBosses()
{
	const AEncounterSpawner* Den = Lair.Get();
	if (!Den)
	{
		return;
	}
	// Each time it looks: one taken away while the player was far comes back as a new body, which is marked in turn.
	for (ACreatureBase* Each : Den->GetAliveCreatures())
	{
		UBossComponent* Fight = Each && IsOwnBoss(*Each) ? Each->FindComponentByClass<UBossComponent>() : nullptr;
		if (Fight && !Fight->IsRematch())
		{
			Fight->MarkRematch();
		}
	}
}

bool ABossShrine::CallPlaced()
{
	ACreatureBase* Boss = PlacedBoss.Get();
	if (!Boss)
	{
		return false;
	}
	if (AAbelKeeper* Abel = Cast<AAbelKeeper>(Boss))
	{
		return CallAbel(*Abel);
	}
	UBossComponent* Fight = Boss->FindComponentByClass<UBossComponent>();
	return Fight && Fight->PrepareRematch();
}

bool ABossShrine::CallAbel(AAbelKeeper& Abel)
{
	UBossComponent* Fight = Abel.GetBoss();
	if (!Fight)
	{
		return false;
	}
	// The echo's fight starts as the first did, a player on the deck (Main 7's FightWhen kept it to the mission's fight step).
	SavedFightWhen = Abel.FightWhen;
	Abel.FightWhen = FStoryCondition();
	// Alive at his spot and his fight unwon (a rematch: his corruption keeps no call-backs), on the boards whatever the story says.
	const FVector Seat = Abel.GetActorLocation();
	if (!Fight->PrepareRematch() || !Abel.ComeBack())
	{
		Abel.FightWhen = SavedFightWhen;
		return false;
	}
	// Unhurt until his fight starts, as Main 7 had him (his fight's start takes it off).
	if (UHealthComponent* Health = Abel.FindComponentByClass<UHealthComponent>())
	{
		Health->bInvulnerable = true;
		Health->bShowDamageNumbers = false;
	}
	SoulLight(Seat + FVector(0.0, 0.0, 60.0), 110.f);
	RematchAbel = &Abel;
	bAbelRematch = true;
	UpdateTickPace();
	UE_LOG(LogLooter, Log, TEXT("%s: the smog's echo of Abel walks the boards (the real Abel keeps Saint Ada)."), *GetActorNameOrLabel());
	return true;
}

void ABossShrine::TickAbelRematch()
{
	AAbelKeeper* Abel = RematchAbel.Get();
	if (!Abel)
	{
		bAbelRematch = false;
		return;
	}
	// Something else ended him (the console): the fight is over all the same.
	if (Abel->IsEndingDone())
	{
		EndAbelRematch(/*bWon*/ true);
		return;
	}
	// Beaten: the echo kneels a moment, then boils away into the fog; the last call and "Free Abel" never come again.
	if (Abel->IsDead() && Abel->GetMove() == EAbelMove::Kneel && Abel->GetMoveTime() >= RematchKneelSeconds)
	{
		EndAbelRematch(/*bWon*/ true);
	}
}

void ABossShrine::EndAbelRematch(bool bWon)
{
	AAbelKeeper* Abel = RematchAbel.Get();
	bAbelRematch = false;
	RematchAbel.Reset();
	if (!Abel)
	{
		return;
	}
	if (bWon && !Abel->IsEndingDone())
	{
		// The echo boils away into the fog (no lantern of his to step into; nothing tells the missions a scene played).
		SoulLight(Abel->GetActorLocation(), 120.f);
		Abel->CompleteFreeing(/*bLanternToo*/ false);
	}
	// Back to the story's word: out of the world after Main 7, his fight kept to Main 7's step.
	Abel->ForcePresent(false);
	Abel->FightWhen = SavedFightWhen;
	UE_LOG(LogLooter, Log, TEXT("%s: the smog's echo's fight is over%s."), *GetActorNameOrLabel(), bWon ? TEXT(", won: it boils away") : TEXT(" (the stone went)"));
}
