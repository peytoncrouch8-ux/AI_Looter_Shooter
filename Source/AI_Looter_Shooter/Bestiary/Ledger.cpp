#include "Bestiary/Ledger.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/StoryCondition.h"

bool Ledger::IsOpen(const FCampaignRecord& Campaign, const UMissionRunner* Runner)
{
	if (Campaign.HasCompleted(Mission))
	{
		return true;
	}
	// Handed over as the deal ends: Main 2 on its "Open the Ledger" step (its words are the mission asset's), here or
	// wherever the campaign stands.
	FStoryCondition HandedOver;
	HandedOver.DuringMission = Mission;
	HandedOver.FromStep = HandedOverStep;
	return HandedOver.IsMet(Campaign, Runner);
}

bool Ledger::IsOpenIn(const UObject* WorldContextObject)
{
	const UMissionRunner* Runner = UMissionRunner::Get(WorldContextObject);
	return Runner && IsOpen(Runner->GetCampaign(), Runner);
}

const TCHAR* Ledger::BookName(bool bOpen)
{
	// The Warden's Log (the user's call, 2026-10-31: the bestiary and Sexton's Ledger are one book, the Wardens' own, so it
	// carries the name from the first page; opening it only writes the story's pages in).
	return TEXT("Warden's Log");
}

const Ledger::FWords& Ledger::Words(bool bOpen)
{
	// A Warden's field notes: plain, practical, written for the next Warden to read.
	static const FWords Log = []()
	{
		FWords Made;
		Made.ListTitle = TEXT("The Warden's Log");
		Made.One = TEXT("entry");
		Made.Many = TEXT("entries");
		Made.NotMet = TEXT("Not met yet");
		Made.NotMetHelp = TEXT("No entry yet. Find it out in the world, or let it find you, and it goes in the log.");
		Made.NotMetPersonHelp = TEXT("You haven't met them yet.");
		Made.Empty = TEXT("Warden's Log");
		Made.EmptyHelp = TEXT("The creatures, enemies and people you meet are logged here.");
		Made.BlankWhereabouts = TEXT("Unknown");
		return Made;
	}();
	// Opened, the story's pages are in it too: the names' whereabouts a blank line until the Lantern finds them.
	static const FWords Opened = []()
	{
		FWords Made = Log;
		Made.BlankWhereabouts = TEXT("______________");
		return Made;
	}();
	return bOpen ? Opened : Log;
}
