#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The dead miners (ADeadMinerCreature, DeadMinerRules) and the Providence's shift spawner (ADeadMinerShiftSpawner): their
// rules alone, then miners in test levels, their frames moved on a step at a time (a test level never ticks anything).

#include "Audio/CreatureVoiceComponent.h"
#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/DeadMinerRules.h"
#include "Creatures/DeadMinerShiftSpawner.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/UnpaidCreature.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

namespace
{
	constexpr float Frame = 1.f / 30.f;

	ADeadMinerCreature* SpawnMiner(UWorld* World, const FVector& Feet)
	{
		return Cast<ADeadMinerCreature>(EncounterTestWorld::SpawnCreature(World, ADeadMinerCreature::StaticClass(), Feet));
	}

	/** A shot's damage as the engine hands it over: its point first (where it hit), then the damage itself. */
	void ShootAt(AActor* Victim, float Damage, const FVector& Where, UPrimitiveComponent* Part, bool bCritical)
	{
		FEditorScriptExecutionGuard RunActorEvents;
		const UDamageType* Type = bCritical ? GetDefault<UWeaponCritDamageType>() : GetDefault<UWeaponDamageType>();
		Victim->OnTakePointDamage.Broadcast(Victim, Damage, nullptr, Where, Part, NAME_None, FVector::ForwardVector, Type, nullptr);
		Victim->OnTakeAnyDamage.Broadcast(Victim, Damage, Type, nullptr, nullptr);
	}

	void Run(ADeadMinerCreature& Miner, float Seconds)
	{
		for (float Time = 0.f; Time < Seconds - KINDA_SMALL_NUMBER; Time += Frame)
		{
			Miner.ConsumeMovementInputVector();
			Miner.Tick(Frame);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadMinerRulesTest, "Looter.Creatures.DeadMiner.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadMinerRulesTest::RunTest(const FString& Parameters)
{
	// The shift works on while Back Pay isn't finished, or once it's finished keeping the payroll; paid, the eleven rest.
	TestTrue(TEXT("Back Pay not done: the shift works"), DeadMinerRules::ShiftStillWorks(false, false));
	TestFalse(TEXT("Paid: the shift is over"), DeadMinerRules::ShiftStillWorks(true, false));
	TestTrue(TEXT("Kept the payroll: they keep working"), DeadMinerRules::ShiftStillWorks(true, true));
	FCampaignRecord Campaign;
	TestTrue(TEXT("A new campaign: the shift works"), DeadMinerRules::ShiftStillWorks(Campaign));
	Campaign.Complete(DeadMinerRules::BackPayMission);
	TestFalse(TEXT("Back Pay turned in, paid: over"), DeadMinerRules::ShiftStillWorks(Campaign));
	Campaign.SetFlag(DeadMinerRules::KeptFlag);
	TestTrue(TEXT("...kept (the story flag): working again"), DeadMinerRules::ShiftStillWorks(Campaign));

	// The swing lands on whoever is in reach in front of it, never behind it or past its reach.
	const FVector Forward = FVector::ForwardVector;
	TestTrue(TEXT("2 m in front: hit"), DeadMinerRules::IsSwingHit(Forward, FVector(200.0, 0.0, 0.0), 250.f, 0.35f, 220.f));
	TestTrue(TEXT("...a little to the side: hit"), DeadMinerRules::IsSwingHit(Forward, FVector(180.0, 90.0, 0.0), 250.f, 0.35f, 220.f));
	TestFalse(TEXT("3 m in front: out of reach"), DeadMinerRules::IsSwingHit(Forward, FVector(300.0, 0.0, 0.0), 250.f, 0.35f, 220.f));
	TestFalse(TEXT("Behind it: missed"), DeadMinerRules::IsSwingHit(Forward, FVector(-150.0, 0.0, 0.0), 250.f, 0.35f, 220.f));
	TestFalse(TEXT("On a ledge 3 m up: missed"), DeadMinerRules::IsSwingHit(Forward, FVector(100.0, 0.0, 300.0), 250.f, 0.35f, 220.f));

	// The lamp swells through the wind-up, brightest at the blow.
	TestNearlyEqual(TEXT("Flare at the wind-up's start"), DeadMinerRules::WindupFlare(0.f, 0.8f), 0.f, 0.001f);
	TestTrue(TEXT("...growing"), DeadMinerRules::WindupFlare(0.4f, 0.8f) > 0.3f && DeadMinerRules::WindupFlare(0.4f, 0.8f) < 0.7f);
	TestNearlyEqual(TEXT("...full at the blow"), DeadMinerRules::WindupFlare(0.8f, 0.8f), 1.f, 0.001f);

	// The light budget: only the nearest few light up round them.
	TestTrue(TEXT("Nearest, in range: lit"), DeadMinerRules::ShouldLightLamp(800.f, 0, 2, 2600.f));
	TestTrue(TEXT("Second nearest: lit"), DeadMinerRules::ShouldLightLamp(900.f, 1, 2, 2600.f));
	TestFalse(TEXT("Third nearest: glow only"), DeadMinerRules::ShouldLightLamp(1000.f, 2, 2, 2600.f));
	TestFalse(TEXT("Out of range: glow only"), DeadMinerRules::ShouldLightLamp(3000.f, 0, 2, 2600.f));

	// At work: one blow a swing, at its blow share.
	const float Swing = 2.4f;
	const float Blow = DeadMinerRules::WorkBlowShare * Swing;
	TestTrue(TEXT("A frame over the blow lands it"), DeadMinerRules::WorkBlowBetween(Blow - 0.01f, Blow + 0.01f, Swing));
	TestFalse(TEXT("...a frame before it doesn't"), DeadMinerRules::WorkBlowBetween(Blow - 0.1f, Blow - 0.05f, Swing));
	TestTrue(TEXT("...nor the next swing's start, but its blow does"), !DeadMinerRules::WorkBlowBetween(Swing, Swing + 0.1f, Swing)
		&& DeadMinerRules::WorkBlowBetween(Swing + Blow - 0.02f, Swing + Blow + 0.02f, Swing));
	int32 Blows = 0;
	for (float Time = 0.f; Time < 3.f * Swing; Time += Frame)
	{
		Blows += DeadMinerRules::WorkBlowBetween(Time, Time + Frame, Swing) ? 1 : 0;
	}
	TestEqual(TEXT("Three swings, three blows"), Blows, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadMinerStatsTest, "Looter.Creatures.DeadMiner.Stats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadMinerStatsTest::RunTest(const FString& Parameters)
{
	// A shade tougher and heavier than the Unpaid (180 and 9 against 160 and 8), one of the Unpaid's pack and cap, its own words.
	const ADeadMinerCreature* Defaults = GetDefault<ADeadMinerCreature>();
	const AUnpaidCreature* Unpaid = GetDefault<AUnpaidCreature>();
	const UHealthComponent* Health = Defaults->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("It has health"), Health))
	{
		TestNearlyEqual(TEXT("180 health at level 1"), Health->MaxHealth, 180.f, 0.01f);
	}
	TestNearlyEqual(TEXT("9 damage at level 1"), Defaults->AttackDamage, 9.f, 0.01f);
	TestTrue(TEXT("Tougher and heavier than an Unpaid"), Defaults->AttackDamage > Unpaid->AttackDamage);
	TestNearlyEqual(TEXT("A 0.8 s wind-up: the tell"), Defaults->AttackWindup, 0.8f, 0.01f);
	TestTrue(TEXT("It works where it was put: no wandering"), Defaults->WanderRadius <= 0.f);
	TestEqual(TEXT("The Unpaid's pack"), Defaults->PackTag, Unpaid->PackTag);
	TestEqual(TEXT("It counts among the Unpaid's 12"), UEncounterSettings::Get().FindClassCap(ADeadMinerCreature::StaticClass()),
		UEncounterSettings::Get().FindClassCap(AUnpaidCreature::StaticClass()));
	const UCreatureVoiceComponent* Voice = Defaults->FindComponentByClass<UCreatureVoiceComponent>();
	if (TestNotNull(TEXT("It has a voice"), Voice))
	{
		TestEqual(TEXT("Its voice says its own lines"), Voice->BarkKind, ADeadMinerCreature::BarkKind);
	}

	// Its lamp: a weak spot of its own, always critical, put out by the first shot on it.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ADeadMinerCreature* Miner = SpawnMiner(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Miner spawned"), Miner))
	{
		return false;
	}
	USphereComponent* Zone = Miner->GetLampHitZone();
	TestTrue(TEXT("Bullets meet the lamp"), Zone->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block
		&& Zone->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	FHitResult OnLamp;
	OnLamp.Component = Zone;
	OnLamp.ImpactPoint = Miner->GetLampCenter();
	TestTrue(TEXT("A shot on the lamp is critical"), Miner->IsCriticalSpot(OnLamp));
	TestTrue(TEXT("Its lamp burns"), Miner->IsLampLit());
	ShootAt(Miner, 5.f, Miner->GetLampCenter(), Zone, true);
	TestFalse(TEXT("Shot, the lamp is out for this life"), Miner->IsLampLit());
	TestFalse(TEXT("...its light with it"), Miner->IsLampLightOn());
	TestTrue(TEXT("...and still a weak spot"), Miner->IsCriticalSpot(OnLamp));
	BossTestWorld::Kill(Miner);
	TestFalse(TEXT("Dead, its lamp takes no shots"), Zone->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadMinerLampBudgetTest, "Looter.Creatures.DeadMiner.LampBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadMinerLampBudgetTest::RunTest(const FString& Parameters)
{
	// Three miners near the player: the two nearest light up round them, the third shows its glow alone; the nearest
	// shot out, the third takes its place.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(0.0, 0.0, 90.0));
	ADeadMinerCreature* Near = SpawnMiner(World, FVector(500.0, 0.0, 0.0));
	ADeadMinerCreature* Middle = SpawnMiner(World, FVector(800.0, 0.0, 0.0));
	ADeadMinerCreature* Far = SpawnMiner(World, FVector(1100.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Three miners"), Far) || !Near || !Middle)
	{
		return false;
	}
	for (ADeadMinerCreature* Each : { Near, Middle, Far })
	{
		Each->RefreshLampLight();
	}
	TestTrue(TEXT("The nearest two light up"), Near->IsLampLightOn() && Middle->IsLampLightOn());
	TestFalse(TEXT("...the third shows its glow alone"), Far->IsLampLightOn());
	Near->PutOutLamp(Player->GetActorLocation());
	Far->RefreshLampLight();
	TestTrue(TEXT("The nearest shot out: the third lights up in its place"), Far->IsLampLightOn() && !Near->IsLampLightOn());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadMinerSwingTest, "Looter.Creatures.DeadMiner.Swing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadMinerSwingTest::RunTest(const FString& Parameters)
{
	// In reach, it plants itself and heaves the pick up, its lamp flaring; shot in the lamp then, the swing is lost and it
	// reels, then it's back after the player.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(200.0, 0.0, 90.0));
	ADeadMinerCreature* Miner = SpawnMiner(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Miner spawned"), Miner))
	{
		return false;
	}
	Miner->AlertTo(Player);
	Run(*Miner, 0.1f);
	TestTrue(TEXT("2 m off: it swings"), Miner->GetCreatureState() == ECreatureState::Attack);
	Run(*Miner, 0.4f);
	TestTrue(TEXT("...the pick going up: its wind-up"), Miner->IsWindingUp());
	TestTrue(FString::Printf(TEXT("...its lamp flaring (%.2f)"), Miner->GetLampFlare()), Miner->GetLampFlare() > 0.2f);
	ShootAt(Miner, 5.f, Miner->GetLampCenter(), Miner->GetLampHitZone(), true);
	TestTrue(TEXT("Its lamp shot in the wind-up: the swing is lost"), Miner->WasSwingBroken() && !Miner->IsWindingUp());
	TestTrue(TEXT("...it reels"), Miner->IsReeling());
	TestFalse(TEXT("...its lamp out"), Miner->IsLampLit());
	Run(*Miner, Miner->Tuning.ReelSeconds + 0.2f);
	TestFalse(TEXT("Then it's done reeling"), Miner->IsReeling());
	TestTrue(TEXT("...and after the player again"), Miner->GetCreatureState() == ECreatureState::Chase
		|| Miner->GetCreatureState() == ECreatureState::Attack);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadMinerWorkTest, "Looter.Creatures.DeadMiner.Work",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadMinerWorkTest::RunTest(const FString& Parameters)
{
	// Unaware with a wall beside it, it swings at the wall, blow after blow, and never walks off; with no wall, its pail.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	// A rock face 1.5 m to its right, from the floor to 3 m.
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 200.0, 150.0), FVector(400.0, 100.0, 300.0));
	ADeadMinerCreature* Worker = SpawnMiner(World, FVector::ZeroVector);
	ADeadMinerCreature* Eater = SpawnMiner(World, FVector(2000.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Miners spawned"), Worker) || !Eater)
	{
		return false;
	}
	FVector Point;
	FVector Normal;
	TestTrue(TEXT("It finds the wall beside it"), Worker->FindWorkWall(Point, Normal) && FMath::IsNearlyEqual(Point.Y, 150.0, 5.0));
	const FVector Start = Worker->GetActorLocation();
	Run(*Worker, 0.7f);
	Run(*Eater, 0.7f);
	TestTrue(TEXT("Unaware, it works the wall"), Worker->IsWorking());
	TestTrue(TEXT("...the other, with no wall near, its lunch pail"), Eater->GetAct() == ADeadMinerCreature::EAct::Pail);
	Run(*Worker, 5.f);
	TestTrue(FString::Printf(TEXT("...a blow every couple of seconds (%d)"), Worker->GetWorkBlows()), Worker->GetWorkBlows() >= 2);
	TestTrue(TEXT("...turned to it"), FVector::DotProduct(Worker->GetActorForwardVector(), FVector(0.0, 1.0, 0.0)) > 0.8);
	TestTrue(TEXT("...where it was put"), FVector::Dist2D(Worker->GetActorLocation(), Start) < 30.0);

	// Alerted, it stops work at once.
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-600.0, 0.0, 90.0));
	Worker->AlertTo(Player);
	Run(*Worker, 0.1f);
	TestFalse(TEXT("Alerted, it drops its work"), Worker->IsWorking());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadMinerShiftSpawnerTest, "Looter.Creatures.DeadMiner.ShiftSpawner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadMinerShiftSpawnerTest::RunTest(const FString& Parameters)
{
	// The Providence's encounters: on while the shift works, off once the eleven are paid, on again when the payroll's kept.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ADeadMinerShiftSpawner* Spawner = World->SpawnActor<ADeadMinerShiftSpawner>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Shift spawner"), Spawner))
	{
		return false;
	}
	Spawner->SpawnerId = TEXT("MineUpper");
	Spawner->Groups = { EncounterTestWorld::MakeGroup(ADeadMinerCreature::StaticClass(), 4) };
	Spawner->DispatchBeginPlay();
	TestTrue(TEXT("No campaign (Back Pay never done): the shift works, the encounter is on"), Spawner->DoesShiftWork() && Spawner->IsStoryActive());
	// The encounter reads the level's story (the mission runner's campaign): Back Pay turned in, paid.
	UMissionRunner* Runner = UMissionRunner::Get(World);
	if (!TestNotNull(TEXT("The level's mission runner"), Runner))
	{
		return false;
	}
	Runner->GetCampaign().Complete(DeadMinerRules::BackPayMission);
	Spawner->ApplyShift(false);
	TestFalse(TEXT("Paid: the encounter goes off"), Spawner->IsStoryActive());
	TestTrue(TEXT("...off by the story"), Spawner->GetState() == EEncounterState::Off);
	Runner->GetCampaign().SetFlag(DeadMinerRules::KeptFlag);
	Spawner->ApplyShift(true);
	TestTrue(TEXT("Kept: on again"), Spawner->IsStoryActive() && Spawner->GetState() == EEncounterState::Waiting);
	TestTrue(TEXT("...its own condition as placed (nothing of the shift's left on it)"), Spawner->ActiveWhen.BeforeMissions.IsEmpty());
	return true;
}

#endif
