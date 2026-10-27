#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionText.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "World/MissionItem.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// A mission item's name inside a sentence (the play-through of 2026-10-14 read "Take The clamp pin"): the data writes the
// name as a title ("The clamp pin", for the Missions page's Carried list), so after a verb its article is lower-cased.

using namespace MissionTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionItemPromptTest, "Looter.Missions.ItemPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionItemPromptTest::RunTest(const FString& Parameters)
{
	// The rule: a leading The, A or An lower-cased; the rest as written.
	TestEqual(TEXT("The clamp pin"), MissionText::InSentence(FString(TEXT("The clamp pin"))), FString(TEXT("the clamp pin")));
	TestEqual(TEXT("...her capital kept"), MissionText::InSentence(FString(TEXT("The Gravemother's fang"))), FString(TEXT("the Gravemother's fang")));
	TestEqual(TEXT("A miner's tag"), MissionText::InSentence(FString(TEXT("A miner's tag"))), FString(TEXT("a miner's tag")));
	TestEqual(TEXT("An old key"), MissionText::InSentence(FString(TEXT("An old key"))), FString(TEXT("an old key")));
	TestEqual(TEXT("A name that only starts like one: kept"), MissionText::InSentence(FString(TEXT("Theodore's watch"))), FString(TEXT("Theodore's watch")));
	TestEqual(TEXT("A name with no article: kept"), MissionText::InSentence(FString(TEXT("Pa's lantern bail"))), FString(TEXT("Pa's lantern bail")));
	TestEqual(TEXT("Already in a sentence's case: kept"), MissionText::InSentence(FString(TEXT("the winch crank"))), FString(TEXT("the winch crank")));
	TestEqual(TEXT("The word alone: kept"), MissionText::InSentence(FString(TEXT("The "))), FString(TEXT("The ")));
	const FText Plain = FText::FromString(TEXT("Pa's lantern bail"));
	TestTrue(TEXT("Unchanged, the text is the name's own"), MissionText::InSentence(Plain).IdenticalTo(Plain));

	// In the world: the pickup's prompt, the objective's line, and the name as a title where it's one.
	const FName PinId(TEXT("TestClampPin"));
	const FName WrenTag(TEXT("Speaker_Wren"));
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AMissionItem* Pin = World->SpawnActor<AMissionItem>(FVector(400.0, 0.0, 0.0), FRotator::ZeroRotator);
	UInteractionComponent* Hands = Player ? NewObject<UInteractionComponent>(Player, TEXT("Hands")) : nullptr;
	if (!Runner || !Player || !Pin || !Hands)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UMissionDefinition* Lift = NewMission(CreatePackage(nullptr), TEXT("TestPinLift"), EMissionKind::Side, EMissionStart::Offered, NAME_None);
	FMissionItemInfo& Info = Lift->Items.AddDefaulted_GetRef();
	Info.Id = PinId;
	Info.Name = FText::FromString(TEXT("The clamp pin"));
	UMissionCollectItemObjective* Collect = AddObjective<UMissionCollectItemObjective>(Lift, 0);
	Collect->Items = { PinId };
	Lift->Offer.GiverTag = WrenTag;
	Lift->TurnIn.SpeakerTag = WrenTag;
	Pin->ItemId = PinId;
	Runner->BeginForTesting({ Lift }, Campaign, Player);
	Pin->DispatchBeginPlay();
	Runner->Update(0.f);
	TestTrue(TEXT("Its mission taken: the pin is there"), Runner->AcceptMission(Lift->GetMissionId()) && Pin->IsShown());
	// Taken with a hold since the mission gates (2026-10-27): its words are the hold's prompt.
	auto PromptOf = [Pin, Hands]()
	{
		const FInteractionOptions Options = Pin->GetInteractionOptions(*Hands);
		return (Options.bHold ? Options.HoldPrompt : Options.TapPrompt).ToString();
	};
	TestEqual(TEXT("Its prompt: \"Take the clamp pin\""), PromptOf(), FString(TEXT("Take the clamp pin")));
	TestEqual(TEXT("Its objective: \"Find the clamp pin\""), Collect->GetDisplayText(World), FString(TEXT("Find the clamp pin")));
	TestEqual(TEXT("Its name, a title, as the data writes it"), Pin->GetItemName().ToString(), FString(TEXT("The clamp pin")));

	// A prompt of its own is said as written.
	Pin->Prompt = FText::FromString(TEXT("Pull the clamp pin"));
	TestEqual(TEXT("Its own prompt as written"), PromptOf(), FString(TEXT("Pull the clamp pin")));
	return true;
}

#endif
