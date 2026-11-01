#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Story/StoryLine.h"
#include "Story/StoryLineSet.h"
#include "Story/VaneBroadcast.h"
#include "World/CompanyBanner.h"
#include "World/CompanyHoseCart.h"
#include "World/CompanyLoudspeaker.h"
#include "World/CompanySign.h"
#include "World/GangSkiffFlyBy.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

// Main 3, "Under New Management" (the Wardens story since 2026-11-01; was "Cold Welcome": MAINS_2_7.md section 5; lane
// MAIN23): its new data, words and pieces. The gate's fight, Tilly's window, the shutters and the safe zones are still
// ColdWelcomeTests.cpp's.

namespace
{
	const FName ManagementMainThree(TEXT("Main3"));
	const FName ManagementBannerTag(TEXT("Banner_MainStreet"));
	const FName ManagementBannerEvent(TEXT("MainStreet.BannerDown"));
	const FName ManagementGateSpeaker(TEXT("Loudspeaker_TownGate"));
	const FName ManagementStreetSpeaker(TEXT("Loudspeaker_MainStreet"));
	const FName ManagementSaloonCart(TEXT("HoseCart_GiltSpur"));

	const UStoryLineSet* ManagementLoadLines(const TCHAR* Name)
	{
		const FString Package = FString(TEXT("/Game/Data/Story/")) + Name;
		return FPackageName::DoesPackageExist(Package) ? LoadObject<UStoryLineSet>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderNewManagementMissionTest, "Looter.Story.UnderNewManagement.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnderNewManagementMissionTest::RunTest(const FString& Parameters)
{
	// Main 3 as its asset has it: its new title and words, Delia's (met first), the checkpoint's seven, the banner optional
	// in the street's step, Tilly's three lines after the card and the bail into the player's pocket.
	if (!FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main3")))
	{
		AddWarning(TEXT("DA_Mission_Main3 isn't made yet: run Tools/Unreal/create_mission_assets.py."));
		return true;
	}
	const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main3.DA_Mission_Main3"));
	if (!TestNotNull(TEXT("DA_Mission_Main3 loads"), Asset))
	{
		return false;
	}
	TestEqual(TEXT("Under New Management"), Asset->Title.ToString(), FString(TEXT("Under New Management")));
	TestTrue(TEXT("Delia's word, met first"), Asset->Offer.bMeetFirst && Asset->Offer.GiverName.ToString() == TEXT("Delia Ransom"));
	const UMissionClearObjective* Gate = Cast<UMissionClearObjective>(Asset->GetObjective(1, 0));
	TestTrue(TEXT("2: the Company's checkpoint broken, all seven"), Gate && Gate->SpawnerId == FName(TEXT("TownGate")) && Gate->Count == 7);
	const UMissionEventObjective* Banner = Cast<UMissionEventObjective>(Asset->GetObjective(2, 1));
	TestTrue(TEXT("3: and, optional, the banner cut down"), Banner && Banner->bOptional && Banner->Event == ManagementBannerEvent);
	TestEqual(TEXT("Tilly's three lines after the card"), Asset->TurnIn.Lines.Num(), 3);
	TestTrue(TEXT("...the bail into the player's pocket"), Asset->TurnIn.HandItems.Contains(FName(TEXT("LanternBail"))));
	TestFalse(TEXT("...no word of the old story (laying out the hero)"), Asset->TurnIn.Lines.ContainsByPredicate([](const FStoryLine& Line)
	{
		return Line.Text.ToString().Contains(TEXT("laid you out")) || Line.Text.ToString().Contains(TEXT("collar"));
	}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderNewManagementLinesTest, "Looter.Story.UnderNewManagement.Lines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnderNewManagementLinesTest::RunTest(const FString& Parameters)
{
	// The words as the story script writes them: Vane's loudspeaker lines all his; Ned on the radio by name; Hob names him
	// after; every caption short enough to read.
	const UStoryLineSet* Radio = ManagementLoadLines(TEXT("DA_Lines_Main3Radio"));
	if (!Radio)
	{
		AddWarning(TEXT("The story's line sets aren't made yet: run Tools/Unreal/create_story_lines.py."));
		return true;
	}
	TestTrue(TEXT("Vane sends Ned off, and Ned answers, by name"), Radio->Lines.Num() == 2
		&& Radio->Lines[0].Speaker.ToString() == UVaneBroadcast::SpeakerName().ToString() && Radio->Lines[1].Speaker.ToString() == TEXT("Lucky Ned"));
	if (const UStoryLineSet* After = ManagementLoadLines(TEXT("DA_Lines_Main3NedAfter")))
	{
		TestTrue(TEXT("Hob names him after"), !After->Lines.IsEmpty() && After->Lines[0].Speaker.ToString() == TEXT("Hob")
			&& After->Lines[0].Text.ToString().Contains(TEXT("Lucky Ned")));
	}
	for (const TCHAR* Name : { TEXT("DA_Lines_Main3VaneGate"), TEXT("DA_Lines_Main3VaneStreet") })
	{
		if (const UStoryLineSet* Vane = ManagementLoadLines(Name))
		{
			TestFalse(FString::Printf(TEXT("%s: all Vane's"), Name), Vane->Lines.ContainsByPredicate([](const FStoryLine& Line)
			{
				return Line.Speaker.ToString() != UVaneBroadcast::SpeakerName().ToString();
			}));
		}
	}
	const TCHAR* const Sets[] = { TEXT("DA_Lines_HobMain3Road"), TEXT("DA_Lines_Main3Radio"), TEXT("DA_Lines_Main3NedAfter"),
		TEXT("DA_Lines_HobMain3Gate"), TEXT("DA_Lines_Main3VaneGate"), TEXT("DA_Lines_Main3VaneStreet"), TEXT("DA_Lines_HobMain3Tilly"),
		TEXT("DA_Lines_TillyClosed"), TEXT("DA_Lines_TillyMain3"), TEXT("DA_Lines_TillyAfterMain3"), TEXT("DA_Lines_DeliaMain3") };
	for (const TCHAR* Name : Sets)
	{
		const UStoryLineSet* Set = ManagementLoadLines(Name);
		if (!TestNotNull(FString::Printf(TEXT("%s is made"), Name), Set))
		{
			continue;
		}
		for (const FStoryLine& Line : Set->Lines)
		{
			TestTrue(FString::Printf(TEXT("%s: \"%s\" fits a caption (90 characters)"), Name, *Line.Text.ToString()), Line.Text.ToString().Len() <= 90);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnderNewManagementPlacedTest, "Looter.Story.UnderNewManagement.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnderNewManagementPlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (build_area_main3.py): the checkpoint's signs, the gate's and the street's loudspeakers with
	// their cues, the banner and its event, the chalk on the doors, the Gilt Spur's cart, and Ned's skiff.
	if (!FPackageName::DoesPackageExist(TEXT("/Game/Maps/Lvl_RansomsRest")))
	{
		AddInfo(TEXT("Lvl_RansomsRest isn't built: skipped."));
		return true;
	}
	const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
	const ULevel* Level = TestNotNull(TEXT("Lvl_RansomsRest loads"), Map) ? Map->PersistentLevel.Get() : nullptr;
	if (!Level)
	{
		return true;
	}
	int32 Boards = 0;
	int32 Chalk = 0;
	const ACompanyLoudspeaker* GateHorn = nullptr;
	const ACompanyLoudspeaker* StreetVoice = nullptr;
	const ACompanyBanner* Banner = nullptr;
	const ACompanyHoseCart* Cart = nullptr;
	const AGangSkiffFlyBy* Skiff = nullptr;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (const ACompanySign* Sign = Cast<ACompanySign>(Actor))
		{
			Boards += Sign->Look == ECompanySignLook::Board ? 1 : 0;
			Chalk += Sign->Look == ECompanySignLook::Chalk ? 1 : 0;
		}
		if (const ACompanyLoudspeaker* Horn = Cast<ACompanyLoudspeaker>(Actor))
		{
			GateHorn = Horn->ActorHasTag(ManagementGateSpeaker) ? Horn : GateHorn;
			StreetVoice = Horn->ActorHasTag(ManagementStreetSpeaker) ? Horn : StreetVoice;
		}
		Banner = Banner ? Banner : Cast<ACompanyBanner>(Actor);
		if (const ACompanyHoseCart* Found = Cast<ACompanyHoseCart>(Actor); Found && Found->ActorHasTag(ManagementSaloonCart))
		{
			Cart = Found;
		}
		Skiff = Skiff ? Skiff : Cast<AGangSkiffFlyBy>(Actor);
	}
	if (!Banner && !Skiff && Boards == 0)
	{
		AddWarning(TEXT("Main 3's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	TestTrue(TEXT("The checkpoint's barrier and board"), Boards >= 2);
	TestTrue(TEXT("ASSESSED chalked on Main Street's doors"), Chalk >= 5);
	if (TestNotNull(TEXT("The gate's loudspeaker"), GateHorn))
	{
		TestTrue(TEXT("...Vane's word as the checkpoint's fight begins"), GateHorn->Cues.ContainsByPredicate([](const FLoudspeakerCue& Cue)
		{
			return Cue.When.DuringMission == ManagementMainThree && Cue.When.FromStep == 1 && Cue.Lines != nullptr;
		}));
	}
	if (TestNotNull(TEXT("Main Street's loudspeaker"), StreetVoice))
	{
		TestTrue(TEXT("...Vane's word once the street is won"), StreetVoice->Cues.ContainsByPredicate([](const FLoudspeakerCue& Cue)
		{
			return Cue.When.WithFlags.Contains(FName(TEXT("SetPiece.MainStreet"))) && Cue.Lines != nullptr;
		}));
	}
	if (TestNotNull(TEXT("The banner"), Banner))
	{
		TestTrue(TEXT("...tagged for its arrow, telling the missions as it falls"), Banner->ActorHasTag(ManagementBannerTag)
			&& Banner->DownEvent == ManagementBannerEvent && !Banner->DownFlag.IsNone());
		TestTrue(TEXT("...tied across the street (its ropes' ties apart)"), Banner->AnchorRight.Y - Banner->AnchorLeft.Y > ACompanyBanner::ClothLength);
	}
	if (TestNotNull(TEXT("The Gilt Spur's hose-cart"), Cart))
	{
		TestTrue(TEXT("...pumping in Main 3, stopped after"), Cart->PumpingWhen.DuringMission == ManagementMainThree && !Cart->StoppedWhen.IsEmpty());
	}
	if (TestNotNull(TEXT("Lucky Ned's skiff"), Skiff))
	{
		TestTrue(TEXT("...a course off the barge and out of the valley"), Skiff->Course.Num() >= 3);
		TestTrue(TEXT("...as Main 3 starts, once"), Skiff->PlayWhen.DuringMission == ManagementMainThree && !Skiff->DoneFlag.IsNone()
			&& Skiff->RadioLines != nullptr);
	}
	return true;
}

#endif
