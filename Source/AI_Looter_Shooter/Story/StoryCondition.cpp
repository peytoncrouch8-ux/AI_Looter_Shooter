#include "Story/StoryCondition.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"

namespace
{
	FString JoinIds(const TArray<FName>& Ids)
	{
		return FString::JoinBy(Ids, TEXT(", "), [](const FName& Id) { return Id.ToString(); });
	}
}

bool FStoryCondition::IsEmpty() const
{
	return AfterMissions.IsEmpty() && BeforeMissions.IsEmpty() && DuringMission.IsNone();
}

bool FStoryCondition::IsMet(const FCampaignRecord& Campaign, const UMissionRunner* Runner) const
{
	for (const FName Needed : AfterMissions)
	{
		if (!Needed.IsNone() && !Campaign.HasCompleted(Needed))
		{
			return false;
		}
	}
	for (const FName NotYet : BeforeMissions)
	{
		if (!NotYet.IsNone() && Campaign.HasCompleted(NotYet))
		{
			return false;
		}
	}
	if (!DuringMission.IsNone())
	{
		// The campaign's main mission is being played wherever the player is; a side mission only while it runs here.
		const bool bPlayed = Campaign.ActiveMission == DuringMission || (Runner && Runner->IsRunning(DuringMission));
		if (!bPlayed)
		{
			return false;
		}
	}
	return true;
}

FString FStoryCondition::Describe() const
{
	TArray<FString> Parts;
	if (!AfterMissions.IsEmpty())
	{
		Parts.Add(TEXT("after ") + JoinIds(AfterMissions));
	}
	if (!BeforeMissions.IsEmpty())
	{
		Parts.Add(TEXT("before ") + JoinIds(BeforeMissions));
	}
	if (!DuringMission.IsNone())
	{
		Parts.Add(TEXT("during ") + DuringMission.ToString());
	}
	return Parts.IsEmpty() ? FString(TEXT("always")) : FString::Join(Parts, TEXT("; "));
}
