#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerSlide.h"
#include "Player/StanceIntent.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"
#include "UObject/UnrealType.h"

namespace
{
	using EStance = FStanceIntent::EStance;

	constexpr float Frame = 1.f / 60.f;

	FStanceIntent MakeIntent(bool bSprintToggle, bool bCrouchToggle)
	{
		FStanceIntent Intent;
		Intent.SetModes(bSprintToggle, bCrouchToggle);
		return Intent;
	}

	/** A component a Blueprint adds (its construction script's template), searched up the Blueprint parents. */
	template <typename T>
	const T* FindBlueprintComponent(const UClass* Class)
	{
		for (const UBlueprintGeneratedClass* Generated = Cast<UBlueprintGeneratedClass>(Class); Generated;
			Generated = Cast<UBlueprintGeneratedClass>(Generated->GetSuperClass()))
		{
			const USimpleConstructionScript* Script = Generated->SimpleConstructionScript;
			for (const USCS_Node* Node : Script ? Script->GetAllNodes() : TArray<USCS_Node*>())
			{
				if (const T* Template = Node ? Cast<T>(Node->ComponentTemplate) : nullptr)
				{
					return Template;
				}
			}
		}
		return nullptr;
	}

	const TCHAR* const PlayerClassPath = TEXT("/Game/Player/BP_LooterCharacter.BP_LooterCharacter_C");

	/** A floor or a ceiling for the world tests: the engine's cube stretched to Size (cm), blocking everything. */
	AStaticMeshActor* SpawnBlock(UWorld* World, const FVector& Center, const FVector& Size)
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		// Sized before it has a mesh, so its collision is made at that size, straight into the level's queries.
		AStaticMeshActor* Block = Cube ? World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(FQuat::Identity, Center, Size / 100.0)) : nullptr;
		if (Block)
		{
			UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
			Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Mesh->SetStaticMesh(Cube);
		}
		return Block;
	}

	/**
	 * The player (BP_LooterCharacter, at the player's size and speeds) standing on the floor at Feet, walking, facing +X;
	 * returns its locomotion component. No controller and no keys bound: the tests press them. The test level never
	 * ticks, so the movement only runs where a test runs it.
	 */
	UPlayerLocomotionComponent* SpawnPlayer(UWorld* World, const FVector& Feet)
	{
		UClass* Class = LoadClass<ACharacter>(nullptr, PlayerClassPath);
		const ACharacter* Defaults = Class ? Class->GetDefaultObject<ACharacter>() : nullptr;
		ACharacter* Player = Defaults ? World->SpawnActor<ACharacter>(Class, Feet + FVector(0.0, 0.0, Defaults->GetDefaultHalfHeight() + 1.0), FRotator::ZeroRotator) : nullptr;
		if (!Player)
		{
			return nullptr;
		}
		// A movement component only takes the capsule it moves in a game level (UMovementComponent::OnRegister), and the
		// test level is an editor preview: without it, it can't crouch, walk or read the movement keys.
		UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
		Movement->SetUpdatedComponent(Player->GetCapsuleComponent());
		Player->DispatchBeginPlay();
		// Falling first, so walking starts afresh: it finds the floor, and crouching then keeps the feet on it.
		Movement->SetMovementMode(MOVE_Falling);
		Movement->SetMovementMode(MOVE_Walking);
		return Player->FindComponentByClass<UPlayerLocomotionComponent>();
	}

	ACharacter* WalkerOf(const UPlayerLocomotionComponent* Locomotion)
	{
		return CastChecked<ACharacter>(Locomotion->GetOwner());
	}

	/** One frame of the locomotion component. */
	void Step(UPlayerLocomotionComponent* Locomotion)
	{
		static_cast<UActorComponent*>(Locomotion)->TickComponent(Frame, LEVELTICK_All, nullptr);
	}

	/** Pushing forward at Speed: the keys as the last move read them, and the velocity they made. */
	void MoveForward(UPlayerLocomotionComponent* Locomotion, float Speed)
	{
		ACharacter* Walker = WalkerOf(Locomotion);
		UCharacterMovementComponent* Movement = Walker->GetCharacterMovement();
		Walker->AddMovementInput(Walker->GetActorForwardVector(), 1.f, true);
		Movement->ConsumeInputVector();
		Movement->Velocity = Walker->GetActorForwardVector() * Speed;
	}

	/** The sprint key held, running forward at sprint speed, and a frame for the stance to take it. */
	void StartSprint(UPlayerLocomotionComponent* Locomotion)
	{
		const UCharacterMovementComponent* Movement = WalkerOf(Locomotion)->GetCharacterMovement();
		Locomotion->HandleSprintPressed();
		MoveForward(Locomotion, Movement->MaxWalkSpeed * Locomotion->SprintSpeedMultiplier);
		Step(Locomotion);
	}

	/** The crouch key held and the capsule crouched (what the next move would do). */
	void CrouchDown(UPlayerLocomotionComponent* Locomotion)
	{
		Locomotion->HandleCrouchPressed();
		Step(Locomotion);
		WalkerOf(Locomotion)->GetCharacterMovement()->Crouch(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceHoldModeTest, "Looter.Locomotion.Stance.HoldMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceHoldModeTest::RunTest(const FString& Parameters)
{
	FStanceIntent Intent = MakeIntent(false, false);
	TestFalse(TEXT("Nothing wanted at rest"), Intent.WantsSprint() || Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Holding sprint sprints"), Intent.WantsSprint());
	Intent.Release(EStance::Sprint);
	TestFalse(TEXT("Letting go stops"), Intent.WantsSprint());

	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("Holding crouch crouches"), Intent.WantsCrouch());
	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Sprint pressed while crouching wins"), Intent.WantsSprint());
	TestFalse(TEXT("...and stands up"), Intent.WantsCrouch());
	Intent.Release(EStance::Sprint);
	TestTrue(TEXT("Crouch still held takes over again"), Intent.WantsCrouch());
	Intent.Release(EStance::Crouch);
	TestFalse(TEXT("All released"), Intent.WantsSprint() || Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	Intent.CancelSprintToggle();
	TestTrue(TEXT("Shooting doesn't erase a held sprint key (it resumes after)"), Intent.WantsSprint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceToggleModeTest, "Looter.Locomotion.Stance.ToggleMode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceToggleModeTest::RunTest(const FString& Parameters)
{
	FStanceIntent Intent = MakeIntent(true, true);

	Intent.Press(EStance::Sprint);
	Intent.Release(EStance::Sprint);
	TestTrue(TEXT("Toggle sprint stays on after release"), Intent.WantsSprint());
	Intent.Press(EStance::Sprint);
	TestFalse(TEXT("Second press turns it off"), Intent.WantsSprint());

	Intent.Press(EStance::Crouch);
	Intent.Release(EStance::Crouch);
	TestTrue(TEXT("Toggle crouch stays on"), Intent.WantsCrouch());
	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Sprint from a toggled crouch"), Intent.WantsSprint());
	TestFalse(TEXT("clears the crouch"), Intent.WantsCrouch());
	Intent.CancelSprintToggle();
	TestFalse(TEXT("Stopping/shooting ends a toggled sprint"), Intent.WantsSprint());
	TestFalse(TEXT("and the old crouch doesn't come back"), Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("Crouch from a toggled sprint"), Intent.WantsCrouch());
	TestFalse(TEXT("ends the sprint"), Intent.WantsSprint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceMixedModeTest, "Looter.Locomotion.Stance.MixedModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceMixedModeTest::RunTest(const FString& Parameters)
{
	// Toggle sprint, hold crouch.
	FStanceIntent Intent = MakeIntent(true, false);
	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("Held crouch overrides toggled sprint"), Intent.WantsCrouch());
	TestFalse(TEXT("Sprint not wanted while crouch held"), Intent.WantsSprint());
	Intent.Release(EStance::Crouch);
	TestFalse(TEXT("Toggled sprint was cleared by the crouch"), Intent.WantsSprint());

	// Hold sprint, toggle crouch.
	Intent = MakeIntent(false, true);
	Intent.Press(EStance::Crouch);
	Intent.Press(EStance::Sprint);
	TestTrue(TEXT("Held sprint overrides toggled crouch"), Intent.WantsSprint());
	Intent.Release(EStance::Sprint);
	TestFalse(TEXT("Toggled crouch was cleared by the sprint"), Intent.WantsCrouch());

	Intent.Press(EStance::Sprint);
	Intent.Reset();
	TestFalse(TEXT("Reset (lost control) clears everything"), Intent.WantsSprint() || Intent.WantsCrouch());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionDefaultsTest, "Looter.Locomotion.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLocomotionDefaultsTest::RunTest(const FString& Parameters)
{
	const UPlayerLocomotionComponent* Defaults = GetDefault<UPlayerLocomotionComponent>();
	TestTrue(TEXT("Sprint is faster than walking"), Defaults->SprintSpeedMultiplier > 1.f);
	TestTrue(TEXT("Crouch is slower than walking"), Defaults->CrouchSpeed < LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale);
	TestTrue(TEXT("Crouched capsule is shorter than standing (96)"), Defaults->CrouchedHalfHeight < 96.f);
	TestTrue(TEXT("Crouching tightens spread"), Defaults->CrouchSpreadMultiplier < 1.f);
	TestTrue(TEXT("Crouched head stays under the crouched capsule top"), Defaults->GetCrouchedHeadHeight() < Defaults->CrouchedHalfHeight * 2.f);
	TestEqual(TEXT("Standing spread is untouched"), Defaults->GetSpreadMultiplier(), 1.f);

	// The player jumps 15% higher than the engine's default jump (420 cm/s under 980 cm/s^2 gravity: 90 cm).
	const UClass* Player = LoadClass<ACharacter>(nullptr, TEXT("/Game/Player/BP_LooterCharacter.BP_LooterCharacter_C"));
	const ACharacter* Character = Player ? Player->GetDefaultObject<ACharacter>() : nullptr;
	if (TestNotNull(TEXT("Player character"), Character))
	{
		const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		const float Gravity = FMath::Abs(UPhysicsSettings::Get()->DefaultGravityZ) * Movement->GravityScale;
		TestEqual(TEXT("Jump height (cm)"), FMath::Square(Movement->JumpZVelocity) / (2.f * Gravity), 90.f * 1.15f, 0.5f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerSizeTest, "Looter.Locomotion.PlayerSize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerSizeTest::RunTest(const FString& Parameters)
{
	// The user's call (2026-10-07): the player 15% smaller and 15% slower, the two together so his strides still cover the
	// ground the animations' do.
	TestEqual(TEXT("Size and speed scale together"), LooterPlayerSize::SpeedScale, LooterPlayerSize::Scale);
	const UClass* Player = LoadClass<ACharacter>(nullptr, TEXT("/Game/Player/BP_LooterCharacter.BP_LooterCharacter_C"));
	const ACharacter* Character = Player ? Player->GetDefaultObject<ACharacter>() : nullptr;
	if (!TestNotNull(TEXT("Player character"), Character))
	{
		return false;
	}
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	TestTrue(FString::Printf(TEXT("The whole character is scaled: its capsule, the root (%s)"), *Capsule->GetRelativeScale3D().ToCompactString()),
		Capsule->GetRelativeScale3D().Equals(FVector(LooterPlayerSize::Scale), 1.e-4));
	TestEqual(TEXT("...its standing half height at that size (landings, respawns)"), Character->GetDefaultHalfHeight(),
		Capsule->GetUnscaledCapsuleHalfHeight() * LooterPlayerSize::Scale, 0.01f);
	TestEqual(TEXT("...its eyes at that size"), Character->BaseEyeHeight, GetDefault<APawn>()->BaseEyeHeight * LooterPlayerSize::Scale, 0.01f);

	const float Walk = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale;
	TestEqual(TEXT("Walks at 85% of the full-size 600 cm/s"), Character->GetCharacterMovement()->MaxWalkSpeed, Walk, 0.01f);
	if (const UPlayerLocomotionComponent* Locomotion = FindBlueprintComponent<UPlayerLocomotionComponent>(Player);
		TestNotNull(TEXT("The Blueprint's locomotion component"), Locomotion))
	{
		TestEqual(TEXT("Sprints at 85% of the full-size 930 cm/s"), Walk * Locomotion->SprintSpeedMultiplier, 930.f * LooterPlayerSize::SpeedScale, 0.5f);
		TestEqual(TEXT("Crouches at 85% of the full-size 300 cm/s"), Locomotion->CrouchSpeed,
			LooterPlayerSize::FullSizeCrouchSpeed * LooterPlayerSize::SpeedScale, 0.01f);
	}

	// The body's Anim Blueprints hand their blend spaces the speed in the body's own size (ULooterCharacterAnimInstance),
	// through their ground speed variable: without it the smaller body's feet would slide.
	const TCHAR* AnimBlueprints[] = {
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"),
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/ABP_Rifle.ABP_Rifle_C"),
	};
	for (const TCHAR* Path : AnimBlueprints)
	{
		const UClass* Anim = LoadClass<UAnimInstance>(nullptr, Path);
		const FNumericProperty* Speed = Anim ? CastField<FNumericProperty>(Anim->FindPropertyByName(TEXT("GroundSpeed"))) : nullptr;
		TestTrue(FString::Printf(TEXT("%s has a float GroundSpeed"), Path), Speed && Speed->IsFloatingPoint());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStanceJumpStandsUpTest, "Looter.Locomotion.Stance.JumpStandsUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStanceJumpStandsUpTest::RunTest(const FString& Parameters)
{
	// Hold: jump stands up with the crouch key still down; letting go changes nothing, and the next press crouches.
	FStanceIntent Intent = MakeIntent(false, false);
	Intent.Press(EStance::Crouch);
	Intent.StandUp();
	TestFalse(TEXT("Hold: jump stands up with the crouch key still down"), Intent.WantsCrouch());
	Intent.Release(EStance::Crouch);
	TestFalse(TEXT("...letting go of it leaves them standing"), Intent.WantsCrouch() || Intent.WantsSprint());
	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("...and the next press crouches again"), Intent.WantsCrouch());

	// Toggle: jump undoes the toggle, and a single press of crouch crouches again.
	Intent = MakeIntent(false, true);
	Intent.Press(EStance::Crouch);
	Intent.Release(EStance::Crouch);
	Intent.StandUp();
	TestFalse(TEXT("Toggle: jump stands up"), Intent.WantsCrouch());
	Intent.Press(EStance::Crouch);
	TestTrue(TEXT("...one press of crouch crouches again"), Intent.WantsCrouch());

	// A sprint key held under the crouch takes over once up.
	Intent = MakeIntent(false, false);
	Intent.Press(EStance::Sprint);
	Intent.Press(EStance::Crouch);
	Intent.StandUp();
	TestTrue(TEXT("Standing up with sprint held sprints"), Intent.WantsSprint() && !Intent.WantsCrouch());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJumpWhileCrouchedTest, "Looter.Locomotion.JumpWhileCrouched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FJumpWhileCrouchedTest::RunTest(const FString& Parameters)
{
	// The user's rule: crouched, the jump key stands the player up; a second press jumps. Under something low there's no
	// room to stand, and the press does nothing at all.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	// A level never readied for play drops the actors' own events (AActor::ProcessEvent), and whether a character can
	// jump is one (ACharacter::CanJumpInternal): let them run, as in the game.
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(4000.0, 4000.0, 100.0));
	UPlayerLocomotionComponent* Open = SpawnPlayer(World, FVector::ZeroVector);
	UPlayerLocomotionComponent* Low = SpawnPlayer(World, FVector(800.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !TestNotNull(TEXT("A walker in the open"), Open) || !TestNotNull(TEXT("A walker under a ledge"), Low))
	{
		return false;
	}

	ACharacter* Walker = WalkerOf(Open);
	UCharacterMovementComponent* Movement = Walker->GetCharacterMovement();
	CrouchDown(Open);
	TestTrue(TEXT("Crouched"), Walker->bIsCrouched);
	Open->HandleJumpPressed();
	TestFalse(TEXT("Jump while crouched stands up"), Walker->bIsCrouched);
	TestFalse(TEXT("...without jumping"), Walker->bPressedJump || Movement->IsFalling());
	Step(Open);
	TestFalse(TEXT("...and stays up with the crouch key still held"), Movement->bWantsToCrouch);
	Open->HandleJumpPressed();
	Walker->CheckJumpInput(Frame);
	TestTrue(TEXT("The next press jumps"), Movement->IsFalling() && Movement->Velocity.Z > 0.0);

	ACharacter* Under = WalkerOf(Low);
	UCharacterMovementComponent* UnderMovement = Under->GetCharacterMovement();
	CrouchDown(Low);
	// Its underside 150 cm up: over the player's crouched capsule (115.6 cm at 0.85), under the standing one (163.2).
	const AStaticMeshActor* Ledge = SpawnBlock(World, FVector(800.0, 0.0, 200.0), FVector(300.0, 300.0, 100.0));
	if (!TestNotNull(TEXT("A ledge"), Ledge) || !TestTrue(TEXT("Crouched under it"), Under->bIsCrouched))
	{
		return false;
	}
	Low->HandleJumpPressed();
	TestTrue(TEXT("No room to stand: still crouched"), Under->bIsCrouched && UnderMovement->bWantsToCrouch);
	TestFalse(TEXT("...and no jump"), Under->bPressedJump || UnderMovement->IsFalling());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerSlideRulesTest, "Looter.Locomotion.Slide.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerSlideRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A sprint on the ground slides"), FPlayerSlide::CanStart(true, true));
	TestFalse(TEXT("A crouch without a sprint doesn't"), FPlayerSlide::CanStart(false, true));
	TestFalse(TEXT("No slide in the air"), FPlayerSlide::CanStart(true, false));

	// Going in at the player's sprint, across the way they face: the slide follows the run.
	const float Sprint = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale * GetDefault<UPlayerLocomotionComponent>()->SprintSpeedMultiplier;
	const float Crouched = LooterPlayerSize::FullSizeCrouchSpeed * LooterPlayerSize::SpeedScale;
	FPlayerSlide Slide;
	Slide.Start(FVector(0.0, Sprint, 0.0), FVector::ForwardVector, Sprint * FPlayerSlide::SpeedMultiplier, Crouched);
	TestEqual(TEXT("10% faster than the sprint going in"), Slide.GetStartSpeed(), Sprint * 1.1f, 0.01f);
	TestEqual(TEXT("...and holding it"), Slide.GetSpeed(), Sprint * 1.1f, 0.01f);
	TestTrue(TEXT("...along the run, not the facing"), Slide.GetDirection().Equals(FVector(0.0, 1.0, 0.0)));

	float Time = 0.f;
	bool bEasing = false;
	while (Slide.Advance(Frame, Slide.GetSpeed(), true) && Time < 5.f)
	{
		Time += Frame;
		bEasing |= Slide.GetSpeed() < Sprint * 1.1f;
	}
	Time += Frame;
	TestTrue(FString::Printf(TEXT("Ends on time (%.3f s)"), Time), Slide.GetLastEnd() == FPlayerSlide::EEnd::Time
		&& Time >= FPlayerSlide::Duration - 0.001f && Time <= FPlayerSlide::Duration + Frame + 0.001f);
	TestTrue(TEXT("...slowing toward the crouched walk at its end"), bEasing && Slide.GetSpeed() <= Sprint * 1.1f);
	TestFalse(TEXT("...and over"), Slide.IsActive());

	Slide.Start(FVector(Sprint, 0.0, 0.0), FVector::ForwardVector, Sprint * FPlayerSlide::SpeedMultiplier, Crouched);
	TestFalse(TEXT("Running into a wall ends it"), Slide.Advance(Frame, 20.f, true));
	TestTrue(TEXT("...as stalled"), Slide.GetLastEnd() == FPlayerSlide::EEnd::Stalled);
	Slide.Start(FVector(Sprint, 0.0, 0.0), FVector::ForwardVector, Sprint * FPlayerSlide::SpeedMultiplier, Crouched);
	TestFalse(TEXT("Leaving the ground ends it"), Slide.Advance(Frame, Sprint * 1.1f, false));
	TestTrue(TEXT("...as airborne"), Slide.GetLastEnd() == FPlayerSlide::EEnd::Airborne);
	Slide.Start(FVector(Sprint, 0.0, 0.0), FVector::ForwardVector, Sprint * FPlayerSlide::SpeedMultiplier, Crouched);
	Slide.Stop();
	TestTrue(TEXT("A jump stops it"), !Slide.IsActive() && Slide.GetLastEnd() == FPlayerSlide::EEnd::Cancelled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerSlideTest, "Looter.Locomotion.Slide.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerSlideTest::RunTest(const FString& Parameters)
{
	// The user's call: crouch while sprinting slides, 10% faster than the sprint, for a moment; it ends in the crouch.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	// As in the game, the actors' own events run (a level never readied for play drops them: AActor::ProcessEvent).
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	const AStaticMeshActor* Floor = SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0));
	UPlayerLocomotionComponent* Slider = SpawnPlayer(World, FVector::ZeroVector);
	UPlayerLocomotionComponent* Walker = SpawnPlayer(World, FVector(0.0, 1000.0, 0.0));
	UPlayerLocomotionComponent* Jumper = SpawnPlayer(World, FVector(0.0, 2000.0, 0.0));
	UPlayerLocomotionComponent* Leaper = SpawnPlayer(World, FVector(0.0, 3000.0, 0.0));
	if (!TestNotNull(TEXT("A floor"), Floor) || !Slider || !Walker || !Jumper || !Leaper)
	{
		AddError(TEXT("The players weren't made."));
		return false;
	}

	// Sprint, then crouch: a slide at 1.1x the sprint, along the run, in the crouched capsule. All at the player's speeds:
	// walk 510, sprint 790.5, slide 869.55 cm/s.
	ACharacter* SliderBody = WalkerOf(Slider);
	UCharacterMovementComponent* Movement = SliderBody->GetCharacterMovement();
	const float Walk = Movement->MaxWalkSpeed;
	TestEqual(TEXT("The player walks at 85% of 600"), Walk, LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale, 0.01f);
	StartSprint(Slider);
	const float Sprint = static_cast<float>(Movement->Velocity.Size2D());
	TestEqual(TEXT("Sprinting"), Movement->MaxWalkSpeed, Sprint, 0.5f);
	TestEqual(TEXT("...at 85% of 930"), Movement->MaxWalkSpeed, 930.f * LooterPlayerSize::SpeedScale, 0.5f);
	Slider->HandleCrouchPressed();
	TestTrue(TEXT("Sprint and crouch: a slide"), Slider->IsSliding());
	TestEqual(TEXT("...at 1.1x the sprint"), static_cast<float>(Movement->Velocity.Size2D()), Sprint * FPlayerSlide::SpeedMultiplier, 0.5f);
	TestTrue(TEXT("...along the run"), Movement->Velocity.GetSafeNormal2D().Equals(FVector::ForwardVector, 1.e-3));
	TestTrue(TEXT("...crouched, to fit under things"), Movement->bWantsToCrouch);
	TestTrue(TEXT("...with room to hold that speed"), Movement->MaxWalkSpeedCrouched >= Sprint * FPlayerSlide::SpeedMultiplier - 0.5f);
	// What the next move does with that wish: the crouched capsule, at the player's size.
	Movement->Crouch(false);
	TestTrue(FString::Printf(TEXT("...in the crouched capsule (%.1f cm tall)"), SliderBody->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f),
		SliderBody->bIsCrouched && FMath::IsNearlyEqual(SliderBody->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Slider->CrouchedHalfHeight * LooterPlayerSize::Scale, 0.01f));

	float Time = 0.f;
	while (Slider->IsSliding() && Time < 5.f)
	{
		Step(Slider);
		Time += Frame;
	}
	TestTrue(FString::Printf(TEXT("The slide ends on time (%.2f s)"), Time), Slider->GetSlide().GetLastEnd() == FPlayerSlide::EEnd::Time
		&& FMath::IsNearlyEqual(Time, FPlayerSlide::Duration, Frame + 0.001f));
	TestTrue(TEXT("...in a crouch (the key is still held)"), Movement->bWantsToCrouch);
	TestEqual(TEXT("...at the crouched walk"), Movement->MaxWalkSpeedCrouched, Slider->CrouchSpeed, 0.01f);
	Slider->HandleCrouchReleased();
	Step(Slider);
	TestFalse(TEXT("Letting go of crouch afterwards stands up"), Movement->bWantsToCrouch);

	// Walking, then crouch: just a crouch.
	UCharacterMovementComponent* WalkerMovement = WalkerOf(Walker)->GetCharacterMovement();
	MoveForward(Walker, Walk);
	Step(Walker);
	Walker->HandleCrouchPressed();
	Step(Walker);
	TestFalse(TEXT("Crouch without a sprint doesn't slide"), Walker->IsSliding());
	TestTrue(TEXT("...it crouches"), WalkerMovement->bWantsToCrouch);
	TestEqual(TEXT("...at the walk it had"), static_cast<float>(WalkerMovement->Velocity.Size2D()), Walk, 0.5f);

	// Jump mid-slide: the slide ends and the player stands, no jump (the crouch-jump rule).
	ACharacter* JumperBody = WalkerOf(Jumper);
	StartSprint(Jumper);
	Jumper->HandleCrouchPressed();
	JumperBody->GetCharacterMovement()->Crouch(false);
	Step(Jumper);
	TestTrue(TEXT("Sliding"), Jumper->IsSliding() && JumperBody->bIsCrouched);
	Jumper->HandleJumpPressed();
	TestFalse(TEXT("Jump mid-slide ends it"), Jumper->IsSliding());
	TestFalse(TEXT("...stands up"), JumperBody->bIsCrouched || JumperBody->GetCharacterMovement()->bWantsToCrouch);
	TestFalse(TEXT("...without jumping"), JumperBody->bPressedJump || JumperBody->GetCharacterMovement()->IsFalling());

	// Sprinting off a ledge, then crouch: no slide in the air.
	StartSprint(Leaper);
	WalkerOf(Leaper)->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	Leaper->HandleCrouchPressed();
	TestFalse(TEXT("No slide in the air"), Leaper->IsSliding());
	return true;
}

#endif
