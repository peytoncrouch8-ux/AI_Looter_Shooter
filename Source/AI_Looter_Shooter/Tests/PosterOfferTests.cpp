#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/MissionArtifact.h"
#include "World/WantedPoster.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Side 1's offer sits behind the town gate's poster (play-through of 2026-10-14): the poster (hold to tear) and the artifact
// over its paper (tap to read, offers the mission) stood at one spot and the key's focus landed on the poster's hold, so a tap
// did nothing. Until the artifact's mission is taken, the poster is read, not torn.

using namespace MissionTestWorld;

namespace
{
	const FName GateTag(TEXT("Artifact_TestGate"));
	const FName LaterTag(TEXT("Artifact_TestLater"));

	/** A mission of one step, offered by the artifact tagged Giver. */
	UMissionDefinition* MakeGateJob(UObject* Outer, const TCHAR* Id, FName Giver)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, EMissionKind::Side, EMissionStart::Offered, NAME_None);
		AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.Never");
		Mission->Offer.GiverTag = Giver;
		Mission->TurnIn.bAutomatic = true;
		return Mission;
	}

	/** A paper on a wall at Where, facing back toward the stand-in at the origin, not yet begun (the artifact goes down first). */
	AWantedPoster* SpawnPaper(UWorld* World, const FVector& Where)
	{
		return World->SpawnActor<AWantedPoster>(Where, FRotator(0.0, 180.0, 0.0));
	}

	/** An artifact laid over a paper as the placing script lays it (1.5 cm in front, the same way round), tagged Giver, nailed up. */
	AMissionArtifact* SpawnCardOver(UWorld* World, const AWantedPoster& Paper, FName Giver)
	{
		const FTransform Frame = Paper.GetActorTransform();
		AMissionArtifact* Card = World->SpawnActor<AMissionArtifact>(Frame.TransformPosition(FVector(1.5, 0.0, 0.0)), Paper.GetActorRotation());
		if (Card)
		{
			Card->Tags.Add(Giver);
			Card->bTakeOnAccept = false;
		}
		return Card;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPosterGateOfferTest, "Looter.Poster.GateOffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPosterGateOfferTest::RunTest(const FString& Parameters)
{
	// A wanted poster with an offering artifact over it takes no hold until that mission is accepted (or finished), whether
	// or not the mission can be offered yet; the key's focus then passes to the artifact. Once it is taken, the poster tears
	// as any other. A poster with nothing over it is unchanged.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);

	// The gate's poster ahead of the player with its card over it; a second pair off to the side whose mission isn't offered
	// yet (its prerequisite is never done); a bare poster with nothing over it.
	AWantedPoster* Gate = SpawnPaper(World, InteractionTestWorld::Ahead(150.0));
	AWantedPoster* Side = SpawnPaper(World, FVector(0.0, 700.0, 64.0));
	AWantedPoster* Bare = SpawnPaper(World, FVector(0.0, -700.0, 64.0));
	if (!Runner || !Player || !Interaction || !Gate || !Side || !Bare)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	AMissionArtifact* GateCard = SpawnCardOver(World, *Gate, GateTag);
	AMissionArtifact* SideCard = SpawnCardOver(World, *Side, LaterTag);
	if (!TestTrue(TEXT("Both cards laid"), GateCard && SideCard))
	{
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* GateJob = MakeGateJob(Scratch, TEXT("TestGateJob"), GateTag);
	UMissionDefinition* LaterJob = MakeGateJob(Scratch, TEXT("TestLaterJob"), LaterTag);
	LaterJob->Prerequisites = { FName(TEXT("TestNever")) };
	Runner->BeginForTesting({ GateJob, LaterJob }, Campaign, Player);

	// Play begins: the cards first, then the papers (a paper looks for the card over it as it begins).
	GateCard->DispatchBeginPlay();
	SideCard->DispatchBeginPlay();
	Gate->DispatchBeginPlay();
	Side->DispatchBeginPlay();
	Bare->DispatchBeginPlay();
	Runner->Update(0.f);

	// The mission is on offer: read first. The poster offers no hold, so the focus is the card's.
	TestTrue(TEXT("The gate's poster waits for its offer"), Gate->IsAwaitingOffer());
	const FInteractionOptions GateOptions = Gate->GetInteractionOptions(*Interaction);
	TestFalse(TEXT("...and can't be used"), GateOptions.CanUse());
	TestTrue(TEXT("The card over it can"), GateCard->GetInteractionOptions(*Interaction).CanUse());
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The key's focus is the card, not the poster"), Interaction->GetFocusedActor() == GateCard);
	TestFalse(TEXT("A hold that reaches the poster tears nothing"), Gate->Interact(*Interaction, /*bHeld*/ true) || Gate->IsTorn());

	// Not on offer yet (what comes before it isn't done): still the card's paper, not for tearing.
	TestTrue(TEXT("A paper whose mission isn't on offer yet waits too"), Side->IsAwaitingOffer()
		&& !Side->GetInteractionOptions(*Interaction).CanUse());
	// A paper with nothing over it is as it always was.
	TestFalse(TEXT("A bare poster doesn't wait"), Bare->IsAwaitingOffer());
	TestTrue(TEXT("...and is torn by a hold"), Bare->GetInteractionOptions(*Interaction).CanUse() && Bare->GetInteractionOptions(*Interaction).bHold);

	// The mission taken: the poster tears as the others do, and the focus comes to it.
	TestTrue(TEXT("The mission is accepted"), Runner->AcceptMission(TEXT("TestGateJob")));
	TestFalse(TEXT("Taken: the poster no longer waits"), Gate->IsAwaitingOffer());
	TestFalse(TEXT("...the card has nothing more to offer"), GateCard->GetInteractionOptions(*Interaction).CanUse());
	const FInteractionOptions TornOptions = Gate->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("...the poster takes a hold, and only a hold"), TornOptions.CanUse() && TornOptions.bHold && !TornOptions.bTap);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The key's focus is the poster now"), Interaction->GetFocusedActor() == Gate);
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.35f);
	Interaction->UpdateInteraction(0.3f);
	TestTrue(TEXT("Held its 0.6 s: torn down"), Gate->IsTorn());
	Interaction->ReleaseInteract();

	// A mission finished without being played (an old save, the console) frees its paper too.
	Campaign.Complete(TEXT("TestLaterJob"));
	TestFalse(TEXT("A finished mission frees its poster"), Side->IsAwaitingOffer());
	return true;
}

#endif
