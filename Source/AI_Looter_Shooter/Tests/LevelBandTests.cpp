#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/LootDropComponent.h"
#include "Progression/LevelRules.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Progression/XPCurve.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	/** A transient area for a test, never listed among the project's (UAreaDefinition::LoadAll). */
	UAreaDefinition* MakeTestArea(int32 Lowest, int32 Highest, float RareChance = 0.f, float EpicChance = 0.f)
	{
		UAreaDefinition* TestArea = NewObject<UAreaDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
		TestArea->MinLevel = Lowest;
		TestArea->MaxLevel = Highest;
		TestArea->RarePromotionChance = RareChance;
		TestArea->EpicPromotionChance = EpicChance;
		return TestArea;
	}

	const TCHAR* RansomsRestMap = TEXT("/Game/Maps/Lvl_RansomsRest");
	const TCHAR* TutorialIslandMap = TEXT("/Game/Maps/Lvl_TutorialIsland");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAreaLevelBandTest, "Looter.Areas.LevelBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAreaLevelBandTest::RunTest(const FString& Parameters)
{
	// Docs/Story.md, "Enemy levels": a creature's level is the player's plus -1, 0 or +1, kept inside its area's band (its
	// rank's levels come on top, from the creature). Bosses skip the roll. Levels follow the player inside the band: a
	// player who rushes the story meets the floor, one who comes back strong the ceiling.
	const UAreaDefinition* Valley = MakeTestArea(1, 10);
	TestTrue(TEXT("A band from 1 to 10"), Valley->HasLevelBand() && Valley->GetBandTop() == 10);
	bool bAlwaysInside = true;
	for (int32 PlayerLevel = 1; PlayerLevel <= 80; ++PlayerLevel)
	{
		for (int32 Spread = -1; Spread <= 1; ++Spread)
		{
			const int32 Rolled = Valley->LevelFor(PlayerLevel, Spread, 3);
			bAlwaysInside &= Rolled >= 1 && Rolled <= 10 && (PlayerLevel + Spread < 1 || PlayerLevel + Spread > 10 || Rolled == PlayerLevel + Spread);
		}
	}
	TestTrue(TEXT("Every player level and roll: the player's level moved by the roll, inside the band"), bAlwaysInside);
	TestEqual(TEXT("A level 1 player rolling -1 meets the floor"), Valley->LevelFor(1, -1, 3), 1);
	TestEqual(TEXT("A level 5 player rolling +1"), Valley->LevelFor(5, 1, 3), 6);
	TestEqual(TEXT("A level 12 player meets the ceiling"), Valley->LevelFor(12, -1, 3), 10);

	// Rolled from a stream, as the area's rules roll them: every one of -1, 0 and +1 shows up, never anything else.
	FRandomStream Random(20261002);
	TSet<int32> Seen;
	bool bNearPlayer = true;
	for (int32 Roll = 0; Roll < 3000; ++Roll)
	{
		const int32 Rolled = UAreaRulesSubsystem::RollLevelIn(Valley, 5, 1, ECreatureRank::Basic, Random);
		bNearPlayer &= Rolled >= 4 && Rolled <= 6;
		Seen.Add(Rolled);
	}
	TestTrue(TEXT("A level 5 player meets levels 4 to 6"), bNearPlayer);
	TestEqual(TEXT("... all three of them"), Seen.Num(), 3);
	bool bBossSteady = true;
	for (int32 Roll = 0; Roll < 100; ++Roll)
	{
		bBossSteady &= UAreaRulesSubsystem::RollLevelIn(Valley, 5, 1, ECreatureRank::Boss, Random) == 5;
	}
	TestTrue(TEXT("A boss is the player's level, never rolled"), bBossSteady);
	TestEqual(TEXT("A boss for a level 14 player: the band's ceiling"), UAreaRulesSubsystem::RollLevelIn(Valley, 14, 1, ECreatureRank::Boss, Random), 10);

	// Later bands (the Gilded Lily's 9-15), a one-level band (Skyreach's), and odd data.
	const UAreaDefinition* Lily = MakeTestArea(9, 15);
	TestEqual(TEXT("A rushed level 3 player on the Lily meets its floor"), Lily->LevelFor(3, 0, 1), 9);
	TestEqual(TEXT("A level 30 player meets its ceiling"), Lily->LevelFor(30, 1, 1), 15);
	const UAreaDefinition* Practice = MakeTestArea(1, 1);
	TestEqual(TEXT("A one-level band is that level for everyone"), Practice->LevelFor(40, 1, 7), 1);
	const UAreaDefinition* Upside = MakeTestArea(5, 3);
	TestEqual(TEXT("A ceiling under the floor reads as the floor"), Upside->GetBandTop(), 5);
	TestEqual(TEXT("... so every level is the floor"), Upside->LevelFor(9, 1, 1), 5);

	// No band (an area asset from before bands, read with the fields' defaults, or a level that's no area's): creatures
	// keep the level they were placed or spawned at.
	const UAreaDefinition* Older = NewObject<UAreaDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
	TestFalse(TEXT("An area asset without the band's fields has no band"), Older->HasLevelBand());
	TestEqual(TEXT("... and its creatures keep their level"), Older->LevelFor(30, 1, 4), 4);
	TestEqual(TEXT("... rolled too"), UAreaRulesSubsystem::RollLevelIn(Older, 30, 4, ECreatureRank::Basic, Random), 4);
	TestEqual(TEXT("No area at all: the creature's own level"), UAreaRulesSubsystem::RollLevelIn(nullptr, 30, 4, ECreatureRank::Basic, Random), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureLevelScalingTest, "Looter.Creatures.LevelScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureLevelScalingTest::RunTest(const FString& Parameters)
{
	// Docs/Story.md, "Health and damage": x(1 + 0.08 x (level - 1)), the rule guns' damage follows, so a gun of the
	// creature's level always takes the same number of hits. Its rank multiplies on top, at its rank's level.
	const FLevelRules Shipped;
	TestNearlyEqual(TEXT("Level 1: as its class made it"), Shipped.EnemyScale(1), 1.f, 1e-6f);
	TestNearlyEqual(TEXT("Level 10: x1.72"), Shipped.EnemyScale(10), 1.72f, 1e-5f);
	const FLevelRules Rules = UPlayerProgressionSubsystem::GetLevelRules();
	TestNearlyEqual(TEXT("Creatures grow as guns' damage does (UWeaponDefinition::DamagePerLevel)"), Rules.EnemyGrowth,
		GetDefault<UWeaponDefinition>()->DamagePerLevel, 1e-6f);

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(FVector::ZeroVector, FRotator::ZeroRotator);
	ASlimeCreature* Slime = World->SpawnActor<ASlimeCreature>(FVector(0.0, 3000.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Spider spawned"), Spider) || !TestNotNull(TEXT("Slime spawned"), Slime))
	{
		return false;
	}
	float SpiderHealth = 0.f;
	const TArray<ACreatureBase*> Creatures = { Spider, Slime };
	for (ACreatureBase* Creature : Creatures)
	{
		// The test world never begins play itself, and has no area: started by hand, at the level 1 it was placed at.
		Creature->DispatchBeginPlay();
		UHealthComponent* Health = Creature->FindComponentByClass<UHealthComponent>();
		const ULootDropComponent* Loot = Creature->FindComponentByClass<ULootDropComponent>();
		if (!TestNotNull(TEXT("Its health"), Health) || !TestNotNull(TEXT("Its loot"), Loot))
		{
			return false;
		}
		const FString Kind = Creature->GetClass()->GetName();
		TestEqual(*FString::Printf(TEXT("%s: placed at level 1 where there's no band, it's level 1"), *Kind), Creature->Level, 1);
		const float BaseHealth = Health->MaxHealth;
		const float BaseDamage = Creature->AttackDamage;
		const int32 BaseXP = Creature->XPReward;
		SpiderHealth = Creature == Spider ? BaseHealth : SpiderHealth;
		for (const int32 NewLevel : { 2, 5, 10, 30 })
		{
			Creature->SetLevel(NewLevel);
			const float Scale = Rules.EnemyScale(NewLevel);
			TestNearlyEqual(*FString::Printf(TEXT("%s at level %d: health x%.2f"), *Kind, NewLevel, Scale), Health->MaxHealth, BaseHealth * Scale, 0.05f);
			TestNearlyEqual(*FString::Printf(TEXT("%s at level %d: damage x%.2f"), *Kind, NewLevel, Scale), Creature->AttackDamage, BaseDamage * Scale, 0.001f);
			TestEqual(*FString::Printf(TEXT("%s at level %d: its experience is the kill's to grow"), *Kind, NewLevel), Creature->XPReward, BaseXP);
			TestEqual(*FString::Printf(TEXT("%s at level %d: drops level %d guns"), *Kind, NewLevel, NewLevel), Loot->Level, NewLevel);
		}

		// Linear: every level adds the same health, 8% of its level 1 health.
		Creature->SetLevel(7);
		const float At7 = Health->MaxHealth;
		Creature->SetLevel(8);
		const float At8 = Health->MaxHealth;
		Creature->SetLevel(9);
		const float At9 = Health->MaxHealth;
		TestNearlyEqual(*FString::Printf(TEXT("%s: each level adds the same health"), *Kind), At9 - At8, At8 - At7, 0.01f);
		TestNearlyEqual(*FString::Printf(TEXT("%s: a level's health is its share of level 1's"), *Kind), At8 - At7, BaseHealth * Rules.EnemyGrowth, 0.01f);

		// Hurt, it keeps its share of health as its level changes.
		Health->SetHealth(Health->MaxHealth * 0.5f);
		Creature->SetLevel(12);
		TestNearlyEqual(*FString::Printf(TEXT("%s: half hurt, still half hurt"), *Kind), Health->GetHealth(), Health->MaxHealth * 0.5f, 0.05f);

		// Its rank on top: a Gravebound one at level 10 is its rank's levels higher, with its rank's multipliers on that level.
		Creature->SetLevel(10);
		Health->SetHealth(Health->MaxHealth);
		const FCreatureRankInfo& Epic = UCreatureRankSettings::Get(ECreatureRank::Epic);
		Creature->SetRank(ECreatureRank::Epic);
		const float EpicScale = Rules.EnemyScale(10 + Epic.LevelOffset);
		TestEqual(*FString::Printf(TEXT("%s, Gravebound: its rank's levels on top"), *Kind), Creature->Level, 10 + Epic.LevelOffset);
		TestNearlyEqual(*FString::Printf(TEXT("%s, Gravebound: its rank's health on its level's"), *Kind), Health->MaxHealth,
			BaseHealth * EpicScale * Epic.HealthMultiplier, 0.05f);
		TestNearlyEqual(*FString::Printf(TEXT("%s, Gravebound: its rank's damage on its level's"), *Kind), Creature->AttackDamage,
			BaseDamage * EpicScale * Epic.DamageMultiplier, 0.001f);
		TestEqual(*FString::Printf(TEXT("%s, Gravebound: its rank's experience"), *Kind), Creature->XPReward, FMath::RoundToInt32(BaseXP * Epic.XPMultiplier));
		Creature->SetRank(ECreatureRank::Basic);
		TestEqual(*FString::Printf(TEXT("%s, Basic again: level 10"), *Kind), Creature->Level, 10);
		TestNearlyEqual(*FString::Printf(TEXT("%s, Basic again: level 10's health"), *Kind), Health->MaxHealth, BaseHealth * Rules.EnemyScale(10), 0.05f);
	}

	// The design's own numbers (Docs/Areas/RansomsRest.md): a spider is 300 at level 1, and the Gravemother, a Soulfed
	// spider at level 8, about 5,600.
	const FCreatureRankInfo& Legendary = UCreatureRankSettings::Get(ECreatureRank::Legendary);
	AddInfo(FString::Printf(TEXT("A spider: %.0f health at level 1, %.0f at level 10; the Gravemother (x%.0f) at level 8: %.0f"), SpiderHealth,
		SpiderHealth * Rules.EnemyScale(10), Legendary.HealthMultiplier, SpiderHealth * Rules.EnemyScale(8) * Legendary.HealthMultiplier));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAreaPromotionsTest, "Looter.Areas.Promotions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAreaPromotionsTest::RunTest(const FString& Parameters)
{
	// Docs/Story.md, "Enemy ranks": on Ransom's Rest an arrival makes 8% of placed Basic creatures Restless and 2% Gravebound,
	// for one life. A seeded simulation of 200,000 creatures' rolls lands on those shares.
	const UAreaDefinition* Valley = MakeTestArea(1, 10, 0.08f, 0.02f);
	TestTrue(TEXT("Ransom's Rest's chances promote"), Valley->HasPromotions());
	constexpr int32 Rolls = 200000;
	FRandomStream Random(20261002);
	int32 Counts[LooterRanks::NumRanks] = {};
	for (int32 Roll = 0; Roll < Rolls; ++Roll)
	{
		++Counts[static_cast<int32>(UAreaRulesSubsystem::RollPromotionIn(Valley, Random))];
	}
	const double RareShare = static_cast<double>(Counts[static_cast<int32>(ECreatureRank::Rare)]) / Rolls;
	const double EpicShare = static_cast<double>(Counts[static_cast<int32>(ECreatureRank::Epic)]) / Rolls;
	AddInfo(FString::Printf(TEXT("%d placed Basic creatures: %.2f%% Restless, %.2f%% Gravebound, %.2f%% Basic"), Rolls, RareShare * 100.0,
		EpicShare * 100.0, 100.0 * Counts[static_cast<int32>(ECreatureRank::Basic)] / Rolls));
	TestNearlyEqual(TEXT("8% Restless"), RareShare, 0.08, 0.003);
	TestNearlyEqual(TEXT("2% Gravebound"), EpicShare, 0.02, 0.0015);
	TestEqual(TEXT("Never a Legendary monster: they're hand-placed"), Counts[static_cast<int32>(ECreatureRank::Legendary)], 0);
	TestEqual(TEXT("Never a boss"), Counts[static_cast<int32>(ECreatureRank::Boss)], 0);

	// The rolls' edges: the rarer rank first, then the next, then Basic.
	TestTrue(TEXT("A roll under 2%: Gravebound"), Valley->PickPromotion(0.f) == ECreatureRank::Epic && Valley->PickPromotion(0.019f) == ECreatureRank::Epic);
	TestTrue(TEXT("From 2% to 10%: Restless"), Valley->PickPromotion(0.021f) == ECreatureRank::Rare && Valley->PickPromotion(0.099f) == ECreatureRank::Rare);
	TestTrue(TEXT("Above: Basic"), Valley->PickPromotion(0.101f) == ECreatureRank::Basic && Valley->PickPromotion(1.f) == ECreatureRank::Basic);
	const UAreaDefinition* Crowded = MakeTestArea(1, 10, 0.9f, 0.6f);
	TestTrue(TEXT("Chances past 100%: the rarer rank keeps its share"), Crowded->PickPromotion(0.59f) == ECreatureRank::Epic
		&& Crowded->PickPromotion(0.61f) == ECreatureRank::Rare && Crowded->PickPromotion(0.99f) == ECreatureRank::Rare);
	const UAreaDefinition* Older = NewObject<UAreaDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
	TestFalse(TEXT("An area asset without the chances' fields (Skyreach's too) promotes nothing"), Older->HasPromotions());
	TestTrue(TEXT("... whatever the roll"), UAreaRulesSubsystem::RollPromotionIn(Older, Random) == ECreatureRank::Basic
		&& UAreaRulesSubsystem::RollPromotionIn(nullptr, Random) == ECreatureRank::Basic);

	// The cooldown (Docs/Story.md, "No reload farming"): a map's promotions roll again only on an arrival 20 minutes of play
	// after their last roll, so Save & Quit then Continue doesn't reroll them. Never rolled, they roll; a time played that
	// went back (an odd save) rolls rather than waiting it out.
	TestEqual(TEXT("Twenty minutes of play"), USessionSubsystem::PromotionCooldown, 1200.0);
	TestTrue(TEXT("Never rolled: rolls"), USessionSubsystem::IsPromotionRollDue(-1.0, 0.0));
	TestFalse(TEXT("Ten minutes on: no"), USessionSubsystem::IsPromotionRollDue(0.0, 600.0));
	TestTrue(TEXT("Twenty minutes on: rolls"), USessionSubsystem::IsPromotionRollDue(0.0, 1200.0));
	TestTrue(TEXT("Rolled \"later\" than now: rolls"), USessionSubsystem::IsPromotionRollDue(500.0, 100.0));
	struct FArrival
	{
		double PlayedSeconds;
		const TCHAR* What;
		bool bRolls;
	};
	const FArrival Arrivals[] = {
		{ 0.0, TEXT("a new session arrives"), true },
		{ 300.0, TEXT("Save & Quit, then Continue"), false },
		{ 900.0, TEXT("back from a practice trip to Skyreach"), false },
		{ 1250.0, TEXT("Continue the next day, 20 minutes of play on"), true },
		{ 1400.0, TEXT("Continue again at once"), false },
		{ 2449.0, TEXT("back from Skyreach a second short of 20 minutes"), false },
		{ 2450.0, TEXT("and at 20 minutes"), true } };
	double RolledAt = -1.0;
	for (const FArrival& Arrival : Arrivals)
	{
		const bool bDue = USessionSubsystem::IsPromotionRollDue(RolledAt, Arrival.PlayedSeconds);
		TestTrue(FString::Printf(TEXT("At %.0f s of play, %s: %s"), Arrival.PlayedSeconds, Arrival.What, Arrival.bRolls ? TEXT("rolls") : TEXT("keeps them")),
			bDue == Arrival.bRolls);
		RolledAt = bDue ? Arrival.PlayedSeconds : RolledAt;
	}

	// Kept per map with the session (FSavedMapWorld::PromotionsRolledAt), through the save format and back.
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->PlayedSeconds = 1300.0;
	Save->FindOrAddWorld(RansomsRestMap).PromotionsRolledAt = 1250.0;
	Save->FindOrAddWorld(TutorialIslandMap);
	TArray<uint8> Bytes;
	const ULooterSessionSave* ReadBack = UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	if (!TestNotNull(TEXT("The session read back"), ReadBack))
	{
		return false;
	}
	const FSavedMapWorld* Rest = ReadBack->FindWorld(RansomsRestMap);
	const FSavedMapWorld* Island = ReadBack->FindWorld(TutorialIslandMap);
	if (!TestTrue(TEXT("Both maps' worlds read back"), Rest && Island))
	{
		return false;
	}
	TestNearlyEqual(TEXT("Ransom's Rest's last roll is kept"), Rest->PromotionsRolledAt, 1250.0, 1e-6);
	TestFalse(TEXT("... so Continue 50 s later keeps its promotions"), USessionSubsystem::IsPromotionRollDue(Rest->PromotionsRolledAt, ReadBack->PlayedSeconds));
	TestTrue(TEXT("A map never rolled rolls on its first arrival"), USessionSubsystem::IsPromotionRollDue(Island->PromotionsRolledAt, ReadBack->PlayedSeconds));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAreaBandsAssetsTest, "Looter.Areas.Bands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAreaBandsAssetsTest::RunTest(const FString& Parameters)
{
	// The project's areas as Tools/Unreal/create_area_assets.py writes them: Ransom's Rest's band is 1-10 until the campaign
	// is finished, with 8% and 2% promotions; Skyreach's creatures are level 1 and never promoted.
	const UAreaDefinition* Skyreach = UAreaDefinition::FindByName(TEXT("Skyreach"));
	const UAreaDefinition* RansomsRest = UAreaDefinition::FindByName(TEXT("RansomsRest"));
	if (!Skyreach || !RansomsRest || !Skyreach->HasLevelBand() || !RansomsRest->HasLevelBand())
	{
		AddError(TEXT("DA_Area_Skyreach and DA_Area_RansomsRest need their level bands: run Tools/Unreal/create_area_assets.py in the editor."));
		return false;
	}
	TestTrue(TEXT("Ransom's Rest's creatures are levels 1 to 10"), RansomsRest->MinLevel == 1 && RansomsRest->GetBandTop() == 10);
	TestNearlyEqual(TEXT("Ransom's Rest: 8% Restless"), RansomsRest->RarePromotionChance, 0.08f, 1e-6f);
	TestNearlyEqual(TEXT("Ransom's Rest: 2% Gravebound"), RansomsRest->EpicPromotionChance, 0.02f, 1e-6f);
	TestTrue(TEXT("Skyreach's creatures are level 1"), Skyreach->MinLevel == 1 && Skyreach->GetBandTop() == 1);
	TestFalse(TEXT("Skyreach promotes nothing"), Skyreach->HasPromotions());

	// Every area: a band inside the levels players can reach, and chances that leave most creatures Basic.
	const int32 TopLevel = UPlayerProgressionSubsystem::GetCurve().MaxLevel;
	for (const UAreaDefinition* Area : UAreaDefinition::LoadAll())
	{
		const FString Label = Area->GetName();
		TestTrue(*FString::Printf(TEXT("%s has a band"), *Label), Area->HasLevelBand());
		TestTrue(*FString::Printf(TEXT("%s's band is inside levels 1 to %d"), *Label, TopLevel), Area->GetBandTop() <= TopLevel);
		TestTrue(*FString::Printf(TEXT("%s promotes fewer than half its creatures"), *Label), Area->RarePromotionChance + Area->EpicPromotionChance < 0.5f);
	}
	return true;
}

#endif
