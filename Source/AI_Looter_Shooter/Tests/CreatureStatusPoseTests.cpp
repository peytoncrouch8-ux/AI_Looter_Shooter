#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureStatusPose.h"
#include "Creatures/PlaceholderRules.h"
#include "Elements/ElementRules.h"
#include "Elements/ElementStatus.h"
#include "Elements/ElementTuning.h"
#include "Templates/Function.h"

// The element statuses' poses on creatures (Looter.Creatures.StatusPose; the "Elements matter" job, 2026-10-29): the alphas a
// status asks for, eased in and out (statuses driven by hand on an FElementStatus), the jolt's jerk at its burst, the rot's
// cough, the salt's slow clock, the pose scaled away, a placeholder's clay taking it, and the burn's panic rules (never a
// boss, a Soulfed or a spider). A default-made FElementTuning (the cvars' defaults).

namespace
{
	constexpr float Step = 1.f / 60.f;

	/** Moves the tracker on for Seconds (the status as it is, or moving on with it when bRunStatus). */
	void Run(FCreatureStatusPoseTracker& Tracker, FElementStatus& Status, const FElementTuning& Tuning, float Seconds, bool bRunStatus,
		bool bAlive = true, float* OutMostJerk = nullptr, float* OutMostCough = nullptr)
	{
		for (float Time = 0.f; Time < Seconds - 0.0001f; Time += Step)
		{
			if (bRunStatus)
			{
				TArray<FElementTick> Ticks;
				Status.Advance(Step, Tuning, Ticks);
			}
			Tracker.Advance(Status, Tuning, bAlive, Step);
			if (OutMostJerk)
			{
				*OutMostJerk = FMath::Max(*OutMostJerk, Tracker.Get().JoltJerk);
			}
			if (OutMostCough)
			{
				*OutMostCough = FMath::Max(*OutMostCough, Tracker.Get().Cough);
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureStatusPoseEaseTest, "Looter.Creatures.StatusPose.Ease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureStatusPoseEaseTest::RunTest(const FString& Parameters)
{
	const FElementTuning Tuning;
	{
		FElementStatus Nothing;
		const FCreatureStatusPose Goal = CreatureStatusPose::GoalOf(Nothing, Tuning);
		TestFalse(TEXT("No status asks for nothing"), Goal.IsAny());
	}

	// A burn: in over a few frames, held, out once it has burned out.
	FElementStatus Status;
	Status.Apply(ELooterElement::Ember, 100.f, EElementAffinity::Neutral, /*bBoss*/ false, Tuning);
	FCreatureStatusPoseTracker Tracker;
	Tracker.Advance(Status, Tuning, true, Step);
	const float FirstFrame = Tracker.Get().Burn;
	TestTrue(TEXT("Burning: the flail eases in (some, not all, after a frame)"), FirstFrame > 0.f && FirstFrame < 0.5f);
	Run(Tracker, Status, Tuning, 0.5f, false);
	TestEqual(TEXT("...and holds at a burn's 0.75"), Tracker.Get().Burn, 0.75f, 0.001f);
	TestEqual(TEXT("...the clock as it is"), Tracker.Get().ClockScale, 1.f, 0.001f);
	Status.Apply(ELooterElement::Ember, 100.f, EElementAffinity::Neutral, false, Tuning);
	Status.Apply(ELooterElement::Ember, 100.f, EElementAffinity::Neutral, false, Tuning);
	Run(Tracker, Status, Tuning, 0.5f, false);
	TestEqual(TEXT("Three burns flail harder (1)"), Tracker.Get().Burn, 1.f, 0.001f);
	Run(Tracker, Status, Tuning, Tuning.EmberSeconds + 0.1f, true);
	TestFalse(TEXT("The burns are out"), Status.IsActive());
	FCreatureStatusPoseTracker Fading;
	{
		FElementStatus Lit;
		Lit.Apply(ELooterElement::Ember, 100.f, EElementAffinity::Neutral, false, Tuning);
		Run(Fading, Lit, Tuning, 0.5f, false);
		Lit.Clear();
		Fading.Advance(Lit, Tuning, true, Step);
		const float After = Fading.Get().Burn;
		TestTrue(TEXT("Burned out: a frame later the flail still shows, less"), After > 0.f && After < 0.75f);
		Run(Fading, Lit, Tuning, 0.5f, false);
		TestEqual(TEXT("...and is gone by half a second"), Fading.Get().Burn, 0.f);
		TestTrue(TEXT("...the tracker at rest"), Fading.IsAtRest());
	}

	// Dead: everything eases out.
	{
		FElementStatus Lit;
		Lit.Apply(ELooterElement::Ember, 100.f, EElementAffinity::Neutral, false, Tuning);
		FCreatureStatusPoseTracker Dying;
		Run(Dying, Lit, Tuning, 0.5f, false);
		Run(Dying, Lit, Tuning, 0.5f, false, /*bAlive*/ false);
		TestEqual(TEXT("Dead, its flail is gone though it still burns"), Dying.Get().Burn, 0.f);
	}

	// A smite: a flinch at once, a cower as the light gathers.
	{
		FElementStatus Smitten;
		Smitten.Apply(ELooterElement::SoulLight, 100.f, EElementAffinity::Neutral, false, Tuning);
		FCreatureStatusPoseTracker Flinch;
		Run(Flinch, Smitten, Tuning, 0.2f, true);
		const float Early = Flinch.Get().Smite;
		Run(Flinch, Smitten, Tuning, Tuning.SoulLightDelay * 0.7f, true);
		TestTrue(TEXT("Smitten: a flinch early on"), Early > 0.f);
		TestTrue(TEXT("...growing as the burst nears"), Flinch.Get().Smite > Early);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureStatusPoseKindsTest, "Looter.Creatures.StatusPose.Statuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureStatusPoseKindsTest::RunTest(const FString& Parameters)
{
	const FElementTuning Tuning;

	// Salt: stiff, its clock at SaltClock; scaled away, the clock as it was.
	{
		FElementStatus Salted;
		Salted.Apply(ELooterElement::GraveSalt, 100.f, EElementAffinity::Neutral, false, Tuning);
		FCreatureStatusPoseTracker Tracker;
		Run(Tracker, Salted, Tuning, 1.f, false);
		TestEqual(TEXT("Salted: stiff"), Tracker.Get().Salt, 1.f, 0.001f);
		TestEqual(TEXT("...its pose's clock at 0.6"), Tracker.Get().ClockScale, CreatureStatusPose::SaltClock, 0.001f);
		TestEqual(TEXT("...SaltClock is about 0.6"), CreatureStatusPose::SaltClock, 0.6f, 0.05f);
		const FCreatureStatusPose Hidden = CreatureStatusPose::Scaled(Tracker.Get(), 0.f);
		TestFalse(TEXT("Looter.Element.StatusPose 0 hides it"), Hidden.IsAny());
		TestEqual(TEXT("...and the clock runs as ever"), Hidden.ClockScale, 1.f, 0.001f);
		const FCreatureStatusPose Half = CreatureStatusPose::Scaled(Tracker.Get(), 0.5f);
		TestEqual(TEXT("At half, half the slowing"), Half.ClockScale, 0.8f, 0.001f);
		Salted.Clear();
		Run(Tracker, Salted, Tuning, 0.25f, false);
		TestTrue(TEXT("The salt off: the clock eases back toward 1"), Tracker.Get().ClockScale > CreatureStatusPose::SaltClock
			&& Tracker.Get().ClockScale < 1.f);
		Run(Tracker, Salted, Tuning, 0.5f, false);
		TestEqual(TEXT("...and is 1 again"), Tracker.Get().ClockScale, 1.f, 0.001f);
	}

	// A jolt: trembling while it waits, the jerk at its burst, locked a moment after, then let go.
	{
		FElementStatus Jolted;
		Jolted.Apply(ELooterElement::Thunder, 100.f, EElementAffinity::Neutral, false, Tuning);
		FCreatureStatusPoseTracker Tracker;
		float MostJerk = 0.f;
		Run(Tracker, Jolted, Tuning, Tuning.ThunderDelay * 0.5f, true, true, &MostJerk);
		TestTrue(TEXT("Jolted: locked up and trembling while it waits"), Tracker.Get().Jolt > 0.f);
		TestEqual(TEXT("...no jerk before the burst"), MostJerk, 0.f);
		Run(Tracker, Jolted, Tuning, Tuning.ThunderDelay, true, true, &MostJerk);
		TestEqual(TEXT("The jolt burst"), Jolted.NumJolts(), 0);
		TestTrue(TEXT("...with a hard jerk"), MostJerk > 0.9f);
		TestTrue(TEXT("...the lock-up held through it"), Tracker.Get().Jolt > 0.5f);
		Run(Tracker, Jolted, Tuning, CreatureStatusPose::JoltLockSeconds + 0.6f, true);
		TestEqual(TEXT("...then let go"), Tracker.Get().Jolt, 0.f);
		TestEqual(TEXT("...the jerk over"), Tracker.Get().JoltJerk, 0.f);
		TestTrue(TEXT("...at rest"), Tracker.IsAtRest());

		// A jolt cleared by a death is no burst.
		FElementStatus Cleared;
		Cleared.Apply(ELooterElement::Thunder, 100.f, EElementAffinity::Neutral, false, Tuning);
		FCreatureStatusPoseTracker Dying;
		float DeadJerk = 0.f;
		Run(Dying, Cleared, Tuning, Tuning.ThunderDelay * 0.9f, true, true, &DeadJerk);
		Cleared.Clear();
		Run(Dying, Cleared, Tuning, 0.2f, false, /*bAlive*/ false, &DeadJerk);
		TestEqual(TEXT("A jolt cleared as it dies doesn't jerk"), DeadJerk, 0.f);
	}

	// The rot: hunched, growing, and coughing every few seconds.
	{
		FElementStatus Rotting;
		Rotting.Apply(ELooterElement::Rot, 100.f, EElementAffinity::Neutral, false, Tuning);
		FCreatureStatusPoseTracker Tracker;
		float EarlyCough = 0.f;
		Run(Tracker, Rotting, Tuning, CreatureStatusPose::CoughLead * 0.8f, true, true, nullptr, &EarlyCough);
		TestTrue(TEXT("Rotting: hunched"), Tracker.Get().Rot >= 0.55f);
		TestEqual(TEXT("...no cough at once"), EarlyCough, 0.f);
		const float Hunch = Tracker.Get().Rot;
		float MostCough = 0.f;
		Run(Tracker, Rotting, Tuning, CreatureStatusPose::CoughEvery, true, true, nullptr, &MostCough);
		TestTrue(TEXT("...a cough within its beat"), MostCough > 0.3f);
		TestTrue(TEXT("...the hunch deeper as the rot grows"), Tracker.Get().Rot > Hunch);
		TestEqual(TEXT("A cough's shape: nothing at its ends"), CreatureStatusPose::CoughShape(0.f) + CreatureStatusPose::CoughShape(CreatureStatusPose::CoughSeconds), 0.f);
		TestTrue(TEXT("...a heave in its middle"), CreatureStatusPose::CoughShape(CreatureStatusPose::CoughSeconds * 0.5f) > 0.4f);
	}

	// A placeholder's clay takes it (the pure pose): the rot slumps it lower; a third of it through an attack.
	{
		FPlaceholderPoseInput In;
		In.Clock = 1.f;
		const float Plain = PlaceholderRules::PoseAt(In).Squash;
		In.Status.Rot = 1.f;
		const float Slumped = PlaceholderRules::PoseAt(In).Squash;
		TestTrue(TEXT("A rotting placeholder sits lower"), Slumped < Plain - 0.05f);
		In.bDead = true;
		const float Dead = PlaceholderRules::PoseAt(In).Squash;
		In.Status = FCreatureStatusPose();
		TestEqual(TEXT("...dead, its death's alone"), Dead, PlaceholderRules::PoseAt(In).Squash, 0.0001f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureStatusPosePanicTest, "Looter.Creatures.StatusPose.Panic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureStatusPosePanicTest::RunTest(const FString& Parameters)
{
	using namespace CreatureStatusPose;
	FPanicAsk Ask;
	Ask.bHunting = true;
	TestTrue(TEXT("A Basic creature after a player may panic"), MayPanic(Ask));
	TestTrue(TEXT("...and does when the roll comes in under the chance"), Panics(Ask, 0.1f, 0.3f));
	TestFalse(TEXT("...not over it"), Panics(Ask, 0.5f, 0.3f));
	TestFalse(TEXT("...never at chance 0"), Panics(Ask, 0.f, 0.f));

	auto With = [&Ask](TFunctionRef<void(FPanicAsk&)> Change)
	{
		FPanicAsk Changed = Ask;
		Change(Changed);
		return MayPanic(Changed);
	};
	TestTrue(TEXT("A Restless one may"), With([](FPanicAsk& A) { A.Rank = ECreatureRank::Rare; }));
	TestFalse(TEXT("A Gravebound one doesn't"), With([](FPanicAsk& A) { A.Rank = ECreatureRank::Epic; }));
	TestFalse(TEXT("...nor a Hallowed"), With([](FPanicAsk& A) { A.Rank = ECreatureRank::Hallowed; }));
	TestFalse(TEXT("...nor an Ethereal"), With([](FPanicAsk& A) { A.Rank = ECreatureRank::Ethereal; }));
	TestFalse(TEXT("A Soulfed never"), With([](FPanicAsk& A) { A.Rank = ECreatureRank::Legendary; }));
	TestFalse(TEXT("A boss never"), With([](FPanicAsk& A) { A.Rank = ECreatureRank::Boss; }));
	TestFalse(TEXT("...nor one built round a boss fight"), With([](FPanicAsk& A) { A.bBoss = true; }));
	TestFalse(TEXT("...nor one keeping its own fight"), With([](FPanicAsk& A) { A.bOwnFight = true; }));
	TestFalse(TEXT("A spider never (its fight stays untouched)"), With([](FPanicAsk& A) { A.bSpider = true; }));
	TestFalse(TEXT("One that shrugs fire off doesn't"), With([](FPanicAsk& A) { A.bFireShrugged = true; }));
	TestFalse(TEXT("Not mid-attack"), With([](FPanicAsk& A) { A.bAttacking = true; }));
	TestFalse(TEXT("Not when it hunts nobody"), With([](FPanicAsk& A) { A.bHunting = false; }));
	TestFalse(TEXT("Not held back"), With([](FPanicAsk& A) { A.bPassive = true; }));
	TestFalse(TEXT("Not already running"), With([](FPanicAsk& A) { A.bRunning = true; }));
	TestFalse(TEXT("Not dead"), With([](FPanicAsk& A) { A.bAlive = false; }));
	TestFalse(TEXT("Not on its cooldown"), With([](FPanicAsk& A) { A.CooldownLeft = 1.f; }));

	TestTrue(TEXT("Fire resisted is shrugged off"), ShrugsFire(EElementAffinity::Resist));
	TestTrue(TEXT("...and fire that's nothing to it"), ShrugsFire(EElementAffinity::Immune));
	TestFalse(TEXT("Fire it's weak to isn't"), ShrugsFire(EElementAffinity::Weak));
	TestFalse(TEXT("...nor plain fire"), ShrugsFire(EElementAffinity::Neutral));
	TestEqual(TEXT("A panic runs 1.5 s"), PanicSeconds, 1.5f);
	return true;
}

#endif
