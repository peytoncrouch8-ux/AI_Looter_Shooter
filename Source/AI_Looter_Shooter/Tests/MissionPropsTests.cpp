#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/StoryLine.h"
#include "Tests/MissionTestWorld.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "World/PorchPlate.h"
#include "World/ReadableProp.h"
#include "World/SafeGround.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The small things of the mission features (wave 2a): a safe zone's height band (Main Street over Tilly's cellar), the
// minimap's underground spaces, Delia's porch plate (the jars once, the biscuits later, kept by story flags) and the
// readable props that tell the missions they were read.

using namespace MissionTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSafeGroundHeightBandTest, "Looter.World.SafeGround.HeightBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSafeGroundHeightBandTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ASafeGround* Street = World->SpawnActor<ASafeGround>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A safe zone placed"), Street))
	{
		return false;
	}
	Street->Radius = 1000.f;
	const FVector OnTheStreet(200.0, 0.0, 50.0);
	const FVector InTheCellar(200.0, 0.0, -400.0);
	TestTrue(TEXT("Without a band, every height over it is inside"), Street->Contains(OnTheStreet) && Street->Contains(InTheCellar));
	Street->bHeightBand = true;
	Street->MinZ = -150.f;
	Street->MaxZ = 1500.f;
	TestTrue(TEXT("With a band: the street is safe"), Street->Contains(OnTheStreet));
	TestFalse(TEXT("...the cellar under it isn't"), Street->Contains(InTheCellar));
	TestFalse(TEXT("...nor high over it"), Street->Contains(FVector(200.0, 0.0, 2000.0)));
	TestFalse(TEXT("...nor beside it at the street's height"), Street->Contains(FVector(3000.0, 0.0, 50.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapUndergroundTest, "Looter.UI.Minimap.Underground",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMinimapUndergroundTest::RunTest(const FString& Parameters)
{
	// The rule: inside any marked space's box.
	TArray<FBox> Zones = { FBox(FVector(-500.0, -500.0, -3000.0), FVector(500.0, 500.0, -1000.0)) };
	TestTrue(TEXT("In the mine"), UHudMinimapWidget::IsUnderground(Zones, FVector(0.0, 0.0, -2000.0)));
	TestFalse(TEXT("On the ridge over it"), UHudMinimapWidget::IsUnderground(Zones, FVector(0.0, 0.0, 200.0)));
	TestFalse(TEXT("Nowhere marked"), UHudMinimapWidget::IsUnderground(TArray<FBox>(), FVector::ZeroVector));

	// The world: an actor tagged Underground (a trigger box, a volume) covers its shape's bounds.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AActor* Cellar = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("A space placed"), Cellar))
	{
		return false;
	}
	UBoxComponent* Box = NewObject<UBoxComponent>(Cellar, TEXT("Box"));
	Box->SetBoxExtent(FVector(400.0, 400.0, 150.0));
	Cellar->SetRootComponent(Box);
	Box->RegisterComponent();
	Cellar->SetActorLocation(FVector(1000.0, 0.0, -300.0));
	Cellar->Tags.Add(UHudMinimapWidget::UndergroundTag);
	AActor* Unmarked = SpawnMarker(World, FVector(5000.0, 0.0, 0.0), TEXT("Something"));
	TArray<FBox> Found;
	UHudMinimapWidget::GatherUndergroundZones(World, Found);
	TestEqual(TEXT("One space marked"), Found.Num(), 1);
	TestTrue(TEXT("In the cellar"), UHudMinimapWidget::IsUnderground(Found, FVector(1100.0, 50.0, -300.0)));
	TestFalse(TEXT("On the street over it"), UHudMinimapWidget::IsUnderground(Found, FVector(1100.0, 50.0, 100.0)));
	TestNotNull(TEXT("(an unmarked actor is ignored)"), Unmarked);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPorchPlateTest, "Looter.World.PorchPlate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPorchPlateTest::RunTest(const FString& Parameters)
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
	APorchPlate* Plate = World->SpawnActor<APorchPlate>(FVector(150.0, 0.0, 80.0), FRotator::ZeroRotator);
	APorchPlate* Later = World->SpawnActor<APorchPlate>(FVector(150.0, 300.0, 80.0), FRotator::ZeroRotator);
	if (!Runner || !Player || !Plate || !Later)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	// Seven Days' step: take what's on the plate; the next asks something else, with eating as its optional objective.
	UMissionDefinition* Supper = NewMission(CreatePackage(nullptr), TEXT("TestSupper"), EMissionKind::Main, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionEventObjective>(Supper, 0)->Event = TEXT("PorchPlate.Taken");
	AddObjective<UMissionEventObjective>(Supper, 1)->Event = TEXT("Test.After");
	UMissionEventObjective* Eat = AddObjective<UMissionEventObjective>(Supper, 1);
	Eat->Event = TEXT("PorchPlate.Eaten");
	Eat->bOptional = true;
	Plate->TakeLines = { FStoryLine::Make(FText::FromString(TEXT("Delia's slip")), FText::FromString(TEXT("SALT FOR THE ROAD.")), 3.f) };
	Later->TakenFlag = TEXT("Test.Later.Taken");
	Later->TakeWhen.AfterMissions = { FName(TEXT("TestNeverDone")) };
	Runner->BeginForTesting({ Supper }, Campaign, Player);
	Plate->DispatchBeginPlay();
	Later->DispatchBeginPlay();
	Runner->Update(0.f);

	TestTrue(TEXT("The jars can be taken"), Plate->CanTake() && !Plate->CanEat());
	TestFalse(TEXT("A plate whose story hasn't come can't"), Later->CanTake());
	TestTrue(TEXT("Taken"), Plate->Take(Player));
	TestTrue(TEXT("...kept by the story's flag"), Campaign.HasFlag(TEXT("PorchPlate.Taken")) && Plate->IsTaken());
	TestFalse(TEXT("...the jars gone from the plate"), Plate->Jars->IsVisible());
	TestEqual(TEXT("...and the missions heard it"), Runner->GetStep(TEXT("TestSupper")), 1);
	TestFalse(TEXT("Never twice"), Plate->Take(Player));

	TestTrue(TEXT("The biscuits can be eaten once the jars are taken"), Plate->CanEat());
	TestTrue(TEXT("Eaten"), Plate->Eat(Player));
	TestTrue(TEXT("...kept by its flag"), Campaign.HasFlag(TEXT("PorchPlate.Eaten")) && !Plate->Biscuits->IsVisible());
	TestTrue(TEXT("...the optional objective done"), Campaign.HasDoneOptional(TEXT("TestSupper"), 1, 1));
	TestTrue(TEXT("Both done: used up for good"), Plate->IsUsedUp() && !Plate->CanEat());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReadablePropTest, "Looter.World.Readable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FReadablePropTest::RunTest(const FString& Parameters)
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
	AReadableProp* Roll = World->SpawnActor<AReadableProp>(FVector(200.0, 0.0, 120.0), FRotator::ZeroRotator);
	AReadableProp* Hat = World->SpawnActor<AReadableProp>(FVector(200.0, 400.0, 0.0), FRotator::ZeroRotator);
	if (!Runner || !Player || !Roll || !Hat)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Roll->ReadableId = TEXT("TestRoll");
	Roll->Title = FText::FromString(TEXT("The keepers' roll"));
	Roll->Text = FText::FromString(TEXT("ADA. ... ABEL. ELLIS."));
	Roll->Lines = { FStoryLine::Make(FText::FromString(TEXT("Hob")), FText::FromString(TEXT("Heir to the family business."))) };
	Hat->ReadableId = TEXT("TestHat");
	Hat->ShownWhen.DuringMission = TEXT("TestNotPlayed");
	UMissionDefinition* Chapel = NewMission(CreatePackage(nullptr), TEXT("TestChapel"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionEventObjective>(Chapel, 0)->Event = TEXT("Test.Bell");
	UMissionEventObjective* Read = AddObjective<UMissionEventObjective>(Chapel, 0);
	Read->Event = AReadableProp::EventFor(TEXT("TestRoll"));
	Read->bOptional = true;
	Runner->BeginForTesting({ Chapel }, Campaign, Player);
	Roll->DispatchBeginPlay();
	Hat->DispatchBeginPlay();
	Runner->Update(0.f);

	TestEqual(TEXT("Its event's name"), Roll->GetReadEvent(), FName(TEXT("Readable.TestRoll")));
	TestTrue(TEXT("It carries its tag, for an arrow"), Roll->ActorHasTag(AReadableProp::TagFor(TEXT("TestRoll"))));
	TestEqual(TEXT("Its lines name their speaker"), Roll->GetLinesToSay().Num() == 1 ? Roll->GetLinesToSay()[0].Speaker.ToString() : FString(),
		FString(TEXT("Hob")));
	TestTrue(TEXT("Read (no HUD here: no card, the rest as ever)"), Roll->Read(Player));
	TestTrue(TEXT("The missions heard it: its optional objective done"), Campaign.HasDoneOptional(TEXT("TestChapel"), 0, 1));
	TestFalse(TEXT("A readable whose story hasn't come isn't there"), Hat->IsShown());
	TestFalse(TEXT("...and can't be read"), Hat->Read(Player));
	return true;
}

#endif
