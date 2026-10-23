#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionTypes.h"
#include "Tests/InteractionTestWorld.h"
#include "World/ChapelBell.h"
#include "World/KeepersLantern.h"
#include "World/MissionItem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"

// Things looked at as a whole part to use (IInteractable::GetInteractionBox): the chapel bell's hanging rope, the Keeper's
// Lantern with the silk round it, a mission item grown to a hand's size. The 2026-10-23 play-check: the rope took focus
// only in a narrow band of pitch (missed at 30 degrees up), the lantern from two spots of four (nothing in reach where the
// objective sends you), Fenn's coins only dead on. A test level's traces don't see new bodies: these work from the cone.

using namespace InteractionTestWorld;

namespace
{
	/** The stand-in at Where (its feet), looking along Yaw and Pitch (degrees). */
	void Pose(APawn* Player, const FVector& Where, double Yaw, double Pitch)
	{
		Player->SetActorLocationAndRotation(Where, FRotator(Pitch, Yaw, 0.0));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionBoxNearestTest, "Looter.Interaction.Box.Nearest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionBoxNearestTest::RunTest(const FString& Parameters)
{
	// The box's point the crosshair's line means: where it goes in, or the box's point it passes nearest.
	const FInteractionBox Post(FTransform(FVector(200.0, 0.0, 0.0)), FVector(10.0, 10.0, 100.0));
	FInteractionView View;
	View.Location = FVector::ZeroVector;
	View.ReachOrigin = View.Location;
	View.Direction = FVector::ForwardVector;
	TestTrue(TEXT("Straight through it: where the line goes in"), InteractionFocus::NearestOnBox(View, Post, 400.0).Equals(FVector(190.0, 0.0, 0.0), 0.5));

	View.Location = FVector(0.0, 0.0, 150.0);
	const FVector Over = InteractionFocus::NearestOnBox(View, Post, 400.0);
	TestTrue(TEXT("Over its top: its top edge, under the line"), FMath::IsNearlyEqual(Over.Z, 100.0, 0.5) && Over.X >= 189.5 && Over.X <= 210.5
		&& FMath::IsNearlyZero(Over.Y, 0.5));

	View.Location = FVector(200.0, 0.0, 20.0);
	TestTrue(TEXT("From inside it: the line's start"), InteractionFocus::NearestOnBox(View, Post, 400.0).Equals(View.Location, 0.5));

	View.Location = FVector::ZeroVector;
	View.Direction = -FVector::ForwardVector;
	FInteractionCandidate Behind;
	Behind.Location = InteractionFocus::NearestOnBox(View, Post, 400.0);
	TestTrue(TEXT("Turned away from it: its point lies behind the eyes"), InteractionFocus::AimOf(View, Behind) < 0.f);

	// A box turned a quarter: along the line's side now, so a line 50 cm aside still goes through it.
	const FInteractionBox Beam(FTransform(FRotator(0.0, 90.0, 0.0), FVector(200.0, 0.0, 0.0)), FVector(100.0, 10.0, 10.0));
	View.Location = FVector(0.0, 50.0, 0.0);
	View.Direction = FVector::ForwardVector;
	TestTrue(TEXT("A turned box: judged in its own frame"), InteractionFocus::NearestOnBox(View, Beam, 400.0).Equals(FVector(190.0, 50.0, 0.0), 0.5));
	TestTrue(TEXT("...and a box knows what's inside it"), Beam.Contains(FVector(200.0, 90.0, 0.0)) && !Beam.Contains(FVector(200.0, 120.0, 0.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionBoxRopeTest, "Looter.Interaction.Box.Rope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionBoxRopeTest::RunTest(const FString& Parameters)
{
	// The bell's rope from its tail to the ceiling is looked at anywhere along it: level at the grip, 30 and 55 degrees up
	// it, down at its tail, and from 2 m off; not with the head turned away.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	AChapelBell* Bell = World->SpawnActor<AChapelBell>(Ahead(70.0), FRotator::ZeroRotator);
	if (!TestTrue(TEXT("Stand-in and bell placed"), Player && Interaction && Bell))
	{
		return false;
	}
	Bell->DispatchBeginPlay();
	const TOptional<FInteractionBox> Rope = Bell->GetInteractionBox();
	if (!TestTrue(TEXT("The rope has its box"), Rope.IsSet()))
	{
		return false;
	}
	const FBox Span = FBox::BuildAABB(Rope->Transform.GetLocation(), Rope->Extent);
	TestTrue(TEXT("...from its tail under the grip up to the ceiling, 1.8 m over it"), Span.Min.Z <= Ahead(70.0).Z - 40.0
		&& Span.Max.Z >= Ahead(70.0).Z + 175.0 && Rope->Extent.X <= 15.0);

	struct FLook { double Yaw; double Pitch; bool bFocused; const TCHAR* What; };
	const FLook Looks[] = {
		{ 0.0, 0.0, true, TEXT("level, at the grip") },
		{ 0.0, 30.0, true, TEXT("30 degrees up the rope (the play-check's miss)") },
		{ 0.0, 55.0, true, TEXT("55 degrees up, near the ceiling") },
		{ 0.0, -25.0, true, TEXT("down at its tail") },
		{ 70.0, 30.0, false, TEXT("30 degrees up, the head turned away") },
	};
	for (const FLook& Look : Looks)
	{
		Pose(Player, FVector::ZeroVector, Look.Yaw, Look.Pitch);
		Interaction->UpdateInteraction(0.f);
		TestTrue(FString::Printf(TEXT("The rope looked at %s: %s"), Look.What, Look.bFocused ? TEXT("offered") : TEXT("not offered")),
			(Interaction->GetFocusedActor() == Bell) == Look.bFocused);
	}
	// From 2 m off, 30 degrees up: still the rope.
	Pose(Player, FVector(-130.0, 0.0, 0.0), 0.0, 30.0);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("From 2 m off, 30 degrees up: offered"), Interaction->GetFocusedActor() == Bell);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionBoxLanternTest, "Looter.Interaction.Box.Lantern",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionBoxLanternTest::RunTest(const FString& Parameters)
{
	// The Keeper's Lantern hangs at chest height in the webbing: stood over, it lies well below the eyes, and from a rock
	// it's looked down at. Its box (the lantern and the silk over it) is looked at either way.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	AKeepersLantern* Lantern = World->SpawnActor<AKeepersLantern>(FVector(400.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!TestTrue(TEXT("Stand-in and lantern placed"), Player && Interaction && Lantern))
	{
		return false;
	}
	// Any time (its story is Main 5's: KeepersLanternTests.cpp).
	Lantern->TakeWhen = FStoryCondition();
	Lantern->DispatchBeginPlay();
	TOptional<FInteractionBox> Box = Lantern->GetInteractionBox();
	if (!TestTrue(TEXT("Hanging, it has its box"), Box.IsSet()))
	{
		return false;
	}
	TestTrue(TEXT("...round the lantern, wider than its guard"), Box->Extent.X >= 24.0 && Box->Extent.Y >= 24.0
		&& Box->Contains(Lantern->GetInteractionLocation().Get(FVector::ZeroVector)));

	// Its box's top 8 cm under the eyes, its middle 45 cm ahead: stood right over it.
	const FVector Want(45.0, 0.0, EyeHeight - 8.0 - Box->Extent.Z);
	Lantern->SetActorLocation(Lantern->GetActorLocation() + (Want - Box->Transform.GetLocation()));
	Box = Lantern->GetInteractionBox();
	const FVector Middle = Lantern->GetInteractionLocation().Get(FVector::ZeroVector);
	const double MiddleDegrees = FMath::RadiansToDegrees(FMath::Acos((Middle - Ahead(0.0)).GetSafeNormal().X));
	AddInfo(FString::Printf(TEXT("The lantern's middle lies %.0f degrees under a level look (the near cone is 30)."), MiddleDegrees));
	Pose(Player, FVector::ZeroVector, 0.0, 0.0);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Stood over it, looking level: \"Take the Keeper's Lantern\""), Interaction->GetFocusedActor() == Lantern
		&& Interaction->GetFocusedOptions().bTap);

	// From a rock a metre up and a step back, looking 60 degrees down onto it.
	Pose(Player, FVector(-15.0, 0.0, 100.0), 0.0, -60.0);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("From a rock, looking down onto it: offered"), Interaction->GetFocusedActor() == Lantern);

	// From beside it (it hangs 1 m off the wall: walked up along the wall), turned to it.
	Pose(Player, Box->Transform.GetLocation() - FVector(0.0, 150.0, EyeHeight - 10.0), 90.0, 0.0);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("From beside it along the wall: offered"), Interaction->GetFocusedActor() == Lantern);

	Pose(Player, FVector::ZeroVector, 120.0, 0.0);
	Interaction->UpdateInteraction(0.f);
	TestNull(TEXT("Turned away: nothing"), Interaction->GetFocusedActor());

	// Taken: there's nothing left to look at.
	Pose(Player, FVector::ZeroVector, 0.0, 0.0);
	TestTrue(TEXT("Taken"), Lantern->Take(Player));
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("...its box gone, never offered again"), !Lantern->GetInteractionBox().IsSet() && Interaction->GetFocusedActor() == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionBoxItemTest, "Looter.Interaction.Box.Item",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionBoxItemTest::RunTest(const FString& Parameters)
{
	// A small mission item (Fenn's coins: a few centimetres) is looked at as a hand's size round it, grown up from the
	// surface it lies on, never down into it.
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("The engine's cube"), Cube))
	{
		return false;
	}
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AMissionItem* Coins = World->SpawnActor<AMissionItem>(FVector(150.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("An item"), Coins))
	{
		return false;
	}
	// 10 x 4 x 0.4 cm, lying on the floor (the cube's pivot is its middle).
	Coins->Mesh->SetStaticMesh(Cube);
	Coins->Mesh->SetRelativeScale3D(FVector(0.1, 0.04, 0.004));
	Coins->Mesh->SetRelativeLocation(FVector(0.0, 0.0, 0.2));
	const FBox Own = Coins->Mesh->Bounds.GetBox();
	const TOptional<FInteractionBox> Box = Coins->GetInteractionBox();
	if (!TestTrue(TEXT("It has its box"), Box.IsSet()))
	{
		return false;
	}
	const FBox Span = FBox::BuildAABB(Box->Transform.GetLocation(), Box->Extent);
	TestTrue(TEXT("At least a hand's size round it"), Box->Extent.X >= 16.0 && Box->Extent.Y >= 16.0 && Box->Extent.Z >= 10.0);
	TestTrue(TEXT("...from the surface it lies on up, never into the ledge under it"), FMath::IsNearlyEqual(Span.Min.Z, Own.Min.Z, 0.1));
	TestTrue(TEXT("...round its own middle"), FMath::IsNearlyEqual(Span.GetCenter().X, Own.GetCenter().X, 0.1)
		&& FMath::IsNearlyEqual(Span.GetCenter().Y, Own.GetCenter().Y, 0.1));

	// A big thing keeps its own size.
	Coins->Mesh->SetRelativeScale3D(FVector(0.6, 0.6, 0.4));
	const TOptional<FInteractionBox> Big = Coins->GetInteractionBox();
	TestTrue(TEXT("A big thing: its own bounds"), Big.IsSet() && FMath::IsNearlyEqual(Big->Extent.X, 30.0, 0.5) && FMath::IsNearlyEqual(Big->Extent.Z, 20.0, 0.5));
	return true;
}

#endif
