#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Settings/GraphicsSettingsSubsystem.h"
#include "Tutorial/ControlHintRules.h"
#include "Tutorial/ControlHintsSave.h"
#include "Kismet/GameplayStatics.h"

// The contextual control hints' rules (Docs/Polish/TutorialRework.md, "Contextual hints"): each hint's trigger and the
// moment it goes (its control used), one at a time, at most a few times, never for a control already used, and off with
// the setting. FControlHintRules is apart from the world, so these drive it look by look.

namespace
{
	/** A look's worth of the player in control and doing nothing. */
	FControlHintInput InControl()
	{
		FControlHintInput In;
		In.bInControl = true;
		return In;
	}

	/** Seconds of looks a tenth of a second apart, each seeing In. */
	void Run(FControlHintRules& Rules, const FControlHintInput& In, float Seconds)
	{
		constexpr float Look = 0.1f;
		const int32 Looks = FMath::RoundToInt32(Seconds / Look);
		for (int32 Index = 0; Index < Looks; ++Index)
		{
			Rules.Update(In, Look);
		}
	}

	/** It went away because its control was used. */
	bool EndedDone(const FControlHintRules& Rules, EControlHint Hint)
	{
		return !Rules.IsShowing() && Rules.GetLastEnded() == Hint && Rules.GetLastEnd() == EControlHintEnd::Done
			&& Rules.GetMemory(Hint).bLearned;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintMoveTest, "Looter.Tutorial.Hints.Move",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintMoveTest::RunTest(const FString& Parameters)
{
	// Move and look: only for a player who hasn't moved for 6 s after control came; gone as they walk.
	FControlHintRules Rules;
	Run(Rules, InControl(), 5.8f);
	TestFalse(TEXT("Standing still under 6 s: nothing"), Rules.IsShowing());
	Run(Rules, InControl(), 0.4f);
	TestTrue(TEXT("6 s still: the move hint"), Rules.GetShown() == EControlHint::Move);
	TestEqual(TEXT("...its keys: the four movement keys"), FControlHintRules::Action(EControlHint::Move), FName(TEXT("Move")));
	FControlHintInput Walking = InControl();
	Walking.Moved = 200.f;
	Walking.bWalking = true;
	Rules.Update(Walking, 0.1f);
	TestTrue(TEXT("Two metres walked: gone, learned"), EndedDone(Rules, EControlHint::Move));
	Run(Rules, InControl(), 20.f);
	TestFalse(TEXT("Standing still again: never again"), Rules.GetShown() == EControlHint::Move);

	// A player who moves at once never sees it, however long they stand later.
	FControlHintRules Quick;
	Quick.Update(Walking, 0.1f);
	Run(Quick, InControl(), 30.f);
	TestTrue(TEXT("Moved straight away: learned without a hint"), Quick.GetMemory(EControlHint::Move).bLearned
		&& Quick.GetMemory(EControlHint::Move).Shows == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintSprintTest, "Looter.Tutorial.Hints.Sprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintSprintTest::RunTest(const FString& Parameters)
{
	// Sprint: after 8 s of walking without sprinting; gone as they sprint. The key's words follow hold or toggle.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	FControlHintInput Walking = InControl();
	Walking.Moved = 30.f;
	Walking.bWalking = true;
	Run(Rules, Walking, 7.8f);
	TestFalse(TEXT("Under 8 s of walking: nothing"), Rules.IsShowing());
	Run(Rules, Walking, 0.4f);
	TestTrue(TEXT("8 s of walking: the sprint hint"), Rules.GetShown() == EControlHint::Sprint);
	TestTrue(TEXT("Hold or toggle, in its words"), FControlHintRules::Words(EControlHint::Sprint, false).ToString().StartsWith(TEXT("Hold"))
		&& FControlHintRules::Words(EControlHint::Sprint, true).ToString().StartsWith(TEXT("Press")));
	FControlHintInput Sprinting = Walking;
	Sprinting.bWalking = false;
	Sprinting.bSprinting = true;
	Rules.Update(Sprinting, 0.1f);
	TestTrue(TEXT("Sprinting: gone, learned"), EndedDone(Rules, EControlHint::Sprint));

	// Sprinting on their own first: never taught.
	FControlHintRules Runner;
	Runner.Update(Sprinting, 0.1f);
	Run(Runner, Walking, 30.f);
	TestTrue(TEXT("Sprinted unasked: learned without a hint"), Runner.GetMemory(EControlHint::Sprint).bLearned
		&& Runner.GetMemory(EControlHint::Sprint).Shows == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintJumpTest, "Looter.Tutorial.Hints.Jump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintJumpTest::RunTest(const FString& Parameters)
{
	// Jump (and climb, where the mantle is): at a knee-to-chest ledge just ahead, once; gone with a jump or a climb.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	FControlHintInput AtLedge = InControl();
	AtLedge.bLedgeAhead = true;
	Rules.Update(AtLedge, 0.1f);
	TestTrue(TEXT("A ledge ahead: the jump hint"), Rules.GetShown() == EControlHint::Jump);
	FControlHintInput Jumped = AtLedge;
	Jumped.bJumped = true;
	Rules.Update(Jumped, 0.1f);
	TestTrue(TEXT("Jumped: gone, learned"), EndedDone(Rules, EControlHint::Jump));

	// Walked past: it goes once the ledge has been gone a moment, and being "once", it never comes back.
	FControlHintRules Past;
	Past.Learn(EControlHint::Move);
	Past.Update(AtLedge, 0.1f);
	Run(Past, InControl(), FControlHintRules::LingerSeconds - 0.3f);
	TestTrue(TEXT("The ledge just behind: still up a moment"), Past.GetShown() == EControlHint::Jump);
	Run(Past, InControl(), 0.6f);
	TestTrue(TEXT("Gone a while: it fades, unheeded"), !Past.IsShowing() && Past.GetLastEnd() == EControlHintEnd::TimedOut);
	Run(Past, AtLedge, FControlHintRules::RetrySeconds + 5.f);
	TestFalse(TEXT("The next ledge, long after: shown once already, never again"), Past.IsShowing());
	TestEqual(TEXT("Once"), FControlHintRules::MaxShows(EControlHint::Jump), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintAimTest, "Looter.Tutorial.Hints.Aim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintAimTest::RunTest(const FString& Parameters)
{
	// Aim down sights: the first time a target is over 25 m away with a gun in hand; gone as the sights come up.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	FControlHintInput Far = InControl();
	Far.bFarTarget = true;
	Rules.Update(Far, 0.1f);
	TestFalse(TEXT("A far target with no gun in hand: nothing"), Rules.IsShowing());
	Far.bGunInHand = true;
	Rules.Update(Far, 0.1f);
	TestTrue(TEXT("With a gun: the aim hint"), Rules.GetShown() == EControlHint::Aim);
	TestEqual(TEXT("Its key: Aim"), FControlHintRules::Action(EControlHint::Aim), FName(TEXT("Aim")));
	FControlHintInput Aiming = Far;
	Aiming.bAiming = true;
	Rules.Update(Aiming, 0.1f);
	TestTrue(TEXT("Aiming: gone, learned"), EndedDone(Rules, EControlHint::Aim));
	TestTrue(TEXT("25 m"), FMath::IsNearlyEqual(FControlHintRules::FarTargetDistance, 2500.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintReloadTest, "Looter.Tutorial.Hints.Reload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintReloadTest::RunTest(const FString& Parameters)
{
	// Reload: the magazine under a quarter (or empty) with a reserve; gone as the reload starts.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	FControlHintInput Low = InControl();
	Low.bGunInHand = true;
	Low.bMagazineLow = true;
	Rules.Update(Low, 0.1f);
	TestTrue(TEXT("Low with rounds to spare: the reload hint"), Rules.GetShown() == EControlHint::Reload);
	FControlHintInput Reloading = Low;
	Reloading.bReloading = true;
	Rules.Update(Reloading, 0.1f);
	TestTrue(TEXT("Reloading: gone, learned"), EndedDone(Rules, EControlHint::Reload));
	TestTrue(TEXT("A quarter"), FMath::IsNearlyEqual(FControlHintRules::LowMagazineShare, 0.25f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintMeleeTest, "Looter.Tutorial.Hints.Melee",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintMeleeTest::RunTest(const FString& Parameters)
{
	// Melee: a creature close in front before the player's first strike, gun or no gun; gone as they strike.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	FControlHintInput Close = InControl();
	Close.bCloseTarget = true;
	Rules.Update(Close, 0.1f);
	TestTrue(TEXT("A creature close, fists only: the melee hint"), Rules.GetShown() == EControlHint::Melee);
	TestEqual(TEXT("Its key: Melee"), FControlHintRules::Action(EControlHint::Melee), FName(TEXT("Melee")));
	TestTrue(TEXT("A momentary hint: about a creature close, not a habit"), FControlHintRules::IsMomentary(EControlHint::Melee));
	TestEqual(TEXT("Three times in all"), FControlHintRules::MaxShows(EControlHint::Melee), 3);
	FControlHintInput Struck = Close;
	Struck.bMeleed = true;
	Rules.Update(Struck, 0.1f);
	TestTrue(TEXT("Struck: gone, learned"), EndedDone(Rules, EControlHint::Melee));
	Run(Rules, Close, FControlHintRules::RetrySeconds + 5.f);
	TestFalse(TEXT("Another creature close, later: never again"), Rules.IsShowing());

	// Right after Reload, before Aim: the most pressing hint comes first.
	FControlHintRules Order;
	Order.Learn(EControlHint::Move);
	FControlHintInput Busy = Close;
	Busy.bGunInHand = true;
	Busy.bMagazineLow = true;
	Busy.bFarTarget = true;
	Order.Update(Busy, 0.1f);
	TestTrue(TEXT("Reload before melee"), Order.GetShown() == EControlHint::Reload);
	FControlHintInput Reloading = Busy;
	Reloading.bReloading = true;
	Reloading.bMagazineLow = false;
	Order.Update(Reloading, 0.1f);
	FControlHintInput After = Busy;
	After.bMagazineLow = false;
	Run(Order, After, FControlHintRules::GapSeconds + 0.2f);
	TestTrue(TEXT("...then melee before aim"), Order.GetShown() == EControlHint::Melee);
	Order.Update(Struck, 0.1f);
	Run(Order, After, FControlHintRules::GapSeconds + 0.2f);
	TestTrue(TEXT("...then the aim hint"), Order.GetShown() == EControlHint::Aim);

	// The creature walks off: the hint lingers a moment, then fades unheeded, and may come again for the next one.
	FControlHintRules Past;
	Past.Learn(EControlHint::Move);
	Past.Update(Close, 0.1f);
	Run(Past, InControl(), FControlHintRules::LingerSeconds - 0.3f);
	TestTrue(TEXT("The creature just gone: still up a moment"), Past.GetShown() == EControlHint::Melee);
	Run(Past, InControl(), 0.6f);
	TestTrue(TEXT("Gone a while: it fades, unheeded"), !Past.IsShowing() && Past.GetLastEnd() == EControlHintEnd::TimedOut);
	Run(Past, Close, FControlHintRules::RetrySeconds + 1.f);
	TestTrue(TEXT("The next creature, after the wait: back"), Past.GetShown() == EControlHint::Melee);

	// Striking before ever seeing it (a player who knows the game): learned without a hint, and under a menu or with hints
	// off too.
	FControlHintRules Knows;
	Knows.Learn(EControlHint::Move);
	Knows.Update(Struck, 0.1f);
	Run(Knows, Close, 30.f);
	TestTrue(TEXT("Struck unasked: learned without a hint"), Knows.GetMemory(EControlHint::Melee).bLearned
		&& Knows.GetMemory(EControlHint::Melee).Shows == 0);
	TestFalse(TEXT("...and a creature close shows nothing"), Knows.IsShowing());
	FControlHintRules Off;
	Off.Learn(EControlHint::Move);
	FControlHintInput OffStruck = Struck;
	OffStruck.bEnabled = false;
	Off.Update(OffStruck, 0.1f);
	TestTrue(TEXT("Hints off: a strike still learns it"), Off.GetMemory(EControlHint::Melee).bLearned);

	// Not in control (a menu, a scene): no hint for a creature close.
	FControlHintRules Held;
	Held.Learn(EControlHint::Move);
	FControlHintInput Away = Close;
	Away.bInControl = false;
	Run(Held, Away, 5.f);
	TestFalse(TEXT("A creature close under a menu: nothing"), Held.IsShowing());

	// Looter.Hints reset brings it back.
	Knows.ResetMemory();
	TestFalse(TEXT("Reset: forgotten"), Knows.GetMemory(EControlHint::Melee).bLearned);
	Knows.Learn(EControlHint::Move);
	Knows.Update(Close, 0.1f);
	TestTrue(TEXT("...and a creature close teaches it again"), Knows.GetShown() == EControlHint::Melee);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintGrenadeTest, "Looter.Tutorial.Hints.Grenade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintGrenadeTest::RunTest(const FString& Parameters)
{
	// Grenade: a crowd ahead (the sense reports it only with a grenade in hand and none thrown yet); gone as they throw.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	FControlHintInput Crowd = InControl();
	Crowd.bCrowdAhead = true;
	Rules.Update(Crowd, 0.1f);
	TestTrue(TEXT("A crowd ahead: the grenade hint"), Rules.GetShown() == EControlHint::Grenade);
	TestEqual(TEXT("Its key: Grenade"), FControlHintRules::Action(EControlHint::Grenade), FName(TEXT("Grenade")));
	TestEqual(TEXT("Its words"), FControlHintRules::Words(EControlHint::Grenade).ToString(), FString(TEXT("Grenade: salt the crowd")));
	TestTrue(TEXT("A momentary hint: about a crowd, not a habit"), FControlHintRules::IsMomentary(EControlHint::Grenade));
	TestEqual(TEXT("Three times in all"), FControlHintRules::MaxShows(EControlHint::Grenade), 3);
	TestTrue(TEXT("Saved by its name"), FControlHintRules::FromId(FControlHintRules::Id(EControlHint::Grenade)) == EControlHint::Grenade);
	FControlHintInput Threw = Crowd;
	Threw.bThrew = true;
	Rules.Update(Threw, 0.1f);
	TestTrue(TEXT("Thrown: gone, learned"), EndedDone(Rules, EControlHint::Grenade));
	Run(Rules, Crowd, FControlHintRules::RetrySeconds + 5.f);
	TestFalse(TEXT("Another crowd, later: never again"), Rules.IsShowing());

	// A creature close comes first (melee is weighed before the grenade), the crowd after it.
	FControlHintRules Order;
	Order.Learn(EControlHint::Move);
	FControlHintInput Both = Crowd;
	Both.bCloseTarget = true;
	Order.Update(Both, 0.1f);
	TestTrue(TEXT("Melee before the grenade"), Order.GetShown() == EControlHint::Melee);
	FControlHintInput Struck = Both;
	Struck.bMeleed = true;
	Order.Update(Struck, 0.1f);
	Run(Order, Crowd, FControlHintRules::GapSeconds + 0.2f);
	TestTrue(TEXT("...then the grenade"), Order.GetShown() == EControlHint::Grenade);

	// The crowd breaks up: the hint lingers a moment, then fades unheeded, and may come again for the next one.
	FControlHintRules Past;
	Past.Learn(EControlHint::Move);
	Past.Update(Crowd, 0.1f);
	Run(Past, InControl(), FControlHintRules::LingerSeconds - 0.3f);
	TestTrue(TEXT("The crowd just gone: still up a moment"), Past.GetShown() == EControlHint::Grenade);
	Run(Past, InControl(), 0.6f);
	TestTrue(TEXT("Gone a while: it fades, unheeded"), !Past.IsShowing() && Past.GetLastEnd() == EControlHintEnd::TimedOut);
	Run(Past, Crowd, FControlHintRules::RetrySeconds + 1.f);
	TestTrue(TEXT("The next crowd, after the wait: back"), Past.GetShown() == EControlHint::Grenade);

	// Throwing before ever seeing it (a player who knows the game): learned without a hint, hints off or not.
	FControlHintRules Knows;
	Knows.Learn(EControlHint::Move);
	Knows.Update(Threw, 0.1f);
	Run(Knows, Crowd, 30.f);
	TestTrue(TEXT("Thrown unasked: learned without a hint"), Knows.GetMemory(EControlHint::Grenade).bLearned
		&& Knows.GetMemory(EControlHint::Grenade).Shows == 0);
	TestFalse(TEXT("...and a crowd shows nothing"), Knows.IsShowing());
	FControlHintRules Off;
	Off.Learn(EControlHint::Move);
	FControlHintInput OffThrew = Threw;
	OffThrew.bEnabled = false;
	Off.Update(OffThrew, 0.1f);
	TestTrue(TEXT("Hints off: a throw still learns it"), Off.GetMemory(EControlHint::Grenade).bLearned);

	// Not in control (a menu, a scene): no hint for a crowd.
	FControlHintRules Held;
	Held.Learn(EControlHint::Move);
	FControlHintInput Away = Crowd;
	Away.bInControl = false;
	Run(Held, Away, 5.f);
	TestFalse(TEXT("A crowd ahead under a menu: nothing"), Held.IsShowing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintSwapTest, "Looter.Tutorial.Hints.Swap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintSwapTest::RunTest(const FString& Parameters)
{
	// Swap guns: the first time a second gun is carried; gone as the player swaps.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	Rules.Learn(EControlHint::Inventory);
	FControlHintInput One = InControl();
	One.GunsEquipped = 1;
	Run(Rules, One, 2.f);
	TestFalse(TEXT("One gun: nothing"), Rules.IsShowing());
	FControlHintInput Two = One;
	Two.GunsEquipped = 2;
	Rules.Update(Two, 0.1f);
	TestTrue(TEXT("Two guns: the swap hint"), Rules.GetShown() == EControlHint::Swap);
	FControlHintInput Swapped = Two;
	Swapped.bSwapped = true;
	Rules.Update(Swapped, 0.1f);
	TestTrue(TEXT("Swapped: gone, learned"), EndedDone(Rules, EControlHint::Swap));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintInventoryTest, "Looter.Tutorial.Hints.Inventory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintInventoryTest::RunTest(const FString& Parameters)
{
	// The inventory: on the first gun picked up past the first ("compare and equip"), due until shown; gone as it opens,
	// which happens under a menu, out of control.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	Rules.Learn(EControlHint::Swap);
	FControlHintInput Picked = InControl();
	Picked.bPickedUpGun = true;
	Picked.GunsEquipped = 2;
	Rules.Update(Picked, 0.1f);
	TestTrue(TEXT("A second gun picked up: the inventory hint"), Rules.GetShown() == EControlHint::Inventory);
	TestEqual(TEXT("Its words"), FControlHintRules::Words(EControlHint::Inventory).ToString(), FString(TEXT("Inventory: compare and equip")));
	FControlHintInput Open;
	Open.bInventoryOpen = true;
	Rules.Update(Open, 0.1f);
	TestTrue(TEXT("The inventory open (a menu, out of control): gone, learned"), EndedDone(Rules, EControlHint::Inventory));

	// Picked up while another hint shows: it waits its turn, then comes.
	FControlHintRules Busy;
	Busy.Learn(EControlHint::Move);
	Busy.Learn(EControlHint::Swap);
	FControlHintInput Low = InControl();
	Low.bGunInHand = true;
	Low.bMagazineLow = true;
	Low.bPickedUpGun = true;
	Busy.Update(Low, 0.1f);
	TestTrue(TEXT("Reloading first (more pressing)"), Busy.GetShown() == EControlHint::Reload);
	FControlHintInput Reloading = InControl();
	Reloading.bReloading = true;
	Busy.Update(Reloading, 0.1f);
	Run(Busy, InControl(), FControlHintRules::GapSeconds + 0.2f);
	TestTrue(TEXT("...then the pickup's hint, still due"), Busy.GetShown() == EControlHint::Inventory);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintBenchTest, "Looter.Tutorial.Hints.Bench",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintBenchTest::RunTest(const FString& Parameters)
{
	// The bench: near one with two guns of a kind; gone as its screen opens.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	FControlHintInput AtBench = InControl();
	AtBench.bAtBenchWithPair = true;
	Rules.Update(AtBench, 0.1f);
	TestTrue(TEXT("At the bench with a pair: the bench hint"), Rules.GetShown() == EControlHint::Bench);
	TestEqual(TEXT("Its key: Interact"), FControlHintRules::Action(EControlHint::Bench), FName(TEXT("Interact")));
	FControlHintInput Open;
	Open.bBenchOpen = true;
	Rules.Update(Open, 0.1f);
	TestTrue(TEXT("The bench's screen open: gone, learned"), EndedDone(Rules, EControlHint::Bench));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintSlideTest, "Looter.Tutorial.Hints.Slide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintSlideTest::RunTest(const FString& Parameters)
{
	// Slide: after 4 s of sprinting, once; gone as the player slides.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	FControlHintInput Sprinting = InControl();
	Sprinting.bSprinting = true;
	Sprinting.Moved = 60.f;
	Run(Rules, Sprinting, 3.8f);
	TestFalse(TEXT("Under 4 s of sprinting: nothing (sprinting is learned by doing it)"), Rules.IsShowing());
	Run(Rules, Sprinting, 0.4f);
	TestTrue(TEXT("4 s: the slide hint"), Rules.GetShown() == EControlHint::Slide);
	TestEqual(TEXT("Its key: Crouch"), FControlHintRules::Action(EControlHint::Slide), FName(TEXT("Crouch")));
	FControlHintInput Sliding = Sprinting;
	Sliding.bSliding = true;
	Rules.Update(Sliding, 0.1f);
	TestTrue(TEXT("Sliding: gone, learned"), EndedDone(Rules, EControlHint::Slide));
	TestEqual(TEXT("Once"), FControlHintRules::MaxShows(EControlHint::Slide), 1);

	// A sprint broken off starts its clock over.
	FControlHintRules Broken;
	Broken.Learn(EControlHint::Move);
	Run(Broken, Sprinting, 3.f);
	Run(Broken, InControl(), 0.5f);
	Run(Broken, Sprinting, 3.f);
	TestFalse(TEXT("Two short sprints: no slide hint"), Broken.IsShowing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintOneAtATimeTest, "Looter.Tutorial.Hints.OneAtATime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintOneAtATimeTest::RunTest(const FString& Parameters)
{
	// Several due at once: the most pressing shows alone, and the next comes after a moment's quiet.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	Rules.Learn(EControlHint::Inventory);
	FControlHintInput Busy = InControl();
	Busy.bGunInHand = true;
	Busy.bMagazineLow = true;
	Busy.bFarTarget = true;
	Busy.GunsEquipped = 2;
	Rules.Update(Busy, 0.1f);
	TestTrue(TEXT("The reload first"), Rules.GetShown() == EControlHint::Reload);
	Run(Rules, Busy, 1.f);
	TestTrue(TEXT("...and only it"), Rules.GetShown() == EControlHint::Reload);
	FControlHintInput Reloading = Busy;
	Reloading.bReloading = true;
	Reloading.bMagazineLow = false;
	Rules.Update(Reloading, 0.1f);
	TestFalse(TEXT("Reloaded: quiet a moment"), Rules.IsShowing());
	FControlHintInput Next = Busy;
	Next.bMagazineLow = false;
	Run(Rules, Next, FControlHintRules::GapSeconds - 0.3f);
	TestFalse(TEXT("...still quiet"), Rules.IsShowing());
	Run(Rules, Next, 0.5f);
	TestTrue(TEXT("Then the aim hint"), Rules.GetShown() == EControlHint::Aim);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintLimitsTest, "Looter.Tutorial.Hints.Limits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintLimitsTest::RunTest(const FString& Parameters)
{
	// Never blocking: an unheeded hint goes after a few seconds, comes back later, and gives up after its third time.
	FControlHintRules Rules;
	Rules.Learn(EControlHint::Move);
	FControlHintInput TwoGuns = InControl();
	TwoGuns.GunsEquipped = 2;
	Rules.Update(TwoGuns, 0.1f);
	TestTrue(TEXT("Shown"), Rules.GetShown() == EControlHint::Swap);
	Run(Rules, TwoGuns, FControlHintRules::ShowSeconds - 0.4f);
	TestTrue(TEXT("Still up within its time"), Rules.GetShown() == EControlHint::Swap);
	Run(Rules, TwoGuns, 0.6f);
	TestTrue(TEXT("Gone after it, unheeded"), !Rules.IsShowing() && Rules.GetLastEnd() == EControlHintEnd::TimedOut);
	Run(Rules, TwoGuns, FControlHintRules::RetrySeconds - 1.f);
	TestFalse(TEXT("Not back at once"), Rules.IsShowing());
	Run(Rules, TwoGuns, 1.5f);
	TestTrue(TEXT("Back later"), Rules.GetShown() == EControlHint::Swap);
	Run(Rules, TwoGuns, (FControlHintRules::ShowSeconds + FControlHintRules::RetrySeconds) * 3.f);
	TestEqual(TEXT("Three times in all"), Rules.GetMemory(EControlHint::Swap).Shows, FControlHintRules::MaxShows(EControlHint::Swap));
	TestTrue(TEXT("...then never again"), Rules.IsRetired(EControlHint::Swap) && !Rules.IsShowing());

	// Under a menu or a scene nothing moves on: standing still there isn't idling, and the hint on show waits.
	FControlHintRules Held;
	FControlHintInput Away;
	Run(Held, Away, 30.f);
	TestFalse(TEXT("Out of control 30 s: no move hint"), Held.IsShowing());
	Run(Held, InControl(), 6.2f);
	TestTrue(TEXT("In control 6 s: now"), Held.GetShown() == EControlHint::Move);
	Run(Held, Away, 30.f);
	TestTrue(TEXT("A menu over it: it waits"), Held.GetShown() == EControlHint::Move);

	// Hints off: the one on show goes, none come, and controls used meanwhile are still learned.
	FControlHintInput Off = InControl();
	Off.bEnabled = false;
	Held.Update(Off, 0.1f);
	TestTrue(TEXT("Turned off: gone"), !Held.IsShowing() && Held.GetLastEnd() == EControlHintEnd::Hidden);
	FControlHintInput OffLow = Off;
	OffLow.bGunInHand = true;
	OffLow.bMagazineLow = true;
	Run(Held, OffLow, 5.f);
	TestFalse(TEXT("Off: none comes"), Held.IsShowing());
	FControlHintInput OffReloading = Off;
	OffReloading.bReloading = true;
	Held.Update(OffReloading, 0.1f);
	TestTrue(TEXT("Off: a reload still learns it"), Held.GetMemory(EControlHint::Reload).bLearned);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintMemoryTest, "Looter.Tutorial.Hints.Memory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintMemoryTest::RunTest(const FString& Parameters)
{
	// Every hint has a key, words and a name of its own; what's learned survives the profile's save format; the setting
	// starts on.
	TSet<FName> Names;
	for (int32 Index = 0; Index < static_cast<int32>(EControlHint::Count); ++Index)
	{
		const EControlHint Hint = static_cast<EControlHint>(Index);
		const FName Name = FControlHintRules::Id(Hint);
		TestFalse(*FString::Printf(TEXT("%s has a key"), *Name.ToString()), FControlHintRules::Action(Hint).IsNone());
		TestFalse(*FString::Printf(TEXT("%s has words"), *Name.ToString()), FControlHintRules::Words(Hint).IsEmpty());
		TestTrue(*FString::Printf(TEXT("%s's name finds it"), *Name.ToString()), FControlHintRules::FromId(Name) == Hint);
		bool bTaken = false;
		Names.Add(Name, &bTaken);
		TestFalse(*FString::Printf(TEXT("%s's name is its own"), *Name.ToString()), bTaken);
	}
	TestTrue(TEXT("An unknown name finds none"), FControlHintRules::FromId(TEXT("Fly")) == EControlHint::Count);

	ULooterControlHintsSave* Save = NewObject<ULooterControlHintsSave>();
	auto AddRecord = [Save](FName Hint, int32 Shows, bool bLearned)
	{
		FControlHintRecord& Record = Save->Hints.AddDefaulted_GetRef();
		Record.Hint = Hint;
		Record.Shows = Shows;
		Record.bLearned = bLearned;
	};
	AddRecord(FControlHintRules::Id(EControlHint::Sprint), 2, true);
	AddRecord(FControlHintRules::Id(EControlHint::Bench), 1, false);
	// A hint the game no longer has is left out as it's read.
	AddRecord(TEXT("Fly"), 3, true);
	TArray<uint8> Bytes;
	const ULooterControlHintsSave* Read = UGameplayStatics::SaveGameToMemory(Save, Bytes)
		? Cast<ULooterControlHintsSave>(UGameplayStatics::LoadGameFromMemory(Bytes)) : nullptr;
	if (!TestNotNull(TEXT("Saved and read back"), Read))
	{
		return false;
	}
	FControlHintRules Rules;
	for (const FControlHintRecord& Record : Read->Hints)
	{
		const EControlHint Hint = FControlHintRules::FromId(Record.Hint);
		if (Hint != EControlHint::Count)
		{
			FControlHintMemory Memory;
			Memory.Shows = Record.Shows;
			Memory.bLearned = Record.bLearned;
			Rules.SetMemory(Hint, Memory);
		}
	}
	TestTrue(TEXT("Sprint comes back learned"), Rules.GetMemory(EControlHint::Sprint).bLearned && Rules.GetMemory(EControlHint::Sprint).Shows == 2);
	TestTrue(TEXT("The bench's one show comes back"), !Rules.GetMemory(EControlHint::Bench).bLearned && Rules.GetMemory(EControlHint::Bench).Shows == 1);
	FControlHintInput Walking = InControl();
	Walking.bWalking = true;
	Walking.Moved = 30.f;
	Rules.Learn(EControlHint::Move);
	Run(Rules, Walking, 20.f);
	TestFalse(TEXT("Learned in an earlier session: never shown"), Rules.GetShown() == EControlHint::Sprint);
	TestTrue(TEXT("A show or a lesson asks for a save"), Rules.ConsumeChanged());
	TestFalse(TEXT("...once"), Rules.ConsumeChanged());

	TestTrue(TEXT("Control hints start on (and saves from before them load on)"), GetDefault<ULooterGraphicsSave>()->bShowControlHints);
	return true;
}

#endif
