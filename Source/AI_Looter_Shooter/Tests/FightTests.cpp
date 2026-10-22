#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/BossComponent.h"
#include "Bosses/BossTestSpider.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightRules.h"
#include "Creatures/FightTuning.h"
#include "Creatures/SpiderCreature.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerSize.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// The fight director (Phase 12.2, FIGHT-AI; Saved/Fight/CONTRACT.md): attack turns (how many at once, the longest waiter
// next, the rest after an attack, a staggered one keeping its place, a turn held too long taken back), the surround ring's
// slots (spread by bearing, off the player's front, closed slots skipped, kept between looks, shared when there are more
// creatures than slots) and feints, the ranged kinds' spots, the difficulty's scales, and the director in a test level
// (turns given and given back by real creatures, its ring round a player, bosses and Legendary monsters left out).

namespace
{
	/** A test level with a fight director in it (one is made only in played worlds unless a test lets it). */
	struct FDirectorLevel
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightTuningDefaultsTest, "Looter.Fight.Tuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightTuningDefaultsTest::RunTest(const FString& Parameters)
{
	// The contract's numbers, as the cvars start.
	const FFightTuning Tuning;
	TestEqual(TEXT("The director is on"), Tuning.Director, 1);
	TestEqual(TEXT("3 melee turns at once"), Tuning.MeleeTokens, 3);
	TestEqual(TEXT("2 ranged turns at once"), Tuning.RangedTokens, 2);
	TestEqual(TEXT("A turn held 3 s is taken back"), Tuning.TokenMaxSeconds, 3.f);
	TestEqual(TEXT("0.6 s rest after an attack"), Tuning.TokenRest, 0.6f);
	TestEqual(TEXT("The ring: 4.8 m"), Tuning.RingRadius, 480.f);
	TestEqual(TEXT("...8 slots"), Tuning.RingSlots, 8);
	TestEqual(TEXT("...looked at every 0.4 s"), Tuning.RingRefresh, 0.4f);
	TestEqual(TEXT("...turning 25 degrees a second"), Tuning.RingDrift, 25.f);
	TestEqual(TEXT("...feints at 0.15 a look"), Tuning.FeintChance, 0.15f);
	TestEqual(TEXT("...0.35 against the front"), Tuning.BehindBias, 0.35f);
	TestEqual(TEXT("...for 3 or more"), Tuning.RingMinPack, 3);
	TestEqual(TEXT("Ranged spots 50 degrees off the pack"), Tuning.RangedOffAngle, 50.f);
	TestEqual(TEXT("...moving on after 6 shots in 10"), Tuning.RangedRelocate, 0.6f);
	TestEqual(TEXT("...backing off inside 9 m"), Tuning.RangedBackOff, 900.f);
	TestEqual(TEXT("Chase x1.3"), Tuning.ChaseScale, 1.3f);
	TestEqual(TEXT("Damage x1.8 (raised from 1.5 for the player's ward, 2026-10-22)"), Tuning.DamageScale, 1.8f);
	TestEqual(TEXT("Regen waits 7 s"), Tuning.RegenDelay, 7.f);
	TestEqual(TEXT("...and heals 7% a second"), Tuning.RegenRate, 0.07f);
	TestEqual(TEXT("Live() with nothing set gives the defaults (chase)"), FFightTuning::Live().ChaseScale, Tuning.ChaseScale);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightTokensTest, "Looter.Fight.Tokens",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightTokensTest::RunTest(const FString& Parameters)
{
	const float Forget = 0.5f;
	// How many at once: three of four get a turn, the fourth waits.
	{
		FFightTokens Tokens;
		TestTrue(TEXT("The first asks: a turn"), Tokens.TryTake(1, 0.0, 3, Forget));
		TestTrue(TEXT("...the second"), Tokens.TryTake(2, 0.0, 3, Forget));
		TestTrue(TEXT("...the third"), Tokens.TryTake(3, 0.0, 3, Forget));
		TestFalse(TEXT("...the fourth waits"), Tokens.TryTake(4, 0.0, 3, Forget));
		TestEqual(TEXT("Three held"), Tokens.NumHeld(), 3);
		TestTrue(TEXT("...the fourth in line"), Tokens.IsWaiting(4));
		TestTrue(TEXT("Asking again with one: still yes"), Tokens.TryTake(2, 0.1, 3, Forget));
		TestEqual(TEXT("...still three"), Tokens.NumHeld(), 3);
		// Gone (dead): the next one in line gets its turn.
		Tokens.Forget(1);
		TestEqual(TEXT("A dead one's turn goes"), Tokens.NumHeld(), 2);
		TestTrue(TEXT("...and the one waiting gets it"), Tokens.TryTake(4, 0.2, 3, Forget));
	}

	// Fairness: the longest waiter gets the next turn, whoever asks first; one that stops asking leaves the line.
	{
		FFightTokens Tokens;
		Tokens.TryTake(1, 0.0, 1, Forget);
		TestFalse(TEXT("A waits (since 0.1)"), Tokens.TryTake(10, 0.1, 1, Forget));
		TestFalse(TEXT("B waits (since 0.2)"), Tokens.TryTake(20, 0.2, 1, Forget));
		TestTrue(TEXT("The holder gives its turn back"), Tokens.Release(1, 0.3, 0.f));
		TestFalse(TEXT("B asks first: no, A has waited longer"), Tokens.TryTake(20, 0.3, 1, Forget));
		TestTrue(TEXT("...A asks: its turn"), Tokens.TryTake(10, 0.3, 1, Forget));
		Tokens.Release(10, 0.4, 0.f);
		// C has waited longest but stopped asking (staggered, mid-hop) longer than Forget: B goes.
		FFightTokens Line;
		Line.TryTake(1, 0.0, 1, Forget);
		Line.TryTake(30, 0.1, 1, Forget);
		Line.TryTake(20, 0.2, 1, Forget);
		Line.Release(1, 0.3, 0.f);
		TestFalse(TEXT("C still in line at 0.5 s: B waits"), Line.TryTake(20, 0.5, 1, Forget));
		TestTrue(TEXT("C silent for over half a second: B's turn"), Line.TryTake(20, 0.7, 1, Forget));
		TestFalse(TEXT("...C left the line"), Line.IsWaiting(30));
	}

	// The rest after an attack: no turn, not even a place in line, until it's over.
	{
		FFightTokens Tokens;
		Tokens.TryTake(1, 0.0, 3, Forget);
		Tokens.Release(1, 1.0, 0.6f);
		TestTrue(TEXT("Resting"), Tokens.IsResting(1, 1.3));
		TestFalse(TEXT("...asks at 1.3 s: no"), Tokens.TryTake(1, 1.3, 3, Forget));
		TestFalse(TEXT("...and not put in line"), Tokens.IsWaiting(1));
		TestTrue(TEXT("...asks at 1.7 s: its turn"), Tokens.TryTake(1, 1.7, 3, Forget));
		// No rest: it may ask again at once.
		Tokens.Release(1, 2.0, 0.f);
		TestTrue(TEXT("Given back with no rest: again at once"), Tokens.TryTake(1, 2.0, 3, Forget));
	}

	// A staggered holder: its turn back, but the head of the line; a long knock lets the next one in.
	{
		FFightTokens Tokens;
		Tokens.TryTake(1, 0.0, 1, Forget);
		TestFalse(TEXT("Another waits"), Tokens.TryTake(2, 0.05, 1, Forget));
		TestTrue(TEXT("Staggered: its turn given back"), Tokens.ReleaseKeepTurn(1, 0.1));
		TestEqual(TEXT("...none held"), Tokens.NumHeld(), 0);
		TestFalse(TEXT("The other asks: no, the staggered one keeps its place"), Tokens.TryTake(2, 0.2, 1, Forget));
		TestTrue(TEXT("Come to: its turn back"), Tokens.TryTake(1, 0.4, 1, Forget));
		TestTrue(TEXT("Staggered again"), Tokens.ReleaseKeepTurn(1, 0.5));
		TestTrue(TEXT("...knocked down a long while: the other's turn"), Tokens.TryTake(2, 1.1, 1, Forget));
		TestFalse(TEXT("A creature with no turn has none to give back"), Tokens.ReleaseKeepTurn(7, 1.2));
	}

	// Ranged turns keep apart: none given within the gap of the last.
	{
		FFightTokens Tokens;
		TestTrue(TEXT("A spit"), Tokens.TryTake(1, 0.0, 2, Forget, 0.35f));
		TestFalse(TEXT("...another 0.1 s later: not yet"), Tokens.TryTake(2, 0.1, 2, Forget, 0.35f));
		TestTrue(TEXT("...0.4 s later: yes"), Tokens.TryTake(2, 0.4, 2, Forget, 0.35f));
	}

	// A held turn's upkeep at each look.
	{
		using namespace FightRules;
		FHolderLook Look;
		Look.Distance = 300.f;
		Look.LeaveDistance = 1150.f;
		Look.HeldSeconds = 1.f;
		TestTrue(TEXT("Fighting, near, held a second: keeps it"), CheckHolder(Look, 3.f) == ETokenCheck::Keep);
		Look.HeldSeconds = 3.2f;
		TestTrue(TEXT("Held past 3 s: taken back"), CheckHolder(Look, 3.f) == ETokenCheck::TakeBack);
		Look.HeldSeconds = 1.f;
		Look.bHeldByHit = true;
		TestTrue(TEXT("Staggered or knocked: given back, its place kept"), CheckHolder(Look, 3.f) == ETokenCheck::KeepTurn);
		Look.bHeldByHit = false;
		Look.Distance = 1500.f;
		TestTrue(TEXT("Gone out of reach: given back"), CheckHolder(Look, 3.f) == ETokenCheck::Return);
		Look.bAttacking = true;
		TestTrue(TEXT("...not while it's in its attack (a lunge)"), CheckHolder(Look, 3.f) == ETokenCheck::Keep);
		Look.bFighting = false;
		TestTrue(TEXT("Dead, calm or after someone else: given back"), CheckHolder(Look, 3.f) == ETokenCheck::Return);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightRingTest, "Looter.Fight.Ring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightRingTest::RunTest(const FString& Parameters)
{
	using namespace FightRules;
	TestFalse(TEXT("Two hunting: no ring"), UsesRing(2, 3));
	TestTrue(TEXT("Three: a ring"), UsesRing(3, 3));
	TestEqual(TEXT("A spider waits on the ring's 4.8 m"), RingReach(480.f, 220.f, 80.f), 480.f);
	TestEqual(TEXT("A slime past its 4.5 m leap"), RingReach(480.f, 450.f, 80.f), 530.f);
	TestEqual(TEXT("It asks for its turn within 1.6 rings"), ApproachDistance(480.f, 1.6f), 768.f, 0.01f);
	TestEqual(TEXT("Slots round the circle"), SlotBearing(2, 8, 10.f), 100.f, 0.01f);
	TestEqual(TEXT("...past half way, negative"), SlotBearing(5, 8, 0.f), -135.f, 0.01f);
	TestEqual(TEXT("Straight ahead is front"), Frontness(30.f, 30.f), 1.f, 1e-4f);
	TestEqual(TEXT("...straight behind is not"), Frontness(-150.f, 30.f), 0.f, 1e-4f);
	TestEqual(TEXT("...beside, half"), Frontness(120.f, 30.f), 0.5f, 1e-4f);

	FRingLayout Layout;
	// Spread: eight crowding in from one side take all eight slots.
	{
		TArray<FRingAsk> Asks;
		Asks.Init(FRingAsk(), 8);
		const TArray<int32> Slots = AssignRingSlots(Asks, Layout);
		TSet<int32> Taken;
		Taken.Append(Slots);
		TestEqual(TEXT("Eight from one bearing: eight slots"), Taken.Num(), 8);
		TestFalse(TEXT("...none without"), Taken.Contains(INDEX_NONE));
	}
	// Each takes the free slot nearest its way in.
	{
		TArray<FRingAsk> Asks;
		for (const float Bearing : { 90.f, -90.f, 180.f })
		{
			FRingAsk Ask;
			Ask.Bearing = Bearing;
			Asks.Add(Ask);
		}
		const TArray<int32> Slots = AssignRingSlots(Asks, Layout);
		TestTrue(TEXT("From the left, right and behind: those slots"), Slots[0] == 2 && Slots[1] == 6 && Slots[2] == 4);
	}
	// Beside or behind the player's view rather than in front: between the front slot and its neighbour, it takes the neighbour.
	{
		FRingAsk Ask;
		Ask.Bearing = 22.5f;
		Layout.ViewYaw = 0.f;
		TestEqual(TEXT("Halfway off the front: the slot away from it"), AssignRingSlots({ Ask }, Layout)[0], 1);
		Layout.BehindBias = 0.f;
		Ask.Bearing = 20.f;
		TestEqual(TEXT("...with no bias, the nearer (the front)"), AssignRingSlots({ Ask }, Layout)[0], 0);
		Layout.BehindBias = 0.35f;
	}
	// Its own slot is kept between looks unless another is clearly better.
	{
		FRingAsk Ask;
		Ask.Bearing = 100.f;
		Ask.CurrentSlot = 3;
		Layout.BehindBias = 0.f;
		TestEqual(TEXT("Nearer slot 2, but it keeps 3"), AssignRingSlots({ Ask }, Layout)[0], 3);
		Ask.Bearing = 80.f;
		TestEqual(TEXT("...gone well past it: it moves on"), AssignRingSlots({ Ask }, Layout)[0], 2);
		Layout.BehindBias = 0.35f;
	}
	// Shut slots (a wall, a drop) and slots off its ground are never taken.
	{
		FRingAsk Ask;
		Ask.Bearing = 0.f;
		Layout.Open = ~1u;
		TestTrue(TEXT("The front shut: another"), AssignRingSlots({ Ask }, Layout)[0] != 0);
		Ask.Allowed = 1u << 5;
		TestEqual(TEXT("Only slot 5 on its ground: slot 5"), AssignRingSlots({ Ask }, Layout)[0], 5);
		Layout.Open = 0u;
		TestEqual(TEXT("All shut: none"), AssignRingSlots({ Ask }, Layout)[0], static_cast<int32>(INDEX_NONE));
		Layout.Open = MAX_uint32;
	}
	// More than the slots: the rest double up, none left without.
	{
		TArray<FRingAsk> Asks;
		Asks.Init(FRingAsk(), 10);
		const TArray<int32> Slots = AssignRingSlots(Asks, Layout);
		TMap<int32, int32> Counts;
		for (const int32 Slot : Slots)
		{
			++Counts.FindOrAdd(Slot);
		}
		int32 Most = 0;
		for (const TPair<int32, int32>& Each : Counts)
		{
			Most = FMath::Max(Most, Each.Value);
		}
		TestFalse(TEXT("Ten on eight slots: everyone has one"), Counts.Contains(INDEX_NONE));
		TestEqual(TEXT("...two at most on any"), Most, 2);
	}

	// A feint: in and back, deepest half way through.
	TestEqual(TEXT("Not feinting: on the ring"), FeintRadius(480.f, -1.f, 0.5f, 0.55f), 480.f);
	TestEqual(TEXT("Half way: at 55%"), FeintRadius(480.f, 0.25f, 0.5f, 0.55f), 264.f, 0.01f);
	TestEqual(TEXT("Done: back on the ring"), FeintRadius(480.f, 0.5f, 0.5f, 0.55f), 480.f);

	// Round to its slot, never across the player.
	{
		const FVector Target = FVector::ZeroVector;
		const FVector Here(500.0, 0.0, 0.0);
		const FVector Way = OrbitWay(Here, Target, FVector(-500.0, 10.0, 0.0));
		TestTrue(TEXT("To the far side: round, not through them"), FMath::Abs(FVector::DotProduct(Way, FVector(-1.0, 0.0, 0.0))) < 0.2);
		TestTrue(TEXT("...the short way (its slot's side)"), Way.Y > 0.9);
		const FVector Near = OrbitWay(Here, Target, FVector(480.0, 30.0, 0.0));
		TestTrue(TEXT("Its slot beside it: straight there"), Near.Equals(FVector(-20.0, 30.0, 0.0).GetSafeNormal(), 1e-3));
		const FVector Closing = OrbitWay(FVector(800.0, 0.0, 0.0), Target, FVector(0.0, 480.0, 0.0));
		TestTrue(TEXT("Farther out than its slot: it closes in as it goes round"), Closing.X < 0.0 && Closing.Y > 0.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightRangedTest, "Looter.Fight.Ranged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightRangedTest::RunTest(const FString& Parameters)
{
	using namespace FightRules;
	const TArray<float> Nobody;
	bool bAny = false;
	const TArray<float> Astride = { 170.f, -170.f };
	TestTrue(TEXT("The pack's side: the mean of its bearings (astride south, still south)"), AngleApart(MeanBearing(Astride, bAny), 180.f) < 0.01f);
	TestTrue(TEXT("...there is one"), bAny);
	MeanBearing(Nobody, bAny);
	TestFalse(TEXT("No pack: none"), bAny);

	// Off the melee pack's side by 50 degrees or more, the nearest such first.
	{
		const TArray<float> Bearings = RangedBearings(10.f, true, 0.f, 50.f, false, Nobody);
		TestTrue(TEXT("Candidates"), Bearings.Num() >= 4);
		bool bAllOff = true;
		for (const float Bearing : Bearings)
		{
			bAllOff &= AngleApart(Bearing, 0.f) >= 50.f - 0.01f;
		}
		TestTrue(TEXT("...every one 50 degrees or more off the pack"), bAllOff);
		TestEqual(TEXT("...the nearest to it first (50, its side)"), Bearings[0], 50.f, 0.01f);
	}
	TestEqual(TEXT("Already well off the pack: it stays"), RangedBearings(90.f, true, 0.f, 50.f, false, Nobody)[0], 90.f, 0.01f);
	{
		const TArray<float> Bearings = RangedBearings(90.f, true, 0.f, 50.f, true, Nobody);
		bool bMoved = true;
		for (const float Bearing : Bearings)
		{
			bMoved &= AngleApart(Bearing, 90.f) >= 30.f - 0.01f;
		}
		TestTrue(TEXT("Moving on after a shot: 30 degrees or more from where it was"), Bearings.Num() > 0 && bMoved);
	}
	// The pack at 90, this one at 0 (far enough off), another spitter at 5: it goes round to the pack's far side (140).
	const TArray<float> OtherSpitter = { 5.f };
	TestEqual(TEXT("Another spitter beside it: the pack's other side first"), RangedBearings(0.f, true, 90.f, 50.f, false, OtherSpitter)[0], 140.f, 0.01f);
	TestEqual(TEXT("No pack: where it is"), RangedBearings(33.f, false, 0.f, 50.f, false, Nobody)[0], 33.f, 0.01f);

	TestEqual(TEXT("Its band (9 to 16 m): kept in from the near edge"), RangedDistance(500.f, 900.f, 1600.f), 1005.f, 0.01f);
	TestEqual(TEXT("...and the far"), RangedDistance(2000.f, 900.f, 1600.f), 1530.f, 0.01f);
	TestEqual(TEXT("...where it is, inside"), RangedDistance(1200.f, 900.f, 1600.f), 1200.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightScalesTest, "Looter.Fight.Scales",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightScalesTest::RunTest(const FString& Parameters)
{
	using namespace FightRules;
	const FFightTuning Tuning;
	TestTrue(TEXT("A boss keeps its own fight"), KeepsOwnFight(ECreatureRank::Boss, false));
	TestTrue(TEXT("...a Legendary monster too"), KeepsOwnFight(ECreatureRank::Legendary, false));
	TestTrue(TEXT("...and a creature a boss fight is built round"), KeepsOwnFight(ECreatureRank::Basic, true));
	TestFalse(TEXT("A Regular takes turns"), KeepsOwnFight(ECreatureRank::Basic, false));
	TestFalse(TEXT("...an Ethereal too"), KeepsOwnFight(ECreatureRank::Ethereal, false));

	// Chase: a walking player is caught, a sprinting one gets away (the contract: spider ~537 against 459 and 711).
	const float Walk = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::MoveScale;
	const float Sprint = Walk * GetDefault<UPlayerLocomotionComponent>()->SprintSpeedMultiplier;
	const float Spider = ChaseSpeedFor(GetDefault<ASpiderCreature>()->ChaseSpeed, false, Tuning.ChaseScale);
	TestEqual(TEXT("A spider chases at 1.3 times its own"), Spider, GetDefault<ASpiderCreature>()->ChaseSpeed * 1.3f, 0.01f);
	TestTrue(FString::Printf(TEXT("...%.0f: over the walk (%.0f), under the sprint (%.0f)"), Spider, Walk, Sprint), Spider > Walk && Spider < Sprint);
	TestEqual(TEXT("A boss's chase is its own"), ChaseSpeedFor(430.f, true, Tuning.ChaseScale), 430.f);
	TestEqual(TEXT("A regular creature hits 1.8 times as hard"), DamageScaleFor(false, Tuning.DamageScale), 1.8f);
	TestEqual(TEXT("...a boss as it always did"), DamageScaleFor(true, Tuning.DamageScale), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFightDirectorTest, "Looter.Fight.Director",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFightDirectorTest::RunTest(const FString& Parameters)
{
	FDirectorLevel Level;
	if (!TestTrue(TEXT("Test level made"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	UFightDirectorSubsystem* Director = World->GetSubsystem<UFightDirectorSubsystem>();
	FEditorScriptExecutionGuard RunActorEvents;
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	TArray<ACreatureBase*> Spiders;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const float Radians = FMath::DegreesToRadians(72.f * static_cast<float>(Index));
		Spiders.Add(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(),
			FVector(FMath::Cos(Radians) * 700.f, FMath::Sin(Radians) * 700.f, 0.f)));
	}
	if (!TestNotNull(TEXT("A director"), Director) || !TestNotNull(TEXT("Player stand-in"), Player)
		|| !TestFalse(TEXT("Five spiders"), Spiders.Contains(nullptr)))
	{
		return false;
	}
	for (ACreatureBase* Spider : Spiders)
	{
		Spider->DevPutInState(ECreatureState::Chase, Player);
		TestTrue(TEXT("A regular spider takes turns"), Spider->UsesFightDirector());
	}

	// Three turns; the fourth waits.
	TestTrue(TEXT("First: a turn"), Director->TryTakeToken(*Spiders[0], *Player, EFightRole::Melee));
	TestTrue(TEXT("...second"), Director->TryTakeToken(*Spiders[1], *Player, EFightRole::Melee));
	TestTrue(TEXT("...third"), Director->TryTakeToken(*Spiders[2], *Player, EFightRole::Melee));
	TestFalse(TEXT("...fourth waits"), Director->TryTakeToken(*Spiders[3], *Player, EFightRole::Melee));
	TestEqual(TEXT("Three held"), Director->CountTokens(*Player, EFightRole::Melee), 3);

	// Killed: its turn goes with it, and the one waiting gets it.
	BossTestWorld::Kill(Spiders[0]);
	TestTrue(TEXT("The first one is dead"), Spiders[0]->IsDead());
	TestFalse(TEXT("...holding nothing"), Director->HoldsToken(*Spiders[0]));
	TestTrue(TEXT("...the fourth's turn now"), Director->TryTakeToken(*Spiders[3], *Player, EFightRole::Melee));

	// Hit off its turn (a stagger): given back, its place kept ahead of the fifth.
	Director->GiveBackForHit(*Spiders[1]);
	TestEqual(TEXT("Staggered: two held"), Director->CountTokens(*Player, EFightRole::Melee), 2);
	TestFalse(TEXT("...the fifth asks: no, the staggered one is first in line"), Director->TryTakeToken(*Spiders[4], *Player, EFightRole::Melee));
	TestTrue(TEXT("...come to, it has its turn back"), Director->TryTakeToken(*Spiders[1], *Player, EFightRole::Melee));

	// Out of the fight (walking home): its turn goes back.
	Spiders[2]->DevPutInState(ECreatureState::Return);
	TestFalse(TEXT("Walking home: no turn"), Director->HoldsToken(*Spiders[2]));

	// The ring: four alive and chasing (one walked home) hold slots round the player, each its own, 4.8 m out.
	Spiders[2]->DevPutInState(ECreatureState::Chase, Player);
	Director->RefreshNow();
	TestTrue(TEXT("Four hunting: a ring"), Director->IsRingUp(*Player));
	TSet<int32> Slots;
	for (int32 Index = 1; Index < 5; ++Index)
	{
		const int32 Slot = Director->GetSlot(*Spiders[Index], *Player);
		TestTrue(FString::Printf(TEXT("Spider %d has a slot"), Index), Slot != INDEX_NONE);
		Slots.Add(Slot);
	}
	TestEqual(TEXT("...each its own"), Slots.Num(), 4);
	FVector Spot;
	if (TestTrue(TEXT("A waiting one's spot"), Director->GetWaitSpot(*Spiders[4], *Player, Spot)))
	{
		TestEqual(TEXT("...on the ring"), static_cast<float>(FVector::Dist2D(Spot, Player->GetActorLocation())), Director->GetRingReach(*Spiders[4]), 1.f);
	}

	// Two left: no ring, they just come in.
	BossTestWorld::Kill(Spiders[1]);
	BossTestWorld::Kill(Spiders[2]);
	Director->RefreshNow();
	TestFalse(TEXT("Two hunting: no ring"), Director->IsRingUp(*Player));
	TestFalse(TEXT("...no spot to wait at"), Director->GetWaitSpot(*Spiders[4], *Player, Spot));

	// Bosses and Legendary monsters keep their own fights.
	UBossComponent* Boss = BossTestSpider::Spawn(World, FVector(-2000.0, 0.0, 0.0), 0.f);
	const ACreatureBase* BossBody = Boss ? Cast<ACreatureBase>(Boss->GetOwner()) : nullptr;
	if (TestNotNull(TEXT("The test boss"), BossBody))
	{
		TestFalse(TEXT("A boss takes no turns"), BossBody->UsesFightDirector());
		TestTrue(TEXT("...and keeps its own speed"), FMath::IsNearlyEqual(BossBody->GetChaseSpeedNow(), BossBody->ChaseSpeed));
	}
	ACreatureBase* Legend = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(2000.0, 0.0, 0.0));
	if (TestNotNull(TEXT("A spider"), Legend))
	{
		Legend->SetRank(ECreatureRank::Legendary);
		TestFalse(TEXT("A Legendary monster takes no turns"), Legend->UsesFightDirector());
		TestTrue(TEXT("...a regular spider chases faster in play"), Spiders[4]->GetChaseSpeedNow() > Spiders[4]->ChaseSpeed);
	}
	return true;
}

#endif
