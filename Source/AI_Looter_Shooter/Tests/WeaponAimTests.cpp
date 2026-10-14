#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The sight in every stance (the user, 2026-10-14: "When I crouch, the aim is misaligned"). Raised to the eye, the sight's
// aim point sits on the camera's line of sight, which is the line the shots fly (and the crosshair's): standing, crouched,
// walking crouched, sliding and in the air; from the hip a crouched player's shots leave along the line of sight too. Played
// on the real character (BP_LooterCharacter) in a test level, held by a player controller whose camera manager makes the
// view the shots take, as in the game.

using namespace LocomotionTestWorld;

namespace
{
	/** How far (world cm) the sight's aim point may be from the line of sight or the shot's line: rounding, nothing more. */
	constexpr float AimTestOnLine = 0.01f;
	/** The most the raised gun may lean (degrees) standing or crouched still: settled, a crouch's 6 degree hip cant left 0.9. */
	constexpr float AimTestMaxCant = 0.2f;
	/** Frames for the sight to come all the way up (a slow gun takes under half a second) and the view to settle after. */
	constexpr int32 AimTestRaiseFrames = 40;
	/** Frames followed with the sight up, and the fewest that must have been measured for a stance to count. */
	constexpr int32 AimTestFollowFrames = 20;
	constexpr int32 AimTestMinFrames = 5;
	/** Where the player looks: a little down, so the line of sight isn't simply level. */
	const FRotator AimTestLook(-6.f, 0.f, 0.f);

	/** A player of the test level as the game has one: held by this machine's player controller, a rifle in hand. */
	struct FAimTestPlayer
	{
		UPlayerLocomotionComponent* Locomotion = nullptr;
		UPlayerViewComponent* View = nullptr;
		APlayerController* Controller = nullptr;
		AWeaponBase* Gun = nullptr;

		bool IsReady() const { return Locomotion && View && Controller && Controller->PlayerCameraManager && Gun; }
	};

	FAimTestPlayer AimTestSpawn(UWorld* World, const FVector& Feet, UWeaponDefinition* Rifle)
	{
		FAimTestPlayer Player;
		Player.Locomotion = SpawnPlayer(World, Feet);
		ACharacter* Body = Player.Locomotion ? BodyOf(Player.Locomotion) : nullptr;
		if (!Body)
		{
			return Player;
		}
		Player.View = Body->FindComponentByClass<UPlayerViewComponent>();

		// Held as this machine's player holds it: the camera turns with the controller's aim, and the shots take the camera
		// manager's view. With no local player here the manager doesn't update itself, so the frames update it (AimTestFrame).
		Player.Controller = World->SpawnActor<APlayerController>();
		if (Player.Controller)
		{
			// A test level never readies its actors for play, so the controller's PostInitializeComponents, which makes its
			// camera manager, never ran: make it here (without the default camera modifiers, which the view doesn't want).
			if (!Player.Controller->PlayerCameraManager)
			{
				Player.Controller->SpawnPlayerCameraManager();
			}
			World->AddController(Player.Controller);
			Player.Controller->SetAsLocalPlayerController();
			Player.Controller->SetPawn(Body);
			Body->Controller = Player.Controller;
			Player.Controller->SetControlRotation(AimTestLook);
			if (APlayerCameraManager* CameraManager = Player.Controller->PlayerCameraManager)
			{
				CameraManager->bUseClientSideCameraUpdates = false;
				CameraManager->SetViewTarget(Body);
			}
		}

		if (UWeaponManagerComponent* Weapons = Body->FindComponentByClass<UWeaponManagerComponent>())
		{
			if (Rifle)
			{
				Weapons->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 1));
			}
			Player.Gun = Weapons->GetActiveWeapon();
		}
		return Player;
	}

	/**
	 * One frame as the game plays it: the movement's crouch and the locomotion (the gun placed on the camera), then the view
	 * (it ticks after them: the sight's raise), then the camera manager at the frame's end (the camera turned to the aim).
	 */
	void AimTestFrame(const FAimTestPlayer& Player)
	{
		PlayFrame(Player.Locomotion);
		static_cast<UActorComponent*>(Player.View)->TickComponent(Frame, LEVELTICK_All, nullptr);
		Player.Controller->PlayerCameraManager->UpdateCamera(Frame);
	}

	/** The worst of the frames followed with the sight all the way up (and up the frame before, so the gun was placed for it). */
	struct FAimTestTrack
	{
		int32 Frames = 0;
		/** The sight's aim point from the camera's line of sight, and from the line the next shot would fly (world cm). */
		float FromSight = 0.f;
		float FromShot = 0.f;
		/** The gun's lean against the camera (degrees). */
		float Cant = 0.f;
		bool bWasUp = false;

		void Sample(const FAimTestPlayer& Player)
		{
			const bool bUp = Player.View->GetAimAlpha() >= 1.f;
			if (bUp && bWasUp)
			{
				const UCameraComponent* Camera = CameraOf(Player.Locomotion);
				const FVector AimPoint = Player.Gun->GetActorTransform().TransformPosition(Player.Gun->GetAimPoint());
				FromSight = FMath::Max(FromSight, FMath::PointDistToLine(AimPoint, Camera->GetForwardVector(), Camera->GetComponentLocation()));
				FVector ShotStart;
				FRotator ShotRotation;
				Player.View->GetAimViewPoint(ShotStart, ShotRotation);
				FromShot = FMath::Max(FromShot, FMath::PointDistToLine(AimPoint, ShotRotation.Vector(), ShotStart));
				Cant = FMath::Max(Cant, FMath::Abs(static_cast<float>(Player.Gun->GetRootComponent()->GetRelativeRotation().Roll)));
				++Frames;
			}
			bWasUp = bUp;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponAimCrouchAlignedTest, "Looter.Weapons.Aim.CrouchAligned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWeaponAimCrouchAlignedTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("The rifle"), Rifle))
	{
		return false;
	}
	// As in the game, the actors' own events run (a level never readied for play drops them: AActor::ProcessEvent).
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	if (!TestNotNull(TEXT("A floor"), SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(8000.0, 8000.0, 100.0))))
	{
		return false;
	}

	enum class EAimTestStance : uint8 { Standing, Crouched, CrouchWalking, Sliding, InTheAir };
	struct FAimTestCase
	{
		const TCHAR* Name;
		EAimTestStance Stance;
		FVector Feet;
	};
	const FAimTestCase Cases[] = {
		{ TEXT("Standing"), EAimTestStance::Standing, FVector(0.0, -3000.0, 0.0) },
		{ TEXT("Crouched"), EAimTestStance::Crouched, FVector(0.0, -1500.0, 0.0) },
		{ TEXT("Walking crouched"), EAimTestStance::CrouchWalking, FVector(0.0, 0.0, 0.0) },
		{ TEXT("Sliding"), EAimTestStance::Sliding, FVector(0.0, 1500.0, 0.0) },
		{ TEXT("In the air"), EAimTestStance::InTheAir, FVector(0.0, 3000.0, 0.0) },
	};
	FAimTestPlayer Players[UE_ARRAY_COUNT(Cases)];
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cases); ++Index)
	{
		const FAimTestPlayer& Made = Players[Index] = AimTestSpawn(World, Cases[Index].Feet, Rifle);
		const TCHAR* Name = Cases[Index].Name;
		// Each piece said on its own, so a setup that doesn't come up says which.
		TestNotNull(FString::Printf(TEXT("%s: the player (BP_LooterCharacter)"), Name), Made.Locomotion);
		TestNotNull(FString::Printf(TEXT("%s: its view component"), Name), Made.View);
		TestNotNull(FString::Printf(TEXT("%s: a player controller holding it"), Name), Made.Controller);
		TestTrue(FString::Printf(TEXT("%s: ...with a camera manager"), Name), Made.Controller && Made.Controller->PlayerCameraManager);
		TestNotNull(FString::Printf(TEXT("%s: a rifle in hand"), Name), Made.Gun);
		if (!Made.IsReady())
		{
			return false;
		}
	}
	// The guns drawn a while ago (each comes up over its ready time), and a clock for the camera manager to stamp its view
	// with: the test level's never runs.
	World->TimeSeconds += 10.0;

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Cases); ++Index)
	{
		const FAimTestCase& Case = Cases[Index];
		const FAimTestPlayer& Player = Players[Index];
		ACharacter* Body = BodyOf(Player.Locomotion);
		UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
		for (int32 Frames = 0; Frames < 10; ++Frames)
		{
			AimTestFrame(Player);
		}

		FAimTestTrack Track;
		bool bInStance = true;
		switch (Case.Stance)
		{
		case EAimTestStance::Standing:
		case EAimTestStance::Crouched:
		case EAimTestStance::CrouchWalking:
		{
			const bool bCrouch = Case.Stance != EAimTestStance::Standing;
			const bool bWalk = Case.Stance == EAimTestStance::CrouchWalking;
			if (bCrouch)
			{
				CrouchDown(Player.Locomotion);
				// From the hip, crouched: the shot leaves the camera along its line of sight, where the crosshair is.
				for (int32 Frames = 0; Frames < 30; ++Frames)
				{
					AimTestFrame(Player);
				}
				const UCameraComponent* Camera = CameraOf(Player.Locomotion);
				FVector ShotStart;
				FRotator ShotRotation;
				Player.View->GetAimViewPoint(ShotStart, ShotRotation);
				TestTrue(FString::Printf(TEXT("%s, from the hip: the shot leaves the camera (%.3f cm away)"), Case.Name,
					FVector::Dist(ShotStart, Camera->GetComponentLocation())), ShotStart.Equals(Camera->GetComponentLocation(), AimTestOnLine));
				TestTrue(TEXT("...along its line of sight"), ShotRotation.Vector().Equals(Camera->GetForwardVector(), 1.e-4));
				TestTrue(TEXT("...which is where the player aims"), ShotRotation.Vector().Equals(AimTestLook.Vector(), 1.e-4));
			}
			Player.View->HandleAimPressed();
			for (int32 Frames = 0; Frames < AimTestRaiseFrames + AimTestFollowFrames; ++Frames)
			{
				if (bWalk)
				{
					MoveForward(Player.Locomotion, Movement->MaxWalkSpeedCrouched);
				}
				AimTestFrame(Player);
				if (Frames >= AimTestRaiseFrames)
				{
					Track.Sample(Player);
				}
			}
			bInStance = static_cast<bool>(Body->bIsCrouched) == bCrouch;
			break;
		}
		case EAimTestStance::Sliding:
		{
			// The sight comes up mid-slide (raising it first would end the sprint a slide needs).
			StartSlide(Player.Locomotion);
			Player.View->HandleAimPressed();
			bInStance = Player.Locomotion->IsSliding();
			for (int32 Frames = 0; Frames < 120 && Player.Locomotion->IsSliding(); ++Frames)
			{
				AimTestFrame(Player);
				if (Player.Locomotion->IsSliding())
				{
					Track.Sample(Player);
				}
			}
			break;
		}
		case EAimTestStance::InTheAir:
		{
			// Up first, then a jump: leaving the ground kicks the gun, and the sight stays on the line through it.
			Player.View->HandleAimPressed();
			for (int32 Frames = 0; Frames < AimTestRaiseFrames; ++Frames)
			{
				AimTestFrame(Player);
			}
			Movement->SetMovementMode(MOVE_Falling);
			Movement->Velocity = FVector(0.0, 0.0, Movement->JumpZVelocity);
			for (int32 Frames = 0; Frames < 25; ++Frames)
			{
				AimTestFrame(Player);
				Track.Sample(Player);
			}
			bInStance = Movement->IsFalling();
			break;
		}
		}

		TestTrue(FString::Printf(TEXT("%s: in that stance"), Case.Name), bInStance);
		if (!TestTrue(FString::Printf(TEXT("%s: the sight came all the way up (%d frames followed)"), Case.Name, Track.Frames), Track.Frames >= AimTestMinFrames))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: the sight's aim point on the line of sight (%.4f cm off at worst)"), Case.Name, Track.FromSight),
			Track.FromSight <= AimTestOnLine);
		TestTrue(FString::Printf(TEXT("%s: ...and on the shot's line (%.4f cm off at worst)"), Case.Name, Track.FromShot),
			Track.FromShot <= AimTestOnLine);
		if (Case.Stance == EAimTestStance::Standing || Case.Stance == EAimTestStance::Crouched)
		{
			TestTrue(FString::Printf(TEXT("%s: the gun held upright, no stance cant (%.3f degrees)"), Case.Name, Track.Cant), Track.Cant <= AimTestMaxCant);
		}
	}
	return true;
}

#endif
