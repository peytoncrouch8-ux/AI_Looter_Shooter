#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/MissionTestWorld.h"
#include "World/InteriorVisibility.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteriorVisibilityViewsTest, "Looter.InteriorVisibility.Views",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteriorVisibilityViewsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Level;
	if (!TestTrue(TEXT("Test level made"), Level.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = Level.GetTestWorld();
	AInteriorVisibility* Mine = World->SpawnActor<AInteriorVisibility>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The mine's visibility"), Mine))
	{
		return false;
	}
	// The rooms 30 x 20 x 12 m round the origin, the mouth at their -X face (the defaults); a side room east of them.
	Mine->AdditionalInside = { FBox(FVector(3000.0, -200.0, -200.0), FVector(3400.0, 200.0, 200.0)) };
	const FName Inside = Mine->InsideTag;
	const FName Outside = Mine->OutsideTag;
	AActor* Gallery = MissionTestWorld::SpawnMarker(World, FVector(0.0, 0.0, 0.0), Inside);
	AActor* Stope = MissionTestWorld::SpawnMarker(World, FVector(800.0, 300.0, 0.0), Inside);
	// A piece of the mine the story keeps hidden (a barricade not built yet): the visibility never shows it.
	AActor* StoryHidden = MissionTestWorld::SpawnMarker(World, FVector(400.0, 0.0, 0.0), Inside);
	AActor* Pines = MissionTestWorld::SpawnMarker(World, FVector(-6000.0, 0.0, 0.0), Outside);
	AActor* Diggings = MissionTestWorld::SpawnMarker(World, FVector(-8000.0, 2000.0, 0.0), Outside);
	if (!Gallery || !Stope || !StoryHidden || !Pines || !Diggings)
	{
		return false;
	}
	StoryHidden->SetActorHiddenInGame(true);
	auto Shows = [](const AActor* Actor) { return !Actor->IsHidden(); };

	// No player yet: the level begins with the player outside, the mine hidden.
	Mine->DispatchBeginPlay();
	TestTrue(TEXT("It begins outside"), Mine->GetView() == EInteriorView::Outside);
	TestTrue(TEXT("...the mine's pieces hidden, the outside shown"), !Shows(Gallery) && !Shows(Stope) && Shows(Pines) && Shows(Diggings));
	TestEqual(TEXT("...two hidden by it (the story's own isn't its)"), Mine->CountHidden(), 2);

	// At the mouth: both.
	Mine->UpdateFor(FVector(-1700.0, 0.0, 0.0));
	TestTrue(TEXT("At the portal"), Mine->GetView() == EInteriorView::Portal);
	TestTrue(TEXT("...everything shows"), Shows(Gallery) && Shows(Stope) && Shows(Pines) && Shows(Diggings) && Mine->CountHidden() == 0);
	TestFalse(TEXT("...but what the story hides stays hidden"), Shows(StoryHidden));

	// In the gallery: the outside goes.
	Mine->UpdateFor(FVector(200.0, 0.0, 0.0));
	TestTrue(TEXT("Inside"), Mine->GetView() == EInteriorView::Inside);
	TestTrue(TEXT("...the outside hidden, the mine shown"), Shows(Gallery) && Shows(Stope) && !Shows(Pines) && !Shows(Diggings));
	TestFalse(TEXT("...the story's piece still hidden"), Shows(StoryHidden));

	// A step past the rooms' east face keeps the view (the margin), several don't; the side room counts as inside.
	Mine->UpdateFor(FVector(1500.0 + 0.5 * Mine->Margin, 0.0, 0.0));
	TestTrue(TEXT("A step past the face: still inside"), Mine->GetView() == EInteriorView::Inside);
	Mine->UpdateFor(FVector(3200.0, 0.0, 0.0));
	TestTrue(TEXT("In the side room: inside"), Mine->GetView() == EInteriorView::Inside);
	Mine->UpdateFor(FVector(2400.0, 0.0, 0.0));
	TestTrue(TEXT("Well past the face, outside the side room: outside"), Mine->GetView() == EInteriorView::Outside);
	TestTrue(TEXT("...the mine hidden again, the outside back"), !Shows(Gallery) && !Shows(Stope) && Shows(Pines) && Shows(Diggings));

	// Coming back from outside, the face itself is the line: the margin only keeps a view that is inside.
	Mine->UpdateFor(FVector(1500.0 + 0.5 * Mine->Margin, 0.0, 0.0));
	TestTrue(TEXT("From outside, a step outside the face stays outside (no margin inward)"), Mine->GetView() == EInteriorView::Outside);
	return true;
}

#endif
