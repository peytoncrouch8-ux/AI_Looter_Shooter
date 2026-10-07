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
	// Handed over as the deal ends: Main 2 on its "Open the Ledger" step, here or wherever the campaign stands.
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
	return bOpen ? TEXT("Ledger") : TEXT("Bestiary");
}

const Ledger::FWords& Ledger::Words(bool bOpen)
{
	// The field guide's words, as the bestiary always had them.
	static const FWords Guide = []()
	{
		FWords Made;
		Made.ListTitle = TEXT("Field guide");
		Made.One = TEXT("entry");
		Made.Many = TEXT("entries");
		Made.NotMet = TEXT("Not met yet");
		Made.NotMetHelp = TEXT("You haven't met one yet. Find it out in the world, or let it find you, to fill in this page.");
		Made.NotMetPersonHelp = TEXT("You haven't met them yet.");
		Made.Empty = TEXT("Bestiary");
		Made.EmptyHelp = TEXT("The creatures, enemies and people you meet are written up here.");
		Made.BlankWhereabouts = TEXT("Unknown");
		return Made;
	}();
	// Sexton's: polite, patient, and keeping count (first drafts, for the user to judge).
	static const FWords Book = []()
	{
		FWords Made;
		Made.ListTitle = TEXT("The Ledger");
		Made.One = TEXT("account");
		Made.Many = TEXT("accounts");
		Made.NotMet = TEXT("Not yet acquainted");
		Made.NotMetHelp = TEXT("We haven't been introduced, friend. Make its acquaintance, and I'll write it in.");
		Made.NotMetPersonHelp = TEXT("You two haven't met, friend. You will.");
		Made.Empty = TEXT("Ledger");
		Made.EmptyHelp = TEXT("Every soul you meet goes in my book, friend.");
		Made.BlankWhereabouts = TEXT("______________");
		return Made;
	}();
	return bOpen ? Book : Guide;
}
