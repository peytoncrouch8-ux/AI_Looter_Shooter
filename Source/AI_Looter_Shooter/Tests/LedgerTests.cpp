#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bestiary/BestiaryEntry.h"
#include "Bestiary/Ledger.h"
#include "Session/CampaignRecord.h"
#include "Story/StoryCondition.h"
#include "UObject/Package.h"

// The Ledger (Docs/Areas/RansomsRest.md, "The Ledger"; Bestiary/Ledger.h): the bestiary becomes Sexton's book at Main 2's
// last step, with the new page type for the story's characters (no actor) and the seven names, whereabouts blank.

namespace
{
	const FName MainOne(TEXT("Main1"));
	const FName MainTwo(TEXT("Main2"));
	const FName MainThree(TEXT("Main3"));
	const FName MainSeven(TEXT("Main7"));

	UBestiaryEntry* NewPage(EBestiaryPage Page, const TCHAR* Name)
	{
		UBestiaryEntry* Entry = NewObject<UBestiaryEntry>(GetTransientPackage(), NAME_None, RF_Transient);
		Entry->Page = Page;
		Entry->DisplayName = FText::FromString(Name);
		return Entry;
	}

	/** The story's character pages the Ledger must have (Docs/Areas/RansomsRest.md, NPCs), by asset. */
	const TCHAR* const StoryPages[] = { TEXT("DA_Bestiary_Sexton"), TEXT("DA_Bestiary_Delia"), TEXT("DA_Bestiary_Tilly"),
		TEXT("DA_Bestiary_Aldana"), TEXT("DA_Bestiary_Ruth") };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLedgerTest, "Looter.Bestiary.Ledger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLedgerTest::RunTest(const FString& Parameters)
{
	// When the bestiary is the Ledger: from Main 2's "Open the Ledger" step, and for good once Main 2 is done; the same on
	// any map, since it's read from the campaign record.
	FCampaignRecord Campaign;
	TestFalse(TEXT("A new story: the field guide"), Ledger::IsOpen(Campaign));
	Campaign.ActiveMission = MainOne;
	TestFalse(TEXT("During Main 1: not yet"), Ledger::IsOpen(Campaign));
	Campaign.Complete(MainOne);
	Campaign.ActiveMission = MainTwo;
	Campaign.ActiveMissionStep = Ledger::HandedOverStep - 1;
	TestFalse(TEXT("Main 2 at the deal: not yet"), Ledger::IsOpen(Campaign));
	Campaign.ActiveMissionStep = Ledger::HandedOverStep;
	TestTrue(TEXT("Main 2 at \"Open the Ledger\": his"), Ledger::IsOpen(Campaign));
	Campaign.Complete(MainTwo);
	Campaign.ActiveMission = MainThree;
	Campaign.ActiveMissionStep = 0;
	TestTrue(TEXT("Main 2 done: his for good"), Ledger::IsOpen(Campaign));
	TestEqual(TEXT("Its tab, before"), FString(Ledger::BookName(false)), FString(TEXT("Bestiary")));
	TestEqual(TEXT("...and after"), FString(Ledger::BookName(true)), FString(TEXT("Ledger")));
	TestNotEqual(TEXT("Its words change with it"), Ledger::Words(true).ListTitle, Ledger::Words(false).ListTitle);
	TestFalse(TEXT("A blank to fill in for the whereabouts"), Ledger::Words(true).BlankWhereabouts.IsEmpty());

	// The page types. An actor page: listed always, open once met (or once its KnownWhen holds).
	FCampaignRecord Fresh;
	UBestiaryEntry* Spider = NewPage(EBestiaryPage::Actor, TEXT("Meadow Wolf"));
	TestTrue(TEXT("An actor page needs its actor"), Spider->NeedsActor());
	TestTrue(TEXT("...is listed in the field guide and the Ledger alike"), Spider->IsListed(false) && Spider->IsListed(true));
	TestFalse(TEXT("...is closed until met"), Spider->IsKnown(false, Fresh));
	TestTrue(TEXT("...and open once met"), Spider->IsKnown(true, Fresh));

	// A story character's page: no actor; open once the story has met them; in the Ledger only when it says so.
	UBestiaryEntry* Tilly = NewPage(EBestiaryPage::StoryCharacter, TEXT("Tilly Bright"));
	Tilly->bLedgerOnly = true;
	Tilly->KnownWhen.AfterMissions = { MainThree };
	TestFalse(TEXT("A story character's page needs no actor"), Tilly->NeedsActor());
	TestTrue(TEXT("...is written in the Ledger only"), !Tilly->IsListed(false) && Tilly->IsListed(true));
	TestFalse(TEXT("...is closed before Main 3, met or not"), Tilly->IsKnown(true, Fresh));
	FCampaignRecord AfterThree;
	AfterThree.Complete(MainThree);
	TestTrue(TEXT("...and open after it, unmet in the world"), Tilly->IsKnown(false, AfterThree));
	UBestiaryEntry* Sexton = NewPage(EBestiaryPage::StoryCharacter, TEXT("Mister Sexton"));
	TestTrue(TEXT("One with no condition is open from the start (Sexton wrote it)"), Sexton->IsKnown(false, Fresh));
	TestTrue(TEXT("...and, not in the Ledger only, listed in the field guide too"), Sexton->IsListed(false));

	// A Ledger name: in the Ledger only (whatever bLedgerOnly says), open from the start, its whereabouts blank until found.
	UBestiaryEntry* Ned = NewPage(EBestiaryPage::LedgerName, TEXT("Lucky Ned Purcell"));
	Ned->Habitat = FText::FromString(TEXT("The Gilded Lily"));
	Ned->FoundWhen.AfterMissions = { MainSeven };
	TestTrue(TEXT("A name is in the Ledger only"), !Ned->IsListed(false) && Ned->IsListed(true));
	TestTrue(TEXT("...and written in from the first"), Ned->IsKnown(false, Fresh));
	TestFalse(TEXT("...its whereabouts blank"), Ned->IsFound(Fresh));
	FCampaignRecord AfterSeven;
	AfterSeven.Complete(MainSeven);
	TestTrue(TEXT("...until the lantern finds him (Main 7)"), Ned->IsFound(AfterSeven));
	UBestiaryEntry* Ira = NewPage(EBestiaryPage::LedgerName, TEXT("Whistling Ira Gale"));
	TestFalse(TEXT("A name with nothing to find him by stays blank"), Ira->IsFound(AfterSeven));

	// The pages as create_bestiary_pages.py writes them, when it has.
	const TArray<UBestiaryEntry*> Entries = UBestiaryEntry::LoadAll();
	TArray<const UBestiaryEntry*> Names;
	for (const UBestiaryEntry* Entry : Entries)
	{
		if (Entry->Page == EBestiaryPage::LedgerName)
		{
			Names.Add(Entry);
		}
	}
	if (Names.IsEmpty())
	{
		AddWarning(TEXT("The Ledger's pages aren't made yet: run Tools/Unreal/create_bestiary_pages.py."));
		return true;
	}
	TestEqual(TEXT("Seven names in the Ledger"), Names.Num(), 7);
	for (const UBestiaryEntry* Name : Names)
	{
		const FString Who = Name->DisplayName.ToString();
		TestTrue(Who + TEXT(": in the Ledger only"), !Name->IsListed(false) && Name->IsListed(true));
		TestTrue(Who + TEXT(": an enemy"), Name->Category == EBestiaryCategory::Enemy);
		TestFalse(Who + TEXT(": whereabouts blank on a new story"), Name->IsFound(Fresh));
		TestTrue(Who + TEXT(": listed before the other enemies"), Name->SortOrder < 0);
	}
	TestEqual(TEXT("One name is found by Main 7: the Gilded Lily's"), Names.FilterByPredicate([&AfterSeven](const UBestiaryEntry* Name)
	{
		return Name->IsFound(AfterSeven);
	}).Num(), 1);
	for (const TCHAR* Asset : StoryPages)
	{
		const UBestiaryEntry* const* Found = Entries.FindByPredicate([Asset](const UBestiaryEntry* Entry) { return Entry->GetName() == Asset; });
		if (TestNotNull(FString::Printf(TEXT("%s is made"), Asset), Found))
		{
			TestTrue(FString::Printf(TEXT("%s: a story character's page, in the Ledger, with no actor"), Asset),
				(*Found)->Page == EBestiaryPage::StoryCharacter && (*Found)->bLedgerOnly && !(*Found)->NeedsActor());
		}
	}
	return true;
}

#endif
