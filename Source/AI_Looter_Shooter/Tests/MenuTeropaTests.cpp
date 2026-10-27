#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/LooterMenuGameMode.h"
#include "Core/LooterMenuPlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

namespace
{
	const TCHAR* MenuMapObject = TEXT("/Game/Maps/Lvl_MenuTeropa.Lvl_MenuTeropa");
	// The menu's view as built: FOV 50 across, looking along +X from the origin (build_menu_teropa.py).
	constexpr float BuiltFov = 50.f;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuTeropaMotionTest, "Looter.Menu.Teropa.Motion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMenuTeropaMotionTest::RunTest(const FString& Parameters)
{
	using Controller = ALooterMenuPlayerController;
	const FRotator Reach(0.4f, 0.7f, 0.3f);

	// The drift stays inside its reach, moves smoothly (no step a frame bigger than a hundredth of a degree), and stops at 0.
	float Previous = 0.f;
	bool bInside = true, bSmooth = true;
	for (int32 Frame = 0; Frame < 60 * 600; Frame += 7)
	{
		const FRotator Drift = Controller::DriftAt(Frame / 60.f, Reach, 1.f);
		bInside &= FMath::Abs(Drift.Pitch) <= Reach.Pitch + 1e-4f && FMath::Abs(Drift.Yaw) <= Reach.Yaw + 1e-4f
			&& FMath::Abs(Drift.Roll) <= Reach.Roll + 1e-4f;
		const FRotator Next = Controller::DriftAt((Frame + 1) / 60.f, Reach, 1.f);
		bSmooth &= FMath::Abs(Next.Yaw - Drift.Yaw) < 0.01f && FMath::Abs(Next.Pitch - Drift.Pitch) < 0.01f;
		Previous = Drift.Yaw;
	}
	TestTrue(TEXT("The drift stays within its reach for ten minutes"), bInside);
	TestTrue(TEXT("The drift never jumps between frames"), bSmooth);
	TestTrue(TEXT("Scale 0 holds the view still"), Controller::DriftAt(37.f, Reach, 0.f).IsNearlyZero(1e-6f));
	AddInfo(FString::Printf(TEXT("Yaw at ten minutes: %.3f"), Previous));

	// The cursor: right and up turn the view right and up, at most the reach; centred, nothing.
	const FVector2D ParallaxReach(0.6f, 0.35f);
	const FRotator Corner = Controller::ParallaxFor(FVector2D(1.f, 1.f), ParallaxReach, 1.f);
	TestEqual(TEXT("Cursor right turns the view right by the reach"), static_cast<float>(Corner.Yaw), 0.6f, 1e-5f);
	TestEqual(TEXT("Cursor up tips the view up by the reach"), static_cast<float>(Corner.Pitch), 0.35f, 1e-5f);
	TestTrue(TEXT("A centred cursor turns nothing"), Controller::ParallaxFor(FVector2D::ZeroVector, ParallaxReach, 1.f).IsNearlyZero(1e-6f));

	// The ease-in starts at its yaw, settles to none, and is off at 0 seconds.
	TestEqual(TEXT("The ease-in starts at its yaw"), Controller::IntroYawAt(0.f, 5.f, 2.5f), 2.5f, 1e-5f);
	TestTrue(TEXT("...is past half way by a third of its time"), Controller::IntroYawAt(5.f / 3.f, 5.f, 2.5f) < 1.25f);
	TestEqual(TEXT("...and done at its end"), Controller::IntroYawAt(5.f, 5.f, 2.5f), 0.f, 1e-6f);
	TestEqual(TEXT("IntroSeconds 0: no ease-in"), Controller::IntroYawAt(0.f, 0.f, 2.5f), 0.f, 1e-6f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuTeropaFramingTest, "Looter.Menu.Teropa.Framing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMenuTeropaFramingTest::RunTest(const FString& Parameters)
{
	using Controller = ALooterMenuPlayerController;
	// A sphere straight ahead: its left edge sits at -tan(its angular radius) / tan(half the field of view).
	const double Dist = 10000.0, SphereRadius = 2000.0;
	const float Expected = -FMath::Tan(FMath::Asin(SphereRadius / Dist)) / FMath::Tan(FMath::DegreesToRadians(BuiltFov * 0.5f));
	TestEqual(TEXT("A sphere ahead: its outline's left edge"),
		Controller::LeftmostNdcX(FTransform::Identity, BuiltFov, FVector(Dist, 0.0, 0.0), SphereRadius), Expected, 1e-3f);
	// Turning the view right moves the sphere left on screen.
	const FTransform TurnedRight(FRotator(0.f, 5.f, 0.f), FVector::ZeroVector);
	TestTrue(TEXT("Turning the view right moves it left"),
		Controller::LeftmostNdcX(TurnedRight, BuiltFov, FVector(Dist, 0.0, 0.0), SphereRadius) < Expected);
	// An eye inside the sphere: everything is behind its outline.
	TestTrue(TEXT("From inside, it fills the view"),
		Controller::LeftmostNdcX(FTransform::Identity, BuiltFov, FVector(100.0, 0.0, 0.0), SphereRadius) < -1000.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMenuTeropaSceneTest, "Looter.Menu.Teropa.Scene",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMenuTeropaSceneTest::RunTest(const FString& Parameters)
{
	// Lvl_MenuTeropa (build_menu_teropa.py): the menu however it's opened, a view, the planet, its moon and the stars, and the
	// planet clear of the menu's column (the left 40%) even at the drift's, the cursor's and the ease-in's furthest turn.
	const UWorld* Menu = LoadObject<UWorld>(nullptr, MenuMapObject);
	if (!TestNotNull(TEXT("Lvl_MenuTeropa loads"), Menu) || !TestNotNull(TEXT("It has a level"), Menu->PersistentLevel.Get()))
	{
		return false;
	}
	const AWorldSettings* Settings = Menu->PersistentLevel->GetWorldSettings();
	TestTrue(TEXT("Its game mode is the menu's"),
		Settings && Settings->DefaultGameMode && Settings->DefaultGameMode->IsChildOf(ALooterMenuGameMode::StaticClass()));

	const ACameraActor* View = nullptr;
	const AActor* Planet = nullptr;
	const AActor* Moon = nullptr;
	const AActor* Stars = nullptr;
	for (const AActor* Actor : Menu->PersistentLevel->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		if (Actor->ActorHasTag(MenuScene::ViewTag) && Actor->IsA<ACameraActor>())
		{
			View = Cast<ACameraActor>(Actor);
		}
		Planet = Actor->ActorHasTag(MenuScene::PlanetTag) ? Actor : Planet;
		Moon = Actor->ActorHasTag(MenuScene::MoonTag) ? Actor : Moon;
		Stars = Actor->ActorHasTag(MenuScene::StarsTag) ? Actor : Stars;
	}
	if (!TestNotNull(TEXT("A camera tagged MenuView"), View) || !TestNotNull(TEXT("A planet tagged MenuPlanet"), Planet))
	{
		return false;
	}
	TestNotNull(TEXT("A moon tagged MenuMoon"), Moon);
	TestNotNull(TEXT("Stars tagged MenuStars"), Stars);

	const float PlanetRadius = ALooterMenuPlayerController::PlanetRadiusOf(Planet);
	if (!TestTrue(TEXT("The planet wears M_Planet (a PlanetRadius)"), PlanetRadius > 0.f))
	{
		return false;
	}
	const float Fov = View->GetCameraComponent()->FieldOfView;
	const FTransform Base(View->GetActorQuat(), View->GetActorLocation());
	const ALooterMenuPlayerController* Defaults = GetDefault<ALooterMenuPlayerController>();
	const float WorstYaw = Defaults->DriftReach.Yaw + Defaults->ParallaxReach.X + Defaults->IntroYaw;
	const FTransform Worst(Base.GetRotation() * FRotator(0.f, WorstYaw, 0.f).Quaternion(), Base.GetLocation());
	const float Limb = ALooterMenuPlayerController::LeftmostNdcX(Base, Fov, Planet->GetActorLocation(), PlanetRadius);
	const float WorstLimb = ALooterMenuPlayerController::LeftmostNdcX(Worst, Fov, Planet->GetActorLocation(), PlanetRadius);
	TestTrue(TEXT("The planet's left limb keeps right of the column"), Limb > MenuScene::ColumnEdgeNdc);
	TestTrue(TEXT("...at the view's furthest turn too"), WorstLimb > MenuScene::ColumnEdgeNdc);
	const FVector Ahead = Base.InverseTransformPositionNoScale(Planet->GetActorLocation());
	TestTrue(TEXT("The planet sits right of the screen's middle"), Ahead.X > 0.0 && Ahead.Y > 0.0);
	AddInfo(FString::Printf(TEXT("Left limb at NDC %.3f (%.3f at the furthest turn); the column ends at %.2f"), Limb, WorstLimb,
		MenuScene::ColumnEdgeNdc));
	if (Stars && Moon)
	{
		// The stars' sphere is round the camera and far behind the planet and the moon (the engine sphere is 50 cm round,
		// scaled; a loaded level's components aren't registered, so their bounds would read empty).
		const double StarsRadius = Stars->GetActorScale3D().X * 50.0;
		const double Farthest = FMath::Max((Planet->GetActorLocation() - Base.GetLocation()).Size(),
			(Moon->GetActorLocation() - Base.GetLocation()).Size());
		TestTrue(TEXT("The stars lie beyond the planet and the moon"), StarsRadius > Farthest * 1.5);
	}
	return true;
}

#endif
