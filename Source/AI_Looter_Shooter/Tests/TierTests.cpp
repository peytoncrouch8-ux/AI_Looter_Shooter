#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/SpiderCreature.h"
#include "Economy/EconomyRules.h"
#include "Progression/ProgressionSettings.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The five enemy tiers (Saved/Tiers/DESIGN.md, the user's approval of 2026-10-20; Docs/Story.md, "Enemy ranks and
// legendary drops"): their order, defense, stats by tier, spawn odds and caps, traits' seed, and what a save keeps.
// Looter.Loot.RankOdds checks their loot; Looter.Areas.Promotions their odds by area.

namespace
{
	/** A spider placed in World under Name (as a level holds it, so a load finds the same name) at Spot, started at Rank. */
	ASpiderCreature* PlaceSpider(UWorld* World, const TCHAR* Name, ECreatureRank Rank, const FVector& Spot = FVector::ZeroVector, int32 Level = 4)
	{
		FActorSpawnParameters Params;
		Params.Name = FName(Name);
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(Spot, FRotator::ZeroRotator, Params);
		if (Spider)
		{
			Spider->StartingRank = Rank;
			Spider->Level = Level;
			Spider->DispatchBeginPlay();
		}
		return Spider;
	}

	TArray<ECreatureTrait> TraitsOf(const ACreatureBase& Creature)
	{
		const UCreatureTraitComponent* Traits = Creature.GetTraitComponent();
		return Traits ? TArray<ECreatureTrait>(Traits->GetTraits()) : TArray<ECreatureTrait>();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTierOrderTest, "Looter.Tiers.Order",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTierOrderTest::RunTest(const FString& Parameters)
{
	// Saved ranks keep their numbers (levels, encounter groups and egg sacs hold them): the two new tiers were appended.
	TestEqual(TEXT("Basic is 0"), static_cast<int32>(ECreatureRank::Basic), 0);
	TestEqual(TEXT("Rare is 1"), static_cast<int32>(ECreatureRank::Rare), 1);
	TestEqual(TEXT("Epic is 2"), static_cast<int32>(ECreatureRank::Epic), 2);
	TestEqual(TEXT("Legendary is 3"), static_cast<int32>(ECreatureRank::Legendary), 3);
	TestEqual(TEXT("Boss is 4"), static_cast<int32>(ECreatureRank::Boss), 4);
	TestEqual(TEXT("Hallowed is 5"), static_cast<int32>(ECreatureRank::Hallowed), 5);
	TestEqual(TEXT("Ethereal is 6"), static_cast<int32>(ECreatureRank::Ethereal), 6);

	// Their order is the helpers', never their numbers.
	for (int32 Index = 0; Index + 1 < LooterRanks::NumRanks; ++Index)
	{
		const ECreatureRank Lower = LooterRanks::All[Index];
		const ECreatureRank Higher = LooterRanks::All[Index + 1];
		TestTrue(FString::Printf(TEXT("%s is above %s"), *UCreatureRankSettings::GetTierName(Higher), *UCreatureRankSettings::GetTierName(Lower)),
			LooterRanks::IsAbove(Higher, Lower) && !LooterRanks::IsAbove(Lower, Higher) && LooterRanks::IsAtLeast(Higher, Lower));
	}
	TestTrue(TEXT("Ethereal is above Gravebound, though its number is above a boss's"), LooterRanks::IsAbove(ECreatureRank::Ethereal, ECreatureRank::Epic)
		&& LooterRanks::IsAbove(ECreatureRank::Boss, ECreatureRank::Ethereal) && LooterRanks::IsAbove(ECreatureRank::Legendary, ECreatureRank::Hallowed));
	TestTrue(TEXT("A rank is at least itself, not above it"), LooterRanks::IsAtLeast(ECreatureRank::Hallowed, ECreatureRank::Hallowed)
		&& !LooterRanks::IsAbove(ECreatureRank::Hallowed, ECreatureRank::Hallowed));
	TestEqual(TEXT("Five tiers"), LooterRanks::NumTiers, 5);
	for (int32 Tier = 0; Tier < LooterRanks::NumTiers; ++Tier)
	{
		TestEqual(FString::Printf(TEXT("Tier %d's index"), Tier), LooterRanks::TierIndex(LooterRanks::Tiers[Tier]), Tier);
		TestTrue(FString::Printf(TEXT("Tier %d is a tier"), Tier), LooterRanks::IsTier(LooterRanks::Tiers[Tier]));
	}
	TestTrue(TEXT("A Soulfed monster and a boss are no tier"), !LooterRanks::IsTier(ECreatureRank::Legendary) && !LooterRanks::IsTier(ECreatureRank::Boss)
		&& LooterRanks::TierIndex(ECreatureRank::Boss) == INDEX_NONE);
	TestTrue(TEXT("The tier below Ethereal is Hallowed, below Regular Regular"), LooterRanks::TierBelow(ECreatureRank::Ethereal) == ECreatureRank::Hallowed
		&& LooterRanks::TierBelow(ECreatureRank::Rare) == ECreatureRank::Basic && LooterRanks::TierBelow(ECreatureRank::Basic) == ECreatureRank::Basic);

	// Players' names and the code's both name them.
	const TPair<const TCHAR*, ECreatureRank> Names[] = { { TEXT("Regular"), ECreatureRank::Basic }, { TEXT("restless"), ECreatureRank::Rare },
		{ TEXT("GRAVEBOUND"), ECreatureRank::Epic }, { TEXT("Hallowed"), ECreatureRank::Hallowed }, { TEXT("ethereal"), ECreatureRank::Ethereal },
		{ TEXT("Soulfed"), ECreatureRank::Legendary }, { TEXT("Epic"), ECreatureRank::Epic } };
	for (const TPair<const TCHAR*, ECreatureRank>& Name : Names)
	{
		ECreatureRank Parsed = ECreatureRank::Boss;
		TestTrue(FString::Printf(TEXT("\"%s\" names its rank"), Name.Key), UCreatureRankSettings::ParseRank(Name.Key, Parsed) && Parsed == Name.Value);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTierDefenseTest, "Looter.Tiers.Defense",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTierDefenseTest::RunTest(const FString& Parameters)
{
	// Defense takes its share of each hit; a crit pierces half of it (the user's call): 35% takes 100 to 65, a crit to 82.5.
	TestNearlyEqual(TEXT("No defense: all of it"), UHealthComponent::AfterDefense(100.f, 0.f, false), 100.f, 1e-4f);
	TestNearlyEqual(TEXT("35%: a body shot for 100 takes 65"), UHealthComponent::AfterDefense(100.f, 0.35f, false), 65.f, 1e-4f);
	TestNearlyEqual(TEXT("35%: a crit for 100 takes 82.5"), UHealthComponent::AfterDefense(100.f, 0.35f, true), 82.5f, 1e-4f);
	TestNearlyEqual(TEXT("Never past 90%"), UHealthComponent::AfterDefense(100.f, 5.f, false), 10.f, 1e-4f);

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const FLevelRules Levels = GetDefault<UProgressionSettings>()->GetLevelRules();
	ASpiderCreature* Regular = PlaceSpider(World, TEXT("DefenseSpider_Regular"), ECreatureRank::Basic, FVector(0.0, -3000.0, 0.0));
	if (!TestNotNull(TEXT("A Regular spider"), Regular))
	{
		return false;
	}
	const float RegularHealth = Regular->FindComponentByClass<UHealthComponent>()->GetMaxHealth();
	const float RegularBite = Regular->AttackDamage;
	const int32 RegularXP = Regular->XPReward;
	double Column = 0.0;
	for (const ECreatureRank Tier : LooterRanks::Tiers)
	{
		Column += 2000.0;
		const FString Name = UCreatureRankSettings::GetTierName(Tier);
		const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Tier);
		for (const bool bCritical : { false, true })
		{
			ASpiderCreature* Spider = PlaceSpider(World, *FString::Printf(TEXT("DefenseSpider_%s_%d"), *Name, bCritical ? 1 : 0), Tier,
				FVector(Column, bCritical ? 1500.0 : 0.0, 0.0));
			UHealthComponent* Health = Spider ? Spider->FindComponentByClass<UHealthComponent>() : nullptr;
			if (!TestNotNull(FString::Printf(TEXT("%s spider"), *Name), Health))
			{
				return false;
			}
			// Defense alone: a trait that takes hits first (Warded's ward) is taken off for this.
			if (UCreatureTraitComponent* Traits = Spider->GetTraitComponent())
			{
				Traits->SetTraits({});
			}
			TestNearlyEqual(FString::Printf(TEXT("%s: its tier's defense"), *Name), Spider->GetDefense(), Info.Defense, 1e-5f);
			const float Before = Health->GetHealth();
			{
				// As AActor::TakeDamage hands a shot to its listeners: a test level has no game mode, so a pawn ignores
				// ApplyDamage, and actors' own events need letting through before play (FeedbackTests' Shoot).
				FEditorScriptExecutionGuard RunActorEvents;
				const UDamageType* Type = bCritical ? GetDefault<UWeaponCritDamageType>() : GetDefault<UWeaponDamageType>();
				Spider->OnTakeAnyDamage.Broadcast(Spider, 100.f, Type, nullptr, nullptr);
			}
			const float Wanted = 100.f * (1.f - Info.Defense * (bCritical ? 0.5f : 1.f));
			TestNearlyEqual(FString::Printf(TEXT("%s: a %s for 100 takes %.1f"), *Name, bCritical ? TEXT("crit") : TEXT("body shot"), Wanted),
				Before - Health->GetHealth(), Wanted, 0.01f);

			if (!bCritical)
			{
				// Its health, bite, experience and level by its tier, at its tier's levels on top of the Regular one's.
				const float Growth = Levels.EnemyScale(4 + Info.LevelOffset) / Levels.EnemyScale(4);
				TestNearlyEqual(FString::Printf(TEXT("%s: x%.2f health at its levels"), *Name, Info.HealthMultiplier), Health->GetMaxHealth(),
					RegularHealth * Info.HealthMultiplier * Growth, 0.05f);
				TestNearlyEqual(FString::Printf(TEXT("%s: x%.2f bite"), *Name, Info.DamageMultiplier), Spider->AttackDamage,
					RegularBite * Info.DamageMultiplier * Growth, 0.01f);
				TestEqual(FString::Printf(TEXT("%s: x%.1f experience"), *Name, Info.XPMultiplier), Spider->XPReward,
					FMath::RoundToInt32(RegularXP * Info.XPMultiplier));
				TestEqual(FString::Printf(TEXT("%s: +%d levels"), *Name, Info.LevelOffset), Spider->Level, 4 + Info.LevelOffset);
				TestNearlyEqual(FString::Printf(TEXT("%s: %.2fx size"), *Name, Info.Size), Spider->GetSizeScale(), Info.Size, 1e-4f);
				const float Effective = Health->GetMaxHealth() / FMath::Max(1.f - Info.Defense, 0.1f) / RegularHealth;
				AddInfo(FString::Printf(TEXT("%s: %.0f health, %.0f%% defense: %.2fx a Regular one's body shots at level 4"), *Name,
					Health->GetMaxHealth(), Info.Defense * 100.f, Effective));
			}
		}
	}

	// The design's effective health at the valley's middle level: 1.0, 2.4, 4.0, 5.5, 7.0 (within 5%).
	const float Effective[] = { 1.f, 2.4f, 4.f, 5.5f, 7.f };
	for (int32 Tier = 0; Tier < LooterRanks::NumTiers; ++Tier)
	{
		const FCreatureRankInfo& Info = UCreatureRankSettings::Get(LooterRanks::Tiers[Tier]);
		const float Got = Info.HealthMultiplier * Levels.EnemyScale(4 + Info.LevelOffset) / Levels.EnemyScale(4) / (1.f - Info.Defense);
		TestTrue(FString::Printf(TEXT("%s: %.2fx a Regular one's effective health (design %.1f)"),
			*UCreatureRankSettings::GetTierName(LooterRanks::Tiers[Tier]), Got, Effective[Tier]), FMath::Abs(Got - Effective[Tier]) <= Effective[Tier] * 0.05f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTierSpawnsTest, "Looter.Tiers.Spawns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTierSpawnsTest::RunTest(const FString& Parameters)
{
	// The share of regular spawns each tier takes, on Ransom's Rest (its place 0) and by the late areas (1), from 2,000,000
	// seeded rolls: 8 / 2 / 0.4 / 0.08% rising to 12 / 4 / 1 / 0.25%.
	struct FOdds
	{
		ECreatureRank Tier;
		double First;
		double Late;
	};
	const FOdds Design[] = { { ECreatureRank::Rare, 0.08, 0.12 }, { ECreatureRank::Epic, 0.02, 0.04 }, { ECreatureRank::Hallowed, 0.004, 0.01 },
		{ ECreatureRank::Ethereal, 0.0008, 0.0025 } };
	constexpr int32 Rolls = 2000000;
	for (const float Rise : { 0.f, 1.f })
	{
		TMap<ECreatureRank, int32> Counts;
		FRandomStream Random(20261020);
		for (int32 Roll = 0; Roll < Rolls; ++Roll)
		{
			++Counts.FindOrAdd(UCreatureRankSettings::PickTier(Random.FRand(), Rise));
		}
		for (const FOdds& Odds : Design)
		{
			const double Wanted = Rise > 0.5f ? Odds.Late : Odds.First;
			const double Got = static_cast<double>(Counts.FindRef(Odds.Tier)) / Rolls;
			const double Spread = 5.0 * FMath::Sqrt(Wanted * (1.0 - Wanted) / Rolls);
			TestTrue(FString::Printf(TEXT("%s, %s: %.3f%% of spawns (design %.2f%%)"), *UCreatureRankSettings::GetTierName(Odds.Tier),
				Rise > 0.5f ? TEXT("the late areas") : TEXT("Ransom's Rest"), Got * 100.0, Wanted * 100.0), FMath::Abs(Got - Wanted) <= Spread);
			TestNearlyEqual(FString::Printf(TEXT("%s's chance as the settings give it"), *UCreatureRankSettings::GetTierName(Odds.Tier)),
				static_cast<double>(UCreatureRankSettings::SpawnChance(Odds.Tier, Rise)), Wanted, 1e-6);
		}
		TestEqual(TEXT("Never a Soulfed monster or a boss"), Counts.FindRef(ECreatureRank::Legendary) + Counts.FindRef(ECreatureRank::Boss), 0);
	}

	// The place in the campaign from the creatures' lowest level: 1 (Ransom's Rest) is 0, 50 and up the late areas.
	TestNearlyEqual(TEXT("Level 1: the first area"), UCreatureRankSettings::CampaignRise(1), 0.f, 1e-6f);
	TestNearlyEqual(TEXT("Level 50: the late areas"), UCreatureRankSettings::CampaignRise(50), 1.f, 1e-6f);
	TestNearlyEqual(TEXT("Level 65: still the late areas"), UCreatureRankSettings::CampaignRise(65), 1.f, 1e-6f);
	TestNearlyEqual(TEXT("Halfway: halfway"), UCreatureRankSettings::CampaignRise(1 + 49 / 2), 24.f / 49.f, 1e-5f);

	// The caps on living ones: Gravebound 4, Hallowed 2, Ethereal 1, Restless none. A roll past one steps down a tier until
	// one has room.
	TestEqual(TEXT("Gravebound: 4 alive at most"), UCreatureRankSettings::Get(ECreatureRank::Epic).MaxAlivePerArea, 4);
	TestEqual(TEXT("Hallowed: 2"), UCreatureRankSettings::Get(ECreatureRank::Hallowed).MaxAlivePerArea, 2);
	TestEqual(TEXT("Ethereal: 1"), UCreatureRankSettings::Get(ECreatureRank::Ethereal).MaxAlivePerArea, 1);
	TestEqual(TEXT("Restless: no cap"), UCreatureRankSettings::Get(ECreatureRank::Rare).MaxAlivePerArea, 0);
	auto Alive = [](int32 Gravebound, int32 Hallowed, int32 Ethereal)
	{
		return [=](ECreatureRank Tier)
		{
			return Tier == ECreatureRank::Epic ? Gravebound : (Tier == ECreatureRank::Hallowed ? Hallowed : (Tier == ECreatureRank::Ethereal ? Ethereal : 50));
		};
	};
	TestTrue(TEXT("Room: an Ethereal stays one"), UCreatureRankSettings::KeepUnderCaps(ECreatureRank::Ethereal, Alive(0, 0, 0)) == ECreatureRank::Ethereal);
	TestTrue(TEXT("One Ethereal alive: the next is Hallowed"), UCreatureRankSettings::KeepUnderCaps(ECreatureRank::Ethereal, Alive(0, 0, 1)) == ECreatureRank::Hallowed);
	TestTrue(TEXT("...and with two Hallowed alive too, Gravebound"), UCreatureRankSettings::KeepUnderCaps(ECreatureRank::Ethereal, Alive(3, 2, 1)) == ECreatureRank::Epic);
	TestTrue(TEXT("...and four Gravebound, Restless, which no cap holds"), UCreatureRankSettings::KeepUnderCaps(ECreatureRank::Ethereal, Alive(4, 2, 1)) == ECreatureRank::Rare);
	TestTrue(TEXT("Restless is never capped"), UCreatureRankSettings::KeepUnderCaps(ECreatureRank::Rare, Alive(4, 2, 1)) == ECreatureRank::Rare);
	TestTrue(TEXT("A Soulfed monster is never stepped down"), UCreatureRankSettings::KeepUnderCaps(ECreatureRank::Legendary, Alive(9, 9, 9)) == ECreatureRank::Legendary);

	// Silver: a Regular kill's times the tier's multiplier, on average (x1.5, x2.5, x4, x6), more often as well as more.
	for (const bool bUnpaid : { true, false })
	{
		const LooterEconomy::FDropOdds Regular = LooterEconomy::KillOdds(ECreatureRank::Basic, bUnpaid);
		float LastChance = Regular.Chance;
		for (int32 Tier = 1; Tier < LooterRanks::NumTiers; ++Tier)
		{
			const ECreatureRank Rank = LooterRanks::Tiers[Tier];
			const LooterEconomy::FDropOdds Odds = LooterEconomy::KillOdds(Rank, bUnpaid);
			const float Wanted = Regular.Average() * UCreatureRankSettings::Get(Rank).SilverMultiplier;
			TestTrue(FString::Printf(TEXT("%s %s: %.2f silver on average (x%.1f a Regular one's %.2f, within 12%%)"), *UCreatureRankSettings::GetTierName(Rank),
				bUnpaid ? TEXT("Unpaid") : TEXT("beast"), Odds.Average(), UCreatureRankSettings::Get(Rank).SilverMultiplier, Regular.Average()),
				FMath::Abs(Odds.Average() - Wanted) <= Wanted * 0.12f);
			TestTrue(FString::Printf(TEXT("%s %s: drops it more often than the tier below"), *UCreatureRankSettings::GetTierName(Rank),
				bUnpaid ? TEXT("Unpaid") : TEXT("beast")), Odds.Chance > LastChance);
			LastChance = Odds.Chance;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTierTraitsTest, "Looter.Tiers.Traits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTierTraitsTest::RunTest(const FString& Parameters)
{
	// Each tier's traits as its rank is applied: none for Regular and Restless, 1 Gravebound, 2 Hallowed, 3 Ethereal (always
	// Phasing among them), never twice the same; none again as it goes back to Regular.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ASpiderCreature* Spider = PlaceSpider(WorldWrapper.GetTestWorld(), TEXT("TraitSpider_1"), ECreatureRank::Basic);
	if (!TestNotNull(TEXT("A spider"), Spider) || !TestNotNull(TEXT("Every creature has a trait component"), Spider->GetTraitComponent()))
	{
		return false;
	}
	for (const ECreatureRank Tier : LooterRanks::Tiers)
	{
		Spider->SetRank(Tier);
		const TArray<ECreatureTrait> Traits = TraitsOf(*Spider);
		const FString Name = UCreatureRankSettings::GetTierName(Tier);
		TestEqual(FString::Printf(TEXT("%s: its tier's traits"), *Name), Traits.Num(), UCreatureRankSettings::Get(Tier).TraitCount);
		TSet<ECreatureTrait> Unique;
		for (const ECreatureTrait Trait : Traits)
		{
			Unique.Add(Trait);
		}
		TestEqual(FString::Printf(TEXT("%s: no trait twice"), *Name), Unique.Num(), Traits.Num());
		if (Tier == ECreatureRank::Ethereal)
		{
			TestTrue(TEXT("Ethereal: Phasing always one of them"), Traits.Contains(ECreatureTrait::Phasing));
		}
		// The same seed, the same traits: applying the rank again changes nothing.
		Spider->SetRank(Tier);
		TestTrue(FString::Printf(TEXT("%s: the same traits again from the same seed"), *Name), TraitsOf(*Spider) == Traits);
	}
	Spider->SetRank(ECreatureRank::Basic);
	TestEqual(TEXT("Back to Regular: no traits"), TraitsOf(*Spider).Num(), 0);
	Spider->SetRank(ECreatureRank::Legendary);
	TestEqual(TEXT("A Soulfed monster's moves are its own: no traits"), TraitsOf(*Spider).Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTierSaveTest, "Looter.Tiers.SaveRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTierSaveTest::RunTest(const FString& Parameters)
{
	// A session keeps no creatures (FSavedMapWorld: they're all back as placed whenever a map is played), only when the
	// map's promotions last rolled. So a save written and read back keeps the ranks as placed, and the traits too: a placed
	// creature's seed is its name in its level (with no promotion roll), which a load finds again. Within 20 minutes of play
	// a load rolls no promotions (no reload farming); a save from before the tiers reads the same (no ranks, nothing to
	// upgrade: the version stays 3).
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->PlayedSeconds = 900.0;
	Save->FindOrAddWorld(TEXT("/Game/Maps/Lvl_RansomsRest")).PromotionsRolledAt = 600.0;
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("The save writes"), UGameplayStatics::SaveGameToMemory(Save, Bytes)))
	{
		return false;
	}
	ULooterSessionSave* Read = Cast<ULooterSessionSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("...and reads back"), Read))
	{
		return false;
	}
	TestEqual(TEXT("Still version 3: the tiers add nothing to a save"), Read->Upgrade(), 3);
	const FSavedMapWorld* Valley = Read->FindWorld(TEXT("/Game/Maps/Lvl_RansomsRest"));
	TestTrue(TEXT("When the valley's promotions rolled is kept"), Valley && FMath::IsNearlyEqual(Valley->PromotionsRolledAt, 600.0));
	TestFalse(TEXT("Five minutes of play later, a load rolls no promotions"),
		USessionSubsystem::IsPromotionRollDue(Valley ? Valley->PromotionsRolledAt : -1.0, Read->PlayedSeconds));

	// The level as a load puts it back: the same placed creatures by name, at their placed ranks, with the same traits.
	TArray<TPair<ECreatureRank, TArray<ECreatureTrait>>> Before;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		FTestWorldWrapper WorldWrapper;
		if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
		{
			return false;
		}
		int32 Index = 0;
		for (const ECreatureRank Tier : { ECreatureRank::Epic, ECreatureRank::Hallowed, ECreatureRank::Ethereal, ECreatureRank::Ethereal })
		{
			const ASpiderCreature* Spider = PlaceSpider(WorldWrapper.GetTestWorld(), *FString::Printf(TEXT("BP_Spider_C_%d"), 10 + Index), Tier,
				FVector(2000.0 * Index, 0.0, 0.0));
			if (!TestNotNull(TEXT("A placed spider"), Spider))
			{
				return false;
			}
			const TArray<ECreatureTrait> Traits = TraitsOf(*Spider);
			if (Pass == 0)
			{
				Before.Add({ Spider->GetRank(), Traits });
			}
			else
			{
				TestTrue(FString::Printf(TEXT("Spider %d: its placed rank after the load"), Index), Spider->GetRank() == Before[Index].Key);
				TestTrue(FString::Printf(TEXT("Spider %d: the same traits after the load (%d)"), Index, Traits.Num()), Traits == Before[Index].Value);
			}
			++Index;
		}
	}
	return true;
}

#endif
