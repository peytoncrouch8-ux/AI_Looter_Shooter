#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother (Docs/Areas/RansomsRest.md, Side 3 and "Enemies by rank"): her body and rank, her charge, her brood, the
// spiders' pack calls by tag, and her loot. Her return and Side 3 are in GravemotherSideTests.cpp.

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GroundCrack.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/LootDropComponent.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootOdds.h"
#include "Loot/LootTable.h"
#include "Progression/ProgressionSettings.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "Weapons/WeaponDefinition.h"
#include "AIController.h"
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

namespace
{
	/** The Gravemother as her lair spawns her (Legendary), at OwnLevel before her rank's, begun, dropping no loot. */
	AGravemotherCreature* SpawnMother(UWorld* World, const FVector& Feet, int32 OwnLevel = 1)
	{
		ACreatureBase::FRuntimeSpawn Spawn;
		Spawn.Rank = ECreatureRank::Legendary;
		Spawn.Level = OwnLevel;
		AGravemotherCreature* Mother = Cast<AGravemotherCreature>(
			ACreatureBase::SpawnAtRuntime(World, AGravemotherCreature::StaticClass(), Feet, 0.f, Spawn));
		if (Mother && !Mother->HasActorBegunPlay())
		{
			Mother->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Mother);
		return Mother;
	}

	/** Hurts a creature down to Share of its health, as the game's damage arrives, from By (or nobody). */
	void HurtTo(ACreatureBase* Creature, float Share, AController* By = nullptr)
	{
		const UHealthComponent* Health = Creature ? Creature->FindComponentByClass<UHealthComponent>() : nullptr;
		if (Health)
		{
			BossTestWorld::Hurt(Creature, Health->GetHealth() - Health->GetMaxHealth() * Share, By);
		}
	}

	/** A hit on one of her bones, as a bullet reports it. */
	FHitResult HitOn(ACreatureBase& Creature, FName Bone)
	{
		FHitResult Hit;
		Hit.Component = Creature.GetMesh();
		Hit.BoneName = Bone;
		return Hit;
	}

	/** Her feet: the ground under her middle. */
	FVector FeetOf(const ACreatureBase& Creature)
	{
		return Creature.GetActorLocation() - FVector(0.0, 0.0, Creature.GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherBodyTest, "Looter.Creatures.Gravemother.Body",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherBodyTest::RunTest(const FString& Parameters)
{
	// As the design has her: a brown spider at 1.8 times its size, Legendary with her name in orange on her tag (no
	// "Soulfed"), twelve times a spider's health (about 5,600 at level 8), crits on the head and the abdomen (their hit
	// hulls), one of the spiders' pack, never back on her own; in her pale hide once it's made.
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
	TestTrue(TEXT("Her tag is orange: the legendary loot beam's color"),
		Legendary.Color.Equals(GetDefault<UWeaponDefinition>()->GetRarityInfo(EWeaponRarity::Legendary).Color, 0.01f));
	TestNearlyEqual(TEXT("1.8 times a brown spider's size, her rank's own included"), Mother->GetSizeScale(), Gravemother::Size, 0.001f);
	TestNearlyEqual(TEXT("...her body with it"), Mother->GetCapsuleComponent()->GetScaledCapsuleRadius(),
		Spider->GetCapsuleComponent()->GetScaledCapsuleRadius() * Gravemother::Size, 0.1f);
	TestFalse(TEXT("She never comes back on her own (her lair says when)"), Mother->WillRespawn());

	// Twelve spiders' health at her level: about 5,600 at level 8.
	const float SpiderHealth = Spider->FindComponentByClass<UHealthComponent>()->GetMaxHealth();
	Mother->SetLevel(8);
	const float Health = Mother->FindComponentByClass<UHealthComponent>()->GetMaxHealth();
	const float Wanted = SpiderHealth * GetDefault<UProgressionSettings>()->GetLevelRules().EnemyScale(8) * Legendary.HealthMultiplier;
	TestNearlyEqual(*FString::Printf(TEXT("At level 8: %.0f health (a spider's %.0f, grown for level 8, times %.0f)"), Health, SpiderHealth,
		Legendary.HealthMultiplier), Health, Wanted, 0.5f);
	TestTrue(TEXT("...about 5,600"), Health > 5400.f && Health < 5800.f);

	// Crits on the head and the abdomen, nowhere else; both have hit hulls on the spider's model.
	TestTrue(TEXT("The head is critical"), Mother->IsCriticalSpot(HitOn(*Mother, TEXT("head"))));
	TestTrue(TEXT("The abdomen is critical"), Mother->IsCriticalSpot(HitOn(*Mother, TEXT("abdomen"))));
	TestFalse(TEXT("A leg isn't"), Mother->IsCriticalSpot(HitOn(*Mother, TEXT("femur_0_l"))));
	TestFalse(TEXT("The thorax isn't"), Mother->IsCriticalSpot(HitOn(*Mother, TEXT("body"))));
	TestFalse(TEXT("A spider's abdomen isn't"), Spider->IsCriticalSpot(HitOn(*Spider, TEXT("abdomen"))));
	if (const UPhysicsAsset* Hulls = Mother->GetMesh()->GetPhysicsAsset())
	{
		TestTrue(TEXT("The head and the abdomen have hit hulls (no new bones)"),
			Hulls->FindBodyIndex(TEXT("head")) != INDEX_NONE && Hulls->FindBodyIndex(TEXT("abdomen")) != INDEX_NONE);
	}
	else
	{
		AddInfo(TEXT("SK_Spider has no physics asset here: its hit hulls weren't checked."));
	}

	// One of the spiders' pack, and her rank's call reaches every spider within 30 m.
	TestTrue(TEXT("A spider by her pack tag"), Mother->PackTag == Spider->PackTag && Mother->SharesPackWith(*Spider)
		&& Spider->SharesPackWith(*Mother));
	TestTrue(TEXT("Her call reaches 30 m"), Mother->GetPackCallRadius() >= 3000.f);

	// Her hide: MI_SpiderBody_Pale once it's made, else the spider's own.
	UMaterialInterface* Worn = Mother->GetMesh()->GetMaterial(0);
	const TCHAR* PaleHide = TEXT("/Game/Art/Materials/MI_SpiderBody_Pale.MI_SpiderBody_Pale");
	if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(PaleHide))))
	{
		TestEqual(TEXT("She wears the pale hide"), GetPathNameSafe(Worn), FString(PaleHide));
	}
	else
	{
		TestTrue(TEXT("Without the pale hide, the spider's own"), Worn == Spider->GetMesh()->GetMaterial(0));
		AddInfo(TEXT("MI_SpiderBody_Pale isn't made yet: she wears the spider's hide."));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherChargeTest, "Looter.Creatures.Gravemother.Charge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherChargeTest::RunTest(const FString& Parameters)
{
	// Her charge: with her target 5 to 16 m off and her way clear she rears and tracks them, then holds her aim while the
	// ground cracks open along her line through a long wind-up; only then she dashes straight down it, running down whoever
	// is in it once, and slams down at its end (the ground bursts); then it cools down and she bites. Up close she bites.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1000.0, 0.0, 0.0));
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("The Gravemother"), Mother))
	{
		return false;
	}
	TestTrue(TEXT("A long telegraph: over a second"), Mother->ChargeTelegraph >= 1.2f && Mother->ChargeAimSeconds < Mother->ChargeTelegraph);
	TestFalse(TEXT("She sizes up whoever she first sees before she charges"), Mother->IsChargeReady());
	Mother->AlertTo(Player);
	Mother->ReadyCharge();
	constexpr float Step = 0.05f;
	Mother->Tick(Step);
	TestTrue(TEXT("10 m off, the charge ready: she rears to charge"), Mother->GetCreatureState() == ECreatureState::Attack
		&& Mother->GetChargeState() == EGravemotherCharge::Telegraph);
	TestNearlyEqual(TEXT("The attack's wind-up is the telegraph"), Mother->AttackWindup, Mother->ChargeTelegraph, 0.001f);

	// Before her aim she tracks her target; after it the line is fixed, whatever they do.
	float Clock = 0.f;
	auto RunUntil = [&](float Time)
	{
		while (Clock + Step * 0.5f < Time && Mother->GetChargeState() == EGravemotherCharge::Telegraph)
		{
			Mother->Tick(Step);
			Clock += Step;
		}
	};
	RunUntil(0.25f);
	Player->SetActorLocation(FVector(1000.0, 300.0, 0.0));
	RunUntil(Mother->ChargeAimSeconds - 0.1f);
	TestNull(TEXT("No crack before she takes her aim"), Mother->GetCrack());
	const double Toward = FMath::RadiansToDegrees(FMath::Atan2(300.0, 1000.0));
	TestNearlyEqual(TEXT("She turns to follow her target while she rears"), Mother->GetActorRotation().Yaw, Toward, 3.0);
	RunUntil(Mother->ChargeAimSeconds + 0.1f);
	TestTrue(TEXT("Her aim taken"), Mother->HasTakenAim());
	const FVector Line = Mother->GetChargeDirection();
	TestNearlyEqual(TEXT("...at where they stood"), Line.Rotation().Yaw, Toward, 0.5);
	const float Scale = Mother->GetSizeScale();
	TestNearlyEqual(TEXT("...running on past them"), Mother->GetChargeLength(),
		static_cast<float>(FVector::Dist2D(Mother->GetActorLocation(), Player->GetActorLocation())) + Mother->ChargeOvershoot * Scale, 1.f);
	Player->SetActorLocation(FVector(1000.0, -400.0, 0.0));
	RunUntil(Mother->ChargeTelegraph - 0.1f);
	TestNearlyEqual(TEXT("Her aim held when they moved"), Mother->GetActorRotation().Yaw, Toward, 0.5);
	TestTrue(TEXT("...the line too"), Mother->GetChargeDirection().Equals(Line, 1e-4));
	TestTrue(TEXT("Still winding up: no dash before the telegraph's end"), Mother->GetChargeState() == EGravemotherCharge::Telegraph);
	const AGroundCrack* Crack = Mother->GetCrack();
	if (!TestNotNull(TEXT("The ground cracks open along her line"), Crack))
	{
		return false;
	}
	TestTrue(*FString::Printf(TEXT("...nearly all the way by now (%.2f)"), Crack->GetOpen()),
		Crack->GetOpen() > 0.9f && Crack->NumPiecesShown() > 0);
	const FVector Feet = FeetOf(*Mother);
	const float Front = Mother->GetCapsuleComponent()->GetScaledCapsuleRadius();
	TestTrue(TEXT("...from under her forelegs to where she'll stop"), Crack->GetStart().Equals(Feet + Line * Front, 1.0)
		&& Crack->GetEnd().Equals(Feet + Line * (Mother->GetChargeLength() + Front), 1.0));
	TestFalse(TEXT("Nobody run down during the wind-up"), Mother->HasChargeHit());

	// The dash, once the wind-up is over: full tilt down the line.
	while (Clock < 3.f && Mother->GetChargeState() == EGravemotherCharge::Telegraph)
	{
		Mother->Tick(Step);
		Clock += Step;
	}
	TestTrue(*FString::Printf(TEXT("She dashes at the telegraph's end (%.2f s)"), Clock), Mother->GetChargeState() == EGravemotherCharge::Dash
		&& FMath::Abs(Clock - Mother->ChargeTelegraph) <= Step * 1.5f);
	const float Speed = Mother->ChargeSpeed * Scale;
	TestTrue(TEXT("...straight down her line at full speed"), Mother->GetVelocity().GetSafeNormal2D().Equals(Line, 1e-3)
		&& FMath::IsNearlyEqual(static_cast<float>(Mother->GetVelocity().Size2D()), Speed, 1.f));
	TestNearlyEqual(TEXT("The crack lies open all the way"), Crack->GetOpen(), 1.f, 0.001f);

	// Someone standing in her line 6 m on is run down as she passes, once. (A test level doesn't move her: she's carried
	// down the line as her movement would carry her.)
	const FVector Start = Mother->GetChargeStart();
	Player->SetActorLocation(FVector(Start.X + Line.X * 600.0, Start.Y + Line.Y * 600.0, 0.0));
	float Covered = 0.f;
	while (Covered < Mother->GetChargeLength() + Speed * Step && Mother->GetChargeState() == EGravemotherCharge::Dash)
	{
		Covered = FMath::Min(Covered + Speed * Step, Mother->GetChargeLength());
		Mother->SetActorLocation(Start + Line * Covered);
		Mother->Tick(Step);
	}
	TestTrue(TEXT("Run down as she passed"), Mother->HasChargeHit());
	TestTrue(TEXT("At the crack's end she slams down: her recovery"), Mother->GetChargeState() == EGravemotherCharge::Recover);
	TestTrue(*FString::Printf(TEXT("...and the ground bursts round her forelegs (%d cracks)"), Crack->NumBurstCracks()),
		Crack->NumBurstCracks() >= 6);
	for (int32 Frame = 0; Frame < 100 && Mother->GetChargeState() != EGravemotherCharge::None; ++Frame)
	{
		Mother->Tick(Step);
	}
	TestTrue(TEXT("Up again: the charge is over"), Mother->GetChargeState() == EGravemotherCharge::None
		&& Mother->GetCreatureState() != ECreatureState::Attack);
	TestFalse(TEXT("...and cooling down"), Mother->IsChargeReady());
	TestTrue(TEXT("...its crack left to close"), Crack->IsClosing() && Mother->GetCrack() == nullptr);
	TestNearlyEqual(TEXT("...her bite's timing back"), Mother->AttackWindup, GetDefault<AGravemotherCreature>()->AttackWindup, 0.001f);

	// Up close (inside her charge's least distance) she bites, charge ready or not.
	ACharacter* Near = EncounterTestWorld::SpawnPlayer(World, FVector(300.0, 6000.0, 0.0));
	AGravemotherCreature* Biter = SpawnMother(World, FVector(0.0, 6000.0, 0.0));
	if (TestNotNull(TEXT("Second player stand-in"), Near) && TestNotNull(TEXT("Second Gravemother"), Biter))
	{
		Biter->AlertTo(Near);
		Biter->ReadyCharge();
		Biter->Tick(Step);
		TestTrue(TEXT("3 m off: a bite, not a charge"), Biter->GetCreatureState() == ECreatureState::Attack
			&& Biter->GetChargeState() == EGravemotherCharge::None);
		TestNearlyEqual(TEXT("...with a bite's wind-up"), Biter->AttackWindup, GetDefault<AGravemotherCreature>()->AttackWindup, 0.001f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherBroodTest, "Looter.Creatures.Gravemother.Brood",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherBroodTest::RunTest(const FString& Parameters)
{
	// Her brood: at 66% and at 33% of her health she calls four spiderlings, each call once a life (a hit past both lines
	// makes both), and a killing blow calls none. A spiderling is a brown spider at 0.45x with a fifth of its health (60 at
	// level 1), Basic and on the Basic table, named for what it is, come up round her on her level.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector, 1);
	if (!TestNotNull(TEXT("The Gravemother"), Mother))
	{
		return false;
	}
	TestEqual(TEXT("Level 1 of her own, and her rank's two on top"), Mother->Level,
		1 + UCreatureRankSettings::Get(ECreatureRank::Legendary).LevelOffset);
	HurtTo(Mother, 0.7f);
	TestTrue(TEXT("At 70%: no brood yet"), Mother->GetBroodCallsMade() == 0 && Mother->GetBrood().Num() == 0);
	HurtTo(Mother, 0.65f);
	const TArray<ASpiderCreature*> First = Mother->GetBrood();
	TestTrue(*FString::Printf(TEXT("At 65%%: her first call, four spiderlings (%d)"), First.Num()),
		Mother->GetBroodCallsMade() == 1 && First.Num() == Gravemother::BroodSize);

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
		TestTrue(TEXT("Come up round her"), FVector::Dist2D(Spot, Feet) <= Mother->BroodRadius * Scale + 1.0
			&& FVector::Dist2D(Spot, Feet) >= Mother->BroodSpacing - 1.0);
		// (A test level has no ground to settle a body on, so a spawned one stands a little off its spot: within a step.)
		TestTrue(TEXT("...on her level"), FMath::Abs(Spot.Z - Feet.Z) <= Mother->BroodMaxStep);
	}
	TestTrue(TEXT("...apart from each other"), EncounterTestWorld::AllApart(EncounterTestWorld::PlacesOf(Brood), Mother->BroodSpacing - 1.0));

	HurtTo(Mother, 0.5f);
	TestEqual(TEXT("At 50%: no more"), Mother->GetBroodCallsMade(), 1);
	HurtTo(Mother, 0.32f);
	TestTrue(TEXT("At 32%: her second call, four more"), Mother->GetBroodCallsMade() == 2
		&& Mother->GetBrood().Num() == 2 * Gravemother::BroodSize);
	HurtTo(Mother, 0.1f);
	TestEqual(TEXT("At 10%: no third"), Mother->GetBroodCallsMade(), 2);

	// One hit past both lines brings both; a killing blow brings none.
	AGravemotherCreature* Struck = SpawnMother(World, FVector(0.0, 8000.0, 0.0), 1);
	AGravemotherCreature* Felled = SpawnMother(World, FVector(0.0, -8000.0, 0.0), 1);
	if (TestNotNull(TEXT("Second Gravemother"), Struck) && TestNotNull(TEXT("Third Gravemother"), Felled))
	{
		HurtTo(Struck, 0.2f);
		TestTrue(TEXT("From full to 20% in one hit: both calls"), Struck->GetBroodCallsMade() == 2
			&& Struck->GetBrood().Num() == 2 * Gravemother::BroodSize);
		BossTestWorld::Kill(Felled);
		TestTrue(TEXT("Killed outright: none"), Felled->IsDead() && Felled->GetBroodCallsMade() == 0 && Felled->GetBrood().Num() == 0);
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
