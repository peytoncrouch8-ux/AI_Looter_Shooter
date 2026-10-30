#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTouched.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/AutomationCommon.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "UObject/Package.h"

// Touched creatures (the user's call, 2026-10-29 night): each kind's element, its counter, the tag's word, the roll's rules,
// and in a test level with an area that touches every creature it may: the cap on how many are alive at once, the tiers that
// may be touched, and the fights that never are (bMayBeTouched off). Looter.Creatures.Touched.

namespace
{
	using EE = ELooterElement;

	/** A test level with area rules (made only in played worlds unless a test lets it). */
	struct FTouchedLevel
	{
		FTestWorldWrapper Wrapper;
		bool Create()
		{
			UAreaRulesSubsystem::bAllowInTestWorlds = true;
			const bool bMade = Wrapper.CreateTestWorld(EWorldType::EditorPreview);
			UAreaRulesSubsystem::bAllowInTestWorlds = false;
			return bMade;
		}
	};

	/** A spider spawned in play as Spawn says, started as play would, dropping no loot. */
	ACreatureBase* SpawnWith(UWorld* World, const FVector& Feet, const ACreatureBase::FRuntimeSpawn& Spawn)
	{
		ACreatureBase* Creature = ACreatureBase::SpawnAtRuntime(World, ASpiderCreature::StaticClass(), Feet, 0.f, Spawn);
		if (Creature && !Creature->HasActorBegunPlay())
		{
			Creature->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Creature);
		return Creature;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureTouchedTest, "Looter.Creatures.Touched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureTouchedTest::RunTest(const FString& Parameters)
{
	// Each kind's touch (the most specific first: the Bell-Ringer and the dead miners are built on the Unpaid).
	TestTrue(TEXT("A spider: Cinder"), CreatureTouched::ElementFor(ASpiderCreature::StaticClass()) == EE::Ember);
	TestTrue(TEXT("A slime: Thunder"), CreatureTouched::ElementFor(ASlimeCreature::StaticClass()) == EE::Thunder);
	TestTrue(TEXT("A dead miner: Rot"), CreatureTouched::ElementFor(ADeadMinerCreature::StaticClass()) == EE::Rot);
	TestTrue(TEXT("The Unpaid: Wardbreak"), CreatureTouched::ElementFor(AUnpaidCreature::StaticClass()) == EE::GraveSalt);
	TestTrue(TEXT("The Bell-Ringer: Hallow"), CreatureTouched::ElementFor(ABellRingerCreature::StaticClass()) == EE::SoulLight);
	TestTrue(TEXT("Nothing: none"), CreatureTouched::ElementFor(nullptr) == EE::None);
	// Their counters.
	TestTrue(TEXT("Cinder <- Wardbreak"), CreatureTouched::CounterOf(EE::Ember) == EE::GraveSalt);
	TestTrue(TEXT("Thunder <- Wardbreak"), CreatureTouched::CounterOf(EE::Thunder) == EE::GraveSalt);
	TestTrue(TEXT("Rot <- Cinder"), CreatureTouched::CounterOf(EE::Rot) == EE::Ember);
	TestTrue(TEXT("Wardbreak <- Thunder"), CreatureTouched::CounterOf(EE::GraveSalt) == EE::Thunder);
	TestTrue(TEXT("Hallow <- Rot"), CreatureTouched::CounterOf(EE::SoulLight) == EE::Rot);
	// The tag's word.
	TestEqual(TEXT("Cinder-touched"), CreatureTouched::Word(EE::Ember), FString(TEXT("Cinder-touched")));
	TestEqual(TEXT("Wardbreak-touched"), CreatureTouched::Word(EE::GraveSalt), FString(TEXT("Wardbreak-touched")));
	TestTrue(TEXT("...none for None"), CreatureTouched::Word(EE::None).IsEmpty());
	// The roll.
	TestTrue(TEXT("Under the chance, room left: touched"), CreatureTouched::Rolls(0.05f, 0.06f, 2, 3));
	TestFalse(TEXT("Over the chance: not"), CreatureTouched::Rolls(0.07f, 0.06f, 0, 3));
	TestFalse(TEXT("At the cap: not"), CreatureTouched::Rolls(0.f, 0.06f, 3, 3));
	TestFalse(TEXT("No chance: never"), CreatureTouched::Rolls(0.f, 0.f, 0, 3));

	// In play, with an area that touches every creature it may.
	FTouchedLevel Level;
	if (!TestTrue(TEXT("Test world created"), Level.Create()))
	{
		return false;
	}
	UWorld* World = Level.Wrapper.GetTestWorld();
	UAreaRulesSubsystem* Rules = World->GetSubsystem<UAreaRulesSubsystem>();
	if (!TestNotNull(TEXT("The test level has area rules"), Rules))
	{
		return false;
	}
	UAreaDefinition* Area = NewObject<UAreaDefinition>(GetTransientPackage());
	Area->TouchedChance = 1.f;
	Area->MaxTouchedAlive = 3;
	Rules->SetAreaForTest(Area);

	int32 Touched = 0;
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const ACreatureBase* Spider = SpawnWith(World, FVector(Index * 300.0, 0.0, 0.0), ACreatureBase::FRuntimeSpawn());
		Touched += Spider && Spider->GetTouchedElement() == EE::Ember ? 1 : 0;
	}
	TestEqual(TEXT("Every one touched by Cinder, up to the cap of three alive"), Touched, 3);

	Area->MaxTouchedAlive = 100;
	ACreatureBase::FRuntimeSpawn SetPiece;
	SetPiece.bMayBeTouched = false;
	const ACreatureBase* Kept = SpawnWith(World, FVector(0.0, 600.0, 0.0), SetPiece);
	TestTrue(TEXT("A set piece's (bMayBeTouched off): never"), Kept && Kept->GetTouchedElement() == EE::None);
	ACreatureBase::FRuntimeSpawn Gravebound;
	Gravebound.Rank = ECreatureRank::Epic;
	const ACreatureBase* Epic = SpawnWith(World, FVector(300.0, 600.0, 0.0), Gravebound);
	TestTrue(TEXT("A Gravebound one may be"), Epic && Epic->GetTouchedElement() == EE::Ember);
	ACreatureBase::FRuntimeSpawn Hallowed;
	Hallowed.Rank = ECreatureRank::Hallowed;
	const ACreatureBase* High = SpawnWith(World, FVector(600.0, 600.0, 0.0), Hallowed);
	TestTrue(TEXT("A Hallowed one never (Basic, Restless, Gravebound only)"), High && High->GetTouchedElement() == EE::None);
	TestTrue(TEXT("A Gravemother's spiderling never (the spider fights)"), !AGravemotherCreature::MakeSpiderlingSpawn().bMayBeTouched);

	Area->bPractice = true;
	const ACreatureBase* Practice = SpawnWith(World, FVector(900.0, 600.0, 0.0), ACreatureBase::FRuntimeSpawn());
	TestTrue(TEXT("A practice area never touches"), Practice && Practice->GetTouchedElement() == EE::None);

	// Chosen by hand (the dev command): kept over the roll.
	ACreatureBase* Chosen = const_cast<ACreatureBase*>(Practice);
	if (Chosen)
	{
		Chosen->SetTouched(EE::Thunder);
		TestTrue(TEXT("Touched by hand, whatever the area"), Chosen->GetTouchedElement() == EE::Thunder);
		Chosen->SetRank(ECreatureRank::Rare);
		TestTrue(TEXT("...kept through a promotion"), Chosen->GetTouchedElement() == EE::Thunder);
	}
	return true;
}

#endif
