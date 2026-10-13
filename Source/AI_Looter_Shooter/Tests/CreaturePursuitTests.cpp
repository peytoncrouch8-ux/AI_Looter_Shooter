#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreaturePursuit.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "AIController.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// Creatures keep after the player until there's a real reason to stop (the user's call, 2026-10-12; FCreaturePursuit,
// CreatureBasePursuit.cpp): out of sight they go where they lost them and sniff round, where they can't get to them they wait
// watching, and they give up only when the player is dead or gone, sheltered, in a scene, lost for long, or out of their
// reach past their patience. Their lock-on radius is a fifth bigger. The rules first, then creatures in test levels.
// (The safe zone and the hunting ground's edge are in EncounterPlayTests.cpp, Looter.Encounters.SafeZone.)

namespace
{
	constexpr float LookEvery = 0.2f;

	/** A look at a target standing at Prey, in sight, a spider's hold (165 cm) away or more. */
	FCreaturePursuit::FLook SeenAt(const FVector& Here, const FVector& Prey)
	{
		FCreaturePursuit::FLook Look;
		Look.Interval = LookEvery;
		Look.Here = Here;
		Look.Prey = Prey;
		Look.bSeen = true;
		Look.HoldDistance = 165.f;
		Look.bFarOff = FVector::Dist2D(Here, Prey) > Look.HoldDistance;
		Look.bBeyondSenses = FVector::Dist(Here, Prey) > 3120.0;
		return Look;
	}

	/** Seconds of looks, each the same: the step at the end. */
	EPursuitStep LookFor(FCreaturePursuit& Pursuit, const FCreaturePursuit::FRules& Rules, const FCreaturePursuit::FLook& Look, float Seconds)
	{
		EPursuitStep Step = Pursuit.GetStep();
		for (float Time = 0.f; Time < Seconds - KINDA_SMALL_NUMBER; Time += Look.Interval)
		{
			Step = Pursuit.Look(Look, Rules);
		}
		return Step;
	}

	/** A creature's frames for Seconds (a look every other frame), as play ticks it (a test level never moves it). */
	void Run(ACreatureBase& Creature, float Seconds)
	{
		constexpr float Frame = 0.1f;
		for (float Time = 0.f; Time < Seconds - KINDA_SMALL_NUMBER; Time += Frame)
		{
			Creature.ConsumeMovementInputVector();
			Creature.Tick(Frame);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreaturePursuitRulesTest, "Looter.Creatures.Pursuit.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreaturePursuitRulesTest::RunTest(const FString& Parameters)
{
	const FCreaturePursuit::FRules Rules;
	const FVector Here = FVector::ZeroVector;
	const FVector Prey(2000.0, 0.0, 0.0);

	// Seen, it hunts on and on: a minute after the player, who runs about 60 m off, it's still after them.
	{
		FCreaturePursuit Pursuit;
		Pursuit.Begin(Prey);
		EPursuitStep Step = Pursuit.GetStep();
		for (int32 Look = 0; Look < 300; ++Look)
		{
			const FVector Running(6000.0 * FMath::Cos(Look * 0.013), 6000.0 * FMath::Sin(Look * 0.013), 0.0);
			Step = Pursuit.Look(SeenAt(Here, Running), Rules);
		}
		TestTrue(TEXT("Seen, a minute on, it still hunts"), Step == EPursuitStep::Hunt && Pursuit.GetEnd() == EPursuitEnd::None);
	}

	// Out of sight a moment it keeps coming where it heard them go; then it tracks to that spot, and sniffs round it.
	{
		FCreaturePursuit Pursuit;
		Pursuit.Begin(Prey);
		FCreaturePursuit::FLook Hidden = SeenAt(Here, FVector(2000.0, 600.0, 0.0));
		Hidden.bSeen = false;
		TestTrue(TEXT("Unseen 1.8 s: still hunting"), LookFor(Pursuit, Rules, Hidden, 1.8f) == EPursuitStep::Hunt);
		TestTrue(TEXT("...following where they went"), Pursuit.GetLastKnown().Equals(FVector(2000.0, 600.0, 0.0)));
		TestTrue(TEXT("Unseen past its grace: it goes where it last knew them"), LookFor(Pursuit, Rules, Hidden, 0.6f) == EPursuitStep::Track);
		Hidden.Prey = FVector(2500.0, 2500.0, 0.0);
		Pursuit.Look(Hidden, Rules);
		TestTrue(TEXT("...the spot it heard them at last, not where they've crept to since"), Pursuit.GetLastKnown().Equals(FVector(2000.0, 600.0, 0.0)));
		FCreaturePursuit::FLook There = Hidden;
		There.Here = Pursuit.GetLastKnown();
		TestTrue(TEXT("At the spot: it sniffs round"), LookFor(Pursuit, Rules, There, LookEvery) == EPursuitStep::Sniff);
		TestTrue(TEXT("Seen again: the hunt is on"), Pursuit.Look(SeenAt(There.Here, Hidden.Prey), Rules) == EPursuitStep::Hunt);
	}

	// Lost: unseen, past its nose, its sniff at the spot done and 13 s without them; not a moment before.
	{
		FCreaturePursuit Pursuit;
		Pursuit.Begin(Prey);
		FCreaturePursuit::FLook Gone = SeenAt(Here, FVector(5000.0, 0.0, 0.0));
		Gone.bSeen = false;
		Gone.bBeyondSenses = true;
		TestTrue(TEXT("Unseen and far 2.4 s: tracking"), LookFor(Pursuit, Rules, Gone, 2.4f) == EPursuitStep::Track);
		// It gets to the spot (a test moves it there).
		Gone.Here = Pursuit.GetLastKnown();
		TestTrue(TEXT("Unseen and far, 12.6 s: still searching"), LookFor(Pursuit, Rules, Gone, 10.2f) == EPursuitStep::Sniff);
		TestTrue(TEXT("13 s: it has lost them"), LookFor(Pursuit, Rules, Gone, 0.8f) == EPursuitStep::GiveUp && Pursuit.GetEnd() == EPursuitEnd::Lost);
		TestTrue(TEXT("...and stays given up"), Pursuit.Look(SeenAt(Here, Prey), Rules) == EPursuitStep::GiveUp);
	}

	// Unseen but near (inside its lock-on radius): its sniff done, it catches their scent and tracks again; only after
	// 30 s without them is it lost.
	{
		FCreaturePursuit Pursuit;
		Pursuit.Begin(Prey);
		FCreaturePursuit::FLook Near = SeenAt(Here, Prey);
		Near.bSeen = false;
		LookFor(Pursuit, Rules, Near, 2.2f);
		FCreaturePursuit::FLook There = Near;
		There.Here = Pursuit.GetLastKnown();
		There.Prey = FVector(2000.0, 1500.0, 0.0);
		TestTrue(TEXT("At the spot, sniffing"), LookFor(Pursuit, Rules, There, LookEvery) == EPursuitStep::Sniff);
		TestTrue(TEXT("Its sniff done, they're near: it tracks their scent"), LookFor(Pursuit, Rules, There, Rules.SniffSeconds + LookEvery) == EPursuitStep::Track
			&& Pursuit.GetLastKnown().Equals(There.Prey));
		const EPursuitStep At29 = LookFor(Pursuit, Rules, There, 29.f - Pursuit.GetUnseen());
		TestTrue(TEXT("Near and unseen, 29 s: still searching"), At29 == EPursuitStep::Track || At29 == EPursuitStep::Sniff);
		TestTrue(TEXT("30 s: lost wherever they are"), LookFor(Pursuit, Rules, There, 1.2f) == EPursuitStep::GiveUp && Pursuit.GetEnd() == EPursuitEnd::Lost);
	}

	// The reasons that end it at once.
	auto EndsAt = [&](TFunctionRef<void(FCreaturePursuit::FLook&)> Make, EPursuitEnd Expected, const TCHAR* What)
	{
		FCreaturePursuit Pursuit;
		Pursuit.Begin(Prey);
		FCreaturePursuit::FLook Look = SeenAt(Here, Prey);
		Make(Look);
		TestTrue(What, Pursuit.Look(Look, Rules) == EPursuitStep::GiveUp && Pursuit.GetEnd() == Expected);
	};
	EndsAt([](FCreaturePursuit::FLook& Look) { Look.bGone = true; }, EPursuitEnd::Gone, TEXT("Dead or gone: it gives up at once"));
	EndsAt([](FCreaturePursuit::FLook& Look) { Look.bSheltered = true; }, EPursuitEnd::Sheltered, TEXT("Over the salt line: at once"));
	EndsAt([](FCreaturePursuit::FLook& Look) { Look.bInScene = true; }, EPursuitEnd::Scene, TEXT("A scene: at once"));

	// Off its ground: it waits at its edge watching; back on, after them at once; its patience is ten seconds a hunt.
	{
		FCreaturePursuit Pursuit;
		Pursuit.Begin(Prey);
		FCreaturePursuit::FLook Off = SeenAt(Here, Prey);
		Off.bOffGround = true;
		TestTrue(TEXT("Off its ground: it prowls its edge"), LookFor(Pursuit, Rules, Off, 6.f) == EPursuitStep::Prowl && Pursuit.IsProwlingOffGround());
		TestTrue(TEXT("Back on: the hunt is on"), Pursuit.Look(SeenAt(Here, Prey), Rules) == EPursuitStep::Hunt);
		TestTrue(TEXT("Off again: still watching, 9.8 s in all"), LookFor(Pursuit, Rules, Off, 3.8f) == EPursuitStep::Prowl);
		TestTrue(TEXT("10 s: it gives up"), LookFor(Pursuit, Rules, Off, 0.4f) == EPursuitStep::GiveUp && Pursuit.GetEnd() == EPursuitEnd::OffGround);

		FCreaturePursuit Shot;
		Shot.Begin(Prey);
		LookFor(Shot, Rules, Off, 9.f);
		Shot.NoteHit(Prey);
		TestTrue(TEXT("Shot from past its edge, it watches on"), LookFor(Shot, Rules, Off, 9.f) == EPursuitStep::Prowl);
	}

	// Out of its reach: up on a rock over it for 4 s, or making its way in without getting nearer for 10 s while they stand.
	{
		FCreaturePursuit Pursuit;
		Pursuit.Begin(Prey);
		FCreaturePursuit::FLook Up = SeenAt(Here, FVector(100.0, 0.0, 450.0));
		Up.bOverhead = true;
		TestTrue(TEXT("Up over it 3.8 s: still hunting"), LookFor(Pursuit, Rules, Up, 3.8f) == EPursuitStep::Hunt);
		TestTrue(TEXT("4 s: out of its reach, it waits there"), LookFor(Pursuit, Rules, Up, 0.4f) == EPursuitStep::Prowl && !Pursuit.IsProwlingOffGround());
		TestTrue(TEXT("Down to it: the hunt is on"), Pursuit.Look(SeenAt(Here, FVector(100.0, 0.0, 0.0)), Rules) == EPursuitStep::Hunt);
		Pursuit.NoteReached();
		LookFor(Pursuit, Rules, Up, 4.f);
		TestTrue(TEXT("Up again: it waits its patience out"), LookFor(Pursuit, Rules, Up, 9.4f) == EPursuitStep::Prowl);
		TestTrue(TEXT("...then gives up, out of its reach"), LookFor(Pursuit, Rules, Up, 1.f) == EPursuitStep::GiveUp
			&& Pursuit.GetEnd() == EPursuitEnd::OutOfReach);

		FCreaturePursuit Stuck;
		Stuck.Begin(Prey);
		const FCreaturePursuit::FLook Standing = SeenAt(Here, Prey);
		TestTrue(TEXT("Getting no nearer to one who stands, 9.6 s: still hunting"), LookFor(Stuck, Rules, Standing, 9.6f) == EPursuitStep::Hunt);
		TestTrue(TEXT("10 s: out of its reach"), LookFor(Stuck, Rules, Standing, 0.8f) == EPursuitStep::Prowl);

		FCreaturePursuit Kited;
		Kited.Begin(Prey);
		EPursuitStep Step = Kited.GetStep();
		for (int32 Look = 0; Look < 150; ++Look)
		{
			// The player strafes 4 m a second round it: never stuck, they're moving.
			Step = Kited.Look(SeenAt(Here, FVector(2000.0, 80.0 * (Look % 40) - 1600.0, 0.0)), Rules);
		}
		TestTrue(TEXT("A player on the move is never out of its reach for that"), Step == EPursuitStep::Hunt);

		FCreaturePursuit Running;
		Running.Begin(Prey);
		FCreaturePursuit::FLook Away = SeenAt(Here, Prey);
		Away.bBreakingOff = true;
		Away.bSeen = false;
		TestTrue(TEXT("Breaking off, out of sight: it runs on, neither stuck nor searching"), LookFor(Running, Rules, Away, 12.f) == EPursuitStep::Hunt);
	}

	// One it gave up on out of its reach is let be a while, while they stay put.
	{
		FCreaturePursuit Pursuit;
		Pursuit.Spurn(7, Prey, 20.0);
		TestTrue(TEXT("Spurned: the same player, still there"), Pursuit.IsSpurned(7, Prey + FVector(200.0, 0.0, 0.0), 5.0, Rules.ResumeShift));
		TestFalse(TEXT("...not once they've moved off"), Pursuit.IsSpurned(7, Prey + FVector(900.0, 0.0, 0.0), 5.0, Rules.ResumeShift));
		TestFalse(TEXT("...nor after a while"), Pursuit.IsSpurned(7, Prey, 21.0, Rules.ResumeShift));
		TestFalse(TEXT("...nor anyone else"), Pursuit.IsSpurned(8, Prey, 5.0, Rules.ResumeShift));
	}

	// The project's numbers are these rules' (Project Settings > Game > Encounters, "Pursuit").
	const FCreaturePursuit::FRules FromSettings = UEncounterSettings::Get().MakePursuitRules();
	TestTrue(TEXT("Its grace is a couple of seconds, not a blink"), FromSettings.UnseenGrace >= 1.5f);
	TestTrue(TEXT("Lost only after 12 to 15 s unseen and far"), FromSettings.LostSeconds >= 12.f && FromSettings.LostSeconds <= 15.f);
	TestTrue(TEXT("...and after its sniff"), FromSettings.SniffSeconds > 0.f && FromSettings.SniffSeconds < FromSettings.LostSeconds);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreaturePursuitRadiiTest, "Looter.Creatures.Pursuit.Radii",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreaturePursuitRadiiTest::RunTest(const FString& Parameters)
{
	// The user's 20% more lock-on radius (2026-10-12), for every kind, their ratios kept: 26 m for most, 18 m for slimes before.
	struct FKind
	{
		const ACreatureBase* Defaults;
		float Before;
		const TCHAR* Name;
	};
	const FKind Kinds[] = {
		{ GetDefault<ASpiderCreature>(), 2600.f, TEXT("Spider") },
		{ GetDefault<ASpitterSpider>(), 2600.f, TEXT("Spitter") },
		{ GetDefault<ASlimeCreature>(), 1800.f, TEXT("Slime") },
		{ GetDefault<AUnpaidCreature>(), 2600.f, TEXT("Unpaid") },
		{ GetDefault<ABellRingerCreature>(), 2600.f, TEXT("Bell-Ringer") },
		{ GetDefault<AGravemotherCreature>(), 2600.f, TEXT("Gravemother") },
	};
	for (const FKind& Kind : Kinds)
	{
		TestNearlyEqual(FString::Printf(TEXT("%s: notices the player 20%% farther"), Kind.Name), Kind.Defaults->AggroRadius, Kind.Before * 1.2f, 0.5f);
		TestTrue(FString::Printf(TEXT("%s: follows well past it before the target is gone"), Kind.Name),
			Kind.Defaults->LoseInterestRadius >= 2.f * Kind.Defaults->AggroRadius);
	}
	TestNearlyEqual(TEXT("A slime still notices at the share of a spider's it did"), GetDefault<ASlimeCreature>()->AggroRadius / GetDefault<ASpiderCreature>()->AggroRadius,
		1800.f / 2600.f, 0.001f);
	TestTrue(TEXT("It hears a hidden player up close, well inside its lock-on radius"), UEncounterSettings::Get().SenseRadius > 0.f
		&& UEncounterSettings::Get().SenseRadius < GetDefault<ASlimeCreature>()->AggroRadius);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreaturePursuitPlayTest, "Looter.Creatures.Pursuit.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreaturePursuitPlayTest::RunTest(const FString& Parameters)
{
	// In a test level (its traces see only what's put there; it never moves a creature itself).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(6000.0, 0.0, 0.0));
	AAIController* Shooter = World->SpawnActor<AAIController>();
	ACreatureBase* Spider = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Its controller"), Shooter) || !TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	Shooter->SetPawn(Player);
	auto Hunting = [Player](const ACreatureBase* Creature)
	{
		return Creature->GetCreatureState() == ECreatureState::Chase && Creature->GetTarget() == Player;
	};
	// Let go for Why: walking home, or settled there already (a test level never moves it, so one still at home settles in
	// the same frame it gives up).
	auto GaveUp = [](const ACreatureBase* Creature, EPursuitEnd Why)
	{
		const ECreatureState Doing = Creature->GetCreatureState();
		return !Creature->GetTarget() && (Doing == ECreatureState::Return || Doing == ECreatureState::Idle) && Creature->GetPursuit().GetEnd() == Why;
	};

	// 60 m off, in sight and running about (once, 55 m away was the end of any chase): 20 s on, it's still after them.
	Spider->AlertTo(Player);
	bool bHuntedThrough = Hunting(Spider);
	for (int32 Second = 0; Second < 20; ++Second)
	{
		Player->SetActorLocation(FVector(6000.0 * FMath::Cos(Second * 0.07), 6000.0 * FMath::Sin(Second * 0.07), 0.0));
		Run(*Spider, 1.f);
		bHuntedThrough &= Hunting(Spider);
	}
	TestTrue(TEXT("60 m off and on the move, 20 s on: still hunting"), bHuntedThrough);

	// Hidden behind a wall 15 m off: a moment's grace, then to the spot it heard them last, sniffing round; 13 s without
	// them, far off, it has lost them and walks home.
	Player->SetActorLocation(FVector(1500.0, 0.0, 0.0));
	Spider->SetActorLocation(FVector(0.0, 0.0, Spider->GetActorLocation().Z));
	Run(*Spider, 0.4f);
	const AStaticMeshActor* Wall = LocomotionTestWorld::SpawnBlock(World, FVector(700.0, 0.0, 0.0), FVector(40.0, 1200.0, 800.0));
	if (!TestNotNull(TEXT("A wall"), Wall) || !TestTrue(TEXT("Seen 15 m off: hunting"), Hunting(Spider)))
	{
		return false;
	}
	Run(*Spider, 1.f);
	Player->SetActorLocation(FVector(1500.0, 300.0, 0.0));
	Run(*Spider, 0.6f);
	TestTrue(TEXT("Behind the wall 1.6 s: still coming (it heard them)"), Hunting(Spider));
	Run(*Spider, 1.f);
	const FCreaturePursuit& Pursuit = Spider->GetPursuit();
	TestTrue(TEXT("Past its grace: searching, going where it lost them"), Spider->GetCreatureState() == ECreatureState::Search
		&& Pursuit.GetStep() == EPursuitStep::Track && Spider->GetTarget() == Player);
	TestTrue(TEXT("...where it heard them last"), FVector::Dist2D(Pursuit.GetLastKnown(), FVector(1500.0, 300.0, 0.0)) < 10.0);
	// They slip away, 40 m off behind the wall; it gets to the spot (walked there: the level doesn't move it).
	Player->SetActorLocation(FVector(-2500.0, 0.0, 0.0));
	Spider->SetActorLocation(FVector(Pursuit.GetLastKnown().X, Pursuit.GetLastKnown().Y, Spider->GetActorLocation().Z));
	Run(*Spider, 0.4f);
	TestTrue(TEXT("At the spot: it sniffs round"), Spider->GetCreatureState() == ECreatureState::Search && Pursuit.GetStep() == EPursuitStep::Sniff);
	Run(*Spider, 12.f - Pursuit.GetUnseen());
	TestTrue(TEXT("12 s unseen: still searching"), Spider->GetCreatureState() == ECreatureState::Search && Spider->GetTarget() == Player);
	Run(*Spider, 1.6f);
	TestTrue(TEXT("13 s unseen and far: it has lost them and walks home"), Spider->GetCreatureState() == ECreatureState::Return
		&& !Spider->GetTarget() && Pursuit.GetEnd() == EPursuitEnd::Lost);

	// A hit always turns it, from beyond its lock-on radius too: a slime 50 m off (it notices at 21.6 m).
	ACreatureBase* Slime = EncounterTestWorld::SpawnCreature(World, ASlimeCreature::StaticClass(), FVector(0.0, -20000.0, 0.0));
	if (TestNotNull(TEXT("A slime"), Slime))
	{
		Player->SetActorLocation(FVector(0.0, -15000.0, 0.0));
		BossTestWorld::Hurt(Slime, 1.f, Shooter);
		TestTrue(TEXT("Shot from 50 m: it comes for the player"), Hunting(Slime));
		// Then they travel far away: gone, it gives up at once.
		Player->SetActorLocation(FVector(0.0, 20000.0, 0.0));
		Run(*Slime, 0.4f);
		TestTrue(TEXT("Travelled 400 m away: gone, it walks home"), GaveUp(Slime, EPursuitEnd::Gone));
	}

	// Up on a rock over it (4 m up, its strike reaches 2.2): no swinging at the rock, it waits there out of its reach, then
	// gives up and lets them be while they stay up there.
	ACreatureBase* Under = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(10000.0, 0.0, 0.0));
	if (TestNotNull(TEXT("Another spider"), Under))
	{
		Player->SetActorLocation(Under->GetActorLocation() + FVector(120.0, 0.0, 400.0));
		Under->AlertTo(Player);
		bool bSwung = false;
		for (int32 Step = 0; Step < 34; ++Step)
		{
			Run(*Under, 0.1f);
			bSwung |= Under->GetCreatureState() == ECreatureState::Attack;
		}
		TestTrue(TEXT("Up over it 3.4 s: still hunting"), Hunting(Under));
		Run(*Under, 1.2f);
		TestTrue(TEXT("4 s: out of its reach, it waits there watching"), Under->GetCreatureState() == ECreatureState::Search
			&& Under->GetPursuit().GetStep() == EPursuitStep::Prowl && Under->GetTarget() == Player);
		TestFalse(TEXT("...never swinging at the rock"), bSwung);
		Run(*Under, UEncounterSettings::Get().ProwlSeconds);
		TestTrue(TEXT("Its patience out: it gives up and walks home"), GaveUp(Under, EPursuitEnd::OutOfReach));
		TestTrue(TEXT("...and lets them be while they stay up there"), Under->GetPursuit().IsSpurned(Player->GetUniqueID(), Player->GetActorLocation(),
			World->GetTimeSeconds(), FCreaturePursuit::FRules().ResumeShift));
	}

	// The player killed: it gives up at once.
	ACreatureBase* Last = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(-10000.0, 0.0, 0.0));
	if (TestNotNull(TEXT("A last spider"), Last))
	{
		Player->SetActorLocation(FVector(-9000.0, 0.0, 0.0));
		Last->AlertTo(Player);
		TestTrue(TEXT("Hunting the player"), Hunting(Last));
		BossTestWorld::Kill(Player);
		const UHealthComponent* Life = Player->FindComponentByClass<UHealthComponent>();
		if (TestTrue(TEXT("The player is dead"), Life && Life->IsDead()))
		{
			Run(*Last, 0.4f);
			TestTrue(TEXT("...so it gives up"), GaveUp(Last, EPursuitEnd::Gone));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreaturePursuitBossTest, "Looter.Creatures.Pursuit.Bosses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreaturePursuitBossTest::RunTest(const FString& Parameters)
{
	// A boss keeps its fight's own rules (its seal, its leash): it lets go of a player 60 m off at once, as creatures used to,
	// and never searches.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ACharacter* Player = nullptr;
	UBossComponent* Boss = BossTestWorld::StartTestFight(*this, WorldWrapper.GetTestWorld(), Player);
	ACreatureBase* Creature = Boss ? Boss->GetCreature() : nullptr;
	if (!Creature || !TestTrue(TEXT("The test boss is after the player"), Creature->GetTarget() == Player))
	{
		return false;
	}
	Player->SetActorLocation(FVector(6000.0, 0.0, 0.0));
	Run(*Creature, 0.4f);
	TestTrue(TEXT("60 m off: it lets go at once, no search"), !Creature->GetTarget() && Creature->GetCreatureState() != ECreatureState::Search);
	return true;
}

#endif
