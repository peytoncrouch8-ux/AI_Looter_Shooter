#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/HUD/LooterHUD.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/PauseMenuArt.h"
#include "UI/Menus/PauseMenuIcons.h"
#include "UI/Menus/PauseMenuLayout.h"
#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterReticleArt.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Tests/AutomationCommon.h"

// The pause menu's hub, E2 "Big Dial" (the user, 2026-10-31: "the ESC menu should encompass the entire screen and the
// options should be on the left hand side ... Esc Menu should not have whats in the inventory tabs"; contract
// Docs/Archive/Work/2026-10-30-PauseMenu/CONTRACT.md): its four options in order, keys and a controller through them as through the
// settings, the needle swinging to the lit option's tick, and the hub's layout fitting 1080p with the in-game type a third
// bigger than the mockup's.

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

	/** The look stepped Seconds on, a frame at a time. */
	void PauseHubRun(UPauseMenuWidget& Hub, float Seconds)
	{
		for (float Done = 0.f; Done < Seconds; Done += 1.f / 60.f)
		{
			Hub.AdvanceLook(1.f / 60.f);
		}
	}

	bool PauseBoxesMeet(const FBox2D& A, const FBox2D& B)
	{
		return A.Min.X < B.Max.X && B.Min.X < A.Max.X && A.Min.Y < B.Max.Y && B.Min.Y < A.Max.Y;
	}

	bool PauseBoxOnScreen(const FBox2D& Box)
	{
		return Box.Min.X >= 0.0 && Box.Min.Y >= 0.0 && Box.Max.X <= PauseMenuLayout::DesignWidth && Box.Max.Y <= PauseMenuLayout::DesignHeight;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPauseHubTest, "Looter.UI.PauseHub.Hub",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPauseHubTest::RunTest(const FString& Parameters)
{
	FPauseHubMouseAfter Mouse;

	// The options, in the contract's order: only what the game has (no inventory tabs).
	const TConstArrayView<EPauseMenuItem> Order = UPauseMenuWidget::ItemOrder();
	if (!TestEqual(TEXT("Four options"), Order.Num(), static_cast<int32>(EPauseMenuItem::Count)))
	{
		return false;
	}
	TestTrue(TEXT("Back to Game, Settings, Save & Quit, Quit Game, in that order"), Order[0] == EPauseMenuItem::Resume
		&& Order[1] == EPauseMenuItem::Settings && Order[2] == EPauseMenuItem::SaveQuit && Order[3] == EPauseMenuItem::QuitGame);
	TestEqual(TEXT("The top option's words"), UPauseMenuWidget::ItemWord(EPauseMenuItem::Resume), FString(TEXT("BACK TO GAME")));
	TestEqual(TEXT("The second's"), UPauseMenuWidget::ItemWord(EPauseMenuItem::Settings), FString(TEXT("SETTINGS")));
	TestEqual(TEXT("The third's"), UPauseMenuWidget::ItemWord(EPauseMenuItem::SaveQuit), FString(TEXT("SAVE & QUIT")));
	TestEqual(TEXT("The last's"), UPauseMenuWidget::ItemWord(EPauseMenuItem::QuitGame), FString(TEXT("QUIT GAME")));
	TestTrue(TEXT("Save & Quit and Quit Game save first; the others don't"), UPauseMenuWidget::SavesFirst(EPauseMenuItem::SaveQuit)
		&& UPauseMenuWidget::SavesFirst(EPauseMenuItem::QuitGame) && !UPauseMenuWidget::SavesFirst(EPauseMenuItem::Resume)
		&& !UPauseMenuWidget::SavesFirst(EPauseMenuItem::Settings));
	TestEqual(TEXT("The callouts say a session is saved first"), UPauseMenuWidget::ItemFlavor(EPauseMenuItem::QuitGame, true),
		FString(TEXT("Your session is saved first.")));
	TestFalse(TEXT("...and never so without one"), UPauseMenuWidget::ItemFlavor(EPauseMenuItem::SaveQuit, false).Contains(TEXT("saved first")));
	for (const EPauseMenuItem Item : Order)
	{
		TestFalse(*FString::Printf(TEXT("%s has its callout's words"), *UPauseMenuWidget::ItemWord(Item)),
			UPauseMenuWidget::ItemHelp(Item).IsEmpty() || UPauseMenuWidget::ItemFlavor(Item, true).IsEmpty());
	}

	// Where backing out goes: the settings page back to the hub, the hub to the game, which stays paused under both.
	TestTrue(TEXT("Esc or Back from the settings: the hub"), ALooterHUD::PauseBackFrom(EPauseScreen::Settings) == EPauseScreen::Hub);
	TestTrue(TEXT("Esc on the hub: the game"), ALooterHUD::PauseBackFrom(EPauseScreen::Hub) == EPauseScreen::None);
	TestTrue(TEXT("The game stays paused under them"), ALooterHUD::PausesGame(true, false));

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UPauseMenuWidget* Hub = CreateWidget<UPauseMenuWidget>(TestLevel.GetTestWorld(), UPauseMenuWidget::StaticClass());
	if (!TestNotNull(TEXT("The hub"), Hub))
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
		TestNotNull(*FString::Printf(TEXT("A button for %s"), *UPauseMenuWidget::ItemWord(Item)), Hub->GetItemButton(Item));
	}

	// Opened under the mouse: on Back to Game, lit, unseen by the focus look; the prompts name the keyboard's keys.
	TestTrue(TEXT("It opens on Back to Game"), Hub->GetFocusedItem() == EPauseMenuItem::Resume);
	TestTrue(TEXT("...lit"), Hub->GetMarkedItem() == EPauseMenuItem::Resume);
	TestEqual(TEXT("...its words"), Hub->GetShownWord(EPauseMenuItem::Resume), FString(TEXT("BACK TO GAME")));
	TestFalse(TEXT("...no focus look under the mouse"), Hub->IsFocusLookShown());
	TestEqual(TEXT("Keyboard prompts"), PromptsText(Hub->GetPrompts()), PromptsText({ MakePrompt(EPromptKey::Select, false, TEXT("Select")),
		MakePrompt(EPromptKey::Back, false, TEXT("Resume")) }));
	// No session in a test world: the session tag says so; no mission is tracked: no mission card.
	TestTrue(TEXT("The session tag says no session"), Hub->GetSessionText().Contains(TEXT("NO SESSION")));
	TestFalse(TEXT("No mission card with no mission tracked"), Hub->IsMissionCardShown());

	// The opening: the needle swings up to Back to Game's tick and settles there.
	TestTrue(TEXT("The needle starts pointing down"), Hub->GetNeedleDegrees() > PauseMenuLayout::RowDegrees(PauseMenuLayout::NumRows - 1));
	PauseHubRun(*Hub, 1.f);
	TestTrue(TEXT("...and settles on Back to Game's tick"), FMath::IsNearlyEqual(Hub->GetNeedleDegrees(), PauseMenuLayout::RowDegrees(0), 0.01f));

	// The first D-pad press only shows where the focus is; then down the options, every one in order, and round the ends.
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("The first press: still on Back to Game, now lit"), Hub->GetFocusedItem() == EPauseMenuItem::Resume && Hub->IsFocusLookShown());
	TestTrue(TEXT("...the controller's prompts"), PromptsText(Hub->GetPrompts()).Contains(TEXT("B Resume")));
	for (int32 Index = 1; Index < Order.Num(); ++Index)
	{
		PauseHubTap(*Hub, EKeys::Gamepad_DPad_Down);
		TestTrue(*FString::Printf(TEXT("Down: %s"), *UPauseMenuWidget::ItemWord(Order[Index])), Hub->GetFocusedItem() == Order[Index]);
	}
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Down);
	TestTrue(TEXT("Down from Quit Game: round to Back to Game"), Hub->GetFocusedItem() == EPauseMenuItem::Resume);
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Up);
	TestTrue(TEXT("Up from Back to Game: round to Quit Game"), Hub->GetFocusedItem() == EPauseMenuItem::QuitGame);
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Left);
	TestTrue(TEXT("Left and right do nothing"), Hub->GetFocusedItem() == EPauseMenuItem::QuitGame);
	TestEqual(TEXT("Moving picks nothing"), Picked.Num(), 0);

	// One click per option: the lit option follows the focus, and the needle swings to its tick, a little past and back.
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Up);
	PauseHubTap(*Hub, EKeys::Gamepad_DPad_Up);
	Hub->AdvanceLook(0.f);
	TestTrue(TEXT("Up twice: Settings, lit"), Hub->GetFocusedItem() == EPauseMenuItem::Settings && Hub->GetMarkedItem() == EPauseMenuItem::Settings);
	const float SettingsTick = PauseMenuLayout::RowDegrees(1);
	TestTrue(TEXT("...the needle swinging to its tick"), FMath::IsNearlyEqual(Hub->GetNeedleTarget(), SettingsTick, 0.01f));
	// From Back to Game's tick down to Settings' (the angle grows): the overshoot goes past it, further round.
	float Furthest = Hub->GetNeedleDegrees();
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		Hub->AdvanceLook(1.f / 60.f);
		Furthest = FMath::Max(Furthest, Hub->GetNeedleDegrees());
	}
	TestTrue(TEXT("...on it within half a second"), FMath::IsNearlyEqual(Hub->GetNeedleDegrees(), SettingsTick, 0.01f));
	TestTrue(TEXT("...after a slight overshoot"), Furthest > SettingsTick + 0.05f && Furthest < SettingsTick + 3.f);

	// A on Settings picks it (the HUD opens the settings page).
	PauseHubTap(*Hub, EKeys::Gamepad_FaceButton_Bottom);
	TestTrue(TEXT("A on Settings picks it"), Picked.Num() == 1 && Picked.Last() == EPauseMenuItem::Settings);

	// Back from the settings page the HUD opens the hub again on Settings, lit, the needle already on it; Esc or B there is
	// Back to Game, and the held Esc's repeat (the one that backed out of the settings) resumes nothing.
	Hub->Open(EPauseMenuItem::Settings, /*bReturning*/ true);
	TestTrue(TEXT("Back from the settings: on Settings again"), Hub->GetFocusedItem() == EPauseMenuItem::Settings);
	TestTrue(TEXT("...lit, the needle on its tick at once"), Hub->GetMarkedItem() == EPauseMenuItem::Settings
		&& FMath::IsNearlyEqual(Hub->GetNeedleDegrees(), SettingsTick, 0.01f));
	TestTrue(TEXT("A held Esc's repeat is taken..."), Hub->HandleNavKey(EKeys::Escape, true));
	TestEqual(TEXT("...and resumes nothing"), Picked.Num(), 1);
	TestTrue(TEXT("Esc on the hub: Back to Game"), Hub->HandleNavKey(EKeys::Escape) && Picked.Num() == 2 && Picked.Last() == EPauseMenuItem::Resume);
	TestTrue(TEXT("B on the hub: Back to Game"), Hub->HandleNavKey(EKeys::Gamepad_FaceButton_Right) && Picked.Num() == 3 && Picked.Last() == EPauseMenuItem::Resume);

	// Quit Game, the last option: picked once, its tag says it's leaving, and nothing is taken after it.
	Hub->Open();
	PauseHubTap(*Hub, EKeys::Up);
	TestTrue(TEXT("Up from the top: Quit Game"), Hub->GetFocusedItem() == EPauseMenuItem::QuitGame);
	PauseHubTap(*Hub, EKeys::Enter);
	TestTrue(TEXT("Enter on it: Quit Game"), Picked.Num() == 4 && Picked.Last() == EPauseMenuItem::QuitGame);
	TestNotEqual(TEXT("...its tag says it's going"), Hub->GetShownWord(EPauseMenuItem::QuitGame), FString(TEXT("QUIT GAME")));
	Hub->HandleNavKey(EKeys::Escape);
	TestEqual(TEXT("...and Esc after it does nothing more"), Picked.Num(), 4);
	Hub->Open();
	TestEqual(TEXT("Opened again, its words are back"), Hub->GetShownWord(EPauseMenuItem::QuitGame), FString(TEXT("QUIT GAME")));

	// The settings page over the game: its Back (Esc) goes back to the hub (the HUD binds OnClose to that).
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
	TestTrue(TEXT("Esc on the settings page is its Back"), Settings->HandleNavKey(EKeys::Escape) && Closed == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPauseHubLayoutTest, "Looter.UI.PauseHub.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPauseHubLayoutTest::RunTest(const FString& Parameters)
{
	using namespace PauseMenuLayout;

	// The needle's aim per option: down the arc in order, inside its ends, and pointing exactly at the option's tick.
	for (int32 Row = 0; Row < NumRows; ++Row)
	{
		const float Degrees = RowDegrees(Row);
		TestTrue(*FString::Printf(TEXT("Option %d's tick is on the arc's span"), Row + 1), Degrees > ArcFrom && Degrees < ArcTo);
		if (Row > 0)
		{
			TestTrue(*FString::Printf(TEXT("Option %d's tick is below option %d's"), Row + 1, Row), Degrees > RowDegrees(Row - 1));
		}
		const float Angle = FMath::DegreesToRadians(Degrees);
		const FVector2D Aim = FVector2D(DialX, DialY) + ArcRadius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		TestTrue(*FString::Printf(TEXT("The needle aimed at option %d meets its tick"), Row + 1), Aim.Equals(RowArcPoint(Row), 0.01));
		TestTrue(*FString::Printf(TEXT("Option %d's tick is level with its tag"), Row + 1), FMath::IsNearlyEqual(RowArcPoint(Row).Y, RowY[Row]));
	}

	// The swing: from 0 to 1, a slight overshoot (under 6%), and settled at its end.
	TestTrue(TEXT("The swing starts at its start and ends at its end"), FMath::IsNearlyZero(NeedleSwing(0.f)) && FMath::IsNearlyEqual(NeedleSwing(1.f), 1.f));
	float Peak = 0.f;
	for (int32 Step = 0; Step <= 100; ++Step)
	{
		Peak = FMath::Max(Peak, NeedleSwing(Step / 100.f));
	}
	TestTrue(TEXT("...past its end a little on the way"), Peak > 1.01f && Peak < 1.06f);
	TestTrue(TEXT("...held at its end after"), FMath::IsNearlyEqual(NeedleSwing(1.5f), 1.f));

	// Everything on a 1920 x 1080 screen, in the in-game type (measured, a third bigger than the mockup's).
	const FSlateFontInfo TitleType = LooterUI::FloatingFont(TitleFont, TitleSpacing);
	const FBox2D Title = TitleBox(MeasureWidth(TEXT("GAME PAUSED"), TitleType));
	TArray<FString> Optionals = { TEXT("• Optional: Read your own headboard."), TEXT("• Optional: Ring the chapel bell at noon.") };
	const FBox2D Mission = MissionBox(MissionHeight(TEXT("• Read the headboard beside yours, then the one at the end of the row."), Optionals));
	const FBox2D Cards[] = { HeroBox(), Mission, SessionBox(), KeysBox(), Title, DialBox() };
	const TCHAR* const CardNames[] = { TEXT("hero card"), TEXT("mission card"), TEXT("session tag"), TEXT("key hints"), TEXT("title"), TEXT("dial") };
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Cards)); ++Index)
	{
		TestTrue(*FString::Printf(TEXT("The %s is on screen"), CardNames[Index]), PauseBoxOnScreen(Cards[Index]));
		for (int32 Other = Index + 1; Other < static_cast<int32>(UE_ARRAY_COUNT(Cards)); ++Other)
		{
			TestFalse(*FString::Printf(TEXT("The %s keeps clear of the %s"), CardNames[Index], CardNames[Other]), PauseBoxesMeet(Cards[Index], Cards[Other]));
		}
	}

	const FSlateFontInfo TagType = LooterUI::Font(TagFont, true, TagSpacing);
	const FSlateFontInfo NumberType = LooterUI::FloatingFont(NumberFont, NumberSpacing);
	const FSlateFontInfo CalloutTitleType = LooterUI::Font(CalloutTitleFont, true, CalloutTitleSpacing);
	const TConstArrayView<EPauseMenuItem> Order = UPauseMenuWidget::ItemOrder();
	for (int32 Row = 0; Row < NumRows; ++Row)
	{
		const EPauseMenuItem Item = Order[Row];
		const FString Word = UPauseMenuWidget::ItemWord(Item);
		const FBox2D Tag = TagBox(Row, false);
		const FBox2D LitTag = TagBox(Row, true);
		TestTrue(*FString::Printf(TEXT("%s's tag is on screen, lit or not"), *Word), PauseBoxOnScreen(Tag) && PauseBoxOnScreen(LitTag));
		// Its words clear of the tag's point: level with the letters' tops and feet the point's slope is 14 px in from the
		// tip, and its 4 px lit line inside that; 24 keeps them off both.
		TestTrue(*FString::Printf(TEXT("%s's words fit its tag"), *Word), TagTextX + MeasureWidth(Word, TagType) <= TagWidth - 24.f);
		// Its number (lit, moved out of the reticle's way) clear of its tag's gem.
		TestTrue(*FString::Printf(TEXT("%s's number clears its gem"), *Word),
			RowArcPoint(Row).X + NumberLitGap + MeasureWidth(FString::Printf(TEXT("%02d"), Row + 1), NumberType) + 4.f <= Tag.Min.X);
		for (const FBox2D& Card : Cards)
		{
			TestFalse(*FString::Printf(TEXT("%s's tag keeps clear of the cards"), *Word), PauseBoxesMeet(LitTag, Card));
		}
		for (int32 Other = Row + 1; Other < NumRows; ++Other)
		{
			TestFalse(*FString::Printf(TEXT("%s's tag keeps clear of the next ones'"), *Word), PauseBoxesMeet(LitTag, TagBox(Other, true)));
		}

		// Its callout, as tall as its words need, beside the lit tag and clear of everything else.
		const FString Help = UPauseMenuWidget::ItemHelp(Item);
		const float Height = FMath::Max(CalloutHeight(Help, UPauseMenuWidget::ItemFlavor(Item, true)), CalloutHeight(Help, UPauseMenuWidget::ItemFlavor(Item, false)));
		const FBox2D Callout = CalloutBox(Row, Height);
		TestTrue(*FString::Printf(TEXT("%s's callout is on screen"), *Word), PauseBoxOnScreen(Callout));
		TestTrue(*FString::Printf(TEXT("%s's callout's title fits on one line"), *Word), MeasureWidth(Word, CalloutTitleType) <= CalloutTextWidth);
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(Cards)); ++Index)
		{
			TestFalse(*FString::Printf(TEXT("%s's callout keeps clear of the %s"), *Word, CardNames[Index]), PauseBoxesMeet(Callout, Cards[Index]));
		}
		for (int32 Other = 0; Other < NumRows; ++Other)
		{
			TestFalse(*FString::Printf(TEXT("%s's callout keeps clear of option %d's tag"), *Word, Other + 1), PauseBoxesMeet(Callout, TagBox(Other, true)));
		}
	}

	// The pictures: every icon drawn (an unknown id blank), the dial's view box round its centre, the needle's pivot inside
	// its strip.
	for (const FName Id : PauseMenuIcons::Ids())
	{
		TestTrue(*FString::Printf(TEXT("Icon %s is drawn"), *Id.ToString()), PauseMenuIcons::Get(Id).Layers.Num() > 0);
	}
	TestEqual(TEXT("An unknown icon is blank"), PauseMenuIcons::Get(TEXT("NoSuchIcon")).Layers.Num(), 0);
	const LooterUI::FPaintedIcon Dial = PauseMenuArt::Dial(104.f, 0.f);
	TestTrue(TEXT("The dial's view box is twice its extent"), Dial.ViewBox.Equals(FVector2D(PauseMenuArt::DialExtent(104.f) * 2.f), 0.01));
	TestTrue(TEXT("...reaching past its long ticks"), PauseMenuArt::DialExtent(104.f) > 104.f * 1.2f);
	const LooterUI::FPaintedIcon NeedlePicture = PauseMenuArt::DialNeedle(DialRadius, ArcRadius);
	const FVector2D Pivot = PauseMenuArt::DialNeedleCentre(DialRadius, ArcRadius);
	TestTrue(TEXT("The needle's pivot is inside its strip"), Pivot.X > 0.0 && Pivot.Y > 0.0 && Pivot.X < NeedlePicture.ViewBox.X && Pivot.Y < NeedlePicture.ViewBox.Y);
	TestTrue(TEXT("...which reaches out to the range arc"), NeedlePicture.ViewBox.X - Pivot.X > ArcRadius);
	TestTrue(TEXT("A tag's view box is its size and the pad"),
		PauseMenuArt::Tag(TagWidth, TagHeight, PauseMenuArt::ELook::Lit).ViewBox.Equals(FVector2D(TagWidth, TagHeight) + FVector2D(LooterReticle::Pad * 2.f), 0.01));
	TestTrue(TEXT("A shape's glass and lines are its two parts"),
		PauseMenuArt::Card(300.f, 200.f, true, false, 0.f, PauseMenuArt::EPart::Fill).Layers.Num() + PauseMenuArt::Card(300.f, 200.f, true, false, 0.f, PauseMenuArt::EPart::Lines).Layers.Num()
		== PauseMenuArt::Card(300.f, 200.f, true, false, 0.f).Layers.Num());
	TestTrue(TEXT("The backdrop is painted"), PauseMenuArt::Backdrop().Layers.Num() > 4);
	return true;
}

#endif
