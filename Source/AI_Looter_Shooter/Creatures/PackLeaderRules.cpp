#include "Creatures/PackLeaderRules.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightEvent.h"
#include "Creatures/FightTuning.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	/** The leader-down sting (Art/Sounds/recipes/fight_kit.py). A cue without sounds plays nothing until the bank has it. */
	const FName LeaderDownCue(TEXT("Fight.LeaderDown"));
}

bool PackLeaderRules::WantsBreakOffRoll(const FBreakOffAsk& Ask)
{
	return Ask.Rank == ECreatureRank::Basic && Ask.bAllowedKind && !Ask.bDead && Ask.bHunting && !Ask.bRetreating;
}

bool PackLeaderRules::BreaksOff(const FBreakOffAsk& Ask, float Roll, float Chance)
{
	return WantsBreakOffRoll(Ask) && Roll < FMath::Clamp(Chance, 0.f, 1.f);
}

bool PackLeaderRules::Falters(const FPackmateLook& Look, float Reach)
{
	return !Look.bSelf && Look.bAlive && Look.bSharesPack && !Look.bOwnFight && Look.Distance <= FMath::Max(Reach, 0.f);
}

bool PackLeaderRules::SamePack(FName TagA, const UClass* ClassA, FName TagB, const UClass* ClassB)
{
	if (TagA.IsNone() || TagB.IsNone())
	{
		return ClassA && ClassA == ClassB;
	}
	return TagA == TagB;
}

void PackLeaderRules::OnLeaderDown(ACreatureBase& Leader)
{
	UWorld* World = Leader.GetWorld();
	if (!World)
	{
		return;
	}
	const FFightTuning Numbers = FFightTuning::Live();
	if (UFightDirectorSubsystem* Director = UFightDirectorSubsystem::Get(&Leader))
	{
		Director->NoteLeaderDown(Leader);
		Director->NoteEvent(Leader, EFightEvent::LeaderDown);
	}
	// Heard wherever the player stands, as the rank sting is: the fight just turned their way.
	LooterSound::Play2D(&Leader, LeaderDownCue);

	// Who it shakes: gathered first, as each one's answer may move it (a break-off) while the list is walked.
	TArray<ACreatureBase*> Shaken;
	const FVector Where = Leader.GetActorLocation();
	auto Consider = [&](ACreatureBase* Other)
	{
		if (!Other || Other->IsActorBeingDestroyed())
		{
			return;
		}
		FPackmateLook Look;
		Look.bSelf = Other == &Leader;
		Look.bAlive = !Other->IsDead();
		Look.bSharesPack = Other->SharesPackWith(Leader);
		Look.bOwnFight = Other->KeepsOwnFight() || Other->IsPassive();
		Look.Distance = static_cast<float>(FVector::Dist(Other->GetActorLocation(), Where));
		if (Falters(Look, Numbers.LeaderReach))
		{
			Shaken.Add(Other);
		}
	};
	if (const UEncounterSubsystem* Encounters = World->GetSubsystem<UEncounterSubsystem>())
	{
		for (const TWeakObjectPtr<ACreatureBase>& Each : Encounters->GetTrackedCreatures())
		{
			Consider(Each.Get());
		}
	}
	else
	{
		for (TActorIterator<ACreatureBase> It(World); It; ++It)
		{
			Consider(*It);
		}
	}
	for (ACreatureBase* Each : Shaken)
	{
		Each->OnPackLeaderDown(Leader);
	}
	UE_LOG(LogLooter, Log, TEXT("%s, its pack's leader, is down: %d of its pack falter."), *Leader.GetName(), Shaken.Num());
}
