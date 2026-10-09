#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/PackRules.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"

// How packs behave (Docs/Polish/BorderlandsComparison.md, item 8; UCreaturePackComponent, PackRules): they come in on
// spread angles rather than in a line, a wounded Basic one whose pack is gone may break off for a moment, and a Restless
// or better one's first sight of the player sounds the rank sting once. Rules first, then in a test level.

namespace
{
	/** A creature in a test level at Feet, turned on the player (as a sighting or a hit would). */
	ACreatureBase* Hunter(FAutomationTestBase& Test, UWorld* World, UClass* Class, const FVector& Feet, APawn* Player,
		ECreatureRank Rank = ECreatureRank::Basic)
	{
		ACreatureBase* Made = EncounterTestWorld::SpawnCreature(World, Class, Feet);
		if (!Test.TestNotNull(TEXT("A creature"), Made))
		{
			return nullptr;
		}
		if (Rank != ECreatureRank::Basic)
		{
			Made->SetRank(Rank);
		}
		Made->AlertTo(Player);
		return Made;
	}

	/** The first seed (from 1) whose first roll is under Chance, or at least it with bUnder off: a roll's outcome chosen by seed. */
	int32 SeedRolling(float Chance, bool bUnder)
	{
		for (int32 Seed = 1; Seed < 10000; ++Seed)
		{
			FRandomStream Stream(Seed);
			if ((Stream.FRand() < Chance) == bUnder)
			{
				return Seed;
			}
		}
		return 1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterPackFlankTest, "Looter.Encounters.Packs.Flank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterPackFlankTest::RunTest(const FString& Parameters)
{
	// The spread: 20 degrees a member either side of the middle, at most 35; a pack of one comes straight.
	TestEqual(TEXT("One alone: no flank"), PackRules::FlankHalfSpread(1, 20.f, 35.f), 0.f);
	TestEqual(TEXT("Two: 20 either side"), PackRules::FlankHalfSpread(2, 20.f, 35.f), 20.f);
	TestEqual(TEXT("Three or more: at most 35"), PackRules::FlankHalfSpread(5, 20.f, 35.f), 35.f);

	// Ranked by bearing round the prey: the low end swings low, the high end high, the middle comes straight.
	const TArray<float> Three = PackRules::FlankOffsets({ 0.f, 30.f, -30.f }, { 1, 2, 3 }, 20.f, 35.f);
	TestTrue(TEXT("Three abreast: the middle straight, the ends out to either side"), FMath::IsNearlyZero(Three[0]) && FMath::IsNearlyEqual(Three[1], 35.f)
		&& FMath::IsNearlyEqual(Three[2], -35.f));
	const TArray<float> Astride = PackRules::FlankOffsets({ 170.f, -170.f }, { 1, 2 }, 20.f, 35.f);
	TestTrue(TEXT("A pair astride the bearing's wrap ranks as one line, not two ends"), FMath::IsNearlyEqual(Astride[0], -20.f)
		&& FMath::IsNearlyEqual(Astride[1], 20.f));
	const TArray<float> Stacked = PackRules::FlankOffsets({ 5.f, 5.f }, { 7, 3 }, 20.f, 35.f);
	const TArray<float> StackedOther = PackRules::FlankOffsets({ 5.f, 5.f }, { 3, 7 }, 20.f, 35.f);
	TestTrue(TEXT("Two on one bearing: their ids part them, the same way whoever asks"), FMath::IsNearlyEqual(Stacked[0], 20.f)
		&& FMath::IsNearlyEqual(Stacked[1], -20.f) && FMath::IsNearlyEqual(StackedOther[0], -20.f) && FMath::IsNearlyEqual(StackedOther[1], 20.f));

	// The way it runs: straight in without a flank or close by; turned off the line by its flank, swinging round the prey
	// the way its bearing grows.
	const FVector Prey = FVector::ZeroVector;
	const FVector Me(1500.0, 0.0, 0.0);
	TestTrue(TEXT("No flank: straight at the prey"), PackRules::FlankDirection(Me, Prey, 0.f, 450.f).Equals(FVector(-1.0, 0.0, 0.0), 0.001));
	TestTrue(TEXT("Close by: straight in"), PackRules::FlankDirection(FVector(400.0, 0.0, 0.0), Prey, 35.f, 450.f).Equals(FVector(-1.0, 0.0, 0.0), 0.001));
	const FVector Way = PackRules::FlankDirection(Me, Prey, 30.f, 450.f);
	const double OffLine = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Way, FVector(-1.0, 0.0, 0.0))));
	TestTrue(FString::Printf(TEXT("A 30 degree flank runs 30 degrees off the line (%.1f)"), OffLine), FMath::IsNearlyEqual(OffLine, 30.0, 0.5));
	const float Before = PackRules::BearingAround(Prey, Me);
	TestTrue(TEXT("...and its bearing round the prey grows as it runs"), PackRules::BearingAround(Prey, Me + Way * 100.0) > Before);
	TestTrue(TEXT("...a negative flank's shrinks"), PackRules::BearingAround(Prey, Me + PackRules::FlankDirection(Me, Prey, -30.f, 450.f) * 100.0) < Before);

	// In a level: three spiders abreast 15 m from the player, hunting them, spread out over angles; one alone comes straight.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	const float Max = UEncounterSettings::Get().FlankMaxDegrees;
	TArray<ACreatureBase*> Pack;
	for (const double Bearing : { -40.0, 0.0, 40.0 })
	{
		const double Radians = FMath::DegreesToRadians(Bearing);
		Pack.Add(Hunter(*this, World, ASpiderCreature::StaticClass(), FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.0) * 1500.0, Player));
	}
	if (Pack.Contains(nullptr))
	{
		return false;
	}
	for (ACreatureBase* Creature : Pack)
	{
		Creature->GetPack()->LookAtPackNow();
	}
	TestTrue(TEXT("Hunting the player"), Pack[0]->GetCreatureState() == ECreatureState::Chase && Pack[2]->GetTarget() == Player);
	TestTrue(FString::Printf(TEXT("The low end swings out low (%.0f)"), Pack[0]->GetPack()->GetFlankDegrees()),
		FMath::IsNearlyEqual(Pack[0]->GetPack()->GetFlankDegrees(), -Max));
	TestTrue(TEXT("The middle comes straight"), FMath::IsNearlyZero(Pack[1]->GetPack()->GetFlankDegrees()));
	TestTrue(TEXT("The high end swings out high"), FMath::IsNearlyEqual(Pack[2]->GetPack()->GetFlankDegrees(), Max));
	const FVector HighGoal = Pack[2]->GetPack()->GetChaseGoal(*Player);
	TestTrue(TEXT("...running for a point round its own side, not at the player"),
		PackRules::BearingAround(FVector::ZeroVector, HighGoal) > PackRules::BearingAround(FVector::ZeroVector, Pack[2]->GetActorLocation()));
	TestTrue(TEXT("The middle runs at the player"), Pack[1]->GetPack()->GetChaseGoal(*Player).Equals(Player->GetActorLocation(), 1.0));

	// Two of them die: the last comes straight in.
	BossTestWorld::Kill(Pack[0]);
	BossTestWorld::Kill(Pack[1]);
	Pack[2]->GetPack()->LookAtPackNow();
	TestTrue(TEXT("Alone now, it comes straight"), FMath::IsNearlyZero(Pack[2]->GetPack()->GetFlankDegrees()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterPackRetreatTest, "Looter.Encounters.Packs.Retreat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterPackRetreatTest::RunTest(const FString& Parameters)
{
	// The rule: a Basic creature of a kind that runs, hurt to 35% or less, its pack gone (it had one), one roll a life.
	PackRules::FRetreatAsk Ask;
	Ask.HealthShare = 0.3f;
	Ask.bHadPack = true;
	Ask.PackmatesLeft = 0;
	Ask.HealthBelow = 0.35f;
	Ask.Chance = 0.5f;
	TestTrue(TEXT("Hurt and alone: a roll under the chance breaks it off"), PackRules::ShouldRetreat(Ask, 0.2f));
	TestFalse(TEXT("...one over it doesn't"), PackRules::ShouldRetreat(Ask, 0.7f));
	PackRules::FRetreatAsk Each = Ask;
	Each.Rank = ECreatureRank::Rare;
	TestFalse(TEXT("A Restless one never runs"), PackRules::WantsRetreatRoll(Each));
	Each = Ask;
	Each.PackmatesLeft = 1;
	TestFalse(TEXT("Not while a packmate stands"), PackRules::WantsRetreatRoll(Each));
	Each = Ask;
	Each.bHadPack = false;
	TestFalse(TEXT("Not one that never had a pack"), PackRules::WantsRetreatRoll(Each));
	Each = Ask;
	Each.HealthShare = 0.5f;
	TestFalse(TEXT("Not before it's hurt enough"), PackRules::WantsRetreatRoll(Each));
	Each = Ask;
	Each.bAlreadyRolled = true;
	TestFalse(TEXT("One roll a life"), PackRules::WantsRetreatRoll(Each));
	Each = Ask;
	Each.bAllowedKind = false;
	TestFalse(TEXT("Not a kind that never runs (the dead)"), PackRules::WantsRetreatRoll(Each));

	// Where it runs: straight away, or bent toward home rather than off its ground.
	auto Anywhere = [](const FVector&) { return true; };
	TestTrue(TEXT("Straight away from the player"), PackRules::RetreatGoal(FVector(200.0, 0.0, 0.0), FVector(1200.0, 0.0, 0.0), FVector::ZeroVector, 700.f,
		Anywhere).Equals(FVector(-500.0, 0.0, 0.0), 1.0));
	auto NearHome = [](const FVector& Point) { return FVector2D(Point.X, Point.Y).Size() <= 400.0; };
	const FVector Me(200.0, 300.0, 0.0);
	const FVector Bent = PackRules::RetreatGoal(Me, FVector(200.0, 1300.0, 0.0), FVector::ZeroVector, 400.f, NearHome);
	TestTrue(TEXT("Away would leave its ground: bent toward home, still away from the player, on its ground"),
		NearHome(Bent) && FVector::Dist2D(Bent, FVector(200.0, 1300.0, 0.0)) > FVector::Dist2D(Me, FVector(200.0, 1300.0, 0.0)));

	// In a level: two spiders hunting the player; one is hurt to 30%, then its packmate dies.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector::ZeroVector);
	ACreatureBase* Wounded = Hunter(*this, World, ASpiderCreature::StaticClass(), FVector(1200.0, 0.0, 0.0), Player);
	ACreatureBase* Mate = Hunter(*this, World, ASpiderCreature::StaticClass(), FVector(1200.0, 600.0, 0.0), Player);
	ACreatureBase* Stubborn = Hunter(*this, World, ASpiderCreature::StaticClass(), FVector(-6000.0, 0.0, 0.0), Player);
	ACreatureBase* StubbornMate = Hunter(*this, World, ASpiderCreature::StaticClass(), FVector(-6000.0, 600.0, 0.0), Player);
	if (!Player || !Wounded || !Mate || !Stubborn || !StubbornMate)
	{
		return false;
	}
	const UEncounterSettings& Settings = UEncounterSettings::Get();
	// Its one roll, chosen by seed: under the chance for the first, over it for the second.
	Wounded->GetPack()->SetSeed(SeedRolling(Settings.RetreatChance, true));
	Stubborn->GetPack()->SetSeed(SeedRolling(Settings.RetreatChance, false));
	for (ACreatureBase* Creature : { Wounded, Stubborn })
	{
		UHealthComponent* Life = Creature->FindComponentByClass<UHealthComponent>();
		Life->SetHealth(Life->GetMaxHealth() * 0.3f);
	}
	// Each looks at its pack while its packmate stands: that's the pack it can lose.
	Wounded->GetPack()->LookAtPackNow();
	Stubborn->GetPack()->LookAtPackNow();
	TestTrue(TEXT("Hurt, but its packmate stands: it fights on"), !Wounded->GetPack()->IsRetreating() && Wounded->GetPack()->HadPack());
	TestTrue(TEXT("...the other too"), !Stubborn->GetPack()->IsRetreating() && Stubborn->GetPack()->HadPack());
	BossTestWorld::Kill(Mate);
	BossTestWorld::Kill(StubbornMate);
	Wounded->GetPack()->LookAtPackNow();
	TestTrue(TEXT("Its pack gone: it breaks off"), Wounded->GetPack()->IsRetreating());
	const FVector Goal = Wounded->GetPack()->GetChaseGoal(*Player);
	TestTrue(TEXT("...running away from the player"), FVector::Dist2D(Goal, Player->GetActorLocation()) > FVector::Dist2D(Wounded->GetActorLocation(),
		Player->GetActorLocation()));
	Stubborn->GetPack()->LookAtPackNow();
	TestTrue(TEXT("Another, its roll over the chance, stands its ground"), !Stubborn->GetPack()->IsRetreating() && Stubborn->GetPack()->HasRolledRetreat());
	Stubborn->GetPack()->LookAtPackNow();
	TestFalse(TEXT("...and doesn't roll again"), Stubborn->GetPack()->IsRetreating());
	Wounded->GetPack()->TickChase(Settings.RetreatSeconds + 0.1f);
	TestFalse(TEXT("A moment later it turns again"), Wounded->GetPack()->IsRetreating());
	Wounded->GetPack()->LookAtPackNow();
	TestFalse(TEXT("...and keeps coming: once a life"), Wounded->GetPack()->IsRetreating());

	// The dead never run, and nor does a ranked creature.
	ACreatureBase* Dead = Hunter(*this, World, AUnpaidCreature::StaticClass(), FVector(0.0, 6000.0, 0.0), Player);
	ACreatureBase* DeadMate = Hunter(*this, World, AUnpaidCreature::StaticClass(), FVector(0.0, 6600.0, 0.0), Player);
	ACreatureBase* Ranked = Hunter(*this, World, ASpiderCreature::StaticClass(), FVector(0.0, -6000.0, 0.0), Player, ECreatureRank::Rare);
	ACreatureBase* RankedMate = Hunter(*this, World, ASpiderCreature::StaticClass(), FVector(0.0, -6600.0, 0.0), Player);
	if (!Dead || !DeadMate || !Ranked || !RankedMate)
	{
		return false;
	}
	for (ACreatureBase* Creature : { Dead, Ranked })
	{
		Creature->GetPack()->SetSeed(SeedRolling(Settings.RetreatChance, true));
		Creature->GetPack()->LookAtPackNow();
		UHealthComponent* Life = Creature->FindComponentByClass<UHealthComponent>();
		Life->SetHealth(Life->GetMaxHealth() * 0.3f);
	}
	BossTestWorld::Kill(DeadMate);
	BossTestWorld::Kill(RankedMate);
	Dead->GetPack()->LookAtPackNow();
	Ranked->GetPack()->LookAtPackNow();
	TestTrue(TEXT("An Unpaid alone and hurt doesn't run"), !Dead->GetPack()->IsRetreating() && !Dead->GetPack()->HasRolledRetreat());
	TestTrue(TEXT("A Restless spider alone and hurt doesn't run"), !Ranked->GetPack()->IsRetreating() && !Ranked->GetPack()->HasRolledRetreat());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterPackRankStingTest, "Looter.Encounters.Packs.RankSting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEncounterPackRankStingTest::RunTest(const FString& Parameters)
{
	// Who stings: Restless and better, not a boss (its own bar and show), its tag shown, once a life.
	TestFalse(TEXT("Basic: no sting"), PackRules::ShouldRankSting(ECreatureRank::Basic, true, false));
	TestTrue(TEXT("Restless"), PackRules::ShouldRankSting(ECreatureRank::Rare, true, false));
	TestTrue(TEXT("Gravebound"), PackRules::ShouldRankSting(ECreatureRank::Epic, true, false));
	TestTrue(TEXT("Soulfed"), PackRules::ShouldRankSting(ECreatureRank::Legendary, true, false));
	TestFalse(TEXT("A boss: its own show"), PackRules::ShouldRankSting(ECreatureRank::Boss, true, false));
	TestFalse(TEXT("A tag hidden for a boss bar"), PackRules::ShouldRankSting(ECreatureRank::Rare, false, false));
	TestFalse(TEXT("Once a life"), PackRules::ShouldRankSting(ECreatureRank::Rare, true, true));

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector::ZeroVector);
	ACreatureBase* Plain = Hunter(*this, World, ASpiderCreature::StaticClass(), FVector(1500.0, 0.0, 0.0), Player);
	ACreatureBase* Leader = Hunter(*this, World, ASpiderCreature::StaticClass(), FVector(1500.0, 800.0, 0.0), Player, ECreatureRank::Rare);
	if (!Player || !Plain || !Leader)
	{
		return false;
	}
	TestEqual(TEXT("A Basic spider turning on the player: no sting"), Plain->GetPack()->GetRankStings(), 0);
	TestEqual(TEXT("The Restless leader's first sight: the sting"), Leader->GetPack()->GetRankStings(), 1);
	TestTrue(TEXT("...its tag can flash its rank's word now"), Leader->GetPack()->GetRankStingAge() < 1.f);

	// It loses the player and finds them again: no second sting this life.
	Leader->DevPutInState(ECreatureState::Return);
	Leader->AlertTo(Player);
	TestTrue(TEXT("Hunting again"), Leader->GetCreatureState() == ECreatureState::Chase);
	TestEqual(TEXT("...without a second sting"), Leader->GetPack()->GetRankStings(), 1);

	// A second Restless turning a moment later: one sting for both.
	ACreatureBase* Second = Hunter(*this, World, ASpiderCreature::StaticClass(), FVector(1500.0, -800.0, 0.0), Player, ECreatureRank::Rare);
	if (!Second)
	{
		return false;
	}
	TestTrue(TEXT("A second Restless just after: its sting stands with the first's"), Second->GetPack()->HasRankStung()
		&& Second->GetPack()->GetRankStings() == 0);

	// A new life (a placed creature coming back) may sting again.
	Leader->GetPack()->ResetLife();
	TestFalse(TEXT("A new life: not stung yet"), Leader->GetPack()->HasRankStung());
	Leader->DevPutInState(ECreatureState::Idle);
	Leader->AlertTo(Player);
	TestEqual(TEXT("...and its first sight of the player stings"), Leader->GetPack()->GetRankStings(), 2);
	return true;
}

#endif
