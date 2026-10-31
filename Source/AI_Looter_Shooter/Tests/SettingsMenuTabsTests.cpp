#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Menus/MenuNav.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "Settings/AudioSettingsSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/Slider.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

// The settings menu's tabs and descriptions (the user, 2026-10-27: tabbed pages, and what each setting does shown as it's
// pointed at): every row has words of one or two sentences, the bumpers turn the tabs round the ends, the keys move only
// through the page on show, a click on a tab turns to it, and the description column follows the focus.

namespace
{
	/** Puts the menus back under the mouse when a test ends, however it ends. */
	struct FSettingsTabsMouseAfter
	{
		FSettingsTabsMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
		~FSettingsTabsMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
	};

	FName SettingsTabsActionOf(const UWidget* Widget)
	{
		const ULooterButton* Button = Cast<ULooterButton>(Widget);
		return Button ? Button->Action : NAME_None;
	}

	/** Sentences in Text: each ends at a full stop, question or exclamation mark followed by a space or the end. */
	int32 SettingsTabsSentences(const FString& Text)
	{
		int32 Count = 0;
		for (int32 Index = 0; Index < Text.Len(); ++Index)
		{
			const TCHAR Char = Text[Index];
			if ((Char == TEXT('.') || Char == TEXT('?') || Char == TEXT('!')) && (Index + 1 == Text.Len() || FChar::IsWhitespace(Text[Index + 1])))
			{
				++Count;
			}
		}
		return Count;
	}

	/** One key down and up, as a press. */
	void SettingsTabsTap(USettingsMenuWidget& Menu, const FKey& Key)
	{
		Menu.HandleNavKey(Key);
		Menu.HandleNavKeyUp(Key);
		Menu.AdvanceNav(0.f);
	}

	int32 PageIndex(SettingsMenu::ESettingsPage Page)
	{
		return static_cast<int32>(Page);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsMenuHelpWordsTest, "Looter.UI.SettingsMenu.Help",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSettingsMenuHelpWordsTest::RunTest(const FString& Parameters)
{
	using namespace SettingsMenu;
	auto CheckWords = [this](const FString& What, const FSettingHelp& Help)
	{
		TestFalse(FString::Printf(TEXT("%s has a name"), *What), Help.Title.IsEmpty());
		const int32 Sentences = SettingsTabsSentences(Help.Body);
		TestTrue(FString::Printf(TEXT("%s says what it does in one or two sentences (%d)"), *What, Sentences), Sentences >= 1 && Sentences <= 2);
	};

	for (int32 Row = 0; Row < static_cast<int32>(ESettingRow::Count); ++Row)
	{
		CheckWords(FString::Printf(TEXT("Row %d"), Row), RowHelp(static_cast<ESettingRow>(Row)));
	}
	for (int32 Page = 0; Page < NumPages; ++Page)
	{
		TestFalse(FString::Printf(TEXT("Page %d has a tab name"), Page), PageName(Page).IsEmpty());
		CheckWords(FString::Printf(TEXT("Page %d"), Page), PageHelp(Page));
	}
	for (const EGraphicsQuality Quality : Qualities)
	{
		TestFalse(FString::Printf(TEXT("%s says what it costs"), *UGraphicsSettingsSubsystem::QualityName(Quality)), QualityPresetHelp(Quality).IsEmpty());
	}
	for (const EAudioVolume Which : { EAudioVolume::Master, EAudioVolume::Effects, EAudioVolume::Interface, EAudioVolume::Music })
	{
		TestTrue(FString::Printf(TEXT("%s volume's row names it"), *UAudioSettingsSubsystem::VolumeName(Which)),
			RowHelp(VolumeRow(Which)).Title.Contains(UAudioSettingsSubsystem::VolumeName(Which)));
	}

	// Every binding UKeyBindingSubsystem lists (its Initialize, in order) has words of its own.
	TArray<FName> Bindings = { TEXT("MoveForward"), TEXT("MoveBackward"), TEXT("MoveLeft"), TEXT("MoveRight"), TEXT("Jump"), TEXT("Sprint"),
		TEXT("Crouch"), TEXT("Fire"), TEXT("Aim"), UKeyBindingSubsystem::MeleeBindingId(), UKeyBindingSubsystem::GrenadeBindingId(),
		UKeyBindingSubsystem::EmberBindingId(), UKeyBindingSubsystem::FlaskBindingId(), TEXT("Reload"), TEXT("NextWeapon"), TEXT("PreviousWeapon"), TEXT("DropWeapon"), TEXT("Interact"),
		TEXT("ToggleView"), TEXT("Inventory"), TEXT("InventoryAlt"), UKeyBindingSubsystem::MapBindingId() };
	for (int32 Slot = 0; Slot < UKeyBindingSubsystem::NumWeaponSlotKeys; ++Slot)
	{
		Bindings.Add(UKeyBindingSubsystem::WeaponSlotBindingId(Slot));
	}
	for (const FName Id : Bindings)
	{
		TestTrue(FString::Printf(TEXT("Key %s has words"), *Id.ToString()), HasKeyWords(Id));
		CheckWords(Id.ToString(), KeyHelp(Id, Id.ToString(), TEXT("W")));
	}
	TestTrue(TEXT("A key row names its default key"), KeyHelp(TEXT("Jump"), TEXT("Jump"), TEXT("Space Bar")).Detail.Contains(TEXT("Space Bar")));
	CheckWords(TEXT("A Hold / Toggle switch"), KeyModeHelp(TEXT("Sprint")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsMenuTabsTest, "Looter.UI.SettingsMenu.Tabs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSettingsMenuTabsTest::RunTest(const FString& Parameters)
{
	using namespace SettingsMenu;
	FSettingsTabsMouseAfter Mouse;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	USettingsMenuWidget* Menu = CreateWidget<USettingsMenuWidget>(TestLevel.GetTestWorld(), USettingsMenuWidget::StaticClass());
	if (!TestNotNull(TEXT("The menu"), Menu))
	{
		return false;
	}
	// Its Slate widget held for the test, as a viewport holds it in play.
	const TSharedRef<SWidget> MenuSlate = Menu->TakeWidget();
	Menu->Open(ESettingsMenuMode::MainMenu);
	Menu->AdvanceNav(0.f);

	// Opened: the Graphics tab, lit alone, and the column saying what the page is for.
	TestEqual(TEXT("Opens on Graphics"), Menu->GetPage(), PageIndex(ESettingsPage::Graphics));
	for (int32 Page = 0; Page < NumPages; ++Page)
	{
		ULooterButton* Tab = Menu->GetTabButton(Page);
		if (TestNotNull(FString::Printf(TEXT("Tab %d"), Page), Tab))
		{
			TestTrue(FString::Printf(TEXT("Tab %d lit only while its page shows"), Page), Tab->IsHighlighted() == (Page == 0));
		}
	}
	TestEqual(TEXT("Under the mouse, nothing pointed at: the page's own words"), Menu->GetShownHelpTitle(), PageName(0));

	// Keys: the focus and the column move together, and only through the page on show (then the footer's Back).
	SettingsTabsTap(*Menu, EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("The first press shows the focus on the quality presets"), SettingsTabsActionOf(Menu->GetFocusedWidget()) == ActionQuality);
	TestEqual(TEXT("...and the column describes them"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::Quality).Title);
	SettingsTabsTap(*Menu, EKeys::Gamepad_DPad_Down);
	TestEqual(TEXT("Down: motion blur, described"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::MotionBlur).Title);
	SettingsTabsTap(*Menu, EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down: the field of view"), Menu->GetFocusedWidget() == Menu->GetFieldOfViewSlider());
	TestEqual(TEXT("...described"), Menu->GetShownHelpText(), RowHelp(ESettingRow::FieldOfView).Body);
	SettingsTabsTap(*Menu, EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down past the page's last row: it stays, not the next page"), Menu->GetFocusedWidget() == Menu->GetFieldOfViewSlider());
	// Back is the reticle row under the title (UI round 3: where Resume is over the game), up from the first setting.
	for (int32 Step = 0; Step < 3; ++Step)
	{
		SettingsTabsTap(*Menu, EKeys::Gamepad_DPad_Up);
	}
	TestTrue(TEXT("Up past the first setting: Back, at the top"), SettingsTabsActionOf(Menu->GetFocusedWidget()) == ActionClose);
	TestEqual(TEXT("...described"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::Back).Title);

	// The bumpers turn the tabs in order, round the ends, the focus to each page's first setting.
	SettingsTabsTap(*Menu, EKeys::Gamepad_RightShoulder);
	TestEqual(TEXT("RB: Audio"), Menu->GetPage(), PageIndex(ESettingsPage::Audio));
	TestTrue(TEXT("...on its first volume slider"), Cast<USlider>(Menu->GetFocusedWidget()) != nullptr);
	TestEqual(TEXT("...described"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::MasterVolume).Title);
	TestTrue(TEXT("...its tab lit, Graphics' out"), Menu->GetTabButton(1)->IsHighlighted() && !Menu->GetTabButton(0)->IsHighlighted());
	SettingsTabsTap(*Menu, EKeys::Gamepad_RightShoulder);
	TestEqual(TEXT("RB: Interface"), Menu->GetPage(), PageIndex(ESettingsPage::Interface));
	SettingsTabsTap(*Menu, EKeys::Gamepad_RightShoulder);
	TestEqual(TEXT("RB: Controls"), Menu->GetPage(), PageIndex(ESettingsPage::Controls));
	TestEqual(TEXT("...its first row, the look sensitivity"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::LookSensitivity).Title);
	for (int32 Step = 0; Step < 3; ++Step)
	{
		SettingsTabsTap(*Menu, EKeys::Gamepad_DPad_Down);
	}
	TestTrue(TEXT("Three down: Reset all keys, after the page's sliders"), SettingsTabsActionOf(Menu->GetFocusedWidget()) == ActionResetAll);
	TestEqual(TEXT("...described"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::ResetAllKeys).Title);
	SettingsTabsTap(*Menu, EKeys::Gamepad_RightShoulder);
	TestEqual(TEXT("RB from the last tab: round to Graphics"), Menu->GetPage(), PageIndex(ESettingsPage::Graphics));
	SettingsTabsTap(*Menu, EKeys::Gamepad_LeftShoulder);
	TestEqual(TEXT("LB from the first tab: round to Controls"), Menu->GetPage(), PageIndex(ESettingsPage::Controls));
	SettingsTabsTap(*Menu, EKeys::PageUp);
	TestEqual(TEXT("Page Up: the tab before (Interface)"), Menu->GetPage(), PageIndex(ESettingsPage::Interface));
	SettingsTabsTap(*Menu, EKeys::PageDown);
	TestEqual(TEXT("Page Down: the tab after (Controls)"), Menu->GetPage(), PageIndex(ESettingsPage::Controls));
	TestTrue(TEXT("The prompts name the tabs"), MenuNav::PromptsText(Menu->GetPrompts()).Contains(TEXT("Tabs")));

	// A click on a tab turns to it, as the mouse does.
	LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse);
	Menu->AdvanceNav(0.f);
	TestTrue(TEXT("The Audio tab clicks"), Menu->GetTabButton(PageIndex(ESettingsPage::Audio))->Press());
	TestEqual(TEXT("...and the Audio page shows"), Menu->GetPage(), PageIndex(ESettingsPage::Audio));
	TestEqual(TEXT("...with its own words"), Menu->GetShownHelpTitle(), PageName(PageIndex(ESettingsPage::Audio)));

	// Over the game (from the pause menu's list): Back to the list first, described as that; opened again it's back on
	// Graphics.
	Menu->Open(ESettingsMenuMode::Pause);
	TestEqual(TEXT("Opened over the game: Graphics"), Menu->GetPage(), PageIndex(ESettingsPage::Graphics));
	SettingsTabsTap(*Menu, EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("The focus on the top row (Back)"), Menu->GetFocusedWidget() == Menu->GetResumeButton());
	TestEqual(TEXT("...described"), Menu->GetShownHelpTitle(), RowHelp(ESettingRow::BackToPause).Title);
	TestEqual(TEXT("...as going back to the pause menu"), Menu->GetShownHelpText(), RowHelp(ESettingRow::BackToPause).Body);
	return true;
}

#endif
