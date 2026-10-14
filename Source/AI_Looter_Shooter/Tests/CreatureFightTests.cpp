#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "World/GraveTravelRules.h"
#include "World/GraveTravelSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// "In a fight" has one definition (ACreatureBase::IsFightingWith, CreatureBasePursuit.cpp), which fast travel
// (UGraveTravelSubsystem::Sense) and the combat music (UMusicDirectorSubsystem::Sense) both ask: a creature chasing or
// attacking the player is one, a creature that lost them and only searches (tracks where it lost them, sniffs there, or
// watches from its edge) is not. The state rule first, then creatures in a test level against the fast-travel rules (the
// music's own look needs a game world; it asks the same function).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureFightStatesTest, "Looter.Creatures.Fight.States",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureFightStatesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Chasing is a fight"), ACreatureBase::IsFightState(ECreatureState::Chase));
	TestTrue(TEXT("Attacking is a fight"), ACreatureBase::IsFightState(ECreatureState::Attack));
	TestFalse(TEXT("Searching (tracking, sniffing, prowling) is not"), ACreatureBase::IsFightState(ECreatureState::Search));
	TestFalse(TEXT("Standing about is not"), ACreatureBase::IsFightState(ECreatureState::Idle));
	TestFalse(TEXT("Strolling is not"), ACreatureBase::IsFightState(ECreatureState::Wander));
	TestFalse(TEXT("Walking home is not"), ACreatureBase::IsFightState(ECreatureState::Return));
	TestFalse(TEXT("Dead is not"), ACreatureBase::IsFightState(ECreatureState::Dead));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureFightSensesTest, "Looter.Creatures.Fight.Senses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureFightSensesTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UGraveTravelSubsystem* Travel = World->GetSubsystem<UGraveTravelSubsystem>();
	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator);
	ACharacter* Other = World->SpawnActor<ACharacter>(FVector(0.0, 4000.0, 100.0), FRotator::ZeroRotator);
	ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(FVector(1500.0, 0.0, 0.0), FRotator::ZeroRotator);
	ASpiderCreature* Second = World->SpawnActor<ASpiderCreature>(FVector(-1500.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!TestTrue(TEXT("The travel, two players and two spiders"), Travel && Player && Other && Spider && Second))
	{
		return false;
	}

	TestFalse(TEXT("Standing about: no fight"), Spider->IsFightingWith(Player));
	TestEqual(TEXT("...so nothing hunts the player"), Travel->Sense(Player).Hunters, 0);

	Spider->DevPutInState(ECreatureState::Chase, Player);
	TestTrue(TEXT("Chasing the player: a fight"), Spider->IsFightingWith(Player));
	TestEqual(TEXT("...fast travel counts it"), Travel->Sense(Player).Hunters, 1);
	TestTrue(TEXT("...and says why not"), Travel->CheckNow(Player) == EGraveTravelBlock::Hunted);

	Spider->DevPutInState(ECreatureState::Attack, Player);
	TestTrue(TEXT("Attacking the player: a fight"), Spider->IsFightingWith(Player));
	TestEqual(TEXT("...fast travel counts it"), Travel->Sense(Player).Hunters, 1);

	// It lost them and searches: still after them (a target), but nothing is coming at the player.
	Spider->DevPutInState(ECreatureState::Search, Player);
	TestTrue(TEXT("Searching still has the player as its target"), Spider->GetTarget() == Player);
	TestFalse(TEXT("...but it is no fight"), Spider->IsFightingWith(Player));
	TestEqual(TEXT("...fast travel doesn't count it"), Travel->Sense(Player).Hunters, 0);
	TestTrue(TEXT("...so the player may travel"), Travel->CheckNow(Player) == EGraveTravelBlock::None);

	// One chasing and one searching: one fight.
	Second->DevPutInState(ECreatureState::Chase, Player);
	TestEqual(TEXT("A chaser and a searcher: one fight"), Travel->Sense(Player).Hunters, 1);
	Spider->DevPutInState(ECreatureState::Chase, Player);
	TestEqual(TEXT("Both chasing: two"), Travel->Sense(Player).Hunters, 2);

	// After someone else, or nobody.
	Spider->DevPutInState(ECreatureState::Chase, Other);
	Second->DevPutInState(ECreatureState::Idle);
	TestFalse(TEXT("Chasing another player is no fight with this one"), Spider->IsFightingWith(Player));
	TestTrue(TEXT("...but it is with that one"), Spider->IsFightingWith(Other));
	TestFalse(TEXT("Nobody is never a fight"), Spider->IsFightingWith(nullptr));
	TestEqual(TEXT("...fast travel counts none for the player"), Travel->Sense(Player).Hunters, 0);

	Spider->DevPutInState(ECreatureState::Return);
	TestFalse(TEXT("Walking home: no fight"), Spider->IsFightingWith(Other));
	return true;
}

#endif
