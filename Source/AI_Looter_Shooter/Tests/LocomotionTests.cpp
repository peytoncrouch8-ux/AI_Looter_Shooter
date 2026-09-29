#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerLocomotionComponent.h"
#include "Player/StanceIntent.h"

namespace
{
	using EStance = FStanceIntent::EStance;

	FStanceIntent MakeIntent(bool bSprintToggle, bool bCrouchToggle)
	{
		FStanceIntent Intent;
		Intent.SetModes(bSprintToggle, bCrouchToggle);
		return Intent;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceHoldModeTest, "Looter.Locomotion.Stance.HoldMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceHoldModeTest::RunTest(const FString& Parameters)
{
	FStanceIntent Intent = MakeIntent(false, false);
	TestFalse(TEXT("Nothing wanted at rest"), Intent.WantsSprint() || Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Holding sprint sprints"), Intent.WantsSprint());
	Intent.Release(EStance::Sprint);
	TestFalse(TEXT("Letting go stops"), Intent.WantsSprint());

	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("Holding crouch crouches"), Intent.WantsCrouch());
	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Sprint pressed while crouching wins"), Intent.WantsSprint());
	TestFalse(TEXT("...and stands up"), Intent.WantsCrouch());
	Intent.Release(EStance::Sprint);
	TestTrue(TEXT("Crouch still held takes over again"), Intent.WantsCrouch());
	Intent.Release(EStance::Crouch);
	TestFalse(TEXT("All released"), Intent.WantsSprint() || Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	Intent.CancelSprintToggle();
	TestTrue(TEXT("Shooting doesn't erase a held sprint key (it resumes after)"), Intent.WantsSprint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceToggleModeTest, "Looter.Locomotion.Stance.ToggleMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceToggleModeTest::RunTest(const FString& Parameters)
{
	FStanceIntent Intent = MakeIntent(true, true);

	Intent.Press(EStance::Sprint);
	Intent.Release(EStance::Sprint);
	TestTrue(TEXT("Toggle sprint stays on after release"), Intent.WantsSprint());
	Intent.Press(EStance::Sprint);
	TestFalse(TEXT("Second press turns it off"), Intent.WantsSprint());

	Intent.Press(EStance::Crouch);
	Intent.Release(EStance::Crouch);
	TestTrue(TEXT("Toggle crouch stays on"), Intent.WantsCrouch());
	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Sprint from a toggled crouch"), Intent.WantsSprint());
	TestFalse(TEXT("clears the crouch"), Intent.WantsCrouch());
	Intent.CancelSprintToggle();
	TestFalse(TEXT("Stopping/shooting ends a toggled sprint"), Intent.WantsSprint());
	TestFalse(TEXT("and the old crouch doesn't come back"), Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("Crouch from a toggled sprint"), Intent.WantsCrouch());
	TestFalse(TEXT("ends the sprint"), Intent.WantsSprint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceMixedModeTest, "Looter.Locomotion.Stance.MixedModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceMixedModeTest::RunTest(const FString& Parameters)
{
	// Toggle sprint, hold crouch.
	FStanceIntent Intent = MakeIntent(true, false);
	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("Held crouch overrides toggled sprint"), Intent.WantsCrouch());
	TestFalse(TEXT("Sprint not wanted while crouch held"), Intent.WantsSprint());
	Intent.Release(EStance::Crouch);
	TestFalse(TEXT("Toggled sprint was cleared by the crouch"), Intent.WantsSprint());

	// Hold sprint, toggle crouch.
	Intent = MakeIntent(false, true);
	Intent.Press(EStance::Crouch);
	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Held sprint overrides toggled crouch"), Intent.WantsSprint());
	Intent.Release(EStance::Sprint);
	TestFalse(TEXT("Toggled crouch was cleared by the sprint"), Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	Intent.Reset();
	TestFalse(TEXT("Reset (lost control) clears everything"), Intent.WantsSprint() || Intent.WantsCrouch());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionDefaultsTest, "Looter.Locomotion.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLocomotionDefaultsTest::RunTest(const FString& Parameters)
{
	const UPlayerLocomotionComponent* Defaults = GetDefault<UPlayerLocomotionComponent>();
	TestTrue(TEXT("Sprint is faster than walking"), Defaults->SprintSpeedMultiplier > 1.f);
	TestTrue(TEXT("Crouch is slower than walking"), Defaults->CrouchSpeed < 600.f);
	TestTrue(TEXT("Crouched capsule is shorter than standing (96)"), Defaults->CrouchedHalfHeight < 96.f);
	TestTrue(TEXT("Crouching tightens spread"), Defaults->CrouchSpreadMultiplier < 1.f);
	TestTrue(TEXT("Crouched head stays under the crouched capsule top"), Defaults->GetCrouchedHeadHeight() < Defaults->CrouchedHalfHeight * 2.f);
	TestEqual(TEXT("Standing spread is untouched"), Defaults->GetSpreadMultiplier(), 1.f);
	return true;
}

#endif
