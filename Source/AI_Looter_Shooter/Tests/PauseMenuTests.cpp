#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HUD/LooterHUD.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

// The pause menu's list (the user, 2026-10-29 night: "When I hit ESC in the game, it should pop up a menu that gives me
// options to go to different menus. For example, settings. Hitting ESC shouldn't take me right to settings. Save and quit
// should be at the bottom of these options and Resume Game at the top. Remove save and quit from the settings menu at the
// bottom."): its rows in order, Resume Game first and Save & Quit last; keys and a controller through it as through the
// settings; backing out of what it opens comes back to it, and only the list itself goes back to the game; the settings
// over the game say Back and have no Save & Quit any more.

using namespace MenuNav;

namespace
{
	/** Puts the menus back under the mouse when a test ends, however it ends. */
	struct FPauseHubMouseAfter
	{
		FPauseHubMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
		~FPauseHubMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
	};

	/** One key down and up, as a press. */
	template <typename TMenu>
	void PauseHubTap(TMenu& Menu, const FKey& Key)
	{
		Menu.HandleNavKey(Key);
		Menu.HandleNavKeyUp(Key);
		Menu.AdvanceNav(0.f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPauseHubTest, "Looter.UI.PauseHub",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPauseHubTest::RunTest(const FString& Parameters)
{
	FPauseHubMouseAfter Mouse;

	// The rows, in the user's order: Resume Game at the top, Save & Quit at the bottom, the screens between.
	const TConstArrayView<EPauseMenuItem> Order = UPauseMenuWidget::ItemOrder();
	if (!TestEqual(TEXT("One row for each choice"), Order.Num(), static_cast<int32>(EPauseMenuItem::Count)))
	{
		return false;
	}
	auto IndexOf = [&Order](EPauseMenuItem Item) { return Order.IndexOfByKey(Item); };
	TestTrue(TEXT("Resume Game is the top row"), Order[0] == EPauseMenuItem::Resume);
	TestTrue(TEXT("Save & Quit is the bottom row"), Order.Last() == EPauseMenuItem::SaveQuit);
	TestTrue(TEXT("Settings just above it"), Order[Order.Num() - 2] == EPauseMenuItem::Settings);
	TestTrue(TEXT("Inventory, Missions, Bestiary and Embers between, in that order"),
		IndexOf(EPauseMenuItem::Inventory) == 1 && IndexOf(EPauseMenuItem::Inventory) < IndexOf(EPauseMenuItem::Missions)
		&& IndexOf(EPauseMenuItem::Missions) < IndexOf(EPauseMenuItem::Bestiary) && IndexOf(EPauseMenuItem::Bestiary) < IndexOf(EPauseMenuItem::Embers)
		&& IndexOf(EPauseMenuItem::Embers) < IndexOf(EPauseMenuItem::Settings) && IndexOf(EPauseMenuItem::Map) != INDEX_NONE);
	TestEqual(TEXT("The top row's words"), UPauseMenuWidget::ItemWord(EPauseMenuItem::Resume, false), FString(TEXT("RESUME GAME")));
	TestEqual(TEXT("The bottom row's"), UPauseMenuWidget::ItemWord(EPauseMenuItem::SaveQuit, false), FString(TEXT("SAVE & QUIT")));
	TestEqual(TEXT("The bestiary's row before Sexton's book is given"), UPauseMenuWidget::ItemWord(EPauseMenuItem::Bestiary, false), FString(TEXT("BESTIARY")));
	TestEqual(TEXT("...and after, as its tab"), UPauseMenuWidget::ItemWord(EPauseMenuItem::Bestiary, true), FString(TEXT("LEDGER")));

	// Where backing out goes: what the list opened comes back to the list; the list goes back to the game.
	TestTrue(TEXT("Esc or Back from the settings: the list"), ALooterHUD::PauseBackFrom(EPauseScreen::Settings) == EPauseScreen::Hub);
	TestTrue(TEXT("Closing a page the list opened: the list"), ALooterHUD::PauseBackFrom(EPauseScreen::Page) == EPauseScreen::Hub);
	TestTrue(TEXT("Esc on the list: the game"), ALooterHUD::PauseBackFrom(EPauseScreen::Hub) == EPauseScreen::None);
	TestTrue(TEXT("The game stays paused under all of them"), ALooterHUD::PausesGame(true, false) && ALooterHUD::PausesGame(true, true));
	EInventoryPage Page = EInventoryPage::Map;
	TestTrue(TEXT("Inventory opens the loadout"), ALooterHUD::PausePageOf(EPauseMenuItem::Inventory, Page) && Page == EInventoryPage::Loadout);
	TestTrue(TEXT("Missions its page"), ALooterHUD::PausePageOf(EPauseMenuItem::Missions, Page) && Page == EInventoryPage::Missions);
	TestTrue(TEXT("Map its page"), ALooterHUD::PausePageOf(EPauseMenuItem::Map, Page) && Page == EInventoryPage::Map);
	TestTrue(TEXT("Bestiary its page"), ALooterHUD::PausePageOf(EPauseMenuItem::Bestiary, Page) && Page == EInventoryPage::Bestiary);
	TestTrue(TEXT("Embers its page"), ALooterHUD::PausePageOf(EPauseMenuItem::Embers, Page) && Page == EInventoryPage::Embers);
	TestFalse(TEXT("Resume, Settings and Save & Quit are no pages"), ALooterHUD::PausePageOf(EPauseMenuItem::Resume, Page)
		|| ALooterHUD::PausePageOf(EPauseMenuItem::Settings, Page) || ALooterHUD::PausePageOf(EPauseMenuItem::SaveQuit, Page));

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UPauseMenuWidget* Hub = CreateWidget<UPauseMenuWidget>(TestLevel.GetTestWorld(), UPauseMenuWidget::StaticClass());
	if (!TestNotNull(TEXT("The list"), Hub))
	{
		return false;
	}
	// Its Slate widget held for the test, as a viewport holds it in play.
	const TSharedRef<SWidget> HubSlate = Hub->TakeWidget();
	TArray<EPauseMenuItem> Picked;
	Hub->OnChoice.BindLambda([&Picked](EPauseMenuItem Item) { Picked.Add(Item); });
	Hub->Open();
	Hub->AdvanceNav(0.f);
	for (const EPauseMenuItem Item : Order)
	{
		TestNotNull(*FString::Printf(TEXT("A button for %s"), *UPauseMenuWidget::ItemWord(Item, false)), Hub->GetItemButton(Item));
	}

	// Opened under the mouse: on Resume Game, unseen; the prompts name the keyboard's keys.
	TestTrue(TEXT("It opens on Resume Game"), Hub->GetFocusedItem() == EPauseMenuItem::Resume);
	TestTrue(TEXT("...the row marked"), Hub->GetMarkedItem() == EPauseMenuItem::Resume);
	TestEqual(TEXT("...its words"), Hub->GetShownWord(EPauseMenuItem::Resume), FString(TEXT("RESUME GAME")));
	TestFalse(TEXT("...no focus look under the mouse"), Hub->IsFocusLookShown());
	TestEqual(TEXT("Keyboard prompts"), PromptsText(Hub->GetPrompts()), PromptsText({ MakePrompt(EPromptKey::Select, false, TEXT("Select")),
		MakePrompt(EPromptKey::Back, false, TEXT("Resume")) }));

	// The first D-pad press only shows where the focus is; then down the list, every row in order, and round the ends.
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("The first press: still on Resume Game, now lit"), Hub->GetFocusedItem() == EPauseMenuItem::Resume && Hub->IsFocusLookShown());
	TestTrue(TEXT("...the controller's prompts"), PromptsText(Hub->GetPrompts()).Contains(TEXT("B Resume")));
	for (int32 Index = 1; Index < Order.Num(); ++Index)
	{
		PauseHubTap(*Hub, EKeys::Gamepad_DPad_Down);
		TestTrue(*FString::Printf(TEXT("Down: %s"), *UPauseMenuWidget::ItemWord(Order[Index], false)), Hub->GetFocusedItem() == Order[Index]);
	}
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down from Save & Quit: round to Resume Game"), Hub->GetFocusedItem() == EPauseMenuItem::Resume);
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Up);
	TestTrue(TEXT("Up from Resume Game: round to Save & Quit"), Hub->GetFocusedItem() == EPauseMenuItem::SaveQuit);
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Left);
	TestTrue(TEXT("Left and right do nothing"), Hub->GetFocusedItem() == EPauseMenuItem::SaveQuit);
	TestEqual(TEXT("Moving picks nothing"), Picked.Num(), 0);

	// A on Settings picks it (the HUD opens the settings over the game).
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Up);
	TestTrue(TEXT("Up: Settings"), Hub->GetFocusedItem() == EPauseMenuItem::Settings);
	PauseHubTap(*Hub, EKeys::Gamepad_FaceButton_Bottom);
	TestTrue(TEXT("A on Settings picks it"), Picked.Num() == 1 && Picked.Last() == EPauseMenuItem::Settings);

	// Back from the settings the HUD opens the list again on the row it left from; Esc or B there is Resume, and the held
	// Esc's repeat (the one that backed out of the settings) resumes nothing.
	Hub->Open(EPauseMenuItem::Settings);
	TestTrue(TEXT("Back from the settings: on Settings again"), Hub->GetFocusedItem() == EPauseMenuItem::Settings);
	TestTrue(TEXT("A held Esc's repeat is taken..."), Hub->HandleNavKey(EKeys::Escape, true));
	TestEqual(TEXT("...and resumes nothing"), Picked.Num(), 1);
	TestTrue(TEXT("Esc on the list: Resume"), Hub->HandleNavKey(EKeys::Escape) && Picked.Num() == 2 && Picked.Last() == EPauseMenuItem::Resume);
	TestTrue(TEXT("B on the list: Resume"), Hub->HandleNavKey(EKeys::Gamepad_FaceButton_Right) && Picked.Num() == 3 && Picked.Last() == EPauseMenuItem::Resume);

	// Save & Quit, the last row: picked once, and nothing is taken after it while the level leaves.
	Hub->Open();
	PauseHubTap(*Hub, EKeys::Up);
	TestTrue(TEXT("Up from the top: Save & Quit"), Hub->GetFocusedItem() == EPauseMenuItem::SaveQuit);
	PauseHubTap(*Hub, EKeys::Enter);
	TestTrue(TEXT("Enter on it: Save & Quit"), Picked.Num() == 4 && Picked.Last() == EPauseMenuItem::SaveQuit);
	Hub->HandleNavKey(EKeys::Escape);
	TestEqual(TEXT("...and Esc after it does nothing more"), Picked.Num(), 4);

	// The settings over the game: their Back (and Esc) goes back to the list (the HUD binds OnClose to that), they say Back
	// rather than Resume, and there is no Save & Quit in them on any page.
	LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse);
	USettingsMenuWidget* Settings = CreateWidget<USettingsMenuWidget>(TestLevel.GetTestWorld(), USettingsMenuWidget::StaticClass());
	if (!TestNotNull(TEXT("The settings"), Settings))
	{
		return false;
	}
	const TSharedRef<SWidget> SettingsSlate = Settings->TakeWidget();
	int32 Closed = 0;
	Settings->OnClose.BindLambda([&Closed]() { ++Closed; });
	Settings->Open(ESettingsMenuMode::Pause);
	Settings->AdvanceNav(0.f);
	const FString SettingsPrompts = PromptsText(Settings->GetPrompts());
	TestTrue(TEXT("Over the game the settings say Esc Back"), SettingsPrompts.Contains(TEXT("Esc Back")) && !SettingsPrompts.Contains(TEXT("Resume")));
	TestTrue(TEXT("Esc in the settings is their Back"), Settings->HandleNavKey(EKeys::Escape) && Closed == 1);
	Settings->Open(ESettingsMenuMode::Pause);
	bool bSawSaveQuit = false;
	for (int32 SettingsPage = 0; SettingsPage < SettingsMenu::NumPages; ++SettingsPage)
	{
		Settings->SelectPage(SettingsPage);
		for (int32 Step = 0; Step < 80; ++Step)
		{
			PauseHubTap(*Settings, EKeys::Gamepad_DPad_Down);
			const ULooterButton* Button = Cast<ULooterButton>(Settings->GetFocusedWidget());
			bSawSaveQuit |= Button && Button->Action == FName(TEXT("SaveQuit"));
		}
	}
	TestFalse(TEXT("No Save & Quit in the settings (it's the list's)"), bSawSaveQuit);
	TestEqual(TEXT("...and walking them closed nothing"), Closed, 1);
	return true;
}

#endif
