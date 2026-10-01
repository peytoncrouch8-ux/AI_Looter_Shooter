#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Loot/WeaponRack.h"
#include "Progression/PlayerProgressData.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Tutorial/TutorialDirector.h"

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
	}
	TestTrue(TEXT("The player takes a weapon from the rack"), bTakesWeapon);

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
