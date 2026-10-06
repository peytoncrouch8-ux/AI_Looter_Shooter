#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterGroup.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/SafeGround.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// How an encounter plays out: its waves, the story switching it, safe zones and hunting grounds ending a chase, and
// nothing it spawned coming back. The rules it plays by (groups, ground, caps) are in EncounterTests.cpp.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterWavesTest, "Looter.Encounters.Waves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterWavesTest::RunTest(const FString& Parameters)
{
	// When waves come: on an interval, once the last is cleared, or only on a trigger; never past the total.
	TestEqual(TEXT("One wave by default"), EncounterRules::PlannedWaves(1, 0), 1);
	TestEqual(TEXT("Neither waves nor a total: one"), EncounterRules::PlannedWaves(0, 0), 1);
	TestTrue(TEXT("A total alone: as many waves as it allows"), EncounterRules::PlannedWaves(0, 8) > 1000);
	TestFalse(TEXT("Two waves, both started: none left"), EncounterRules::HasWavesLeft(2, 2, 12, 0));
	TestFalse(TEXT("The total reached: none left"), EncounterRules::HasWavesLeft(2, 0, 8, 8));
	TestTrue(TEXT("Every 45 s: due at 45, not before"), EncounterRules::IsWaveDue(true, 45.f, 45.f, false, 3)
		&& !EncounterRules::IsWaveDue(true, 44.f, 45.f, false, 3));
	TestFalse(TEXT("Waiting for a clear: not while any are left"), EncounterRules::IsWaveDue(true, 100.f, 4.f, true, 1));
	TestTrue(TEXT("...then after its interval"), EncounterRules::IsWaveDue(true, 4.f, 4.f, true, 0));
	TestFalse(TEXT("No interval and no waiting: only a trigger brings it"), EncounterRules::IsWaveDue(true, 1000.f, 0.f, false, 0));
	TestFalse(TEXT("No waves left: never"), EncounterRules::IsWaveDue(false, 1000.f, 1.f, false, 0));

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(World);
	EncounterTestWorld::SpawnPlayer(World, FVector(2500.0, 0.0, 0.0));

	// The chapel yard's way: two waves, the second once the first is dead and 4 s have passed, with a Restless one.
	AEncounterSpawner* Yard = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, [](AEncounterSpawner& Setup)
	{
		FEncounterGroup Lieutenant = EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1, ECreatureRank::Rare);
		Lieutenant.FirstWave = 2;
		Lieutenant.LastWave = 2;
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 3), Lieutenant };
		Setup.NumWaves = 2;
		Setup.bWaitForClear = true;
		Setup.WaveInterval = 4.f;
		Setup.SpawnRadius = 1000.f;
		Setup.ActivationRadius = 4000.f;
	});
	if (!TestNotNull(TEXT("The level's encounters"), Encounters) || !TestNotNull(TEXT("Spawner"), Yard))
	{
		return false;
	}
	Yard->UpdateEncounter(0.5f);
	TestTrue(TEXT("The first wave: three Basic spiders"), Yard->GetWavesStarted() == 1 && Yard->NumAlive() == 3
		&& EncounterTestWorld::CountOf(Yard->GetAliveCreatures(), ASpiderCreature::StaticClass(), ECreatureRank::Basic) == 3);
	for (int32 Step = 0; Step < 20; ++Step)
	{
		Yard->UpdateEncounter(0.5f);
	}
	TestEqual(TEXT("Ten seconds on, the first alive: no second"), Yard->GetWavesStarted(), 1);
	EncounterTestWorld::KillAll(*Yard);
	Yard->UpdateEncounter(0.5f);
	Yard->UpdateEncounter(3.f);
	TestEqual(TEXT("Cleared 3.5 s ago: not yet"), Yard->GetWavesStarted(), 1);
	Yard->UpdateEncounter(1.f);
	TestTrue(TEXT("4 s after the clear: the second wave, with its Restless one"), Yard->GetWavesStarted() == 2 && Yard->NumAlive() == 4
		&& EncounterTestWorld::CountOf(Yard->GetAliveCreatures(), ASpiderCreature::StaticClass(), ECreatureRank::Rare) == 1);
	EncounterTestWorld::KillAll(*Yard);
	Yard->UpdateEncounter(0.5f);
	TestTrue(TEXT("Both waves dead: cleared"), Yard->GetState() == EEncounterState::Cleared && Yard->GetKilled() == 7 && Yard->GetTotalSpawned() == 7);
	TestFalse(TEXT("A cleared encounter takes no trigger"), Yard->TriggerWave());

	// A trickle: a slime every 10 s, two alive at most, three in all.
	AEncounterSpawner* Trickle = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, 4000.0, 0.0), [](AEncounterSpawner& Setup)
	{
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASlimeCreature::StaticClass(), 1) };
		Setup.NumWaves = 0;
		Setup.MaxTotal = 3;
		Setup.WaveInterval = 10.f;
		Setup.MaxAlive = 2;
		Setup.SpawnRadius = 800.f;
		Setup.ActivationRadius = 6000.f;
	});
	if (!TestNotNull(TEXT("Spawner"), Trickle))
	{
		return false;
	}
	Trickle->UpdateEncounter(0.5f);
	TestEqual(TEXT("A trickle: one"), Trickle->NumAlive(), 1);
	Trickle->UpdateEncounter(10.f);
	TestEqual(TEXT("10 s on: two"), Trickle->NumAlive(), 2);
	Trickle->UpdateEncounter(10.f);
	TestTrue(TEXT("20 s on: the third waits for room"), Trickle->NumAlive() == 2 && Trickle->NumOwed() == 1 && Trickle->GetWavesStarted() == 3);
	const TArray<ACreatureBase*> SlimesOut = Trickle->GetAliveCreatures();
	if (!TestTrue(TEXT("Slimes out to kill"), SlimesOut.Num() > 0))
	{
		return false;
	}
	BossTestWorld::Kill(SlimesOut[0]);
	Trickle->UpdateEncounter(0.5f);
	TestTrue(TEXT("One killed: the third comes"), Trickle->NumAlive() == 2 && Trickle->NumOwed() == 0);
	Trickle->UpdateEncounter(30.f);
	TestEqual(TEXT("Its total reached: no more"), Trickle->GetTotalSpawned(), 3);
	EncounterTestWorld::KillAll(*Trickle);
	Trickle->UpdateEncounter(0.5f);
	TestTrue(TEXT("All dead: cleared"), Trickle->GetState() == EEncounterState::Cleared);

	// Waves only on a trigger (the Gravemother's call): nothing on approach, a wave each call, then cleared.
	AEncounterSpawner* Brood = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, -4000.0, 0.0), [](AEncounterSpawner& Setup)
	{
		FEncounterGroup Spiderlings = EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 4);
		Spiderlings.BodyScale = 0.45f;
		Spiderlings.HealthScale = 0.2f;
		Setup.Groups = { Spiderlings };
		Setup.bSpawnOnApproach = false;
		Setup.NumWaves = 2;
		Setup.WaveEvent = TEXT("Test.Brood");
		Setup.SpawnRadius = 800.f;
		Setup.ActivationRadius = 6000.f;
	});
	if (!TestNotNull(TEXT("Spawner"), Brood))
	{
		return false;
	}
	Brood->UpdateEncounter(0.5f);
	TestTrue(TEXT("Brought only by a call: nothing on approach"), Brood->NumAlive() == 0 && Brood->GetState() == EEncounterState::Waiting);
	TestEqual(TEXT("Its event starts a wave"), Encounters->SendEvent(TEXT("Test.Brood")), 1);
	TestEqual(TEXT("...four spiderlings"), Brood->NumAlive(), 4);
	for (const ACreatureBase* Spiderling : Brood->GetAliveCreatures())
	{
		TestNearlyEqual(TEXT("A spiderling's size"), Spiderling->GetSizeScale(), 0.45f, 0.001f);
	}
	EncounterTestWorld::KillAll(*Brood);
	Brood->UpdateEncounter(0.5f);
	TestTrue(TEXT("The first brood dead: it waits for the next call"), Brood->GetState() == EEncounterState::Waiting);
	Brood->UpdateEncounter(30.f);
	TestEqual(TEXT("...which nothing else brings"), Brood->GetWavesStarted(), 1);
	TestEqual(TEXT("The second call"), Encounters->SendEvent(TEXT("Test.Brood")), 1);
	EncounterTestWorld::KillAll(*Brood);
	Brood->UpdateEncounter(0.5f);
	TestTrue(TEXT("Both broods dead: cleared"), Brood->GetState() == EEncounterState::Cleared);
	TestEqual(TEXT("A third call finds it done"), Encounters->SendEvent(TEXT("Test.Brood")), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterStoryTest, "Looter.Encounters.Story",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterStoryTest::RunTest(const FString& Parameters)
{
	// The story switches encounters: the town gate's fight is on only during its mission's second step, and goes when the
	// mission is done; the town becomes a safe zone after it. Both look again whenever the missions change.
	FCampaignRecord Campaign;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(World);
	if (!TestNotNull(TEXT("The level has a mission runner"), Runner) || !TestNotNull(TEXT("...and encounters"), Encounters))
	{
		return false;
	}
	const FName Valley(TEXT("TestValley"));
	const FName FightId(TEXT("TestGateFight"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Fight = MissionTestWorld::NewMission(Scratch, TEXT("TestGateFight"), EMissionKind::Main, EMissionStart::Automatic, Valley);
	MissionTestWorld::AddObjective<UMissionEventObjective>(Fight, 0)->Event = TEXT("Gate.Reached");
	MissionTestWorld::AddObjective<UMissionEventObjective>(Fight, 1)->Event = TEXT("Gate.Won");

	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(2000.0, 0.0, 0.0));
	AEncounterSpawner* Gate = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, [FightId](AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("TestGate");
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 2) };
		Setup.ActiveWhen.DuringMission = FightId;
		Setup.FromStep = 2;
		Setup.SpawnRadius = 800.f;
		Setup.ActivationRadius = 3000.f;
	});
	const ASafeGround* Town = EncounterTestWorld::SpawnSquareZone(World, FVector(-5000.0, 0.0, 0.0), 1000.0, [FightId](ASafeGround& Zone)
	{
		Zone.ZoneId = TEXT("TestTown");
		Zone.ActiveWhen.AfterMissions = { FightId };
	});
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Spawner"), Gate) || !TestNotNull(TEXT("Safe zone"), Town))
	{
		return false;
	}

	Gate->UpdateEncounter(0.5f);
	TestTrue(TEXT("No story yet: off, nothing out"), Gate->GetState() == EEncounterState::Off && Gate->NumAlive() == 0);
	TestFalse(TEXT("A trigger finds it off"), Gate->TriggerWave());
	TestFalse(TEXT("The town isn't safe yet"), Town->IsActive() || Encounters->IsInSafeZone(FVector(-5000.0, 0.0, 0.0)));

	Runner->BeginForTesting({ Fight }, Campaign, Player, Valley);
	Runner->Update(0.f);
	TestTrue(TEXT("The gate's mission runs, on its first step"), Runner->IsRunning(FightId) && Runner->GetStep(FightId) == 0);
	Gate->UpdateEncounter(0.5f);
	TestTrue(TEXT("Its first step: the fight waits for the second, off"), Gate->GetState() == EEncounterState::Off && Gate->NumAlive() == 0);

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Gate.Reached")));
	TestTrue(TEXT("The second step: on, waiting for the player"), Gate->IsStoryActive() && Gate->GetState() == EEncounterState::Waiting);
	Gate->UpdateEncounter(0.5f);
	TestEqual(TEXT("...who is near: its group is out"), Gate->NumAlive(), 2);
	TArray<TWeakObjectPtr<ACreatureBase>> Out;
	for (ACreatureBase* Creature : Gate->GetAliveCreatures())
	{
		Out.Add(Creature);
	}

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Gate.Won")));
	TestTrue(TEXT("The mission done"), Campaign.HasCompleted(FightId) && !Runner->IsRunning(FightId));
	TestTrue(TEXT("...the fight is off"), Gate->GetState() == EEncounterState::Off && !Gate->IsStoryActive());
	TestEqual(TEXT("...and what was out is gone (nobody saw it go)"), Gate->NumAlive(), 0);
	TestFalse(TEXT("...gone from the level"), Out.ContainsByPredicate([](const TWeakObjectPtr<ACreatureBase>& One) { return One.IsValid(); }));
	TestTrue(TEXT("The town is safe after the fight"), Town->IsActive() && Encounters->IsInSafeZone(FVector(-5000.0, 0.0, 0.0)));
	for (int32 Step = 0; Step < 10; ++Step)
	{
		Gate->UpdateEncounter(0.5f);
	}
	TestEqual(TEXT("Off, nothing comes back"), Gate->NumAlive(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterSafeZoneTest, "Looter.Encounters.SafeZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterSafeZoneTest::RunTest(const FString& Parameters)
{
	// Delia's salt line: a creature chasing the player into a safe zone gives up and walks home, and takes up no hunt while
	// they stay in it; out of it, they're fair game again. A zone that's off shelters nobody. And a spawner's creatures keep
	// to their hunting ground: off it, the chase is over too.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(2000.0, 0.0, 0.0));
	const ASafeGround* Farm = EncounterTestWorld::SpawnSquareZone(World, FVector(6000.0, 0.0, 0.0), 1000.0, [](ASafeGround& Zone)
	{
		Zone.ZoneId = TEXT("TestFarm");
	});
	const ASafeGround* Unopened = EncounterTestWorld::SpawnSquareZone(World, FVector(-3000.0, 0.0, 0.0), 500.0, [](ASafeGround& Zone)
	{
		Zone.ActiveWhen.AfterMissions = { FName(TEXT("TestNeverDone")) };
	});
	ACreatureBase* Hunter = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Safe zones"), Farm) || !TestNotNull(TEXT("Safe zones"), Unopened)
		|| !TestNotNull(TEXT("Hunter"), Hunter))
	{
		return false;
	}
	const double PlayerZ = Player->GetActorLocation().Z;
	const double HunterZ = Hunter->GetActorLocation().Z;
	const FVector Home = Hunter->GetHome().GetLocation();
	TestTrue(TEXT("The farm is safe from the start"), Farm->IsActive() && UEncounterSubsystem::IsSheltered(World, FVector(6000.0, 0.0, 0.0)));
	TestFalse(TEXT("...only inside its line"), UEncounterSubsystem::IsSheltered(World, FVector(4500.0, 0.0, 0.0)));
	TestFalse(TEXT("A zone whose story hasn't come is off"), Unopened->IsActive() || UEncounterSubsystem::IsSheltered(World, FVector(-3000.0, 0.0, 0.0)));

	Hunter->AlertTo(Player);
	TestTrue(TEXT("Out in the open, it hunts the player"), Hunter->GetCreatureState() == ECreatureState::Chase);
	// It chases them to the salt line (a test level doesn't move it: put it there), and they step over it.
	Hunter->SetActorLocation(FVector(4500.0, 0.0, HunterZ));
	Player->SetActorLocation(FVector(6000.0, 300.0, PlayerZ));
	Hunter->Tick(0.3f);
	TestTrue(TEXT("Over the salt line: it gives up and walks home"), Hunter->GetCreatureState() == ECreatureState::Return);
	Hunter->AlertTo(Player);
	Hunter->Tick(0.3f);
	TestTrue(TEXT("Called to them again, it hunts nobody in the zone: still walking home"), Hunter->GetCreatureState() == ECreatureState::Return);
	Hunter->SetActorLocation(Home);
	Hunter->Tick(0.3f);
	TestTrue(TEXT("Home again, it settles"), Hunter->GetCreatureState() == ECreatureState::Idle);
	Player->SetActorLocation(FVector(3000.0, 0.0, PlayerZ));
	Hunter->AlertTo(Player);
	TestTrue(TEXT("Out of the zone, the player is fair game again"), Hunter->GetCreatureState() == ECreatureState::Chase);
	Player->SetActorLocation(FVector(-3000.0, 0.0, PlayerZ));
	Hunter->Tick(0.3f);
	TestTrue(TEXT("In a zone that's off, the hunt goes on"), Hunter->GetCreatureState() == ECreatureState::Chase);

	// Its hunting ground: a spawner's creature hunts only a player within 15 m of the spawner, and gives up past it.
	AEncounterSpawner* Yard = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, 8000.0, 0.0), [](AEncounterSpawner& Setup)
	{
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1) };
		Setup.SpawnRadius = 300.f;
		Setup.GiveUpRadius = 1500.f;
		Setup.ActivationRadius = 20000.f;
	});
	if (!TestNotNull(TEXT("Spawner"), Yard))
	{
		return false;
	}
	Yard->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Guards = Yard->GetAliveCreatures();
	if (!TestEqual(TEXT("Its guard is out"), Guards.Num(), 1))
	{
		return false;
	}
	ACreatureBase* Guard = Guards[0];
	const FVector GuardHome = Guard->GetHome().GetLocation();
	Player->SetActorLocation(FVector(0.0, 7000.0, PlayerZ));
	Guard->AlertTo(Player);
	TestTrue(TEXT("10 m from its spawner, on its ground: hunted"), Guard->GetCreatureState() == ECreatureState::Chase);
	Guard->SetActorLocation(FVector(0.0, 6800.0, Guard->GetActorLocation().Z));
	Player->SetActorLocation(FVector(0.0, 6000.0, PlayerZ));
	Guard->Tick(0.3f);
	TestTrue(TEXT("20 m out, off its ground: it gives up and walks home"), Guard->GetCreatureState() == ECreatureState::Return);
	Guard->AlertTo(Player);
	TestTrue(TEXT("...and won't be called back out to them"), Guard->GetCreatureState() == ECreatureState::Return);
	TestTrue(TEXT("...its home is where it appeared, on its ground"), FVector::Dist2D(GuardHome, Yard->GetActorLocation()) <= 301.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterNoRespawnTest, "Looter.Encounters.NoRespawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterNoRespawnTest::RunTest(const FString& Parameters)
{
	// Nothing a spawner makes comes back once killed: a kill stays a kill, its waves only ever add new ones up to their
	// count, and survivors taken away while the player is far come back alone.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(2500.0, 0.0, 0.0));
	AEncounterSpawner* Spawner = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, [](AEncounterSpawner& Setup)
	{
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 3) };
		Setup.SpawnRadius = 900.f;
		Setup.ActivationRadius = 4000.f;
		Setup.DespawnRadius = 6000.f;
	});
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Spawner"), Spawner))
	{
		return false;
	}
	const double PlayerZ = Player->GetActorLocation().Z;
	Spawner->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Out = Spawner->GetAliveCreatures();
	if (!TestEqual(TEXT("Three out"), Out.Num(), 3))
	{
		return false;
	}
	ACreatureBase* Fallen = Out[0];
	BossTestWorld::Kill(Fallen);
	for (int32 Step = 0; Step < 120; ++Step)
	{
		Spawner->UpdateEncounter(0.5f);
	}
	TestTrue(TEXT("A minute after a kill: two alive, nothing new"), Spawner->NumAlive() == 2 && Spawner->GetTotalSpawned() == 3
		&& Spawner->GetKilled() == 1 && Spawner->NumOwed() == 0);
	TestTrue(TEXT("The fallen one is dead, for good"), Fallen->IsDead() && !Fallen->WillRespawn());

	// The player goes far away: the two left are taken away (owed, not killed), and come back with the player.
	TArray<TWeakObjectPtr<ACreatureBase>> Survivors;
	for (ACreatureBase* Creature : Spawner->GetAliveCreatures())
	{
		Survivors.Add(Creature);
	}
	Player->SetActorLocation(FVector(20000.0, 0.0, PlayerZ));
	Spawner->UpdateEncounter(0.5f);
	TestTrue(TEXT("Far away and unseen: taken away, owed back"), Spawner->NumAlive() == 0 && Spawner->NumOwed() == 2
		&& Spawner->GetState() == EEncounterState::Waiting);
	TestFalse(TEXT("...gone from the level"), Survivors.ContainsByPredicate([](const TWeakObjectPtr<ACreatureBase>& One) { return One.IsValid(); }));
	TestEqual(TEXT("...not counted as kills"), Spawner->GetKilled(), 1);
	Player->SetActorLocation(FVector(2500.0, 0.0, PlayerZ));
	Spawner->UpdateEncounter(0.5f);
	TestTrue(TEXT("Back: the two survivors return, not the fallen one"), Spawner->NumAlive() == 2 && Spawner->NumOwed() == 0
		&& Spawner->GetTotalSpawned() == 3);

	// Cleared stays cleared while the level lasts.
	EncounterTestWorld::KillAll(*Spawner);
	Spawner->UpdateEncounter(0.5f);
	TestTrue(TEXT("All killed: cleared"), Spawner->GetState() == EEncounterState::Cleared && Spawner->GetKilled() == 3);
	Player->SetActorLocation(FVector(20000.0, 0.0, PlayerZ));
	Spawner->UpdateEncounter(0.5f);
	Player->SetActorLocation(FVector(2500.0, 0.0, PlayerZ));
	Spawner->UpdateEncounter(0.5f);
	TestTrue(TEXT("Away and back: still cleared, nothing out"), Spawner->GetState() == EEncounterState::Cleared && Spawner->NumAlive() == 0);
	TestFalse(TEXT("...and it takes no trigger"), Spawner->TriggerWave());

	// Whatever its rank, a creature spawned in play never comes back on its own (a placed Basic or Restless one would).
	for (const ECreatureRank Rank : LooterRanks::All)
	{
		ACreatureBase::FRuntimeSpawn Setup;
		Setup.Rank = Rank;
		const ACreatureBase* Spawned = ACreatureBase::SpawnAtRuntime(World, ASpiderCreature::StaticClass(), FVector(0.0, -5000.0, 0.0), 0.f, Setup);
		TestTrue(TEXT("Spawned in play: never comes back"), Spawned && !Spawned->WillRespawn());
	}
	return true;
}

#endif
