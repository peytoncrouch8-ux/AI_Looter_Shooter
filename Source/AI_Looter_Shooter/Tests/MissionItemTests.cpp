#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "World/MissionItem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Mission items (wave 2a): a pickup that belongs to a mission (AMissionItem), there while its mission is under way, carried
// by the campaign once taken and counted by the collect objective, listed as carried until the mission finishes; a creature
// carrying one drops it on its first death, and one dropped and lost to a level change is given rather than strand the
// mission.

using namespace MissionTestWorld;

namespace
{
	const FName CrankId(TEXT("TestCrank"));
	const FName PinId(TEXT("TestPin"));
	const FName AbeTag(TEXT("Carrier_TestAbe"));
	const FName WrenTag(TEXT("Speaker_Wren"));

	/** Step 1: have the crank and the pin (the pin carried by a dummy tagged Carrier_TestAbe); step 2: an event. */
	UMissionDefinition* MakeLift(UObject* Outer, EMissionStart Start)
	{
		UMissionDefinition* Mission = NewMission(Outer, TEXT("TestLift"), EMissionKind::Side, Start, NAME_None);
		FMissionItemInfo& Crank = Mission->Items.AddDefaulted_GetRef();
		Crank.Id = CrankId;
		Crank.Name = FText::FromString(TEXT("the winch crank"));
		Crank.Line = FText::FromString(TEXT("Taken off the lift when they shut her."));
		FMissionItemInfo& Pin = Mission->Items.AddDefaulted_GetRef();
		Pin.Id = PinId;
		Pin.Name = FText::FromString(TEXT("the clamp pin"));
		Pin.DroppedBy.ActorTag = AbeTag;
		UMissionCollectItemObjective* Collect = AddObjective<UMissionCollectItemObjective>(Mission, 0);
		Collect->Items = { CrankId, PinId };
		AddObjective<UMissionEventObjective>(Mission, 1)->Event = TEXT("Test.Ride");
		Mission->TurnIn.SpeakerTag = WrenTag;
		if (Start == EMissionStart::Offered)
		{
			Mission->Offer.GiverTag = WrenTag;
		}
		return Mission;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionItemCarryTest, "Looter.Missions.Items.Carry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionItemCarryTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(300.0, 0.0, 0.0), WrenTag);
	AMissionItem* Crank = World->SpawnActor<AMissionItem>(FVector(800.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!Runner || !Player || !Wren || !Crank)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Crank->ItemId = CrankId;
	const FName Id(TEXT("TestLift"));
	UMissionDefinition* Lift = MakeLift(CreatePackage(nullptr), EMissionStart::Offered);
	Runner->BeginForTesting({ Lift }, Campaign, Player);
	Crank->DispatchBeginPlay();
	Runner->Update(0.f);
	TestTrue(TEXT("The item knows its mission"), Crank->GetMission() == Lift && Crank->GetItemName().ToString() == TEXT("the winch crank"));
	TestTrue(TEXT("It carries its tags"), Crank->ActorHasTag(AMissionItem::ItemTag) && Crank->ActorHasTag(AMissionItem::TagFor(CrankId)));
	TestFalse(TEXT("Its mission not taken yet: it isn't there"), Crank->IsShown());
	TestFalse(TEXT("...and can't be taken"), Crank->Take(Player));

	TestTrue(TEXT("The mission taken from its giver"), Runner->AcceptMission(Id));
	TestTrue(TEXT("Now it's there"), Crank->IsShown());
	const UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	const FMission* Tracked = Display ? Display->GetTracked() : nullptr;
	TestTrue(TEXT("The arrow points at it while it lies there"), Tracked && Tracked->Waypoint.IsSet()
		&& Tracked->Waypoint->Equals(Crank->GetActorLocation(), 1.0));
	TestTrue(TEXT("Taken"), Crank->Take(Player));
	TestTrue(TEXT("The campaign carries it"), Campaign.IsCarrying(CrankId) && Runner->IsCarrying(CrankId));
	TestFalse(TEXT("It's gone from the level"), Crank->IsShown());
	TestEqual(TEXT("The collect objective counts it"), Runner->GetObjectiveViews(Id).Num() == 1 ? Runner->GetObjectiveViews(Id)[0].Progress
		: FString(), FString(TEXT("1 / 2")));
	const TArray<FMissionItemInfo> Carried = Runner->GetCarriedItems(Id);
	TestTrue(TEXT("The Missions page lists it as carried, with its line"), Carried.Num() == 1 && Carried[0].Id == CrankId
		&& !Carried[0].Line.IsEmpty());
	TestFalse(TEXT("Given twice: once"), Runner->GiveItem(CrankId));

	// The console's way in: the pin straight into the pocket (a Collect event with its tag), and the step moves on.
	TestTrue(TEXT("The pin given"), Runner->GiveItem(PinId));
	TestEqual(TEXT("Both carried: the next step"), Runner->GetStep(Id), 1);

	// Finished, its things go with it.
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Ride")));
	Runner->NotifyEvent(FMissionEvent::Talked(Wren));
	TestTrue(TEXT("Turned in"), Campaign.HasCompleted(Id));
	TestTrue(TEXT("Its items are handed over"), !Campaign.IsCarrying(CrankId) && !Campaign.IsCarrying(PinId) && Campaign.CarriedItems.IsEmpty());
	TestFalse(TEXT("A finished mission's item can't be carried again"), Runner->GiveItem(CrankId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionItemDropTest, "Looter.Missions.Items.Drop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionItemDropTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	APlayerController* Shooter = World->SpawnActor<APlayerController>();
	if (!Runner || !Player || !Shooter)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	const FName Id(TEXT("TestLift"));
	UMissionDefinition* Lift = MakeLift(CreatePackage(nullptr), EMissionStart::Automatic);
	Runner->BeginForTesting({ Lift }, Campaign, Player);
	Runner->Update(0.f);
	TestTrue(TEXT("It runs"), Runner->IsRunning(Id));

	// Abe carries the pin: killed, it falls at his body, once.
	ATargetDummy* Abe = SpawnDummy(World, FVector(1200.0, 0.0, 0.0), AbeTag);
	ATargetDummy* Other = SpawnDummy(World, FVector(1400.0, 0.0, 0.0));
	if (!TestTrue(TEXT("Abe and another placed"), Abe && Other))
	{
		return false;
	}
	Hurt(Other, 1000.f, Shooter);
	TestNull(TEXT("Somebody else's death drops nothing"), AMissionItem::FindIn(World, PinId));
	Hurt(Abe, 1000.f, Shooter);
	AMissionItem* Pin = AMissionItem::FindIn(World, PinId);
	TestNotNull(TEXT("Abe's death drops the pin"), Pin);
	TestTrue(TEXT("...recorded as dropped"), Campaign.DroppedItems.Contains(PinId));
	if (Pin)
	{
		TestTrue(TEXT("...a drop, taken by running over it"), Pin->IsDropped() && Pin->bPickUpOnTouch);
		TestTrue(TEXT("...near where he fell"), FVector::Dist2D(Pin->GetActorLocation(), Abe->GetActorLocation()) < 100.0);
	}
	ATargetDummy* AbeAgain = SpawnDummy(World, FVector(1250.0, 0.0, 0.0), AbeTag);
	if (AbeAgain)
	{
		Hurt(AbeAgain, 1000.f, Shooter);
	}
	int32 Pins = 0;
	for (TActorIterator<AMissionItem> It(World); It; ++It)
	{
		Pins += It->ItemId == PinId && !It->IsActorBeingDestroyed() ? 1 : 0;
	}
	TestEqual(TEXT("Called back and killed again: no second pin"), Pins, 1);

	// Lost before it was picked up (the level left): the next look gives it, so the mission never waits on it.
	if (Pin)
	{
		Pin->Destroy();
	}
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("A dropped pin lying nowhere is given"), Campaign.IsCarrying(PinId));
	TestEqual(TEXT("...and counted"), Runner->GetObjectiveViews(Id).Num() == 1 ? Runner->GetObjectiveViews(Id)[0].Progress : FString(),
		FString(TEXT("1 / 2")));
	return true;
}

#endif
