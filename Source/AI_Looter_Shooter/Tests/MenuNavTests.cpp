#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/Menus/MainMenuWidget.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The menus for a controller and the keyboard: the plain rules (keys, the stick, held keys, prompts), the shared focus
// (FMenuNavigator over real buttons, a slider and a switch), and the settings menu and the main menu driven by keys as a
// player would: focus order, A, B, LB / RB, a slider stepped and held, the previews, and the focus look only under keys
// or a gamepad. Every test leaves the menus under the mouse, as the game starts.

using namespace MenuNav;

namespace
{
	/** Puts the menus back under the mouse when a test ends, however it ends. */
	struct FMouseAfter
	{
		FMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
		~FMouseAfter() { LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse); }
	};

	ULooterButton* MakeTestButton(UWidgetTree* Tree, const TCHAR* Label, FName Action, int32 Index = 0)
	{
		ULooterButton* Button = Tree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		Button->Setup(Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), Action, Index, FText::FromString(Label));
		return Button;
	}

	FName ActionOf(const UWidget* Widget)
	{
		const ULooterButton* Button = Cast<ULooterButton>(Widget);
		return Button ? Button->Action : NAME_None;
	}

	/** Holds the menu's clock on for Seconds, a frame at a time. */
	template <typename TMenu>
	void HoldFor(TMenu& Menu, float Seconds)
	{
		constexpr float Frame = 1.f / 60.f;
		for (float Time = 0.f; Time < Seconds - 1e-4f; Time += Frame)
		{
			Menu.AdvanceNav(Frame);
		}
	}
}

// ---------------------------------------------------------------------------
// The plain rules
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuNavRulesTest, "Looter.UI.MenuNav.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMenuNavRulesTest::RunTest(const FString& Parameters)
{
	FMouseAfter Mouse;

	// The keys: arrows, W A S D and the D-pad move; Enter, Space, E and A pick; Esc and B back out; the bumpers turn sections.
	TestTrue(TEXT("Up: the arrow, W, the D-pad"), MoveOf(EKeys::Up) == EMove::Up && MoveOf(EKeys::W) == EMove::Up && MoveOf(EKeys::Gamepad_DPad_Up) == EMove::Up);
	TestTrue(TEXT("Left: the arrow, A, the D-pad"), MoveOf(EKeys::Left) == EMove::Left && MoveOf(EKeys::A) == EMove::Left && MoveOf(EKeys::Gamepad_DPad_Left) == EMove::Left);
	TestTrue(TEXT("The face buttons don't move"), MoveOf(EKeys::Gamepad_FaceButton_Bottom) == EMove::None);
	TestTrue(TEXT("A and Enter pick"), IsAccept(EKeys::Gamepad_FaceButton_Bottom) && IsAccept(EKeys::Enter) && !IsAccept(EKeys::Escape));
	TestTrue(TEXT("B and Esc back out"), IsBack(EKeys::Gamepad_FaceButton_Right) && IsBack(EKeys::Escape) && !IsBack(EKeys::Gamepad_FaceButton_Bottom));
	TestTrue(TEXT("Start is the menu button"), IsMenuButton(EKeys::Gamepad_Special_Right));
	TestEqual(TEXT("LB: the previous section"), SectionStepOf(EKeys::Gamepad_LeftShoulder), -1);
	TestEqual(TEXT("RB: the next"), SectionStepOf(EKeys::Gamepad_RightShoulder), 1);
	TestEqual(TEXT("Page Down too"), SectionStepOf(EKeys::PageDown), 1);
	TestTrue(TEXT("The stick's own direction keys are known (and left to its axes)"), IsStickKey(EKeys::Gamepad_LeftStick_Down) && !IsStickKey(EKeys::Down));

	// The stick as a D-pad: past half way it takes the larger axis; it holds until it drops under the let-go line.
	TestTrue(TEXT("Pushed up"), StickMove(FVector2D(0.f, 0.7f), EMove::None) == EMove::Up);
	TestTrue(TEXT("Pushed mostly right"), StickMove(FVector2D(0.8f, -0.3f), EMove::None) == EMove::Right);
	TestTrue(TEXT("Barely pushed: nothing"), StickMove(FVector2D(0.3f, 0.3f), EMove::None) == EMove::None);
	TestTrue(TEXT("Held up, easing back but past the let-go line: still up"), StickMove(FVector2D(0.f, 0.35f), EMove::Up) == EMove::Up);
	TestTrue(TEXT("...and let go under it"), StickMove(FVector2D(0.f, 0.2f), EMove::Up) == EMove::None);
	TestTrue(TEXT("A stick direction stands for its key"), StickKey(EMove::Down) == EKeys::Gamepad_LeftStick_Down);

	// Held keys: nothing more until the delay, then an even pace through a list...
	FRepeat Repeat;
	Repeat.Press(EKeys::Down, EMove::Down, false);
	TestEqual(TEXT("Not before the delay"), Repeat.Tick(RepeatDelay * 0.8f), 0);
	TestEqual(TEXT("The first repeat after it"), Repeat.Tick(RepeatDelay * 0.3f), 1);
	int32 Even = 0;
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		Even += Repeat.Tick(1.f / 60.f);
	}
	TestTrue(TEXT("An even pace: about nine a second"), Even >= 8 && Even <= 10);
	TestFalse(TEXT("Another key's let-go doesn't stop it"), Repeat.Release(EKeys::Up));
	TestTrue(TEXT("Its own does"), Repeat.Release(EKeys::Down) && !Repeat.IsHeld());
	TestEqual(TEXT("Stopped: no more"), Repeat.Tick(1.f), 0);

	// ...and a held slider speeds up, to many times the pace by the end of the run-up; a hitch never fires a burst.
	TestTrue(TEXT("A slider starts at the even pace"), FMath::IsNearlyEqual(FRepeat::IntervalAt(RepeatDelay, true), RepeatInterval));
	TestTrue(TEXT("...and races once held"), FMath::IsNearlyEqual(FRepeat::IntervalAt(RepeatDelay + AccelerateSeconds, true), FastestInterval));
	Repeat.Press(EKeys::Right, EMove::Right, true);
	TestTrue(TEXT("A long frame fires a few at most"), Repeat.Tick(5.f) <= 4);

	// The prompts' key names follow the device.
	TestEqual(TEXT("A on a gamepad"), PromptKeyName(EPromptKey::Select, true), FString(TEXT("A")));
	TestEqual(TEXT("Enter on the keyboard"), PromptKeyName(EPromptKey::Select, false), FString(TEXT("Enter")));
	TestEqual(TEXT("The bumpers"), PromptKeyName(EPromptKey::Sections, true), FString(TEXT("LB / RB")));
	TestEqual(TEXT("A bar as one line"), PromptsText({ MakePrompt(EPromptKey::Select, true, TEXT("Select")), MakePrompt(EPromptKey::Back, true, TEXT("Back")) }),
		FString(TEXT("A Select  B Back")));

	// A slider's step from where it is: an off-step value lands on the step that way, never skips one; the ends hold.
	TestNearlyEqual(TEXT("0.83 right: 0.85"), FMenuNavigator::SteppedValue(0.83f, 0.f, 1.f, 0.05f, 1), 0.85f, 1e-4f);
	TestNearlyEqual(TEXT("0.83 left: 0.80"), FMenuNavigator::SteppedValue(0.83f, 0.f, 1.f, 0.05f, -1), 0.80f, 1e-4f);
	TestNearlyEqual(TEXT("0.85 right: 0.90"), FMenuNavigator::SteppedValue(0.85f, 0.f, 1.f, 0.05f, 1), 0.90f, 1e-4f);
	TestNearlyEqual(TEXT("At the top it stays"), FMenuNavigator::SteppedValue(1.f, 0.f, 1.f, 0.05f, 1), 1.f, 1e-4f);
	TestNearlyEqual(TEXT("90 degrees right: 91"), FMenuNavigator::SteppedValue(90.f, 60.f, 120.f, 1.f, 1), 91.f, 1e-4f);
	TestNearlyEqual(TEXT("Below the range: from its bottom"), FMenuNavigator::SteppedValue(0.f, 60.f, 120.f, 1.f, 1), 61.f, 1e-4f);

	// The device: the mouse hides the focus look, a key brings it up, a gamepad's names the gamepad's buttons.
	LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse);
	TestFalse(TEXT("Under the mouse: no focus look"), LooterUI::IsMenuFocusShown());
	TestTrue(TEXT("An arrow key: the keyboard"), LooterUI::NoteMenuKey(EKeys::Down) && LooterUI::IsMenuFocusShown() && !LooterUI::IsMenuGamepad());
	TestFalse(TEXT("Another key: no change"), LooterUI::NoteMenuKey(EKeys::Up));
	TestTrue(TEXT("The D-pad: the gamepad"), LooterUI::NoteMenuKey(EKeys::Gamepad_DPad_Down) && LooterUI::IsMenuGamepad());
	TestTrue(TEXT("The mouse again: the look goes"), LooterUI::NoteMenuPointer() && !LooterUI::IsMenuFocusShown());
	TestEqual(TEXT("The look arrives full"), LooterUI::FocusGlowAt(0.f), 1.f);
	bool bBright = true;
	for (float Time = 0.f; Time < 5.f; Time += 0.05f)
	{
		bBright &= LooterUI::FocusGlowAt(Time) >= 0.7f && LooterUI::FocusGlowAt(Time) <= 1.f;
	}
	TestTrue(TEXT("...and breathes without ever going dim"), bBright);
	return true;
}

// ---------------------------------------------------------------------------
// The shared focus over real widgets
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuNavigatorTest, "Looter.UI.MenuNav.Navigator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMenuNavigatorTest::RunTest(const FString& Parameters)
{
	FMouseAfter Mouse;
	UWidgetTree* Tree = NewObject<UWidgetTree>(GetTransientPackage());
	// A button on top; a three-way switch (its click lights the one clicked, as the settings' do); a slider; a row of
	// three buttons with the middle one disabled.
	ULooterButton* Top = MakeTestButton(Tree, TEXT("Top"), TEXT("Top"));
	TArray<ULooterButton*> Choices = { MakeTestButton(Tree, TEXT("Low"), TEXT("Choice"), 0), MakeTestButton(Tree, TEXT("Mid"), TEXT("Choice"), 1),
		MakeTestButton(Tree, TEXT("High"), TEXT("Choice"), 2) };
	for (ULooterButton* Choice : Choices)
	{
		Choice->OnButtonClicked.BindLambda([&Choices](ULooterButton* Clicked)
		{
			for (ULooterButton* Other : Choices)
			{
				Other->SetHighlighted(Other == Clicked);
			}
		});
	}
	Choices[0]->SetHighlighted(true);
	UWidget* ChoiceRow = LooterUI::MakeRow(Tree, nullptr);
	UWidget* ChoiceLight = LooterUI::AddFocusFrame(ChoiceRow);
	USlider* Slider = Tree->ConstructWidget<USlider>(USlider::StaticClass());
	Slider->SetMinValue(0.f);
	Slider->SetMaxValue(1.f);
	Slider->SetStepSize(0.1f);
	Slider->SetValue(0.5f);
	UWidget* SliderRow = LooterUI::MakeRow(Tree, nullptr);
	UWidget* SliderLight = LooterUI::AddFocusFrame(SliderRow);
	ULooterButton* Left = MakeTestButton(Tree, TEXT("Left"), TEXT("Left"));
	ULooterButton* Off = MakeTestButton(Tree, TEXT("Off"), TEXT("Off"));
	Off->SetIsEnabled(false);
	ULooterButton* Right = MakeTestButton(Tree, TEXT("Right"), TEXT("Right"));
	int32 RightPresses = 0;
	Right->OnButtonClicked.BindLambda([&RightPresses](ULooterButton*) { ++RightPresses; });
	if (!TestTrue(TEXT("Focus looks for the rows"), ChoiceLight && SliderLight))
	{
		return false;
	}

	FMenuNavigator Nav;
	Nav.SetRows({
		FMenuNavRow{ FMenuNavItem::MakeButton(Top) },
		FMenuNavRow{ FMenuNavItem::MakeChoices(Choices, ChoiceLight, ChoiceRow, 0) },
		FMenuNavRow{ FMenuNavItem::MakeSlider(Slider, SliderLight, SliderRow, 1) },
		FMenuNavRow{ FMenuNavItem::MakeButton(Left, 2), FMenuNavItem::MakeButton(Off, 2), FMenuNavItem::MakeButton(Right, 2) } });
	TestTrue(TEXT("The focus starts at the top"), Nav.GetFocusedWidget() == Top);
	TestTrue(TEXT("Up from the top, no wrap: it stays"), Nav.MoveVertical(-1) == EMenuNavResult::None && Nav.GetFocusedWidget() == Top);

	// The switch: right picks the next, A goes round, right stops at the end.
	TestTrue(TEXT("Down: the switch"), Nav.MoveVertical(1) == EMenuNavResult::Moved && Nav.GetFocused()->IsChoices());
	TestTrue(TEXT("Right picks Mid"), Nav.MoveHorizontal(1) == EMenuNavResult::Chose && Choices[1]->IsHighlighted() && !Choices[0]->IsHighlighted());
	TestTrue(TEXT("A picks High"), Nav.Activate() == EMenuNavResult::Chose && Choices[2]->IsHighlighted());
	TestTrue(TEXT("Right at the end: nothing"), Nav.MoveHorizontal(1) == EMenuNavResult::None && Choices[2]->IsHighlighted());
	TestTrue(TEXT("A at the end goes round to Low"), Nav.Activate() == EMenuNavResult::Chose && Choices[0]->IsHighlighted());
	TestTrue(TEXT("Left at the start: nothing"), Nav.MoveHorizontal(-1) == EMenuNavResult::None);

	// The slider: a step each way, held as a controller holds it until let go; the ends stop it.
	TestTrue(TEXT("Down: the slider"), Nav.MoveVertical(1) == EMenuNavResult::Moved && Nav.GetFocusedWidget() == Slider);
	TestTrue(TEXT("Right steps it up a step"), Nav.MoveHorizontal(1) == EMenuNavResult::Stepped && FMath::IsNearlyEqual(Slider->GetValue(), 0.6f, 1e-4f));
	TestTrue(TEXT("...and holds it"), Nav.GetHeldSlider() == Slider);
	TestTrue(TEXT("Left steps it back"), Nav.MoveHorizontal(-1) == EMenuNavResult::Stepped && FMath::IsNearlyEqual(Slider->GetValue(), 0.5f, 1e-4f));
	Nav.EndSliderHold();
	TestNull(TEXT("Let go"), Nav.GetHeldSlider());
	Slider->SetValue(1.f);
	TestTrue(TEXT("At its top: right does nothing"), Nav.MoveHorizontal(1) == EMenuNavResult::None && Nav.GetHeldSlider() == nullptr);
	Nav.MoveHorizontal(-1);
	TestTrue(TEXT("Moving off a held slider lets it go"), Nav.MoveVertical(1) == EMenuNavResult::Moved && Nav.GetHeldSlider() == nullptr);

	// The bottom row: along it, past the disabled one; the column is kept.
	TestTrue(TEXT("Down: the row's first"), Nav.GetFocusedWidget() == Left);
	TestTrue(TEXT("Right skips the disabled one"), Nav.MoveHorizontal(1) == EMenuNavResult::Moved && Nav.GetFocusedWidget() == Right);
	TestTrue(TEXT("Right at the row's end: nothing"), Nav.MoveHorizontal(1) == EMenuNavResult::None);
	TestTrue(TEXT("A presses it"), Nav.Activate() == EMenuNavResult::Pressed && RightPresses == 1);
	TestTrue(TEXT("Down at the bottom, no wrap: it stays"), Nav.MoveVertical(1) == EMenuNavResult::None);
	Nav.SetWrap(true);
	TestTrue(TEXT("With wrap: round to the top"), Nav.MoveVertical(1) == EMenuNavResult::Moved && Nav.GetFocusedWidget() == Top);
	TestTrue(TEXT("...and back up round to the bottom row, its kept column"), Nav.MoveVertical(-1) == EMenuNavResult::Moved && Nav.GetFocusedWidget() == Right);

	// The mouse pointing: focus follows to what it points at, never to a disabled button.
	TestTrue(TEXT("Pointing at a choice focuses the switch"), Nav.FocusOn(Choices[1]) && Nav.GetFocused()->IsChoices());
	TestFalse(TEXT("A disabled button isn't a stop"), Nav.FocusOn(Off));

	// The focus look: only shown when told (keys or a gamepad), on the stop with the focus alone.
	TestFalse(TEXT("Not shown: nothing lit"), LooterUI::IsFocusLit(ChoiceLight) || Top->IsFocusLit());
	Nav.SetShown(true);
	TestTrue(TEXT("Shown: the switch's row lit"), LooterUI::IsFocusLit(ChoiceLight));
	Nav.MoveVertical(-1);
	TestTrue(TEXT("Moved up: the top button lit, the row out"), Top->IsFocusLit() && !LooterUI::IsFocusLit(ChoiceLight));
	Nav.Tick(0.5f);
	TestTrue(TEXT("...still lit as it breathes"), Top->IsFocusLit());
	Nav.SetShown(false);
	TestFalse(TEXT("Hidden again: nothing lit"), Top->IsFocusLit() || LooterUI::IsFocusLit(ChoiceLight) || LooterUI::IsFocusLit(SliderLight));

	// The bumpers: the next section's first stop, round the ends (the top button is above the first, the row below the last).
	Nav.FocusOn(Top);
	TestTrue(TEXT("RB from above the sections: the first"), Nav.JumpSection(1, 2) == EMenuNavResult::Moved && Nav.GetFocused()->IsChoices());
	TestTrue(TEXT("RB: the second"), Nav.JumpSection(1, 2) == EMenuNavResult::Moved && Nav.GetFocusedWidget() == Slider);
	TestTrue(TEXT("RB round to the first"), Nav.JumpSection(1, 2) == EMenuNavResult::Moved && Nav.GetFocused()->IsChoices());
	TestTrue(TEXT("LB round to the last"), Nav.JumpSection(-1, 2) == EMenuNavResult::Moved && Nav.GetFocusedWidget() == Slider);
	Nav.FocusOn(Left);
	TestTrue(TEXT("LB from below the sections: the last"), Nav.JumpSection(-1, 2) == EMenuNavResult::Moved && Nav.GetFocusedWidget() == Slider);

	// New stops keep the focus where it was, or go where they're told.
	Nav.SetRows({ FMenuNavRow{ FMenuNavItem::MakeButton(Top) }, FMenuNavRow{ FMenuNavItem::MakeSlider(Slider, SliderLight, SliderRow, 1) } });
	TestTrue(TEXT("Rebuilt: still on the slider"), Nav.GetFocusedWidget() == Slider);
	Nav.SetRows({ FMenuNavRow{ FMenuNavItem::MakeButton(Top) }, FMenuNavRow{ FMenuNavItem::MakeButton(Right) } }, Right);
	TestTrue(TEXT("Rebuilt with a preference: there"), Nav.GetFocusedWidget() == Right);
	return true;
}

// ---------------------------------------------------------------------------
// The settings menu
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSettingsMenuNavTest, "Looter.UI.MenuNav.SettingsMenu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSettingsMenuNavTest::RunTest(const FString& Parameters)
{
	FMouseAfter Mouse;
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
	int32 Closed = 0;
	Menu->OnClose.BindLambda([&Closed]() { ++Closed; });
	USlider* FieldOfView = Menu->GetFieldOfViewSlider();
	if (!TestTrue(TEXT("Its sliders and ESC BACK"), FieldOfView && Menu->GetMinimapSizeSlider() && Menu->GetResumeButton()))
	{
		return false;
	}
	FieldOfView->SetValue(UGraphicsSettingsSubsystem::MinFieldOfView);
	Menu->Open(ESettingsMenuMode::Pause);
	Menu->AdvanceNav(0.f);

	// Opened under the mouse: the focus waits on the page's first setting (the full page has no Back row: Esc, B or ESC
	// BACK clicked go back), unseen, and the hints name the keyboard's keys.
	TestTrue(TEXT("The focus starts on the quality presets"), ActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionQuality);
	TestFalse(TEXT("...with no focus look under the mouse"), Menu->IsFocusLookShown());
	TestTrue(TEXT("Keyboard hints"), PromptsText(Menu->GetPrompts()).StartsWith(TEXT("Enter Change")) && PromptsText(Menu->GetPrompts()).EndsWith(TEXT("Esc Back")));

	// The first D-pad press only shows where the focus is; the hints turn to the gamepad's buttons.
	TestTrue(TEXT("The D-pad is the menu's"), Menu->HandleNavKey(EKeys::Gamepad_DPad_Down));
	TestTrue(TEXT("Still on the quality presets, now lit"), ActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionQuality && Menu->IsFocusLookShown());
	const FString PadPrompts = PromptsText(Menu->GetPrompts());
	TestTrue(TEXT("Gamepad hints: A Change (a switch), LB / RB Tabs, B Back (to the pause menu)"),
		PadPrompts.StartsWith(TEXT("A Change")) && PadPrompts.Contains(TEXT("LB / RB Tabs")) && PadPrompts.EndsWith(TEXT("B Back")));
	TestTrue(TEXT("Up from the first setting: it stays (the list doesn't wrap)"),
		Menu->HandleNavKey(EKeys::Gamepad_DPad_Up) && ActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionQuality);

	// Down the list in order: motion blur, the field of view.
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down: motion blur"), ActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionMotionBlurOn);
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down: the field of view slider"), Menu->GetFocusedWidget() == FieldOfView);
	TestTrue(TEXT("...adjusted with the D-pad"), PromptsText(Menu->GetPrompts()).StartsWith(TEXT("D-pad Adjust")));
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Down);

	// The slider: a press steps it a degree and takes it, so the view's preview shows as for a drag.
	const float Bottom = UGraphicsSettingsSubsystem::MinFieldOfView;
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Right);
	TestNearlyEqual(TEXT("Right: a degree up"), FieldOfView->GetValue(), Bottom + 1.f, 1e-3f);
	TestTrue(TEXT("...the field of view preview up"), Menu->IsFieldOfViewPreviewActive());
	TestTrue(TEXT("The platform's own repeat is ignored"), Menu->HandleNavKey(EKeys::Gamepad_DPad_Right, true));
	TestNearlyEqual(TEXT("...no extra step"), FieldOfView->GetValue(), Bottom + 1.f, 1e-3f);
	HoldFor(*Menu, RepeatDelay * 0.8f);
	TestNearlyEqual(TEXT("Held, before the delay: no more"), FieldOfView->GetValue(), Bottom + 1.f, 1e-3f);
	HoldFor(*Menu, RepeatDelay * 0.3f);
	TestNearlyEqual(TEXT("...then it repeats"), FieldOfView->GetValue(), Bottom + 2.f, 1e-3f);
	// Half a second just after the delay against half a second at the end of the run-up: the later steps come far faster.
	const float BeforeEarly = FieldOfView->GetValue();
	HoldFor(*Menu, 0.5f);
	const float EarlySteps = FieldOfView->GetValue() - BeforeEarly;
	HoldFor(*Menu, AccelerateSeconds - 1.f);
	const float BeforeLate = FieldOfView->GetValue();
	HoldFor(*Menu, 0.5f);
	const float LateSteps = FieldOfView->GetValue() - BeforeLate;
	TestTrue(TEXT("Held on, a slider speeds up (twice the early pace and more)"), EarlySteps >= 3.f && LateSteps >= EarlySteps * 2.f);
	TestTrue(TEXT("...never past its end"), FieldOfView->GetValue() <= UGraphicsSettingsSubsystem::MaxFieldOfView + 1e-3f);

	// Let go: the slider's let-go (the setting saved), the preview lingering a moment; nothing left to cancel.
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Right);
	const float LetGo = FieldOfView->GetValue();
	HoldFor(*Menu, 1.f);
	TestNearlyEqual(TEXT("Let go: no more steps"), FieldOfView->GetValue(), LetGo, 1e-3f);
	TestFalse(TEXT("...and nothing held to cancel"), Menu->CancelPreviewDrag());

	// Stepped again and B while it's held: back where it was taken, and the menu stays open.
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Left);
	TestNearlyEqual(TEXT("Left: a degree down"), FieldOfView->GetValue(), LetGo - 1.f, 1e-3f);
	TestTrue(TEXT("B is the menu's"), Menu->HandleNavKey(EKeys::Gamepad_FaceButton_Right));
	TestNearlyEqual(TEXT("B while held: back where it was taken"), FieldOfView->GetValue(), LetGo, 1e-3f);
	TestEqual(TEXT("...and the menu stays open"), Closed, 0);
	HoldFor(*Menu, 1.f);
	TestNearlyEqual(TEXT("...with nothing repeating after"), FieldOfView->GetValue(), LetGo, 1e-3f);

	// The bumpers turn the tabs: Graphics (on show), Audio, Interface; the minimap's size two stops into Interface.
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Up);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Up);
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Up);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Up);
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Up);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Up);
	TestTrue(TEXT("Back up to the quality presets, the page's top"), ActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionQuality);
	TestEqual(TEXT("...on the Graphics tab"), Menu->GetPage(), static_cast<int32>(SettingsMenu::ESettingsPage::Graphics));
	Menu->HandleNavKey(EKeys::Gamepad_RightShoulder);
	const UWidget* Master = Menu->GetFocusedWidget();
	TestTrue(TEXT("RB: the Audio tab, its first volume slider"), Cast<USlider>(Master) && Master != FieldOfView
		&& Menu->GetPage() == static_cast<int32>(SettingsMenu::ESettingsPage::Audio));
	Menu->HandleNavKey(EKeys::Gamepad_RightShoulder);
	TestTrue(TEXT("RB: the Interface tab, a slider (the UI transparency)"), Cast<USlider>(Menu->GetFocusedWidget()) && Menu->GetFocusedWidget() != Master
		&& Menu->GetPage() == static_cast<int32>(SettingsMenu::ESettingsPage::Interface));
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down: the minimap's switch"), ActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionMinimapOn);
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down: the minimap's size"), Menu->GetFocusedWidget() == Menu->GetMinimapSizeSlider());
	Menu->HandleNavKey(EKeys::Gamepad_LeftShoulder);
	TestTrue(TEXT("LB: back to the start of the Audio tab"), Menu->GetFocusedWidget() == Master);

	// The left stick moves like the D-pad, and its own direction keys never count twice.
	Menu->HandleNavStick(FVector2D(0.f, -0.9f));
	const UWidget* StickedTo = Menu->GetFocusedWidget();
	TestTrue(TEXT("The stick down: the next stop"), StickedTo != Master && Cast<USlider>(StickedTo));
	TestTrue(TEXT("The stick's down key is taken..."), Menu->HandleNavKey(EKeys::Gamepad_LeftStick_Down));
	TestTrue(TEXT("...and moves nothing"), Menu->GetFocusedWidget() == StickedTo);
	Menu->HandleNavStick(FVector2D::ZeroVector);
	HoldFor(*Menu, 1.f);
	TestTrue(TEXT("The stick at rest: no repeats"), Menu->GetFocusedWidget() == StickedTo);

	// The mouse: the focus look goes; a keyboard arrow brings it back with the keyboard's prompts.
	LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse);
	Menu->AdvanceNav(0.f);
	TestFalse(TEXT("Under the mouse: no focus look"), Menu->IsFocusLookShown());
	Menu->HandleNavKey(EKeys::Down);
	TestTrue(TEXT("An arrow key: the look is back where the focus was"), Menu->IsFocusLookShown() && Menu->GetFocusedWidget() == StickedTo);
	TestTrue(TEXT("...with the keyboard's prompts"), PromptsText(Menu->GetPrompts()).Contains(TEXT("PgUp / PgDn Tabs")));
	Menu->HandleNavKeyUp(EKeys::Down);

	// Opened again: back on the first setting of the first tab; ESC BACK clicked goes back (OnClose: the HUD shows the pause
	// menu), and B goes back from anywhere.
	Menu->Open(ESettingsMenuMode::Pause);
	TestTrue(TEXT("Opened again: back on the quality presets"), ActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionQuality);
	TestEqual(TEXT("...on the Graphics tab"), Menu->GetPage(), static_cast<int32>(SettingsMenu::ESettingsPage::Graphics));
	TestTrue(TEXT("ESC BACK clicked closes the page"), Menu->GetResumeButton()->Press() && Closed == 1);
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("B with nothing held closes it"), Menu->HandleNavKey(EKeys::Gamepad_FaceButton_Right) && Closed == 2);
	TestTrue(TEXT("A held B's repeat doesn't close it again"), Menu->HandleNavKey(EKeys::Gamepad_FaceButton_Right, true) && Closed == 2);

	// From the main menu the focus starts on the first setting, and B says Back.
	Menu->Open(ESettingsMenuMode::MainMenu);
	TestTrue(TEXT("From the main menu: the quality presets first"), ActionOf(Menu->GetFocusedWidget()) == SettingsMenu::ActionQuality);
	TestTrue(TEXT("...B goes back"), PromptsText(Menu->GetPrompts()).Contains(TEXT("B Back")));
	return true;
}

// ---------------------------------------------------------------------------
// The main menu
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMainMenuNavTest, "Looter.UI.MenuNav.MainMenu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMainMenuNavTest::RunTest(const FString& Parameters)
{
	FMouseAfter Mouse;
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
	ULooterButton* SinglePlayer = Menu->GetMainButton(TEXT("SinglePlayer"));
	ULooterButton* Settings = Menu->GetMainButton(TEXT("Settings"));
	ULooterButton* Quit = Menu->GetMainButton(TEXT("Quit"));
	if (!TestTrue(TEXT("Its buttons"), SinglePlayer && Settings && Quit && Menu->GetPickerBackButton()))
	{
		return false;
	}
	Menu->AdvanceNav(0.f);

	// Under the mouse the focus waits on Single Player, unseen.
	TestTrue(TEXT("The focus starts on Single Player"), Menu->GetFocusedWidget() == SinglePlayer);
	TestFalse(TEXT("...with no focus look"), Menu->IsFocusLookShown());

	// The first press shows it; then down the buttons, past Multiplayer (disabled), round the ends.
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("The first press shows the focus where it is"), Menu->GetFocusedWidget() == SinglePlayer && Menu->IsFocusLookShown());
	TestTrue(TEXT("...and the gamepad's prompt"), PromptsText(Menu->GetPrompts()) == TEXT("A Select"));
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down: Settings (Multiplayer is passed over)"), Menu->GetFocusedWidget() == Settings);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Down);
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down: Quit Game"), Menu->GetFocusedWidget() == Quit);
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Down);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down: round to Single Player"), Menu->GetFocusedWidget() == SinglePlayer);
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Up);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Up);
	TestTrue(TEXT("Up: round to Quit Game"), Menu->GetFocusedWidget() == Quit);

	// Held, the D-pad repeats on the menu's clock, and stops when let go.
	Menu->HandleNavKey(EKeys::Gamepad_DPad_Up);
	TestTrue(TEXT("Held up: Settings"), Menu->GetFocusedWidget() == Settings);
	HoldFor(*Menu, RepeatDelay + 0.02f);
	TestTrue(TEXT("...then Single Player"), Menu->GetFocusedWidget() == SinglePlayer);
	Menu->HandleNavKeyUp(EKeys::Gamepad_DPad_Up);
	HoldFor(*Menu, 1.f);
	TestTrue(TEXT("Let go: it stays"), Menu->GetFocusedWidget() == SinglePlayer);

	// A on Single Player: the session picker, the focus on its first card (or Back with no sessions to read).
	TestTrue(TEXT("A is the menu's"), Menu->HandleNavKey(EKeys::Gamepad_FaceButton_Bottom));
	TestTrue(TEXT("A: the session picker"), Menu->IsPickerShown());
	const FName PickerFocus = ActionOf(Menu->GetFocusedWidget());
	TestTrue(TEXT("...the focus on a session or Back"), PickerFocus == FName(TEXT("Play")) || PickerFocus == FName(TEXT("Back")));
	TestTrue(TEXT("...and B steps back"), PromptsText(Menu->GetPrompts()) == TEXT("A Select  B Back"));
	TestTrue(TEXT("The A held on doesn't press again"), Menu->HandleNavKey(EKeys::Gamepad_FaceButton_Bottom, true) && Menu->IsPickerShown());

	// B: back to the main buttons, on Single Player; B there does nothing.
	TestTrue(TEXT("B is the menu's"), Menu->HandleNavKey(EKeys::Gamepad_FaceButton_Right));
	TestTrue(TEXT("B: the main buttons, on Single Player"), !Menu->IsPickerShown() && Menu->GetFocusedWidget() == SinglePlayer);
	Menu->HandleNavKey(EKeys::Gamepad_FaceButton_Right);
	TestTrue(TEXT("B on the main buttons: nothing"), !Menu->IsPickerShown() && Menu->GetFocusedWidget() == SinglePlayer);

	// The stick moves like the D-pad.
	Menu->HandleNavStick(FVector2D(0.f, -0.9f));
	TestTrue(TEXT("The stick down: Settings"), Menu->GetFocusedWidget() == Settings);
	Menu->HandleNavStick(FVector2D::ZeroVector);

	// The mouse: the look goes, the prompts name the keyboard's keys.
	LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Mouse);
	Menu->AdvanceNav(0.f);
	TestFalse(TEXT("Under the mouse: no focus look"), Menu->IsFocusLookShown() || Settings->IsFocusLit());
	TestTrue(TEXT("...keyboard prompts"), PromptsText(Menu->GetPrompts()) == TEXT("Enter Select"));
	return true;
}

#endif
