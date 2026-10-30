#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother (Docs/Areas/RansomsRest.md, Side 3 and "Enemies by rank"): her body, rank and crits, her brood (Unearthed:
// mounds that bulge, then spiderlings; a mound shot while it bulges), the spiders' pack calls by tag, and her loot. Her
// Furrow, Grave Clod and headstone have their own files (GravemotherFurrowTests.cpp, GravemotherClodTests.cpp,
// GravemotherStoneTests.cpp); her return and Side 3 are GravemotherSideTests.cpp's.

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBodyHulls.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GravemotherAssets.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherMound.h"
#include "Creatures/GravemotherStone.h"
#include "Creatures/GravemotherTuning.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/LootDropComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootOdds.h"
#include "Loot/LootTable.h"
#include "Progression/ProgressionSettings.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/GravemotherTestWorld.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/WeaponDefinition.h"
#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "Misc/PackageName.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Tests/AutomationCommon.h"

using namespace GravemotherTestWorld;

namespace
{
	/** A point on her abdomen (world): its soft rear (Rear) or its front by the pedicel, from her physics asset as her code reads it. */
	bool AbdomenPoint(AGravemotherCreature& Mother, bool bRear, FVector& OutWorld)
	{
		const USkeletalMesh* Model = Mother.GetMesh()->GetSkeletalMeshAsset();
		const UPhysicsAsset* Physics = Mother.GetMesh()->GetPhysicsAsset();
		if (!Model || !Physics)
		{
			return false;
		}
		FCreatureBodyHulls Zones;
		const FName Bones[] = { AGravemotherCreature::AbdomenBone };
		Zones.AddFromPhysicsAsset(Physics, &Model->GetRefSkeleton(), Bones);
		const FCreatureBodyHulls::FShape* Round = Zones.Shapes.FindByPredicate([](const FCreatureBodyHulls::FShape& Shape) { return Shape.IsRound(); });
		const FCreatureBodyHulls::FShape* Hull = Zones.Shapes.FindByPredicate([](const FCreatureBodyHulls::FShape& Shape) { return !Shape.IsRound(); });
		if (!Hull)
		{
			return false;
		}
		const FVector Axis = Hull->BoundCenter.GetSafeNormal();
		double Farthest = 0.0;
		for (const FVector& Corner : Hull->Points)
		{
			Farthest = FMath::Max(Farthest, FVector::DotProduct(Corner, Axis));
		}
		const FVector InBone = bRear ? (Round ? Round->BoundCenter : Axis * (Farthest * 0.92)) : Axis * (Farthest * 0.15);
		OutWorld = Mother.GetMesh()->GetSocketTransform(AGravemotherCreature::AbdomenBone, RTS_World).TransformPosition(InBone);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherBodyTest, "Looter.Creatures.Gravemother.Body",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherBodyTest::RunTest(const FString& Parameters)
{
	// As the design has her: a brown spider at 1.8 times its size, Legendary with her name in orange on her tag, twelve times
	// a spider's health and the playtest's 1.6x (about 9,000 at level 8), one of the spiders' pack, never back on her own;
	// her headstone on her back.
	// Crits: her head always; her abdomen's soft rear while the stone stands, all of it once it's off; never a leg or her
	// thorax. Her own model once it's imported, else the spider's in the pale hide.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACreatureBase* Spider = EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), FVector(0.0, 4000.0, 0.0));
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Spider"), Spider) || !TestNotNull(TEXT("The Gravemother"), Mother))
	{
		return false;
	}
	const AGravemotherCreature* Defaults = GetDefault<AGravemotherCreature>();
	const FCreatureRankInfo& Legendary = UCreatureRankSettings::Get(ECreatureRank::Legendary);
	TestTrue(TEXT("Her kind is Legendary, and her name stands for its word"), Defaults->StartingRank == ECreatureRank::Legendary
		&& Defaults->bNameIsRankWord && Defaults->DisplayName.ToString() == TEXT("The Gravemother"));
	TestTrue(TEXT("Spawned by her lair: Legendary"), Mother->GetRank() == ECreatureRank::Legendary);
	TestTrue(TEXT("Her tag is the legendary orange (#FF9900, apart from Hallowed's yellow)"),
		Legendary.Color.Equals(LooterUI::Color::TierSoulfed(), 0.01f));
	TestNearlyEqual(TEXT("1.8 times a brown spider's size, her rank's own included"), Mother->GetSizeScale(), Gravemother::Size, 0.001f);
	TestNearlyEqual(TEXT("...her body with it"), Mother->GetCapsuleComponent()->GetScaledCapsuleRadius(),
		Spider->GetCapsuleComponent()->GetScaledCapsuleRadius() * Gravemother::Size, 0.1f);
	TestFalse(TEXT("She never comes back on her own (her lair says when)"), Mother->WillRespawn());

	// Twelve spiders' health at her level, then the playtest's 1.6x (2026-10-27): about 10,500 at level 8 since growth
	// compounds +9% a level (the user, 2026-10-29 night: x1.83 at level 8; about 9,000 on the old linear +8%).
	// Her own base: the spider's 300 before spiders were softened (2026-10-22); her fight was tuned on it.
	const float SpiderHealth = 300.f * GravemotherTuning::HealthScale();
	Mother->SetLevel(8);
	const float Health = Mother->FindComponentByClass<UHealthComponent>()->GetMaxHealth();
	const float Wanted = SpiderHealth * GetDefault<UProgressionSettings>()->GetLevelRules().EnemyScale(8) * Legendary.HealthMultiplier;
	TestNearlyEqual(*FString::Printf(TEXT("At level 8: %.0f health (a spider's 300 x %.2f, grown for level 8, times %.0f)"), Health,
		GravemotherTuning::HealthScale(), Legendary.HealthMultiplier), Health, Wanted, 0.5f);
	if (FMath::IsNearlyEqual(GravemotherTuning::HealthScale(), GravemotherTuning::DefaultHealthScale))
	{
		TestTrue(TEXT("...about 10,500"), Health > 10200.f && Health < 10900.f);
		TestNearlyEqual(TEXT("Her class carries the tuning (the Ledger's page reads it)"), Defaults->FindComponentByClass<UHealthComponent>()->MaxHealth,
			300.f * GravemotherTuning::DefaultHealthScale, 0.01f);
	}

	// Her headstone is on her back, a piece of its own on her abdomen bone.
	AGravemotherStone* Stone = Mother->GetStone();
	if (!TestNotNull(TEXT("Her headstone"), Stone))
	{
		return false;
	}
	TestTrue(TEXT("...on, attached to her abdomen"), Mother->IsStoneOn() && Stone->GetAttachParentActor() == Mother
		&& Stone->GetAttachParentSocketName() == AGravemotherCreature::AbdomenBone);

	// Crits: the head; a leg, the thorax and a plain spider's abdomen never.
	TestTrue(TEXT("The head is critical"), Mother->IsCriticalSpot(HitOn(*Mother, TEXT("head"))));
	TestFalse(TEXT("A leg isn't"), Mother->IsCriticalSpot(HitOn(*Mother, TEXT("femur_0_l"))));
	TestFalse(TEXT("The thorax isn't"), Mother->IsCriticalSpot(HitOn(*Mother, TEXT("body"))));
	TestFalse(TEXT("A spider's abdomen isn't"), Spider->IsCriticalSpot(HitOn(*Spider, TEXT("abdomen"))));
	FVector Rear;
	FVector Front;
	if (AbdomenPoint(*Mother, true, Rear) && AbdomenPoint(*Mother, false, Front))
	{
		TestTrue(TEXT("With the stone on, her abdomen's soft rear is critical"), Mother->IsCriticalSpot(HitOn(*Mother, AGravemotherCreature::AbdomenBone, Rear)));
		TestFalse(TEXT("...its front, by the pedicel, isn't"), Mother->IsCriticalSpot(HitOn(*Mother, AGravemotherCreature::AbdomenBone, Front)));
		Stone->Break(false);
		TestFalse(TEXT("Broken, the stone is off"), Mother->IsStoneOn());
		TestTrue(TEXT("...and all of her abdomen is critical: its front"), Mother->IsCriticalSpot(HitOn(*Mother, AGravemotherCreature::AbdomenBone, Front)));
		TestTrue(TEXT("...and its rear"), Mother->IsCriticalSpot(HitOn(*Mother, AGravemotherCreature::AbdomenBone, Rear)));
	}
	else
	{
		AddWarning(TEXT("Her model has no physics asset here: her abdomen's crits weren't checked."));
	}
	if (const UPhysicsAsset* Hulls = Mother->GetMesh()->GetPhysicsAsset())
	{
		TestTrue(TEXT("The head and the abdomen have hit hulls"),
			Hulls->FindBodyIndex(TEXT("head")) != INDEX_NONE && Hulls->FindBodyIndex(AGravemotherCreature::AbdomenBone) != INDEX_NONE);
	}

	// One of the spiders' pack, and her rank's call reaches every spider within 30 m.
	TestTrue(TEXT("A spider by her pack tag"), Mother->PackTag == Spider->PackTag && Mother->SharesPackWith(*Spider)
		&& Spider->SharesPackWith(*Mother));
	TestTrue(TEXT("Her call reaches 30 m"), Mother->GetPackCallRadius() >= 3000.f);

	// Her look: her own model once it's imported (Spider.py's rig: the spider's bones), else the spider's in the pale hide.
	const USkeletalMesh* Model = Mother->GetMesh()->GetSkeletalMeshAsset();
	if (GravemotherAssets::IsImported(GravemotherAssets::Body))
	{
		TestEqual(TEXT("She wears her own body"), GetPathNameSafe(Model), FString(GravemotherAssets::Body));
	}
	else
	{
		TestTrue(TEXT("Until her body is imported, the spider's"), Model == Spider->GetMesh()->GetSkeletalMeshAsset());
		const TCHAR* PaleHide = TEXT("/Game/Art/Materials/MI_SpiderBody_Pale.MI_SpiderBody_Pale");
		if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(PaleHide))))
		{
			TestEqual(TEXT("...in the pale hide"), GetPathNameSafe(Mother->GetMesh()->GetMaterial(0)), FString(PaleHide));
		}
		AddInfo(TEXT("SK_Gravemother isn't imported yet: she's SK_Spider."));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherMoundsTest, "Looter.Creatures.Gravemother.Unearthed.Mounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherMoundsTest::RunTest(const FString& Parameters)
{
	// Unearthed: at 66% and at 33% of her health, each once a life (a hit past both lines makes both), her next attack is
	// the roar whose slam raises four mounds round her; they bulge for 1 s, then spiderlings dig out: a brown spider at 0.45x
	// with a fifth of its health (60 at level 1), Basic and on the Basic table, named for what it is, round her on her level.
	// A killing blow calls none.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(1000.0, 0.0, 0.0));
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector, 1);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The Gravemother"), Mother))
	{
		return false;
	}
	TestEqual(TEXT("Level 1 of her own, and her rank's two on top"), Mother->Level, 1 + UCreatureRankSettings::Get(ECreatureRank::Legendary).LevelOffset);
	HurtTo(Mother, 0.7f);
	TestTrue(TEXT("At 70%: no call yet"), Mother->GetBroodCallsMade() == 0 && Mother->GetMounds().Num() == 0);
	HurtTo(Mother, 0.65f);
	TestTrue(TEXT("At 65%: her first call, waiting on her roar"), Mother->GetBroodCallsMade() == 1 && Mother->GetPendingUnearths() == 1
		&& Mother->IsRoarWanted() && Mother->GetMounds().Num() == 0);

	// Her roar (she rears and screams), and its slam raises the mounds.
	Mother->AlertTo(Player);
	const float RoarAt = TickUntil(Mother, 1.f, [Mother]() { return Mother->GetMove() == EGravemotherMove::Roar; });
	if (!TestTrue(TEXT("She roars"), RoarAt >= 0.f))
	{
		return false;
	}
	TickFor(Mother, Mother->RoarWindup + Step);
	const TArray<AGravemotherMound*> Mounds = Mother->GetMounds();
	TestTrue(*FString::Printf(TEXT("Her slam raises four mounds (%d)"), Mounds.Num()), Mounds.Num() == Gravemother::BroodSize
		&& Mother->GetPendingUnearths() == 0);
	TestEqual(TEXT("...and no spiderling yet"), Mother->GetBrood().Num(), 0);
	for (const AGravemotherMound* Mound : Mounds)
	{
		TestTrue(TEXT("...each bulging"), Mound->IsBulging() && Mound->GetBulge() < 1.f);
	}
	AdvanceMounds(*Mother, Mother->Unearth.BulgeSeconds * 0.5f);
	TestEqual(TEXT("Half way: still no spiderling"), Mother->GetBrood().Num(), 0);
	AdvanceMounds(*Mother, Mother->Unearth.BulgeSeconds * 0.5f + Step * 2.f);
	const TArray<ASpiderCreature*> First = Mother->GetBrood();
	TestEqual(TEXT("A second on: four spiderlings dug out"), First.Num(), Gravemother::BroodSize);

	const float Scale = Mother->GetSizeScale();
	const FVector Feet = FeetOf(*Mother);
	TArray<ACreatureBase*> Brood;
	for (ASpiderCreature* Spiderling : First)
	{
		Brood.Add(Spiderling);
		const UHealthComponent* Health = Spiderling->FindComponentByClass<UHealthComponent>();
		const ULootDropComponent* Loot = Spiderling->FindComponentByClass<ULootDropComponent>();
		TestTrue(TEXT("A brown spider (the spider's Ledger page), Basic"), Spiderling->GetClass() == ASpiderCreature::StaticClass()
			&& Spiderling->GetRank() == ECreatureRank::Basic);
		TestNearlyEqual(TEXT("0.45x"), Spiderling->GetSizeScale(), Gravemother::SpiderlingSize, 0.001f);
		TestTrue(*FString::Printf(TEXT("60 health at level 1 (%.0f at %d)"), Health ? Health->GetMaxHealth() : 0.f, Spiderling->Level),
			Health && Spiderling->Level == 1 && FMath::IsNearlyEqual(Health->GetMaxHealth(), 60.f, 0.01f));
		TestTrue(TEXT("Named for what it is"), Spiderling->DisplayName.EqualTo(AGravemotherCreature::SpiderlingName()));
		TestTrue(TEXT("The Basic table: it can drop a legendary"), Loot && Loot->LootTable == ULootLibrary::GetDefaultLootTable());
		TestFalse(TEXT("Gone for good once killed"), Spiderling->WillRespawn());
		const FVector Spot = FeetOf(*Spiderling);
		TestTrue(TEXT("Dug out round her"), FVector::Dist2D(Spot, Feet) <= Mother->Unearth.Radius * Scale + 1.0
			&& FVector::Dist2D(Spot, Feet) >= Mother->Unearth.Spacing - 1.0);
		TestTrue(TEXT("...on her level"), FMath::Abs(Spot.Z - Feet.Z) <= Mother->Unearth.MaxStep);
	}
	TestTrue(TEXT("...apart from each other"), EncounterTestWorld::AllApart(EncounterTestWorld::PlacesOf(Brood), Mother->Unearth.Spacing - 1.0));

	HurtTo(Mother, 0.5f);
	TestEqual(TEXT("At 50%: no more"), Mother->GetBroodCallsMade(), 1);
	HurtTo(Mother, 0.32f);
	TestTrue(TEXT("At 32%: her second call"), Mother->GetBroodCallsMade() == 2 && Mother->GetPendingUnearths() == 1);
	HurtTo(Mother, 0.1f);
	TestEqual(TEXT("At 10%: no third"), Mother->GetBroodCallsMade(), 2);

	// One hit past both lines calls both; with nobody to roar at, they come up anyway a moment later. A killing blow, none.
	AGravemotherCreature* Struck = SpawnMother(World, FVector(0.0, 8000.0, 0.0), 1);
	AGravemotherCreature* Felled = SpawnMother(World, FVector(0.0, -8000.0, 0.0), 1);
	if (TestNotNull(TEXT("Second Gravemother"), Struck) && TestNotNull(TEXT("Third Gravemother"), Felled))
	{
		HurtTo(Struck, 0.2f);
		TestTrue(TEXT("From full to 20% in one hit: both calls"), Struck->GetBroodCallsMade() == 2 && Struck->GetPendingUnearths() == 2);
		TickFor(Struck, Struck->Unearth.LateRaiseSeconds + Step * 2.f);
		TestEqual(TEXT("...her slam not coming, both calls' mounds come up anyway"), Struck->GetMounds().Num(), 2 * Gravemother::BroodSize);
		AdvanceMounds(*Struck, Struck->Unearth.BulgeSeconds + Step * 2.f);
		TestEqual(TEXT("...and eight spiderlings"), Struck->GetBrood().Num(), 2 * Gravemother::BroodSize);
		BossTestWorld::Kill(Felled);
		TestTrue(TEXT("Killed outright: none"), Felled->IsDead() && Felled->GetBroodCallsMade() == 0 && Felled->GetMounds().Num() == 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherMoundShotTest, "Looter.Creatures.Gravemother.Unearthed.Shot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherMoundShotTest::RunTest(const FString& Parameters)
{
	// A mound shot while it bulges caves in on its spiderling: one less digs out. Burst, it can't be shot any more. Her
	// death caves the rest in.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector, 1);
	if (!TestNotNull(TEXT("The Gravemother"), Mother) || !TestEqual(TEXT("She raises four mounds"), Mother->CallBrood(), Gravemother::BroodSize))
	{
		return false;
	}
	const TArray<AGravemotherMound*> Mounds = Mother->GetMounds();
	AGravemotherMound* Shot = Mounds[0];
	TestTrue(TEXT("A bulging mound can be shot (its box on the Weapon channel)"), Shot->GetHitBox()->GetCollisionEnabled() != ECollisionEnabled::NoCollision
		&& Shot->GetHitBox()->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block);
	AdvanceMounds(*Mother, Mother->Unearth.BulgeSeconds * 0.5f);
	BossTestWorld::Hurt(Shot, 10.f);
	TestTrue(TEXT("Shot while it bulges: it caves in"), Shot->WasPopped() && !Shot->IsBulging());
	AdvanceMounds(*Mother, Mother->Unearth.BulgeSeconds * 0.5f + Step * 2.f);
	TestEqual(TEXT("Three spiderlings dig out, not four"), Mother->GetBrood().Num(), Gravemother::BroodSize - 1);
	TestNull(TEXT("...none from the one shot"), Shot->GetSpiderling());
	AGravemotherMound* Burst = Mounds[1];
	TestTrue(TEXT("A burst mound can't be shot"), Burst->HasBurst() && Burst->GetHitBox()->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& Burst->GetSpiderling() != nullptr);

	// Her death caves in what's still bulging.
	AGravemotherCreature* Other = SpawnMother(World, FVector(0.0, 8000.0, 0.0), 1);
	if (TestNotNull(TEXT("Second Gravemother"), Other) && TestTrue(TEXT("Her mounds"), Other->CallBrood() > 0))
	{
		const TArray<AGravemotherMound*> Hers = Other->GetMounds();
		BossTestWorld::Kill(Other);
		bool bAllCaved = true;
		for (const AGravemotherMound* Mound : Hers)
		{
			bAllCaved &= Mound->IsCollapsed();
		}
		TestTrue(TEXT("Killed, her bulging mounds cave in"), bAllCaved);
		AdvanceMounds(*Other, Other->Unearth.BulgeSeconds + Step * 2.f);
		TestEqual(TEXT("...and no spiderling digs out"), Other->GetBrood().Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherPackCallsTest, "Looter.Creatures.Gravemother.PackCalls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherPackCallsTest::RunTest(const FString& Parameters)
{
	// Pack calls go by pack tag: hurt, the Gravemother calls every spider within 30 m onto her attacker, whatever its class
	// (a spider, a spiderling), and nothing else (a slime) nor anything farther; a Gravebound spider's call reaches her too.
	// The slime's own call is as it was: slimes within 25 m, no spiders. A Basic spider calls nobody.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(0.0, -1000.0, 0.0));
	AAIController* Shooter = World->SpawnActor<AAIController>();
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Its controller"), Shooter))
	{
		return false;
	}
	// The hurt creature turns on its attacker: the pawn of whoever dealt the damage.
	Shooter->SetPawn(Player);
	auto Spawn = [World](TSubclassOf<ACreatureBase> Class, double X, double Y, ECreatureRank Rank = ECreatureRank::Basic)
	{
		ACreatureBase::FRuntimeSpawn Setup;
		Setup.Rank = Rank;
		ACreatureBase* Creature = ACreatureBase::SpawnAtRuntime(World, Class, FVector(X, Y, 0.0), 0.f, Setup);
		if (Creature && !Creature->HasActorBegunPlay())
		{
			Creature->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Creature);
		return Creature;
	};
	auto Hunting = [](const ACreatureBase* Creature)
	{
		return Creature && Creature->GetCreatureState() == ECreatureState::Chase;
	};

	// Her call, round the origin.
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector);
	ACreatureBase* Spider = Spawn(ASpiderCreature::StaticClass(), 1500.0, 0.0);
	ACreatureBase* Spiderling = AGravemotherCreature::SpawnSpiderling(World, FVector(0.0, 2000.0, 0.0), 0.f);
	ACreatureBase* Slime = Spawn(ASlimeCreature::StaticClass(), -1000.0, 0.0);
	ACreatureBase* FarSpider = Spawn(ASpiderCreature::StaticClass(), 4000.0, 0.0);
	// A Gravebound spider's call, 20 km off.
	AGravemotherCreature* Answering = SpawnMother(World, FVector(0.0, 20000.0, 0.0));
	ACreatureBase* Gravebound = Spawn(ASpiderCreature::StaticClass(), 0.0, 21500.0, ECreatureRank::Epic);
	// The slimes' call, 20 km the other way.
	ACreatureBase* SlimeA = Spawn(ASlimeCreature::StaticClass(), 0.0, -20000.0);
	ACreatureBase* SlimeB = Spawn(ASlimeCreature::StaticClass(), 1500.0, -20000.0);
	ACreatureBase* FarSlime = Spawn(ASlimeCreature::StaticClass(), 3000.0, -20000.0);
	ACreatureBase* SlimesSpider = Spawn(ASpiderCreature::StaticClass(), -1000.0, -20000.0);
	// A Basic spider's call, 40 km off.
	ACreatureBase* Lone = Spawn(ASpiderCreature::StaticClass(), 0.0, 40000.0);
	ACreatureBase* Neighbour = Spawn(ASpiderCreature::StaticClass(), 500.0, 40000.0);
	const TArray<ACreatureBase*> All = { Mother, Spider, Spiderling, Slime, FarSpider, Answering, Gravebound, SlimeA, SlimeB, FarSlime,
		SlimesSpider, Lone, Neighbour };
	if (!TestFalse(TEXT("Everything spawned"), All.Contains(nullptr)))
	{
		return false;
	}
	BossTestWorld::NoLoot(Spiderling);

	BossTestWorld::Hurt(Mother, 10.f, Shooter);
	TestTrue(TEXT("Hurt, she turns on her attacker"), Hunting(Mother));
	TestTrue(TEXT("A spider 15 m off answers her"), Hunting(Spider));
	TestTrue(TEXT("So does a spiderling 20 m off (another configuration, the same pack)"), Hunting(Spiderling));
	TestFalse(TEXT("A slime 10 m off doesn't"), Hunting(Slime));
	TestFalse(TEXT("A spider 40 m off doesn't"), Hunting(FarSpider));

	BossTestWorld::Hurt(Gravebound, 10.f, Shooter);
	TestTrue(TEXT("A Gravebound spider's call reaches the Gravemother 15 m off"), Hunting(Answering));

	BossTestWorld::Hurt(SlimeA, 10.f, Shooter);
	TestTrue(TEXT("The slimes as before: a slime 15 m off answers"), Hunting(SlimeB));
	TestFalse(TEXT("...one 30 m off doesn't"), Hunting(FarSlime));
	TestFalse(TEXT("...nor does a spider"), Hunting(SlimesSpider));

	BossTestWorld::Hurt(Lone, 10.f, Shooter);
	TestTrue(TEXT("A Basic spider turns on its attacker"), Hunting(Lone));
	TestFalse(TEXT("...and calls nobody"), Hunting(Neighbour));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherLootTest, "Looter.Creatures.Gravemother.Loot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherLootTest::RunTest(const FString& Parameters)
{
	// Her loot is the Legendary table (its asset, or until Tools/Unreal/create_rank_assets.py makes it, the stand-in with
	// its odds) on every kill: guns every time, two of them, at Luck 1.3 (a legendary on about 21% of kills), at her level.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector, 6);
	const ULootDropComponent* Loot = Mother ? Mother->FindComponentByClass<ULootDropComponent>() : nullptr;
	if (!TestNotNull(TEXT("The Gravemother and her loot"), Loot))
	{
		return false;
	}
	const ULootTable* Table = Loot->LootTable;
	TestTrue(TEXT("The Legendary table"), Table && Table == UCreatureRankSettings::GetLootTable(ECreatureRank::Legendary));
	if (!Table)
	{
		return false;
	}
	TestTrue(TEXT("Guns on every kill, two of them"), Table->WeaponDropChance >= 1.f && Table->MinWeaponDrops == 2
		&& Table->MaxWeaponDrops == 2);
	TestNearlyEqual(TEXT("At Luck 1.3"), Table->Luck, 1.3f, 0.001f);
	TestTrue(*FString::Printf(TEXT("Her guns are her level (%d)"), Mother->Level), Loot->Level == Mother->Level && Mother->Level == 8);
	const double Legendary = LootOdds::Expected(Table).LegendaryPerKill;
	TestTrue(*FString::Printf(TEXT("A legendary on about 21%% of kills (%.1f%%)"), Legendary * 100.0),
		FMath::Abs(Legendary - 0.214) <= 0.0214);
	if (Table->Entries.IsEmpty())
	{
		AddWarning(TEXT("The Legendary table has no guns to roll (the default table's weapons aren't there)."));
		return true;
	}
	FRandomStream Random(20261007);
	int32 TwoGunKills = 0;
	constexpr int32 Kills = 200;
	for (int32 Kill = 0; Kill < Kills; ++Kill)
	{
		ULootLibrary::RollAmmo(Table, Random);
		TwoGunKills += ULootLibrary::RollWeaponPicks(Table, Loot->ExtraLuck, Random).Num() == 2 ? 1 : 0;
	}
	TestEqual(TEXT("Every one of 200 kills drops two guns"), TwoGunKills, Kills);
	return true;
}

#endif
