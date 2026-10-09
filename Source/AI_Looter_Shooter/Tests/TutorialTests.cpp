#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Loot/WeaponRack.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionSubsystem.h"
#include "Missions/MissionText.h"
#include "Progression/PlayerProgressData.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Tutorial/TutorialChoice.h"
#include "Tutorial/TutorialDirector.h"
#include "World/NoticeBoard.h"
#include "World/SkiffJetty.h"
#include "UObject/Package.h"

// Skyreach's tutorial, reworked (Docs/Polish/TutorialRework.md): no forced checklist, one short first goal (find a gun in
// town, read the notice board), the board's postings after it, the tutorial done once Web Hollow is turned in, and the main
// menu's choice to start there or skip to the story.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialFirstGoalTest, "Looter.Tutorial.FirstGoal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTutorialFirstGoalTest::RunTest(const FString& Parameters)
{
	// Two steps, no more: a gun from the rack, then the notice board. Each has its words and a shorter line for the
	// tracker, and no key on the tracker (the contextual hints teach keys when they're needed).
	const ATutorialDirector* Director = GetDefault<ATutorialDirector>();
	if (!TestEqual(TEXT("Two steps"), Director->Steps.Num(), 2))
	{
		return false;
	}
	TestTrue(TEXT("A gun first"), Director->Steps[0].Goal == ETutorialGoal::HoldWeapon);
	TestTrue(TEXT("Then the notice board"), Director->Steps[1].Goal == ETutorialGoal::ReadBoard);
	// The photo scenes rest on the board step: a gun in hand, one goal still on the tracker.
	TestTrue(TEXT("The photo scenes' resting step is the notice board's"), Director->Steps.IsValidIndex(ATutorialDirector::BoardStep)
		&& Director->Steps[ATutorialDirector::BoardStep].Goal == ETutorialGoal::ReadBoard);
	TestEqual(TEXT("\"Find a gun in town\""), Director->Steps[0].ShortText, FString(TEXT("Find a gun in town")));
	TestEqual(TEXT("\"Read the notice board\""), Director->Steps[1].ShortText, FString(TEXT("Read the notice board")));
	for (const FTutorialStep& Step : Director->Steps)
	{
		TestFalse(TEXT("Step has text"), Step.Text.IsEmpty());
		TestTrue(TEXT("Step has an amount"), Step.Amount > 0.f);
		TestTrue(TEXT("A shorter line for the tracker"), !Step.ShortText.IsEmpty() && Step.ShortText.Len() < Step.Text.Len());
		TestTrue(TEXT("No key on the tracker"), Step.HintAction.IsNone() && Step.HintText.IsEmpty());
	}
	TestEqual(TEXT("The first goal's mission"), Director->MissionId, FName(TEXT("Tutorial")));
	TestFalse(TEXT("It has a name to show"), Director->MissionTitle.IsEmpty());

	// Web Hollow ends the tutorial: the director and the skiff's jetty agree on which mission that is.
	TestEqual(TEXT("The main posting"), Director->MainPostingId, FName(TEXT("WebHollow")));
	TestEqual(TEXT("The jetty waits on the same mission"), GetDefault<ASkiffJetty>()->TutorialMissionId, Director->MainPostingId);
	TestEqual(TEXT("The board tracks it as the postings go up"), GetDefault<ANoticeBoard>()->MainPosting, Director->MainPostingId);

	// A new game hasn't done the tutorial.
	TestFalse(TEXT("New game: tutorial not done"), FPlayerProgressData().bTutorialDone);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialStartTest, "Looter.Tutorial.Start",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTutorialStartTest::RunTest(const FString& Parameters)
{
	// What the director does as Skyreach begins, from what the session knows.
	TestTrue(TEXT("A new player: the first goal"), ATutorialDirector::DecideStart(false, false, false) == ETutorialStart::Teach);
	TestTrue(TEXT("The board read, Web Hollow not turned in: the postings carry on by themselves"),
		ATutorialDirector::DecideStart(false, true, false) == ETutorialStart::Postings);
	TestTrue(TEXT("Web Hollow turned in: done"), ATutorialDirector::DecideStart(true, true, true) == ETutorialStart::Done);
	TestTrue(TEXT("Turned in but the flag missed the save: done all the same"), ATutorialDirector::DecideStart(false, true, true)
		== ETutorialStart::Done);
	TestTrue(TEXT("Skipped from the main menu (the flag, nothing played): done, the postings there for practice"),
		ATutorialDirector::DecideStart(true, false, false) == ETutorialStart::Done);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialKeysTest, "Looter.Tutorial.Keys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTutorialKeysTest::RunTest(const FString& Parameters)
{
	// Outside a game there is no player to ask for bindings, so each {Action} shows its own name, in brackets; the
	// text around it is kept as it is.
	const ATutorialDirector* Director = GetDefault<ATutorialDirector>();
	TestEqual(TEXT("One key"), Director->ResolveKeys(TEXT("Press {Interact} now.")), FString(TEXT("Press [Interact] now.")));
	TestEqual(TEXT("Movement keys"), Director->ResolveKeys(TEXT("{Move}")),
		FString(TEXT("[MoveForward MoveLeft MoveBackward MoveRight]")));
	TestEqual(TEXT("No keys"), Director->ResolveKeys(TEXT("Plain text.")), FString(TEXT("Plain text.")));
	TestEqual(TEXT("Unclosed brace"), Director->ResolveKeys(TEXT("Odd {text")), FString(TEXT("Odd {text")));

	// A keycap shows the key alone, without brackets; the movement keys together (the control hints' keycaps too).
	TestEqual(TEXT("A keycap's key"), MissionText::KeyName(nullptr, TEXT("Reload")), FString(TEXT("Reload")));
	TestEqual(TEXT("A keycap's movement keys"), MissionText::KeyName(nullptr, TEXT("Move")),
		FString(TEXT("MoveForward MoveLeft MoveBackward MoveRight")));
	TestTrue(TEXT("No action, no key"), MissionText::KeyName(nullptr, NAME_None).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialTrackerTest, "Looter.Tutorial.Tracker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTutorialTrackerTest::RunTest(const FString& Parameters)
{
	// What the HUD's mission tracker shows for the first goal: its two short lines, no count, no key; the arrow on the gun
	// rack, then on the notice board. The full sentences stay the objectives' words, for the Missions page.
	const ATutorialDirector* Director = GetDefault<ATutorialDirector>();
	const UMissionDefinition* Mission = Director->MakeBuiltInMission(GetTransientPackage());
	if (!TestNotNull(TEXT("The built-in steps make a mission"), Mission) || !TestEqual(TEXT("Two steps"), Mission->Steps.Num(), 2))
	{
		return false;
	}
	const TCHAR* const Lines[] = { TEXT("Find a gun in town"), TEXT("Read the notice board") };
	for (int32 Index = 0; Index < 2; ++Index)
	{
		const UMissionObjective* Objective = Mission->GetObjective(Index, 0);
		const FString What = FString::Printf(TEXT("Step %d"), Index + 1);
		if (!TestNotNull(*(What + TEXT(" has its objective")), Objective))
		{
			continue;
		}
		FMissionTrackerParts Parts;
		Objective->FillTrackerParts(nullptr, FMissionObjectiveState(), Parts);
		TestEqual(*(What + TEXT(": the short line")), Parts.Line, FString(Lines[Index]));
		TestTrue(*(What + TEXT(": no count")), Parts.Count.IsEmpty() && Parts.CountDone.IsEmpty());
		TestTrue(*(What + TEXT(": no key")), Parts.HintKey.IsEmpty() && Parts.HintText.IsEmpty());
		TestFalse(*(What + TEXT(": not done in the inventory")), Parts.bOverInventory);
		TestEqual(*(What + TEXT(": the Missions page keeps the full sentence")), Objective->GetDisplayText(nullptr),
			MissionText::ResolveKeys(nullptr, Director->Steps[Index].Text));
	}
	const UMissionCollectObjective* Gun = Cast<UMissionCollectObjective>(Mission->GetObjective(0, 0));
	TestTrue(TEXT("A gun carried, the arrow on the rack"), Gun && Gun->What == EMissionCollect::Weapons && Gun->Count == 1
		&& Gun->Waypoint == EMissionWaypoint::Actor && Gun->WaypointActor.ActorClass == AWeaponRack::StaticClass());
	const UMissionEventObjective* Read = Cast<UMissionEventObjective>(Mission->GetObjective(1, 0));
	TestTrue(TEXT("The board read, the arrow on the board"), Read && Read->Event == ANoticeBoard::ReadEvent && Read->Count == 1
		&& Read->Waypoint == EMissionWaypoint::Actor && Read->WaypointActor.ActorClass == ANoticeBoard::StaticClass());
	TestTrue(TEXT("Outside the story, started by the director, finished by its last step"), Mission->Kind == EMissionKind::Tutorial
		&& Mission->Start == EMissionStart::Manual && Mission->TurnIn.bAutomatic && !Mission->Rewards.GivesExperience());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialMenuChoiceTest, "Looter.Tutorial.MenuChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTutorialMenuChoiceTest::RunTest(const FString& Parameters)
{
	// A new session asks: "Start on Skyreach (learn the basics)" or "Skip to Ransom's Rest". The skip is the session's own
	// (USessionSubsystem::ApplyTutorialSkip): the tutorial counts as done and the first cast-off as taken, so a practice
	// visit later finds the board's postings up and never offers the skiff's first trip.
	const FText Skyreach = FText::FromString(TEXT("Skyreach"));
	const FText Rest = FText::FromString(TEXT("Ransom's Rest"));
	TestEqual(TEXT("The call to action"), TutorialChoice::StartLabel(Skyreach).ToString(), FString(TEXT("Start on Skyreach (learn the basics)")));
	TestEqual(TEXT("The skip"), TutorialChoice::SkipLabel(Rest).ToString(), FString(TEXT("Skip to Ransom's Rest")));
	const FString Explained = TutorialChoice::Explain(Skyreach, Rest).ToString();
	TestTrue(TEXT("The words name both places"), Explained.Contains(TEXT("Skyreach")) && Explained.Contains(TEXT("Ransom's Rest")));
	TestFalse(TEXT("...and no story character (Skyreach is outside the story)"), Explained.Contains(TEXT("Ellis")) || Explained.Contains(TEXT("Delia")));

	UPackage* Scratch = CreatePackage(nullptr);
	UAreaDefinition* First = NewObject<UAreaDefinition>(Scratch, TEXT("DA_Area_RansomsRest"), RF_Transient);
	First->DisplayName = Rest;
	First->Map = FSoftObjectPath(TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::ApplyTutorialSkip(*Save, *First, USessionSubsystem::LoadBullpup());
	TestTrue(TEXT("Skipped: the tutorial is done"), Save->Progress.bTutorialDone);
	TestTrue(TEXT("...and the first cast-off taken"), Save->Campaign.bFirstCastOff);
	TestTrue(TEXT("On a practice visit the director counts it done: the first goal recorded, the postings up"),
		ATutorialDirector::DecideStart(Save->Progress.bTutorialDone, Save->Campaign.HasCompleted(TEXT("Tutorial")),
			Save->Campaign.HasCompleted(TEXT("WebHollow"))) == ETutorialStart::Done);
	TestFalse(TEXT("...and the skiff's first trip is never offered"), ASkiffJetty::ShowsBoardingFor(Save->Progress.bTutorialDone,
		Save->Campaign.bFirstCastOff));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponRackDefaultsTest, "Looter.Tutorial.WeaponRack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponRackDefaultsTest::RunTest(const FString& Parameters)
{
	// The rack's weapon lies on the gun rack model's Weapon socket, comes with ammo, and restocks after a while.
	const AWeaponRack* Rack = GetDefault<AWeaponRack>();
	TestEqual(TEXT("Socket"), Rack->WeaponSocket, FName(TEXT("Weapon")));
	TestTrue(TEXT("Comes with ammo"), Rack->AmmoMagazines > 0);
	TestTrue(TEXT("Restocks"), Rack->RestockSeconds > 0.f);
	TestEqual(TEXT("Level 1 for the tutorial"), Rack->Level, 1);

	// The minimap zoom setting starts at 1 (35 m around the player) and its range covers it.
	TestEqual(TEXT("Minimap zoom default"), GetDefault<ULooterGraphicsSave>()->MinimapZoom, 1.f);
	TestTrue(TEXT("Zoom range"), UGraphicsSettingsSubsystem::MinMinimapZoom < 1.f && UGraphicsSettingsSubsystem::MaxMinimapZoom > 1.f);
	return true;
}

#endif
