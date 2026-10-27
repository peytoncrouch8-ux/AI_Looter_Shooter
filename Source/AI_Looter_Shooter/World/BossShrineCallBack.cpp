// ABossShrine: bringing its boss back. A lair brings its boss anew; a placed boss is readied at its spot; Abel, whose fight
// ends in a scene and a story that must not replay, has his own way (Pa leaves his board while the Keeper walks the boards
// again, and is back on it before "Sit with Pa" could play a second time).

#include "World/BossShrine.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSpawner.h"
#include "Story/AbelOnBoard.h"
#include "Engine/World.h"

namespace
{
	/**
	 * Abel, beaten again, kneels this long (s) before he's back on his board: well inside his kneel's settling (2 s,
	 * AbelKeeperMoves.cpp), after which his own code would play "Sit with Pa" again.
	 */
	constexpr float RematchKneelSeconds = 1.25f;

	/** A mission no campaign ever finishes: Pa's board's story condition while the Keeper walks the boards again. */
	const FName NeverMission(TEXT("VigilStone.KeeperWalks"));
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
	// Pa leaves his board while the Keeper walks the boards again: his board's story is set to one that never holds until
	// the fight is over (ComeBack has it read again).
	AAbelOnBoard* Board = Abel.OnBoard;
	if (Board)
	{
		SavedShownWhen = Board->ShownWhen;
		FStoryCondition Never;
		Never.AfterMissions.Add(NeverMission);
		Board->ShownWhen = Never;
	}
	// His fight starts as it first did, a player on the deck (Main 6's FightWhen kept it to the mission's fight step).
	SavedFightWhen = Abel.FightWhen;
	Abel.FightWhen = FStoryCondition();
	// Alive at his spot and his fight unwon, then back on the boards, in the world whatever the story says.
	const FVector Seat = Board ? Board->GetActorLocation() : Abel.GetActorLocation();
	if (!Fight->PrepareRematch() || !Abel.ComeBack())
	{
		Abel.FightWhen = SavedFightWhen;
		if (Board)
		{
			Board->ShownWhen = SavedShownWhen;
			Board->RefreshShown();
		}
		return false;
	}
	// Unhurt until his fight starts, as Main 6 had him (his fight's start takes it off).
	if (UHealthComponent* Health = Abel.FindComponentByClass<UHealthComponent>())
	{
		Health->bInvulnerable = true;
		Health->bShowDamageNumbers = false;
	}
	SoulLight(Seat + FVector(0.0, 0.0, 60.0), 110.f);
	RematchAbel = &Abel;
	bAbelRematch = true;
	UpdateTickPace();
	UE_LOG(LogLooter, Log, TEXT("%s: the Keeper walks the boards again; Pa's board stands empty until his fight is over."),
		*GetActorNameOrLabel());
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
	// Something else sat him down (the console): the fight is over all the same.
	if (Abel->IsEndingDone())
	{
		EndAbelRematch(/*bWon*/ true);
		return;
	}
	// Beaten again: he kneels a moment, his coal sinking, then he's back on his board; "Sit with Pa" never plays again.
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
	AAbelOnBoard* Board = Abel->OnBoard;
	if (bWon && !Abel->IsEndingDone())
	{
		// His ending as the scene leaves it, without the scene: the Keeper's Lantern lit on its post, Pa on his board, the
		// boss gone from the boards. (SitOnBoard, not FinishEndingAtOnce: nothing tells the missions a scene played.)
		SoulLight(Abel->GetActorLocation(), 120.f);
		Abel->SitOnBoard();
	}
	// Back to the story's word: out of the world after Main 6, his fight kept to Main 6's step.
	Abel->ForcePresent(false);
	Abel->FightWhen = SavedFightWhen;
	if (Board)
	{
		Board->ShownWhen = SavedShownWhen;
		Board->RefreshShown();
		if (bWon)
		{
			SoulLight(Board->GetActorLocation() + FVector(0.0, 0.0, 60.0), 100.f);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the Keeper's fight is over%s; Pa sits on his board."), *GetActorNameOrLabel(),
		bWon ? TEXT(", won") : TEXT(" (the stone went)"));
}
