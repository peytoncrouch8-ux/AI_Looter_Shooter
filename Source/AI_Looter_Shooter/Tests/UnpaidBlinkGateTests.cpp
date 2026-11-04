#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Creatures/UnpaidPackSubsystem.h"
#include "Tests/CreatureTraitTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

// The Unpaid's one blink gate (2026-11-03, the audit's change 11; Creatures/UnpaidPackSubsystemBlinks.cpp, FUnpaidBlinkGate,
// UnpaidBlinkRules): its budget (at most Looter.Unpaid.BlinksPerMinute in any minute at one player), its gap (never two within
// Looter.Unpaid.BlinkGap), an owed press step first, its landing rule (never behind the player's back nearer than 2.5 m; a
// flank within 60-120 degrees of their view, never straight behind), and every blink path through it in a level: the
// phase-step, the phase-strike, the fade-back and the Phasing trait (the flank asks the same MayPackStep and lands by the same
// bearings and rule, checked here as rules: its turn can't be driven alone in a test level).

namespace
{
	/** A test level with a fight director in it (one is made only in played worlds unless a test lets it). */
	struct FBlinkLevel
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

	/** Sets a cvar for one test and puts back what it was (the user may have tuned it in this editor). */
	struct FBlinkCVar
	{
		IConsoleVariable* Variable = nullptr;
		FString Was;
		FBlinkCVar(const TCHAR* Name, const TCHAR* Value)
		{
			Variable = IConsoleManager::Get().FindConsoleVariable(Name);
			if (Variable)
			{
				Was = Variable->GetString();
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}
		~FBlinkCVar()
		{
			if (Variable)
			{
				Variable->Set(*Was, ECVF_SetByConsole);
			}
		}
	};

	constexpr float Frame = 1.f / 60.f;

	AUnpaidCreature* SpawnUnpaid(UWorld* World, const FVector& Feet)
	{
		return Cast<AUnpaidCreature>(EncounterTestWorld::SpawnCreature(World, AUnpaidCreature::StaticClass(), Feet));
	}

	/** One frame: the world's clock on (the gate's), the Unpaid's own tick (its movement isn't ticked: it stays unless it blinks). */
	void TickUnpaid(UWorld& World, AUnpaidCreature& Unpaid)
	{
		World.TimeSeconds += Frame;
		Unpaid.ConsumeMovementInputVector();
		Unpaid.Tick(Frame);
	}

	/** Spends the player's gate: a full minute's budget noted now (other ids), so no blink may go for a minute. */
	void SpendBudget(UUnpaidPackSubsystem& Packs, const APawn& Player)
	{
		const int32 Budget = FMath::Max(FUnpaidFightTuning::Live().BlinksPerMinute, 1);
		for (int32 Index = 0; Index < Budget; ++Index)
		{
			Packs.NoteBlink(Player, 900000u + static_cast<uint32>(Index));
		}
	}

	FVector OnBearing(float Degrees, float Distance)
	{
		const float Radians = FMath::DegreesToRadians(Degrees);
		return FVector(FMath::Cos(Radians) * Distance, FMath::Sin(Radians) * Distance, 0.f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidBlinkGateTest, "Looter.Creatures.Unpaid.Blink.Gate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidBlinkGateTest::RunTest(const FString& Parameters)
{
	// The defaults: the audit's ~50 a minute at one player cut to 15, never two within 2 s.
	const FUnpaidFightTuning Tuning;
	TestEqual(TEXT("15 blinks a minute at a player, across the pack"), Tuning.BlinksPerMinute, 15);
	TestNearlyEqual(TEXT("...never two within 2 s"), Tuning.BlinkGap, 2.f, 0.0001f);
	TestTrue(TEXT("The gap alone would allow more than the budget (the budget is the cap)"), 60.f / Tuning.BlinkGap > Tuning.BlinksPerMinute);

	// The gap.
	const float Gap = Tuning.BlinkGap;
	const int32 Budget = Tuning.BlinksPerMinute;
	FUnpaidBlinkGate Gate;
	TestTrue(TEXT("The first may blink"), Gate.MayBlink(1, false, 0.0, Gap, Budget));
	Gate.Note(1, 0.0);
	TestFalse(TEXT("...the next not within the gap"), Gate.MayBlink(2, false, Gap - 0.05, Gap, Budget));
	TestTrue(TEXT("...but after it"), Gate.MayBlink(2, false, Gap + 0.05, Gap, Budget));

	// The budget: a minute's worth spent, the next waits for the oldest to age out, however long the gap has been.
	FUnpaidBlinkGate Full;
	for (int32 Index = 0; Index < Budget; ++Index)
	{
		Full.Note(static_cast<uint32>(Index + 1), Index * Gap);
	}
	const double Spent = (Budget - 1) * Gap;
	TestEqual(TEXT("A minute's budget noted"), Full.CountInMinute(Spent), Budget);
	TestFalse(TEXT("Spent: no blink, the gap long up"), Full.MayBlink(99, false, Spent + 10.0, Gap, Budget));
	TestTrue(TEXT("...until the first ages out of the minute"), Full.MayBlink(99, false, FUnpaidBlinkGate::Minute + 0.01, Gap, Budget));
	TestTrue(TEXT("No budget (0): the gap alone"), Full.MayBlink(99, false, Spent + Gap + 0.01, Gap, 0));

	// A pack asking every frame for five minutes: never more than the budget in any minute, never two within the gap, and the
	// budget (not the gap) is what holds it.
	FUnpaidBlinkGate Pack;
	TArray<double> Went;
	for (double Clock = 0.0; Clock < 300.0; Clock += Frame)
	{
		const uint32 Asker = 1 + static_cast<uint32>(Went.Num() % 6);
		if (Pack.MayBlink(Asker, false, Clock, Gap, Budget))
		{
			Pack.Note(Asker, Clock);
			Went.Add(Clock);
		}
	}
	int32 Busiest = 0;
	double Closest = 1000.0;
	for (int32 Index = 0; Index < Went.Num(); ++Index)
	{
		int32 InMinute = 0;
		for (int32 Later = Index; Later < Went.Num() && Went[Later] - Went[Index] < FUnpaidBlinkGate::Minute; ++Later)
		{
			++InMinute;
		}
		Busiest = FMath::Max(Busiest, InMinute);
		if (Index > 0)
		{
			Closest = FMath::Min(Closest, Went[Index] - Went[Index - 1]);
		}
	}
	TestTrue(FString::Printf(TEXT("Never more than %d in any minute (busiest %d)"), Budget, Busiest), Busiest <= Budget);
	TestTrue(FString::Printf(TEXT("Never two within %.1f s (closest %.2f)"), Gap, Closest), Closest >= Gap - 0.5 * Frame);
	TestTrue(FString::Printf(TEXT("...and the pack does blink: %d in five minutes"), Went.Num()), Went.Num() >= 5 * Budget - 1 && Went.Num() <= 5 * Budget + 1);

	// An owed press step goes first; its hold runs out if it finds nowhere to land (it stops asking).
	FUnpaidBlinkGate Owed;
	Owed.Note(1, 0.0);
	TestFalse(TEXT("An owed one waits the gap too"), Owed.MayBlink(3, true, Gap - 0.1, Gap, Budget));
	TestFalse(TEXT("...but, asking, goes before the rest"), Owed.MayBlink(1, false, Gap + 0.01, Gap, Budget));
	TestTrue(TEXT("...its turn when the gap is up"), Owed.MayBlink(3, true, Gap + 0.02, Gap, Budget));
	Owed.Note(3, Gap + 0.02);
	const double Open = 2.0 * Gap + 0.05;
	TestTrue(TEXT("Another owed one may go"), Owed.MayBlink(2, true, Open, Gap, Budget));
	TestFalse(TEXT("...holding the opening a moment"), Owed.MayBlink(1, false, Open + 0.01, Gap, Budget));
	TestTrue(TEXT("...then, found nowhere, its hold runs out"), Owed.MayBlink(1, false, Open + FUnpaidBlinkGate::OwedHold + 0.01, Gap, Budget));
	TestTrue(TEXT("No gap, no budget: the gate is off"), Owed.MayBlink(4, false, Open, 0.f, 0));
	TestTrue(TEXT("The director's old ask is the gap alone"), Owed.MayStep(4, false, Open + Gap + 1.0, Gap));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidBlinkLandingTest, "Looter.Creatures.Unpaid.Blink.Landing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidBlinkLandingTest::RunTest(const FString& Parameters)
{
	// A player at the origin looking along +X.
	const FUnpaidBlinkLanding Landing;
	TestNearlyEqual(TEXT("Never behind them nearer than 2.5 m"), Landing.NearestBehind, 250.f, 0.0001f);
	TestTrue(TEXT("A flank's arc: 60 to 120 degrees off their view"), FMath::IsNearlyEqual(Landing.ArcMin, 60.f) && FMath::IsNearlyEqual(Landing.ArcMax, 120.f));
	const FVector Player = FVector::ZeroVector;
	auto May = [&](float Degrees, float Distance, bool bFlank) { return UnpaidBlinkRules::MayLand(Player, 0.f, OnBearing(Degrees, Distance), bFlank, Landing); };
	TestNearlyEqual(TEXT("Straight behind is 180 off their view"), UnpaidBlinkRules::AngleOffView(Player, 0.f, FVector(-100.0, 0.0, 0.0)), 180.f, 0.01f);
	TestNearlyEqual(TEXT("...their side 90 (either way)"), UnpaidBlinkRules::AngleOffView(Player, 0.f, FVector(0.0, -100.0, 0.0)), 90.f, 0.01f);
	TestNearlyEqual(TEXT("...the view's own yaw counts"), UnpaidBlinkRules::AngleOffView(Player, 90.f, FVector(0.0, 100.0, 0.0)), 0.f, 0.01f);

	// Behind their back: never nearer than 2.5 m; in front or at their side, any distance.
	TestFalse(TEXT("Behind them at 2.4 m: never"), May(180.f, 240.f, false));
	TestTrue(TEXT("...at 2.6 m: yes"), May(180.f, 260.f, false));
	TestFalse(TEXT("Behind the shoulder (125 degrees) at 1.7 m: never (the old phase-strike's landing)"), May(125.f, 170.f, false));
	TestTrue(TEXT("At their side (90 degrees) at 1.7 m: yes (the edge of their view)"), May(90.f, 170.f, false));
	TestTrue(TEXT("In front at 1 m: yes"), May(0.f, 100.f, false));

	// A flank: within its arc only, never straight behind (the audit's flanks came back at up to 180 degrees).
	TestFalse(TEXT("A flank at 55 degrees: too near the middle of their view"), May(55.f, 330.f, true));
	TestTrue(TEXT("...at 60"), May(60.f, 330.f, true));
	TestTrue(TEXT("...at 90"), May(-90.f, 330.f, true));
	TestTrue(TEXT("...at 120"), May(120.f, 330.f, true));
	TestFalse(TEXT("...at 130: out of sight"), May(130.f, 330.f, true));
	TestFalse(TEXT("...straight behind: never"), May(180.f, 330.f, true));

	// The flank's bearings: all in the arc, its own side first, the middle first, ten to try; none straight behind.
	const FUnpaidFightTuning Tuning;
	const FUnpaidMoveTuning Moves;
	const TArray<float> Bearings = UnpaidFightRules::FlankBearings(0.f, 20.f, Landing.ArcMin, Landing.ArcMax);
	if (TestEqual(TEXT("Ten spots to try"), Bearings.Num(), 10))
	{
		TestNearlyEqual(TEXT("First: its own side, square to their view"), Bearings[0], 90.f, 0.01f);
		TestNearlyEqual(TEXT("Then the other side"), Bearings[1], -90.f, 0.01f);
		TestNearlyEqual(TEXT("Then toward their front"), Bearings[2], 75.f, 0.01f);
	}
	int32 StrikeSpots = 0;
	for (const float Bearing : Bearings)
	{
		const float Off = FMath::Abs(FMath::UnwindDegrees(Bearing));
		TestTrue(FString::Printf(TEXT("%.0f is in the arc, never behind"), Bearing), Off >= 59.9f && Off <= 120.1f);
		// Every flank spot (3.3 m) passes the rule; a phase-strike's (1.7 m) only at their side or in front of it.
		TestTrue(FString::Printf(TEXT("A flank at %.0f lands"), Bearing), May(Bearing, Tuning.FlankRadius, true));
		const bool bStrike = May(Bearing, Moves.PhaseStrikeLand, true);
		TestTrue(FString::Printf(TEXT("A phase-strike at %.0f lands only at their side or nearer their front"), Bearing), bStrike == (Off <= 90.5f));
		StrikeSpots += bStrike ? 1 : 0;
	}
	TestEqual(TEXT("Six phase-strike spots (60, 75, 90 either side)"), StrikeSpots, 6);
	TestNearlyEqual(TEXT("From their right, the right side first"), UnpaidFightRules::FlankBearings(0.f, -30.f, 60.f, 120.f)[0], -90.f, 0.01f);
	TestEqual(TEXT("No arc: one spot a side"), UnpaidFightRules::FlankBearings(0.f, 20.f, 90.f, 90.f).Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidBlinkPathsTest, "Looter.Creatures.Unpaid.Blink.Paths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidBlinkPathsTest::RunTest(const FString& Parameters)
{
	// Every blink path asks the one gate at its player: shut (a minute's budget spent) none goes; open, each goes and is counted.
	FBlinkLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	UUnpaidPackSubsystem* Packs = World->GetSubsystem<UUnpaidPackSubsystem>();
	FEditorScriptExecutionGuard RunActorEvents;
	FBlinkCVar KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
	FBlinkCVar VarietyOn(TEXT("Looter.Unpaid.Variety"), TEXT("1"));
	CreatureTraitTestWorld::SpawnFloor(World);
	// The player at the origin, looking along +X.
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	if (!TestNotNull(TEXT("The pack subsystem"), Packs) || !TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	const float Gap = FUnpaidFightTuning::Live().BlinkGap;
	auto Shut = [&]()
	{
		World->TimeSeconds += Gap + 0.1;
		SpendBudget(*Packs, *Player);
	};
	auto Reopen = [&]()
	{
		World->TimeSeconds += FUnpaidBlinkGate::Minute + Gap + 0.1;
	};

	// The phase-step: far behind the player (18 m), it wants to step; shut, it doesn't; open, it does, once, counted.
	AUnpaidCreature* Stepper = SpawnUnpaid(World, FVector(-1800.0, 0.0, 0.0));
	if (TestNotNull(TEXT("A phase-stepper"), Stepper))
	{
		Shut();
		Stepper->DevPutInState(ECreatureState::Chase, Player);
		bool bStepped = false;
		for (int32 Index = 0; Index < 240; ++Index)
		{
			TickUnpaid(*World, *Stepper);
			bStepped |= Stepper->IsPhasing();
		}
		TestFalse(TEXT("Phase-step: the gate shut, it doesn't blink (4 s far behind)"), bStepped);
		Reopen();
		const int32 Before = Packs->GetCount(EUnpaidTally::Blink);
		for (int32 Index = 0; Index < 240 && !bStepped; ++Index)
		{
			TickUnpaid(*World, *Stepper);
			bStepped = Stepper->IsPhasing();
		}
		TestTrue(TEXT("...open, it does"), bStepped);
		TestEqual(TEXT("...counted at the gate"), Packs->BlinksInMinute(*Player), 1);
		TestEqual(TEXT("...and in the tally"), Packs->GetCount(EUnpaidTally::Blink), Before + 1);
		for (int32 Index = 0; Index < 90 && Stepper->IsPhasing(); ++Index)
		{
			TickUnpaid(*World, *Stepper);
		}
		const FVector Landed = Stepper->GetActorLocation();
		TestTrue(FString::Printf(TEXT("...landing where the rule lets it (%.0f cm off, %.0f degrees off their view)"), FVector::Dist2D(Landed, Player->GetActorLocation()),
			UnpaidBlinkRules::AngleOffView(Player->GetActorLocation(), 0.f, Landed)), UUnpaidPackSubsystem::MayLandAt(*Player, Landed, false));
		// Out of the fight, and off the pack's board (its blink showed there as a move round the player), so the next pick is its own.
		Stepper->DevPutInState(ECreatureState::Idle, nullptr);
		Packs->Release(Stepper->GetUniqueID());
	}

	// The phase-strike: in front at 4.5 m with only the blink to pick (the lunge's and feint's weights 0); shut, it picks
	// another move; open, it blinks to their side at the edge of their view, counted.
	{
		FBlinkCVar NoLunge(TEXT("Looter.Unpaid.Weight.Lunge"), TEXT("0"));
		FBlinkCVar NoFeint(TEXT("Looter.Unpaid.Weight.Feint"), TEXT("0"));
		AUnpaidCreature* Striker = SpawnUnpaid(World, FVector(450.0, 0.0, 0.0));
		if (TestNotNull(TEXT("A phase-striker"), Striker))
		{
			Shut();
			Striker->DevPutInState(ECreatureState::Chase, Player);
			Striker->DevPutInState(ECreatureState::Attack, Player);
			TestTrue(TEXT("Phase-strike: the gate shut, another move"), Striker->GetKitMove() != EUnpaidMove::PhaseStrike && !Striker->IsPhasing());
			Reopen();
			const int32 Before = Packs->BlinksInMinute(*Player);
			Striker->DevPutInState(ECreatureState::Chase, Player);
			Striker->DevPutInState(ECreatureState::Attack, Player);
			if (TestTrue(TEXT("...open, it blinks to strike"), Striker->GetKitMove() == EUnpaidMove::PhaseStrike && Striker->IsPhasing()))
			{
				TestEqual(TEXT("...counted at the gate"), Packs->BlinksInMinute(*Player), Before + 1);
				for (int32 Index = 0; Index < 90 && Striker->IsPhasing(); ++Index)
				{
					TickUnpaid(*World, *Striker);
				}
				const FVector Landed = Striker->GetActorLocation();
				const float Off = UnpaidBlinkRules::AngleOffView(Player->GetActorLocation(), 0.f, Landed);
				TestTrue(FString::Printf(TEXT("...at the edge of their view (%.0f degrees), never behind them"), Off), Off >= 59.f && Off <= 91.f);
			}
			Striker->DevPutInState(ECreatureState::Idle, nullptr);
			Packs->Release(Striker->GetUniqueID());
		}
	}

	// The fade-back (off by default: one flee rule): with its chance on, shut it stays; open it goes, counted.
	{
		FBlinkCVar FadeOn(TEXT("Looter.Unpaid.FadeBack"), TEXT("0.5"));
		AUnpaidCreature* Hurt = SpawnUnpaid(World, FVector(250.0, 300.0, 0.0));
		if (TestNotNull(TEXT("A hurt one"), Hurt))
		{
			Hurt->SetRank(ECreatureRank::Basic);
			Hurt->DevPutInState(ECreatureState::Chase, Player);
			if (UHealthComponent* Health = Hurt->FindComponentByClass<UHealthComponent>())
			{
				Health->SetHealth(Health->GetMaxHealth() * 0.3f);
			}
			Shut();
			TestFalse(TEXT("Fade-back: the gate shut, it stays"), Hurt->TryFadeBack(false));
			Reopen();
			const int32 Before = Packs->BlinksInMinute(*Player);
			TestTrue(TEXT("...open, it fades back"), Hurt->TryFadeBack(false));
			TestEqual(TEXT("...counted at the gate"), Packs->BlinksInMinute(*Player), Before + 1);
		}
	}

	// The Phasing trait (it asked no gate before): hit hard, shut, it stays; open, it blinks, counted.
	AUnpaidCreature* Ethereal = SpawnUnpaid(World, FVector(0.0, -800.0, 0.0));
	if (TestNotNull(TEXT("A Phasing one"), Ethereal))
	{
		Ethereal->DevPutInState(ECreatureState::Chase, Player);
		UCreatureTraitComponent* Traits = CreatureTraitTestWorld::Give(*Ethereal, { ECreatureTrait::Phasing });
		const float Max = Ethereal->FindComponentByClass<UHealthComponent>()->GetMaxHealth();
		Shut();
		CreatureTraitTestWorld::Hurt(Ethereal, Max * 0.12f);
		Traits->Advance(CreatureTraitTestWorld::Frame);
		TestFalse(TEXT("Phasing: the gate shut, it stays"), Traits->IsBlinking());
		Reopen();
		const int32 Before = Packs->BlinksInMinute(*Player);
		Traits->Advance(CreatureTraitTestWorld::Settings().Phasing.Cooldown + CreatureTraitTestWorld::Frame);
		CreatureTraitTestWorld::Hurt(Ethereal, Max * 0.12f);
		Traits->Advance(CreatureTraitTestWorld::Frame);
		TestTrue(TEXT("...open, it blinks"), Traits->IsBlinking());
		TestEqual(TEXT("...counted at the gate"), Packs->BlinksInMinute(*Player), Before + 1);
	}
	return true;
}

#endif
