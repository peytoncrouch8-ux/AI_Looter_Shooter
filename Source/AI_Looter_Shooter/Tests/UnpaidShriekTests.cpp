#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidKitWork.h"
#include "Creatures/UnpaidMoveRules.h"
#include "Creatures/UnpaidMoveTuning.h"
#include "Player/PlayerRattleSubsystem.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"

// The Unpaid's failed-lunge shriek (2026-10-29; Creatures/UnpaidCreatureShriek.cpp): its chance and cooldowns, its phases,
// its interrupt (the coal shot in the tell, never once it screams), its reach on the player (a rattle, never a stun), its
// rally of the pack; and in play: a lunge that misses turns into a shriek, one of a pack at a time.

namespace
{
	struct FShriekCVar
	{
		IConsoleVariable* Variable = nullptr;
		FString Was;
		FShriekCVar(const TCHAR* Name, const TCHAR* Value)
		{
			Variable = IConsoleManager::Get().FindConsoleVariable(Name);
			if (Variable)
			{
				Was = Variable->GetString();
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}
		~FShriekCVar()
		{
			if (Variable)
			{
				Variable->Set(*Was, ECVF_SetByConsole);
			}
		}
	};

	struct FShriekLevel
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

	constexpr float Step = 1.f / 60.f;

	void TickFor(AUnpaidCreature& Unpaid, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds; Time += Step)
		{
			Unpaid.ConsumeMovementInputVector();
			Unpaid.Tick(Step);
		}
	}

	AUnpaidCreature* SpawnUnpaid(UWorld* World, const FVector& Feet, FName Pack)
	{
		AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(EncounterTestWorld::SpawnCreature(World, AUnpaidCreature::StaticClass(), Feet));
		if (Unpaid && !Pack.IsNone())
		{
			Unpaid->PackTag = Pack;
		}
		return Unpaid;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidShriekRulesTest, "Looter.Creatures.Unpaid.Shriek.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidShriekRulesTest::RunTest(const FString& Parameters)
{
	const FUnpaidMoveTuning Tuning;
	using UnpaidMoveRules::ShriekOnMiss;

	// The chance, its own cooldown and its pack's gap.
	TestTrue(TEXT("A low roll, both clocks run out: it shrieks"), ShriekOnMiss(0.1f, 0.35f, 9.f, 8.f, 5.f, 4.f));
	TestFalse(TEXT("A high roll: it doesn't"), ShriekOnMiss(0.5f, 0.35f, 9.f, 8.f, 5.f, 4.f));
	TestFalse(TEXT("Its own shriek 7.9 s ago (8 s cooldown): no"), ShriekOnMiss(0.1f, 0.35f, 7.9f, 8.f, 5.f, 4.f));
	TestFalse(TEXT("Its pack's 3.9 s ago (4 s gap): no"), ShriekOnMiss(0.1f, 0.35f, 9.f, 8.f, 3.9f, 4.f));
	TestFalse(TEXT("Chance 0: never"), ShriekOnMiss(0.f, 0.f, 100.f, 8.f, 100.f, 4.f));
	FRandomStream Dice(1029);
	int32 Shrieks = 0;
	constexpr int32 Misses = 20000;
	for (int32 Index = 0; Index < Misses; ++Index)
	{
		Shrieks += ShriekOnMiss(Dice.FRand(), Tuning.ShriekChance, 1000.f, Tuning.ShriekCooldown, 1000.f, Tuning.ShriekPackGap) ? 1 : 0;
	}
	const float Share = static_cast<float>(Shrieks) / Misses;
	TestTrue(FString::Printf(TEXT("About a third of misses shriek (%.1f%%)"), 100.f * Share), FMath::Abs(Share - Tuning.ShriekChance) < 0.02f);

	// Its phases: the tell, the scream, the recovery, then over.
	const float Tell = Tuning.ShriekTell;
	const float Scream = Tuning.ShriekScream;
	const float Recover = Tuning.ShriekRecover;
	TestTrue(TEXT("0.25 s: the tell"), UnpaidMoveRules::ShriekPhaseAt(0.25f, Tell, Scream, Recover) == EUnpaidShriekPhase::Tell);
	TestTrue(TEXT("0.6 s: the scream"), UnpaidMoveRules::ShriekPhaseAt(Tell + 0.1f, Tell, Scream, Recover) == EUnpaidShriekPhase::Scream);
	TestTrue(TEXT("Into the recovery"), UnpaidMoveRules::ShriekPhaseAt(Tell + Scream + 0.1f, Tell, Scream, Recover) == EUnpaidShriekPhase::Recover);
	TestTrue(TEXT("Past its end: over"), UnpaidMoveRules::ShriekPhaseAt(Tell + Scream + Recover + 0.01f, Tell, Scream, Recover) == EUnpaidShriekPhase::None);

	// The interrupt: only its coal, only in the tell.
	TestTrue(TEXT("Coal shot in the tell: interrupted"), UnpaidMoveRules::InterruptsShriek(EUnpaidShriekPhase::Tell, true));
	TestFalse(TEXT("A body hit in the tell: not"), UnpaidMoveRules::InterruptsShriek(EUnpaidShriekPhase::Tell, false));
	TestFalse(TEXT("Coal shot once it screams: committed"), UnpaidMoveRules::InterruptsShriek(EUnpaidShriekPhase::Scream, true));
	TestFalse(TEXT("...or as it recovers"), UnpaidMoveRules::InterruptsShriek(EUnpaidShriekPhase::Recover, true));

	// Its reach on the player: full close by, falling to a third at its edge, nothing past.
	const float Radius = Tuning.ShriekRattleRadius;
	TestNearlyEqual(TEXT("Close by: full"), UnpaidMoveRules::RattleShare(150.f, Radius), 1.f, 0.001f);
	TestNearlyEqual(TEXT("At its edge: a third"), UnpaidMoveRules::RattleShare(Radius, Radius), 1.f / 3.f, 0.01f);
	TestEqual(TEXT("Past it: nothing"), UnpaidMoveRules::RattleShare(Radius + 1.f, Radius), 0.f);
	TestTrue(TEXT("Nearer is harder"), UnpaidMoveRules::RattleShare(300.f, Radius) > UnpaidMoveRules::RattleShare(450.f, Radius));

	// The low swipe passes under a jump.
	TestTrue(TEXT("Feet 40 cm up: cleared"), UnpaidMoveRules::ClearsLowSwipe(40.f, false, 30.f));
	TestFalse(TEXT("Standing: caught"), UnpaidMoveRules::ClearsLowSwipe(0.f, false, 30.f));
	TestTrue(TEXT("In the air a little: cleared"), UnpaidMoveRules::ClearsLowSwipe(12.f, true, 30.f));
	TestFalse(TEXT("Barely off the ground: caught"), UnpaidMoveRules::ClearsLowSwipe(5.f, true, 30.f));

	// Its numbers: the brief's timings, punishing (not a stun), not spammable.
	TestNearlyEqual(TEXT("A half-second tell"), Tuning.ShriekTell, 0.5f, 0.1f);
	TestNearlyEqual(TEXT("A scream of about 1.2 s"), Tuning.ShriekScream, 1.2f, 0.2f);
	TestNearlyEqual(TEXT("A recovery of about 0.6 s"), Tuning.ShriekRecover, 0.6f, 0.15f);
	TestNearlyEqual(TEXT("About a third of misses"), Tuning.ShriekChance, 0.35f, 0.05f);
	TestTrue(TEXT("Its own cooldown longer than its pack's gap"), Tuning.ShriekCooldown > Tuning.ShriekPackGap && Tuning.ShriekPackGap > 0.f);
	TestTrue(TEXT("A slight slow, never a stun"), Tuning.ShriekSlow >= 0.75f && Tuning.ShriekSlow < 1.f && Tuning.ShriekSlowSeconds <= 1.5f);
	TestTrue(TEXT("A short sway of a degree or two"), Tuning.ShriekSway > 0.f && Tuning.ShriekSway <= 2.f && Tuning.ShriekSwaySeconds <= 1.2f);
	TestTrue(TEXT("Rattled within about 6 m, felt farther"), FMath::IsNearlyEqual(Tuning.ShriekRattleRadius, 600.f, 100.f)
		&& Tuning.ShriekFeltRadius > Tuning.ShriekRattleRadius);
	TestTrue(TEXT("It rallies within about 12 m"), FMath::IsNearlyEqual(Tuning.ShriekRallyRadius, 1200.f, 200.f));
	TestTrue(TEXT("The sway and slow come at most once a gap"), Tuning.ShriekRattleGap >= 2.f);
	TestTrue(TEXT("The interrupt's coal is bigger, a while"), Tuning.ShriekExposedCoal > 1.f && Tuning.ShriekExposedSeconds >= 1.f);

	// The player's rattle: no sway at either end (the aim comes back), its frost rising and thawing.
	TestTrue(TEXT("The sway starts at zero"), UPlayerRattleSubsystem::SwayAt(0.f, 0.9f, 1.f, 0.3f).IsNearlyZero(0.001f));
	TestTrue(TEXT("...and ends at zero"), UPlayerRattleSubsystem::SwayAt(0.9f, 0.9f, 1.f, 0.3f).IsNearlyZero(0.001f));
	TestTrue(TEXT("...within its degrees"), FMath::Abs(UPlayerRattleSubsystem::SwayAt(0.3f, 0.9f, 1.f, 0.3f).Yaw) <= 1.f
		&& FMath::Abs(UPlayerRattleSubsystem::SwayAt(0.3f, 0.9f, 1.f, 0.3f).Pitch) <= 1.f);
	TestTrue(TEXT("Frost: up quickly"), UPlayerRattleSubsystem::Envelope(0.1f, 1.1f) > 0.9f);
	TestTrue(TEXT("...thawing"), UPlayerRattleSubsystem::Envelope(0.9f, 1.1f) < UPlayerRattleSubsystem::Envelope(0.3f, 1.1f));
	TestEqual(TEXT("...gone at its end"), UPlayerRattleSubsystem::Envelope(1.1f, 1.1f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidShriekPlayTest, "Looter.Creatures.Unpaid.Shriek.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidShriekPlayTest::RunTest(const FString& Parameters)
{
	// In play: one of a pack shrieks at a time; it screams on through its tell into the scream and recovers; its pack is
	// rallied; its coal shot in the tell chokes it (exposed after), a body hit doesn't, and once it screams nothing does.
	FShriekLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	FEditorScriptExecutionGuard RunActorEvents;
	FShriekCVar KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
	FShriekCVar VarietyOn(TEXT("Looter.Unpaid.Variety"), TEXT("1"));
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	AUnpaidCreature* Shrieker = SpawnUnpaid(World, FVector(300.0, 0.0, 0.0), NAME_None);
	AUnpaidCreature* Packmate = SpawnUnpaid(World, FVector(-300.0, 500.0, 0.0), NAME_None);
	AUnpaidCreature* Choker = SpawnUnpaid(World, FVector(0.0, -350.0, 0.0), TEXT("UnpaidShriekTestB"));
	AUnpaidCreature* Screamer = SpawnUnpaid(World, FVector(0.0, 350.0, 0.0), TEXT("UnpaidShriekTestC"));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Shrieker"), Shrieker) || !TestNotNull(TEXT("Packmate"), Packmate)
		|| !TestNotNull(TEXT("Choker"), Choker) || !TestNotNull(TEXT("Screamer"), Screamer))
	{
		return false;
	}
	for (AUnpaidCreature* Each : { Shrieker, Packmate, Choker, Screamer })
	{
		Each->DevPutInState(ECreatureState::Chase, Player);
	}
	const FUnpaidMoveTuning Tuning = FUnpaidMoveTuning::Live();

	// One of a pack at a time.
	if (!TestTrue(TEXT("It shrieks"), Shrieker->StartKitAction(EUnpaidAction::Shriek)))
	{
		return false;
	}
	TestTrue(TEXT("...its action is the shriek"), Shrieker->GetKitAction() == EUnpaidAction::Shriek);
	TestFalse(TEXT("Its pack-mate can't shriek on top of it (the pack's gap)"), Packmate->StartKitAction(EUnpaidAction::Shriek));
	TestFalse(TEXT("...nor can it shriek twice"), Shrieker->StartKitAction(EUnpaidAction::Shriek));

	// Through the tell into the scream: its pack-mate rallied.
	TestFalse(TEXT("Before the scream: the pack-mate is not rallied"), FUnpaidMoveWork::IsRallied(*Packmate));
	TickFor(*Shrieker, Tuning.ShriekTell + 0.15f);
	TestTrue(TEXT("Screaming"), Shrieker->GetKitAction() == EUnpaidAction::Shriek);
	TestTrue(TEXT("...its pack-mate within reach rallied"), FUnpaidMoveWork::IsRallied(*Packmate));
	TestFalse(TEXT("...another pack's not"), FUnpaidMoveWork::IsRallied(*Choker));
	TickFor(*Shrieker, Tuning.ShriekScream + Tuning.ShriekRecover + 0.1f);
	TestTrue(TEXT("Recovered: back in the fight"), Shrieker->GetKitAction() == EUnpaidAction::None);

	// The interrupt: a body hit in the tell doesn't, its coal shot does (and its coal is laid bare a while).
	const FVector From = Player->GetActorLocation();
	if (TestTrue(TEXT("Another pack's shrieks"), Choker->StartKitAction(EUnpaidAction::Shriek)))
	{
		TickFor(*Choker, 0.2f);
		TestEqual(TEXT("In its tell, before any hit: its coal as it is"), FUnpaidShriekWork::CoalExposure(*Choker), 1.f);
		TestFalse(TEXT("A body hit in its tell: it shrieks on"), FUnpaidShriekWork::OnHurt(*Choker, false, From));
		TestTrue(TEXT("...still shrieking"), Choker->GetKitAction() == EUnpaidAction::Shriek);
		TestTrue(TEXT("Its coal shot in the tell: interrupted"), FUnpaidShriekWork::OnHurt(*Choker, true, From));
		TestTrue(TEXT("...the shriek is over"), Choker->GetKitAction() == EUnpaidAction::None);
		TestTrue(TEXT("...staggered"), Choker->IsHeldByHit());
		TestTrue(TEXT("...its coal laid bare (a bigger crit spot)"), FUnpaidShriekWork::CoalExposure(*Choker) > 1.f);
	}
	if (TestTrue(TEXT("A third pack's shrieks"), Screamer->StartKitAction(EUnpaidAction::Shriek)))
	{
		TickFor(*Screamer, Tuning.ShriekTell + 0.2f);
		TestFalse(TEXT("Its coal shot once it screams: committed"), FUnpaidShriekWork::OnHurt(*Screamer, true, From));
		TestTrue(TEXT("...it screams on"), Screamer->GetKitAction() == EUnpaidAction::Shriek);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidShriekMissTest, "Looter.Creatures.Unpaid.Shriek.Miss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidShriekMissTest::RunTest(const FString& Parameters)
{
	// A lunge that misses (the player sidestepped it) turns into a shriek as its turn ends, when the dice say so; with the
	// chance at 0 it never does.
	FShriekLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	FEditorScriptExecutionGuard RunActorEvents;
	FShriekCVar KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
	FShriekCVar VarietyOn(TEXT("Looter.Unpaid.Variety"), TEXT("1"));
	// From 4.5 m only the lunge (no feint, no blink), so the miss is a lunge's.
	FShriekCVar NoFeint(TEXT("Looter.Unpaid.Weight.Feint"), TEXT("0"));
	FShriekCVar NoBlink(TEXT("Looter.Unpaid.Weight.PhaseStrike"), TEXT("0"));
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}

	// One lunge from 4.5 m with the player sidestepping 3 m once it flies; whether a shriek followed within half a second.
	auto MissedLunge = [&](AUnpaidCreature& Unpaid, const FVector& Start)
	{
		Player->SetActorLocation(FVector(Start.X - 450.0, Start.Y, 90.0));
		Unpaid.DevPutInState(ECreatureState::Chase, Player);
		Unpaid.DevPutInState(ECreatureState::Attack, Player);
		TestTrue(TEXT("It lunges"), Unpaid.GetKitMove() == EUnpaidMove::Lunge);
		int32 Frames = 0;
		while (!Unpaid.IsLunging() && Frames++ < 120)
		{
			Unpaid.ConsumeMovementInputVector();
			Unpaid.Tick(Step);
		}
		TestTrue(TEXT("...it flies"), Unpaid.IsLunging());
		Player->SetActorLocation(Player->GetActorLocation() + FVector(0.0, 300.0, 0.0));
		Frames = 0;
		while (Unpaid.IsLunging() && Frames++ < 120)
		{
			Unpaid.ConsumeMovementInputVector();
			Unpaid.Tick(Step);
		}
		bool bShrieked = false;
		for (int32 Frame = 0; Frame < 30 && !bShrieked; ++Frame)
		{
			Unpaid.ConsumeMovementInputVector();
			Unpaid.Tick(Step);
			bShrieked = Unpaid.GetKitAction() == EUnpaidAction::Shriek;
		}
		return bShrieked;
	};
	{
		FShriekCVar Always(TEXT("Looter.Unpaid.Shriek.Chance"), TEXT("1"));
		const FVector Start(450.0, 0.0, 0.0);
		AUnpaidCreature* Unpaid = SpawnUnpaid(World, Start, TEXT("UnpaidMissTestA"));
		if (TestNotNull(TEXT("Unpaid spawned"), Unpaid))
		{
			TestTrue(TEXT("Its lunge missed: it shrieks instead of lunging again"), MissedLunge(*Unpaid, Start));
		}
	}
	{
		FShriekCVar Never(TEXT("Looter.Unpaid.Shriek.Chance"), TEXT("0"));
		const FVector Start(450.0, 3000.0, 0.0);
		AUnpaidCreature* Unpaid = SpawnUnpaid(World, Start, TEXT("UnpaidMissTestB"));
		if (TestNotNull(TEXT("Second Unpaid spawned"), Unpaid))
		{
			TestFalse(TEXT("Chance 0: a miss is just a miss"), MissedLunge(*Unpaid, Start));
		}
	}
	return true;
}

#endif
