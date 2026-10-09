#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerTraversal.h"
#include "Player/TraversalProbe.h"
#include "Player/TraversalRules.h"
#include "Tests/LocomotionTestWorld.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/AmmoPickup.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// Mantles and vaults played out in a test level on the real character (BP_LooterCharacter): up onto a 1.2 m box and a
// ledge caught in the air, never a 2 m wall; over a rail at a sprint keeping the run; never onto a creature, loot or a
// NoClimb prop. The view is followed every frame (no pop, no jolt). The rules and plans alone:
// LocomotionTraversalTests.cpp; the jump's forgiveness and the unstick: LocomotionJumpAssistTests.cpp.

using namespace LocomotionTestWorld;

namespace
{
	/** The view's limits at 60 fps through a move (the plan's own, LocomotionTraversalTests.cpp): steps to 10 cm, changes to 1.5. */
	constexpr float EyeStepLimit = 10.f;
	constexpr float EyeStepChangeLimit = 1.5f;

	float RadiusOf(const UPlayerLocomotionComponent* Locomotion)
	{
		return BodyOf(Locomotion)->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}

	/** The view through a move and the eye's settling after it: no pop, no jolt, and the eye back at standing. */
	void CheckView(FAutomationTestBase& Test, const TCHAR* Name, UPlayerLocomotionComponent* Locomotion, FViewTrack& Track, float Standing)
	{
		for (int32 Frames = 0; Frames < 30; ++Frames)
		{
			PlayFrame(Locomotion);
			Track.Sample(Locomotion);
		}
		Test.TestTrue(FString::Printf(TEXT("%s: no pop (at most %.2f cm in a frame, frame %d)"), Name, Track.MaxStep, Track.WorstStepFrame),
			Track.MaxStep <= EyeStepLimit);
		Test.TestTrue(FString::Printf(TEXT("%s: no jolt (a frame's move changed by at most %.2f cm, frame %d)"), Name, Track.MaxStepChange,
			Track.WorstChangeFrame), Track.MaxStepChange <= EyeStepChangeLimit);
		const float Eye = EyeAboveFeet(Locomotion);
		Test.TestTrue(FString::Printf(TEXT("%s: the eye rises back to standing (%.2f cm over the feet, wants %.2f)"), Name, Eye, Standing),
			FMath::IsNearlyEqual(Eye, Standing, 0.5f));
	}

	/** In the air, Lift over where it stands, falling at FallSpeed and pushing forward (a jump at a ledge, caught). */
	void PutInTheAir(UPlayerLocomotionComponent* Locomotion, float Lift, float FallSpeed)
	{
		ACharacter* Body = BodyOf(Locomotion);
		Body->SetActorLocation(Body->GetActorLocation() + FVector(0.0, 0.0, Lift));
		UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
		Movement->SetMovementMode(MOVE_Falling);
		Movement->Velocity = FVector(0.0, 0.0, -FallSpeed);
		Locomotion->HandleMoveInput(FVector2D(0.0, 1.0));
	}

	/**
	 * One frame of a fall as the game plays it: the movement carries the body on (gravity on its velocity, the move at the
	 * frame's mean speed, as the movement's falling does), then the locomotion ticks. The test level never moves a falling
	 * body by itself, so without this a fall would stand still until the catch and the view would seem to jolt into it.
	 */
	void FallFrame(UPlayerLocomotionComponent* Locomotion)
	{
		ACharacter* Body = BodyOf(Locomotion);
		UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
		const FVector Before = Movement->Velocity;
		Movement->Velocity.Z += Movement->GetGravityZ() * Frame;
		Body->SetActorLocation(Body->GetActorLocation() + 0.5 * (Before + Movement->Velocity) * Frame);
		Step(Locomotion);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTraversalMantleTest, "Looter.Locomotion.Traversal.Mantle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTraversalMantleTest::RunTest(const FString& Parameters)
{
	// The user's ask (2026-10-08): jump at a chest-high ledge (to about 1.5 m) and the player climbs onto it, smoothly and
	// quickly; one met in the air is caught. A 2 m wall is out of reach, from the ground or from a jump.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	UPlayerLocomotionComponent* Climber = SpawnPlayer(World, FVector::ZeroVector);
	UPlayerLocomotionComponent* Short = SpawnPlayer(World, FVector(0.0, 1000.0, 0.0));
	UPlayerLocomotionComponent* Catcher = SpawnPlayer(World, FVector(0.0, 2000.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !Climber || !Short || !Catcher || !CameraOf(Climber))
	{
		AddError(TEXT("The players weren't made."));
		return false;
	}
	// Each wall's face 10 cm in front of its player's body, wide and deep enough to stand on.
	const float Radius = RadiusOf(Climber);
	const double Face = Radius + 10.0;
	SpawnBlock(World, FVector(Face + 100.0, 0.0, 60.0), FVector(200.0, 300.0, 120.0));
	SpawnBlock(World, FVector(Face + 100.0, 1000.0, 100.0), FVector(200.0, 300.0, 200.0));
	SpawnBlock(World, FVector(Face + 100.0, 2000.0, 70.0), FVector(200.0, 300.0, 140.0));

	// 1.2 m, from a stand: a quick, smooth climb onto the top, the gun lowered on the way, standing and walking at the end.
	ACharacter* ClimberBody = BodyOf(Climber);
	const float Standing = SettleStanding(Climber);
	FViewTrack Track;
	Track.Sample(Climber);
	Climber->HandleJumpPressed();
	if (TestTrue(FString::Printf(TEXT("Jump at a 1.2 m box: a mantle (refused: %s)"), FTraversalProbe::RefusalName(Climber->GetLastTraversalRefusal())),
		Climber->IsTraversing() && Climber->GetTraversalKind() == ETraversalKind::Mantle))
	{
		float Pose = 0.f;
		float Slowest = 0.f;
		const float Time = RunTraversalOut(Climber, Track, Pose, Slowest);
		TestTrue(FString::Printf(TEXT("...quick (%.2f s)"), Time), Time >= 0.3f - Frame && Time <= 0.5f + Frame);
		TestTrue(FString::Printf(TEXT("...the gun lowered on the way (pose %.2f)"), Pose), Pose >= 0.8f);
		TestTrue(FString::Printf(TEXT("...on the top (feet %.2f cm up)"), FeetHeight(Climber)), FMath::IsNearlyEqual(FeetHeight(Climber), 120.f + WalkingFloorGap(), 1.f));
		TestTrue(FString::Printf(TEXT("...the body over it (%.1f cm past the face)"), ClimberBody->GetActorLocation().X - Face),
			ClimberBody->GetActorLocation().X >= Face + Radius);
		TestTrue(TEXT("...walking, standing"), ClimberBody->GetCharacterMovement()->IsMovingOnGround() && !ClimberBody->bIsCrouched);
		CheckView(*this, TEXT("Mantle onto 1.2 m"), Climber, Track, Standing);
	}

	// 2 m: out of reach from the ground (the key jumps instead)...
	ACharacter* ShortBody = BodyOf(Short);
	SettleStanding(Short);
	Short->HandleJumpPressed();
	TestFalse(TEXT("A 2 m wall: no mantle"), Short->IsTraversing());
	TestTrue(FString::Printf(TEXT("...out of reach (%s)"), FTraversalProbe::RefusalName(Short->GetLastTraversalRefusal())),
		Short->GetLastTraversalRefusal() == ETraversalRefusal::TooHigh);
	TestTrue(TEXT("...the key jumps"), ShortBody->bPressedJump);
	ShortBody->StopJumping();
	// ...and from a jump: a metre up beside it, pushing at it, it isn't caught (the reach counts from the ground left).
	PutInTheAir(Short, 100.f, 0.f);
	Step(Short);
	TestFalse(TEXT("...nor caught from a jump"), Short->IsTraversing());
	TestTrue(FString::Printf(TEXT("...still out of reach (%s)"), FTraversalProbe::RefusalName(Short->GetLastTraversalRefusal())),
		Short->GetLastTraversalRefusal() == ETraversalRefusal::TooHigh);

	// 1.4 m, met in the air 60 cm up while falling, pushing on: caught and climbed, the view going on from the fall
	// without a jolt.
	const float CatcherStanding = SettleStanding(Catcher);
	PutInTheAir(Catcher, 60.f, 150.f);
	FViewTrack CatchTrack;
	CatchTrack.Sample(Catcher);
	FallFrame(Catcher);
	CatchTrack.Sample(Catcher);
	if (TestTrue(FString::Printf(TEXT("A 1.4 m ledge met in the air: caught (refused: %s)"), FTraversalProbe::RefusalName(Catcher->GetLastTraversalRefusal())),
		Catcher->IsTraversing() && Catcher->GetTraversalKind() == ETraversalKind::Mantle))
	{
		float Pose = 0.f;
		float Slowest = 0.f;
		RunTraversalOut(Catcher, CatchTrack, Pose, Slowest);
		TestTrue(FString::Printf(TEXT("...and climbed (feet %.2f cm up)"), FeetHeight(Catcher)), FMath::IsNearlyEqual(FeetHeight(Catcher), 140.f + WalkingFloorGap(), 1.f));
		CheckView(*this, TEXT("Caught at 1.4 m"), Catcher, CatchTrack, CatcherStanding);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTraversalVaultTest, "Looter.Locomotion.Traversal.Vault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTraversalVaultTest::RunTest(const FString& Parameters)
{
	// The user's ask (2026-10-08): a fence rail or a low wall (to about 1.1 m) taken at a sprint is vaulted in one fluid
	// motion that keeps the run. At a walk a low wall wide enough to stand on is climbed onto instead; a rail with nothing
	// to stand on is hopped over.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	UPlayerLocomotionComponent* Runner = SpawnPlayer(World, FVector::ZeroVector);
	UPlayerLocomotionComponent* Walker = SpawnPlayer(World, FVector(0.0, 1500.0, 0.0));
	UPlayerLocomotionComponent* Stander = SpawnPlayer(World, FVector(0.0, 3000.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !Runner || !Walker || !Stander)
	{
		AddError(TEXT("The players weren't made."));
		return false;
	}
	const float Radius = RadiusOf(Runner);
	// A 1 m rail 10 cm thick 60 cm in front of the runner; a 1 m wall 60 cm deep 10 cm in front of the walker; the same
	// rail 10 cm in front of the one standing.
	const double RailFace = Radius + 60.0;
	SpawnBlock(World, FVector(RailFace + 5.0, 0.0, 50.0), FVector(10.0, 400.0, 100.0));
	const double WallFace = Radius + 10.0;
	SpawnBlock(World, FVector(WallFace + 30.0, 1500.0, 50.0), FVector(60.0, 400.0, 100.0));
	SpawnBlock(World, FVector(WallFace + 5.0, 3000.0, 50.0), FVector(10.0, 400.0, 100.0));

	// At a sprint, forward held: over the rail in one hop, down on the floor beyond, the run kept through it.
	ACharacter* RunnerBody = BodyOf(Runner);
	UCharacterMovementComponent* RunnerMovement = RunnerBody->GetCharacterMovement();
	const float Standing = SettleStanding(Runner);
	Runner->HandleMoveInput(FVector2D(0.0, 1.0));
	StartSprint(Runner);
	const float Sprint = RunnerMovement->MaxWalkSpeed;
	TestTrue(FString::Printf(TEXT("Sprinting (%.0f cm/s)"), Sprint), FMath::IsNearlyEqual(static_cast<float>(RunnerMovement->Velocity.Size2D()), Sprint, 1.f)
		&& Sprint > LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale * 1.2f);
	FViewTrack Track;
	Track.Sample(Runner);
	Runner->HandleJumpPressed();
	if (TestTrue(FString::Printf(TEXT("Jump at a 1 m rail at a sprint: a vault (refused: %s)"), FTraversalProbe::RefusalName(Runner->GetLastTraversalRefusal())),
		Runner->IsTraversing() && Runner->GetTraversalKind() == ETraversalKind::Vault))
	{
		float Pose = 0.f;
		float Slowest = 0.f;
		const float Time = RunTraversalOut(Runner, Track, Pose, Slowest);
		const float Exit = static_cast<float>(RunnerMovement->Velocity.Size2D());
		TestTrue(FString::Printf(TEXT("...one fluid hop (%.2f s)"), Time), Time <= 0.55f);
		TestTrue(FString::Printf(TEXT("...over it (%.1f cm past the rail)"), RunnerBody->GetActorLocation().X - (RailFace + 10.0)),
			RunnerBody->GetActorLocation().X >= RailFace + 10.0 + Radius);
		TestTrue(FString::Printf(TEXT("...down on the floor beyond (feet %.2f)"), FeetHeight(Runner)), FMath::IsNearlyEqual(FeetHeight(Runner), WalkingFloorGap(), 1.f));
		TestTrue(TEXT("...walking on"), RunnerMovement->IsMovingOnGround());
		TestTrue(FString::Printf(TEXT("...the run kept out of it (%.0f of %.0f cm/s)"), Exit, Sprint), Exit >= Sprint * 0.95f);
		TestTrue(FString::Printf(TEXT("...and through it (%.0f cm/s at the slowest)"), Slowest), Slowest >= Sprint * 0.7f);
		CheckView(*this, TEXT("Vault at a sprint"), Runner, Track, Standing);
	}

	// At a walk, a low wall wide enough to stand on: climbed onto, not vaulted.
	SettleStanding(Walker);
	Walker->HandleMoveInput(FVector2D(0.0, 1.0));
	Walker->HandleJumpPressed();
	if (TestTrue(FString::Printf(TEXT("At a walk, a 1 m wall 60 cm deep: climbed onto (refused: %s)"), FTraversalProbe::RefusalName(Walker->GetLastTraversalRefusal())),
		Walker->IsTraversing() && Walker->GetTraversalKind() == ETraversalKind::Mantle))
	{
		FViewTrack WalkerTrack;
		float Pose = 0.f;
		float Slowest = 0.f;
		RunTraversalOut(Walker, WalkerTrack, Pose, Slowest);
		TestTrue(FString::Printf(TEXT("...standing on it (feet %.2f)"), FeetHeight(Walker)), FMath::IsNearlyEqual(FeetHeight(Walker), 100.f + WalkingFloorGap(), 1.f));
	}

	// Standing still at a rail: hopped over (there's nothing to stand on), landing just past it.
	ACharacter* StanderBody = BodyOf(Stander);
	SettleStanding(Stander);
	Stander->HandleJumpPressed();
	if (TestTrue(FString::Printf(TEXT("Standing at a 1 m rail: hopped over (refused: %s)"), FTraversalProbe::RefusalName(Stander->GetLastTraversalRefusal())),
		Stander->IsTraversing() && Stander->GetTraversalKind() == ETraversalKind::Vault))
	{
		FViewTrack StanderTrack;
		float Pose = 0.f;
		float Slowest = 0.f;
		RunTraversalOut(Stander, StanderTrack, Pose, Slowest);
		TestTrue(TEXT("...past it, on the floor"), StanderBody->GetActorLocation().X >= WallFace + 10.0 + Radius
			&& FMath::IsNearlyEqual(FeetHeight(Stander), WalkingFloorGap(), 1.f));
		TestTrue(FString::Printf(TEXT("...the view smooth (%.2f cm a frame at most, changing by %.2f)"), StanderTrack.MaxStep, StanderTrack.MaxStepChange),
			StanderTrack.MaxStep <= EyeStepLimit && StanderTrack.MaxStepChange <= EyeStepChangeLimit);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTraversalNeverClimbsTest, "Looter.Locomotion.Traversal.NeverClimbs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTraversalNeverClimbsTest::RunTest(const FString& Parameters)
{
	// Never onto a creature, loot or anything tagged NoClimb (on the actor or its mesh); the same box without the tag is
	// climbed, so it's the tag that refuses.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	UPlayerLocomotionComponent* Players[4] = {};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Players); ++Index)
	{
		Players[Index] = SpawnPlayer(World, FVector(0.0, 1000.0 * Index, 0.0));
	}
	if (!TestNotNull(TEXT("A floor"), Floor) || !Players[0] || !Players[1] || !Players[2] || !Players[3])
	{
		AddError(TEXT("The players weren't made."));
		return false;
	}
	const float Radius = RadiusOf(Players[0]);
	const double Face = Radius + 10.0;

	// A spider in front (its capsule 1.24 m tall: in reach), a pawn like every creature.
	ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(FVector(Face + 62.0, 0.0, 63.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A spider"), Spider))
	{
		SettleStanding(Players[0]);
		Players[0]->HandleJumpPressed();
		TestFalse(TEXT("A spider in front: no mantle onto it"), Players[0]->IsTraversing());
		const ETraversalRefusal Why = Players[0]->GetLastTraversalRefusal();
		if (Why == ETraversalRefusal::NoWall)
		{
			AddWarning(TEXT("The test level didn't put the spider's capsule in its queries, so only the outcome was checked."));
		}
		else
		{
			TestTrue(FString::Printf(TEXT("...a creature isn't climbed (%s)"), FTraversalProbe::RefusalName(Why)), Why == ETraversalRefusal::NotClimbable);
		}
	}

	// A 1 m box tagged NoClimb, one with the tag on its mesh, and one without.
	AStaticMeshActor* Tagged = SpawnBlock(World, FVector(Face + 100.0, 1000.0, 50.0), FVector(200.0, 300.0, 100.0));
	AStaticMeshActor* MeshTagged = SpawnBlock(World, FVector(Face + 100.0, 2000.0, 50.0), FVector(200.0, 300.0, 100.0));
	AStaticMeshActor* Plain = SpawnBlock(World, FVector(Face + 100.0, 3000.0, 50.0), FVector(200.0, 300.0, 100.0));
	if (TestNotNull(TEXT("The boxes"), Tagged) && MeshTagged && Plain)
	{
		Tagged->Tags.Add(LooterTraversal::NoClimbTag());
		MeshTagged->GetStaticMeshComponent()->ComponentTags.Add(LooterTraversal::NoClimbTag());
		const TCHAR* Names[] = { TEXT("A box tagged NoClimb"), TEXT("A box whose mesh is tagged NoClimb") };
		for (int32 Index = 1; Index <= 2; ++Index)
		{
			SettleStanding(Players[Index]);
			Players[Index]->HandleJumpPressed();
			TestFalse(FString::Printf(TEXT("%s: not climbed"), Names[Index - 1]), Players[Index]->IsTraversing());
			TestTrue(FString::Printf(TEXT("%s: ...for the tag (%s)"), Names[Index - 1], FTraversalProbe::RefusalName(Players[Index]->GetLastTraversalRefusal())),
				Players[Index]->GetLastTraversalRefusal() == ETraversalRefusal::NotClimbable);
		}
		SettleStanding(Players[3]);
		Players[3]->HandleJumpPressed();
		TestTrue(FString::Printf(TEXT("The same box untagged: climbed (refused: %s)"), FTraversalProbe::RefusalName(Players[3]->GetLastTraversalRefusal())),
			Players[3]->IsTraversing());
	}

	// The rule itself, for what never blocks the player anyway: loot on the ground, a creature, a plain box.
	AAmmoPickup* Ammo = World->SpawnActor<AAmmoPickup>(FVector(0.0, -1000.0, 20.0), FRotator::ZeroRotator);
	APawn* Climber = BodyOf(Players[0]);
	if (TestNotNull(TEXT("Some loot"), Ammo))
	{
		const FHitResult LootHit(Ammo, Cast<UPrimitiveComponent>(Ammo->GetRootComponent()), Ammo->GetActorLocation(), FVector::UpVector);
		TestFalse(TEXT("Loot is never climbed"), FTraversalProbe::IsClimbable(LootHit, Climber));
	}
	if (Spider)
	{
		const FHitResult CreatureHit(Spider, Spider->GetCapsuleComponent(), Spider->GetActorLocation(), FVector::UpVector);
		TestFalse(TEXT("A creature is never climbed"), FTraversalProbe::IsClimbable(CreatureHit, Climber));
	}
	if (Plain)
	{
		const FHitResult BoxHit(Plain, Plain->GetStaticMeshComponent(), Plain->GetActorLocation(), FVector::UpVector);
		TestTrue(TEXT("A plain box is"), FTraversalProbe::IsClimbable(BoxHit, Climber));
	}
	return true;
}

#endif
