#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SpiderCreature.h"
#include "Tests/AutomationCommon.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "UObject/Package.h"

// A creature's level is where it stands in play, never the world's origin (the play-check of 2026-10-23: the Providence's
// dead miners, in a 5-6 zone, were level 2). AActor::BeginPlay counts an actor begun before ACreatureBase's own start asks
// its area for a level, and the area read the home it hadn't set yet, (0, 0), in the valley's 2-3, for every creature
// since the area levels came (2026-10-13). Looter.Creatures.AreaLevels checks the zones; this, a creature asking in play.

namespace
{
	/** A square zone HalfSize (cm) round (X, Y), its creatures Low to High before their ranks. */
	FAreaLevelZone Square(FName Id, double X, double Y, double HalfSize, int32 Low, int32 High)
	{
		FAreaLevelZone Zone;
		Zone.Id = Id;
		Zone.Outline = { FVector2D(X - HalfSize, Y - HalfSize), FVector2D(X + HalfSize, Y - HalfSize), FVector2D(X + HalfSize, Y + HalfSize),
			FVector2D(X - HalfSize, Y + HalfSize) };
		Zone.MinLevel = Low;
		Zone.MaxLevel = High;
		return Zone;
	}

	/** A test level with area rules (made only in played worlds unless a test lets it). */
	struct FAreaLevel
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureLevelInPlayTest, "Looter.Creatures.LevelFromHome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureLevelInPlayTest::RunTest(const FString& Parameters)
{
	FAreaLevel Level;
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
	// Ransom's Rest's shape in small: a mountain zone at 5-6 far from the origin (the Providence's gallery is at (27200, 7400)),
	// and the valley's 2-3 everywhere else, the origin too.
	UAreaDefinition* Area = NewObject<UAreaDefinition>(GetTransientPackage());
	Area->MinLevel = 1;
	Area->MaxLevel = 7;
	Area->LevelZones = { Square(TEXT("providence"), 27200.0, 7400.0, 2000.0, 5, 6), Square(TEXT("valley"), 0.0, 0.0, 40000.0, 2, 3) };
	Rules->SetAreaForTest(Area);

	// Spawned in play in the gallery, as a spawner's are: the gallery's 5-6, every one.
	bool bGallery = true;
	FString Seen;
	for (int32 Index = 0; Index < 8; ++Index)
	{
		const ACreatureBase* Miner = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(),
			FVector(26600.0 + Index * 150.0, 7400.0, 0.0));
		bGallery &= Miner && Miner->Level >= 5 && Miner->Level <= 6;
		Seen += FString::Printf(TEXT("%d "), Miner ? Miner->Level : 0);
	}
	TestTrue(FString::Printf(TEXT("Spawned in play in a 5-6 zone far from the origin: 5-6 (%s)"), *Seen), bGallery);

	// Placed there (begun as a level's are as play begins): the same.
	ASpiderCreature* Placed = World->SpawnActor<ASpiderCreature>(FVector(27800.0, 7000.0, 120.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A placed spider"), Placed))
	{
		BossTestWorld::NoLoot(Placed);
		Placed->DispatchBeginPlay();
		TestTrue(FString::Printf(TEXT("Placed in that zone: 5-6 (%d)"), Placed->Level), Placed->Level >= 5 && Placed->Level <= 6);
		TestTrue(TEXT("...its home where it stands, not the origin"), FVector::Dist2D(Placed->GetHome().GetLocation(), FVector(27800.0, 7000.0, 0.0)) < 1.0);
	}

	// By the origin: the valley's 2-3.
	const ACreatureBase* Valley = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(300.0, -200.0, 0.0));
	TestTrue(FString::Printf(TEXT("Spawned by the origin: the valley's 2-3 (%d)"), Valley ? Valley->Level : 0), Valley && Valley->Level >= 2
		&& Valley->Level <= 3);

	// One spawned at a level of its own keeps it, wherever it stands.
	ACreatureBase::FRuntimeSpawn Own;
	Own.Level = 4;
	ACreatureBase* Kept = ACreatureBase::SpawnAtRuntime(World, ASpiderCreature::StaticClass(), FVector(27000.0, 7600.0, 0.0), 0.f, Own);
	if (Kept && !Kept->HasActorBegunPlay())
	{
		Kept->DispatchBeginPlay();
	}
	BossTestWorld::NoLoot(Kept);
	TestTrue(FString::Printf(TEXT("Spawned at level 4 in the 5-6 zone: 4 (%d)"), Kept ? Kept->Level : 0), Kept && Kept->Level == 4);
	return true;
}

#endif
