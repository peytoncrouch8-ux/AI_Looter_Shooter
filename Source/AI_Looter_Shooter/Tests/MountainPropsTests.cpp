#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/StoryCondition.h"
#include "Tests/MissionTestWorld.h"
#include "World/MissionFitting.h"
#include "World/StoryProp.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The mountain's story props (wave 4, Tools/Unreal/build_area_mountain.py and build_area_missionprops_mountain.py): the
// models the story shows and hides (the tag board's tags on IN until the eleven are paid, then on OUT; the barricade's
// tags until taken) and the places a mission has something fitted (Dead Lift's crank, coal and clamp pin), open only in
// their step and done for good after it.

using namespace MissionTestWorld;

namespace
{
	FStoryCondition WhenFlag(FName Flag, bool bSet)
	{
		FStoryCondition When;
		(bSet ? When.WithFlags : When.WithoutFlags).Add(Flag);
		return When;
	}

	FStoryCondition WhenDuring(FName Mission, int32 FromStep, int32 BeforeStep = 0)
	{
		FStoryCondition When;
		When.DuringMission = Mission;
		When.FromStep = FromStep;
		When.BeforeStep = BeforeStep;
		return When;
	}

	FStoryCondition WhenAfter(FName Mission)
	{
		FStoryCondition When;
		When.AfterMissions.Add(Mission);
		return When;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryPropRuleTest, "Looter.World.StoryProp.Rule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoryPropRuleTest::RunTest(const FString& Parameters)
{
	// Shown while any of ShownWhen holds (none: always) and none of HiddenWhen does; without a campaign only an unconditioned one.
	const FName Paid(TEXT("Test.Paid"));
	FCampaignRecord Campaign;
	const TArray<FStoryCondition> Nothing;
	TestTrue(TEXT("No conditions, no campaign: shown"), AStoryProp::Applies(Nothing, Nothing, nullptr, nullptr));
	TestFalse(TEXT("A condition and no campaign: hidden"), AStoryProp::Applies({ WhenFlag(Paid, true) }, Nothing, nullptr, nullptr));
	TestTrue(TEXT("IN's tags before the pay"), AStoryProp::Applies({ WhenFlag(Paid, false) }, Nothing, &Campaign, nullptr));
	TestFalse(TEXT("OUT's tags before the pay"), AStoryProp::Applies({ WhenFlag(Paid, true) }, Nothing, &Campaign, nullptr));
	Campaign.SetFlag(Paid);
	TestFalse(TEXT("IN's tags once paid"), AStoryProp::Applies({ WhenFlag(Paid, false) }, Nothing, &Campaign, nullptr));
	TestTrue(TEXT("OUT's tags once paid"), AStoryProp::Applies({ WhenFlag(Paid, true) }, Nothing, &Campaign, nullptr));
	TestTrue(TEXT("Any one of ShownWhen will do"),
		AStoryProp::Applies({ WhenAfter(TEXT("TestNeverDone")), WhenFlag(Paid, true) }, Nothing, &Campaign, nullptr));
	Campaign.Complete(TEXT("TestBackPay"));
	TestFalse(TEXT("HiddenWhen wins over ShownWhen"),
		AStoryProp::Applies({ WhenFlag(Paid, true) }, { WhenAfter(TEXT("TestBackPay")) }, &Campaign, nullptr));
	TestTrue(TEXT("...only while it holds"), AStoryProp::Applies(Nothing, { WhenAfter(TEXT("TestNeverDone")) }, &Campaign, nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryPropLevelTest, "Looter.World.StoryProp.Level",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStoryPropLevelTest::RunTest(const FString& Parameters)
{
	// The tag board's two sets of tags in a level: IN until the eleven are paid, OUT after, swapped as the flag is set.
	const FName Paid(TEXT("Test.Paid"));
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AStoryProp* TagsIn = World->SpawnActor<AStoryProp>(FVector(300.0, 0.0, 0.0), FRotator::ZeroRotator);
	AStoryProp* TagsOut = World->SpawnActor<AStoryProp>(FVector(300.0, 0.0, 0.0), FRotator::ZeroRotator);
	AStoryProp* Always = World->SpawnActor<AStoryProp>(FVector(300.0, 200.0, 0.0), FRotator::ZeroRotator);
	if (!Runner || !Player || !TagsIn || !TagsOut || !Always)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	TagsIn->ShownWhen = { WhenFlag(Paid, false) };
	TagsOut->ShownWhen = { WhenFlag(Paid, true) };
	Runner->BeginForTesting({}, Campaign, Player);
	TagsIn->DispatchBeginPlay();
	TagsOut->DispatchBeginPlay();
	Always->DispatchBeginPlay();
	Runner->Update(0.f);

	TestTrue(TEXT("Before the pay: the tags hang on IN"), TagsIn->IsShown() && !TagsIn->IsHidden());
	TestTrue(TEXT("...and none on OUT"), !TagsOut->IsShown() && TagsOut->IsHidden());
	TestTrue(TEXT("Nothing collides with a story prop"), TagsIn->Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestTrue(TEXT("Paid"), Runner->SetFlag(Paid));
	Runner->Update(0.f);
	TestTrue(TEXT("...the tags go on OUT"), TagsOut->IsShown() && !TagsOut->IsHidden());
	TestTrue(TEXT("...and IN's hooks are bare"), !TagsIn->IsShown() && TagsIn->IsHidden());
	TestTrue(TEXT("One without conditions stays"), Always->IsShown() && !Always->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionFittingTest, "Looter.World.MissionFitting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionFittingTest::RunTest(const FString& Parameters)
{
	// Dead Lift's way: the fittings open only in their step (its second, from 0: 1), a held use fits one for good and shows
	// it, and from the next step on (or after the mission) they're done whether fitted here or not (a session loaded then).
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	UInteractionComponent* Hands = Player ? NewObject<UInteractionComponent>(Player, TEXT("Hands")) : nullptr;
	AMissionFitting* Crank = World->SpawnActor<AMissionFitting>(FVector(200.0, 0.0, 100.0), FRotator::ZeroRotator);
	AMissionFitting* Pin = World->SpawnActor<AMissionFitting>(FVector(200.0, 300.0, 100.0), FRotator::ZeroRotator);
	AMissionFitting* Chest = World->SpawnActor<AMissionFitting>(FVector(200.0, -300.0, 0.0), FRotator::ZeroRotator);
	if (!Runner || !Player || !Hands || !Crank || !Pin || !Chest)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	// Never registered: the fittings only read it as their user (what can be used now), and it mustn't look about the level.
	const FName LiftId(TEXT("TestLift"));
	UMissionDefinition* Lift = NewMission(CreatePackage(nullptr), TEXT("TestLift"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	for (int32 Step = 0; Step < 3; ++Step)
	{
		AddObjective<UMissionEventObjective>(Lift, Step)->Event = FName(*FString::Printf(TEXT("Test.Lift.%d"), Step));
	}
	for (AMissionFitting* Fitting : { Crank, Pin })
	{
		Fitting->UsableWhen = WhenDuring(LiftId, 1, 2);
		Fitting->DoneWhen = { WhenDuring(LiftId, 2), WhenAfter(LiftId) };
		Fitting->Tags.Add(FName(TEXT("LiftFitting")));
	}
	Chest->bHideUntilDone = false;
	Chest->UsableWhen = WhenDuring(LiftId, 2);
	Runner->BeginForTesting({ Lift }, Campaign, Player);
	Crank->DispatchBeginPlay();
	Pin->DispatchBeginPlay();
	Chest->DispatchBeginPlay();
	Runner->Update(0.f);

	TestTrue(TEXT("Its first step: the crank can't be fitted yet"), !Crank->CanFit() && !Crank->GetInteractionOptions(*Hands).CanUse());
	TestFalse(TEXT("...nor is it seen"), Crank->Mesh->IsVisible());
	TestTrue(TEXT("...a chest is seen all the same"), Chest->Mesh->IsVisible() && !Chest->CanFit());
	TestEqual(TEXT("...the hand goes to its own spot"), Crank->GetInteractionLocation().Get(FVector::ZeroVector), Crank->GetActorLocation());

	TestTrue(TEXT("The fittings' step"), Runner->SetStep(LiftId, 1));
	Runner->Update(0.f);
	TestTrue(TEXT("...the crank can be fitted, with a hold"), Crank->CanFit() && Crank->GetInteractionOptions(*Hands).bHold
		&& Crank->GetInteractionOptions(*Hands).CanUse());
	TestTrue(TEXT("Fitted"), Crank->Use(Player, true));
	TestTrue(TEXT("...for good: done, used up, seen"), Crank->IsDone() && Crank->IsUsedUp() && Crank->Mesh->IsVisible());
	TestFalse(TEXT("...and never offered again"), Crank->GetInteractionOptions(*Hands).CanUse());
	TestTrue(TEXT("The pin not yet"), !Pin->IsDone() && Pin->CanFit() && !Pin->Mesh->IsVisible());

	TestTrue(TEXT("The next step"), Runner->SetStep(LiftId, 2));
	Runner->Update(0.f);
	TestTrue(TEXT("...the pin is done by the story (a session loaded then)"), Pin->IsDone() && Pin->IsUsedUp() && Pin->Mesh->IsVisible());
	TestFalse(TEXT("...and can't be fitted twice"), Pin->CanFit());
	TestTrue(TEXT("...the chest opens in its own step"), Chest->CanFit());
	return true;
}

#endif
