#include "Missions/ObjectiveHighlightTargets.h"
#include "Interaction/Interactable.h"
#include "Loot/WeaponRack.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRunner.h"
#include "Tutorial/MissionClearZoneObjective.h"
#include "Weapons/WeaponBase.h"
#include "World/MissionItem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

namespace ObjectiveHighlightTargets
{
	FMissionActorFilter FilterFor(const UMissionObjective& Objective)
	{
		// A fight is the creatures' own: whatever its filter names, nothing is lit for it.
		const bool bFight = Objective.IsA<UMissionKillObjective>() || Objective.IsA<UMissionKillNamedObjective>()
			|| Objective.IsA<UMissionClearObjective>() || Objective.IsA<UMissionClearZoneObjective>();
		// A place to hold out in is ground to stand on, not a thing to find.
		if (bFight || Objective.IsA<UMissionDefendObjective>())
		{
			return FMissionActorFilter();
		}
		const FMissionActorFilter Own = Objective.GetTargets();
		if (Own.IsSet())
		{
			return Own;
		}
		// Objectives done by an event name their thing only through the arrow (shoot the egg sacs, read the roll, ring the
		// bell, take a gun from the rack): what the arrow points at is what to find.
		if (Objective.Waypoint == EMissionWaypoint::Actor && Objective.WaypointActor.IsSet())
		{
			return Objective.WaypointActor;
		}
		return FMissionActorFilter();
	}

	TArray<const UMissionObjective*> LiveObjectives(const UMissionRunner& Runner, FName MissionId)
	{
		TArray<const UMissionObjective*> Live;
		const UMissionDefinition* Mission = Runner.FindDefinition(MissionId);
		if (!Mission || !Runner.IsRunning(MissionId))
		{
			return Live;
		}
		const bool bReady = Runner.IsReadyToTurnIn(MissionId);
		// Waiting for its turn-in, its step is one past its last, and only the last step's optional objectives are left.
		const int32 Step = bReady ? Mission->Steps.Num() - 1 : Runner.GetStep(MissionId);
		if (!Mission->Steps.IsValidIndex(Step))
		{
			return Live;
		}
		// The runner's views list the step's objectives in its order, leaving out empty entries in the data (and, waiting for a
		// turn-in, all but the optional ones): read them side by side with the step, leaving out the same.
		const TArray<FMissionObjectiveView> Views = bReady ? Runner.GetOptionalViews(MissionId) : Runner.GetObjectiveViews(MissionId);
		int32 ViewIndex = 0;
		for (int32 Index = 0; Index < Mission->Steps[Step].Objectives.Num(); ++Index)
		{
			const UMissionObjective* Objective = Mission->GetObjective(Step, Index);
			if (!Objective || (bReady && !Objective->bOptional))
			{
				continue;
			}
			const bool bDone = Views.IsValidIndex(ViewIndex) && Views[ViewIndex].bDone;
			++ViewIndex;
			if (!bDone)
			{
				Live.Add(Objective);
			}
		}
		return Live;
	}

	void Gather(const UMissionRunner& Runner, FName MissionId, TArray<AActor*>& Out)
	{
		const UMissionDefinition* Mission = Runner.FindDefinition(MissionId);
		const UWorld* World = Runner.GetWorld();
		if (!Mission || !World || !Runner.IsRunning(MissionId))
		{
			return;
		}
		auto Add = [&Out](AActor* Found)
		{
			// The gun lying on a rack is the thing to take; the rack only holds it (and once it's taken, nothing is).
			if (const AWeaponRack* Rack = Cast<AWeaponRack>(Found))
			{
				Found = Rack->IsWeaponOffered() ? Rack->GetOfferedWeapon() : nullptr;
			}
			if (Found && Out.Num() < MaxTargets && CanLight(*Found))
			{
				Out.AddUnique(Found);
			}
		};
		for (const UMissionObjective* Objective : LiveObjectives(Runner, MissionId))
		{
			// Mission items are named by id, not by a filter: each one not carried yet that lies in the level (one a creature
			// still carries is that creature's, and creatures are never lit).
			if (const UMissionCollectItemObjective* Collect = Cast<UMissionCollectItemObjective>(Objective))
			{
				for (const FName ItemId : Collect->Items)
				{
					if (!Runner.IsCarrying(ItemId))
					{
						Add(AMissionItem::FindIn(World, ItemId));
					}
				}
				continue;
			}
			MissionTargets::ForEach(World, FilterFor(*Objective), [&Add](AActor& Each) { Add(&Each); });
		}
		// Everything done, the one to turn it in to (Delia's door, the notice board) is the thing to find.
		if (Runner.IsReadyToTurnIn(MissionId) && Mission->NeedsTurnIn())
		{
			FMissionActorFilter Giver;
			Giver.ActorTag = Mission->TurnIn.SpeakerTag;
			MissionTargets::ForEach(World, Giver, [&Add](AActor& Each) { Add(&Each); });
		}
	}

	bool CanLight(const AActor& Actor)
	{
		if (Actor.IsActorBeingDestroyed() || Actor.IsHidden() || Actor.IsA<APawn>() || !MissionTargets::IsAlive(Actor))
		{
			return false;
		}
		// Used for good (a poster torn down, a bale loaded): done with, though its objective may still count others.
		if (const IInteractable* Usable = Cast<IInteractable>(&Actor))
		{
			if (Usable->IsUsedUp())
			{
				return false;
			}
		}
		const AMissionItem* Item = Cast<AMissionItem>(&Actor);
		return !Item || Item->IsShown();
	}
}
