#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/DeadMinerCreature.h"
#include "Creatures/ForemanCreature.h"
#include "Creatures/PlaceholderCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/StoneSentinelCreature.h"
#include "Creatures/StoneSentinelRules.h"
#include "Economy/EconomyRules.h"
#include "Loot/KillLootRules.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// The RR tuning the user took (2026-10-29 night): a Regular Stone Sentinel pays as a Restless one (its loot table, a
// Restless Unpaid's silver) and is worth 45; the Gravepicker placeholders pay the Unpaid's silver odds.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKillLootRulesTest, "Looter.Loot.KillLootRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKillLootRulesTest::RunTest(const FString& Parameters)
{
	const ACreatureBase* Sentinel = GetDefault<AStoneSentinelCreature>();
	const ACreatureBase* Spider = GetDefault<ASpiderCreature>();
	TestEqual(TEXT("A Sentinel is worth 45"), Sentinel->XPReward, 45);
	TestTrue(TEXT("A Regular Sentinel carries the Restless table"), KillLootRules::LootRankFor(*Sentinel, ECreatureRank::Basic) == ECreatureRank::Rare);
	TestTrue(TEXT("...a Gravebound one its own"), KillLootRules::LootRankFor(*Sentinel, ECreatureRank::Epic) == ECreatureRank::Epic);
	TestTrue(TEXT("...a Soulfed one its own"), KillLootRules::LootRankFor(*Sentinel, ECreatureRank::Legendary) == ECreatureRank::Legendary);
	TestTrue(TEXT("A spider carries its own rank's"), KillLootRules::LootRankFor(*Spider, ECreatureRank::Basic) == ECreatureRank::Basic);

	const FKillSilver StatueSilver = KillLootRules::SilverFor(*Sentinel);
	TestTrue(TEXT("A Regular Sentinel pays a Restless Unpaid's purse"), StatueSilver.Rank == ECreatureRank::Rare && StatueSilver.bUnpaid);
	TestTrue(TEXT("...more than a Regular Unpaid's"), LooterEconomy::KillOdds(StatueSilver.Rank, StatueSilver.bUnpaid).Average()
		> LooterEconomy::KillOdds(ECreatureRank::Basic, true).Average());
	TestFalse(TEXT("A spider pays as a beast"), KillLootRules::PaysAsUnpaid(*Spider));
	TestTrue(TEXT("A dead miner pays as the Unpaid"), KillLootRules::PaysAsUnpaid(*GetDefault<ADeadMinerCreature>()));
	TestTrue(TEXT("...and the Foreman"), KillLootRules::PaysAsUnpaid(*GetDefault<AForemanCreature>()));

	// The placeholders: by their pack (create_placeholder_creatures.py gives the diggers and the lookouts Gravepicker).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	APlaceholderCreature* Digger = World->SpawnActor<APlaceholderCreature>(FVector::ZeroVector, FRotator::ZeroRotator);
	APlaceholderCreature* Knocker = World->SpawnActor<APlaceholderCreature>(FVector(0.0, 600.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A placeholder"), Digger) || !TestNotNull(TEXT("Another"), Knocker))
	{
		return false;
	}
	Digger->PackTag = TEXT("Gravepicker");
	Knocker->PackTag = TEXT("Knocker");
	TestTrue(TEXT("A Gravepicker pays as the Unpaid"), KillLootRules::PaysAsUnpaid(*Digger));
	TestFalse(TEXT("...a Knocker as a beast"), KillLootRules::PaysAsUnpaid(*Knocker));
	return true;
}

#endif
