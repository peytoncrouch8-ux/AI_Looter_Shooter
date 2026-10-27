#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Menus/MainMenuWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

// The main menu's look (the user, 2026-10-27: "very AI generated and needs some love"): the menu keeps to the left 40% of
// a 16:9 screen (the planet has the rest), the title is the game's, the main buttons are bare words that light up under
// the focus and sound heavier, and the menu arrives in motion (held under the startup cards until they're done).

namespace
{
	struct FMainMenuLookMouseAfter
	{
		FMainMenuLookMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
		~FMainMenuLookMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMainMenuLookTest, "Looter.UI.MainMenu.Look",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMainMenuLookTest::RunTest(const FString& Parameters)
{
	FMainMenuLookMouseAfter Mouse;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UMainMenuWidget* Menu = CreateWidget<UMainMenuWidget>(TestLevel.GetTestWorld(), UMainMenuWidget::StaticClass());
	if (!TestNotNull(TEXT("The main menu"), Menu))
	{
		return false;
	}
	const TSharedRef<SWidget> MenuSlate = Menu->TakeWidget();

	TestEqual(TEXT("The menu's side is 40% of a 16:9 screen (the planet has the right 60%)"), UMainMenuWidget::ColumnWidth, 1920.f * 0.4f);
	TestTrue(TEXT("The title is the game's"), Menu->GetTitleText()
		&& Menu->GetTitleText()->GetText().ToString() == UTransitionScreenSubsystem::GameTitle().ToString());

	// The main buttons: bare words (no boxes), pressed with the menu's own heavier sound, still the four in order.
	for (const TCHAR* Action : { TEXT("SinglePlayer"), TEXT("Multiplayer"), TEXT("Settings"), TEXT("Quit") })
	{
		const ULooterButton* Button = Menu->GetMainButton(Action);
		TestTrue(FString::Printf(TEXT("%s: a bare button with the menu's press"), Action), Button
			&& Button->GetKind() == LooterUI::EButtonKind::Bare && Button->ClickCue == FName(TEXT("UI.Menu.Select")));
	}

	// Held under the startup cards it waits, unseen; let go, it arrives in its own time.
	Menu->HoldReveal();
	Menu->AdvanceLook(5.f);
	TestFalse(TEXT("Held: it hasn't arrived"), Menu->IsRevealed());
	Menu->BeginReveal();
	Menu->AdvanceLook(0.2f);
	TestFalse(TEXT("Arriving: not at once"), Menu->IsRevealed());
	Menu->AdvanceLook(3.f);
	TestTrue(TEXT("...then there"), Menu->IsRevealed());

	// Under a controller the focus lights its button (tick, wash, orange words), eased in; the rest stay dark.
	LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Gamepad);
	Menu->AdvanceNav(0.f);
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		Menu->AdvanceLook(1.f / 60.f);
	}
	TestTrue(TEXT("The focus lights Single Player"), Menu->GetButtonLit(TEXT("SinglePlayer")) > 0.99f);
	TestTrue(TEXT("...not Settings"), Menu->GetButtonLit(TEXT("Settings")) < 0.01f);
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Down);
	Menu->AdvanceLook(1.f / 60.f);
	TestTrue(TEXT("Down: Settings starts to light (eased, not snapped)"), Menu->GetButtonLit(TEXT("Settings")) > 0.f
		&& Menu->GetButtonLit(TEXT("Settings")) < 1.f);
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		Menu->AdvanceLook(1.f / 60.f);
	}
	TestTrue(TEXT("...lit"), Menu->GetButtonLit(TEXT("Settings")) > 0.99f);
	TestTrue(TEXT("...Single Player dark again"), Menu->GetButtonLit(TEXT("SinglePlayer")) < 0.01f);
	TestTrue(TEXT("Multiplayer (disabled) never lights"), Menu->GetButtonLit(TEXT("Multiplayer")) == 0.f);

	// Back to the mouse with nothing under the pointer: everything goes dark.
	LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse);
	Menu->AdvanceNav(0.f);
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		Menu->AdvanceLook(1.f / 60.f);
	}
	TestTrue(TEXT("Under the mouse, unpointed: dark"), Menu->GetButtonLit(TEXT("Settings")) < 0.01f);
	return true;
}

#endif
