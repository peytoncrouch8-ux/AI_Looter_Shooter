#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/StoryCondition.h"
#include "Tests/MissionTestWorld.h"
#include "World/MissionItem.h"
#include "World/ReadableProp.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The mission gates (the user's playtest, 2026-10-27: "you can speed run quests and just spam click the use button"): a
// step's things work only from their step (an item's TakeWhen, a readable's UsableWhen), a step-only reading's prompt goes
// once it's done (Back Pay's "Take the tags"), lore stays a tap any time, and a mission's key actions take a short hold
// (Looter.Missions.HoldSeconds; 0 turns them back into taps).

using namespace MissionTestWorld;

namespace
{
	const FName Valley(TEXT("TestValley"));
	const TCHAR* MissionName = TEXT("TestGates");
	const FName PayId(TEXT("TestPayroll"));

	/** During the test mission from step From (and before step Before, when given), counted from 0. */
	FStoryCondition During(int32 From, int32 Before = 0)
	{
		FStoryCondition When;
		When.DuringMission = FName(MissionName);
		When.FromStep = From;
		When.BeforeStep = Before;
		return When;
	}

	/** Its prompt as the HUD shows it: the hold's words when it's held, else the tap's. */
	FString PromptOf(const FInteractionOptions& Options)
	{
		return (Options.bHold ? Options.HoldPrompt : Options.TapPrompt).ToString();
	}

	/** Sets Looter.Missions.HoldSeconds for a test and puts it back after. */
	struct FHoldSecondsScope
	{
		IConsoleVariable* Var = nullptr;
		float Old = 0.f;

		explicit FHoldSecondsScope(float Seconds)
		{
			Var = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Missions.HoldSeconds"));
			if (Var)
			{
				Old = Var->GetFloat();
				Var->Set(Seconds, ECVF_SetByCode);
			}
		}

		~FHoldSecondsScope()
		{
			if (Var)
			{
				Var->Set(Old, ECVF_SetByCode);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGateItemTest, "Looter.Missions.Gates.Item",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGateItemTest::RunTest(const FString& Parameters)
{
	FHoldSecondsScope Hold(0.6f);
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AMissionItem* Payroll = World->SpawnActor<AMissionItem>(FVector(300.0, 0.0, 0.0), FRotator::ZeroRotator);
	UInteractionComponent* Hands = Player ? NewObject<UInteractionComponent>(Player, TEXT("Hands")) : nullptr;
	if (!Runner || !Player || !Payroll || !Hands)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	// Step 0 the Foreman (an event), step 1 the payroll: seen from the Foreman's step, taken from its own.
	UMissionDefinition* Mission = NewMission(CreatePackage(nullptr), MissionName, EMissionKind::Side, EMissionStart::Automatic, Valley);
	FMissionItemInfo& Info = Mission->Items.AddDefaulted_GetRef();
	Info.Id = PayId;
	Info.Name = FText::FromString(TEXT("The payroll"));
	AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.Foreman");
	AddObjective<UMissionCollectItemObjective>(Mission, 1)->Items = { PayId };
	AddObjective<UMissionEventObjective>(Mission, 2)->Event = TEXT("Test.Home");
	Payroll->ItemId = PayId;
	Payroll->TakeWhen = During(1);
	Runner->BeginForTesting({ Mission }, Campaign, Player, Valley);
	Payroll->DispatchBeginPlay();
	Runner->Update(0.f);
	const FName Id(MissionName);
	if (!TestTrue(TEXT("The mission runs, on its first step"), Runner->IsRunning(Id) && Runner->GetStep(Id) == 0))
	{
		return false;
	}

	TestTrue(TEXT("Before its step: it's seen"), Payroll->IsShown());
	TestFalse(TEXT("...but can't be taken"), Payroll->CanTake());
	TestFalse(TEXT("...never focused: no prompt"), Payroll->GetInteractionOptions(*Hands).bUsable);
	TestFalse(TEXT("...and Take refuses it"), Payroll->Take(Player));
	TestFalse(TEXT("Nothing carried early"), Campaign.IsCarrying(PayId));

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Foreman")));
	TestEqual(TEXT("The Foreman down: the payroll's step"), Runner->GetStep(Id), 1);
	const FInteractionOptions Options = Payroll->GetInteractionOptions(*Hands);
	TestTrue(TEXT("Its step: it can be taken"), Payroll->CanTake() && Options.bUsable);
	TestTrue(TEXT("...with a short hold, not a tap"), Options.bHold && !Options.bTap && FMath::IsNearlyEqual(Options.HoldSeconds, 0.6f));
	TestEqual(TEXT("...its words on the hold"), PromptOf(Options), FString(TEXT("Take the payroll")));
	TestTrue(TEXT("Taken"), Payroll->Take(Player));
	TestEqual(TEXT("Carried, its step done"), Runner->GetStep(Id), 2);

	// The hold turned off: a tap, as before (the options are asked of it whether it lies there or not).
	{
		FHoldSecondsScope Tap(0.f);
		const FInteractionOptions Tapped = Payroll->GetInteractionOptions(*Hands);
		TestTrue(TEXT("Looter.Missions.HoldSeconds 0: a tap"), Tapped.bTap && !Tapped.bHold && !Tapped.TapPrompt.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGateReadableTest, "Looter.Missions.Gates.Readable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGateReadableTest::RunTest(const FString& Parameters)
{
	FHoldSecondsScope Hold(0.6f);
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AReadableProp* Tags = World->SpawnActor<AReadableProp>(FVector(300.0, 0.0, 0.0), FRotator::ZeroRotator);
	AReadableProp* Book = World->SpawnActor<AReadableProp>(FVector(300.0, 200.0, 0.0), FRotator::ZeroRotator);
	AReadableProp* Chalk = World->SpawnActor<AReadableProp>(FVector(300.0, -200.0, 0.0), FRotator::ZeroRotator);
	UInteractionComponent* Hands = Player ? NewObject<UInteractionComponent>(Player, TEXT("Hands")) : nullptr;
	if (!Runner || !Player || !Tags || !Book || !Chalk || !Hands)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	// Step 0 the drift (an event), 1 the payroll book read, 2 the tags taken, 3 the Foreman.
	UMissionDefinition* Mission = NewMission(CreatePackage(nullptr), MissionName, EMissionKind::Side, EMissionStart::Automatic, Valley);
	AddObjective<UMissionEventObjective>(Mission, 0)->Event = TEXT("Test.Drift");
	AddObjective<UMissionEventObjective>(Mission, 1)->Event = AReadableProp::EventFor(TEXT("TestBook"));
	AddObjective<UMissionEventObjective>(Mission, 2)->Event = AReadableProp::EventFor(TEXT("TestTags"));
	AddObjective<UMissionEventObjective>(Mission, 3)->Event = TEXT("Test.Foreman");
	// The book: read from its step on, and after the mission as lore. The tags: in their step alone. The chalk: lore.
	Book->ReadableId = TEXT("TestBook");
	Book->Text = FText::FromString(TEXT("PAYROLL"));
	Book->bHoldToRead = true;
	FStoryCondition After;
	After.AfterMissions = { FName(MissionName) };
	Book->UsableWhen = { During(1), After };
	Tags->ReadableId = TEXT("TestTags");
	Tags->Prompt = FText::FromString(TEXT("Take the tags"));
	Tags->bHoldToRead = true;
	Tags->UsableWhen = { During(2, 3) };
	Chalk->ReadableId = TEXT("TestChalk");
	Chalk->Text = FText::FromString(TEXT("DAY 1"));
	Runner->BeginForTesting({ Mission }, Campaign, Player, Valley);
	Tags->DispatchBeginPlay();
	Book->DispatchBeginPlay();
	Chalk->DispatchBeginPlay();
	Runner->Update(0.f);
	const FName Id(MissionName);
	if (!TestTrue(TEXT("The mission runs, on its first step"), Runner->IsRunning(Id) && Runner->GetStep(Id) == 0))
	{
		return false;
	}

	// Lore: a tap, any time.
	const FInteractionOptions ChalkOptions = Chalk->GetInteractionOptions(*Hands);
	TestTrue(TEXT("Lore: read any time, with a tap"), ChalkOptions.bUsable && ChalkOptions.bTap && !ChalkOptions.bHold
		&& PromptOf(ChalkOptions) == TEXT("Read"));

	// Before their steps: seen, never focused, never read.
	TestTrue(TEXT("Before its step the book is seen"), Book->IsShown());
	TestFalse(TEXT("...but not read"), Book->CanRead() || Book->GetInteractionOptions(*Hands).bUsable);
	TestFalse(TEXT("...Read refuses it"), Book->Read(Player));
	TestFalse(TEXT("The tags: not before their step"), Tags->GetInteractionOptions(*Hands).bUsable || Tags->Read(Player));

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Drift")));
	TestEqual(TEXT("The book's step"), Runner->GetStep(Id), 1);
	const FInteractionOptions BookOptions = Book->GetInteractionOptions(*Hands);
	TestTrue(TEXT("Its step: the book is read, with a short hold"), BookOptions.bUsable && BookOptions.bHold && !BookOptions.bTap
		&& PromptOf(BookOptions) == TEXT("Read"));
	TestFalse(TEXT("The tags still wait for theirs"), Tags->GetInteractionOptions(*Hands).bUsable);
	TestTrue(TEXT("Read"), Book->Read(Player));
	TestEqual(TEXT("...its step done"), Runner->GetStep(Id), 2);
	TestTrue(TEXT("A document stays readable after its step"), Book->GetInteractionOptions(*Hands).bUsable);

	const FInteractionOptions TagOptions = Tags->GetInteractionOptions(*Hands);
	TestTrue(TEXT("The tags' step: \"Take the tags\", held"), TagOptions.bUsable && TagOptions.bHold
		&& PromptOf(TagOptions) == TEXT("Take the tags"));
	TestTrue(TEXT("Taken"), Tags->Read(Player));
	TestEqual(TEXT("...their step done"), Runner->GetStep(Id), 3);
	TestFalse(TEXT("Taken, the tags' prompt is gone (Back Pay, 2026-10-27)"), Tags->GetInteractionOptions(*Hands).bUsable);
	TestFalse(TEXT("...and never taken twice"), Tags->Read(Player));

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Foreman")));
	TestTrue(TEXT("The mission done"), Campaign.HasCompleted(Id));
	TestTrue(TEXT("After it the book is lore: still read"), Book->CanRead() && Book->GetInteractionOptions(*Hands).bUsable);
	TestFalse(TEXT("The tags stay done"), Tags->GetInteractionOptions(*Hands).bUsable);
	return true;
}

#endif
