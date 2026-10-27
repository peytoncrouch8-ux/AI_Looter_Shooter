#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Settings/GraphicsSettingsSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLine.h"
#include "UI/HUD/HudCaptionWidget.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/World/CreatureBarkActor.h"
#include "Blueprint/UserWidget.h"
#include "Components/Slider.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

// The Subtitles option (the settings menu's Interface section): its row and its save, and what it hides. Off, the words of
// spoken lines are not drawn, by the caption widget or by a creature's floating words, while the lines play on their clock
// all the same (what waits on a conversation still waits).

namespace
{
	FStoryLine SubtitleTestLine(const TCHAR* Speaker, const TCHAR* Words, float Seconds)
	{
		return FStoryLine::Make(FText::FromString(Speaker), FText::FromString(Words), Seconds);
	}

	FName SubtitleTestActionOf(const UWidget* Widget)
	{
		const ULooterButton* Button = Cast<ULooterButton>(Widget);
		return Button ? Button->Action : NAME_None;
	}

	/** Puts the menus back under the mouse when a test ends, however it ends (the device is the game's own, shared by every menu). */
	struct FSubtitleTestMouseAfter
	{
		FSubtitleTestMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
		~FSubtitleTestMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSubtitlesSavedTest, "Looter.Settings.Subtitles.Saved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSubtitlesSavedTest::RunTest(const FString& Parameters)
{
	using Graphics = UGraphicsSettingsSubsystem;

	// On for a fresh install, for a save from before the option existed (its field then keeps the class default), and
	// for no save at all.
	TestTrue(TEXT("A fresh save starts with subtitles on"), GetDefault<ULooterGraphicsSave>()->bShowSubtitles);
	TestTrue(TEXT("...so does no save object at all"), Graphics::SubtitlesOf(nullptr));
	TestTrue(TEXT("...and the default save"), Graphics::SubtitlesOf(GetDefault<ULooterGraphicsSave>()));

	// Written and read back as the subsystem does, through a slot of the test's own so the player's settings are never touched.
	const FString Slot = TEXT("GraphicsSettings_SubtitlesAutomationTest");
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	ULooterGraphicsSave* Written = NewObject<ULooterGraphicsSave>();
	Written->bShowSubtitles = false;
	Written->MinimapScale = 1.25f;
	if (!TestTrue(TEXT("Saved with subtitles off"), UGameplayStatics::SaveGameToSlot(Written, Slot, 0)))
	{
		return false;
	}
	const ULooterGraphicsSave* ReadBack = Cast<ULooterGraphicsSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!TestTrue(TEXT("Read back a new copy"), ReadBack && ReadBack != Written))
	{
		UGameplayStatics::DeleteGameInSlot(Slot, 0);
		return false;
	}
	TestFalse(TEXT("...with subtitles off"), Graphics::SubtitlesOf(ReadBack));
	TestEqual(TEXT("...and the other settings as they were"), ReadBack->MinimapScale, 1.25f);

	Written->bShowSubtitles = true;
	UGameplayStatics::SaveGameToSlot(Written, Slot, 0);
	const ULooterGraphicsSave* TurnedOn = Cast<ULooterGraphicsSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	TestTrue(TEXT("Saved on again: read back on"), TurnedOn && Graphics::SubtitlesOf(TurnedOn));
	UGameplayStatics::DeleteGameInSlot(Slot, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSubtitlesRowTest, "Looter.Settings.Subtitles.Row",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSubtitlesRowTest::RunTest(const FString& Parameters)
{
	FSubtitleTestMouseAfter Mouse;
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

	// The first press from the mouse only shows the focus; then down the list, with a key let go after each (no repeat).
	// The keyboard and a gamepad drive it as they do every other row.
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("The list starts at the quality presets"), SubtitleTestActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionQuality);
	// Subtitles are on the Interface tab, two turns of RB from Graphics.
	Menu->HandleNavKey(EKeys::Gamepad_RightShoulder);
	Menu->HandleNavKey(EKeys::Gamepad_RightShoulder);
	TestEqual(TEXT("RB twice: the Interface tab"), Menu->GetPage(), static_cast<int32>(SettingsMenu::ESettingsPage::Interface));
	FName Before = NAME_None;
	for (int32 Step = 0; Step < 40 && SubtitleTestActionOf(Menu->GetFocusedWidget()) != SettingsMenu::ActionSubtitlesOn; ++Step)
	{
		Before = SubtitleTestActionOf(Menu->GetFocusedWidget());
		Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
		Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Down);
	}
	if (!TestTrue(TEXT("The list reaches a Subtitles switch (On, Off)"), SubtitleTestActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionSubtitlesOn))
	{
		return false;
	}
	TestTrue(TEXT("...right after the Interface section's control hints"), Before == SettingsMenu::ActionHintsOn);

	// It is the last stop of the Interface tab: the next one is the footer's Back (the tab's last row), and back up lands on it.
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down: the footer's Back"), SubtitleTestActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionClose);
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Up);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Up);
	TestTrue(TEXT("Up: back on the subtitles"), SubtitleTestActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionSubtitlesOn);

	// Right picks Off (with no player to save for, the menu simply takes the press); the stop stays the switch's.
	TestTrue(TEXT("Right is the menu's"), Menu->HandleNavKey(EKeys::Gamepad_DPad_Right));
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Right);
	const FName Here = SubtitleTestActionOf(Menu->GetFocusedWidget());
	TestTrue(TEXT("...still on the subtitles switch"), Here == SettingsMenu::ActionSubtitlesOn || Here == SettingsMenu::ActionSubtitlesOff);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSubtitlesHideCaptionsTest, "Looter.Settings.Subtitles.HidesCaptions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSubtitlesHideCaptionsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	UHudCaptionWidget* Widget = CreateWidget<UHudCaptionWidget>(World, UHudCaptionWidget::StaticClass());
	if (!TestNotNull(TEXT("The level has captions"), Captions) || !TestNotNull(TEXT("and the caption widget"), Widget))
	{
		return false;
	}
	// Its Slate widget held for the test, as a viewport holds it in play. No player here, so the captions' own switch is the
	// one that counts (in play the widget sets it from the player's setting each frame).
	const TSharedRef<SWidget> WidgetSlate = Widget->TakeWidget();

	// On by default: a line's words go up.
	TestTrue(TEXT("Subtitles start on"), Captions->AreSubtitlesShown());
	const int32 Said = Captions->Play({ SubtitleTestLine(TEXT("Deacon"), TEXT("Home, kid."), 4.f) });
	Captions->Update(0.5f);
	Widget->Advance(0.f);
	TestTrue(TEXT("On: the line is up"), Widget->AreWordsShown());
	TestEqual(TEXT("...with its words"), Widget->GetShownWords().ToString(), FString(TEXT("Home, kid.")));

	// Off while it shows: the words go at once, and the line goes on being said.
	Captions->SetSubtitlesShown(false);
	Widget->Advance(0.f);
	TestFalse(TEXT("Off: the words are gone at once"), Widget->AreWordsShown());
	TestTrue(TEXT("...the line is still being said (its conversation waits on it)"), Captions->IsPlaying(Said) && Captions->GetCurrent() != nullptr);
	Captions->Update(1.f);
	Widget->Advance(1.f);
	TestFalse(TEXT("Off a second on: still no words"), Widget->AreWordsShown());
	TestTrue(TEXT("...and the line's clock ran"), Captions->IsPlaying(Said) && Captions->GetQueue().GetElapsed() > 1.4f);

	// On again with the line still being said: it comes back from where it is.
	Captions->SetSubtitlesShown(true);
	Widget->Advance(0.f);
	TestTrue(TEXT("On again mid-line: the words are back"), Widget->AreWordsShown());
	TestEqual(TEXT("...the same words"), Widget->GetShownWords().ToString(), FString(TEXT("Home, kid.")));

	// Off for a whole line: nothing is drawn or even set, and it ends on its clock like any other.
	Captions->SetSubtitlesShown(false);
	Captions->Update(5.f);
	Widget->Advance(0.f);
	TestFalse(TEXT("The line ends while off: it is said"), Captions->IsPlaying(Said));
	const int32 Whispered = Captions->Play({ SubtitleTestLine(TEXT("Hob"), TEXT("Nothing to read."), 2.f) });
	Captions->Update(0.5f);
	Widget->Advance(0.f);
	TestFalse(TEXT("A new line while off: no words"), Widget->AreWordsShown());
	TestNotEqual(TEXT("...none even put up behind the scenes"), Widget->GetShownWords().ToString(), FString(TEXT("Nothing to read.")));
	TestTrue(TEXT("...but it is being said"), Captions->IsPlaying(Whispered));
	Captions->SetSubtitlesShown(true);
	Widget->Advance(0.f);
	TestTrue(TEXT("On: that line comes up"), Widget->AreWordsShown());
	TestEqual(TEXT("...with its own words"), Widget->GetShownWords().ToString(), FString(TEXT("Nothing to read.")));
	Captions->Update(5.f);
	Widget->Advance(0.f);
	TestFalse(TEXT("Said, and gone"), Widget->AreWordsShown() || Captions->IsPlaying(Whispered));

	// A line with no speaker is a notice or a remark (the lift's bell, a sign, a time card), not a subtitle: it shows off or on.
	Captions->SetSubtitlesShown(false);
	const int32 Notice = Captions->Play({ SubtitleTestLine(TEXT(""), TEXT("The bell is dead."), 3.f) });
	Captions->Update(0.5f);
	Widget->Advance(0.f);
	TestTrue(TEXT("Off: a line with no speaker still shows"), Widget->AreWordsShown() && Captions->GetCurrentShown() != nullptr);
	TestEqual(TEXT("...with its words"), Widget->GetShownWords().ToString(), FString(TEXT("The bell is dead.")));
	TestTrue(TEXT("...and it is being said"), Captions->IsPlaying(Notice));
	Captions->Update(5.f);
	Widget->Advance(0.f);
	TestFalse(TEXT("The notice ends on its clock"), Widget->AreWordsShown() || Captions->IsPlaying(Notice));

	// Someone speaking, same switch: hidden; the shown-line query agrees (the control hints use it).
	Captions->Play({ SubtitleTestLine(TEXT("Hob"), TEXT("Caw."), 3.f) });
	Captions->Update(0.5f);
	Widget->Advance(0.f);
	TestTrue(TEXT("Off: someone speaking is still hidden"), !Widget->AreWordsShown() && Captions->GetCurrentShown() == nullptr && Captions->GetCurrent() != nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSubtitlesHideBarksTest, "Looter.Settings.Subtitles.HidesBarks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSubtitlesHideBarksTest::RunTest(const FString& Parameters)
{
	// A creature's words over its head are a subtitle of its murmur: off, they are not drawn, but the bark goes on (its
	// clock, its place on the board).
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACreatureBarkActor* Words = World->SpawnActor<ACreatureBarkActor>(ACreatureBarkActor::StaticClass(), FTransform::Identity, Params);
	if (!TestNotNull(TEXT("The level has captions"), Captions) || !TestNotNull(TEXT("and a bark actor"), Words))
	{
		return false;
	}

	Words->Show(nullptr, FText::FromString(TEXT("Mine. All mine.")), 4.f, false);
	TestTrue(TEXT("On: the bark's words are up"), Words->IsShowing() && Words->AreWordsDrawn());

	Captions->SetSubtitlesShown(false);
	Words->Advance(0.5f);
	TestTrue(TEXT("Off: still being said..."), Words->IsShowing());
	TestFalse(TEXT("...but its words are not drawn"), Words->AreWordsDrawn());

	Captions->SetSubtitlesShown(true);
	Words->Advance(0.5f);
	TestTrue(TEXT("On again mid-bark: the words are back"), Words->IsShowing() && Words->AreWordsDrawn());

	// A bark that starts while off is said all the same, and its words come up if the switch does before it ends.
	Words->Hide();
	TestFalse(TEXT("Gone: nothing drawn"), Words->IsShowing() || Words->AreWordsDrawn());
	Captions->SetSubtitlesShown(false);
	Words->Show(nullptr, FText::FromString(TEXT("Who's there?")), 4.f, true);
	TestTrue(TEXT("A bark started while off is showing..."), Words->IsShowing());
	TestFalse(TEXT("...with no words drawn"), Words->AreWordsDrawn());
	Captions->SetSubtitlesShown(true);
	Words->Advance(0.5f);
	TestTrue(TEXT("On: the words appear"), Words->AreWordsDrawn());
	Words->Advance(10.f);
	TestFalse(TEXT("Said, and gone"), Words->IsShowing() || Words->AreWordsDrawn());
	return true;
}

#endif
