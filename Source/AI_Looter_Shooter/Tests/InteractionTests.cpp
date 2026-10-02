#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractableProp.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

using namespace InteractionTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionFocusTest, "Looter.Interaction.Focus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionFocusTest::RunTest(const FString& Parameters)
{
	// Choosing among candidates, as the interaction component does every frame: in reach, in the cone, in sight, the most
	// directly looked at with nearer winning near-ties.
	FInteractionView View;
	View.Location = Ahead(0.0);
	View.Direction = FVector::ForwardVector;
	View.ReachOrigin = View.Location;
	auto Candidate = [](const FVector& Where)
	{
		FInteractionCandidate Made;
		Made.Location = Where;
		Made.Reach = 250.f;
		return Made;
	};
	TArray<FInteractionCandidate> Candidates = {
		Candidate(FVector(-120.0, 0.0, EyeHeight)),  // 0: behind
		Candidate(Ahead(400.0)),                      // 1: ahead, out of reach
		Candidate(FVector(60.0, 200.0, EyeHeight)),   // 2: off to the side
		Candidate(Ahead(220.0)),                      // 3: ahead, farther
		Candidate(Ahead(140.0)),                      // 4: ahead, the nearest in front
		Candidate(FVector(100.0, 45.0, EyeHeight)),   // 5: near, but 24 degrees off
	};
	auto InSight = [](const FInteractionCandidate&) { return true; };
	TestEqual(TEXT("The nearest straight ahead"), InteractionFocus::Select(View, Candidates, 0.8f, InSight), 4);

	auto Hidden = [](const FInteractionCandidate& Each) { return !Each.Location.Equals(Ahead(140.0)); };
	TestEqual(TEXT("Out of sight: the one behind it"), InteractionFocus::Select(View, Candidates, 0.8f, Hidden), 3);

	Candidates[4].Options.bUsable = false;
	TestEqual(TEXT("Can't be used now: passed over"), InteractionFocus::Select(View, Candidates, 0.8f, InSight), 3);
	Candidates[4].Options.bUsable = true;

	FInteractionView TurnedView = View;
	TurnedView.Direction = -FVector::ForwardVector;
	TestEqual(TEXT("Turned around: the one that was behind"), InteractionFocus::Select(TurnedView, Candidates, 0.8f, InSight), 0);

	FInteractionView SkyView = View;
	SkyView.Direction = FVector::UpVector;
	TestEqual(TEXT("Looking at the sky: nothing"), InteractionFocus::Select(SkyView, Candidates, 0.8f, InSight), static_cast<int32>(INDEX_NONE));

	// The same in a level, through the component: what's there, what can be used, which way the player faces.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	AInteractableProp* Lantern = SpawnLantern(World, Ahead(100.0), /*bLit*/ true);
	AInteractableProp* Door = SpawnDoor(World, Ahead(150.0));
	AInteractableProp* Bell = SpawnBell(World, Ahead(220.0));
	AInteractableProp* Behind = SpawnDoor(World, FVector(-120.0, 0.0, EyeHeight));
	AInteractableProp* Far = SpawnBell(World, Ahead(400.0));
	if (!TestTrue(TEXT("Stand-in and props placed"), Player && Interaction && Lantern && Door && Bell && Behind && Far))
	{
		return false;
	}

	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The nearest usable thing in front (a lit lantern can't be lit)"), Interaction->GetFocusedActor() == Door);
	TestEqual(TEXT("Its words"), Interaction->GetFocusedOptions().TapPrompt.ToString(), FString(TEXT("Open the door")));
	TestTrue(TEXT("A door takes a tap"), Interaction->GetFocusedOptions().bTap && !Interaction->GetFocusedOptions().bHold);

	Door->bEnabled = false;
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The door turned off: the bell behind it"), Interaction->GetFocusedActor() == Bell);
	Door->bEnabled = true;

	Lantern->SetOn(false, /*bInstant*/ true);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The lantern put out is the nearest"), Interaction->GetFocusedActor() == Lantern);
	TestTrue(TEXT("A lantern is held to light"), Interaction->GetFocusedOptions().bHold && !Interaction->GetFocusedOptions().bTap
		&& FMath::IsNearlyEqual(Interaction->GetFocusedOptions().HoldSeconds, 1.5f));

	Face(Player, 180.0);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Turned around: the door behind"), Interaction->GetFocusedActor() == Behind);

	Face(Player, 90.0);
	Interaction->UpdateInteraction(0.f);
	TestNull(TEXT("Looking at nothing: no focus"), Interaction->GetFocusedActor());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionTapHoldTest, "Looter.Interaction.TapAndHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionTapHoldTest::RunTest(const FString& Parameters)
{
	// A thing with only a tap is used as the key goes down; one with a hold fills while the key is held and is used when
	// the hold runs its time, and a tap does nothing to it.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	AInteractableProp* Door = SpawnDoor(World, Ahead(150.0));
	// The bell stands to the left.
	AInteractableProp* Bell = SpawnBell(World, FVector(0.0, 150.0, EyeHeight));
	if (!TestTrue(TEXT("Stand-in, door and bell placed"), Player && Interaction && Door && Bell))
	{
		return false;
	}
	int32 Rings = 0;
	bool bRungByHold = false;
	Bell->OnUsedNative.AddLambda([&Rings, &bRungByHold](AInteractableProp&, AActor*, bool bHeld)
	{
		++Rings;
		bRungByHold = bHeld;
	});

	// The door: only a tap, used as the key goes down.
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Looking at the door"), Interaction->GetFocusedActor() == Door);
	Interaction->PressInteract();
	TestTrue(TEXT("Opened as the key went down"), Door->IsOn());
	TestFalse(TEXT("Nothing to hold for"), Interaction->IsHolding());
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Letting go changes nothing"), Door->IsOn());
	TestNull(TEXT("Swinging (its cooldown), it can't be used"), Interaction->GetFocusedActor());
	Door->Advance(0.5f);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Swung open, it can be used again"), Interaction->GetFocusedActor() == Door);
	TestEqual(TEXT("...to shut it"), Interaction->GetFocusedOptions().TapPrompt.ToString(), FString(TEXT("Close the door")));
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestFalse(TEXT("Shut again"), Door->IsOn());

	// The bell: only a hold.
	Face(Player, 90.0);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Looking at the bell"), Interaction->GetFocusedActor() == Bell);
	TestEqual(TEXT("Its words, for the hold"), Interaction->GetFocusedOptions().HoldPrompt.ToString(), FString(TEXT("Ring the bell")));
	Interaction->PressInteract();
	TestTrue(TEXT("Holding"), Interaction->IsHolding());
	TestEqual(TEXT("Nothing yet"), Rings, 0);
	Interaction->UpdateInteraction(0.5f);
	TestTrue(TEXT("Halfway through the hold"), FMath::IsNearlyEqual(Interaction->GetHoldProgress(), 0.5f, 0.01f));
	TestEqual(TEXT("Not rung halfway"), Rings, 0);
	Interaction->UpdateInteraction(0.6f);
	TestEqual(TEXT("Rung when the hold ran its time"), Rings, 1);
	TestTrue(TEXT("...as a hold"), bRungByHold);
	TestFalse(TEXT("The hold is over"), Interaction->IsHolding());
	TestEqual(TEXT("No hold to show"), Interaction->GetHoldProgress(), 0.f);
	Interaction->ReleaseInteract();
	TestEqual(TEXT("Letting go after doesn't ring it again"), Rings, 1);

	// A tap on the bell: nothing.
	Bell->Advance(2.f);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The bell can be rung again"), Interaction->GetFocusedActor() == Bell);
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.1f);
	Interaction->ReleaseInteract();
	TestEqual(TEXT("A tap doesn't ring it"), Rings, 1);

	// Nothing looked at: the key does nothing.
	Face(Player, -90.0);
	Interaction->UpdateInteraction(0.f);
	Interaction->PressInteract();
	TestFalse(TEXT("A press on nothing holds nothing"), Interaction->IsHolding());
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Nothing was used"), Rings == 1 && !Door->IsOn());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionHoldCancelTest, "Looter.Interaction.HoldCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionHoldCancelTest::RunTest(const FString& Parameters)
{
	// A hold ends without a use when the key comes up early or the player looks away, and looking back doesn't bring it
	// back. Something else coming close doesn't take a hold away.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	AInteractableProp* Bell = SpawnBell(World, Ahead(180.0));
	if (!TestTrue(TEXT("Stand-in and bell placed"), Player && Interaction && Bell))
	{
		return false;
	}
	int32 Rings = 0;
	Bell->OnUsedNative.AddLambda([&Rings](AInteractableProp&, AActor*, bool) { ++Rings; });

	// Let go early.
	Interaction->UpdateInteraction(0.f);
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.5f);
	Interaction->ReleaseInteract();
	TestEqual(TEXT("Let go early: not rung"), Rings, 0);
	TestFalse(TEXT("No hold left"), Interaction->IsHolding());
	TestEqual(TEXT("The bar is empty"), Interaction->GetHoldProgress(), 0.f);

	// Look away, then back, still holding the key.
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.5f);
	Face(Player, 90.0);
	Interaction->UpdateInteraction(0.f);
	TestFalse(TEXT("Looked away: the hold is gone"), Interaction->IsHolding());
	Face(Player, 0.0);
	Interaction->UpdateInteraction(0.6f);
	TestTrue(TEXT("Looking back at the bell"), Interaction->GetFocusedActor() == Bell);
	TestFalse(TEXT("...doesn't bring the hold back"), Interaction->IsHolding());
	Interaction->UpdateInteraction(1.f);
	Interaction->ReleaseInteract();
	TestEqual(TEXT("Never rung"), Rings, 0);

	// A door turning up nearer, right in front of it, doesn't take the hold away (the key is held on the bell).
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.4f);
	AInteractableProp* Door = SpawnDoor(World, Ahead(100.0));
	Interaction->UpdateInteraction(0.4f);
	TestTrue(TEXT("Still holding on the bell"), Door && Interaction->IsHolding() && Interaction->GetFocusedActor() == Bell);
	Interaction->UpdateInteraction(0.4f);
	TestEqual(TEXT("Rung once the hold ran its time"), Rings, 1);
	Interaction->ReleaseInteract();
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The bell ringing, the door is what's looked at"), Interaction->GetFocusedActor() == Door);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionMissionEventTest, "Looter.Interaction.MissionEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionMissionEventTest::RunTest(const FString& Parameters)
{
	// Every use that does something tells the mission runner once (FMissionEvent::Interaction), held or tapped; a press
	// that uses nothing tells it nothing.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	if (!TestNotNull(TEXT("The level has a mission runner"), Runner))
	{
		return false;
	}

	// One mission counting every Interact event, and one needing the bell held.
	const FName UsesId(TEXT("TestUses"));
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Uses = MissionTestWorld::NewMission(Scratch, TEXT("TestUses"), EMissionKind::Side, EMissionStart::Automatic, TEXT("TestValley"));
	UMissionEventObjective* Counted = MissionTestWorld::AddObjective<UMissionEventObjective>(Uses, 0);
	Counted->Event = FMissionEvent::Interact;
	Counted->Count = 5;
	UMissionInteractObjective* Rung = MissionTestWorld::AddObjective<UMissionInteractObjective>(Uses, 0);
	Rung->Target.ActorTag = TEXT("Bell");
	Rung->bHold = true;

	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	AInteractableProp* Door = SpawnDoor(World, Ahead(150.0));
	AInteractableProp* Bell = SpawnBell(World, FVector(0.0, 150.0, EyeHeight));
	if (!TestTrue(TEXT("Stand-in, door and bell placed"), Player && Interaction && Door && Bell))
	{
		return false;
	}
	Runner->BeginForTesting({ Uses }, Campaign, Player, TEXT("TestValley"));
	Runner->Update(0.f);
	TestTrue(TEXT("The counting mission runs"), Runner->IsRunning(UsesId));
	auto Counts = [Runner, &UsesId]()
	{
		const TArray<FMissionObjectiveView> Views = Runner->GetObjectiveViews(UsesId);
		return Views.IsEmpty() ? FString(TEXT("not running")) : Views[0].Progress;
	};
	auto BellHeld = [Runner, &UsesId]()
	{
		const TArray<FMissionObjectiveView> Views = Runner->GetObjectiveViews(UsesId);
		return Views.Num() > 1 && Views[1].bDone;
	};

	// A tap on the door: one event.
	Interaction->UpdateInteraction(0.f);
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestEqual(TEXT("The door opened: one event"), Counts(), FString(TEXT("1 / 5")));

	// The door swinging can't be used: pressing on it does nothing, and tells nothing.
	Interaction->UpdateInteraction(0.f);
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestEqual(TEXT("Nothing used: no event"), Counts(), FString(TEXT("1 / 5")));

	// The bell let go early: no event. Held its time: one, as a hold.
	Face(Player, 90.0);
	Interaction->UpdateInteraction(0.f);
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.5f);
	Interaction->ReleaseInteract();
	TestEqual(TEXT("Let go early: no event"), Counts(), FString(TEXT("1 / 5")));
	TestFalse(TEXT("The bell isn't counted as held"), BellHeld());
	Interaction->PressInteract();
	Interaction->UpdateInteraction(1.1f);
	TestEqual(TEXT("Rung: one event"), Counts(), FString(TEXT("2 / 5")));
	TestTrue(TEXT("...a held one, about the bell"), BellHeld());
	Interaction->ReleaseInteract();
	TestEqual(TEXT("Letting go after: no more"), Counts(), FString(TEXT("2 / 5")));

	// The door again, once it's done swinging: one more.
	Door->Advance(1.f);
	Face(Player, 0.0);
	Interaction->UpdateInteraction(0.f);
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestEqual(TEXT("The door shut: one more event"), Counts(), FString(TEXT("3 / 5")));
	return true;
}

#endif
