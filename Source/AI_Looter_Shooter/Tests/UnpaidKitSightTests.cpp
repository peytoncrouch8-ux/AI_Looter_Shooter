#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightRules.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Creatures/UnpaidRules.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

// The Unpaid's sight and its pack's blinks (the 2026-10-28 play-check; Creatures/UnpaidCreatureSight.cpp): no melee turn or
// lunge through a wall, a turn it can't use let go, a blink to where it sees its target when it's too near to gain ground
// (the chapel's nave), and one phase-step or flank at a player at a time across the pack, an owed press step first.

namespace
{
	/** A test level with a fight director in it (one is made only in played worlds unless a test lets it). */
	struct FSightLevel
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
	struct FSightCVar
	{
		IConsoleVariable* Variable = nullptr;
		FString Was;
		FSightCVar(const TCHAR* Name, const TCHAR* Value)
		{
			Variable = IConsoleManager::Get().FindConsoleVariable(Name);
			if (Variable)
			{
				Was = Variable->GetString();
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}
		~FSightCVar()
		{
			if (Variable)
			{
				Variable->Set(*Was, ECVF_SetByConsole);
			}
		}
	};

	AUnpaidCreature* SpawnUnpaid(UWorld* World, const FVector& Feet)
	{
		return Cast<AUnpaidCreature>(EncounterTestWorld::SpawnCreature(World, AUnpaidCreature::StaticClass(), Feet));
	}

	constexpr float Frame = 1.f / 60.f;

	/** One frame of an Unpaid's own (its movement isn't ticked: it stays where it stands unless it blinks). */
	void TickUnpaid(AUnpaidCreature& Unpaid)
	{
		Unpaid.ConsumeMovementInputVector();
		Unpaid.Tick(Frame);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitSightTest, "Looter.Creatures.Unpaid.Kit.Sight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitSightTest::RunTest(const FString& Parameters)
{
	// The rules: a blind step only when stalled and off its cooldown; its spots round the target, never nearer than its
	// nearest, never a longer blink than its longest step, its own side first.
	const FPhaseStepRules Rules;
	TestTrue(TEXT("Out of sight, stalled, off cooldown: a step to sight"), UnpaidRules::WantsSightStep(Rules, false, Rules.StuckSeconds, Rules.Cooldown));
	TestFalse(TEXT("...not while it sees them"), UnpaidRules::WantsSightStep(Rules, true, Rules.StuckSeconds, Rules.Cooldown));
	TestFalse(TEXT("...not before it has stalled"), UnpaidRules::WantsSightStep(Rules, false, Rules.StuckSeconds - 0.1f, Rules.Cooldown));
	TestFalse(TEXT("...nor within its cooldown"), UnpaidRules::WantsSightStep(Rules, false, Rules.StuckSeconds, Rules.Cooldown - 0.1f));
	const FVector Near(200.0, 0.0, 0.0);
	const FVector Middle(0.0, 0.0, 90.0);
	const TArray<FVector> Spots = UnpaidRules::SightCandidates(Rules, Near, Middle);
	if (TestTrue(TEXT("Spots round them to try"), Spots.Num() >= 6))
	{
		TestTrue(TEXT("Its own side first"), Spots[0].X > 0.0 && FMath::Abs(Spots[0].Y) < 1.0);
	}
	for (const FVector& Spot : Spots)
	{
		const double FromThem = FVector::Dist2D(Spot, Middle);
		if (FromThem < Rules.NearestToTarget - 0.5 || FVector::Dist2D(Spot, Near) > Rules.MaxStep + 0.5 || !FMath::IsNearlyEqual(Spot.Z, Near.Z))
		{
			AddError(FString::Printf(TEXT("%s is %.0f cm from them (at least %.0f), %.0f from it (at most %.0f), or off its height"), *Spot.ToString(),
				FromThem, Rules.NearestToTarget, FVector::Dist2D(Spot, Near), Rules.MaxStep));
		}
	}

	// In a level: a wall 1 m from the player between them and an Unpaid 2 m off (the chapel nave's wall), and one more Unpaid
	// on the player's open side. One melee turn at a time, so the one let go shows.
	FSightLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	UFightDirectorSubsystem* Director = World->GetSubsystem<UFightDirectorSubsystem>();
	FEditorScriptExecutionGuard RunActorEvents;
	FSightCVar KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
	FSightCVar SightOn(TEXT("Looter.Unpaid.Sight"), TEXT("1"));
	FSightCVar BlindOn(TEXT("Looter.Unpaid.BlindStep"), TEXT("1"));
	FSightCVar OneTurn(TEXT("Looter.Fight.MeleeTokens"), TEXT("1"));
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	LocomotionTestWorld::SpawnBlock(World, FVector(100.0, 0.0, 200.0), FVector(20.0, 3000.0, 400.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	AUnpaidCreature* Walled = SpawnUnpaid(World, FVector(200.0, 0.0, 0.0));
	AUnpaidCreature* Open = SpawnUnpaid(World, FVector(-300.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A director"), Director) || !TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Walled-off Unpaid"), Walled)
		|| !TestNotNull(TEXT("Open-side Unpaid"), Open))
	{
		return false;
	}
	TestFalse(TEXT("A wall between: it doesn't see them"), Walled->SeesTarget(*Player, true));
	TestTrue(TEXT("Nothing between: it does"), Open->SeesTarget(*Player, true));

	// Hunting them from behind the wall, in its lunge's reach: it starts no attack, and the turn it takes it lets go again
	// (0.75 s: short of the stall that would let it step).
	Walled->DevPutInState(ECreatureState::Chase, Player);
	Open->DevPutInState(ECreatureState::Chase, Player);
	bool bAttacked = false;
	for (int32 Index = 0; Index < 45; ++Index)
	{
		TickUnpaid(*Walled);
		bAttacked |= Walled->GetCreatureState() == ECreatureState::Attack;
	}
	TestFalse(TEXT("Through the wall: no attack"), bAttacked);
	TestFalse(TEXT("...and it holds no turn"), Director->HoldsToken(*Walled));
	// It took the one turn and let it go, rested (it would have it now otherwise), so the one with a clear way in takes it.
	TestFalse(TEXT("...it let its turn go and rests"), Director->TryTakeToken(*Walled, *Player, EFightRole::Melee));
	TestTrue(TEXT("...so the one with a clear way in takes the turn"), Director->TryTakeToken(*Open, *Player, EFightRole::Melee));

	// Stalled out of sight, too near to step nearer: it blinks to where it sees them (round the wall's end is 15 m off).
	bool bStepped = false;
	for (int32 Index = 0; Index < 300 && !bStepped; ++Index)
	{
		TickUnpaid(*Walled);
		bAttacked |= Walled->GetCreatureState() == ECreatureState::Attack;
		bStepped = Walled->IsPhasing();
	}
	TestFalse(TEXT("Still no attack through the wall"), bAttacked);
	if (TestTrue(TEXT("Out of sight and stalled, it phase-steps"), bStepped))
	{
		for (int32 Index = 0; Index < 120 && Walled->IsPhasing(); ++Index)
		{
			TickUnpaid(*Walled);
		}
		const float Off = static_cast<float>(FVector::Dist2D(Walled->GetActorLocation(), Player->GetActorLocation()));
		TestTrue(TEXT("...back where it sees them"), Walled->SeesTarget(*Player, true));
		TestTrue(FString::Printf(TEXT("...no nearer than a step may bring it (%.0f cm)"), Off), Off >= Walled->PhaseStep.NearestToTarget - 5.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitPackStepTest, "Looter.Creatures.Unpaid.Kit.PackStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitPackStepTest::RunTest(const FString& Parameters)
{
	// The gate's rules on their own clock: one step a gap, an owed one first, its hold running out if it finds nowhere.
	const FUnpaidFightTuning Tuning;
	TestTrue(TEXT("A gap under a second: a pack still presses"), Tuning.PackStepGap > 0.f && Tuning.PackStepGap <= 1.f);
	TestTrue(TEXT("A jitter of a second or two"), Tuning.StepJitter > 0.f && Tuning.StepJitter <= 2.f);
	const float Gap = Tuning.PackStepGap;
	FUnpaidStepGate Gate;
	TestTrue(TEXT("The first may step"), Gate.MayStep(1, false, 0.0, Gap));
	Gate.Note(1, 0.0);
	TestFalse(TEXT("...the next not within the gap"), Gate.MayStep(2, false, Gap - 0.05, Gap));
	TestTrue(TEXT("...but after it"), Gate.MayStep(2, false, Gap + 0.05, Gap));
	Gate.Note(2, Gap + 0.05);
	// An owed one asks every frame while it waits (WantsToStep), so its hold is fresh when the gap is up.
	const double Then = Gap + 0.05;
	TestFalse(TEXT("An owed one waits the gap too"), Gate.MayStep(3, true, Then + Gap - 0.1, Gap));
	TestFalse(TEXT("...but, asking, goes before the rest"), Gate.MayStep(1, false, Then + Gap + 0.01, Gap));
	TestTrue(TEXT("...its turn when the gap is up"), Gate.MayStep(3, true, Then + Gap + 0.02, Gap));
	Gate.Note(3, Then + Gap + 0.02);
	const double Later = Then + Gap + 0.02;
	TestTrue(TEXT("After it, the rest again"), Gate.MayStep(1, false, Later + Gap + 0.01, Gap));
	// One owed that asks and finds nowhere to land (it stops asking a while): its hold runs out and the rest go.
	const double Open = Later + Gap + 0.02;
	TestTrue(TEXT("An owed one may go"), Gate.MayStep(2, true, Open, Gap));
	TestFalse(TEXT("...holding the opening a moment"), Gate.MayStep(1, false, Open + 0.01, Gap));
	TestTrue(TEXT("...then, found nowhere, its hold runs out"), Gate.MayStep(1, false, Open + FUnpaidStepGate::OwedHold + 0.01, Gap));
	TestTrue(TEXT("No gap: anyone, any time"), Gate.MayStep(4, false, Open, 0.f));

	// The step clock jittered at a new target: later by up to the jitter, never earlier, never sooner than a step just taken.
	TestNearlyEqual(TEXT("Rolled 0: as soon as its cooldown allows"), UnpaidFightRules::JitterStepClock(1000.f, 3.f, 1.5f, 0.f), 3.f, 0.001f);
	TestNearlyEqual(TEXT("Rolled 1: the whole jitter later"), UnpaidFightRules::JitterStepClock(1000.f, 3.f, 1.5f, 1.f), 1.5f, 0.001f);
	TestNearlyEqual(TEXT("Just stepped: its clock as it was"), UnpaidFightRules::JitterStepClock(1.f, 3.f, 1.5f, 0.5f), 1.f, 0.001f);

	// The director keeps a gate per player.
	FSightLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	UFightDirectorSubsystem* Director = World->GetSubsystem<UFightDirectorSubsystem>();
	FEditorScriptExecutionGuard RunActorEvents;
	FSightCVar KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
	FSightCVar GapOn(TEXT("Looter.Unpaid.PackStepGap"), *FString::SanitizeFloat(Gap));
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	ACharacter* Other = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 3000.0, 90.0));
	AUnpaidCreature* First = SpawnUnpaid(World, FVector(1800.0, 0.0, 0.0));
	AUnpaidCreature* Second = SpawnUnpaid(World, FVector(-1800.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A director"), Director) || !TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("..."), Other)
		|| !TestNotNull(TEXT("First Unpaid"), First) || !TestNotNull(TEXT("Second Unpaid"), Second))
	{
		return false;
	}
	TestTrue(TEXT("The director: the first may step"), Director->MayStep(*First, *Player, Gap, false));
	Director->NoteStep(*First, *Player);
	TestFalse(TEXT("...the second not at once"), Director->MayStep(*Second, *Player, Gap, false));
	TestTrue(TEXT("...though at another player it may"), Director->MayStep(*Second, *Other, Gap, false));
	World->TimeSeconds += Gap - 0.1;
	TestFalse(TEXT("The second owes a step: it waits the gap too, and asks"), Director->MayStep(*Second, *Player, Gap, true));
	World->TimeSeconds += 0.15;
	TestFalse(TEXT("...the gap up, the first waits for the owed one"), Director->MayStep(*First, *Player, Gap, false));
	TestTrue(TEXT("...which goes"), Director->MayStep(*Second, *Player, Gap, true));
	Director->NoteStep(*Second, *Player);

	// In play: both far behind the player, they blink at least the gap apart (they once went on one frame).
	World->TimeSeconds += 10.0;
	First->DevPutInState(ECreatureState::Chase, Player);
	Second->DevPutInState(ECreatureState::Chase, Player);
	TArray<double> Starts;
	bool bWasPhasing[2] = { false, false };
	for (int32 Index = 0; Index < 240; ++Index)
	{
		World->TimeSeconds += Frame;
		AUnpaidCreature* Pair[2] = { First, Second };
		for (int32 Which = 0; Which < 2; ++Which)
		{
			TickUnpaid(*Pair[Which]);
			const bool bPhasing = Pair[Which]->IsPhasing();
			if (bPhasing && !bWasPhasing[Which])
			{
				Starts.Add(World->TimeSeconds);
			}
			bWasPhasing[Which] = bPhasing;
		}
	}
	TestTrue(FString::Printf(TEXT("Both blink in 4 s (%d steps)"), Starts.Num()), Starts.Num() >= 2);
	Starts.Sort();
	for (int32 Index = 1; Index < Starts.Num(); ++Index)
	{
		const double Apart = Starts[Index] - Starts[Index - 1];
		TestTrue(FString::Printf(TEXT("Steps %d and %d %.2f s apart (the gap %.2f)"), Index, Index + 1, Apart, Gap), Apart >= Gap - 0.5 * Frame);
	}
	return true;
}

#endif
