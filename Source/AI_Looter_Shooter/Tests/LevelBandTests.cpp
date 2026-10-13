#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Loot/LootDropComponent.h"
#include "Progression/LevelRules.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Progression/XPCurve.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/AreaLevelsLayout.h"
#include "Tests/EncounterLayoutReader.h"
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

	/** A square zone HalfSide (cm) either side of (X, Y), at levels Lowest to Highest. */
	FAreaLevelZone MakeSquareZone(const TCHAR* Id, double X, double Y, double HalfSide, int32 Lowest, int32 Highest)
	{
		FAreaLevelZone Zone;
		Zone.Id = FName(Id);
		Zone.Outline = { FVector2D(X - HalfSide, Y - HalfSide), FVector2D(X + HalfSide, Y - HalfSide), FVector2D(X + HalfSide, Y + HalfSide),
			FVector2D(X - HalfSide, Y + HalfSide) };
		Zone.MinLevel = Lowest;
		Zone.MaxLevel = Highest;
		return Zone;
	}

	/** Every level RollLevelIn gives a creature of Kind at Spot over many rolls (none chosen for it). */
	TSet<int32> LevelsRolledAt(const UAreaDefinition* Area, const FVector& Spot, const UClass* Kind, FRandomStream& Random)
	{
		TSet<int32> Seen;
		for (int32 Roll = 0; Roll < 2000; ++Roll)
		{
			Seen.Add(UAreaRulesSubsystem::RollLevelIn(Area, Spot, Kind, 1, false, Random));
		}
		return Seen;
	}

	const TCHAR* RansomsRestMap = TEXT("/Game/Maps/Lvl_RansomsRest");
	const TCHAR* TutorialIslandMap = TEXT("/Game/Maps/Lvl_TutorialIsland");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAreaLevelBandTest, "Looter.Areas.LevelBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAreaLevelBandTest::RunTest(const FString& Parameters)
{
	// Docs/Progression.md, "Creature levels" (the user's call, 2026-10-12): a creature's level is where it stands, never the
	// player's. Its kind's set level here (a boss, a Legendary monster), else the first zone holding its spot, rolled evenly
	// between the zone's levels; outside every zone the band's floor or one above; always inside the area's band. A level
	// chosen for it (placed at another than its class's) is kept. Its rank's levels come on top, from the creature.
	UAreaDefinition* Valley = MakeTestArea(1, 8);
	Valley->LevelZones = { MakeSquareZone(TEXT("pit"), 0.0, 0.0, 1000.0, 5, 6), MakeSquareZone(TEXT("fields"), 0.0, 0.0, 5000.0, 2, 3),
		MakeSquareZone(TEXT("peak"), 20000.0, 0.0, 1000.0, 9, 12) };
	FAreaKindLevel& Boss = Valley->KindLevels.AddDefaulted_GetRef();
	Boss.Kind = ASpiderCreature::StaticClass();
	Boss.Level = 7;
	const FVector InPit(200.0, -300.0, 50.0);
	const FVector InFields(3000.0, 2000.0, 0.0);
	const FVector Nowhere(9000.0, 9000.0, 0.0);
	const FVector OnPeak(20000.0, 0.0, 0.0);

	// The zone holding a spot: the first listed, so the pit stands out of the fields round it.
	const FAreaLevelZone* Zone = Valley->FindLevelZone(InPit);
	TestTrue(TEXT("The pit holds a spot in it, though the fields do too"), Zone && Zone->Id == FName(TEXT("pit")));
	Zone = Valley->FindLevelZone(InFields);
	TestTrue(TEXT("The fields hold one outside the pit"), Zone && Zone->Id == FName(TEXT("fields")));
	TestNull(TEXT("No zone holds a spot past them all"), Valley->FindLevelZone(Nowhere));
	TestFalse(TEXT("A zone with fewer than three corners holds nothing"), FAreaLevelZone().Contains(FVector2D::ZeroVector));

	// The levels at a spot: its zone's, the band's floor and one above outside them, kept inside the band.
	int32 Lowest = 0;
	int32 Highest = 0;
	Valley->GetLevelsAt(InPit, Lowest, Highest);
	TestTrue(TEXT("The pit: 5-6"), Lowest == 5 && Highest == 6);
	Valley->GetLevelsAt(InFields, Lowest, Highest);
	TestTrue(TEXT("The fields: 2-3"), Lowest == 2 && Highest == 3);
	Valley->GetLevelsAt(Nowhere, Lowest, Highest);
	TestTrue(TEXT("Outside every zone: the band's floor or one above (1-2)"), Lowest == 1 && Highest == 2);
	Valley->GetLevelsAt(OnPeak, Lowest, Highest);
	TestTrue(TEXT("A zone past the band's top is kept inside it (8)"), Lowest == 8 && Highest == 8);

	// Rolled evenly: each of the zone's levels comes up, nothing else; a roll's edges land where they should.
	FRandomStream Random(20261012);
	const UClass* Slime = ASlimeCreature::StaticClass();
	const TSet<int32> PitLevels = LevelsRolledAt(Valley, InPit, Slime, Random);
	TestTrue(TEXT("A slime in the pit is level 5 or 6, both seen"), PitLevels.Num() == 2 && PitLevels.Contains(5) && PitLevels.Contains(6));
	const TSet<int32> FieldLevels = LevelsRolledAt(Valley, InFields, Slime, Random);
	TestTrue(TEXT("...in the fields 2 or 3"), FieldLevels.Num() == 2 && FieldLevels.Contains(2) && FieldLevels.Contains(3));
	TestEqual(TEXT("A roll of 0 picks the zone's lowest"), Valley->PickLevel(InPit, Slime, 0.f), 5);
	TestEqual(TEXT("...of 0.49 still the lowest"), Valley->PickLevel(InPit, Slime, 0.49f), 5);
	TestEqual(TEXT("...of 0.5 the next"), Valley->PickLevel(InPit, Slime, 0.5f), 6);
	TestEqual(TEXT("...of 1 the highest"), Valley->PickLevel(InPit, Slime, 1.f), 6);

	// A kind at a set level is it wherever it stands, never rolled; its subclasses too (the Spitter is a spider).
	TestEqual(TEXT("The set kind's level"), Valley->FindKindLevel(ASpiderCreature::StaticClass()), 7);
	TestEqual(TEXT("...and its subclass's"), Valley->FindKindLevel(ASpitterSpider::StaticClass()), 7);
	TestEqual(TEXT("Another kind has none"), Valley->FindKindLevel(Slime), 0);
	TestTrue(TEXT("The set kind is 7 in the pit, the fields and nowhere"), LevelsRolledAt(Valley, InPit, ASpiderCreature::StaticClass(), Random).Array()
		== TArray<int32>({ 7 }) && LevelsRolledAt(Valley, Nowhere, ASpiderCreature::StaticClass(), Random).Array() == TArray<int32>({ 7 }));

	// A level chosen for it is kept, inside the band; the kind's set level doesn't override it.
	TestEqual(TEXT("Placed at 4 in the pit: 4"), UAreaRulesSubsystem::RollLevelIn(Valley, InPit, Slime, 4, true, Random), 4);
	TestEqual(TEXT("Placed at 12: the band's top"), UAreaRulesSubsystem::RollLevelIn(Valley, InPit, Slime, 12, true, Random), 8);
	TestEqual(TEXT("A set kind placed at 3: 3"), UAreaRulesSubsystem::RollLevelIn(Valley, InPit, ASpiderCreature::StaticClass(), 3, true, Random), 3);

	// Other bands: a one-level one (Skyreach's), one with no zones yet (the Gilded Lily's 9-15), one upside down.
	const UAreaDefinition* Practice = MakeTestArea(1, 1);
	TestTrue(TEXT("A one-level band is that level everywhere"), LevelsRolledAt(Practice, InPit, Slime, Random).Array() == TArray<int32>({ 1 }));
	const UAreaDefinition* Lily = MakeTestArea(9, 15);
	const TSet<int32> LilyLevels = LevelsRolledAt(Lily, InPit, Slime, Random);
	TestTrue(TEXT("A band with no zones: its floor or one above (9-10)"), LilyLevels.Num() == 2 && LilyLevels.Contains(9) && LilyLevels.Contains(10));
	const UAreaDefinition* Upside = MakeTestArea(5, 3);
	TestEqual(TEXT("A ceiling under the floor reads as the floor"), Upside->GetBandTop(), 5);
	TestTrue(TEXT("...so every level is the floor"), LevelsRolledAt(Upside, InPit, Slime, Random).Array() == TArray<int32>({ 5 }));

	// No band (an area asset from before bands, read with the fields' defaults, or a level that's no area's): creatures
	// keep the level they were placed or spawned at.
	const UAreaDefinition* Older = NewObject<UAreaDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
	TestFalse(TEXT("An area asset without the band's fields has no band"), Older->HasLevelBand());
	TestEqual(TEXT("...and its creatures keep their level"), UAreaRulesSubsystem::RollLevelIn(Older, InPit, Slime, 4, false, Random), 4);
	TestEqual(TEXT("No area at all: the creature's own level"), UAreaRulesSubsystem::RollLevelIn(nullptr, InPit, Slime, 4, false, Random), 4);

	// Chests' loot still follows the player inside the band (AChest::GetLootLevel): only creatures stopped doing so.
	TestEqual(TEXT("A chest's level for a level 12 player in a 1-8 band: 8"), Valley->LevelFor(12, 0, 1), 8);
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
	// spider at her level 6 on Ransom's Rest (4 and her rank's two), about 5,000.
	const FCreatureRankInfo& Legendary = UCreatureRankSettings::Get(ECreatureRank::Legendary);
	AddInfo(FString::Printf(TEXT("A spider: %.0f health at level 1, %.0f at level 10; the Gravemother (x%.0f) at level 6: %.0f"), SpiderHealth,
		SpiderHealth * Rules.EnemyScale(10), Legendary.HealthMultiplier, SpiderHealth * Rules.EnemyScale(6) * Legendary.HealthMultiplier));
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
	// The project's areas as Tools/Unreal/create_area_assets.py writes them from the layouts' gameplay.levels: Ransom's
	// Rest's creatures are levels 1-6 before their ranks by zone (its band 1-7 since the mountain: Abel and the Foreman at 7
	// with their ranks), its boss and Legendary monsters at set levels, with 8% and 2% promotions; Skyreach's creatures are
	// level 1 and never promoted.
	const UAreaDefinition* Skyreach = UAreaDefinition::FindByName(TEXT("Skyreach"));
	const UAreaDefinition* RansomsRest = UAreaDefinition::FindByName(TEXT("RansomsRest"));
	if (!Skyreach || !RansomsRest || !Skyreach->HasLevelBand() || !RansomsRest->HasLevelBand())
	{
		AddError(TEXT("DA_Area_Skyreach and DA_Area_RansomsRest need their level bands: run Tools/Unreal/create_area_assets.py in the editor."));
		return false;
	}
	TestNearlyEqual(TEXT("Ransom's Rest: 8% Restless"), RansomsRest->RarePromotionChance, 0.08f, 1e-6f);
	TestNearlyEqual(TEXT("Ransom's Rest: 2% Gravebound"), RansomsRest->EpicPromotionChance, 0.02f, 1e-6f);
	TestTrue(TEXT("Skyreach's creatures are level 1"), Skyreach->MinLevel == 1 && Skyreach->GetBandTop() == 1);
	TestTrue(TEXT("...wherever they stand: no zones, no set kinds"), Skyreach->LevelZones.IsEmpty() && Skyreach->KindLevels.IsEmpty());
	TestFalse(TEXT("Skyreach promotes nothing"), Skyreach->HasPromotions());

	// Ransom's Rest's asset holds its layout's levels (the script was run since gameplay.levels last changed).
	EncounterLayout::FAreaLayout Layout;
	const UAreaDefinition* Planned = EncounterLayout::ReadLayout(*this, Layout) ? AreaLevelsLayout::ReadArea(*this, Layout) : nullptr;
	if (!Planned)
	{
		return false;
	}
	const FString Rerun = TEXT("(run Tools/Unreal/create_area_assets.py again)");
	TestTrue(*FString::Printf(TEXT("Ransom's Rest's band is the layout's, %d-%d %s"), Planned->MinLevel, Planned->GetBandTop(), *Rerun),
		RansomsRest->MinLevel == Planned->MinLevel && RansomsRest->GetBandTop() == Planned->GetBandTop());
	if (TestEqual(*FString::Printf(TEXT("...its zones are the layout's %s"), *Rerun), RansomsRest->LevelZones.Num(), Planned->LevelZones.Num()))
	{
		for (int32 Index = 0; Index < Planned->LevelZones.Num(); ++Index)
		{
			const FAreaLevelZone& Made = RansomsRest->LevelZones[Index];
			const FAreaLevelZone& Wanted = Planned->LevelZones[Index];
			bool bSameOutline = Made.Outline.Num() == Wanted.Outline.Num();
			for (int32 Corner = 0; bSameOutline && Corner < Wanted.Outline.Num(); ++Corner)
			{
				bSameOutline = FVector2D::Distance(Made.Outline[Corner], Wanted.Outline[Corner]) <= 1.0;
			}
			TestTrue(*FString::Printf(TEXT("Zone %s: levels %d-%d and its outline, in the layout's order"), *Wanted.Id.ToString(), Wanted.MinLevel,
				Wanted.GetTop()), Made.Id == Wanted.Id && Made.MinLevel == Wanted.MinLevel && Made.GetTop() == Wanted.GetTop() && bSameOutline);
		}
	}
	if (TestEqual(*FString::Printf(TEXT("...its kinds at set levels are the layout's %s"), *Rerun), RansomsRest->KindLevels.Num(), Planned->KindLevels.Num()))
	{
		for (int32 Index = 0; Index < Planned->KindLevels.Num(); ++Index)
		{
			const FAreaKindLevel& Made = RansomsRest->KindLevels[Index];
			const FAreaKindLevel& Wanted = Planned->KindLevels[Index];
			TestTrue(*FString::Printf(TEXT("%s at level %d"), *GetNameSafe(Wanted.Kind.Get()), Wanted.Level), Made.Kind.Get() == Wanted.Kind.Get() && Made.Level == Wanted.Level);
		}
	}

	// Every area: a band inside the levels players can reach, its zones and set kinds inside its band, and chances that
	// leave most creatures Basic.
	const int32 TopLevel = UPlayerProgressionSubsystem::GetCurve().MaxLevel;
	for (const UAreaDefinition* Area : UAreaDefinition::LoadAll())
	{
		const FString Label = Area->GetName();
		TestTrue(*FString::Printf(TEXT("%s has a band"), *Label), Area->HasLevelBand());
		TestTrue(*FString::Printf(TEXT("%s's band is inside levels 1 to %d"), *Label, TopLevel), Area->GetBandTop() <= TopLevel);
		for (const FAreaLevelZone& Zone : Area->LevelZones)
		{
			TestTrue(*FString::Printf(TEXT("%s's zone %s: levels inside its band, an outline"), *Label, *Zone.Id.ToString()),
				Zone.MinLevel >= Area->MinLevel && Zone.GetTop() <= Area->GetBandTop() && Zone.Outline.Num() >= 3);
		}
		for (const FAreaKindLevel& Kind : Area->KindLevels)
		{
			TestTrue(*FString::Printf(TEXT("%s's %s: a kind at a level inside its band"), *Label, *GetNameSafe(Kind.Kind.Get())),
				Kind.Kind.Get() && Kind.Level >= Area->MinLevel && Kind.Level <= Area->GetBandTop());
		}
		TestTrue(*FString::Printf(TEXT("%s promotes fewer than half its creatures"), *Label), Area->RarePromotionChance + Area->EpicPromotionChance < 0.5f);
	}
	return true;
}

#endif
