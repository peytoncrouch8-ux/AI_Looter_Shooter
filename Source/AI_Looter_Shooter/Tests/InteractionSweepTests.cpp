#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionTypes.h"
#include "Tests/InteractionTestWorld.h"
#include "World/KeeperLanternPost.h"
#include "World/MissionFitting.h"
#include "World/OreLiftBellPull.h"
#include "World/ReadableProp.h"
#include "World/WatchFire.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"

// The 2026-10-23 play-check's third round: objectives that took pixel-exact aim. Abel's lantern posts (only from the canyon
// side), the Last Watch's watch fire (a low aim from one spot of twelve), Dead Lift's boiler (one spot of three) and pin
// (one of four, at 71 degrees up). Each is now looked at as a whole (IInteractable::GetInteractionBox), and what the sight
// line meets inside a thing's own box (the brazier's basket, the gantry round the clamp) never hides it. A test level's
// traces don't see new bodies: the focus checks here work from the cone; the sight rule is checked as plain math.

using namespace InteractionTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionSweepRulesTest, "Looter.Interaction.Box.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionSweepRulesTest::RunTest(const FString& Parameters)
{
	// A small thing grown to a hand's size, up from what it lies on; a big one keeps its own size.
	const FInteractionBox Small = InteractionFocus::GrownBox(FBox(FVector(-5.0, -2.0, 0.0), FVector(5.0, 2.0, 0.4)), 16.0, 10.0);
	TestTrue(TEXT("Small: a hand's size, from its bottom up"), Small.Extent.Equals(FVector(16.0, 16.0, 10.0), 0.01)
		&& Small.Transform.GetLocation().Equals(FVector(0.0, 0.0, 10.0), 0.01));
	const FInteractionBox Big = InteractionFocus::GrownBox(FBox(FVector(-50.0), FVector(50.0)), 16.0, 10.0);
	TestTrue(TEXT("Big: its own"), Big.Extent.Equals(FVector(50.0), 0.01) && Big.Transform.GetLocation().Equals(FVector::ZeroVector, 0.01));
	const FInteractionBox Turned = InteractionFocus::BoxAround(FVector(100.0, 0.0, 0.0), FVector(40.0, 5.0, 5.0), FRotator(0.0, 90.0, 0.0).Quaternion());
	TestTrue(TEXT("A box turned a quarter reaches along Y"), Turned.Contains(FVector(100.0, 35.0, 0.0)) && !Turned.Contains(FVector(135.0, 0.0, 0.0)));

	// The sight rule: a hit inside the thing's own box (grown by the sight tolerance) doesn't hide it; one outside does.
	FInteractionCandidate Fire;
	Fire.Location = FVector(300.0, 0.0, 100.0);
	TestTrue(TEXT("Without a box: a hit 40 cm short of its point hides it"),
		InteractionFocus::DoesHitHide(Fire, nullptr, nullptr, Fire.Location - FVector(40.0, 0.0, 0.0)));
	Fire.Box = InteractionFocus::BoxAround(Fire.Location, FVector(55.0, 55.0, 50.0));
	TestFalse(TEXT("With its basket's box: the basket's rim 40 cm short doesn't"),
		InteractionFocus::DoesHitHide(Fire, nullptr, nullptr, Fire.Location - FVector(40.0, 0.0, 0.0)));
	TestFalse(TEXT("...nor anything just outside it (the tolerance)"),
		InteractionFocus::DoesHitHide(Fire, nullptr, nullptr, Fire.Location - FVector(65.0, 0.0, 0.0)));
	TestTrue(TEXT("...but a wall a metre short still does"),
		InteractionFocus::DoesHitHide(Fire, nullptr, nullptr, Fire.Location - FVector(100.0, 0.0, 0.0)));

	// Met by the crosshair's line beats gone through by it beats near it: Dead Lift's pin, met by the line 3.2 m off, over
	// the bell pull's rope box the line clips 1.1 m off (the 2026-10-23 play-check: the rope took the key from the pin's
	// landing).
	FInteractionView View;
	View.Direction = FVector::ForwardVector;
	FInteractionCandidate Pin;
	Pin.Location = FVector(320.0, 0.0, 0.0);
	Pin.Reach = 390.f;
	Pin.LineRank = 2;
	FInteractionCandidate Rope;
	Rope.Location = FVector(110.0, 3.0, 0.0);
	Rope.Reach = 250.f;
	Rope.LineRank = 1;
	const TArray<FInteractionCandidate> Both = { Rope, Pin };
	TestEqual(TEXT("The pin the line meets, not the rope box it clips"),
		InteractionFocus::Select(View, Both, 0.8f, [](const FInteractionCandidate&) { return true; }), 1);
	TArray<FInteractionCandidate> Neither = Both;
	Neither[0].LineRank = 0;
	Neither[1].LineRank = 0;
	TestEqual(TEXT("...neither on it: the nearer, about as centred"),
		InteractionFocus::Select(View, Neither, 0.8f, [](const FInteractionCandidate&) { return true; }), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionSweepThingsTest, "Looter.Interaction.Box.Sweep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionSweepThingsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	if (!TestTrue(TEXT("The stand-in"), Player && Interaction))
	{
		return false;
	}

	// The watch fire: its whole basket and wood, reached a little past hands' reach (it hangs out past the deck's rail).
	AWatchFire* Fire = World->SpawnActor<AWatchFire>(FVector(0.0, 3000.0, 0.0), FRotator::ZeroRotator);
	const TOptional<FInteractionBox> Basket = Fire ? Fire->GetInteractionBox() : TOptional<FInteractionBox>();
	TestTrue(TEXT("Watch fire: its basket's box round the fire bed and the point looked at"), Basket.IsSet()
		&& Basket->Contains(Fire->GetActorLocation()) && Basket->Contains(Fire->GetInteractionLocation().Get(FVector::ZeroVector))
		&& Basket->Contains(Fire->GetActorLocation() + FVector(36.0, 0.0, 27.0)) && Basket->Contains(Fire->GetActorLocation() - FVector(0.0, 0.0, 20.0)));
	TestTrue(TEXT("...reached from 3.2 m"), Fire && FMath::IsNearlyEqual(Fire->Reach, 320.f));

	// Dead Lift's pin: 2.9 m up the gantry, looked up at 71 degrees from under it, and 8 degrees off that.
	AMissionFitting* Pin = World->SpawnActor<AMissionFitting>(FVector(100.0, 0.0, EyeHeight + 290.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The pin's fitting"), Pin))
	{
		return false;
	}
	Pin->Reach = 390.f;
	Pin->bHold = true;
	Pin->DispatchBeginPlay();
	const TOptional<FInteractionBox> Clamp = Pin->GetInteractionBox();
	TestTrue(TEXT("Pin: a hand's room round the clamp"), Clamp.IsSet() && Clamp->Contains(Pin->GetActorLocation() + FVector(30.0, -30.0, 30.0)));
	for (const double Pitch : { 71.0, 64.0 })
	{
		Player->SetActorLocationAndRotation(FVector::ZeroVector, FRotator(Pitch, 0.0, 0.0));
		Interaction->UpdateInteraction(0.f);
		TestTrue(FString::Printf(TEXT("Pin looked up at, %.0f degrees: \"hold to drive\""), Pitch), Interaction->GetFocusedActor() == Pin
			&& Interaction->GetFocusedOptions().bHold);
	}
	Pin->Destroy();

	// Dead Lift's boiler: the firebox door, looked at a hand off its middle while Hiram talks beside it.
	AMissionFitting* Boiler = World->SpawnActor<AMissionFitting>(Ahead(160.0), FRotator(0.0, 180.0, 0.0));
	if (!TestNotNull(TEXT("The boiler's fitting"), Boiler))
	{
		return false;
	}
	Boiler->bHold = true;
	Boiler->DispatchBeginPlay();
	Player->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	const FVector HandOff = Ahead(160.0) + FVector(0.0, 28.0, -25.0);
	Player->SetActorRotation((HandOff - Ahead(0.0)).Rotation());
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Boiler: the firebox door's corner looked at"), Interaction->GetFocusedActor() == Boiler);
	Boiler->Destroy();

	// The bell pull: its handle and the rope up to the bell.
	AOreLiftBellPull* Pull = World->SpawnActor<AOreLiftBellPull>(FVector(0.0, -3000.0, 120.0), FRotator::ZeroRotator);
	const TOptional<FInteractionBox> Rope = Pull ? Pull->GetInteractionBox() : TOptional<FInteractionBox>();
	TestTrue(TEXT("Bell pull: the handle and the rope 1.2 m up it"), Rope.IsSet() && Rope->Contains(Pull->GetActorLocation())
		&& Rope->Contains(Pull->GetActorLocation() + FVector(0.0, 0.0, 120.0)));

	// A readable without a model of its own (the barricade's tags on their nail): a hand's room round its spot.
	AReadableProp* Tags = World->SpawnActor<AReadableProp>(FVector(3000.0, 0.0, 140.0), FRotator::ZeroRotator);
	const TOptional<FInteractionBox> Nail = Tags ? Tags->GetInteractionBox() : TOptional<FInteractionBox>();
	TestTrue(TEXT("A readable with no model: a box round its spot"), Nail.IsSet() && Nail->Contains(Tags->GetActorLocation() + FVector(20.0, 0.0, -20.0)));

	// Abel's lantern post: the whole post, its arm and lantern; relit looked at anywhere on it.
	AKeeperLanternPost* Post = World->SpawnActor<AKeeperLanternPost>(FVector(250.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A lantern post"), Post))
	{
		return false;
	}
	Post->DispatchBeginPlay();
	Post->SetDark(true);
	const TOptional<FInteractionBox> Whole = Post->GetInteractionBox();
	TestTrue(TEXT("Post: its box holds the lantern and the hook"), Whole.IsSet() && Whole->Contains(Post->GetInteractionLocation().Get(FVector::ZeroVector)));
	Player->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	if (Post->Post->GetStaticMesh())
	{
		TestTrue(TEXT("...and the post down to the deck"), Whole.IsSet() && Whole->Contains(FVector(250.0, 0.0, 40.0)));
		// From the deck side, looking at the post's column at chest height, the lantern high over the far side.
		Player->SetActorRotation((FVector(250.0, 0.0, 110.0) - Ahead(0.0)).Rotation());
		Interaction->UpdateInteraction(0.f);
		TestTrue(TEXT("The post's column looked at from the deck: \"relight\""), Interaction->GetFocusedActor() == Post
			&& Interaction->GetFocusedOptions().bHold);
	}
	else
	{
		AddInfo(TEXT("SM_KeeperLanternPost isn't in this checkout: the post's box is its lantern's alone."));
		Player->SetActorRotation((Post->GetInteractionLocation().Get(FVector::ZeroVector) - Ahead(0.0)).Rotation());
		Interaction->UpdateInteraction(0.f);
		TestTrue(TEXT("Its lantern looked at: \"relight\""), Interaction->GetFocusedActor() == Post);
	}
	return true;
}

#endif
