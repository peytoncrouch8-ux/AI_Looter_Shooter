// AEncounterSpawner's creatures sent away: its story moved on (a mission's step done) while they were seen or fighting. They
// break off, walk home, and go once nobody sees them; a fight the story has moved past never fights on through what comes
// next (the 2026-10-23 play-check: Fenn's coins laid back mid-fight, the depot's second push turned off, and its Unpaid
// clawed at the player through Fenn's lines over his coffin).

#include "Creatures/EncounterSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Engine/World.h"

namespace
{
	/**
	 * The longest one sent away stays (s), seen or not: home again and standing in view, it would wait there for good. A
	 * walk home across a room or a street takes a few seconds.
	 */
	constexpr float LeaveAtMostSeconds = 12.f;
}

void AEncounterSpawner::SendLivingAway()
{
	PruneLiving();
	int32 Sent = 0;
	for (const FLivingCreature& Each : Living)
	{
		ACreatureBase* Creature = Each.Creature.Get();
		if (!Creature || Creature->IsDead())
		{
			continue;
		}
		// Not killed: no loot, no experience, no death for the missions to count. It lets go of the player (an attack under
		// way plays out first) and hunts nobody any more.
		Creature->SetPassive(true);
		Leaving.Add(Creature);
		++Sent;
	}
	Living.Reset();
	LeavingSeconds = 0.f;
	if (Sent > 0)
	{
		UE_LOG(LogLooter, Log, TEXT("Encounter %s: its story moved on; %d break off and go once nobody sees them."),
			*GetSpawnerId().ToString(), Sent);
	}
	UpdateTimer();
}

void AEncounterSpawner::UpdateLeaving(float DeltaSeconds)
{
	if (Leaving.IsEmpty())
	{
		return;
	}
	LeavingSeconds += DeltaSeconds;
	const bool bTimeUp = LeavingSeconds >= LeaveAtMostSeconds;
	for (int32 Index = Leaving.Num() - 1; Index >= 0; --Index)
	{
		ACreatureBase* Creature = Leaving[Index].Get();
		if (!Creature || Creature->IsDead())
		{
			// Killed on its way (it counts as any kill does), or gone already.
			Leaving.RemoveAtSwap(Index);
			continue;
		}
		if (bTimeUp || (!IsInView(*Creature) && Creature->GetCreatureState() != ECreatureState::Attack))
		{
			Creature->Destroy();
			Leaving.RemoveAtSwap(Index);
		}
	}
}
