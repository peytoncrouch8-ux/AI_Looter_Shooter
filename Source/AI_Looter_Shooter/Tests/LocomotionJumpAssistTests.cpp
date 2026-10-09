#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerLocomotionComponent.h"
#include "Player/TraversalRules.h"
#include "Tests/LocomotionTestWorld.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The jump's forgiveness played out in a test level on the real character (BP_LooterCharacter): coyote time off an edge,
// the buffer before landing, the movement's own step up lips, and a player wedged on a ridge nudged free.

using namespace LocomotionTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTraversalJumpAssistTest, "Looter.Locomotion.Traversal.JumpAssist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTraversalJumpAssistTest::RunTest(const FString& Parameters)
{
	// The jump forgives (the user's ask, 2026-10-08): walked off an edge, the key still jumps for a moment (coyote time);
	// pressed just before landing, it jumps on landing (the buffer). Neither stretches further: a late press is just late.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	UPlayerLocomotionComponent* Players[5] = {};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Players); ++Index)
	{
		Players[Index] = SpawnPlayer(World, FVector(0.0, 1000.0 * Index, 0.0));
	}
	if (!TestNotNull(TEXT("A floor"), Floor) || !Players[0] || !Players[1] || !Players[2] || !Players[3] || !Players[4])
	{
		AddError(TEXT("The players weren't made."));
		return false;
	}
	const auto WalkOff = [](UPlayerLocomotionComponent* Locomotion, int32 Frames)
	{
		// Walked off an edge: falling, with no lift.
		BodyOf(Locomotion)->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		for (int32 Index = 0; Index < Frames; ++Index)
		{
			Step(Locomotion);
		}
	};

	// 0.05 s off the edge: the key jumps, a full jump, and only once.
	UPlayerLocomotionComponent* Coyote = Players[0];
	UCharacterMovementComponent* CoyoteMovement = BodyOf(Coyote)->GetCharacterMovement();
	SettleStanding(Coyote);
	WalkOff(Coyote, 3);
	TestTrue(TEXT("Walked off an edge: the jump still counts"), Coyote->CanCoyoteJump());
	Coyote->HandleJumpPressed();
	TestTrue(FString::Printf(TEXT("...and jumps (%.0f cm/s up)"), CoyoteMovement->Velocity.Z),
		CoyoteMovement->IsFalling() && FMath::IsNearlyEqual(static_cast<float>(CoyoteMovement->Velocity.Z), CoyoteMovement->JumpZVelocity, 0.5f));
	TestTrue(TEXT("...once"), !Coyote->CanCoyoteJump() && BodyOf(Coyote)->JumpCurrentCount == 1);

	// 0.2 s off the edge: too late, no jump in the air; the press is kept, and landing 0.05 s later jumps.
	UPlayerLocomotionComponent* Buffered = Players[1];
	ACharacter* BufferedBody = BodyOf(Buffered);
	SettleStanding(Buffered);
	WalkOff(Buffered, 12);
	TestFalse(TEXT("0.2 s off the edge: too late"), Buffered->CanCoyoteJump());
	Buffered->HandleJumpPressed();
	TestTrue(TEXT("...no jump in the air"), !BufferedBody->bPressedJump && BufferedBody->GetCharacterMovement()->Velocity.Z < 1.0);
	WalkOff(Buffered, 3);
	BufferedBody->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Step(Buffered);
	TestTrue(TEXT("Pressed just before landing: it jumps on landing"), BufferedBody->bPressedJump);

	// Pressed 0.3 s before landing: nothing on landing.
	UPlayerLocomotionComponent* Early = Players[2];
	ACharacter* EarlyBody = BodyOf(Early);
	SettleStanding(Early);
	WalkOff(Early, 12);
	Early->HandleJumpPressed();
	WalkOff(Early, 18);
	EarlyBody->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Step(Early);
	TestFalse(TEXT("Pressed 0.3 s before landing: no jump on landing"), EarlyBody->bPressedJump);

	// Left the ground by jumping: no second jump from the edge's grace.
	UPlayerLocomotionComponent* Jumper = Players[3];
	ACharacter* JumperBody = BodyOf(Jumper);
	SettleStanding(Jumper);
	Jumper->HandleJumpPressed();
	JumperBody->CheckJumpInput(Frame);
	Step(Jumper);
	TestTrue(TEXT("Jumped"), JumperBody->GetCharacterMovement()->IsFalling());
	TestFalse(TEXT("...no coyote time after a jump"), Jumper->CanCoyoteJump());

	// Walking up a lip under the step's height is the movement's own step, not a climb: the step reaches past the
	// smaller body's knee (about a quarter of its height) and the climb starts just above it.
	const UCharacterMovementComponent* Movement = BodyOf(Players[4])->GetCharacterMovement();
	const float Height = BodyOf(Players[4])->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f;
	TestTrue(FString::Printf(TEXT("The movement steps up lips to %.0f cm (%.0f%% of the body's %.0f)"), Movement->MaxStepHeight,
		100.f * Movement->MaxStepHeight / Height, Height), Movement->MaxStepHeight >= 0.2f * Height && Movement->MaxStepHeight <= 0.3f * Height);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTraversalUnstickTest, "Looter.Locomotion.Traversal.Unstick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTraversalUnstickTest::RunTest(const FString& Parameters)
{
	// A player hung on a steep ridge (neither falling nor walking) is nudged off it, after a moment, to a free spot with
	// ground under it, with a short glide rather than a pop.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	// A ridge: a 40 cm beam turned 45 degrees on its long side, its edge up, 57 cm over the floor (sized before it has a
	// mesh, as SpawnBlock does, so its collision is made at that size).
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	AStaticMeshActor* Ridge = Cube ? World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(),
		FTransform(FRotator(45.0, 0.0, 0.0), FVector(0.0, 0.0, 28.3), FVector(0.4, 4.0, 0.4))) : nullptr;
	if (Ridge)
	{
		Ridge->GetStaticMeshComponent()->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Ridge->GetStaticMeshComponent()->SetStaticMesh(Cube);
	}
	UPlayerLocomotionComponent* Hung = SpawnPlayer(World, FVector(0.0, 0.0, 200.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("A ridge"), Ridge) || !TestNotNull(TEXT("A player"), Hung))
	{
		return false;
	}
	// Set down on the ridge's edge, hung there in the air (the test level never moves it: as wedged as it gets).
	ACharacter* Body = BodyOf(Hung);
	const UCapsuleComponent* Capsule = Body->GetCapsuleComponent();
	FHitResult Rest;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UnstickTest), false, Body);
	const FCollisionShape Shape = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
	if (!TestTrue(TEXT("The ridge is under the player"), World->SweepSingleByChannel(Rest, FVector(0.0, 0.0, 300.0), FVector(0.0, 0.0, 0.0),
		FQuat::Identity, ECC_Pawn, Shape, Params)) || !TestTrue(TEXT("...its edge"), Rest.GetActor() == Ridge))
	{
		return false;
	}
	Body->SetActorLocation(Rest.Location + FVector(0.0, 0.0, 0.5));
	SettleStanding(Hung);
	UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
	Movement->SetMovementMode(MOVE_Falling);
	Movement->Velocity = FVector::ZeroVector;
	const FVector Wedged = Body->GetActorLocation();

	FViewTrack Track;
	Track.Sample(Hung);
	int32 Frames = 0;
	while (!Hung->IsTraversing() && Frames < 60)
	{
		Step(Hung);
		Track.Sample(Hung);
		++Frames;
	}
	if (!TestTrue(FString::Printf(TEXT("Hung on the ridge: nudged free (after %.2f s)"), Frames * Frame), Hung->IsTraversing()
		&& Hung->GetTraversalKind() == ETraversalKind::Unstick))
	{
		return false;
	}
	TestTrue(TEXT("...not at once (a moment's grace)"), Frames * Frame >= LooterTraversal::StuckSeconds - Frame);
	float Pose = 0.f;
	float Slowest = 0.f;
	RunTraversalOut(Hung, Track, Pose, Slowest);
	const FVector Free = Body->GetActorLocation();
	TestTrue(FString::Printf(TEXT("...moved off it (%.0f cm)"), FVector::Dist2D(Free, Wedged)), FVector::Dist2D(Free, Wedged) >= 25.0);
	TestTrue(TEXT("...falling the last little way"), Movement->IsFalling());
	FHitResult Ground;
	TestTrue(TEXT("...over ground it can stand on"), World->SweepSingleByChannel(Ground, Free, Free - FVector(0.0, 0.0, 300.0), FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeCapsule(Shape.GetCapsuleRadius() - 0.5f, Shape.GetCapsuleHalfHeight()), Params) && Movement->IsWalkable(Ground));
	// The glide's view: the eye goes along with it, never popping (10 cm a frame, as through a climb).
	TestTrue(FString::Printf(TEXT("...gliding there, the view never popping (%.2f cm a frame at most)"), Track.MaxStep), Track.MaxStep <= 10.f);
	return true;
}

#endif
