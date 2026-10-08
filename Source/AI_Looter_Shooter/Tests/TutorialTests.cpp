#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Loot/WeaponRack.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionSubsystem.h"
#include "Missions/MissionText.h"
#include "Progression/PlayerProgressData.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Tutorial/TutorialDirector.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialStepsTest, "Looter.Tutorial.Steps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTutorialStepsTest::RunTest(const FString& Parameters)
{
	// The tutorial island's route: move, reach the village, take the rifle, shoot the dummies, hunt spiders, open the
	// loadout. Every step has a text and something to do.
	const ATutorialDirector* Director = GetDefault<ATutorialDirector>();
	TestTrue(TEXT("Has steps"), Director->Steps.Num() >= 5);
	TestFalse(TEXT("Has a closing line"), Director->DoneText.IsEmpty());
	bool bTakesWeapon = false;
	for (const FTutorialStep& Step : Director->Steps)
	{
		TestFalse(TEXT("Step has text"), Step.Text.IsEmpty());
		TestTrue(TEXT("Step has an amount"), Step.Amount > 0.f);
		bTakesWeapon |= Step.Goal == ETutorialGoal::HoldWeapon;
		// The tracker's line: shorter than the full sentence, the key left to the hint, and a hint's key has its words.
		TestTrue(TEXT("Step has a shorter line for the tracker"), !Step.ShortText.IsEmpty() && Step.ShortText.Len() < Step.Text.Len());
		TestFalse(TEXT("The short line leaves the key to the hint"), Step.ShortText.Contains(TEXT("{")));
		TestTrue(TEXT("A hint's key comes with its words"), Step.HintAction.IsNone() == Step.HintText.IsEmpty());
	}
	TestTrue(TEXT("The player takes a weapon from the rack"), bTakesWeapon);
	// The mission's name is the tracker's title, so the first line doesn't repeat it.
	TestFalse(TEXT("The first line leaves Skyreach to the title"), Director->Steps.IsEmpty() || Director->Steps[0].ShortText.Contains(TEXT("Skyreach")));

	// A new game hasn't done the tutorial.
	TestFalse(TEXT("New game: tutorial not done"), FPlayerProgressData().bTutorialDone);
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

	// A keycap shows the key alone, without brackets; the movement keys together.
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
	// What the HUD's mission tracker shows for each tutorial step (the user's table): the short line, the count for the
	// dummies and the spiders, and the key hint. Outside a game each key shows its binding's name. The full sentences
	// stay the objectives' words, for the Missions page.
	const ATutorialDirector* Director = GetDefault<ATutorialDirector>();
	const UMissionDefinition* Mission = Director->MakeBuiltInMission(GetTransientPackage());
	if (!TestNotNull(TEXT("The built-in steps make a mission"), Mission) || !TestEqual(TEXT("Six steps"), Mission->Steps.Num(), 6))
	{
		return false;
	}
	struct FExpected
	{
		const TCHAR* Line;
		const TCHAR* Count;
		const TCHAR* CountDone;
		const TCHAR* Key;
		const TCHAR* Hint;
		bool bOverInventory;
	};
	const FExpected Table[] = {
		{ TEXT("Move and look around"), TEXT(""), TEXT(""), TEXT("MoveForward MoveLeft MoveBackward MoveRight"), TEXT("Move"), false },
		{ TEXT("Follow the road to the village"), TEXT(""), TEXT(""), TEXT("Sprint"), TEXT("Hold to run"), false },
		{ TEXT("Grab the rifle from the gun rack"), TEXT(""), TEXT(""), TEXT("Interact"), TEXT("Take it"), false },
		{ TEXT("Shoot the target dummies"), TEXT("0 / 5"), TEXT("5 / 5"), TEXT("Reload"), TEXT("Reload"), false },
		{ TEXT("Hunt spiders past the pond"), TEXT("0 / 2"), TEXT("2 / 2"), TEXT(""), TEXT(""), false },
		{ TEXT("Check your loadout"), TEXT(""), TEXT(""), TEXT("Inventory"), TEXT("Inventory"), true },
	};
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Table)); ++Index)
	{
		const UMissionObjective* Objective = Mission->GetObjective(Index, 0);
		const FString What = FString::Printf(TEXT("Step %d"), Index + 1);
		if (!TestNotNull(*(What + TEXT(" has its objective")), Objective))
		{
			continue;
		}
		FMissionTrackerParts Parts;
		Objective->FillTrackerParts(nullptr, FMissionObjectiveState(), Parts);
		const FExpected& Expected = Table[Index];
		TestEqual(*(What + TEXT(": the short line")), Parts.Line, FString(Expected.Line));
		TestEqual(*(What + TEXT(": the count")), Parts.Count, FString(Expected.Count));
		TestEqual(*(What + TEXT(": the count once done")), Parts.CountDone, FString(Expected.CountDone));
		TestEqual(*(What + TEXT(": the hint's key")), Parts.HintKey, FString(Expected.Key));
		TestEqual(*(What + TEXT(": the hint's words")), Parts.HintText, FString(Expected.Hint));
		TestTrue(*(What + TEXT(": over the inventory only when done there")), Parts.bOverInventory == Expected.bOverInventory);
		TestEqual(*(What + TEXT(": the Missions page keeps the full sentence")), Objective->GetDisplayText(nullptr),
			MissionText::ResolveKeys(nullptr, Director->Steps[Index].Text));
	}

	// A count under way: the parts carry it for the pop, and the full line keeps its "(2/5)".
	const UMissionObjective* Dummies = Mission->GetObjective(3, 0);
	if (Dummies)
	{
		FMissionObjectiveState Shooting;
		Shooting.Count = 2;
		FMissionTrackerParts Parts;
		Dummies->FillTrackerParts(nullptr, Shooting, Parts);
		TestTrue(TEXT("Two hits: 2 / 5"), Parts.Count == TEXT("2 / 5") && Parts.Progress == 2 && Parts.Required == 5);
		TestTrue(TEXT("The full line counts too"), Dummies->GetTrackerText(nullptr, Shooting).EndsWith(TEXT("(2/5)")));
	}
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
