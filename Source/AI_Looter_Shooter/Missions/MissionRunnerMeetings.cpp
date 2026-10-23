// UMissionRunner: main missions are never offered on a card (the user's call, 2026-10-22). One met first
// (FMissionOffer::bMeetFirst: Main 3 and Main 6, Delia's word) is listed and tracked as "Talk to <giver>" from the moment
// it's due until the talk that starts it (USpeakerPointComponent::Talk accepts it); the others start by themselves
// (StartDue), and here they're tracked and their giver's words said on the move.

#include "Missions/MissionRunner.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionSubsystem.h"
#include "Missions/MissionTalk.h"

#define LOCTEXT_NAMESPACE "MissionMeetings"

bool UMissionRunner::IsMeeting(FName MissionId) const
{
	// Worked out from the campaign, never saved: a save's main mission left at its giver is met again as the level begins.
	const UMissionDefinition* Mission = FindDefinition(MissionId);
	return Mission && Mission->IsMetFirst() && IsAvailable(MissionId);
}

FText UMissionRunner::MeetText(const UMissionDefinition& Mission, bool bShort)
{
	// The giver as the played hero names them (GetOfferGiverName goes through HeroLines).
	const FText Giver = Mission.GetOfferGiverName();
	return bShort ? FText::Format(LOCTEXT("MeetShort", "Talk to {0}"), Giver) : FText::Format(LOCTEXT("Meet", "Talk to {0}."), Giver);
}

void UMissionRunner::SyncMeetings(UMissionSubsystem& Display)
{
	// Off the list once started (its run has its own line now) or no longer due (finished from the console).
	for (int32 Index = Meetings.Num() - 1; Index >= 0; --Index)
	{
		if (IsMeeting(Meetings[Index].Id))
		{
			continue;
		}
		if (Meetings[Index].BookId != INDEX_NONE)
		{
			Display.RemoveMission(Meetings[Index].BookId);
		}
		Meetings.RemoveAt(Index);
		bChanged = true;
	}
	for (UMissionDefinition* Mission : Definitions)
	{
		const FName MissionId = Mission ? Mission->GetMissionId() : NAME_None;
		if (!Mission || !IsMeeting(MissionId))
		{
			continue;
		}
		FMeeting* Meeting = Meetings.FindByPredicate([MissionId](const FMeeting& Each) { return Each.Id == MissionId; });
		const bool bNew = Meeting == nullptr;
		if (bNew)
		{
			Meeting = &Meetings.AddDefaulted_GetRef();
			Meeting->Id = MissionId;
			Meeting->Mission = Mission;
			Meeting->BookId = Display.AddMission(Mission->Title);
			bChanged = true;
			UE_LOG(LogLooter, Log, TEXT("Mission %s: listed, waiting for %s to be talked to."), *MissionId.ToString(),
				*Mission->GetOfferGiverName().ToString());
		}
		// One objective, the talk: the step bar shows the mission's steps all still to come.
		FMissionTrackerParts Tracker;
		Tracker.Line = MeetText(*Mission, /*bShort*/ true).ToString();
		Tracker.Step = 0;
		Tracker.StepCount = FMath::Max(Mission->Steps.Num(), 1);
		Tracker.ObjectiveIndex = 0;
		Tracker.bAnnouncedEnd = true;
		// Unchanged words and a giver who only moved (Hob) don't wake the display's listeners.
		Display.SetObjective(Meeting->BookId, MeetText(*Mission), FindOfferGiver(*Mission), Tracker);
		if (bNew)
		{
			// The story's next step is what the player follows: the tracker and the minimap's arrow on the giver.
			Display.TrackMission(Meeting->BookId);
		}
	}
}

void UMissionRunner::SayStartTalks()
{
	if (StartTalks.IsEmpty())
	{
		return;
	}
	// A copy: tracking tells listeners, who may ask the runner things.
	const TArray<FName> Started = MoveTemp(StartTalks);
	StartTalks.Reset();
	for (const FName MissionId : Started)
	{
		const UMissionDefinition* Mission = FindDefinition(MissionId);
		if (!Mission || !IsRunning(MissionId))
		{
			continue;
		}
		TrackMission(MissionId);
		// After whatever is being said (the turn-in that unlocked it), wherever the player is.
		MissionTalk::SayOnTheMove(this, *Mission, FindOfferGiverActor(*Mission));
	}
}

#undef LOCTEXT_NAMESPACE
