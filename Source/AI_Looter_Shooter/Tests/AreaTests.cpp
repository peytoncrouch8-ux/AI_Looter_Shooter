#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/AreaLandings.h"
#include "Session/SessionSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerStart.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* IslandLevelPath = TEXT("/Game/Maps/Lvl_TutorialIsland");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAreaDefinitionTest, "Looter.Areas.Definition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAreaDefinitionTest::RunTest(const FString& Parameters)
{
	// An area's names and level, without the project's area assets: a level that isn't built yet is named, but nobody can
	// go there. The test areas are transient, in a scratch package of their own each run, so they never meet an older
	// run's, and the asset registry never lists them as the project's areas.
	UPackage* Scratch = CreatePackage(nullptr);
	UAreaDefinition* Valley = NewObject<UAreaDefinition>(Scratch, TEXT("DA_Area_TestValley"), RF_Transient);
	Valley->DisplayName = FText::FromString(TEXT("Widow's Test Valley"));
	Valley->Map = FSoftObjectPath(TEXT("/Game/Maps/Lvl_NotBuiltYet.Lvl_NotBuiltYet"));
	Valley->Landings = { FName(TEXT("Landing_Platform")), FName(TEXT("Landing_Back")) };
	TestTrue(TEXT("Its id is the asset's name without the prefix"), Valley->GetAreaId() == FName(TEXT("TestValley")));
	TestEqual(TEXT("Its level"), Valley->GetMapPackage(), FString(TEXT("/Game/Maps/Lvl_NotBuiltYet")));
	TestFalse(TEXT("A level that isn't built can't be travelled to"), Valley->HasMap());
	TestTrue(TEXT("Trips arrive at the first landing"), Valley->GetDefaultLanding() == FName(TEXT("Landing_Platform")));
	TestTrue(TEXT("Named by its id, any case"), Valley->IsNamed(TEXT("TestValley")) && Valley->IsNamed(TEXT("testvalley")));
	TestTrue(TEXT("Named by its asset's name"), Valley->IsNamed(TEXT("DA_Area_TestValley")));
	TestTrue(TEXT("Named by its name, with punctuation or without"), Valley->IsNamed(TEXT("Widow's Test Valley")) && Valley->IsNamed(TEXT("widows test valley"))
		&& Valley->IsNamed(TEXT("WidowsTestValley")));
	TestFalse(TEXT("Not by part of a name, nothing, or its level"), Valley->IsNamed(TEXT("Valley")) || Valley->IsNamed(TEXT("")) || Valley->IsNamed(TEXT("Lvl_NotBuiltYet")));
	TestEqual(TEXT("Words simplified"), UAreaDefinition::Simplify(TEXT("Ransom's Rest")), FString(TEXT("ransomsrest")));

	UAreaDefinition* Island = NewObject<UAreaDefinition>(Scratch, TEXT("DA_Area_TestIsland"), RF_Transient);
	Island->Map = FSoftObjectPath(TEXT("/Game/Maps/Lvl_TutorialIsland.Lvl_TutorialIsland"));
	TestTrue(TEXT("A level in the game can be travelled to"), Island->HasMap());
	TestTrue(TEXT("No landings: trips arrive at the level's start"), Island->GetDefaultLanding().IsNone());

	UAreaDefinition* Blank = NewObject<UAreaDefinition>(Scratch, TEXT("DA_Area_TestBlank"), RF_Transient);
	TestTrue(TEXT("No level set: nobody goes"), Blank->GetMapPackage().IsEmpty() && !Blank->HasMap());

	// Without an area a place is named after its level's file, and a session continues where it was saved while that
	// level is in the game.
	TestEqual(TEXT("A place with no area"), USessionSubsystem::AreaName(TEXT("/Game/Maps/Lvl_SomewhereElse")), FString(TEXT("Somewhere Else")));
	TestEqual(TEXT("Continues where it was saved"), USessionSubsystem::ContinueMap(TEXT("/Game/Maps/Lvl_Skyreach")), FString(TEXT("/Game/Maps/Lvl_Skyreach")));
	TestEqual(TEXT("A level not in the game: the first level"), USessionSubsystem::ContinueMap(TEXT("/Game/Maps/Lvl_NotBuiltYet")), FString(IslandLevelPath));
	TestEqual(TEXT("No level saved: the first level"), USessionSubsystem::ContinueMap(FString()), FString(IslandLevelPath));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAreaAssetsTest, "Looter.Areas.Assets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAreaAssetsTest::RunTest(const FString& Parameters)
{
	// The project's areas (Tools/Unreal/create_area_assets.py makes them): Skyreach on the tutorial island, for practice,
	// and Ransom's Rest, whose level may not be built yet.
	const UAreaDefinition* Skyreach = UAreaDefinition::FindByName(TEXT("Skyreach"));
	const UAreaDefinition* RansomsRest = UAreaDefinition::FindByName(TEXT("RansomsRest"));
	if (!Skyreach || !RansomsRest)
	{
		AddError(TEXT("DA_Area_Skyreach and DA_Area_RansomsRest belong in /Game/Data/Areas: run Tools/Unreal/create_area_assets.py in the editor."));
		return false;
	}
	TestEqual(TEXT("Skyreach's name"), Skyreach->DisplayName.ToString(), FString(TEXT("Skyreach")));
	TestEqual(TEXT("Skyreach is the tutorial island"), Skyreach->GetMapPackage(), FString(IslandLevelPath));
	TestTrue(TEXT("Skyreach is for practice"), Skyreach->bPractice);
	TestTrue(TEXT("Skyreach's level is in the game"), Skyreach->HasMap());
	TestTrue(TEXT("Trips to Skyreach arrive at its jetty"), Skyreach->GetDefaultLanding() == FName(TEXT("Landing_Jetty")));
	TestEqual(TEXT("Ransom's Rest's name"), RansomsRest->DisplayName.ToString(), FString(TEXT("Ransom's Rest")));
	TestEqual(TEXT("Ransom's Rest's level"), RansomsRest->GetMapPackage(), FString(TEXT("/Game/Maps/Lvl_RansomsRest")));
	TestFalse(TEXT("Ransom's Rest is the story, not practice"), RansomsRest->bPractice);
	TestTrue(TEXT("Trips to Ransom's Rest arrive at the depot"), RansomsRest->GetDefaultLanding() == FName(TEXT("Landing_Depot")));
	TestFalse(TEXT("Ransom's Rest opens with a mission"), RansomsRest->OpeningMission.IsNone());
	AddInfo(RansomsRest->HasMap() ? TEXT("Ransom's Rest's level is in the game.") : TEXT("Ransom's Rest's level isn't built yet: a trip there says so and nobody goes."));
	TestTrue(TEXT("The tutorial island is Skyreach's level"), UAreaDefinition::FindByMap(IslandLevelPath) == Skyreach);
	TestEqual(TEXT("The session picker calls the tutorial island Skyreach"), USessionSubsystem::AreaName(IslandLevelPath), FString(TEXT("Skyreach")));

	// Every area: named, with a level and its landings named as landings, none sharing an id or a level.
	TSet<FName> Ids;
	TSet<FString> Levels;
	for (const UAreaDefinition* Area : UAreaDefinition::LoadAll())
	{
		const FString Label = Area->GetName();
		TestFalse(*FString::Printf(TEXT("%s has a name"), *Label), Area->DisplayName.IsEmpty());
		TestFalse(*FString::Printf(TEXT("%s has a level"), *Label), Area->GetMapPackage().IsEmpty());
		TestTrue(*FString::Printf(TEXT("%s has a landing"), *Label), Area->Landings.Num() > 0);
		for (const FName Landing : Area->Landings)
		{
			TestTrue(*FString::Printf(TEXT("%s's landing %s starts with Landing_"), *Label, *Landing.ToString()), AreaLandings::IsLandingTag(Landing));
		}
		bool bIdTaken = false;
		Ids.Add(Area->GetAreaId(), &bIdTaken);
		TestFalse(*FString::Printf(TEXT("%s's id is its own"), *Label), bIdTaken);
		bool bLevelTaken = false;
		Levels.Add(Area->GetMapPackage(), &bLevelTaken);
		TestFalse(*FString::Printf(TEXT("%s's level is its own"), *Label), bLevelTaken);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAreaLandingsTest, "Looter.Areas.Landings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAreaLandingsTest::RunTest(const FString& Parameters)
{
	// Where a trip arrives: the player start or actor tagged with its landing. A level's own start is never a landing.
	TestTrue(TEXT("A landing's name"), AreaLandings::IsLandingTag(TEXT("Landing_Jetty")));
	TestFalse(TEXT("Other tags aren't landings"), AreaLandings::IsLandingTag(TEXT("Ground")) || AreaLandings::IsLandingTag(NAME_None));

	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	APlayerStart* LevelStart = World->SpawnActor<APlayerStart>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator);
	APlayerStart* JettyStart = World->SpawnActor<APlayerStart>(FVector(800.0, 0.0, 100.0), FRotator(0.f, 90.f, 0.f));
	AActor* DepotMarker = World->SpawnActor<AActor>();
	if (!TestTrue(TEXT("Starts and a marker placed"), LevelStart && JettyStart && DepotMarker))
	{
		return false;
	}
	JettyStart->PlayerStartTag = TEXT("Landing_Jetty");
	DepotMarker->Tags.Add(TEXT("Landing_Depot"));

	TestTrue(TEXT("A player start by its tag"), AreaLandings::Find(World, TEXT("Landing_Jetty")) == JettyStart);
	TestTrue(TEXT("Any actor by its tag"), AreaLandings::Find(World, TEXT("Landing_Depot")) == DepotMarker);
	TestNull(TEXT("A landing the level doesn't have"), AreaLandings::Find(World, TEXT("Landing_Nowhere")));
	TestNull(TEXT("No landing named"), AreaLandings::Find(World, NAME_None));
	TestFalse(TEXT("The level's own start isn't a landing"), AreaLandings::IsLanding(LevelStart));
	TestTrue(TEXT("The jetty's start is"), AreaLandings::IsLanding(JettyStart));
	TestTrue(TEXT("So is the depot's marker"), AreaLandings::IsLanding(DepotMarker));
	TestFalse(TEXT("Nothing isn't"), AreaLandings::IsLanding(nullptr));
	return true;
}

#endif
