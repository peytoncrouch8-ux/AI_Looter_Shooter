#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureSteerPlanner.h"
#include "Creatures/CreatureSteerProbe.h"
#include "Creatures/CreatureUnstick.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The creature steering against real geometry in test levels: what its looks see (a knee-high rail, a kerb under its step,
// a wall, a wall's top edge grazed, a ramp), a spider hung on a crate's edge sliding free, and a spider chasing across two
// fence lines, carried as its walking movement would carry it (a test level never moves anything itself).
// CreatureSteeringTests.cpp has the planner and the unstick's rules on their own.

namespace
{
	constexpr float PlayFrame = 1.f / 60.f;

	/** A block of the engine's cube at Size (cm), turned by Rotation, blocking everything (sized before it has a mesh, as LocomotionTestWorld::SpawnBlock). */
	AStaticMeshActor* SpawnTurnedBlock(UWorld* World, const FVector& Center, const FVector& Size, const FRotator& Rotation)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		AStaticMeshActor* Block = Cube ? World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(Rotation, Center, Size / 100.0)) : nullptr;
		if (Block)
		{
			Block->GetStaticMeshComponent()->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Block->GetStaticMeshComponent()->SetStaticMesh(Cube);
		}
		return Block;
	}

	/** A creature of Class spawned as play spawns it, standing on the floor at Feet, dropping no loot. */
	template <typename TCreature>
	TCreature* SpawnStanding(UWorld* World, const FVector& Feet)
	{
		return Cast<TCreature>(EncounterTestWorld::SpawnCreature(World, TCreature::StaticClass(), Feet));
	}

	/**
	 * Its walking movement given its capsule (a test level's movement takes none: Docs/Pipeline.md, "Tests"), in Mode,
	 * so the steering sees it walking or falling as in the game.
	 */
	void GiveMovement(ACreatureBase& Creature, EMovementMode Mode)
	{
		UCharacterMovementComponent* Movement = Creature.GetCharacterMovement();
		Movement->SetUpdatedComponent(Creature.GetCapsuleComponent());
		Movement->SetMovementMode(MOVE_Falling);
		if (Mode != MOVE_Falling)
		{
			Movement->SetMovementMode(Mode);
		}
		Movement->Velocity = FVector::ZeroVector;
	}

	/**
	 * One frame of the walking movement carrying Creature along the way its brain asked for (taking its input), sliding
	 * along what blocks it and telling it so, as UCharacterMovementComponent does (MoveBlockedBy); its velocity is what it
	 * moved. Nothing while it glides free (the glide moves it).
	 */
	void CarryOneFrame(ACreatureBase& Creature)
	{
		UCharacterMovementComponent* Movement = Creature.GetCharacterMovement();
		const FVector Wanted = Creature.ConsumeMovementInputVector();
		if (Creature.IsUnsticking())
		{
			Movement->Velocity = FVector::ZeroVector;
			return;
		}
		const FVector Was = Creature.GetActorLocation();
		FVector Move = FVector(Wanted.X, Wanted.Y, 0.0).GetClampedToMaxSize(1.0) * (Movement->MaxWalkSpeed * PlayFrame);
		for (int32 Slide = 0; Slide < 2 && !Move.IsNearlyZero(); ++Slide)
		{
			FHitResult Hit;
			Creature.SetActorLocation(Creature.GetActorLocation() + Move, true, &Hit);
			if (!Hit.bBlockingHit)
			{
				break;
			}
			Creature.MoveBlockedBy(Hit);
			const FVector Normal = FVector(Hit.Normal.X, Hit.Normal.Y, 0.0).GetSafeNormal();
			const FVector Left = Move * (1.0 - Hit.Time);
			Move = Left - Normal * FVector::DotProduct(Left, Normal);
		}
		const FVector Moved = Creature.GetActorLocation() - Was;
		Movement->Velocity = FVector(Moved.X, Moved.Y, 0.0) / PlayFrame;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureSteeringLooksTest, "Looter.Creatures.Steering.Looks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureSteeringLooksTest::RunTest(const FString& Parameters)
{
	// What the steering's looks see, against real geometry, each creature in a lane of its own 2 m short of its obstacle:
	// a knee-high rail (an Unpaid's old look, a sphere at its middle, passed over it and it walked into rails); a kerb and a
	// box under the step it walks up, and a box over it; a wall's top edge grazed (a slope that isn't there); a ramp.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(12000.0, 12000.0, 100.0));
	// Obstacles, their near face at X = 200: a rail 60 cm high, a kerb 30 and a box 40 (both under a spider's 55 cm step),
	// a box 60 (over it), and a 20 degree ramp starting at X = 160.
	const AStaticMeshActor* Rail = LocomotionTestWorld::SpawnBlock(World, FVector(205.0, 0.0, 30.0), FVector(10.0, 400.0, 60.0));
	LocomotionTestWorld::SpawnBlock(World, FVector(220.0, 1500.0, 15.0), FVector(40.0, 400.0, 30.0));
	LocomotionTestWorld::SpawnBlock(World, FVector(220.0, 3000.0, 20.0), FVector(40.0, 400.0, 40.0));
	LocomotionTestWorld::SpawnBlock(World, FVector(220.0, 4500.0, 30.0), FVector(40.0, 400.0, 60.0));
	const AStaticMeshActor* Ramp = SpawnTurnedBlock(World, FVector(445.0, 6000.0, 93.0), FVector(600.0, 400.0, 20.0), FRotator(20.0, 0.0, 0.0));
	AUnpaidCreature* Unpaid = SpawnStanding<AUnpaidCreature>(World, FVector(0.0, 0.0, 0.0));
	ASpiderCreature* KerbSpider = SpawnStanding<ASpiderCreature>(World, FVector(0.0, 1500.0, 0.0));
	ASpiderCreature* BoxSpider = SpawnStanding<ASpiderCreature>(World, FVector(0.0, 3000.0, 0.0));
	ASpiderCreature* WallSpider = SpawnStanding<ASpiderCreature>(World, FVector(0.0, 4500.0, 0.0));
	ASpiderCreature* RampSpider = SpawnStanding<ASpiderCreature>(World, FVector(0.0, 6000.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("A rail"), Rail) || !TestNotNull(TEXT("A ramp"), Ramp)
		|| !TestNotNull(TEXT("An Unpaid"), Unpaid) || !KerbSpider || !BoxSpider || !WallSpider || !TestNotNull(TEXT("Spiders"), RampSpider))
	{
		return false;
	}
	const FVector Ahead = FVector::ForwardVector;

	// The rail: under the Unpaid's middle, over its step. Its old look (a sphere 0.8 of its width at its middle) cleared it.
	const ACreatureBase::FSteerProbes UnpaidProbes = Unpaid->GetSteerProbes();
	const float UnpaidRadius = Unpaid->GetCapsuleComponent()->GetScaledCapsuleRadius();
	TestTrue(FString::Printf(TEXT("The Unpaid's old look passed over a 60 cm rail (its foot %.0f cm up)"), Unpaid->GetActorLocation().Z - 0.8f * UnpaidRadius),
		Unpaid->GetActorLocation().Z - 0.8f * UnpaidRadius > 60.0);
	const FCreatureSteerPlanner::FLookResult AtRail = Unpaid->LookAlong(Ahead, UnpaidProbes.SweepLength);
	TestFalse(TEXT("Its look sees the knee-high rail"), AtRail.bClear);
	TestTrue(FString::Printf(TEXT("...short of it (%.0f cm on)"), AtRail.FreeDistance), AtRail.FreeDistance < 200.f);
	TestTrue(FString::Printf(TEXT("...facing back at it (%.2f, %.2f)"), AtRail.WallNormal.X, AtRail.WallNormal.Y),
		FVector::DotProduct(AtRail.WallNormal, Ahead) < -0.7);
	TestFalse(TEXT("...not as a drop"), AtRail.bLedge);

	// Under its step it walks on; over it, it's in the way.
	const float Step = KerbSpider->GetCharacterMovement()->MaxStepHeight;
	TestTrue(FString::Printf(TEXT("A spider steps %.0f cm: over a 40 cm box, under a 60 cm one"), Step), Step > 40.f / 0.9f && Step < 60.f);
	TestTrue(TEXT("A 30 cm kerb isn't in its way"), KerbSpider->LookAlong(Ahead, KerbSpider->GetSteerProbes().SweepLength).bClear);
	TestTrue(TEXT("...nor a 40 cm box"), BoxSpider->LookAlong(Ahead, BoxSpider->GetSteerProbes().SweepLength).bClear);
	const FCreatureSteerPlanner::FLookResult AtWall = WallSpider->LookAlong(Ahead, WallSpider->GetSteerProbes().SweepLength);
	TestTrue(TEXT("A 60 cm box is"), !AtWall.bClear && FVector::DotProduct(AtWall.WallNormal, Ahead) < -0.7);
	TestTrue(TEXT("A 20 degree ramp isn't: it walks up slopes"), RampSpider->LookAlong(Ahead, RampSpider->GetSteerProbes().SweepLength).bClear);

	// A touch at the 60 cm box's top edge reported as a slope (as a mesh grazed there reports its top) isn't ground it walks
	// on; one at the kerb's edge reported as an upright face (as a box's edge is) is.
	const FCreatureSteerProbe WallProbe(*WallSpider);
	FHitResult Graze;
	Graze.bBlockingHit = true;
	Graze.ImpactPoint = FVector(200.0, 4500.0, 60.0);
	Graze.Location = FVector(170.0, 4500.0, 72.0);
	Graze.ImpactNormal = FVector::UpVector;
	Graze.Normal = FVector(-0.5, 0.0, 0.866);
	TestFalse(TEXT("A wall's top edge grazed isn't walkable"), WallProbe.IsWalkableContact(Graze, Ahead));
	const FCreatureSteerProbe KerbProbe(*KerbSpider);
	FHitResult KerbEdge = Graze;
	KerbEdge.ImpactPoint = FVector(200.0, 1500.0, 30.0);
	KerbEdge.ImpactNormal = FVector(-1.0, 0.0, 0.0);
	FVector Surface = FVector::ZeroVector;
	TestTrue(TEXT("A kerb's edge is"), KerbProbe.IsWalkableContact(KerbEdge, Ahead, &Surface));
	TestTrue(FString::Printf(TEXT("...judged by the kerb's top (%.2f up)"), Surface.Z), Surface.Z > 0.99);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureSteeringHungTest, "Looter.Creatures.Steering.Hung",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureSteeringHungTest::RunTest(const FString& Parameters)
{
	// A spider hung on a crate's edge (in the air, going nowhere) glides free after a moment to a spot with ground under it,
	// as the player's unstick does: not at once, not a pop, and clear of the crate.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	// An 80 cm crate, its edge at X = 0.
	const AStaticMeshActor* Crate = LocomotionTestWorld::SpawnBlock(World, FVector(-50.0, 0.0, 40.0), FVector(100.0, 100.0, 80.0));
	ASpiderCreature* Spider = SpawnStanding<ASpiderCreature>(World, FVector(600.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("A crate"), Crate) || !TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	GiveMovement(*Spider, MOVE_Falling);
	UCharacterMovementComponent* Movement = Spider->GetCharacterMovement();
	const UCapsuleComponent* Capsule = Spider->GetCapsuleComponent();
	const FCollisionShape Body = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CreatureHungTest), false, Spider);

	// Set down on the crate's edge, its middle 30 cm off it: hung there in the air (the test level never moves it).
	FHitResult Rest;
	if (!TestTrue(TEXT("The crate is under it"), World->SweepSingleByChannel(Rest, FVector(30.0, 0.0, 400.0), FVector(30.0, 0.0, 0.0), FQuat::Identity,
		ECC_Pawn, Body, Params)) || !TestTrue(TEXT("...its edge"), Rest.GetActor() == Crate))
	{
		return false;
	}
	Spider->SetActorLocation(Rest.Location + FVector(0.0, 0.0, 0.5));
	Movement->Velocity = FVector::ZeroVector;
	const FVector Hung = Spider->GetActorLocation();
	Spider->DevPutInState(ECreatureState::Wander, nullptr, FVector(1500.0, 0.0, 0.0));

	int32 Frames = 0;
	while (!Spider->IsUnsticking() && Frames < 90)
	{
		Spider->Tick(PlayFrame);
		Spider->ConsumeMovementInputVector();
		++Frames;
	}
	if (!TestTrue(FString::Printf(TEXT("Hung on the crate's edge: it glides free (after %.2f s)"), Frames * PlayFrame), Spider->IsUnsticking()))
	{
		return false;
	}
	TestTrue(TEXT("...not at once (a moment's grace)"), Frames * PlayFrame >= FCreatureUnstick::HungSeconds - PlayFrame);
	FVector Was = Spider->GetActorLocation();
	double Biggest = 0.0;
	int32 GlideFrames = 0;
	while (Spider->IsUnsticking() && GlideFrames < 60)
	{
		Spider->Tick(PlayFrame);
		Spider->ConsumeMovementInputVector();
		Biggest = FMath::Max(Biggest, FVector::Dist(Spider->GetActorLocation(), Was));
		Was = Spider->GetActorLocation();
		++GlideFrames;
	}
	const FVector Free = Spider->GetActorLocation();
	const double Slid = FVector::Dist(Free, Hung);
	TestFalse(TEXT("The glide ends"), Spider->IsUnsticking());
	TestTrue(FString::Printf(TEXT("...in about a quarter second (%.2f s)"), GlideFrames * PlayFrame), GlideFrames * PlayFrame <= FCreatureUnstick::GlideSeconds + 3.f * PlayFrame);
	TestTrue(FString::Printf(TEXT("...off the crate (%.0f cm)"), FVector::Dist2D(Free, Hung)), FVector::Dist2D(Free, Hung) >= 25.0);
	TestTrue(FString::Printf(TEXT("...gliding, not popping (%.1f cm a frame at most of %.0f)"), Biggest, Slid), Biggest <= 0.2 * Slid);
	const FCollisionShape Narrower = FCollisionShape::MakeCapsule(Body.GetCapsuleRadius() - 1.f, Body.GetCapsuleHalfHeight());
	TestFalse(TEXT("...clear of the crate"), World->OverlapBlockingTestByChannel(Free, FQuat::Identity, ECC_Pawn, Narrower, Params));
	FHitResult Ground;
	TestTrue(TEXT("...over the floor, which it stands on"), World->SweepSingleByChannel(Ground, Free, Free - FVector(0.0, 0.0, 300.0), FQuat::Identity,
		ECC_Pawn, Narrower, Params) && Ground.GetActor() == Floor && Movement->IsWalkable(Ground));
	TestTrue(TEXT("...falling the last little way"), Movement->IsFalling());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureSteeringFenceRunTest, "Looter.Creatures.Steering.FenceRun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureSteeringFenceRunTest::RunTest(const FString& Parameters)
{
	// A spider chasing a player across two fence lines (8 m each, 1.2 m high, offset), carried as its walking movement
	// would carry it: it goes round each and reaches them, never through a fence, without ping-pong.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = LocomotionTestWorld::SpawnBlock(World, FVector(1000.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	const AStaticMeshActor* NearFence = LocomotionTestWorld::SpawnBlock(World, FVector(600.0, 0.0, 60.0), FVector(10.0, 800.0, 120.0));
	const AStaticMeshActor* FarFence = LocomotionTestWorld::SpawnBlock(World, FVector(1300.0, 300.0, 60.0), FVector(10.0, 800.0, 120.0));
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1900.0, 0.0, 100.0));
	ASpiderCreature* Spider = SpawnStanding<ASpiderCreature>(World, FVector(0.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("Fences"), NearFence) || !FarFence || !TestNotNull(TEXT("A player stand-in"), Player)
		|| !TestNotNull(TEXT("A spider"), Spider))
	{
		return false;
	}
	GiveMovement(*Spider, MOVE_Walking);
	Spider->AlertTo(Player);
	if (!TestTrue(TEXT("It hunts the player"), Spider->GetCreatureState() == ECreatureState::Chase))
	{
		return false;
	}

	const float Reach = Spider->GetAttackRange() * 1.25f;
	const FCollisionShape Body = FCollisionShape::MakeCapsule(Spider->GetCapsuleComponent()->GetScaledCapsuleRadius() - 2.f,
		Spider->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	// Only the fences are left for the check (its body a little narrower, so sliding along one doesn't count).
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CreatureFenceRunTest), false, Spider);
	Params.AddIgnoredActor(Player);
	Params.AddIgnoredActor(Floor);
	float Clock = 0.f;
	bool bCaught = false;
	bool bThroughFence = false;
	while (Clock < 30.f)
	{
		Spider->Tick(PlayFrame);
		if (Spider->GetCreatureState() == ECreatureState::Attack || FVector::Dist2D(Spider->GetActorLocation(), Player->GetActorLocation()) <= Reach)
		{
			bCaught = true;
			break;
		}
		CarryOneFrame(*Spider);
		bThroughFence |= World->OverlapBlockingTestByChannel(Spider->GetActorLocation(), FQuat::Identity, ECC_Pawn, Body, Params);
		Clock += PlayFrame;
	}
	const FCreatureSteerPlanner& Planner = Spider->GetSteerPlanner();
	const FString Seen = FString::Printf(TEXT("after %.1f s at (%.0f, %.0f): %d side changes, %d turns back, %d walls followed"), Clock,
		Spider->GetActorLocation().X, Spider->GetActorLocation().Y, Planner.GetSideFlips(), Planner.GetReversals(), Planner.GetWallFollows());
	TestTrue(*FString::Printf(TEXT("It reaches the player round both fences (%s)"), *Seen), bCaught);
	TestTrue(*FString::Printf(TEXT("...past them (%s)"), *Seen), Spider->GetActorLocation().X > 1300.0);
	TestTrue(TEXT("...never inside a fence"), !bThroughFence);
	TestTrue(*FString::Printf(TEXT("...going round, not pressing into them (%s)"), *Seen), Planner.GetWallFollows() >= 1);
	TestTrue(*FString::Printf(TEXT("...without ping-pong (%s)"), *Seen), Planner.GetReversals() <= 2 && Planner.GetSideFlips() <= 3);
	return true;
}

#endif
