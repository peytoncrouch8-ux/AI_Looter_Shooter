#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/UnpaidMoveRules.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Creatures/UnpaidOpeningRules.h"

// The Unpaid pack's punish of an opening (2026-10-31; Creatures/UnpaidOpeningRules, UnpaidCreatureOpening.cpp): what counts as
// open, the gate (one punish per player per gap, only in an opening's first moments, once an opening), who may claim it (behind
// the player or at a side, near; right on them, anywhere), and how a turn's pick weighs at an open player (the quick commits up,
// the feint down, no hang-back; the punisher lunges from behind, blinks round from in front).

namespace
{
	/** A turn on the ring (4.8 m), its pack-mates fighting, nothing on cooldown, none committed. */
	FUnpaidMoveContext OnRing(int32 Mates = 2)
	{
		FUnpaidMoveContext Context;
		Context.Distance = 480.f;
		Context.Mates = Mates;
		return Context;
	}

	float WeightOf(const FUnpaidMoveContext& Context, EUnpaidMove Move)
	{
		const FUnpaidMoveTuning Tuning;
		float Weights[NumUnpaidMoves];
		UnpaidMoveRules::Weights(Context, Tuning, Weights);
		return Weights[UnpaidMoveRules::Index(Move)];
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidOpeningRulesTest, "Looter.Creatures.Unpaid.Opening.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidOpeningRulesTest::RunTest(const FString& Parameters)
{
	// What's open: hands busy with the gun or the flask.
	FUnpaidOpeningRead Hands;
	TestFalse(TEXT("Shooting: not open"), Hands.IsOpen());
	Hands.bReloading = true;
	TestTrue(TEXT("Reloading: open"), Hands.IsOpen());
	Hands = FUnpaidOpeningRead();
	Hands.bSwapping = true;
	TestTrue(TEXT("Swapping guns: open"), Hands.IsOpen());
	Hands = FUnpaidOpeningRead();
	Hands.bEmpty = true;
	TestTrue(TEXT("An empty gun: open"), Hands.IsOpen());
	Hands = FUnpaidOpeningRead();
	Hands.bDrinking = true;
	TestTrue(TEXT("The flask at the lips: open"), Hands.IsOpen());

	// The gate: an opening's start makes a punish due; claimed once; due only in its first moments; one per gap.
	constexpr float Gap = 6.f;
	constexpr float Window = 0.6f;
	FUnpaidOpeningGate Gate;
	TestFalse(TEXT("Not open: nothing due"), UnpaidOpeningRules::Step(Gate, false, 1.0, Gap, Window) || Gate.bDue);
	TestTrue(TEXT("A reload begins: reported, a punish due"), UnpaidOpeningRules::Step(Gate, true, 10.0, Gap, Window) && Gate.bDue);
	TestFalse(TEXT("...not reported again while it goes on"), UnpaidOpeningRules::Step(Gate, true, 10.2, Gap, Window));
	TestTrue(TEXT("...the first to ask claims it"), UnpaidOpeningRules::Claim(Gate, 7, 10.2));
	TestFalse(TEXT("...and nobody else"), UnpaidOpeningRules::Claim(Gate, 8, 10.25));
	TestTrue(TEXT("The claimer is the punisher a moment"), UnpaidOpeningRules::IsPunisher(Gate, 7, 11.0, 2.5f));
	TestFalse(TEXT("...not another"), UnpaidOpeningRules::IsPunisher(Gate, 8, 11.0, 2.5f));
	TestFalse(TEXT("...nor once its moment has passed"), UnpaidOpeningRules::IsPunisher(Gate, 7, 13.0, 2.5f));
	UnpaidOpeningRules::Step(Gate, false, 11.5, Gap, Window);
	TestFalse(TEXT("The reload done: shut"), Gate.bOpen || Gate.bDue);
	UnpaidOpeningRules::Step(Gate, true, 13.0, Gap, Window);
	TestFalse(TEXT("Another reload within the gap: nothing due"), Gate.bDue);
	UnpaidOpeningRules::Step(Gate, false, 14.0, Gap, Window);
	UnpaidOpeningRules::Step(Gate, true, 17.0, Gap, Window);
	TestTrue(TEXT("Past the gap: due again"), Gate.bDue);
	UnpaidOpeningRules::Step(Gate, true, 17.7, Gap, Window);
	TestFalse(TEXT("...but only in its first moments: a long reload isn't punished late"), Gate.bDue);
	TestFalse(TEXT("Unclaimed and past its moment: nobody claims it"), UnpaidOpeningRules::Claim(Gate, 9, 17.8));

	// Who may claim it: behind the player or at a side, near; right on them, anywhere; never across the field.
	constexpr float Range = 900.f;
	constexpr float Near = 300.f;
	constexpr float Front = 0.55f;
	TestTrue(TEXT("Behind them, near"), UnpaidOpeningRules::MayClaim(0.f, 600.f, Range, Near, Front));
	TestTrue(TEXT("At their side"), UnpaidOpeningRules::MayClaim(0.5f, 600.f, Range, Near, Front));
	TestFalse(TEXT("In front of their eyes, a way off"), UnpaidOpeningRules::MayClaim(0.95f, 600.f, Range, Near, Front));
	TestTrue(TEXT("Right on them, anywhere"), UnpaidOpeningRules::MayClaim(0.95f, 250.f, Range, Near, Front));
	TestFalse(TEXT("Too far off"), UnpaidOpeningRules::MayClaim(0.f, 1200.f, Range, Near, Front));

	// The turn's pick at an open player: the quick commits up, the feint down, no hang-back.
	const FUnpaidMoveContext Plain = OnRing();
	FUnpaidMoveContext Open = OnRing();
	Open.bOpening = true;
	TestTrue(TEXT("Open: the lunge likelier"), WeightOf(Open, EUnpaidMove::Lunge) > WeightOf(Plain, EUnpaidMove::Lunge));
	TestTrue(TEXT("...the phase-strike likelier"), WeightOf(Open, EUnpaidMove::PhaseStrike) > WeightOf(Plain, EUnpaidMove::PhaseStrike));
	TestTrue(TEXT("...the feint much less"), WeightOf(Open, EUnpaidMove::Feint) < 0.5f * WeightOf(Plain, EUnpaidMove::Feint));
	TestEqual(TEXT("...no hang-back"), WeightOf(Open, EUnpaidMove::HangBack), 0.f);
	TestTrue(TEXT("Not open: the hang-back as before"), WeightOf(Plain, EUnpaidMove::HangBack) > 0.f);
	// The punisher: from behind it lunges, from in front it blinks round to their back.
	FUnpaidMoveContext Behind = Open;
	Behind.bPunisher = true;
	Behind.bInFront = false;
	FUnpaidMoveContext InFront = Open;
	InFront.bPunisher = true;
	InFront.bInFront = true;
	TestTrue(TEXT("The punisher behind them lunges"), WeightOf(Behind, EUnpaidMove::Lunge) > WeightOf(Open, EUnpaidMove::Lunge));
	TestTrue(TEXT("The punisher in front blinks round"), WeightOf(InFront, EUnpaidMove::PhaseStrike) > WeightOf(Open, EUnpaidMove::PhaseStrike));
	// Nothing in range with pack-mates fighting: at an open player it never waits.
	FUnpaidMoveContext Spent = Open;
	for (float& Cooldown : Spent.Cooldowns)
	{
		Cooldown = 5.f;
	}
	TestTrue(TEXT("Every move spent at an open player: it presses anyway"),
		UnpaidMoveRules::Choose(Spent, FUnpaidMoveTuning(), 0.5f) != EUnpaidMove::HangBack);
	FUnpaidMoveContext SpentPlain = Plain;
	for (float& Cooldown : SpentPlain.Cooldowns)
	{
		Cooldown = 5.f;
	}
	TestTrue(TEXT("...where at a player not open it waits for a mate"), UnpaidMoveRules::Choose(SpentPlain, FUnpaidMoveTuning(), 0.5f) == EUnpaidMove::HangBack);
	return true;
}

#endif
