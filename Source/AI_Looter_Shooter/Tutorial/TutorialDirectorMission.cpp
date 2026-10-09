// ATutorialDirector: its built-in steps as a mission (what DA_Mission_Tutorial holds).

#include "Tutorial/TutorialDirector.h"
#include "Loot/WeaponRack.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlayerObjectives.h"
#include "World/NoticeBoard.h"
#include "UObject/Package.h"

namespace
{
	/** The objective that finishes a step with this goal, and where its arrow points. */
	UMissionObjective* MakeObjective(UObject* Outer, const FTutorialStep& Step)
	{
		switch (Step.Goal)
		{
		case ETutorialGoal::HoldWeapon:
		{
			// Carrying a gun; the arrow on the rifle while it lies on the rack (the rack once it's gone), which is also where
			// the road from the farm leads.
			UMissionCollectObjective* Collect = NewObject<UMissionCollectObjective>(Outer);
			Collect->What = EMissionCollect::Weapons;
			Collect->Count = FMath::Max(1, FMath::RoundToInt32(Step.Amount));
			Collect->Waypoint = EMissionWaypoint::Actor;
			Collect->WaypointActor.ActorClass = AWeaponRack::StaticClass();
			return Collect;
		}
		case ETutorialGoal::ReadBoard:
		{
			// The board tells the missions it was read; the arrow on it. One read: no count on the tracker.
			UMissionEventObjective* Read = NewObject<UMissionEventObjective>(Outer);
			Read->Event = ANoticeBoard::ReadEvent;
			Read->Count = FMath::Max(1, FMath::RoundToInt32(Step.Amount));
			Read->bShowCount = false;
			Read->Waypoint = EMissionWaypoint::Actor;
			Read->WaypointActor.ActorClass = ANoticeBoard::StaticClass();
			return Read;
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
	Mission->Summary = FText::FromString(TEXT("Find yourself a gun, then see what the town's notice board has going."));
	Mission->Kind = EMissionKind::Tutorial;
	Mission->Area = TEXT("Skyreach");
	Mission->Start = EMissionStart::Manual;
	// Finished by its last step: the postings it puts up are what's turned in.
	Mission->TurnIn.bAutomatic = true;
	for (const FTutorialStep& Step : Steps)
	{
		UMissionObjective* Objective = MakeObjective(Mission, Step);
		if (!Objective)
		{
			continue;
		}
		// The full sentence for the Missions page; the short line (and a key hint, when a step has one) for the HUD's tracker.
		Objective->Text = FText::FromString(Step.Text);
		Objective->ShortText = FText::FromString(Step.ShortText);
		Objective->HintAction = Step.HintAction;
		Objective->HintText = FText::FromString(Step.HintText);
		Mission->Steps.AddDefaulted_GetRef().Objectives.Add(Objective);
	}
	return Mission;
}
