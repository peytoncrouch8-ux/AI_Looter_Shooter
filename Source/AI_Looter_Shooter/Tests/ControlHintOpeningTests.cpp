#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tutorial/ControlHintRules.h"

// The contextual hints as the game's whole tutorial (2026-10-13): a new game starts on Ransom's Rest and the controls are
// taught there from the grave on, Borderlands' way. These are the hints added for it (fire, crouch, interact and hold, the
// Ledger, the Missions page, the map, a grenade picked up) and the rule that no hint comes over a conversation. The first
// ones' rules are Tests/ControlHintTests.cpp's.

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

	/** A player past the first lessons (moving), so a test sees only its own hint. */
	FControlHintRules Walked()
	{
		FControlHintRules Rules;
		Rules.Learn(EControlHint::Move);
		return Rules;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintFireTest, "Looter.Tutorial.Hints.Fire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintFireTest::RunTest(const FString& Parameters)
{
	// Fire: something alive under the crosshair with a gun in hand, before the first shot; gone as the player fires.
	FControlHintRules Rules = Walked();
	FControlHintInput Target = InControl();
	Target.bTargetInSights = true;
	Rules.Update(Target, 0.1f);
	TestFalse(TEXT("A target and no gun: nothing"), Rules.IsShowing());
	Target.bGunInHand = true;
	Rules.Update(Target, 0.1f);
	TestTrue(TEXT("With a gun: the fire hint"), Rules.GetShown() == EControlHint::Fire);
	TestEqual(TEXT("Its key: Fire"), FControlHintRules::Action(EControlHint::Fire), FName(TEXT("Fire")));
	TestTrue(TEXT("About a moment (the target)"), FControlHintRules::IsMomentary(EControlHint::Fire));
	FControlHintInput Fired = Target;
	Fired.bFired = true;
	Rules.Update(Fired, 0.1f);
	TestTrue(TEXT("Fired: gone, learned"), EndedDone(Rules, EControlHint::Fire));
	Run(Rules, Target, FControlHintRules::RetrySeconds + 5.f);
	TestFalse(TEXT("The next target, later: never again"), Rules.IsShowing());

	// A far target: the shot first, the sights after it; a low magazine before either.
	FControlHintRules Order = Walked();
	FControlHintInput Far = Target;
	Far.bFarTarget = true;
	Far.bMagazineLow = true;
	Order.Update(Far, 0.1f);
	TestTrue(TEXT("Reload first"), Order.GetShown() == EControlHint::Reload);
	FControlHintInput Reloading = Far;
	Reloading.bReloading = true;
	Reloading.bMagazineLow = false;
	Order.Update(Reloading, 0.1f);
	Far.bMagazineLow = false;
	Run(Order, Far, FControlHintRules::GapSeconds + 0.2f);
	TestTrue(TEXT("...then fire"), Order.GetShown() == EControlHint::Fire);
	FControlHintInput FarFired = Far;
	FarFired.bFired = true;
	Order.Update(FarFired, 0.1f);
	Run(Order, Far, FControlHintRules::GapSeconds + 0.2f);
	TestTrue(TEXT("...then aim"), Order.GetShown() == EControlHint::Aim);

	// Firing before ever seeing it: learned without a hint.
	FControlHintRules Knows = Walked();
	Knows.Update(Fired, 0.1f);
	TestTrue(TEXT("Fired unasked: learned, never shown"), Knows.GetMemory(EControlHint::Fire).bLearned && Knows.GetMemory(EControlHint::Fire).Shows == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintCrouchTest, "Looter.Tutorial.Hints.Crouch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintCrouchTest::RunTest(const FString& Parameters)
{
	// Crouch: aiming at a target a while, standing (a crouch steadies the aim); gone as the player crouches.
	FControlHintRules Rules = Walked();
	Rules.Learn(EControlHint::Fire);
	FControlHintInput Aiming = InControl();
	Aiming.bGunInHand = true;
	Aiming.bAiming = true;
	Aiming.bTargetInSights = true;
	Run(Rules, Aiming, FControlHintRules::CrouchAimSeconds - 0.2f);
	TestFalse(TEXT("Under its time: nothing"), Rules.IsShowing());
	Run(Rules, Aiming, 0.4f);
	TestTrue(TEXT("Aimed a while standing: the crouch hint"), Rules.GetShown() == EControlHint::Crouch);
	TestEqual(TEXT("Its key: Crouch"), FControlHintRules::Action(EControlHint::Crouch), FName(TEXT("Crouch")));
	TestEqual(TEXT("Twice at most"), FControlHintRules::MaxShows(EControlHint::Crouch), 2);
	FControlHintInput Crouched = Aiming;
	Crouched.bCrouching = true;
	Rules.Update(Crouched, 0.1f);
	TestTrue(TEXT("Crouched: gone, learned"), EndedDone(Rules, EControlHint::Crouch));

	// Aiming at nothing isn't its moment, and the sights let down end it.
	FControlHintRules Empty = Walked();
	FControlHintInput AimingAtNothing = Aiming;
	AimingAtNothing.bTargetInSights = false;
	Run(Empty, AimingAtNothing, 5.f);
	TestFalse(TEXT("Aiming at nothing: no crouch hint"), Empty.GetShown() == EControlHint::Crouch);
	FControlHintRules Lowered = Walked();
	Lowered.Learn(EControlHint::Fire);
	Run(Lowered, Aiming, FControlHintRules::CrouchAimSeconds + 0.2f);
	Run(Lowered, InControl(), FControlHintRules::LingerSeconds + 0.3f);
	TestTrue(TEXT("The sights down: it fades, unheeded"), !Lowered.IsShowing() && Lowered.GetLastEnded() == EControlHint::Crouch
		&& Lowered.GetLastEnd() == EControlHintEnd::TimedOut);

	// A slide is no crouch (its capsule is, its lesson isn't): the sense reports a crouch only out of a slide.
	FControlHintRules Slider = Walked();
	FControlHintInput Sliding = InControl();
	Sliding.bSliding = true;
	Slider.Update(Sliding, 0.1f);
	TestFalse(TEXT("A slide doesn't learn the crouch"), Slider.GetMemory(EControlHint::Crouch).bLearned);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintInteractTest, "Looter.Tutorial.Hints.Interact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintInteractTest::RunTest(const FString& Parameters)
{
	// Interact: something a press uses in front of the player for 2 s (Main 1's headboard), before the first use; gone as
	// anything is used.
	FControlHintRules Rules = Walked();
	FControlHintInput Headboard = InControl();
	Headboard.bTapFocused = true;
	Run(Rules, Headboard, FControlHintRules::InteractDwellSeconds - 0.2f);
	TestFalse(TEXT("Under 2 s: nothing (the prompt is enough for most)"), Rules.IsShowing());
	Run(Rules, Headboard, 0.4f);
	TestTrue(TEXT("2 s looking at it: the interact hint"), Rules.GetShown() == EControlHint::Interact);
	TestEqual(TEXT("Its key: Interact"), FControlHintRules::Action(EControlHint::Interact), FName(TEXT("Interact")));
	FControlHintInput Used = Headboard;
	Used.bInteracted = true;
	Rules.Update(Used, 0.1f);
	TestTrue(TEXT("Used: gone, learned"), EndedDone(Rules, EControlHint::Interact));

	// Something that takes only a hold (the station board, the bell): the hold hint, and holding it teaches both.
	FControlHintRules Bell = Walked();
	FControlHintInput Rope = InControl();
	Rope.bHoldFocused = true;
	Run(Bell, Rope, FControlHintRules::HoldDwellSeconds + 0.2f);
	TestTrue(TEXT("A hold-only thing: the hold hint"), Bell.GetShown() == EControlHint::Hold);
	TestEqual(TEXT("Its key: Interact"), FControlHintRules::Action(EControlHint::Hold), FName(TEXT("Interact")));
	FControlHintInput Held = Rope;
	Held.bInteracted = true;
	Held.bHeldUse = true;
	Bell.Update(Held, 0.1f);
	TestTrue(TEXT("Held its time: gone, learned"), EndedDone(Bell, EControlHint::Hold));
	TestTrue(TEXT("...and pressing with it"), Bell.GetMemory(EControlHint::Interact).bLearned);

	// Loot takes a tap and a hold (a tap picks it up, a hold equips it): pressing is taught first, the hold only after it.
	FControlHintRules Loot = Walked();
	FControlHintInput Gun = InControl();
	Gun.bTapFocused = true;
	Gun.bHoldFocused = true;
	Run(Loot, Gun, FControlHintRules::InteractDwellSeconds + 0.2f);
	TestTrue(TEXT("Loot: the interact hint first"), Loot.GetShown() == EControlHint::Interact);
	FControlHintInput Tapped = Gun;
	Tapped.bInteracted = true;
	Loot.Update(Tapped, 0.1f);
	TestTrue(TEXT("Picked up: interact learned, the hold not"), EndedDone(Loot, EControlHint::Interact)
		&& !Loot.GetMemory(EControlHint::Hold).bLearned);
	Run(Loot, InControl(), FControlHintRules::GapSeconds);
	Run(Loot, Gun, FControlHintRules::HoldDwellSeconds + 0.2f);
	TestTrue(TEXT("The next gun, looked at a while: the hold hint"), Loot.GetShown() == EControlHint::Hold);
	FControlHintRules Fresh = Walked();
	Run(Fresh, Gun, FControlHintRules::HoldDwellSeconds + 0.2f);
	TestTrue(TEXT("Before pressing is learned, loot never shows the hold first"), Fresh.GetShown() == EControlHint::Interact);

	// Looking away: it lingers a moment and goes.
	FControlHintRules Away = Walked();
	Run(Away, Headboard, FControlHintRules::InteractDwellSeconds + 0.2f);
	Run(Away, InControl(), FControlHintRules::LingerSeconds + 0.3f);
	TestTrue(TEXT("Looked away: it fades, unheeded"), !Away.IsShowing() && Away.GetLastEnd() == EControlHintEnd::TimedOut);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintPagesTest, "Looter.Tutorial.Hints.Pages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintPagesTest::RunTest(const FString& Parameters)
{
	// The Ledger: when a mission asks to open it (Main 2's last step); gone as its page opens, under the menu.
	FControlHintRules Rules = Walked();
	FControlHintInput Asked = InControl();
	Asked.bAskedLedger = true;
	Rules.Update(Asked, 0.1f);
	TestTrue(TEXT("Main 2 asks for the Ledger: the Ledger hint"), Rules.GetShown() == EControlHint::Ledger);
	TestEqual(TEXT("Its key: the inventory's"), FControlHintRules::Action(EControlHint::Ledger), FName(TEXT("Inventory")));
	TestTrue(TEXT("...its words say which page"), FControlHintRules::Words(EControlHint::Ledger).ToString().Contains(TEXT("Then 2")));
	FControlHintInput OnLedger;
	OnLedger.bLedgerOpen = true;
	Rules.Update(OnLedger, 0.1f);
	TestTrue(TEXT("The Ledger open (a menu, out of control): gone, learned"), EndedDone(Rules, EControlHint::Ledger));

	// The Missions page: two missions going for 3 s (a choice of which to follow), or a mission asking.
	FControlHintRules Two = Walked();
	FControlHintInput Going = InControl();
	Going.MissionsGoing = 2;
	Run(Two, Going, FControlHintRules::MissionsSettleSeconds - 0.2f);
	TestFalse(TEXT("Two missions, under 3 s (the new one's banner): nothing"), Two.IsShowing());
	Run(Two, Going, 0.4f);
	TestTrue(TEXT("Two missions a while: the Missions hint"), Two.GetShown() == EControlHint::Missions);
	FControlHintInput OnMissions;
	OnMissions.bMissionsOpen = true;
	Two.Update(OnMissions, 0.1f);
	TestTrue(TEXT("The Missions page open: gone, learned"), EndedDone(Two, EControlHint::Missions));
	FControlHintRules One = Walked();
	FControlHintInput Single = InControl();
	Single.MissionsGoing = 1;
	Run(One, Single, 30.f);
	TestFalse(TEXT("One mission: nothing to choose"), One.IsShowing());
	FControlHintInput AskedMissions = InControl();
	AskedMissions.bAskedMissions = true;
	One.Update(AskedMissions, 0.1f);
	TestTrue(TEXT("...unless a mission asks for the page"), One.GetShown() == EControlHint::Missions);

	// The inventory comes when a mission asks for it too, as well as on a second gun picked up.
	FControlHintRules Inventory = Walked();
	FControlHintInput AskedInventory = InControl();
	AskedInventory.bAskedInventory = true;
	Inventory.Update(AskedInventory, 0.1f);
	TestTrue(TEXT("A mission asks for the inventory: the inventory hint"), Inventory.GetShown() == EControlHint::Inventory);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintMapTest, "Looter.Tutorial.Hints.Map",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintMapTest::RunTest(const FString& Parameters)
{
	// The map: 10 s of walking toward a tracked objective over 80 m off (Main 3's farm road into town); gone as it opens.
	FControlHintRules Rules = Walked();
	Rules.Learn(EControlHint::Sprint);
	FControlHintInput Heading = InControl();
	Heading.bWalking = true;
	Heading.Moved = 40.f;
	Heading.bFarWaypoint = true;
	Run(Rules, Heading, FControlHintRules::MapWalkSeconds - 0.2f);
	TestFalse(TEXT("Under 10 s: nothing"), Rules.IsShowing());
	FControlHintInput Stopped = InControl();
	Stopped.bFarWaypoint = true;
	Run(Rules, Stopped, 3.f);
	TestFalse(TEXT("A stop on the way (a fight): the clock waits"), Rules.IsShowing());
	Run(Rules, Heading, 0.4f);
	TestTrue(TEXT("10 s on the way: the map hint"), Rules.GetShown() == EControlHint::Map);
	TestEqual(TEXT("Its key: Map"), FControlHintRules::Action(EControlHint::Map), FName(TEXT("Map")));
	FControlHintInput OnMap;
	OnMap.bMapOpen = true;
	Rules.Update(OnMap, 0.1f);
	TestTrue(TEXT("The map open: gone, learned"), EndedDone(Rules, EControlHint::Map));

	// An objective near at hand never asks for the map, and reaching near one starts the clock over.
	FControlHintRules Near = Walked();
	Near.Learn(EControlHint::Sprint);
	FControlHintInput Close = Heading;
	Close.bFarWaypoint = false;
	Run(Near, Close, 30.f);
	TestFalse(TEXT("Walking to something near: no map hint"), Near.IsShowing());
	Run(Near, Heading, FControlHintRules::MapWalkSeconds - 1.f);
	Run(Near, Close, 0.2f);
	Run(Near, Heading, 2.f);
	TestFalse(TEXT("...and coming near one starts it over"), Near.IsShowing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintGrenadePickupTest, "Looter.Tutorial.Hints.GrenadePickup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintGrenadePickupTest::RunTest(const FString& Parameters)
{
	// Grenades picked up before the first throw: the grenade hint once for it, crowd or no crowd.
	FControlHintRules Rules = Walked();
	FControlHintInput Picked = InControl();
	Picked.bGrenadesGained = true;
	Rules.Update(Picked, 0.1f);
	TestTrue(TEXT("Grenades picked up: the grenade hint"), Rules.GetShown() == EControlHint::Grenade);
	Run(Rules, InControl(), FControlHintRules::ShowSeconds - 0.5f);
	TestTrue(TEXT("...up its full time (no moment to pass)"), Rules.GetShown() == EControlHint::Grenade);
	Run(Rules, InControl(), 1.f);
	TestTrue(TEXT("...then it fades, unheeded"), !Rules.IsShowing() && Rules.GetLastEnd() == EControlHintEnd::TimedOut);
	Run(Rules, InControl(), FControlHintRules::RetrySeconds + 5.f);
	TestFalse(TEXT("The pickup has had its show: it doesn't come back by itself"), Rules.IsShowing());
	FControlHintInput Crowd = InControl();
	Crowd.bCrowdAhead = true;
	Rules.Update(Crowd, 0.1f);
	TestTrue(TEXT("...a crowd brings it back"), Rules.GetShown() == EControlHint::Grenade);
	FControlHintInput Threw = Crowd;
	Threw.bThrew = true;
	Rules.Update(Threw, 0.1f);
	TestTrue(TEXT("Thrown: gone, learned"), EndedDone(Rules, EControlHint::Grenade));

	// Thrown already: a pickup asks nothing.
	FControlHintRules Knows = Walked();
	Knows.Learn(EControlHint::Grenade);
	Knows.Update(Picked, 0.1f);
	TestFalse(TEXT("Learned: a pickup shows nothing"), Knows.IsShowing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintCaptionsTest, "Looter.Tutorial.Hints.Captions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintCaptionsTest::RunTest(const FString& Parameters)
{
	// Never over a conversation: while a caption is on screen no hint comes, and the one on show waits it out.
	FControlHintRules Rules = Walked();
	FControlHintInput Talking = InControl();
	Talking.bCaptionsOn = true;
	Talking.bAskedLedger = true;
	Run(Rules, Talking, 10.f);
	TestFalse(TEXT("Sexton talking: no Ledger hint yet"), Rules.IsShowing());
	FControlHintInput Quiet = Talking;
	Quiet.bCaptionsOn = false;
	Rules.Update(Quiet, 0.1f);
	TestTrue(TEXT("He's done: the Ledger hint"), Rules.GetShown() == EControlHint::Ledger);

	// A hint on show when someone starts talking: it waits, its time stopped, and carries on after.
	Run(Rules, Quiet, 3.f);
	const float Before = Rules.GetShownSeconds();
	Run(Rules, Talking, 20.f);
	TestTrue(TEXT("Under the talk: still the Ledger's, its time stopped"), Rules.GetShown() == EControlHint::Ledger
		&& FMath::IsNearlyEqual(Rules.GetShownSeconds(), Before, 0.01f));
	Run(Rules, Quiet, FControlHintRules::ShowSeconds - Before - 0.5f);
	TestTrue(TEXT("After it: the rest of its time"), Rules.GetShown() == EControlHint::Ledger);

	// Controls used during a talk are learned all the same.
	FControlHintRules Learner = Walked();
	FControlHintInput Sprinting = Talking;
	Sprinting.bSprinting = true;
	Learner.Update(Sprinting, 0.1f);
	TestTrue(TEXT("Sprinting through a talk: learned"), Learner.GetMemory(EControlHint::Sprint).bLearned);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FControlHintCoverageTest, "Looter.Tutorial.Hints.Coverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FControlHintCoverageTest::RunTest(const FString& Parameters)
{
	// Every control the opening teaches has its hint, with a key and words (the user's list, 2026-10-13): look and move,
	// jump (and the mantle), sprint, crouch, slide, aim, fire, reload, interact and hold, swap guns, melee, grenade, the
	// inventory and the Ledger, the map, the Missions page.
	const TCHAR* const Taught[] = { TEXT("Move"), TEXT("Jump"), TEXT("Sprint"), TEXT("Crouch"), TEXT("Slide"), TEXT("Aim"), TEXT("Fire"),
		TEXT("Reload"), TEXT("Interact"), TEXT("Hold"), TEXT("Swap"), TEXT("Melee"), TEXT("Grenade"), TEXT("Inventory"), TEXT("Ledger"),
		TEXT("Map"), TEXT("Missions") };
	for (const TCHAR* Name : Taught)
	{
		const EControlHint Hint = FControlHintRules::FromId(Name);
		if (TestTrue(FString::Printf(TEXT("%s has a hint"), Name), Hint != EControlHint::Count))
		{
			TestFalse(FString::Printf(TEXT("%s: a key"), Name), FControlHintRules::Action(Hint).IsNone());
			TestFalse(FString::Printf(TEXT("%s: words"), Name), FControlHintRules::Words(Hint).IsEmpty());
		}
	}
	// The names the profile saved before keep finding their hints (never renamed).
	const TCHAR* const Saved[] = { TEXT("Move"), TEXT("Reload"), TEXT("Melee"), TEXT("Grenade"), TEXT("Aim"), TEXT("Jump"), TEXT("Sprint"),
		TEXT("Slide"), TEXT("Swap"), TEXT("Inventory"), TEXT("Bench") };
	for (const TCHAR* Old : Saved)
	{
		TestTrue(FString::Printf(TEXT("The saved name %s still finds its hint"), Old), FControlHintRules::FromId(Old) != EControlHint::Count);
	}
	return true;
}

#endif
