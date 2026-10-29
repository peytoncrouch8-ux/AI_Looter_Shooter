#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SlimeFightRules.h"
#include "Progression/ProgressionSettings.h"
#include "Tests/BossTestWorld.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

// Looter.Creatures.Slime.Fight: the meadow slime's fight pass (FIGHTS.md section 3, lane 4): the split's rules and its slimelets, the
// drumbeat gap, the dodge hop's rules. All of it runs on plain data or in a test level, so no cvar a player set counts.

namespace
{
	ASlimeCreature* SpawnSlime(UWorld* World, const FVector& At, ECreatureRank Rank)
	{
		ACreatureBase::FRuntimeSpawn Setup;
		Setup.Rank = Rank;
		ASlimeCreature* Slime = Cast<ASlimeCreature>(ACreatureBase::SpawnAtRuntime(World, ASlimeCreature::StaticClass(), At, 0.f, Setup));
		if (Slime && !Slime->HasActorBegunPlay())
		{
			Slime->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Slime);
		return Slime;
	}

	TArray<ASlimeCreature*> LivingSlimes(UWorld* World)
	{
		TArray<ASlimeCreature*> Alive;
		for (TActorIterator<ASlimeCreature> It(World); It; ++It)
		{
			if (!It->IsDead())
			{
				Alive.Add(*It);
			}
		}
		return Alive;
	}

	/** A level with Extras plain slimes about and one slime of Rank; kills the one; how many slimes live afterward (-1: no level). */
	int32 AliveAfterKill(ECreatureRank Rank, int32 Extras)
	{
		FTestWorldWrapper WorldWrapper;
		if (!WorldWrapper.CreateTestWorld(EWorldType::EditorPreview))
		{
			return -1;
		}
		UWorld* World = WorldWrapper.GetTestWorld();
		for (int32 Index = 0; Index < Extras; ++Index)
		{
			SpawnSlime(World, FVector(3000.0 + 400.0 * Index, 0.0, 0.0), ECreatureRank::Basic);
		}
		ASlimeCreature* Slime = SpawnSlime(World, FVector::ZeroVector, Rank);
		if (!Slime)
		{
			return -1;
		}
		BossTestWorld::Kill(Slime);
		return LivingSlimes(World).Num();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlimeFightRulesTest, "Looter.Creatures.Slime.Fight.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlimeFightRulesTest::RunTest(const FString& Parameters)
{
	const FSlimeFightTuning Tuning;
	TestEqual(TEXT("The drumbeat is 0.45 s"), Tuning.MeleeGap, 0.45f);
	TestEqual(TEXT("A split makes two"), Tuning.SplitPieces, 2);

	// Who splits: the tiers above Basic, once, under the caps.
	TestEqual(TEXT("Basic never splits"), SlimeFightRules::SplitPieces(Tuning, ECreatureRank::Basic, false, 0, 0, 20), 0);
	for (const ECreatureRank Rank : { ECreatureRank::Rare, ECreatureRank::Epic, ECreatureRank::Hallowed, ECreatureRank::Ethereal })
	{
		TestEqual(TEXT("A tier above Basic splits in two"), SlimeFightRules::SplitPieces(Tuning, Rank, false, 1, 3, 20), 2);
	}
	TestEqual(TEXT("A Legendary monster never splits"), SlimeFightRules::SplitPieces(Tuning, ECreatureRank::Legendary, false, 0, 0, 20), 0);
	TestEqual(TEXT("A boss never splits"), SlimeFightRules::SplitPieces(Tuning, ECreatureRank::Boss, false, 0, 0, 20), 0);
	TestEqual(TEXT("A slimelet never splits"), SlimeFightRules::SplitPieces(Tuning, ECreatureRank::Rare, true, 0, 0, 20), 0);
	FSlimeFightTuning Off = Tuning;
	Off.bSplit = false;
	TestEqual(TEXT("Switched off, no split"), SlimeFightRules::SplitPieces(Off, ECreatureRank::Epic, false, 0, 0, 20), 0);
	// The caps: slimes alive after the split at most SplitCap (10), creatures near the player at most the level's cap.
	TestEqual(TEXT("8 slimes alive: the two fit"), SlimeFightRules::SplitPieces(Tuning, ECreatureRank::Rare, false, 8, 0, 0), 2);
	TestEqual(TEXT("9 slimes alive: a denied split just dies"), SlimeFightRules::SplitPieces(Tuning, ECreatureRank::Rare, false, 9, 0, 0), 0);
	TestEqual(TEXT("18 creatures near the player of 20: the two fit"), SlimeFightRules::SplitPieces(Tuning, ECreatureRank::Rare, false, 1, 18, 20), 2);
	TestEqual(TEXT("19 near the player of 20: denied"), SlimeFightRules::SplitPieces(Tuning, ECreatureRank::Rare, false, 1, 19, 20), 0);
	TestEqual(TEXT("No near cap: only the slime cap counts"), SlimeFightRules::SplitPieces(Tuning, ECreatureRank::Rare, false, 1, 500, 0), 2);

	// Slimelets: Basic, the parent's level less its rank's offset, 0.55 size, 0.3 health.
	TestEqual(TEXT("A Restless level 3 (offset 1) makes level 2"), SlimeFightRules::SlimeletLevel(3, 1), 2);
	TestEqual(TEXT("Never under level 1"), SlimeFightRules::SlimeletLevel(1, 3), 1);
	const FCreatureRuntimeSpawn Spawn = SlimeFightRules::SlimeletSpawn(Tuning, 4, 1);
	TestTrue(TEXT("Basic"), Spawn.Rank == ECreatureRank::Basic);
	TestEqual(TEXT("Level 3"), Spawn.Level, 3);
	TestEqual(TEXT("Size 0.55"), Spawn.BodyScale, 0.55f);
	TestEqual(TEXT("Health 0.3"), Spawn.HealthScale, 0.3f);

	// Where they land: neighbours Spread (150 cm) apart.
	for (const int32 Pieces : { 2, 3, 4 })
	{
		const TArray<FVector> Offsets = SlimeFightRules::SplitOffsets(Pieces, Tuning.SplitSpread, 37.f);
		if (!TestEqual(TEXT("A spot each"), Offsets.Num(), Pieces))
		{
			continue;
		}
		for (int32 Index = 0; Index < Pieces; ++Index)
		{
			TestEqual(TEXT("Neighbours 150 cm apart"), static_cast<float>(FVector::Dist(Offsets[Index], Offsets[(Index + 1) % Pieces])),
				Tuning.SplitSpread, 0.5f);
			TestEqual(TEXT("Flat"), static_cast<float>(Offsets[Index].Z), 0.f);
		}
	}
	const TArray<FVector> Lone = SlimeFightRules::SplitOffsets(1, Tuning.SplitSpread, 0.f);
	TestTrue(TEXT("One piece stays in the middle"), Lone.Num() == 1 && Lone[0].IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlimeFightDodgeTest, "Looter.Creatures.Slime.Fight.Dodge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlimeFightDodgeTest::RunTest(const FString& Parameters)
{
	const FSlimeFightTuning Tuning;
	{
		// Three hits in 0.8 s while waiting its turn: a dodge on the third, with the roll under the chance.
		FSlimeDodgeTracker Tracker;
		TestFalse(TEXT("One hit: no"), Tracker.Note(10.0, Tuning, true, false, 0.f));
		TestFalse(TEXT("Two hits: no"), Tracker.Note(10.2, Tuning, true, false, 0.f));
		TestTrue(TEXT("Three in 0.8 s: dodge"), Tracker.Note(10.4, Tuning, true, false, 0.f));
		TestEqual(TEXT("The hits are forgotten"), Tracker.NumRecent(), 0);
		// The cooldown (4 s) holds the next one back.
		TestFalse(TEXT("Cooling down: no"), Tracker.Note(11.0, Tuning, true, false, 0.f) || Tracker.Note(11.1, Tuning, true, false, 0.f)
			|| Tracker.Note(11.2, Tuning, true, false, 0.f));
		Tracker.Note(14.5, Tuning, true, false, 0.f);
		Tracker.Note(14.6, Tuning, true, false, 0.f);
		TestTrue(TEXT("After the cooldown: again"), Tracker.Note(14.7, Tuning, true, false, 0.f));
	}
	{
		// Slow hits never add up.
		FSlimeDodgeTracker Tracker;
		bool bAny = false;
		for (int32 Index = 0; Index < 8; ++Index)
		{
			bAny |= Tracker.Note(5.0 + 0.5 * Index, Tuning, true, false, 0.f);
		}
		TestFalse(TEXT("A hit every half second: never three in 0.8 s"), bAny);
	}
	{
		// Only while it waits its turn, and not while busy.
		FSlimeDodgeTracker Waiting;
		Waiting.Note(1.0, Tuning, false, false, 0.f);
		Waiting.Note(1.1, Tuning, false, false, 0.f);
		TestFalse(TEXT("Not waiting its turn: no"), Waiting.Note(1.2, Tuning, false, false, 0.f));
		FSlimeDodgeTracker Busy;
		Busy.Note(1.0, Tuning, true, true, 0.f);
		Busy.Note(1.1, Tuning, true, true, 0.f);
		TestFalse(TEXT("Mid-hop or attacking: no"), Busy.Note(1.2, Tuning, true, true, 0.f));
		TestTrue(TEXT("The hits still count: free again, the next hit dodges"), Busy.Note(1.3, Tuning, true, false, 0.f));
	}
	{
		// The roll: over the chance, no dodge, and the burst is spent.
		FSlimeDodgeTracker Tracker;
		Tracker.Note(2.0, Tuning, true, false, 0.9f);
		Tracker.Note(2.1, Tuning, true, false, 0.9f);
		TestFalse(TEXT("A roll over 0.5: no"), Tracker.Note(2.2, Tuning, true, false, 0.9f));
		TestEqual(TEXT("Each burst rolls once"), Tracker.NumRecent(), 0);
		FSlimeFightTuning Never = Tuning;
		Never.DodgeChance = 0.f;
		FSlimeDodgeTracker Quiet;
		Quiet.Note(3.0, Never, true, false, 0.f);
		Quiet.Note(3.1, Never, true, false, 0.f);
		TestFalse(TEXT("Chance 0: never"), Quiet.Note(3.2, Never, true, false, 0.f));
	}

	// The hop goes across the line to its target: flat, a unit long, to either side.
	const FVector ToTarget(300.f, 0.f, 80.f);
	const FVector Right = SlimeFightRules::DodgeDirection(ToTarget, true);
	const FVector Left = SlimeFightRules::DodgeDirection(ToTarget, false);
	TestEqual(TEXT("A unit long"), static_cast<float>(Right.Size()), 1.f, 0.001f);
	TestEqual(TEXT("Flat"), static_cast<float>(Right.Z), 0.f);
	TestEqual(TEXT("Across the line to the target"), static_cast<float>(FVector::DotProduct(Right, ToTarget.GetSafeNormal2D())), 0.f, 0.001f);
	TestTrue(TEXT("Its left is the other way"), (Right + Left).IsNearlyZero());
	TestTrue(TEXT("Facing +X, right is +Y"), Right.Y > 0.9);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlimeFightSplitTest, "Looter.Creatures.Slime.Fight.Split",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlimeFightSplitTest::RunTest(const FString& Parameters)
{
	// A Restless slime killed leaves two slimelets: Basic, 0.55 size, 0.3 health, half the damage, a quarter of the experience, the
	// slime's page (the class). The switch and the cap are the console variable's and the rules'.
	IConsoleVariable* SplitVar = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Slime.Split"));
	if (!TestNotNull(TEXT("Looter.Slime.Split exists"), SplitVar))
	{
		return false;
	}
	const int32 SplitBefore = SplitVar->GetInt();
	SplitVar->Set(1, ECVF_SetByCode);

	{
		FTestWorldWrapper WorldWrapper;
		if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
		{
			SplitVar->Set(SplitBefore, ECVF_SetByCode);
			return false;
		}
		UWorld* World = WorldWrapper.GetTestWorld();
		ASlimeCreature* Parent = SpawnSlime(World, FVector::ZeroVector, ECreatureRank::Rare);
		if (!TestNotNull(TEXT("Restless slime"), Parent))
		{
			SplitVar->Set(SplitBefore, ECVF_SetByCode);
			return false;
		}
		const int32 ParentLevel = Parent->Level;
		const FVector DiedAt = Parent->GetActorLocation();
		BossTestWorld::Kill(Parent);
		TestTrue(TEXT("It died"), Parent->IsDead());

		const TArray<ASlimeCreature*> Pieces = LivingSlimes(World);
		TestEqual(TEXT("Two slimelets"), Pieces.Num(), 2);
		const float Scale = GetDefault<UProgressionSettings>()->GetLevelRules().EnemyScale(FMath::Max(ParentLevel - UCreatureRankSettings::Get(ECreatureRank::Rare).LevelOffset, 1));
		const FCreatureRankInfo& Basic = UCreatureRankSettings::Get(ECreatureRank::Basic);
		const ASlimeCreature* Default = GetDefault<ASlimeCreature>();
		for (const ASlimeCreature* Piece : Pieces)
		{
			TestTrue(TEXT("A slimelet"), Piece->IsSlimelet());
			TestTrue(TEXT("Basic"), Piece->GetRank() == ECreatureRank::Basic);
			TestEqual(TEXT("Level: the parent's less its rank's offset"), Piece->Level,
				FMath::Max(ParentLevel - UCreatureRankSettings::Get(ECreatureRank::Rare).LevelOffset, 1));
			TestEqual(TEXT("0.55 size"), Piece->GetSizeScale(), 0.55f * Basic.Size, 0.01f);
			const UHealthComponent* Health = Piece->FindComponentByClass<UHealthComponent>();
			if (TestNotNull(TEXT("Health"), Health))
			{
				TestEqual(TEXT("0.3 of a slime's health"), Health->GetMaxHealth(), 120.f * 0.3f * Scale * Basic.HealthMultiplier, 0.5f);
			}
			TestEqual(TEXT("Half the damage"), Piece->AttackDamage, Default->AttackDamage * Scale * Basic.DamageMultiplier * 0.5f, 0.05f);
			TestEqual(TEXT("A quarter of the experience"), Piece->XPReward,
				FMath::Max(0, FMath::RoundToInt32(Default->XPReward * Basic.XPMultiplier * 0.25f)));
			TestEqual(TEXT("Named for what it is"), Piece->DisplayName.ToString(), FString(TEXT("Slimelet")));
			TestTrue(TEXT("Born where the slime fell"), FVector::Dist(Piece->GetActorLocation(), DiedAt) < 300.f);
			TestTrue(TEXT("Same pack as the slimes"), Piece->SharesPackWith(*Parent));
		}

		// A slimelet killed leaves nothing.
		if (Pieces.Num() == 2)
		{
			BossTestWorld::Kill(Pieces[0]);
			TestEqual(TEXT("A slimelet never splits"), LivingSlimes(World).Num(), 1);
		}
	}

	TestEqual(TEXT("A Basic slime leaves nothing"), AliveAfterKill(ECreatureRank::Basic, 0), 0);
	TestEqual(TEXT("A Restless slime leaves two"), AliveAfterKill(ECreatureRank::Rare, 0), 2);
	TestEqual(TEXT("8 other slimes: two fit under the cap of 10"), AliveAfterKill(ECreatureRank::Epic, 8), 10);
	TestEqual(TEXT("9 other slimes: a denied split just dies"), AliveAfterKill(ECreatureRank::Epic, 9), 9);

	SplitVar->Set(0, ECVF_SetByCode);
	TestEqual(TEXT("Switched off: a plain death"), AliveAfterKill(ECreatureRank::Rare, 0), 0);
	SplitVar->Set(SplitBefore, ECVF_SetByCode);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlimeFightGapTest, "Looter.Creatures.Slime.Fight.Gap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlimeFightGapTest::RunTest(const FString& Parameters)
{
	// The pack's drumbeat: a slime's leaps at one target begin 0.45 s apart (the director reads it per kind); slimelets too.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const ASlimeCreature* Slime = SpawnSlime(World, FVector::ZeroVector, ECreatureRank::Basic);
	if (!TestNotNull(TEXT("Slime"), Slime))
	{
		return false;
	}
	TestEqual(TEXT("Its melee gap is 0.45 s"), Slime->GetMeleeGap(), 0.45f, 0.001f);
	return true;
}

#endif
