#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/ShriekRing.h"
#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidRules.h"
#include "Progression/ProgressionSettings.h"
#include "Tests/BossTestWorld.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/PackageName.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Tests/AutomationCommon.h"

namespace
{
	/** An Unpaid in a test world, started as play starts it (a test world never begins play itself), dropping no loot. */
	AUnpaidCreature* SpawnUnpaid(UWorld* World, const FVector& Where, ECreatureRank Rank = ECreatureRank::Basic, USkeletalMesh* Model = nullptr)
	{
		AUnpaidCreature* Unpaid = World->SpawnActor<AUnpaidCreature>(Where, FRotator::ZeroRotator);
		if (Unpaid)
		{
			if (Model)
			{
				Unpaid->GetMesh()->SetSkeletalMeshAsset(Model);
			}
			Unpaid->StartingRank = Rank;
			Unpaid->DispatchBeginPlay();
			BossTestWorld::NoLoot(Unpaid);
		}
		return Unpaid;
	}

	/** The coal's color as its mesh carries it (custom primitive data), and how strongly it glows. */
	bool ReadCoalColor(const AUnpaidCreature& Unpaid, FLinearColor& OutColor, float& OutGlow)
	{
		const TArray<float>& Data = Unpaid.GetMesh()->GetCustomPrimitiveData().Data;
		const int32 First = UnpaidLook::RankColorIndex;
		if (!Data.IsValidIndex(First + 3))
		{
			return false;
		}
		OutColor = FLinearColor(Data[First], Data[First + 1], Data[First + 2]);
		OutGlow = Data[First + 3];
		return true;
	}

	/** A shot along From to To that lands on the chest's front, 10 cm before its line comes nearest the coal. */
	FHitResult ChestShot(const AUnpaidCreature& Unpaid, FName Bone, const FVector& From, const FVector& To)
	{
		const FVector Coal = Unpaid.GetCoalLocation();
		const FVector Direction = (To - From).GetSafeNormal();
		const FVector Nearest = From + Direction * FVector::DotProduct(Coal - From, Direction);
		FHitResult Hit;
		Hit.Component = Unpaid.GetMesh();
		Hit.BoneName = Bone;
		Hit.ImpactPoint = Nearest - Direction * 10.0;
		// Bullets keep the segment they traced on the hit; the test's segment stands in for it.
		Hit.TraceStart = From;
		Hit.TraceEnd = To;
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidStatsTest, "Looter.Creatures.Unpaid.Stats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidStatsTest::RunTest(const FString& Parameters)
{
	// The design's numbers (Docs/Areas/RansomsRest.md, "Enemies by rank"): 160 health and 8 damage at level 1; Restless
	// x2.5 health with a faster lunge and a blue coal; Gravebound x5 health, a purple coal and a shriek that slows. Each
	// rank's levels grow it as levels do, under its rank's multipliers.
	const AUnpaidCreature* Defaults = GetDefault<AUnpaidCreature>();
	TestEqual(TEXT("Level 1"), Defaults->Level, 1);
	TestEqual(TEXT("8 damage at level 1"), Defaults->AttackDamage, 8.f);
	TestTrue(TEXT("The Unpaid answer each other's calls"), !Defaults->PackTag.IsNone() && Defaults->PackAlertRadius > 0.f);
	// The encounters' cap names it by its class's path: no more than 12 at once.
	TestEqual(TEXT("12 Unpaid at once"), UEncounterSettings::Get().FindClassCap(AUnpaidCreature::StaticClass()), 12);

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const FLevelRules LevelRules = GetDefault<UProgressionSettings>()->GetLevelRules();
	struct FRankCase
	{
		ECreatureRank Rank;
		float Health;
	};
	const FRankCase Cases[] = { { ECreatureRank::Basic, 1.f }, { ECreatureRank::Rare, 2.5f }, { ECreatureRank::Epic, 5.f } };
	TMap<ECreatureRank, const AUnpaidCreature*> ByRank;
	double Column = 0.0;
	for (const FRankCase& Case : Cases)
	{
		const AUnpaidCreature* Unpaid = SpawnUnpaid(World, FVector(Column, 0.0, 0.0), Case.Rank);
		Column += 2000.0;
		if (!TestNotNull(TEXT("Unpaid spawned"), Unpaid))
		{
			return false;
		}
		ByRank.Add(Case.Rank, Unpaid);
		const FString Name = UCreatureRankSettings::GetRankName(Case.Rank);
		const FCreatureRankInfo& Info = UCreatureRankSettings::Get(Case.Rank);
		TestNearlyEqual(FString::Printf(TEXT("%s: its rank multiplies health by %.1f"), *Name, Case.Health), Info.HealthMultiplier, Case.Health, 0.001f);
		TestEqual(FString::Printf(TEXT("%s: level 1 and its rank's"), *Name), Unpaid->Level, 1 + Info.LevelOffset);
		const float Growth = LevelRules.EnemyScale(Unpaid->Level);
		const UHealthComponent* Health = Unpaid->FindComponentByClass<UHealthComponent>();
		if (TestNotNull(FString::Printf(TEXT("%s: has health"), *Name), Health))
		{
			TestNearlyEqual(FString::Printf(TEXT("%s: health"), *Name), Health->GetMaxHealth(), 160.f * Case.Health * Growth, 0.01f);
		}
		TestNearlyEqual(FString::Printf(TEXT("%s: damage"), *Name), Unpaid->AttackDamage, 8.f * Info.DamageMultiplier * Growth, 0.001f);

		// Its coal burns in its rank's color, which its mesh carries (no material instance per creature): dull red for Basic,
		// which is no rarity color; above it, the rank tag's color.
		FLinearColor Coal;
		float Glow = 0.f;
		if (TestTrue(FString::Printf(TEXT("%s: the mesh carries its coal's color"), *Name), ReadCoalColor(*Unpaid, Coal, Glow)))
		{
			const FLinearColor Wanted = Case.Rank == ECreatureRank::Basic ? FLinearColor::FromSRGBColor(FColor(0xB0, 0x2A, 0x18)) : Info.Color;
			TestTrue(FString::Printf(TEXT("%s: coal color %s"), *Name, *Coal.ToString()), Coal.Equals(Wanted, 0.001f));
			TestTrue(FString::Printf(TEXT("%s: the coal glows"), *Name), Glow > 0.f);
			TestTrue(FString::Printf(TEXT("%s: the color it reports is the one it wears"), *Name), Unpaid->GetCoalColor().Equals(Wanted, 0.001f));
		}
	}

	// What the ranks add: Restless lunges faster, after a shorter shriek; Gravebound too, and its shriek slows.
	const AUnpaidCreature* Basic = ByRank.FindRef(ECreatureRank::Basic);
	const AUnpaidCreature* Restless = ByRank.FindRef(ECreatureRank::Rare);
	const AUnpaidCreature* Gravebound = ByRank.FindRef(ECreatureRank::Epic);
	TestTrue(TEXT("A Restless one lunges faster than a Basic one (beyond its size)"),
		Restless->GetLungeSpeed() / Restless->GetSizeScale() > Basic->GetLungeSpeed() / Basic->GetSizeScale() * 1.2f);
	TestTrue(TEXT("A Restless one's shriek is shorter"), Restless->AttackWindup < Basic->AttackWindup);
	TestNearlyEqual(TEXT("A Basic one's wind-up is the class's"), Basic->AttackWindup, Defaults->AttackWindup, 0.0001f);
	TestFalse(TEXT("A Basic one's shriek slows nobody"), Basic->GetRankTraits().bSlowingShriek);
	TestFalse(TEXT("Nor a Restless one's"), Restless->GetRankTraits().bSlowingShriek);
	TestTrue(TEXT("A Gravebound one's shriek slows"), Gravebound->GetRankTraits().bSlowingShriek);
	TestFalse(TEXT("A boss tunes its own fight"), UnpaidRules::RankTraits(ECreatureRank::Boss).bSlowingShriek);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidCoalTest, "Looter.Creatures.Unpaid.Coal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidCoalTest::RunTest(const FString& Parameters)
{
	// The coal is the crit spot, found by the shot's line as the slime's core is: a shot from within 60 degrees of the way
	// it faces whose line passes within 12 cm of it. The rule as plain geometry: a coal at chest height facing +X.
	const FVector Coal(0.0, 0.0, 130.0);
	const FVector Facing = FVector::ForwardVector;
	auto IsCrit = [&Coal, &Facing](const FVector& From, const FVector& Through)
	{
		const FVector Direction = (Through - From).GetSafeNormal();
		const FVector Nearest = From + Direction * FVector::DotProduct(Coal - From, Direction);
		return UnpaidRules::IsCoalShot(Coal, Facing, Nearest - Direction * 10.0, Direction, 12.f, 60.f, 60.f);
	};
	auto Around = [&Coal](float Degrees, float Up)
	{
		const float Radians = FMath::DegreesToRadians(Degrees);
		return Coal + FVector(FMath::Cos(Radians) * 500.f, FMath::Sin(Radians) * 500.f, Up);
	};
	TestTrue(TEXT("Straight at it from the front"), IsCrit(Around(0.f, 0.f), Coal));
	TestTrue(TEXT("From 50 degrees to the side"), IsCrit(Around(50.f, 0.f), Coal));
	TestTrue(TEXT("From the front and above"), IsCrit(Around(0.f, 350.f), Coal));
	TestFalse(TEXT("Not from 70 degrees to the side: the body is in the way"), IsCrit(Around(70.f, 0.f), Coal));
	TestFalse(TEXT("Not from behind"), IsCrit(Around(180.f, 0.f), Coal));
	TestTrue(TEXT("A line 10 cm beside it"), IsCrit(Coal + FVector(500.0, 10.0, 0.0), Coal + FVector(-100.0, 10.0, 0.0)));
	TestFalse(TEXT("Not a line 14 cm beside it"), IsCrit(Coal + FVector(500.0, 14.0, 0.0), Coal + FVector(-100.0, 14.0, 0.0)));

	// An Unpaid with no model (a test level's) carries its coal where the model does, and plays the same rule on the shots
	// that land on its chest; an arm or the head in the way takes the hit.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	const AUnpaidCreature* Unpaid = SpawnUnpaid(World, FVector::ZeroVector);
	const AUnpaidCreature* Gravebound = SpawnUnpaid(World, FVector(0.0, 3000.0, 0.0), ECreatureRank::Epic);
	if (!TestNotNull(TEXT("Unpaid spawned"), Unpaid) || !TestNotNull(TEXT("Gravebound spawned"), Gravebound))
	{
		return false;
	}
	const FVector Chest = Unpaid->GetCoalLocation();
	if (!Unpaid->GetMesh()->GetSkeletalMeshAsset())
	{
		TestTrue(TEXT("Without its model, the coal is at chest height, a little forward and to its left"),
			Chest.Equals(Unpaid->GetActorLocation() + FVector(12.0, -8.0, 40.0), 0.5));
	}
	TestTrue(TEXT("Its coal faces the way it faces (give or take its hunch)"), FVector::DotProduct(Unpaid->GetCoalFacing(), FVector::ForwardVector) > 0.95);
	const FName ChestBone = Unpaid->Rig.Chest;
	TestTrue(TEXT("On the chest from the front: critical"),
		Unpaid->IsCriticalSpot(ChestShot(*Unpaid, ChestBone, Chest + FVector(600.0, 0.0, 0.0), Chest - FVector(100.0, 0.0, 0.0))));
	TestTrue(TEXT("On the coal itself: critical"),
		Unpaid->IsCriticalSpot(ChestShot(*Unpaid, Unpaid->Rig.Coal, Chest + FVector(600.0, 0.0, 0.0), Chest - FVector(100.0, 0.0, 0.0))));
	TestFalse(TEXT("Through an arm: not critical"),
		Unpaid->IsCriticalSpot(ChestShot(*Unpaid, Unpaid->Rig.LeftArm.LowerArm, Chest + FVector(600.0, 0.0, 0.0), Chest - FVector(100.0, 0.0, 0.0))));
	TestFalse(TEXT("On the head: not critical"),
		Unpaid->IsCriticalSpot(ChestShot(*Unpaid, Unpaid->Rig.Head, Chest + FVector(600.0, 0.0, 0.0), Chest - FVector(100.0, 0.0, 0.0))));
	TestFalse(TEXT("On its back, from behind: not critical"),
		Unpaid->IsCriticalSpot(ChestShot(*Unpaid, ChestBone, Chest - FVector(600.0, 0.0, 0.0), Chest + FVector(100.0, 0.0, 0.0))));
	FHitResult Elsewhere = ChestShot(*Unpaid, ChestBone, Chest + FVector(600.0, 0.0, 0.0), Chest - FVector(100.0, 0.0, 0.0));
	Elsewhere.Component = nullptr;
	TestFalse(TEXT("A hit on something else isn't its coal"), Unpaid->IsCriticalSpot(Elsewhere));

	// A bigger one's coal is as much bigger: a line 14 cm off misses a Basic one's and finds a Gravebound one's (x1.2).
	const FVector BigCoal = Gravebound->GetCoalLocation();
	TestFalse(TEXT("14 cm off a Basic one's coal: not critical"),
		Unpaid->IsCriticalSpot(ChestShot(*Unpaid, ChestBone, Chest + FVector(600.0, 14.0, 0.0), Chest + FVector(-100.0, 14.0, 0.0))));
	TestTrue(TEXT("14 cm off a Gravebound one's: critical"),
		Gravebound->IsCriticalSpot(ChestShot(*Gravebound, ChestBone, BigCoal + FVector(600.0, 14.0, 0.0), BigCoal + FVector(-100.0, 14.0, 0.0))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidShriekTest, "Looter.Creatures.Unpaid.Shriek",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidShriekTest::RunTest(const FString& Parameters)
{
	// A Gravebound one's shriek sends out a ring that slows the player it shrieks at, as it passes them, for 2 seconds:
	// they walk at half speed, then at their own again. A Basic one's shriek slows nobody, and one too far off is spared.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(400.0, 0.0, 0.0));
	AUnpaidCreature* Gravebound = SpawnUnpaid(World, FVector::ZeroVector, ECreatureRank::Epic);
	AUnpaidCreature* Basic = SpawnUnpaid(World, FVector(0.0, 600.0, 0.0));
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Gravebound spawned"), Gravebound) || !TestNotNull(TEXT("Basic spawned"), Basic))
	{
		return false;
	}
	UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
	const float OwnSpeed = Movement->MaxWalkSpeed;

	Basic->AlertTo(Player);
	TestNull(TEXT("A Basic one's shriek sends no ring"), Basic->Shriek());

	Gravebound->AlertTo(Player);
	AShriekRing* Ring = Gravebound->Shriek();
	if (!TestNotNull(TEXT("A Gravebound one's shriek sends out a ring"), Ring))
	{
		return false;
	}
	TestNull(TEXT("Not slowed before the ring reaches them"), Player->FindComponentByClass<UMovementSlowComponent>());
	for (int32 Step = 0; Step < 40 && !Ring->HasSlowed(); ++Step)
	{
		Ring->Advance(0.02f);
	}
	TestTrue(TEXT("The ring slowed the player as it passed them"), Ring->HasSlowed());
	TestTrue(TEXT("It reached them about where they stand"), Ring->GetRadius() > 300.f && Ring->GetRadius() < 480.f);
	UMovementSlowComponent* Slow = Player->FindComponentByClass<UMovementSlowComponent>();
	if (!TestNotNull(TEXT("The player is slowed"), Slow))
	{
		return false;
	}
	TestNearlyEqual(TEXT("For 2 seconds"), Slow->GetTimeLeft(), Gravebound->ShriekSlowSeconds, 0.001f);
	TestNearlyEqual(TEXT("2 seconds, as the design has it"), Gravebound->ShriekSlowSeconds, 2.f, 0.001f);
	TestNearlyEqual(TEXT("At the shriek's share of their speed"), Movement->MaxWalkSpeed, OwnSpeed * Gravebound->ShriekSlowMultiplier, 0.01f);
	TestTrue(TEXT("A real slow"), Gravebound->ShriekSlowMultiplier < 0.9f);
	TestNull(TEXT("Its next shriek waits for its cooldown"), Gravebound->Shriek());

	Slow->Advance(1.9f);
	TestTrue(TEXT("Still slowed at 1.9 seconds"), Slow->IsSlowed());
	TestNearlyEqual(TEXT("Still at the slowed speed"), Movement->MaxWalkSpeed, OwnSpeed * Gravebound->ShriekSlowMultiplier, 0.01f);
	Slow->Advance(0.2f);
	TestFalse(TEXT("Free at 2.1 seconds"), Slow->IsSlowed());
	TestNearlyEqual(TEXT("At their own speed again"), Movement->MaxWalkSpeed, OwnSpeed, 0.01f);

	// A player beyond the ring's reach isn't slowed, however long it runs.
	ACharacter* Far = BossTestWorld::SpawnPlayer(World, FVector(0.0, -3000.0, 0.0));
	AUnpaidCreature* FarShrieker = SpawnUnpaid(World, FVector(0.0, -500.0, 0.0), ECreatureRank::Epic);
	if (TestNotNull(TEXT("Far player"), Far) && TestNotNull(TEXT("Far shrieker"), FarShrieker))
	{
		FarShrieker->AlertTo(Far);
		AShriekRing* FarRing = FarShrieker->Shriek();
		if (TestNotNull(TEXT("It shrieks at them"), FarRing))
		{
			for (int32 Step = 0; Step < 25; ++Step)
			{
				FarRing->Advance(0.02f);
			}
			TestFalse(TEXT("The ring never reaches 25 m"), FarRing->HasSlowed());
		}
		TestNull(TEXT("The far player isn't slowed"), Far->FindComponentByClass<UMovementSlowComponent>());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidModelTest, "Looter.Creatures.Unpaid.Model",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidModelTest::RunTest(const FString& Parameters)
{
	// SK_Unpaid against what the code expects of it (Art/Models/Creatures/Unpaid.py): every bone the code poses by the
	// names in its Rig, the body and coal slots, the coal at chest height in front, hit zones that answer bullets' complex
	// traces, and the coal found through the chest's hit zone from the front, never from behind. Until the model is
	// imported there is nothing to check.
	const TCHAR* ModelPath = TEXT("/Game/Art/Creatures/SK_Unpaid.SK_Unpaid");
	if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ModelPath))))
	{
		AddWarning(TEXT("SK_Unpaid isn't imported yet (Art/Models/Creatures/Unpaid.py): the model's checks wait for it."));
		return true;
	}
	USkeletalMesh* Model = LoadObject<USkeletalMesh>(nullptr, ModelPath);
	if (!TestNotNull(TEXT("SK_Unpaid loads"), Model))
	{
		return false;
	}
	const AUnpaidCreature* Defaults = GetDefault<AUnpaidCreature>();
	const FUnpaidRigBones& Rig = Defaults->Rig;
	TArray<FName> Bones = { Rig.Pelvis, Rig.Spine, Rig.Chest, Rig.Coal, Rig.Neck, Rig.Head, Rig.Jaw };
	for (const FUnpaidArmBones* Arm : { &Rig.LeftArm, &Rig.RightArm })
	{
		Bones.Append({ Arm->UpperArm, Arm->LowerArm, Arm->Hand });
		Bones.Append(Arm->Fingers);
	}
	Bones.Append(Rig.Shroud);
	Bones.Append(Rig.LeftStrip);
	Bones.Append(Rig.RightStrip);
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	for (const FName Bone : Bones)
	{
		TestTrue(FString::Printf(TEXT("The rig has %s"), *Bone.ToString()), Skeleton.FindBoneIndex(Bone) != INDEX_NONE);
	}
	for (const TCHAR* Slot : { TEXT("Ghost_A"), TEXT("GhostCoal") })
	{
		TestTrue(FString::Printf(TEXT("A %s material slot"), Slot),
			Model->GetMaterials().ContainsByPredicate([Slot](const FSkeletalMaterial& Material) { return Material.MaterialSlotName == FName(Slot); }));
	}
	UPhysicsAsset* Physics = Model->GetPhysicsAsset();
	if (TestNotNull(TEXT("Hit zones (physics asset)"), Physics))
	{
		TestTrue(TEXT("A hit zone on the chest"), Physics->FindBodyIndex(Rig.Chest) != INDEX_NONE);
		TestTrue(TEXT("A hit zone on the head"), Physics->FindBodyIndex(Rig.Head) != INDEX_NONE);
		for (const USkeletalBodySetup* Body : Physics->SkeletalBodySetups)
		{
			TestTrue(FString::Printf(TEXT("%s's hit zone answers complex traces"), *Body->BoneName.ToString()),
				Body->CollisionTraceFlag == CTF_UseSimpleAsComplex);
		}
	}

	// In a test world, with the model, standing at the origin and facing +X in its rest pose.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AUnpaidCreature* Unpaid = SpawnUnpaid(WorldWrapper.GetTestWorld(), FVector::ZeroVector, ECreatureRank::Basic, Model);
	if (!TestNotNull(TEXT("Unpaid spawned"), Unpaid))
	{
		return false;
	}
	USkeletalMeshComponent* Mesh = Unpaid->GetMesh();
	const FVector Coal = Unpaid->GetCoalLocation();
	const FVector Ground = Mesh->GetComponentLocation();
	TestTrue(FString::Printf(TEXT("The coal is at chest height (%.0f cm)"), Coal.Z - Ground.Z), Coal.Z - Ground.Z > 110.0 && Coal.Z - Ground.Z < 150.0);
	TestTrue(TEXT("The coal is on the front of the chest"), Coal.X > Ground.X);
	TestTrue(TEXT("Its pose has the bones it moves"), Unpaid->GetBonePose().Num() >= 20);

	const FCollisionQueryParams BulletQuery(SCENE_QUERY_STAT(UnpaidModelTest), /*bTraceComplex*/ true);
	struct FShot
	{
		const TCHAR* What;
		FVector From;
		FVector To;
		bool bCritical;
	};
	const FShot Shots[] = {
		{ TEXT("at the coal from the front"), Coal + FVector(500.0, 0.0, 0.0), Coal - FVector(100.0, 0.0, 0.0), true },
		{ TEXT("at the coal from the front and a little above"), Coal + FVector(450.0, 0.0, 120.0), Coal, true },
		{ TEXT("at the coal from behind"), Coal - FVector(500.0, 0.0, 0.0), Coal + FVector(100.0, 0.0, 0.0), false } };
	for (const FShot& Shot : Shots)
	{
		FHitResult Hit;
		if (TestTrue(FString::Printf(TEXT("%s: hits"), Shot.What), Mesh->LineTraceComponent(Hit, Shot.From, Shot.To, BulletQuery)))
		{
			Hit.TraceStart = Shot.From;
			Hit.TraceEnd = Shot.To;
			TestEqual(FString::Printf(TEXT("%s (on %s): critical"), Shot.What, *Hit.BoneName.ToString()), Unpaid->IsCriticalSpot(Hit), Shot.bCritical);
		}
	}
	return true;
}

#endif
