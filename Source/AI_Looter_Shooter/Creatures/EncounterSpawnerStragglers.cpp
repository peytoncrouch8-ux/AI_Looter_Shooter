// AEncounterSpawner's stragglers: in a fight that came for the player (bHuntOnSpawn), one of its creatures out of the fight
// while the player stands on its ground is called back into it, and brought back on its ground when it doesn't come
// (EncounterRules::StragglerCall). The 2026-10-23 play-check: after a death at the town gate the Unpaid walked home, one
// of them behind a cottage out of sight and hearing, and Main 3's count stood at 6/7 for good.

#include "Creatures/EncounterSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/HuntingGround.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Drawn on screen this lately (s) still counts as seen (as the spawner's own IsAnyInView has it). */
	constexpr float StragglerSeenLatelySeconds = 0.5f;

	/**
	 * The most one look counts (s): its timer looks twice a second, so only a hitch or a test's single long step is longer,
	 * and neither should bring a creature back in one go.
	 */
	constexpr float MostPerLook = 1.f;
}

bool AEncounterSpawner::IsPlayerOnFightGround(const APawn& Player) const
{
	// Alive (a dead player's fight is over: they gave up on the body), out of a safe zone, near enough to have brought
	// the fight (ActivationRadius) and on its hunting ground: where its creatures would hunt them anyway.
	const UHealthComponent* Life = Player.FindComponentByClass<UHealthComponent>();
	if (Life && Life->IsDead())
	{
		return false;
	}
	const FVector Where = Player.GetActorLocation();
	const UEncounterSubsystem* Encounters = GetEncounters();
	if (Encounters && Encounters->IsInSafeZone(Where))
	{
		return false;
	}
	return IsPlayerWithin(ActivationRadius) && MakeHuntingGround().Contains(Where, GetSpawnCenter());
}

void AEncounterSpawner::UpdateStragglers(float DeltaSeconds)
{
	UEncounterSubsystem* Encounters = GetEncounters();
	APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	const bool bCalling = bHuntOnSpawn && Player && IsPlayerOnFightGround(*Player);
	const float Step = FMath::Min(DeltaSeconds, MostPerLook);
	const UWorld* World = GetWorld();
	TArray<int32> ToBringBack;
	for (int32 Index = 0; Index < Living.Num(); ++Index)
	{
		FLivingCreature& Each = Living[Index];
		ACreatureBase* Creature = Each.Creature.Get();
		if (!Creature || Creature->IsDead() || !bCalling || Creature->IsFightingWith(Player))
		{
			// In the fight (or nothing to call it to): its clock starts over, and a recall is ready for it.
			Each.OutOfFightSeconds = 0.f;
			Each.SinceRecallSeconds = EncounterRules::StragglerRecallEverySeconds;
			continue;
		}
		Each.OutOfFightSeconds += Step;
		Each.SinceRecallSeconds += Step;
		const USkeletalMeshComponent* Body = Creature->GetMesh();
		const float BodyRadius = Body ? static_cast<float>(Body->Bounds.SphereRadius) : 100.f;
		const bool bInView = Encounters->IsInPlayersView(Creature->GetActorLocation(), BodyRadius)
			|| (World && Body && World->TimeSince(Body->GetLastRenderTimeOnScreen()) <= StragglerSeenLatelySeconds);
		switch (EncounterRules::StragglerCall(Each.OutOfFightSeconds, Each.SinceRecallSeconds, bInView))
		{
		case EncounterRules::EStragglerCall::Recall:
			UE_LOG(LogLooter, Verbose, TEXT("Encounter %s: %s is out of the fight (%.0f s): told where the player is."),
				*GetSpawnerId().ToString(), *Creature->GetName(), Each.OutOfFightSeconds);
			Each.SinceRecallSeconds = 0.f;
			Creature->AlertTo(Player);
			break;
		case EncounterRules::EStragglerCall::BringBack:
			// A Legendary lair's monster keeps its own fight and is never moved; a boss neither.
			if (LegendaryId.IsNone() && !Creature->KeepsOwnFight())
			{
				ToBringBack.Add(Index);
			}
			break;
		case EncounterRules::EStragglerCall::None:
			break;
		}
	}
	// Last first, so the indices still hold as each goes.
	for (int32 Pick = ToBringBack.Num() - 1; Pick >= 0; --Pick)
	{
		const int32 Index = ToBringBack[Pick];
		ACreatureBase* Creature = Living[Index].Creature.Get();
		// Owed, not killed (no loot, no experience, no death for the missions): it comes again at once on its ground, near
		// the player but out of their sight, coming for them (SpawnOwed, bHuntOnSpawn), its rank kept.
		FOwedCreature& Entry = Owed.AddDefaulted_GetRef();
		Entry.Group = Living[Index].Group;
		Entry.Rank = Creature ? Creature->GetRank() : ECreatureRank::Basic;
		UE_LOG(LogLooter, Log, TEXT("Encounter %s: %s stayed out of the fight %.0f s out of sight: brought back on its ground."),
			*GetSpawnerId().ToString(), Creature ? *Creature->GetName() : TEXT("a creature"), Living[Index].OutOfFightSeconds);
		if (Creature)
		{
			Creature->Destroy();
		}
		Living.RemoveAt(Index);
	}
}
