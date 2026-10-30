#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Creatures/UnpaidKitWork.h"
#include "Creatures/UnpaidMoveRules.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"

// The Unpaid's varied fight (2026-10-29; Creatures/UnpaidMoveRules, UnpaidCreatureVariety.cpp): the pick's rules (range,
// cooldowns, no third in a row, never a pack-mate's move, pressure paired with a flank, the hang-back), the pack's board, the
// numbers' fairness, a four-Unpaid pack's mix over a long run, and the pick as play makes it.

namespace
{
	/** Sets a cvar for one test and puts back what it was. */
	struct FMoveCVar
	{
		IConsoleVariable* Variable = nullptr;
		FString Was;
		FMoveCVar(const TCHAR* Name, const TCHAR* Value)
		{
			Variable = IConsoleManager::Get().FindConsoleVariable(Name);
			if (Variable)
			{
				Was = Variable->GetString();
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}
		~FMoveCVar()
		{
			if (Variable)
			{
				Variable->Set(*Was, ECVF_SetByConsole);
			}
		}
	};

	/** A test level with a fight director in it. */
	struct FMoveLevel
	{
		FTestWorldWrapper Wrapper;
		bool Create()
		{
			UFightDirectorSubsystem::bAllowInTestWorlds = true;
			const bool bMade = Wrapper.CreateTestWorld(EWorldType::EditorPreview);
			UFightDirectorSubsystem::bAllowInTestWorlds = false;
			return bMade;
		}
	};

	/** A turn on the ring (4.8 m), its pack-mates fighting, nothing on cooldown, none committed. */
	FUnpaidMoveContext OnRing(float Distance = 480.f, int32 Mates = 2)
	{
		FUnpaidMoveContext Context;
		Context.Distance = Distance;
		Context.Mates = Mates;
		return Context;
	}

	float WeightOf(const FUnpaidMoveContext& Context, const FUnpaidMoveTuning& Tuning, EUnpaidMove Move)
	{
		float Weights[NumUnpaidMoves];
		UnpaidMoveRules::Weights(Context, Tuning, Weights);
		return Weights[UnpaidMoveRules::Index(Move)];
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidMovesRulesTest, "Looter.Creatures.Unpaid.Moves.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidMovesRulesTest::RunTest(const FString& Parameters)
{
	const FUnpaidMoveTuning Tuning;
	using M = EUnpaidMove;

	// From the ring (where every turn used to begin as a lunge): every move is open, the close ones by closing in.
	const FUnpaidMoveContext Ring = OnRing();
	TestTrue(TEXT("Ring: the lunge"), WeightOf(Ring, Tuning, M::Lunge) > 0.f);
	TestTrue(TEXT("Ring: the feint"), WeightOf(Ring, Tuning, M::Feint) > 0.f);
	TestTrue(TEXT("Ring: the phase-strike"), WeightOf(Ring, Tuning, M::PhaseStrike) > 0.f);
	TestNearlyEqual(TEXT("Ring: the rake, closing in (its share)"), WeightOf(Ring, Tuning, M::Rake), Tuning.RakeWeight * Tuning.CloseInShare, 0.0001f);
	TestNearlyEqual(TEXT("Ring: the low swipe, closing in"), WeightOf(Ring, Tuning, M::LowSwipe), Tuning.LowSwipeWeight * Tuning.CloseInShare, 0.0001f);
	TestTrue(TEXT("Ring, pack-mates fighting: the hang-back"), WeightOf(Ring, Tuning, M::HangBack) > 0.f);
	TestEqual(TEXT("Alone: no hang-back"), WeightOf(OnRing(480.f, 0), Tuning, M::HangBack), 0.f);

	// Picked as the attack begins: only what begins from here.
	FUnpaidMoveContext Now = OnRing();
	Now.bNow = true;
	TestEqual(TEXT("Now, 4.8 m: no rake"), WeightOf(Now, Tuning, M::Rake), 0.f);
	TestEqual(TEXT("...no low swipe"), WeightOf(Now, Tuning, M::LowSwipe), 0.f);
	TestEqual(TEXT("...no hang-back"), WeightOf(Now, Tuning, M::HangBack), 0.f);
	TestTrue(TEXT("...the lunge"), WeightOf(Now, Tuning, M::Lunge) > 0.f);

	// Range: right on top of them it rakes (no lunge, no feint, no blink, no skim); at 2 m it rakes, lunges or swipes low.
	const FUnpaidMoveContext Close = OnRing(100.f);
	TestTrue(TEXT("1 m: the rake"), WeightOf(Close, Tuning, M::Rake) > 0.f);
	TestEqual(TEXT("1 m: no lunge"), WeightOf(Close, Tuning, M::Lunge), 0.f);
	TestEqual(TEXT("1 m: no feint"), WeightOf(Close, Tuning, M::Feint), 0.f);
	TestEqual(TEXT("1 m: no phase-strike"), WeightOf(Close, Tuning, M::PhaseStrike), 0.f);
	TestEqual(TEXT("1 m: no low swipe"), WeightOf(Close, Tuning, M::LowSwipe), 0.f);
	const FUnpaidMoveContext Near = OnRing(200.f);
	TestNearlyEqual(TEXT("2 m: the rake at its full weight"), WeightOf(Near, Tuning, M::Rake), Tuning.RakeWeight, 0.0001f);
	TestTrue(TEXT("2 m: the low swipe"), WeightOf(Near, Tuning, M::LowSwipe) > 0.f);
	TestEqual(TEXT("2 m: no feint"), WeightOf(Near, Tuning, M::Feint), 0.f);
	TestEqual(TEXT("Past the close-in reach: no rake"), WeightOf(OnRing(Tuning.CloseInRange + 10.f), Tuning, M::Rake), 0.f);

	// Never a third time in a row; a second is fine.
	FUnpaidMoveContext Twice = OnRing();
	Twice.Last = M::Lunge;
	Twice.BeforeLast = M::Lunge;
	TestEqual(TEXT("Two lunges: no third"), WeightOf(Twice, Tuning, M::Lunge), 0.f);
	Twice.BeforeLast = M::Rake;
	TestTrue(TEXT("Lunge after a rake and a lunge: fine"), WeightOf(Twice, Tuning, M::Lunge) > 0.f);

	// Each move's own cooldown; a pack-mate's move is never the same as its own at once (a hang-back may be shared).
	FUnpaidMoveContext Cooling = OnRing();
	Cooling.Cooldowns[UnpaidMoveRules::Index(M::Feint)] = 1.f;
	TestEqual(TEXT("Feint cooling down: none"), WeightOf(Cooling, Tuning, M::Feint), 0.f);
	FUnpaidMoveContext Taken = OnRing();
	Taken.Taken = UnpaidMoveRules::Bit(M::Feint) | UnpaidMoveRules::Bit(M::HangBack);
	TestEqual(TEXT("A pack-mate feints: no feint"), WeightOf(Taken, Tuning, M::Feint), 0.f);
	TestTrue(TEXT("...a hang-back is no attack: still open"), WeightOf(Taken, Tuning, M::HangBack) > 0.f);

	// One pressures, another flanks: a mate in front makes the blink likelier; one at their side, the frontal moves.
	FUnpaidMoveContext Front = OnRing();
	Front.bMateFront = true;
	TestNearlyEqual(TEXT("A mate in front: the phase-strike paired"), WeightOf(Front, Tuning, M::PhaseStrike), Tuning.PhaseStrikeWeight * Tuning.PairBoost, 0.0001f);
	TestTrue(TEXT("...and it may still hang back for the next"), WeightOf(Front, Tuning, M::HangBack) > 0.f);
	FUnpaidMoveContext Side = OnRing();
	Side.bMateSide = true;
	TestNearlyEqual(TEXT("A mate at their side: the lunge presses"), WeightOf(Side, Tuning, M::Lunge), Tuning.LungeWeight * Tuning.PressBoost, 0.0001f);
	TestNearlyEqual(TEXT("...a second blink rare"), WeightOf(Side, Tuning, M::PhaseStrike), Tuning.PhaseStrikeWeight * 0.25f, 0.0001f);
	TestEqual(TEXT("...and no hang-back (the front needs pressing)"), WeightOf(Side, Tuning, M::HangBack), 0.f);
	FUnpaidMoveContext Gated = OnRing();
	Gated.bMayBlink = false;
	TestEqual(TEXT("The pack's blink gate shut: no phase-strike"), WeightOf(Gated, Tuning, M::PhaseStrike), 0.f);

	// Reading the player: after a missed lunge a feint is likelier; a camper isn't baited (it blinks at them instead).
	FUnpaidMoveContext Missed = OnRing();
	Missed.bLastLungeMissed = true;
	TestNearlyEqual(TEXT("After a miss: the feint"), WeightOf(Missed, Tuning, M::Feint), Tuning.FeintWeight * Tuning.MissFeintBoost, 0.0001f);
	FUnpaidMoveContext Camping = OnRing();
	Camping.bCamping = true;
	TestNearlyEqual(TEXT("A camper: few feints"), WeightOf(Camping, Tuning, M::Feint), Tuning.FeintWeight * 0.3f, 0.0001f);
	TestNearlyEqual(TEXT("...more blinks"), WeightOf(Camping, Tuning, M::PhaseStrike), Tuning.PhaseStrikeWeight * 1.5f, 0.0001f);

	// Nothing open: with pack-mates it hangs back; alone it takes any move in range that is no third and no mate's.
	FUnpaidMoveContext Spent = OnRing();
	for (const M Move : { M::Rake, M::Feint, M::PhaseStrike, M::LowSwipe, M::HangBack })
	{
		Spent.Cooldowns[UnpaidMoveRules::Index(Move)] = 2.f;
	}
	Spent.Last = M::Lunge;
	Spent.BeforeLast = M::Lunge;
	TestTrue(TEXT("All spent, mates fighting: it hangs back"), UnpaidMoveRules::Choose(Spent, Tuning, 0.5f) == M::HangBack);
	Spent.Mates = 0;
	const M Alone = UnpaidMoveRules::Choose(Spent, Tuning, 0.5f);
	TestTrue(FString::Printf(TEXT("All spent, alone: an attack, not a third lunge (%s)"), UnpaidMoveRules::Name(Alone)),
		UnpaidMoveRules::IsAttack(Alone) && Alone != M::Lunge);

	// The roll walks the weights in order.
	TestTrue(TEXT("Roll 0: the first open move (the lunge)"), UnpaidMoveRules::Choose(Ring, Tuning, 0.f) == M::Lunge);
	TestTrue(TEXT("Roll 1: the last open one (the hang-back)"), UnpaidMoveRules::Choose(Ring, Tuning, 1.f) == M::HangBack);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidMovesBoardTest, "Looter.Creatures.Unpaid.Moves.Board",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidMovesBoardTest::RunTest(const FString& Parameters)
{
	using M = EUnpaidMove;
	FUnpaidMoveBoard Board;
	Board.Commit(1, M::Lunge, 10.0);
	Board.Commit(2, M::PhaseStrike, 10.5);
	TestEqual(TEXT("Two committed"), Board.Entries.Num(), 2);
	TestTrue(TEXT("1 sees 2's blink"), Board.TakenBy(1) == UnpaidMoveRules::Bit(M::PhaseStrike));
	TestTrue(TEXT("2 sees a mate in front"), Board.AnyFront(2));
	TestFalse(TEXT("...1 sees none in front but itself"), Board.AnyFront(1));
	TestTrue(TEXT("1 sees a mate at their side"), Board.AnySide(1));
	TestTrue(TEXT("Since 10.2: 2 committed after it"), Board.AnyCommittedSince(1, 10.2));
	TestFalse(TEXT("Since 11: nobody yet"), Board.AnyCommittedSince(1, 11.0));
	Board.Commit(1, M::Lunge, 12.0);
	TestFalse(TEXT("The same move again keeps its time"), Board.AnyCommittedSince(2, 11.0));
	Board.Commit(1, M::Rake, 12.0);
	TestTrue(TEXT("A new move is a new commit"), Board.AnyCommittedSince(2, 11.0) && Board.MoveOf(1) == M::Rake);
	Board.Commit(1, M::None, 12.5);
	TestTrue(TEXT("None releases it"), Board.MoveOf(1) == M::None && Board.Entries.Num() == 1);
	Board.Commit(3, M::HangBack, 13.0);
	TestFalse(TEXT("A hang-back is no commit to wait for"), Board.AnyCommittedSince(1, 12.9));
	Board.Prune(17.0, 6.0f);
	TestEqual(TEXT("Stale entries go"), Board.Entries.Num(), 1);
	Board.Release(3);
	TestTrue(TEXT("Released: empty"), Board.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidMovesTuningTest, "Looter.Creatures.Unpaid.Moves.Tuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidMovesTuningTest::RunTest(const FString& Parameters)
{
	// Every move fair: a tell long enough to read (a reaction is about a quarter second), blows no harder than the lunge's.
	const FUnpaidMoveTuning Tuning;
	const FUnpaidFightTuning Kit;
	const AUnpaidCreature* Class = GetDefault<AUnpaidCreature>();
	TestEqual(TEXT("The variety is on"), Tuning.Variety, 1);
	TestTrue(TEXT("The phase-strike's tell: clear (0.35 s or more, back in first)"), Tuning.PhaseStrikeTell >= 0.35f);
	TestTrue(TEXT("The low swipe's crouch: a tell"), Tuning.LowSwipeWindup >= 0.4f);
	TestTrue(TEXT("The claw no harder than a lunge"), Tuning.PhaseStrikeDamageShare <= Kit.LungeDamageShare + 0.1f);
	TestTrue(TEXT("The low swipe no harder than a lunge"), Tuning.LowSwipeDamageShare <= Kit.LungeDamageShare);
	TestTrue(TEXT("A jump clears the low swipe (feet a hand's height up)"), Tuning.LowSwipeClear > 0.f && Tuning.LowSwipeClear <= 45.f);
	TestTrue(TEXT("A feint stops out of a lunge's landing reach"), Tuning.FeintKeep > Kit.LungeHitRadius);
	TestTrue(TEXT("A phase-strike lands inside its claw's reach"), Tuning.PhaseStrikeLand < Tuning.PhaseStrikeReach);
	TestTrue(TEXT("...and outside the player's own body"), Tuning.PhaseStrikeLand > 100.f);
	TestTrue(TEXT("A close move is planned no farther than a turn begins"), Tuning.CloseInRange <= Kit.LungeStart + 100.f);
	TestTrue(TEXT("The lunge is the plain move: the heaviest attack weight but the rake's (close in)"),
		Tuning.LungeWeight >= Tuning.FeintWeight && Tuning.LungeWeight >= Tuning.LowSwipeWeight && Tuning.LungeWeight >= Tuning.PhaseStrikeWeight);
	TestTrue(TEXT("The special moves wait their turn (cooldowns of seconds)"), Tuning.FeintCooldown >= 2.f && Tuning.PhaseStrikeCooldown >= 4.f
		&& Tuning.LowSwipeCooldown >= 2.f);
	TestTrue(TEXT("A hang-back is short"), Tuning.HangBackSeconds <= 3.f && Tuning.HangBackMin < Tuning.HangBackSeconds);
	TestTrue(TEXT("A low swipe's skim is slower than the lunge (readable)"), Tuning.LowSwipeSpeed < Class->LungeSpeed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidMovesMixTest, "Looter.Creatures.Unpaid.Moves.Mix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidMovesMixTest::RunTest(const FString& Parameters)
{
	// Four Unpaid taking turns round a player for a long while (a turn every 0.65 s: each about every 2.6 s, its attack
	// committed 1.3 s), most turns from the ring: every move shows, none dominates, none comes a third time in a row, two
	// never commit the same move at once. The numbers printed are the expected mix (the play-check measures the real one).
	const FUnpaidMoveTuning Tuning;
	using M = EUnpaidMove;
	struct FMate
	{
		M Last = M::None;
		M BeforeLast = M::None;
		float Cooldowns[NumUnpaidMoves] = {};
		M Move = M::None;
		float CommittedUntil = -1.f;
		bool bMissed = false;
	};
	FMate Mates[4];
	FRandomStream Dice(20261029);
	int32 Counts[NumUnpaidMoves] = {};
	int32 Attacks = 0;
	int32 Threes = 0;
	int32 Clashes = 0;
	float LastBlink = -10.f;
	float Clock = 0.f;
	constexpr int32 Turns = 4000;
	constexpr float TurnGap = 0.65f;
	for (int32 Turn = 0; Turn < Turns; ++Turn)
	{
		Clock += TurnGap;
		for (FMate& Each : Mates)
		{
			for (float& Cooldown : Each.Cooldowns)
			{
				Cooldown = FMath::Max(Cooldown - TurnGap, 0.f);
			}
		}
		FMate& Me = Mates[Turn % 4];
		FUnpaidMoveContext Context;
		const float Where = Dice.FRand();
		Context.Distance = Where < 0.75f ? 480.f : (Where < 0.9f ? 330.f : 220.f);
		Context.Last = Me.Last;
		Context.BeforeLast = Me.BeforeLast;
		for (int32 Slot = 0; Slot < NumUnpaidMoves; ++Slot)
		{
			Context.Cooldowns[Slot] = Me.Cooldowns[Slot];
		}
		for (const FMate& Other : Mates)
		{
			if (&Other != &Me && Clock < Other.CommittedUntil)
			{
				Context.Taken |= UnpaidMoveRules::Bit(Other.Move);
				Context.bMateFront |= UnpaidMoveRules::IsFrontal(Other.Move);
				Context.bMateSide |= Other.Move == M::PhaseStrike;
			}
		}
		Context.bMayBlink = Clock - LastBlink >= 1.2f;
		Context.bLastLungeMissed = Me.bMissed;
		Context.Mates = 3;
		const M Move = UnpaidMoveRules::Choose(Context, Tuning, Dice.FRand());
		++Counts[UnpaidMoveRules::Index(Move)];
		if (Move == M::HangBack)
		{
			Me.Cooldowns[UnpaidMoveRules::Index(M::HangBack)] = Tuning.HangBackCooldown;
			continue;
		}
		Clashes += (Context.Taken & UnpaidMoveRules::Bit(Move)) != 0 ? 1 : 0;
		Threes += UnpaidMoveRules::WouldRepeat(Move, Me.Last, Me.BeforeLast) ? 1 : 0;
		++Attacks;
		Me.BeforeLast = Me.Last;
		Me.Last = Move;
		const float Cooldowns[NumUnpaidMoves] = { 0.f, 0.f, Tuning.RakeCooldown, Tuning.FeintCooldown, Tuning.PhaseStrikeCooldown, Tuning.LowSwipeCooldown,
			Tuning.HangBackCooldown };
		Me.Cooldowns[UnpaidMoveRules::Index(Move)] = Cooldowns[UnpaidMoveRules::Index(Move)];
		Me.Move = Move;
		Me.CommittedUntil = Clock + 1.3f;
		Me.bMissed = Move == M::Lunge && Dice.FRand() < 0.7f;
		LastBlink = Move == M::PhaseStrike ? Clock : LastBlink;
	}
	TestEqual(TEXT("Never a third time in a row"), Threes, 0);
	TestEqual(TEXT("Never a pack-mate's move at once"), Clashes, 0);
	FString Mix;
	for (const M Move : { M::Lunge, M::Rake, M::Feint, M::PhaseStrike, M::LowSwipe })
	{
		const float Share = Attacks > 0 ? static_cast<float>(Counts[UnpaidMoveRules::Index(Move)]) / Attacks : 0.f;
		Mix += FString::Printf(TEXT(" %s %.0f%%"), UnpaidMoveRules::Name(Move), 100.f * Share);
		TestTrue(FString::Printf(TEXT("%s shows (%.0f%% of attacks)"), UnpaidMoveRules::Name(Move), 100.f * Share), Share >= 0.08f);
		TestTrue(FString::Printf(TEXT("%s doesn't dominate (%.0f%%)"), UnpaidMoveRules::Name(Move), 100.f * Share), Share <= 0.45f);
	}
	const float Hang = static_cast<float>(Counts[UnpaidMoveRules::Index(M::HangBack)]) / Turns;
	TestTrue(FString::Printf(TEXT("Hang-backs: now and then (%.0f%% of turns)"), 100.f * Hang), Hang > 0.02f && Hang < 0.25f);
	AddInfo(FString::Printf(TEXT("Expected mix of %d attacks:%s; hang-backs %.0f%% of turns."), Attacks, *Mix, 100.f * Hang));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidMovesPlayTest, "Looter.Creatures.Unpaid.Moves.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidMovesPlayTest::RunTest(const FString& Parameters)
{
	// As play picks them: an Unpaid's attacks from 4.5 m vary (the lunge, the feint, a blink to strike), never a third the same
	// in a row; with the variety off, the lunge as before; up close the close moves.
	FMoveLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	FEditorScriptExecutionGuard RunActorEvents;
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(EncounterTestWorld::SpawnCreature(World, AUnpaidCreature::StaticClass(), FVector(450.0, 0.0, 0.0)));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Unpaid spawned"), Unpaid))
	{
		return false;
	}
	FMoveCVar KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
	FMoveCVar VarietyOn(TEXT("Looter.Unpaid.Variety"), TEXT("1"));
	const float Feet = static_cast<float>(Unpaid->GetActorLocation().Z);
	auto AttackFrom = [&](float Distance)
	{
		Unpaid->DevPutInState(ECreatureState::Chase, Player);
		Unpaid->SetActorLocation(FVector(Distance, 0.0, Feet));
		Unpaid->DevPutInState(ECreatureState::Attack, Player);
		const EUnpaidMove Move = Unpaid->GetKitMove();
		// Its clocks on as if a turn's cycle passed (cooldowns), its board kept.
		FUnpaidMoveWork::TickKit(*Unpaid, 3.f, *Player);
		return Move;
	};
	TArray<EUnpaidMove> Picked;
	TSet<int32> Kinds;
	for (int32 Index = 0; Index < 14; ++Index)
	{
		const EUnpaidMove Move = AttackFrom(450.f);
		Picked.Add(Move);
		Kinds.Add(static_cast<int32>(Move));
		TestTrue(FString::Printf(TEXT("4.5 m: a move that begins from there (%s)"), UnpaidMoveRules::Name(Move)),
			Move == EUnpaidMove::Lunge || Move == EUnpaidMove::Feint || Move == EUnpaidMove::PhaseStrike);
	}
	for (int32 Index = 2; Index < Picked.Num(); ++Index)
	{
		TestFalse(FString::Printf(TEXT("Attack %d isn't a third %s in a row"), Index, UnpaidMoveRules::Name(Picked[Index])),
			Picked[Index] == Picked[Index - 1] && Picked[Index] == Picked[Index - 2]);
	}
	TestTrue(FString::Printf(TEXT("More than one kind of attack from 4.5 m (%d)"), Kinds.Num()), Kinds.Num() >= 2);
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const EUnpaidMove Move = AttackFrom(200.f);
		TestTrue(FString::Printf(TEXT("2 m: a close move (%s)"), UnpaidMoveRules::Name(Move)),
			Move == EUnpaidMove::Rake || Move == EUnpaidMove::Lunge || Move == EUnpaidMove::LowSwipe);
	}
	{
		FMoveCVar VarietyOff(TEXT("Looter.Unpaid.Variety"), TEXT("0"));
		TestTrue(TEXT("Variety off: the lunge from 4.5 m, as before"), AttackFrom(450.f) == EUnpaidMove::Lunge);
	}
	return true;
}

#endif
