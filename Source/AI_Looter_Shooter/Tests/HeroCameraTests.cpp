#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Embers/HeroDefinition.h"
#include "Player/Animation/BodyFigureComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Tests/AutomationCommon.h"
#include "Tests/LocomotionTestWorld.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"

// The over-the-shoulder camera fitted to each hero (UHeroDefinition::ThirdPersonCamera, UBodyFigureComponent::GetCameraOffset,
// UPlayerViewComponent::UpdateBoom; 2026-10-20): one camera tuned to Ellis's size had tall Crane's hat and broad Gauge's
// smokestacks filling the left of the aimed view. The worn hero's offset (arm length, right, up) rides on the base arm and
// shoulder, on a cvar's override of them too, eases in when the hero changes mid-play, and the front view and first person
// don't use it.

using namespace LocomotionTestWorld;

namespace
{
	/** The third-person spring arm: the one the camera rides on (the view makes it as play begins). */
	const USpringArmComponent* FindBoom(const AActor* Owner)
	{
		TInlineComponentArray<UCameraComponent*> Cameras(Owner);
		for (const UCameraComponent* Camera : Cameras)
		{
			if (const USpringArmComponent* Arm = Cast<USpringArmComponent>(Camera->GetAttachParent()))
			{
				return Arm;
			}
		}
		return nullptr;
	}

	/** One frame of the view (the boom is placed in it), or Seconds of them. */
	void StepView(UPlayerViewComponent* View)
	{
		static_cast<UActorComponent*>(View)->TickComponent(Frame, LEVELTICK_All, nullptr);
	}

	void RunView(UPlayerViewComponent* View, float Seconds)
	{
		for (int32 Frames = FMath::CeilToInt(Seconds / Frame); Frames > 0; --Frames)
		{
			StepView(View);
		}
	}

	/** The boom as the base view leaves it (Ellis's), to read the hero's offset off as the difference. */
	struct FBoomBase
	{
		float Arm = 0.f;
		FVector Socket = FVector::ZeroVector;
		float Scale = 1.f;

		FBoomBase(const USpringArmComponent& Boom, float InScale)
			: Arm(Boom.TargetArmLength), Socket(Boom.SocketOffset), Scale(InScale)
		{
		}

		/** What the boom has beyond the base, on the full-size body (the hero's offset: X arm, Y right, Z up). */
		FVector Beyond(const USpringArmComponent& Boom) const
		{
			return FVector(Boom.TargetArmLength - Arm, Boom.SocketOffset.Y - Socket.Y, Boom.SocketOffset.Z - Socket.Z) / Scale;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroCameraDataTest, "Looter.Heroes.CameraData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroCameraDataTest::RunTest(const FString& Parameters)
{
	// The data (create_ember_assets.py's THIRD_PERSON_CAMERA): Ellis is the size the camera was tuned to; Crane, the tallest
	// with a wide hat, rides higher and never closer; Gauge, the broadest, out to the right and back; the two smaller heroes
	// only a touch; nobody far enough to put the camera in a wall.
	const UHeroDefinition* Ellis = UHeroDefinition::Load(TEXT("Ellis"));
	const UHeroDefinition* Odessa = UHeroDefinition::Load(TEXT("Odessa"));
	const UHeroDefinition* Crane = UHeroDefinition::Load(TEXT("Crane"));
	const UHeroDefinition* Gauge = UHeroDefinition::Load(TEXT("Gauge"));
	const UHeroDefinition* Pike = UHeroDefinition::Load(TEXT("Pike"));
	if (!TestTrue(TEXT("The heroes' definitions load"), Ellis && Odessa && Crane && Gauge && Pike))
	{
		return false;
	}
	TestTrue(TEXT("Ellis: the camera was tuned to his size"), Ellis->ThirdPersonCamera.IsNearlyZero());
	TestTrue(FString::Printf(TEXT("Crane: up, and back and out, never closer (%s)"), *Crane->ThirdPersonCamera.ToCompactString()),
		Crane->ThirdPersonCamera.Z > 0.f && Crane->ThirdPersonCamera.X >= 0.f && Crane->ThirdPersonCamera.Y >= 0.f);
	TestTrue(FString::Printf(TEXT("Gauge: out to the right and back (%s)"), *Gauge->ThirdPersonCamera.ToCompactString()),
		Gauge->ThirdPersonCamera.Y > 0.f && Gauge->ThirdPersonCamera.X > 0.f);
	TestTrue(FString::Printf(TEXT("Odessa and Pike: only a touch (%s, %s)"), *Odessa->ThirdPersonCamera.ToCompactString(), *Pike->ThirdPersonCamera.ToCompactString()),
		Odessa->ThirdPersonCamera.GetAbsMax() <= 15.f && Pike->ThirdPersonCamera.GetAbsMax() <= 15.f);
	for (const UHeroDefinition* Hero : { Ellis, Odessa, Crane, Gauge, Pike })
	{
		TestTrue(FString::Printf(TEXT("%s: within reach of the base camera (%s)"), *Hero->Id.ToString(), *Hero->ThirdPersonCamera.ToCompactString()),
			Hero->ThirdPersonCamera.GetAbsMax() <= 60.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeroCameraBoomTest, "Looter.Heroes.CameraBoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHeroCameraBoomTest::RunTest(const FString& Parameters)
{
	// The real character in a test level, no hero picked: the body hands the view nothing, then each hero worn in turn.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UPlayerLocomotionComponent* Locomotion = SpawnPlayer(World, FVector::ZeroVector);
	ACharacter* Player = Locomotion ? BodyOf(Locomotion) : nullptr;
	UPlayerViewComponent* View = Player ? Player->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	UBodyFigureComponent* Figure = Player ? Player->FindComponentByClass<UBodyFigureComponent>() : nullptr;
	const UHeroDefinition* Ellis = UHeroDefinition::Load(TEXT("Ellis"));
	const UHeroDefinition* Crane = UHeroDefinition::Load(TEXT("Crane"));
	if (!TestTrue(TEXT("A player with a view and a body, and the heroes"), View && Figure && Ellis && Crane))
	{
		return false;
	}
	const FVector Want = Crane->ThirdPersonCamera;
	TestTrue(TEXT("No hero picked: the body hands the view nothing"), Figure->GetCameraOffset().IsNearlyZero());

	// Over the shoulder, nobody's offset: the base arm and shoulder to read the heroes' off.
	View->SetViewMode(EPlayerViewMode::ThirdPerson);
	RunView(View, 2.f);
	const USpringArmComponent* Boom = FindBoom(Player);
	if (!TestNotNull(TEXT("The third-person spring arm"), Boom))
	{
		return false;
	}
	const FBoomBase Base(*Boom, static_cast<float>(Player->GetActorScale3D().Z));
	TestTrue(TEXT("...the view's own offset is none"), View->GetHeroCameraOffset().IsNearlyZero());

	// Crane put on mid-play: the body hands the view his offset, and the arm glides to it, not a jump.
	Figure->SetShownHero(Crane);
	TestTrue(TEXT("Crane's body is worn"), Figure->IsWorn());
	TestTrue(FString::Printf(TEXT("...and hands the view his offset (%s)"), *Figure->GetCameraOffset().ToCompactString()),
		Figure->GetCameraOffset().Equals(Want, 0.01));
	StepView(View);
	const double Early = Base.Beyond(*Boom).Size();
	TestTrue(FString::Printf(TEXT("A frame on, it is on its way, not there (%.2f of %.2f cm)"), Early, Want.Size()),
		Early > 0.1 && Early < Want.Size() * 0.5);
	RunView(View, 2.f);
	const FVector Settled = Base.Beyond(*Boom);
	TestTrue(FString::Printf(TEXT("Two seconds on, the arm, the right and the up are his (%s, want %s)"), *Settled.ToCompactString(), *Want.ToCompactString()),
		FVector::Dist(Settled, Want) <= 0.1);
	TestTrue(TEXT("...the shoulder's forward component is untouched"), FMath::IsNearlyEqual(Boom->SocketOffset.X, Base.Socket.X, 0.01));

	// A cvar's override of the shoulder is the base: his offset rides on top of it.
	IConsoleVariable* Right = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.ThirdPerson.ShoulderRight"));
	if (TestNotNull(TEXT("The Looter.ThirdPerson.ShoulderRight cvar"), Right))
	{
		const FString Before = Right->GetString();
		Right->Set(TEXT("90"), ECVF_SetByConsole);
		StepView(View);
		const double WantY = (90.0 + Want.Y) * Base.Scale;
		TestTrue(FString::Printf(TEXT("Shoulder cvar 90: his offset on top (%.2f cm, want %.2f)"), Boom->SocketOffset.Y, WantY),
			FMath::IsNearlyEqual(Boom->SocketOffset.Y, WantY, 0.05));
		Right->Set(*Before, ECVF_SetByConsole);
		StepView(View);
	}

	// Back to Ellis: it glides home.
	Figure->SetShownHero(Ellis);
	RunView(View, 2.f);
	TestTrue(TEXT("Ellis again: the base camera"), Base.Beyond(*Boom).IsNearlyZero(0.1f));

	// The front view and first person don't use it: the same arm for Crane as for Ellis, and in first person the offset is
	// taken at once, in place when the view comes back over the shoulder.
	View->SetViewMode(EPlayerViewMode::ThirdPersonFront);
	RunView(View, 2.f);
	const float FrontArm = Boom->TargetArmLength;
	const FVector FrontSocket = Boom->SocketOffset;
	Figure->SetShownHero(Crane);
	RunView(View, 2.f);
	TestTrue(TEXT("Front view: Crane's arm is Ellis's"), FMath::IsNearlyEqual(Boom->TargetArmLength, FrontArm, 0.01f)
		&& Boom->SocketOffset.Equals(FrontSocket, 0.01));

	Figure->SetShownHero(Ellis);
	View->SetViewMode(EPlayerViewMode::FirstPerson);
	RunView(View, 1.f);
	Figure->SetShownHero(Crane);
	StepView(View);
	TestTrue(TEXT("First person: his offset is taken at once"), View->GetHeroCameraOffset().Equals(Want, 0.01));
	View->SetViewMode(EPlayerViewMode::ThirdPerson);
	TestTrue(TEXT("...and in place as the view comes out over the shoulder"), View->GetHeroCameraOffset().Equals(Want, 0.01));
	RunView(View, 2.f);
	TestTrue(TEXT("...at the same fit as before"), FVector::Dist(Base.Beyond(*Boom), Want) <= 0.1);
	View->SetViewMode(EPlayerViewMode::FirstPerson);
	return true;
}

#endif
