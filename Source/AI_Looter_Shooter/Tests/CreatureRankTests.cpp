#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/LootDropComponent.h"
#include "Loot/LootTable.h"
#include "Progression/ProgressionSettings.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"

namespace
{
	/** A creature in a test world, started as play starts it (the test world never begins play itself). */
	template <typename TCreature>
	TCreature* SpawnStarted(UWorld* World, const FVector& Location, float Scale, ECreatureRank Rank = ECreatureRank::Basic)
	{
		TCreature* Creature = World->SpawnActor<TCreature>(Location, FRotator::ZeroRotator);
		if (Creature)
		{
			Creature->BodyScale = Scale;
			Creature->StartingRank = Rank;
			Creature->DispatchBeginPlay();
		}
		return Creature;
	}

	/** Everything Sized measures in centimetres must be Reference's (a full-size creature of its kind) times Size. */
	void TestScaled(FAutomationTestBase& Test, const ACreatureBase& Reference, const ACreatureBase& Sized, float Size)
	{
		const FString Kind = FString::Printf(TEXT("%s at %.2fx"), *Sized.GetClass()->GetName(), Size);
		auto Scaled = [&Test, &Kind, Size](const TCHAR* What, double Got, double FullSize)
		{
			const double Wanted = FullSize * Size;
			Test.TestNearlyEqual(FString::Printf(TEXT("%s: %s (%.2f; full size %.2f)"), *Kind, What, Got, FullSize), Got, Wanted,
				FMath::Max(0.01, FMath::Abs(Wanted) * 0.001));
		};
		Test.TestNearlyEqual(FString::Printf(TEXT("%s: its size"), *Kind), static_cast<double>(Sized.GetSizeScale()), static_cast<double>(Size), 0.0001);

		// The body: capsule, model (and so its hit zones) standing on the capsule's foot, the step it walks up, and the
		// health bar over it.
		Scaled(TEXT("capsule radius"), Sized.GetCapsuleComponent()->GetScaledCapsuleRadius(), Reference.GetCapsuleComponent()->GetScaledCapsuleRadius());
		Scaled(TEXT("capsule half height"), Sized.GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
			Reference.GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		Scaled(TEXT("model's scale"), Sized.GetMesh()->GetComponentScale().X, Reference.GetMesh()->GetComponentScale().X);
		Scaled(TEXT("model's foot under the middle"), Sized.GetActorLocation().Z - Sized.GetMesh()->GetComponentLocation().Z,
			Reference.GetActorLocation().Z - Reference.GetMesh()->GetComponentLocation().Z);
		Scaled(TEXT("step height"), Sized.GetCharacterMovement()->MaxStepHeight, Reference.GetCharacterMovement()->MaxStepHeight);
		const UWidgetComponent* Bar = Sized.FindComponentByClass<UWidgetComponent>();
		const UWidgetComponent* FullBar = Reference.FindComponentByClass<UWidgetComponent>();
		if (Test.TestTrue(FString::Printf(TEXT("%s: has a health bar"), *Kind), Bar && FullBar))
		{
			Scaled(TEXT("health bar's height"), Bar->GetComponentLocation().Z - Sized.GetActorLocation().Z,
				FullBar->GetComponentLocation().Z - Reference.GetActorLocation().Z);
		}

		// Its reach, and what its steering looks ahead with.
		Scaled(TEXT("attack range"), Sized.GetAttackRange(), Reference.GetAttackRange());
		Scaled(TEXT("strike reach"), Sized.GetStrikeReach(), Reference.GetStrikeReach());
		const ACreatureBase::FSteerProbes Probes = Sized.GetSteerProbes();
		const ACreatureBase::FSteerProbes FullProbes = Reference.GetSteerProbes();
		Scaled(TEXT("obstacle probe's length"), Probes.SweepLength, FullProbes.SweepLength);
		Scaled(TEXT("obstacle probe's width"), Probes.SweepRadius, FullProbes.SweepRadius);
		Scaled(TEXT("ledge probe's distance"), Probes.LedgeDistance, FullProbes.LedgeDistance);
		Scaled(TEXT("deepest drop it walks down"), Probes.LedgeDrop, FullProbes.LedgeDrop);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureSizeTest, "Looter.Creatures.Size",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureSizeTest::RunTest(const FString& Parameters)
{
	// A creature's size (its BodyScale, times its rank's) scales everything it measures in centimetres, from its capsule
	// to its steering probes and its gait: a 0.45x spiderling and a 1.8x giant against full-size ones, spider and slime.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const ASpiderCreature* Spider = SpawnStarted<ASpiderCreature>(World, FVector::ZeroVector, 1.f);
	const ASlimeCreature* Slime = SpawnStarted<ASlimeCreature>(World, FVector(0.0, 5000.0, 0.0), 1.f);
	if (!TestNotNull(TEXT("Spider spawned"), Spider) || !TestNotNull(TEXT("Slime spawned"), Slime))
	{
		return false;
	}
	TestEqual(TEXT("A spider is full size unless told otherwise"), Spider->GetSizeScale(), 1.f);

	double Column = 0.0;
	for (const float Size : { 0.45f, 1.8f })
	{
		Column += 3000.0;
		const ASpiderCreature* SizedSpider = SpawnStarted<ASpiderCreature>(World, FVector(Column, 0.0, 0.0), Size);
		const ASlimeCreature* SizedSlime = SpawnStarted<ASlimeCreature>(World, FVector(Column, 5000.0, 0.0), Size);
		if (!TestNotNull(TEXT("Sized spider spawned"), SizedSpider) || !TestNotNull(TEXT("Sized slime spawned"), SizedSlime))
		{
			return false;
		}
		TestScaled(*this, *Spider, *SizedSpider, Size);
		TestScaled(*this, *Slime, *SizedSlime, Size);

		// The spider's body and legs take the full-size spider's pose against its model: its leg reach, ride height and
		// feet's spots grew or shrank with it, so the legs reach the ground where its size puts them.
		const TArray<FCreatureBonePose>& Pose = SizedSpider->GetBonePose();
		const TArray<FCreatureBonePose>& FullPose = Spider->GetBonePose();
		if (TestTrue(FString::Printf(TEXT("Spider at %.2fx is posed like a full-size one (%d bones)"), Size, Pose.Num()),
			Pose.Num() > 0 && Pose.Num() == FullPose.Num()))
		{
			double Farthest = 0.0;
			double MostTurned = 0.0;
			double MostScaled = 0.0;
			for (int32 Index = 0; Index < Pose.Num(); ++Index)
			{
				const FTransform& Bone = Pose[Index].Transform;
				const FTransform& FullBone = FullPose[Index].Transform;
				Farthest = FMath::Max(Farthest, FVector::Dist(Bone.GetLocation(), FullBone.GetLocation()));
				MostTurned = FMath::Max(MostTurned, FMath::RadiansToDegrees(Bone.GetRotation().AngularDistance(FullBone.GetRotation())));
				MostScaled = FMath::Max(MostScaled, FVector::Dist(Bone.GetScale3D(), FullBone.GetScale3D()));
			}
			TestTrue(FString::Printf(TEXT("Spider at %.2fx: every bone where the full-size spider's is, against the model (off by at most %.3f cm, ")
				TEXT("%.3f degrees and %.4f in scale)"), Size, Farthest, MostTurned, MostScaled), Farthest < 0.5 && MostTurned < 0.5 && MostScaled < 0.001);
		}
	}

	// A rank's size comes on top of the creature's own.
	const ASpiderCreature* Gravebound = SpawnStarted<ASpiderCreature>(World, FVector(Column + 3000.0, 0.0, 0.0), 0.45f, ECreatureRank::Epic);
	if (TestNotNull(TEXT("Gravebound spiderling spawned"), Gravebound))
	{
		TestNearlyEqual(TEXT("A Gravebound spiderling is its rank's size times its own"), Gravebound->GetSizeScale(),
			0.45f * UCreatureRankSettings::Get(ECreatureRank::Epic).Size, 0.0001f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureRanksTest, "Looter.Creatures.Ranks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureRanksTest::RunTest(const FString& Parameters)
{
	// The ranks as decided (Docs/Story.md, "Enemy ranks and legendary drops"). Basic is creatures as they always were;
	// Restless, Gravebound and Soulfed show their word in the rarity color of the odds they carry (the loot beams' colors),
	// each a little bigger and tougher than the one below.
	const FCreatureRankInfo& Basic = UCreatureRankSettings::Get(ECreatureRank::Basic);
	TestTrue(TEXT("Basic shows no word"), Basic.Word.IsEmpty());
	TestTrue(TEXT("Basic is as its class made it"), Basic.Size == 1.f && Basic.HealthMultiplier == 1.f && Basic.DamageMultiplier == 1.f
		&& Basic.XPMultiplier == 1.f && Basic.LevelOffset == 0 && Basic.PackCallRadius == 0.f && Basic.bRespawns);
	struct FSoulLight
	{
		ECreatureRank Rank;
		const TCHAR* Word;
		EWeaponRarity Rarity;
	};
	const FSoulLight Lights[] = { { ECreatureRank::Rare, TEXT("Restless"), EWeaponRarity::Rare },
		{ ECreatureRank::Epic, TEXT("Gravebound"), EWeaponRarity::Epic }, { ECreatureRank::Legendary, TEXT("Soulfed"), EWeaponRarity::Legendary } };
	const UWeaponDefinition* Palette = GetDefault<UWeaponDefinition>();
	float LastSize = Basic.Size;
	for (const FSoulLight& Light : Lights)
	{
		const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Light.Rank);
		TestEqual(FString::Printf(TEXT("%s's word"), Light.Word), Info.Word.ToString(), FString(Light.Word));
		const FLinearColor Beam = Palette->GetRarityInfo(Light.Rarity).Color;
		TestTrue(FString::Printf(TEXT("%s shows in the %s loot beam's color (%s against %s)"), Light.Word,
			*UEnum::GetDisplayValueAsText(Light.Rarity).ToString(), *Info.Color.ToString(), *Beam.ToString()), Info.Color.Equals(Beam, 0.01f));
		TestTrue(FString::Printf(TEXT("%s is bigger than the rank below (%.2fx)"), Light.Word, Info.Size), Info.Size > LastSize);
		TestTrue(FString::Printf(TEXT("%s is tougher than Basic"), Light.Word), Info.HealthMultiplier > 1.f && Info.DamageMultiplier > 1.f);
		LastSize = Info.Size;
		ECreatureRank ByWord = ECreatureRank::Basic;
		TestTrue(FString::Printf(TEXT("%s names its rank, any case"), Light.Word),
			UCreatureRankSettings::ParseRank(FString(Light.Word).ToLower(), ByWord) && ByWord == Light.Rank);
	}
	TestTrue(TEXT("A boss's name shows in orange"),
		UCreatureRankSettings::Get(ECreatureRank::Boss).Color.Equals(Palette->GetRarityInfo(EWeaponRarity::Legendary).Color, 0.01f));
	TestFalse(TEXT("A Legendary monster doesn't come back on its own"), UCreatureRankSettings::Get(ECreatureRank::Legendary).bRespawns);
	TestFalse(TEXT("Nor does a boss"), UCreatureRankSettings::Get(ECreatureRank::Boss).bRespawns);
	ECreatureRank ByName = ECreatureRank::Basic;
	TestTrue(TEXT("Ranks by name, any case"), UCreatureRankSettings::ParseRank(TEXT("boss"), ByName) && ByName == ECreatureRank::Boss);
	TestFalse(TEXT("No such rank"), UCreatureRankSettings::ParseRank(TEXT("Mythic"), ByName));

	// Who comes back: placed creatures, as they started; never a Legendary monster, a boss, or anything spawned in play.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Placed = World->SpawnActor<ASpiderCreature>(FVector::ZeroVector, FRotator::ZeroRotator);
	ASlimeCreature* Slime = World->SpawnActor<ASlimeCreature>(FVector(0.0, 3000.0, 0.0), FRotator::ZeroRotator);
	ACreatureBase* Spawned = ACreatureBase::SpawnAtRuntime(World, ASpiderCreature::StaticClass(), FVector(3000.0, 0.0, 0.0), 0.f,
		ACreatureBase::FRuntimeSpawn());
	if (!TestNotNull(TEXT("Placed spider"), Placed) || !TestNotNull(TEXT("Slime"), Slime) || !TestNotNull(TEXT("Spider spawned in play"), Spawned))
	{
		return false;
	}
	TestTrue(TEXT("A placed spider comes back"), Placed->WillRespawn());
	TestFalse(TEXT("A spider spawned in play doesn't"), Spawned->WillRespawn());
	Placed->StartingRank = ECreatureRank::Rare;
	TestTrue(TEXT("A placed Restless one does (as Restless)"), Placed->WillRespawn());
	Placed->StartingRank = ECreatureRank::Legendary;
	TestFalse(TEXT("A placed Legendary monster doesn't"), Placed->WillRespawn());
	Placed->StartingRank = ECreatureRank::Boss;
	TestFalse(TEXT("A placed boss doesn't"), Placed->WillRespawn());
	Placed->StartingRank = ECreatureRank::Basic;

	// Pack calls reach every creature with the same pack tag, whatever its class, and never another kind; a creature with
	// no pack tag calls only its own class.
	TestTrue(TEXT("A spider answers a spider's call"), Spawned->SharesPackWith(*Placed));
	TestFalse(TEXT("A slime doesn't answer a spider's call"), Slime->SharesPackWith(*Placed));
	Spawned->PackTag = NAME_None;
	TestTrue(TEXT("Without a pack tag, its own class answers"), Spawned->SharesPackWith(*Placed));
	TestFalse(TEXT("Without a pack tag, no other class does"), Spawned->SharesPackWith(*Slime));

	// A promotion lasts one life: bigger, tougher, worth more, its rank's level, loot and call; then back as it started.
	Placed->DispatchBeginPlay();
	UHealthComponent* Health = Placed->FindComponentByClass<UHealthComponent>();
	const ULootDropComponent* Loot = Placed->FindComponentByClass<ULootDropComponent>();
	if (!TestNotNull(TEXT("Spider's health"), Health) || !TestNotNull(TEXT("Spider's loot"), Loot))
	{
		return false;
	}
	const float FullHealth = Health->MaxHealth;
	const float FullDamage = Placed->AttackDamage;
	const int32 FullXP = Placed->XPReward;
	const int32 FullLevel = Placed->Level;
	const ULootTable* OwnTable = Loot->LootTable.Get();
	const FCreatureRankInfo& Epic = UCreatureRankSettings::Get(ECreatureRank::Epic);
	// Its rank's levels are levels like any other: health and damage grow with them too, under the rank's multipliers.
	const FLevelRules LevelRules = GetDefault<UProgressionSettings>()->GetLevelRules();
	const float RankLevelGrowth = LevelRules.EnemyScale(FullLevel + Epic.LevelOffset) / LevelRules.EnemyScale(FullLevel);
	Placed->SetRank(ECreatureRank::Epic);
	TestTrue(TEXT("Promoted to Gravebound"), Placed->GetRank() == ECreatureRank::Epic);
	TestNearlyEqual(TEXT("Gravebound: its rank's size"), Placed->GetSizeScale(), Epic.Size, 0.0001f);
	TestNearlyEqual(TEXT("Gravebound: its rank's health, at its rank's level"), Health->MaxHealth,
		FullHealth * Epic.HealthMultiplier * RankLevelGrowth, 0.01f);
	TestNearlyEqual(TEXT("Gravebound: promoted unhurt, it's at full health"), Health->GetHealth(), Health->MaxHealth, 0.01f);
	TestNearlyEqual(TEXT("Gravebound: its rank's bite, at its rank's level"), Placed->AttackDamage,
		FullDamage * Epic.DamageMultiplier * RankLevelGrowth, 0.001f);
	TestEqual(TEXT("Gravebound: its rank's experience"), Placed->XPReward, FMath::RoundToInt32(FullXP * Epic.XPMultiplier));
	TestEqual(TEXT("Gravebound: its rank's level"), Placed->Level, FullLevel + Epic.LevelOffset);
	TestTrue(TEXT("Gravebound: its rank's loot"), Loot->LootTable.Get() == UCreatureRankSettings::GetLootTable(ECreatureRank::Epic));
	TestTrue(TEXT("Gravebound: its call reaches every spider within 30 m"), Placed->GetPackCallRadius() >= 3000.f);
	Placed->SetRank(Placed->StartingRank);
	TestTrue(TEXT("Back as it started"), Placed->GetRank() == ECreatureRank::Basic);
	TestNearlyEqual(TEXT("Back: full size"), Placed->GetSizeScale(), 1.f, 0.0001f);
	TestNearlyEqual(TEXT("Back: its own health"), Health->MaxHealth, FullHealth, 0.01f);
	TestNearlyEqual(TEXT("Back: its own bite"), Placed->AttackDamage, FullDamage, 0.001f);
	TestEqual(TEXT("Back: its own experience"), Placed->XPReward, FullXP);
	TestEqual(TEXT("Back: its own level"), Placed->Level, FullLevel);
	TestTrue(TEXT("Back: its own loot table"), Loot->LootTable.Get() == OwnTable);
	TestNearlyEqual(TEXT("Back: its own call"), Placed->GetPackCallRadius(), Placed->PackAlertRadius, 0.001f);
	return true;
}

#endif
