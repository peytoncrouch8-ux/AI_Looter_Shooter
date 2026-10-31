#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// A brown spider's cunning in a fight (Creatures/SpiderHuntRules; the user, 2026-10-31: their AI is "lackluster"): it circles
// toward the player's side or back before it rushes in, backs off when hurt, now and then skips back after a bite, and one
// given its turn just as a packmate's bite began circles first (a plain brown spider only: not a spiderling, whose pacing is
// the Gravemother's fight's). The fight director's own rules for spiders (no family gap) are untouched.

#include "Creatures/SpiderCreature.h"
#include "Creatures/SpiderHuntRules.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderHuntRulesTest, "Looter.Creatures.Spider.Hunt.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderHuntRulesTest::RunTest(const FString& Parameters)
{
	using namespace SpiderHuntRules;
	const FSpiderHuntTuning Tuning;
	// Circling first: a roll under its chance, from near enough.
	TestTrue(TEXT("A fresh turn near the player, a low roll: it circles first"), WantsCircle(400.f, 1.f, 0.1f, false, Tuning));
	TestFalse(TEXT("...a high roll: straight in"), WantsCircle(400.f, 1.f, 0.95f, false, Tuning));
	TestFalse(TEXT("...far off: it closes in first"), WantsCircle(Tuning.CircleRadius * Tuning.CircleFrom + 50.f, 1.f, 0.1f, false, Tuning));
	TestTrue(TEXT("A packmate's bite just began: it circles whatever its roll"), WantsCircle(400.f, 1.f, 0.99f, true, Tuning));
	TestTrue(TEXT("Its circle lasts a moment"), CircleSeconds(0.f, false, Tuning) >= 0.5f && CircleSeconds(1.f, false, Tuning) <= 1.2f);
	TestTrue(TEXT("...crowded, at least the bites' spacing"), CircleSeconds(0.f, true, Tuning) >= Tuning.BiteSpacing);

	// Which way round: on toward their back.
	const FVector Player = FVector::ZeroVector;
	const FVector Facing(1.0, 0.0, 0.0);
	TestTrue(TEXT("On their right (+Y as they face +X) it goes on round (+1)"), CircleSide(FVector(0.0, 300.0, 0.0), Player, Facing, 0.9f) > 0.f);
	TestTrue(TEXT("...on their left, the other way (-1)"), CircleSide(FVector(0.0, -300.0, 0.0), Player, Facing, 0.1f) < 0.f);
	TestTrue(TEXT("Straight in front of them, its dice choose"), CircleSide(FVector(300.0, 0.0, 0.0), Player, Facing, 0.1f) < 0.f
		&& CircleSide(FVector(300.0, 0.0, 0.0), Player, Facing, 0.9f) > 0.f);
	// Going on round from their right with +1 takes it behind them.
	const FVector Right(0.0, 330.0, 0.0);
	const FVector Way = CircleWay(Right, Player, 1.f, 330.f, Tuning.CircleRadialGain);
	TestTrue(FString::Printf(TEXT("From their right, +1 heads round behind them (%s)"), *Way.ToString()), Way.X < -0.9);
	TestTrue(TEXT("...along the circle at its radius"), FMath::Abs(FVector::DotProduct(Way, Right.GetSafeNormal())) < 1e-3);
	TestTrue(TEXT("Outside its radius it pulls in; inside, out"), FVector::DotProduct(CircleWay(FVector(0.0, 600.0, 0.0), Player, 1.f, 330.f,
		Tuning.CircleRadialGain), FVector(0.0, 1.0, 0.0)) < 0.0 && FVector::DotProduct(CircleWay(FVector(0.0, 150.0, 0.0), Player, 1.f, 330.f,
		Tuning.CircleRadialGain), FVector(0.0, 1.0, 0.0)) > 0.0);
	TestTrue(TEXT("Standing on the player, no way"), CircleWay(Player, Player, 1.f, 330.f, 1.f).IsNearlyZero());

	// Backing off.
	TestFalse(TEXT("A scratch doesn't send it back"), WantsBackOff(Tuning.BackOffShare * 0.5f, 100.f, 0.f, Tuning));
	TestTrue(TEXT("A burst of damage does (with its roll)"), WantsBackOff(Tuning.BackOffShare, 100.f, 0.1f, Tuning));
	TestFalse(TEXT("...not again so soon"), WantsBackOff(1.f, Tuning.BackOffCooldown - 0.5f, 0.1f, Tuning));
	TestFalse(TEXT("...nor on a high roll (it doesn't always)"), WantsBackOff(1.f, 100.f, 0.99f, Tuning));
	TestNearlyEqual(TEXT("Its hurt fades over its window"), FadeHurt(1.f, Tuning.BackOffWindow, Tuning), FMath::Exp(-1.f), 1e-4f);
	const FVector Back = BackOffWay(FVector(200.0, 0.0, 0.0), Player, 1.f);
	TestTrue(FString::Printf(TEXT("It backs away from the player, a little aside (%s)"), *Back.ToString()), Back.X > 0.8 && FMath::Abs(Back.Y) > 0.1);
	TestTrue(TEXT("After a bite it sometimes skips back"), WantsHitAndRun(100.f, 0.1f, Tuning) && !WantsHitAndRun(100.f, 0.9f, Tuning));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderHuntGapTest, "Looter.Creatures.Spider.Hunt.Gap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderHuntGapTest::RunTest(const FString& Parameters)
{
	// The fight director's rules for spiders are as they were: no family gap (their spacing is their own circling), a
	// spiderling's either.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Spider = Cast<ASpiderCreature>(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector::ZeroVector));
	ASpiderCreature* Spiderling = Cast<ASpiderCreature>(EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(600.0, 0.0, 0.0)));
	if (!TestNotNull(TEXT("A spider"), Spider) || !TestNotNull(TEXT("A spiderling"), Spiderling))
	{
		return false;
	}
	Spiderling->SetBodyScale(0.45f);
	TestNearlyEqual(TEXT("A brown spider's family gap at the director: none"), Spider->GetMeleeGap(), 0.f, 1e-4f);
	TestNearlyEqual(TEXT("...a spiderling's: none"), Spiderling->GetMeleeGap(), 0.f, 1e-4f);
	TestTrue(TEXT("A plain spider fights its own way (it holds no turn yet: plans nothing)"), !Spider->GetHunt().bTurnPlanned
		&& Spider->GetHunt().SkitterLeft <= 0.f);
	TestTrue(TEXT("Not in an ambush, it isn't clinging"), !Spider->IsClinging() && Spider->GetLurkPhase() == ESpiderLurkPhase::None);
	return true;
}

#endif
