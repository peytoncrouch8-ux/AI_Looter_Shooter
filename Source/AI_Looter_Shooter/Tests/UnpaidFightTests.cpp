#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/GunfeelReactTuning.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightTuning.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

// The Unpaid's fighting kit (round 2, 2026-10-27; Creatures/UnpaidCreatureMoves.cpp): its numbers' bounds (fair tells, a rake
// no stronger than a lunge and a half), its rules (the lead, the move it picks, the swipe's arc, the flank's spots, its pace),
// who fights with it, its moves as play starts them, and the fight director's pack moments (a frenzy's extra turns, one
// rally, howl and cinder at a time, a creature's own chase boost).

namespace
{
	/** A test level with a fight director in it (one is made only in played worlds unless a test lets it). */
	struct FKitLevel
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
	struct FCVarScope
	{
		IConsoleVariable* Variable = nullptr;
		FString Was;
		FCVarScope(const TCHAR* Name, const TCHAR* Value)
		{
			Variable = IConsoleManager::Get().FindConsoleVariable(Name);
			if (Variable)
			{
				Was = Variable->GetString();
				// As the console sets it, so it wins over whatever set it before (as ColdOpenTests does).
				Variable->Set(Value, ECVF_SetByConsole);
			}
		}
		~FCVarScope()
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitTuningTest, "Looter.Creatures.Unpaid.Kit.Tuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitTuningTest::RunTest(const FString& Parameters)
{
	// The defaults (the cvars' own): harder, but every move fair.
	const FUnpaidFightTuning Tuning;
	const AUnpaidCreature* Defaults = GetDefault<AUnpaidCreature>();
	TestEqual(TEXT("The kit is on"), Tuning.Kit, 1);
	TestTrue(TEXT("It leads, but never perfectly"), Tuning.LeadShare > 0.f && Tuning.LeadShare < 1.f);
	TestTrue(TEXT("...and never far ahead"), Tuning.LeadMaxSeconds > 0.f && Tuning.LeadMaxSeconds <= 0.6f);
	TestTrue(TEXT("It lunges from farther out than it did"), Tuning.LungeStart > Defaults->AttackRange);
	TestTrue(TEXT("...and farther"), Tuning.LungeDistance > Defaults->LungeDistance);
	TestTrue(TEXT("Its lunge bends little if at all in flight"), Tuning.LungeHoming >= 0.f && Tuning.LungeHoming <= 60.f);
	// A walking strafe beats a Regular one's lunge from its farthest start (DEMO 2026-10-27: 11 of 13 landed at the old lead).
	// Measured as the lunge really flies (AUnpaidCreature::Strike): at its aim, stopping LungeStopShort short of it, landing on
	// whoever comes within LungeHitRadius of its middle on the way (TryLungeHit, each frame). The first version measured from
	// the aim point and left out the stop-short (103 cm; the real nearest is about 139).
	const float Lunge = Defaults->LungeSpeed;
	const FVector Player(Tuning.LungeStart, 0.0, 0.0);
	const FVector Strafe(0.0, 400.0, 0.0);
	const FVector Aim = UnpaidFightRules::LeadPoint(FVector::ZeroVector, Player, Strafe, Lunge, Tuning.LeadShare, Tuning.LeadMaxSeconds);
	const FVector Way = FVector(Aim.X, Aim.Y, 0.0).GetSafeNormal();
	const float Length = FMath::Clamp(static_cast<float>(FVector::Dist2D(FVector::ZeroVector, Aim)) - Defaults->LungeStopShort, 0.f, Tuning.LungeDistance);
	const float Flight = Length / Lunge;
	float Miss = TNumericLimits<float>::Max();
	for (int32 Sample = 0; Sample <= 120; ++Sample)
	{
		const float Time = Flight * static_cast<float>(Sample) / 120.f;
		Miss = FMath::Min(Miss, static_cast<float>(FVector::Dist2D(Way * (Lunge * Time), Player + Strafe * Time)));
	}
	TestTrue(FString::Printf(TEXT("A walking strafe dodges its lunge (it comes within %.0f cm of them, it lands within %.0f)"), Miss, Tuning.LungeHitRadius),
		Miss > Tuning.LungeHitRadius);
	// Both measured from its middle: a lunge stops LungeStopShort short of a target standing still, so it must land farther
	// out than that (DEMO 2026-10-28: at 85 no lunge landed on a still player).
	TestTrue(FString::Printf(TEXT("It lands farther out than it stops short (%.0f > %.0f): a standing target is hit"), Tuning.LungeHitRadius,
		Defaults->LungeStopShort), Tuning.LungeHitRadius > Defaults->LungeStopShort);
	TestTrue(TEXT("A landed lunge hurts no more than its attack"), Tuning.LungeDamageShare > 0.f && Tuning.LungeDamageShare <= 1.f);
	// Its last stretch is committed (2026-10-28: steady rifle fire kept every lunge short), never more than its whole flight; its
	// hit slows come once a gap longer than one slow lasts.
	TestTrue(TEXT("Its lunge commits its last stretch, within its flight"), Tuning.LungeCommit > 0.f && Tuning.LungeCommit <= Tuning.LungeDistance / Lunge + 0.05f);
	TestTrue(TEXT("It takes the hit slow once a gap, longer than a slow"), Tuning.HitSlowGap >= FGunfeelReactTuning().FlinchSlowSeconds);
	TestTrue(TEXT("It rakes only close in, nearer than it lunges from"), Tuning.RakeRange < Tuning.LungeStart);
	TestTrue(TEXT("Two swipes hurt no more than a lunge and a half"), 2.f * Tuning.RakeDamageShare <= 1.5f);
	TestTrue(TEXT("A cinder hurts less than a lunge"), Tuning.CinderDamageShare < 1.f);
	// Every tell long enough to see and answer (a reaction is about a quarter second).
	TestTrue(TEXT("The rake's arm drawn back: a tell"), Tuning.RakeWindup >= 0.4f);
	TestTrue(TEXT("The cinder's swelling glow: a tell"), Tuning.CinderTell >= 0.5f);
	TestTrue(TEXT("The rally's scream: a tell"), Tuning.RallyTell >= 0.5f);
	TestTrue(TEXT("A cinder is slow enough to sidestep"), Tuning.CinderSpeed <= 2000.f);
	// The pack comes on harder together, for a while, and not often.
	const FFightTuning Fight;
	TestTrue(TEXT("A frenzy lets more in at once"), Tuning.FrenzyTokens > Fight.MeleeTokens);
	TestTrue(TEXT("...for a few seconds"), Tuning.FrenzySeconds > 1.f && Tuning.FrenzySeconds <= 6.f);
	TestTrue(TEXT("...at most once in a good while"), Tuning.RallyCooldown >= 3.f * Tuning.FrenzySeconds);
	TestTrue(TEXT("A pack, not a pair"), Tuning.RallyMinPack >= 3);
	// A flank comes from where its turn would still hold: inside 1.5 approach distances of the ring.
	const float TurnHeld = 1.5f * Fight.RingApproach * FMath::Max(Fight.RingRadius, Defaults->AttackRange + Fight.RingMargin);
	TestTrue(TEXT("It flanks from within its turn's reach"), Tuning.FlankMaxRange < TurnHeld);
	TestTrue(TEXT("...and comes back outside a rake's reach"), Tuning.FlankRadius > Tuning.RakeRange);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitRulesTest, "Looter.Creatures.Unpaid.Kit.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitRulesTest::RunTest(const FString& Parameters)
{
	// The lead: a target standing still is aimed at where they stand; one running across is led along their run, by the
	// share and never past the cap; a jump changes nothing.
	const FVector From(0.0, 0.0, 0.0);
	const FVector Target(1000.0, 0.0, 0.0);
	TestTrue(TEXT("Standing still: where they stand"), UnpaidFightRules::LeadPoint(From, Target, FVector::ZeroVector, 1400.f, 0.7f, 0.45f).Equals(Target, 0.01));
	const FVector Run(0.0, 400.0, 0.0);
	const FVector Full = UnpaidFightRules::LeadPoint(From, Target, Run, 1400.f, 1.f, 10.f);
	TestTrue(TEXT("Running across: led their way"), Full.Y > 250.0 && Full.Y < 330.0 && FMath::IsNearlyEqual(Full.X, 1000.0, 0.01));
	const FVector Capped = UnpaidFightRules::LeadPoint(From, Target, Run, 1400.f, 1.f, 0.45f);
	TestTrue(TEXT("...never more than its cap ahead"), FMath::IsNearlyEqual(Capped.Y, 180.0, 0.5));
	const FVector Shared = UnpaidFightRules::LeadPoint(From, Target, Run, 1400.f, 0.5f, 0.45f);
	TestTrue(TEXT("...by its share"), FMath::IsNearlyEqual(Shared.Y, 90.0, 0.5));
	TestTrue(TEXT("A jump isn't a run"), UnpaidFightRules::LeadPoint(From, Target, FVector(0.0, 0.0, 600.0), 1400.f, 1.f, 1.f).Equals(Target, 0.01));
	TestTrue(TEXT("No share: no lead"), UnpaidFightRules::LeadPoint(From, Target, Run, 1400.f, 0.f, 1.f).Equals(Target, 0.01));

	// Its move: close in, a rake when the dice say so; farther, always the lunge.
	TestTrue(TEXT("Close, a low roll: rake"), UnpaidFightRules::ChooseMove(200.f, 260.f, 0.7f, 0.5f) == EUnpaidMove::Rake);
	TestTrue(TEXT("Close, a high roll: lunge"), UnpaidFightRules::ChooseMove(200.f, 260.f, 0.7f, 0.8f) == EUnpaidMove::Lunge);
	TestTrue(TEXT("Farther: lunge"), UnpaidFightRules::ChooseMove(300.f, 260.f, 0.7f, 0.f) == EUnpaidMove::Lunge);

	// The swipe: its arc across its front, its reach, up and down.
	const FVector Ahead = FVector::ForwardVector;
	auto At = [](float Degrees, float Distance, float Up = 0.f)
	{
		const float Radians = FMath::DegreesToRadians(Degrees);
		return FVector(FMath::Cos(Radians) * Distance, FMath::Sin(Radians) * Distance, Up);
	};
	TestTrue(TEXT("Straight ahead, in reach"), UnpaidFightRules::InSwipe(From, Ahead, At(0.f, 200.f), 240.f, 130.f, 220.f));
	TestTrue(TEXT("60 degrees aside, in a 130 degree arc"), UnpaidFightRules::InSwipe(From, Ahead, At(60.f, 200.f), 240.f, 130.f, 220.f));
	TestFalse(TEXT("70 degrees aside: out of the arc"), UnpaidFightRules::InSwipe(From, Ahead, At(70.f, 200.f), 240.f, 130.f, 220.f));
	TestFalse(TEXT("Behind it"), UnpaidFightRules::InSwipe(From, Ahead, At(180.f, 100.f), 240.f, 130.f, 220.f));
	TestFalse(TEXT("Out of reach"), UnpaidFightRules::InSwipe(From, Ahead, At(0.f, 260.f), 240.f, 130.f, 220.f));
	TestFalse(TEXT("Up on a ledge"), UnpaidFightRules::InSwipe(From, Ahead, At(0.f, 150.f, 300.f), 240.f, 130.f, 220.f));

	// The lunge's bend: never faster than its rate.
	const FVector Turned = UnpaidFightRules::TurnToward(Ahead, FVector::RightVector, 30.f);
	TestNearlyEqual(TEXT("Turns 30 of 90 degrees"), static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Turned.Y, Turned.X))), 30.f, 0.01f);
	TestTrue(TEXT("A small turn all the way"), UnpaidFightRules::TurnToward(Ahead, At(10.f, 1.f), 30.f).Equals(At(10.f, 1.f), 0.001));

	// The flank: its side first, at their side and behind them, never in the middle of their view.
	const TArray<float> Bearings = UnpaidFightRules::FlankBearings(0.f, 20.f);
	if (TestEqual(TEXT("Seven spots to try"), Bearings.Num(), 7))
	{
		TestNearlyEqual(TEXT("First: its own side, wide"), Bearings[0], 105.f, 0.01f);
		TestNearlyEqual(TEXT("Then the other side"), Bearings[1], -105.f, 0.01f);
		TestNearlyEqual(TEXT("Then behind the shoulder"), Bearings[2], 145.f, 0.01f);
		TestNearlyEqual(TEXT("Straight behind, fifth"), FMath::Abs(Bearings[4]), 180.f, 0.01f);
		for (const float Bearing : Bearings)
		{
			TestTrue(FString::Printf(TEXT("%.0f is out of the middle of their view"), Bearing), FMath::Abs(FMath::UnwindDegrees(Bearing)) >= 70.f);
		}
	}
	TestNearlyEqual(TEXT("From their right, the right side first"), UnpaidFightRules::FlankBearings(0.f, -30.f)[0], -105.f, 0.01f);
	TestTrue(TEXT("In front, in range: flank"), UnpaidFightRules::WantsFlank(0.9f, 700.f, 0.75f, 450.f, 1100.f));
	TestFalse(TEXT("Already beside them: no"), UnpaidFightRules::WantsFlank(0.5f, 700.f, 0.75f, 450.f, 1100.f));
	TestFalse(TEXT("Too near: no"), UnpaidFightRules::WantsFlank(0.9f, 300.f, 0.75f, 450.f, 1100.f));

	// Its pace: faster far off (not waiting on the ring), faster still in its pack's frenzy.
	TestNearlyEqual(TEXT("Near: its chase"), UnpaidFightRules::ChaseBoost(500.f, false, 900.f, 1.2f, false, 1.3f), 1.f, 0.0001f);
	TestNearlyEqual(TEXT("Far: closing in"), UnpaidFightRules::ChaseBoost(1500.f, false, 900.f, 1.2f, false, 1.3f), 1.2f, 0.0001f);
	TestNearlyEqual(TEXT("Waiting on the ring: no closing"), UnpaidFightRules::ChaseBoost(1500.f, true, 900.f, 1.2f, false, 1.3f), 1.f, 0.0001f);
	TestNearlyEqual(TEXT("Frenzy, far: both"), UnpaidFightRules::ChaseBoost(1500.f, false, 900.f, 1.2f, true, 1.3f), 1.56f, 0.0001f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitCommitTest, "Looter.Creatures.Unpaid.Kit.Commit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitCommitTest::RunTest(const FString& Parameters)
{
	// The lunge against a gun's hit slow (DEMO 2026-10-28, the chapel yard: 59 attacks, 2 hits): the slow is taken once a gap,
	// a slowed flight goes its whole way, and its last stretch flies at full speed with the slow held off.
	FKitLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	FEditorScriptExecutionGuard RunActorEvents;
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	AUnpaidCreature* Unpaid = SpawnUnpaid(World, FVector(450.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Unpaid spawned"), Unpaid))
	{
		return false;
	}
	FCVarScope KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
	FCVarScope NeverRake(TEXT("Looter.Unpaid.RakeChance"), TEXT("0"));
	// The lunge itself, as this test flies it: the varied pick (a feint or a blink from 4.5 m) and a miss's shriek are their own tests.
	FCVarScope PlainKit(TEXT("Looter.Unpaid.Variety"), TEXT("0"));
	const FGunfeelReactTuning Gunfeel;
	constexpr float Step = 1.f / 60.f;

	// The slow once a gap: a second hit in the gap neither deepens nor lengthens it.
	BossTestWorld::Hurt(Unpaid, 1.f);
	UMovementSlowComponent* Slow = Unpaid->FindComponentByClass<UMovementSlowComponent>();
	if (!TestNotNull(TEXT("A hit slows it"), Slow))
	{
		return false;
	}
	Slow->Advance(0.1f);
	const float Left = Slow->GetTimeLeft();
	BossTestWorld::Hurt(Unpaid, 1.f);
	TestEqual(TEXT("A second hit in the gap doesn't slow it again"), Slow->GetTimeLeft(), Left, 0.001f);
	Slow->Advance(1.f);
	TestFalse(TEXT("The slow ran out"), Slow->IsSlowed());

	// A lunge from 4.5 m is in its committed stretch from the start: a hit's slow is held off it, and it flies at full speed.
	Unpaid->DevPutInState(ECreatureState::Chase, Player);
	Unpaid->DevPutInState(ECreatureState::Attack, Player);
	TestTrue(TEXT("4.5 m off: it lunges"), Unpaid->GetKitMove() == EUnpaidMove::Lunge);
	int32 Frames = 0;
	while (!Unpaid->IsLunging() && Frames < 120)
	{
		Unpaid->ConsumeMovementInputVector();
		Unpaid->Tick(Step);
		++Frames;
	}
	if (!TestTrue(TEXT("The lunge flies after the shriek"), Unpaid->IsLunging()))
	{
		return false;
	}
	TestTrue(TEXT("A short flight is committed from its first frame"), Unpaid->IsLungeCommitted());
	const float Full = Unpaid->GetLungeSpeed();
	UMovementSlowComponent::Apply(Unpaid, Gunfeel.FlinchSlow, 10.f);
	Unpaid->ConsumeMovementInputVector();
	Unpaid->Tick(Step);
	TestTrue(TEXT("...the slow is held off it"), Slow->IsSuspended() && Slow->GetMultiplier() == 1.f);
	TestEqual(TEXT("...and it flies at full speed"), Unpaid->GetCharacterMovement()->MaxWalkSpeed, Full, 1.f);
	Frames = 0;
	while (Unpaid->IsLunging() && Frames < 120)
	{
		Unpaid->ConsumeMovementInputVector();
		Unpaid->Tick(Step);
		++Frames;
	}
	TestFalse(TEXT("The lunge ends"), Unpaid->IsLunging());
	TestFalse(TEXT("...and the slow it took on the way holds it again"), Slow->IsSuspended());
	TestEqual(TEXT("...at its share"), Unpaid->GetCharacterMovement()->MaxWalkSpeed, Full * Gunfeel.FlinchSlow, 1.f);
	Slow->Advance(20.f);

	// A longer flight with a short commit: slowed, it flies its whole way (longer, never cut short), then commits.
	FCVarScope ShortCommit(TEXT("Looter.Unpaid.LungeCommit"), TEXT("0.1"));
	Unpaid->DevPutInState(ECreatureState::Chase, Player);
	Unpaid->SetActorLocation(FVector(520.0, 0.0, Unpaid->GetActorLocation().Z));
	Unpaid->DevPutInState(ECreatureState::Attack, Player);
	Frames = 0;
	while (!Unpaid->IsLunging() && Frames < 120)
	{
		Unpaid->ConsumeMovementInputVector();
		Unpaid->Tick(Step);
		++Frames;
	}
	if (!TestTrue(TEXT("The second lunge flies"), Unpaid->IsLunging()))
	{
		return false;
	}
	TestFalse(TEXT("A long flight isn't committed at first"), Unpaid->IsLungeCommitted());
	UMovementSlowComponent::Apply(Unpaid, Gunfeel.FlinchSlow, 10.f);
	TestFalse(TEXT("...so the slow holds it"), Slow->IsSuspended());
	bool bCommitted = false;
	float Flight = 0.f;
	while (Unpaid->IsLunging() && Flight < 2.f)
	{
		Unpaid->ConsumeMovementInputVector();
		Unpaid->Tick(Step);
		Flight += Step;
		bCommitted |= Unpaid->IsLungeCommitted();
	}
	// Its way: 520 less the stop-short, at 55% until the last 0.1 s' worth, then full: longer than the unslowed flight, and
	// never cut off by the wall-grace (which counts the way it really flew).
	const AUnpaidCreature* Class = GetDefault<AUnpaidCreature>();
	const float Way = 520.f - Class->LungeStopShort;
	const float Unslowed = Way / Full;
	const float Expected = (Way - 0.1f * Full) / (Full * Gunfeel.FlinchSlow) + 0.1f;
	TestTrue(FString::Printf(TEXT("Slowed, the lunge flies its whole way (%.2f s; unslowed %.2f, expected %.2f)"), Flight, Unslowed, Expected),
		Flight > Unslowed + 0.05f && FMath::Abs(Flight - Expected) < 0.08f);
	TestTrue(TEXT("...and commits its last stretch"), bCommitted);
	TestFalse(TEXT("...letting the slow hold it again after"), Slow->IsSuspended());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitPressTest, "Looter.Creatures.Unpaid.Kit.Press",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitPressTest::RunTest(const FString& Parameters)
{
	// Shot at from just past its ground's edge (DEMO 2026-10-28: from the chapel's doorway, nine stood in a line at the edge),
	// an Unpaid fights them there; from farther off, and with its kit off, it keeps to its edge as before. Never higher up.
	FKitLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	FEditorScriptExecutionGuard RunActorEvents;
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Near = EncounterTestWorld::SpawnPlayer(World, FVector(1100.0, 0.0, 90.0));
	ACharacter* Far = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 2000.0, 90.0));
	ACharacter* Above = EncounterTestWorld::SpawnPlayer(World, FVector(-1000.0, 0.0, 690.0));
	AUnpaidCreature* Unpaid = SpawnUnpaid(World, FVector(0.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Players"), Near) || !TestNotNull(TEXT("..."), Far) || !TestNotNull(TEXT("..."), Above)
		|| !TestNotNull(TEXT("Unpaid spawned"), Unpaid))
	{
		return false;
	}
	FCVarScope KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
	// Its yard: 6 m round its home, 3 m of height either way.
	Unpaid->HuntingGround.Radius = 600.f;
	Unpaid->HuntingGround.MaxRise = 300.f;
	const FUnpaidFightTuning Tuning;
	TestTrue(TEXT("The reach is a doorway's, not a field's"), Tuning.PressReach >= 400.f && Tuning.PressReach <= 1200.f);
	TestFalse(TEXT("5 m past its edge: not on its ground"), Unpaid->IsOnItsGround(Near->GetActorLocation()));
	{
		FCVarScope KitOff(TEXT("Looter.Unpaid.Kit"), TEXT("0"));
		TestFalse(TEXT("Kit off: a hit from there doesn't press it"), Unpaid->NotePressed(Near));
	}
	TestFalse(TEXT("14 m past its edge: too far to press it"), Unpaid->NotePressed(Far));
	TestFalse(TEXT("6 m up, past its height band: never"), Unpaid->NotePressed(Above));
	TestTrue(TEXT("5 m past its edge: pressed"), Unpaid->NotePressed(Near));
	TestTrue(TEXT("...so it fights them there"), Unpaid->IsOnItsGround(Near->GetActorLocation()) && Unpaid->IsPressed());
	TestNearlyEqual(TEXT("...its ground reaching past its edge"), Unpaid->GetGroundReach(), Tuning.PressReach, 0.5f);
	TestFalse(TEXT("...not as far as the far one"), Unpaid->IsOnItsGround(Far->GetActorLocation()));
	TestTrue(TEXT("Spiders never press"), GetDefault<ACreatureBase>()->GetPressReach() == 0.f);

	// A hit from past its edge owes the hit one a blink in, even pressed already (the fight begun on its ground: DEMO
	// 2026-10-28, the yard's pack pressed before the player stepped out past the fence).
	TestTrue(TEXT("...and it may step though waiting its turn: its target is past its own edge"), Unpaid->WaitGoesNowhere(*Near));
	if (APlayerController* Shooter = World->SpawnActor<APlayerController>())
	{
		Shooter->Possess(Near);
		BossTestWorld::Hurt(Unpaid, 1.f, Shooter);
		TestTrue(TEXT("Shot from past its edge, pressed already: it owes a blink in"), Unpaid->OwesPressStep());
	}
	AUnpaidCreature* Hit = SpawnUnpaid(World, FVector(0.0, -300.0, 0.0));
	if (TestNotNull(TEXT("A second Unpaid"), Hit))
	{
		Hit->HuntingGround.Radius = 600.f;
		Hit->HuntingGround.MaxRise = 300.f;
		Hit->AlertTo(Near);
		TestTrue(TEXT("Its pack's call presses it"), Hit->IsPressed());
		TestFalse(TEXT("...but only the one hit owes a blink in"), Hit->OwesPressStep());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitMovesTest, "Looter.Creatures.Unpaid.Kit.Moves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitMovesTest::RunTest(const FString& Parameters)
{
	// Who fights with it: the Unpaid; never the kinds on its rig with their own attacks, nor Abel.
	{
		FCVarScope KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
		TestTrue(TEXT("The Unpaid has its kit"), GetDefault<AUnpaidCreature>()->HasFightKit());
		TestFalse(TEXT("A dead miner swings his pick"), GetDefault<ADeadMinerCreature>()->HasFightKit());
		TestFalse(TEXT("The Bell-Ringer rings"), GetDefault<ABellRingerCreature>()->HasFightKit());
		TestFalse(TEXT("Abel keeps his fight"), GetDefault<AAbelKeeper>()->HasFightKit());
	}

	FKitLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	FEditorScriptExecutionGuard RunActorEvents;
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	AUnpaidCreature* Unpaid = SpawnUnpaid(World, FVector(200.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Unpaid spawned"), Unpaid))
	{
		return false;
	}
	const AUnpaidCreature* Class = GetDefault<AUnpaidCreature>();
	{
		FCVarScope KitOn(TEXT("Looter.Unpaid.Kit"), TEXT("1"));
		FCVarScope AlwaysRake(TEXT("Looter.Unpaid.RakeChance"), TEXT("1"));
		// The kit's own two moves, as they were: the varied pick has its tests (Looter.Creatures.Unpaid.Moves).
		FCVarScope PlainKit(TEXT("Looter.Unpaid.Variety"), TEXT("0"));
		TestTrue(TEXT("A spawned Unpaid has its kit"), Unpaid->HasFightKit());

		// Close in: the rake, with its own wind-up and the two swipes' recovery.
		Unpaid->DevPutInState(ECreatureState::Attack, Player);
		TestTrue(TEXT("2 m off: it rakes"), Unpaid->GetKitMove() == EUnpaidMove::Rake);
		TestNearlyEqual(TEXT("...after its arm is drawn back"), Unpaid->AttackWindup, FUnpaidFightTuning::Live().RakeWindup, 0.001f);
		TestNearlyEqual(TEXT("...and hangs after the second swipe"), Unpaid->AttackRecovery,
			FUnpaidFightTuning::Live().RakeGap + FUnpaidFightTuning::Live().RakeRecovery, 0.001f);

		// Farther: the lunge, with the class's own timings back.
		Unpaid->DevPutInState(ECreatureState::Chase, Player);
		Unpaid->SetActorLocation(FVector(450.0, 0.0, Unpaid->GetActorLocation().Z));
		Unpaid->DevPutInState(ECreatureState::Attack, Player);
		TestTrue(TEXT("4.5 m off: it lunges"), Unpaid->GetKitMove() == EUnpaidMove::Lunge);
		TestNearlyEqual(TEXT("...after the class's shriek"), Unpaid->AttackWindup, Class->AttackWindup, 0.001f);
		TestNearlyEqual(TEXT("...and the class's recovery"), Unpaid->AttackRecovery, Class->AttackRecovery, 0.001f);
	}
	{
		// The kit off: the old shriek and lunge, nothing chosen.
		FCVarScope KitOff(TEXT("Looter.Unpaid.Kit"), TEXT("0"));
		Unpaid->DevPutInState(ECreatureState::Chase, Player);
		Unpaid->DevPutInState(ECreatureState::Attack, Player);
		TestTrue(TEXT("Kit off: no move of its own"), Unpaid->GetKitMove() == EUnpaidMove::None);
		TestFalse(TEXT("...and no kit"), Unpaid->HasFightKit());
	}

	// On the ring: a cinder's tell, then the cinder thrown (one pellet in flight, its own); busy, it can't scream as well.
	UEnemyProjectileSubsystem* Shots = World->GetSubsystem<UEnemyProjectileSubsystem>();
	AUnpaidCreature* Thrower = SpawnUnpaid(World, FVector(1000.0, 400.0, 0.0));
	if (!TestNotNull(TEXT("A projectile subsystem"), Shots) || !TestNotNull(TEXT("Thrower spawned"), Thrower))
	{
		return false;
	}
	Player->GetCharacterMovement()->Velocity = FVector(0.0, 400.0, 0.0);
	Thrower->DevPutInState(ECreatureState::Chase, Player);
	TestTrue(TEXT("It readies a cinder"), Thrower->StartKitAction(EUnpaidAction::Cinder));
	TestTrue(TEXT("...its tell is on"), Thrower->GetKitAction() == EUnpaidAction::Cinder);
	TestFalse(TEXT("...and it can't scream at the same time"), Thrower->StartKitAction(EUnpaidAction::Rally));
	TestEqual(TEXT("Nothing thrown in the tell"), Shots->NumShotsFrom(Thrower), 0);
	TestTrue(TEXT("Thrown"), Thrower->ThrowCinder());
	TestEqual(TEXT("...one cinder in flight, its own"), Shots->NumShotsFrom(Thrower), 1);

	// A rally scream, on its own.
	AUnpaidCreature* Screamer = SpawnUnpaid(World, FVector(-800.0, 0.0, 0.0));
	if (TestNotNull(TEXT("Screamer spawned"), Screamer))
	{
		TestFalse(TEXT("Calm, it screams at nobody"), Screamer->StartKitAction(EUnpaidAction::Rally));
		Screamer->DevPutInState(ECreatureState::Chase, Player);
		TestTrue(TEXT("Hunting, it screams its pack on"), Screamer->StartKitAction(EUnpaidAction::Rally));
		TestTrue(TEXT("...screaming"), Screamer->GetKitAction() == EUnpaidAction::Rally);
		TestFalse(TEXT("Mid-scream it starts nothing else"), Screamer->StartKitAction(EUnpaidAction::Cinder));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidKitDirectorTest, "Looter.Creatures.Unpaid.Kit.Director",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidKitDirectorTest::RunTest(const FString& Parameters)
{
	// The director's pack moments: a frenzy's extra turns for its pack only, one rally, howl and cinder at a time at a player,
	// the pack counted at a look, and a creature's own chase boost (forgotten with it).
	FKitLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	UFightDirectorSubsystem* Director = World->GetSubsystem<UFightDirectorSubsystem>();
	FEditorScriptExecutionGuard RunActorEvents;
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	TArray<AUnpaidCreature*> Pack;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const float Radians = FMath::DegreesToRadians(72.f * static_cast<float>(Index));
		Pack.Add(SpawnUnpaid(World, FVector(FMath::Cos(Radians) * 700.f, FMath::Sin(Radians) * 700.f, 0.f)));
	}
	if (!TestNotNull(TEXT("A director"), Director) || !TestNotNull(TEXT("Player stand-in"), Player) || !TestFalse(TEXT("Five Unpaid"), Pack.Contains(nullptr)))
	{
		return false;
	}
	for (AUnpaidCreature* Unpaid : Pack)
	{
		Unpaid->DevPutInState(ECreatureState::Chase, Player);
	}
	const int32 Usual = FFightTuning::Live().MeleeTokens;
	for (int32 Index = 0; Index < Usual; ++Index)
	{
		TestTrue(FString::Printf(TEXT("Turn %d"), Index + 1), Director->TryTakeToken(*Pack[Index], *Player, EFightRole::Melee));
	}
	TestFalse(TEXT("The next waits"), Director->TryTakeToken(*Pack[Usual], *Player, EFightRole::Melee));

	// The rally's frenzy: more turns for the Unpaid at this player, for a while.
	TestEqual(TEXT("No frenzy yet"), Director->GetFrenzyLeft(*Player, TEXT("Unpaid")), 0.f);
	Director->OpenFrenzy(*Player, TEXT("Unpaid"), 4.f, Usual + 1);
	TestTrue(TEXT("A frenzy on"), Director->GetFrenzyLeft(*Player, TEXT("Unpaid")) > 3.9f);
	TestEqual(TEXT("...not another pack's"), Director->GetFrenzyLeft(*Player, TEXT("Spider")), 0.f);
	TestTrue(TEXT("...the one waiting comes in"), Director->TryTakeToken(*Pack[Usual], *Player, EFightRole::Melee));
	TestEqual(TEXT("...one more turn held than usual"), Director->CountTokens(*Player, EFightRole::Melee), Usual + 1);

	// One at a time, per player.
	TestTrue(TEXT("A rally"), Director->TryStartRally(*Player, 16.f));
	TestFalse(TEXT("...not another so soon"), Director->TryStartRally(*Player, 16.f));
	TestTrue(TEXT("A howl"), Director->TryHowl(*Player, 12.f));
	TestFalse(TEXT("...one howl in a while"), Director->TryHowl(*Player, 12.f));
	TestTrue(TEXT("A cinder"), Director->TryTakeCinder(*Player, 1.4f));
	TestFalse(TEXT("...one at a time"), Director->TryTakeCinder(*Player, 1.4f));
	TestTrue(TEXT("...none to wait for with no gap"), Director->TryTakeCinder(*Player, 0.f));

	// The pack, counted at a look.
	Director->RefreshNow();
	TestEqual(TEXT("Five Unpaid on the player"), Director->CountPack(*Player, TEXT("Unpaid")), 5);
	TestEqual(TEXT("...no spiders"), Director->CountPack(*Player, TEXT("Spider")), 0);

	// A creature's own chase boost rides on its chase, and goes with it.
	AUnpaidCreature* Runner = Pack[4];
	const float Before = Runner->GetChaseSpeedNow();
	Director->SetChaseBoost(*Runner, 1.5f);
	TestNearlyEqual(TEXT("Boosted: half as fast again"), Runner->GetChaseSpeedNow(), Before * 1.5f, 0.01f);
	Director->ForgetCreature(*Runner);
	TestNearlyEqual(TEXT("Forgotten: its own chase"), Runner->GetChaseSpeedNow(), Before, 0.01f);
	TestNearlyEqual(TEXT("...a boost of 1"), Director->GetChaseBoost(*Runner), 1.f, 0.0001f);
	return true;
}

#endif
