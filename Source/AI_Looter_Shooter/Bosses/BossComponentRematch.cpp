// UBossComponent: a won fight readied to be fought again (a vigil stone's call back, World/BossShrine), and the static
// word every boss's win sends out. The fight's course is in BossComponent.cpp.

#include "Bosses/BossComponent.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Loot/LootDropComponent.h"

FOnBossWonAnywhere UBossComponent::OnAnyBossWon;

bool UBossComponent::PrepareRematch()
{
	ACreatureBase* Creature = GetCreature();
	if (!Creature || !GetBossHealth() || bFighting)
	{
		return false;
	}
	// Whatever the last fight left behind goes: its slow beat, pellets, adds, wall and bar.
	EndDeathSlow();
	ClearShots();
	DespawnAdds();
	DropSeal();
	HideBar();
	BarHideTime = 0.f;
	EndStagger();
	StaggerBuildUp = 0.f;
	StaggerCooldownLeft = 0.f;
	Scheduled.Reset();
	Phase = INDEX_NONE;
	PhaseTime = 0.f;
	bWon = false;
	bRematch = true;
	// Its loot is the shower's again: the loot drop component's own toss was turned off the first time (TakeOverLoot), and
	// the shower let it go as it threw.
	if (!bLootTakenOver && LootShower.bEnabled)
	{
		if (ULootDropComponent* Loot = Creature->FindComponentByClass<ULootDropComponent>())
		{
			Loot->bDropOnDeath = false;
			bLootTakenOver = true;
		}
	}
	// Alive at its spot at full health, calm, at its rank and level; then waiting for its fight as it did the first time
	// (one that lives as its kind does is free).
	Creature->ResetToHome();
	Creature->SetPassive(bWaitsPassive);
	SetComponentTickEnabled(true);
	UE_LOG(LogLooter, Log, TEXT("Boss %s: called back; its fight waits to start again (%d phases)."), *GetLabel(), Phases.Num());
	return true;
}
