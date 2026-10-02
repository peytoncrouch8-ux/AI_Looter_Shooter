// ATutorialDirector: its built-in steps as a mission (what DA_Mission_Tutorial holds).

#include "Tutorial/TutorialDirector.h"
#include "Combat/TargetDummy.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/WeaponRack.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionPlayerObjectives.h"
#include "UObject/Package.h"

namespace
{
	/** The weapon rack, where the road to the village leads and the first rifle lies. */
	FMissionActorFilter WeaponRack()
	{
		FMissionActorFilter Rack;
		Rack.ActorClass = AWeaponRack::StaticClass();
		return Rack;
	}

	/** The objective that finishes a step with this goal, pointing where the tutorial has always pointed. */
	UMissionObjective* MakeObjective(UObject* Outer, const FTutorialStep& Step)
	{
		switch (Step.Goal)
		{
		case ETutorialGoal::Move:
		{
			// Moving at all; the road leads to the village and its gun rack, so the arrow already points there.
			UMissionTravelObjective* Travel = NewObject<UMissionTravelObjective>(Outer);
			Travel->Distance = Step.Amount;
			Travel->Waypoint = EMissionWaypoint::Actor;
			Travel->WaypointActor = WeaponRack();
			return Travel;
		}
		case ETutorialGoal::ReachRack:
		{
			// No rack in the level: nothing to walk to, so it passes.
			UMissionReachObjective* Reach = NewObject<UMissionReachObjective>(Outer);
			Reach->Place.Actor = WeaponRack();
			Reach->Place.Radius = Step.Amount;
			Reach->Place.bIgnoreHeight = true;
			Reach->bPassWithoutTargets = true;
			return Reach;
		}
		case ETutorialGoal::HoldWeapon:
		{
			// The rifle itself while it lies on the rack; once it's gone (taken, restocking), the rack.
			UMissionCollectObjective* Collect = NewObject<UMissionCollectObjective>(Outer);
			Collect->What = EMissionCollect::Weapons;
			Collect->Count = FMath::Max(1, FMath::RoundToInt32(Step.Amount));
			Collect->Waypoint = EMissionWaypoint::Actor;
			Collect->WaypointActor = WeaponRack();
			return Collect;
		}
		case ETutorialGoal::HitDummies:
		{
			// The middle of the training ground, not one dummy: any of them counts. No dummies: it passes.
			UMissionHitObjective* Hit = NewObject<UMissionHitObjective>(Outer);
			Hit->Target.ActorClass = ATargetDummy::StaticClass();
			Hit->Count = FMath::Max(1, FMath::RoundToInt32(Step.Amount));
			Hit->bPlayerHitsOnly = true;
			Hit->bPassWithoutTargets = true;
			Hit->bShowCount = false;
			Hit->Waypoint = EMissionWaypoint::TargetsCenter;
			return Hit;
		}
		case ETutorialGoal::KillCreatures:
		{
			// The step asks for spiders, so the nearest one; any creature counts, so with no spider left, the nearest of
			// those. No creatures: it passes.
			UMissionKillObjective* Kill = NewObject<UMissionKillObjective>(Outer);
			Kill->Target.ActorClass = ACreatureBase::StaticClass();
			Kill->Count = FMath::Max(1, FMath::RoundToInt32(Step.Amount));
			Kill->bPlayerKillsOnly = true;
			Kill->bPassWithoutTargets = true;
			Kill->bShowCount = false;
			Kill->Waypoint = EMissionWaypoint::Actor;
			Kill->WaypointActor.ActorClass = ASpiderCreature::StaticClass();
			return Kill;
		}
		case ETutorialGoal::OpenInventory:
		{
			UMissionOpenPageObjective* Open = NewObject<UMissionOpenPageObjective>(Outer);
			Open->Page = EMissionPage::Any;
			Open->Waypoint = EMissionWaypoint::None;
			return Open;
		}
		}
		return nullptr;
	}
}

UMissionDefinition* ATutorialDirector::MakeBuiltInMission(UObject* Outer) const
{
	UMissionDefinition* Mission = NewObject<UMissionDefinition>(Outer ? Outer : GetTransientPackage(), NAME_None, RF_Transient);
	Mission->Id = MissionId;
	Mission->Title = FText::FromString(MissionTitle);
	Mission->Summary = FText::FromString(TEXT("Learn to move, fight and loot on Skyreach."));
	Mission->Kind = EMissionKind::Tutorial;
	Mission->Area = TEXT("Skyreach");
	Mission->Start = EMissionStart::Manual;
	for (const FTutorialStep& Step : Steps)
	{
		UMissionObjective* Objective = MakeObjective(Mission, Step);
		if (!Objective)
		{
			continue;
		}
		Objective->Text = FText::FromString(Step.Text);
		Mission->Steps.AddDefaulted_GetRef().Objectives.Add(Objective);
	}
	return Mission;
}
