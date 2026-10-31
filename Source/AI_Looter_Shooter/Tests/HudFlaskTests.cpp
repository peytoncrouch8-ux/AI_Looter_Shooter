#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerFlaskRules.h"
#include "Tutorial/ControlHintRules.h"
#include "UI/HUD/HudFlaskWidget.h"
#include "UI/HUD/HudPlayerFrameShapes.h"
#include "UI/HUD/HudPlayerFrameWidget.h"
#include "UI/HUD/PlayerHUDWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The soul flask on the HUD (Docs/Archive/Work/2026-10-30-SoulFlask): the row's plain rules (Looter.UI.Flask.Rules), its place
// under the player frame clear of the medallion, the gem and the experience bar (Looter.UI.Flask.Place), the widget's
// pips, flashes and words (Looter.UI.Flask.Widget), and the first-time hint (Looter.Tutorial.Hints.Flask).

namespace
{
	/** Seconds of updates a twentieth of a second apart, the flask as given. */
	void RunFlaskRow(UHudFlaskWidget& Row, int32 Charges, float SoulShare, float Seconds, float DrinkAlpha = 0.f, bool bHealing = false)
	{
		constexpr float Step = 0.05f;
		const int32 Steps = FMath::Max(1, FMath::RoundToInt32(Seconds / Step));
		for (int32 Index = 0; Index < Steps; ++Index)
		{
			Row.UpdateState(true, Charges, SoulShare, DrinkAlpha, bHealing, TEXT("H"), Step);
		}
	}

	/** Seconds of hint looks a tenth of a second apart, each seeing In. */
	void RunHints(FControlHintRules& Rules, const FControlHintInput& In, float Seconds)
	{
		const int32 Looks = FMath::RoundToInt32(Seconds / 0.1f);
		for (int32 Index = 0; Index < Looks; ++Index)
		{
			Rules.Update(In, 0.1f);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudFlaskRulesTest, "Looter.UI.Flask.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudFlaskRulesTest::RunTest(const FString& Parameters)
{
	using Row = UHudFlaskWidget;

	// The pips: full charges lit, the next one filling with the soul toward it, none past it.
	TestEqual(TEXT("Three pips, as the flask's charges"), Row::MaxPips, FFlaskRules::MaxCharges);
	TestEqual(TEXT("A full charge: lit"), Row::PipTarget(0, 2, 0.4f), 1.f);
	TestEqual(TEXT("The next one: the soul toward it"), Row::PipTarget(2, 2, 0.4f), 0.4f);
	TestEqual(TEXT("Past it: empty"), Row::PipTarget(2, 1, 0.4f), 0.f);
	TestEqual(TEXT("Soul over a charge never overfills the pip"), Row::PipTarget(1, 1, 1.5f), 1.f);
	TestEqual(TEXT("Empty flask, no soul: all dark"), Row::PipTarget(0, 0, 0.f), 0.f);

	// The flashes: each change that should show does; soul and a load don't (the pip's rise and the snap show them).
	EHudFlaskFlash Flash = EHudFlaskFlash::Filled;
	TestTrue(TEXT("A drink pours"), Row::FlashFor(EFlaskChange::Drunk, Flash) && Flash == EHudFlaskFlash::Drunk);
	TestTrue(TEXT("A filled charge pops"), Row::FlashFor(EFlaskChange::Filled, Flash) && Flash == EHudFlaskFlash::Filled);
	TestTrue(TEXT("A respawn's top-up flashes softly"), Row::FlashFor(EFlaskChange::Refilled, Flash) && Flash == EHudFlaskFlash::Refilled);
	TestTrue(TEXT("None left: red and a shake"), Row::FlashFor(EFlaskChange::DeniedEmpty, Flash) && Flash == EHudFlaskFlash::DeniedEmpty);
	TestTrue(TEXT("Full health: the words"), Row::FlashFor(EFlaskChange::DeniedFull, Flash) && Flash == EHudFlaskFlash::DeniedFull);
	TestFalse(TEXT("Soul coming in: no flash"), Row::FlashFor(EFlaskChange::Soul, Flash));
	TestFalse(TEXT("A load: no flash"), Row::FlashFor(EFlaskChange::Restored, Flash));

	// The icon's tip: up and back over the drink, the most at its middle, toward the drinker (left).
	TestEqual(TEXT("No tip before the drink"), Row::TipAngle(0.f), 0.f);
	TestTrue(TEXT("Tipped most at the middle"), FMath::IsNearlyEqual(Row::TipAngle(0.5f), -Row::TipDegrees, 0.01f));
	TestTrue(TEXT("Upright again at the end"), FMath::IsNearlyZero(Row::TipAngle(1.f), 0.01f));

	// A pour drains a whole pip in DrunkSeconds; soul rises to its level and gets there.
	float Level = 1.f;
	for (float Time = 0.f; Time < Row::DrunkSeconds + 0.01f; Time += 0.05f)
	{
		Level = Row::StepLevel(Level, 0.f, 0.05f);
	}
	TestEqual(TEXT("Poured out in DrunkSeconds"), Level, 0.f);
	TestTrue(TEXT("Half way, half a pip"), FMath::IsNearlyEqual(Row::StepLevel(1.f, 0.f, Row::DrunkSeconds * 0.5f), 0.5f, 0.001f));
	Level = 0.f;
	for (int32 Step = 0; Step < 40; ++Step)
	{
		Level = Row::StepLevel(Level, 0.34f, 0.05f);
	}
	TestEqual(TEXT("A mote's soul rises to its level"), Level, 0.34f);
	TestTrue(TEXT("...not at once"), Row::StepLevel(0.f, 0.34f, 0.05f) < 0.34f);

	// The refusal's shake dies away; the feed's line is in the feed's words.
	TestEqual(TEXT("The shake is still at its end"), Row::ShakeOffset(Row::DeniedSeconds), 0.f);
	TestTrue(TEXT("...and moving in it"), FMath::Abs(Row::ShakeOffset(0.03f)) > 0.5f);
	TestTrue(TEXT("A filled charge's feed line"), UPlayerHUDWidget::FlaskChargeLine().StartsWith(TEXT("+1 ")));

	// The icon (Art/Icons/InkedIcons.py), found by its name too.
	TestTrue(TEXT("The flask's Inked icon"), Row::Icon().Shapes.Num() > 0);
	TestNotNull(TEXT("...by its name"), LooterUI::FindInkedIcon(TEXT("SoulFlask")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudFlaskPlaceTest, "Looter.UI.Flask.Place",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudFlaskPlaceTest::RunTest(const FString& Parameters)
{
	// Its place (1080p pixels): under the experience bar and the level gem, right of the medallion, on the screen.
	using Row = UHudFlaskWidget;
	using Frame = UHudPlayerFrameWidget;
	const float Top = Row::Middle - Row::IconSize * 0.5f;
	const float Bottom = Row::Middle + Row::IconSize * 0.5f;
	TestTrue(TEXT("Under the experience bar"), Top > Frame::XPBarY + HudFrameShapes::XPThickness + 4.f);
	TestTrue(TEXT("Right of the level gem's right tip"), Row::Left > Frame::GemX + Frame::GemReach);
	// The icon's box: the flask drawn in it keeps about 6 px of it clear at the foot besides.
	TestTrue(TEXT("On the screen, with room under it"), Bottom <= Row::ScreenBottom - 16.f);
	TestTrue(TEXT("The pips and the tab fit in the icon's height"), Row::PipHeight <= Row::IconSize && Row::TabHeight <= Row::IconSize);
	TestTrue(TEXT("Big enough to read beside the frame: the flask about the level gem's height"),
		Row::IconSize * 0.72f >= Frame::GemReach * 2.f * 0.85f);
	TestTrue(TEXT("Starting where the experience bar does"), FMath::IsNearlyEqual(Row::Left, Frame::XPBarX));
	// The medallion is a diamond: at the row's top its edge is this far right of its middle.
	const float MedallionEdge = Frame::MedallionX + FMath::Max(0.f, Frame::MedallionReach - FMath::Abs(Top - Frame::MedallionY));
	TestTrue(TEXT("Right of the medallion's edge"), Row::Left > MedallionEdge + 8.f);

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UPlayerHUDWidget* Hud = CreateWidget<UPlayerHUDWidget>(TestLevel.GetTestWorld(), UPlayerHUDWidget::StaticClass());
	if (!TestNotNull(TEXT("The HUD"), Hud))
	{
		return false;
	}
	const TSharedRef<SWidget> HudSlate = Hud->TakeWidget();
	const UHudFlaskWidget* Flask = Hud->GetFlaskRow();
	const UCanvasPanelSlot* FlaskSlot = Flask ? Cast<UCanvasPanelSlot>(Flask->Slot) : nullptr;
	if (!TestNotNull(TEXT("The flask's row on the HUD's canvas"), FlaskSlot))
	{
		return false;
	}
	TestTrue(TEXT("Anchored bottom-left, as the player frame"), FlaskSlot->GetAnchors().Minimum.Equals(FVector2D(0.f, 1.f)));
	TestTrue(TEXT("Its middle at Middle, its left end at Left"),
		FlaskSlot->GetPosition().Equals(FVector2D(Row::Left, -(Row::ScreenBottom - Row::Middle)), 0.01f));
	TestTrue(TEXT("...by its left end's middle"), FlaskSlot->GetAlignment().Equals(FVector2D(0.f, 0.5f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHudFlaskWidgetTest, "Looter.UI.Flask.Widget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHudFlaskWidgetTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UHudFlaskWidget* Row = CreateWidget<UHudFlaskWidget>(TestLevel.GetTestWorld(), UHudFlaskWidget::StaticClass());
	if (!TestNotNull(TEXT("The flask's row"), Row))
	{
		return false;
	}
	const TSharedRef<SWidget> RowSlate = Row->TakeWidget();

	// No flask: nothing shown. A full flask: shown as it is at once, all three lit.
	Row->Update(nullptr, TEXT("H"), 0.05f);
	TestFalse(TEXT("No flask: hidden"), Row->IsShowing());
	RunFlaskRow(*Row, 3, 0.f, 0.05f);
	TestTrue(TEXT("A flask: shown"), Row->IsShowing());
	TestTrue(TEXT("...full at once, nothing filled on the way"), Row->GetShownLevel(0) == 1.f && Row->GetShownLevel(2) == 1.f);

	// A drink: its pip pours out over DrunkSeconds, flashing.
	Row->Flash(EHudFlaskFlash::Drunk, 2);
	TestTrue(TEXT("A drink flashes"), Row->IsFlashing() && Row->GetFlashKind() == EHudFlaskFlash::Drunk);
	RunFlaskRow(*Row, 2, 0.f, 0.1f);
	TestTrue(TEXT("...its pip pouring"), Row->GetShownLevel(2) > 0.f && Row->GetShownLevel(2) < 1.f);
	RunFlaskRow(*Row, 2, 0.f, UHudFlaskWidget::DrunkSeconds);
	TestEqual(TEXT("...poured out"), Row->GetShownLevel(2), 0.f);
	TestEqual(TEXT("...the others still full"), Row->GetShownLevel(1), 1.f);

	// Soul comes in: the next pip rises to it; a filled charge pops.
	RunFlaskRow(*Row, 2, 0.6f, 1.f);
	TestTrue(TEXT("Soul: the next pip at its level"), FMath::IsNearlyEqual(Row->GetShownLevel(2), 0.6f, 0.01f));
	Row->Flash(EHudFlaskFlash::Filled, 3);
	RunFlaskRow(*Row, 3, 0.f, 0.05f);
	TestTrue(TEXT("A filled charge flashes"), Row->IsFlashing() && Row->GetFlashKind() == EHudFlaskFlash::Filled);

	// Refusals: at full health the words, once per press; with none left red and a shake, and the row dim.
	Row->Flash(EHudFlaskFlash::DeniedFull);
	RunFlaskRow(*Row, 3, 0.f, 0.2f);
	TestTrue(TEXT("Full health: \"Health full\""), Row->IsSayingHealthFull());
	RunFlaskRow(*Row, 3, 0.f, UHudFlaskWidget::WordsSeconds);
	TestFalse(TEXT("...gone after its time"), Row->IsSayingHealthFull());
	Row->SnapNext();
	RunFlaskRow(*Row, 0, 0.f, 0.05f);
	TestTrue(TEXT("None left: dim"), Row->IsDim());
	TestTrue(TEXT("...a load shows it at once"), Row->GetShownLevel(0) == 0.f);
	Row->Flash(EHudFlaskFlash::DeniedEmpty);
	TestTrue(TEXT("A press with none: red and a shake"), Row->IsFlashing() && Row->GetFlashKind() == EHudFlaskFlash::DeniedEmpty);
	RunFlaskRow(*Row, 0, 0.f, UHudFlaskWidget::DeniedSeconds + 0.1f);
	TestFalse(TEXT("...over soon"), Row->IsFlashing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintFlaskTest, "Looter.Tutorial.Hints.Flask",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintFlaskTest::RunTest(const FString& Parameters)
{
	// The flask's hint: the first time health drops under 40% in a fight with a charge to drink (the sense reports it so);
	// gone, learned, as they drink; once only; weighed before the rest (a player about to die needs it most).
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	FControlHintInput Hurt;
	Hurt.bInControl = true;
	Hurt.bHurtInFight = true;
	Rules.Update(Hurt, 0.1f);
	TestTrue(TEXT("Hurt in a fight: the flask hint"), Rules.GetShown() == EControlHint::Flask);
	TestEqual(TEXT("Its key: Flask"), FControlHintRules::Action(EControlHint::Flask), FName(TEXT("Flask")));
	TestEqual(TEXT("Its words"), FControlHintRules::Words(EControlHint::Flask).ToString(), FString(TEXT("Soul flask")));
	TestTrue(TEXT("About a moment"), FControlHintRules::IsMomentary(EControlHint::Flask));
	TestEqual(TEXT("Once"), FControlHintRules::MaxShows(EControlHint::Flask), 1);
	TestEqual(TEXT("Under 40%"), FControlHintRules::FlaskHealthShare, 0.4f);
	TestTrue(TEXT("Saved by its name"), FControlHintRules::FromId(TEXT("Flask")) == EControlHint::Flask);
	FControlHintInput Drank = Hurt;
	Drank.bDrankFlask = true;
	Rules.Update(Drank, 0.1f);
	TestTrue(TEXT("A drink: gone, learned"), !Rules.IsShowing() && Rules.GetMemory(EControlHint::Flask).bLearned);

	// Unheeded, it fades once the moment passes and never comes again.
	FControlHintRules Once;
	Once.Learn(EControlHint::Move);
	Once.Update(Hurt, 0.1f);
	FControlHintInput Calm;
	Calm.bInControl = true;
	RunHints(Once, Calm, FControlHintRules::LingerSeconds + 0.5f);
	TestFalse(TEXT("The fight over: it fades"), Once.IsShowing());
	RunHints(Once, Hurt, FControlHintRules::RetrySeconds + 5.f);
	TestFalse(TEXT("...and never comes again"), Once.GetShown() == EControlHint::Flask);

	// Before a low magazine's reload: the wound first.
	FControlHintRules Order;
	Order.Learn(EControlHint::Move);
	FControlHintInput Both = Hurt;
	Both.bGunInHand = true;
	Both.bMagazineLow = true;
	Order.Update(Both, 0.1f);
	TestTrue(TEXT("The flask before the reload"), Order.GetShown() == EControlHint::Flask);
	return true;
}

#endif
