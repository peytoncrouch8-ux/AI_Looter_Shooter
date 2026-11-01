#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/StationBoard.h"
#include "Creatures/EncounterSpawner.h"
#include "Session/CampaignRecord.h"
#include "Story/AbelOnBoard.h"
#include "Story/DoorHandoff.h"
#include "Story/HobBird.h"
#include "World/CarriedLanternRules.h"
#include "World/ChapelReliquary.h"
#include "World/LanternFlame.h"
#include "World/ReliquaryLantern.h"
#include "World/SoulCoal.h"
#include "World/SoulCoalField.h"
#include "World/Train.h"
#include "World/TrainStation.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

// Main 7, "Given to the Wind" (the Wardens story, lane MAIN67), as the scripts make it: the Gilded Lily on the depot's board
// once it's turned in (create_area_assets.py), and its pieces as Ransom's Rest is built (build_area_main7.py,
// build_area_depot.py): the train warm after Main 7, no hand-off at Delia's door (Heirloom is the turn-in's, by hand), no
// leaning flame on the deck (the lantern is carried home), the twelve soul coal lumps, the coal's fields, the Reliquary's
// lantern rest, Abel's soul by her, Delia at the chapel door, the west road's Company men, Hob's Main 7 perches. Was
// LanternLeansPlacedTests.cpp (its Board test kept as it was).

namespace
{
	const FName MainSeven(TEXT("Main7"));
	const FName LilyId(TEXT("GildedLily"));
	const FName DepotPlace(TEXT("Place_Depot"));

	const ULevel* LoadRansomsRest(FAutomationTestBase& Test)
	{
		if (!FPackageName::DoesPackageExist(TEXT("/Game/Maps/Lvl_RansomsRest")))
		{
			Test.AddInfo(TEXT("Lvl_RansomsRest isn't built: skipped."));
			return nullptr;
		}
		const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
		return Test.TestNotNull(TEXT("Lvl_RansomsRest loads"), Map) ? Map->PersistentLevel.Get() : nullptr;
	}

	FString MeshName(const UStaticMeshComponent* Mesh)
	{
		return Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetName() : FString();
	}

	UAreaDefinition* NewArea(UObject* Outer, const TCHAR* AssetName, const TCHAR* Name, const TCHAR* Map, const TCHAR* Landing)
	{
		UAreaDefinition* Area = NewObject<UAreaDefinition>(Outer, AssetName, RF_Transient);
		Area->DisplayName = FText::FromString(Name);
		Area->Map = FSoftObjectPath(FString::Printf(TEXT("%s.%s"), Map, *FPackageName::GetShortName(Map)));
		Area->Landings = { FName(Landing) };
		return Area;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGivenToTheWindBoardTest, "Looter.Story.GivenToTheWind.Board",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGivenToTheWindBoardTest::RunTest(const FString& Parameters)
{
	// The Lily as the area script makes it: an area of the story with no level yet, "the Lily" in a sentence, so the board
	// lists it once Main 7 is turned in and choosing it says "The line to the Lily isn't open yet."
	UPackage* Scratch = CreatePackage(nullptr);
	UAreaDefinition* Stand = NewArea(Scratch, TEXT("DA_Area_GildedLily"), TEXT("The Gilded Lily"), TEXT("/Game/Maps/Lvl_GildedLily"), TEXT("Landing_Platform"));
	Stand->SpokenName = FText::FromString(TEXT("the Lily"));
	TestEqual(TEXT("A sentence calls it the Lily"), StationBoard::SpokenName(*Stand).ToString(), FString(TEXT("the Lily")));
	const UAreaDefinition* Lily = FPackageName::DoesPackageExist(TEXT("/Game/Data/Areas/DA_Area_GildedLily"))
		? LoadObject<UAreaDefinition>(nullptr, TEXT("/Game/Data/Areas/DA_Area_GildedLily.DA_Area_GildedLily")) : nullptr;
	if (!Lily)
	{
		AddWarning(TEXT("DA_Area_GildedLily isn't made yet: run Tools/Unreal/create_area_assets.py."));
		return true;
	}
	FCampaignRecord Opened;
	StationBoard::RecordFirstCastOff(Opened);
	Opened.Complete(MainSeven);
	Opened.OpenArea(LilyId);
	const TArray<FStationBoardLine> Lines = StationBoard::BuildLines(Opened, UAreaDefinition::LoadAll(), {}, StationBoard::FirstAreaId());
	const FStationBoardLine* LilyLine = Lines.FindByPredicate([](const FStationBoardLine& Line) { return Line.AreaId == LilyId; });
	if (TestNotNull(TEXT("Once Main 7 is done the depot's board lists it"), LilyLine))
	{
		TestTrue(TEXT("...a destination nobody can go to yet"), LilyLine->IsDestination() && !LilyLine->bLevelBuilt);
		TestEqual(TEXT("...and choosing it says so"), StationBoard::NotOpenText(FStationBoardWords::Station(), *LilyLine).ToString(),
			FString(TEXT("The line to the Lily isn't open yet.")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGivenToTheWindPlacedTest, "Looter.Story.GivenToTheWind.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGivenToTheWindPlacedTest::RunTest(const FString& Parameters)
{
	const ULevel* Level = LoadRansomsRest(*this);
	if (!Level)
	{
		return true;
	}
	const ATrain* Train = nullptr;
	const ADoorHandoff* Handoff = nullptr;
	const ALanternFlame* Flame = nullptr;
	const AHobBird* Hob = nullptr;
	const AActor* Depot = nullptr;
	const AReliquaryLantern* Rest = nullptr;
	const AChapelReliquary* Saint = nullptr;
	const AAbelOnBoard* AbelSoul = nullptr;
	const AActor* DeliaChapel = nullptr;
	const AEncounterSpawner* Road = nullptr;
	TArray<const ASoulCoalLump*> Lumps;
	int32 FieldLumps = 0;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		Train = Train ? Train : Cast<ATrain>(Actor);
		Handoff = Handoff ? Handoff : Cast<ADoorHandoff>(Actor);
		Flame = Flame ? Flame : Cast<ALanternFlame>(Actor);
		Hob = Hob ? Hob : Cast<AHobBird>(Actor);
		Rest = Rest ? Rest : Cast<AReliquaryLantern>(Actor);
		Saint = Saint ? Saint : Cast<AChapelReliquary>(Actor);
		Depot = Depot || !Actor->ActorHasTag(DepotPlace) ? Depot : Actor;
		DeliaChapel = DeliaChapel || !Actor->ActorHasTag(GivenToTheWind::DeliaChapelTag()) ? DeliaChapel : Actor;
		if (const AAbelOnBoard* Abel = Cast<AAbelOnBoard>(Actor); Abel && Abel->bSoulLook)
		{
			AbelSoul = Abel;
		}
		if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor); Spawner && Spawner->SpawnerId == GivenToTheWind::WestRoadCrewId())
		{
			Road = Spawner;
		}
		if (const ASoulCoalLump* Lump = Cast<ASoulCoalLump>(Actor))
		{
			Lumps.Add(Lump);
		}
		if (const ASoulCoalField* Field = Cast<ASoulCoalField>(Actor))
		{
			FieldLumps += Field->NumLumps();
		}
	}
	if (!Train && !Rest && Lumps.IsEmpty())
	{
		AddWarning(TEXT("Main 7's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	if (TestNotNull(TEXT("The train at the platform"), Train))
	{
		TestTrue(TEXT("...cold and shut until Main 7 is turned in"), !Train->bWarm && Train->WarmWhen.AfterMissions == TArray<FName>({ MainSeven }));
		TestTrue(TEXT("...the depot's place at its door"), Depot && FVector::Dist2D(Depot->GetActorLocation(), Train->GetArrivalSpot().GetLocation()) < 200.0);
	}
	TestNull(TEXT("No hand-off at Delia's door (Heirloom comes by hand at the turn-in)"), Handoff);
	TestNull(TEXT("No leaning flame on the deck (Abel's lantern is carried home)"), Flame);
	TestEqual(TEXT("Twelve soul coal lumps to free"), Lumps.Num(), 12);
	TSet<FName> Ids;
	for (const ASoulCoalLump* Lump : Lumps)
	{
		Ids.Add(Lump->LumpId);
	}
	TestTrue(TEXT("...each its own id, none empty"), Ids.Num() == Lumps.Num() && !Ids.Contains(NAME_None));
	TestTrue(TEXT("The barge's coal strewn over the Rest"), FieldLumps >= 40);
	if (TestNotNull(TEXT("The Reliquary's lantern rest"), Rest) && Saint)
	{
		TestTrue(TEXT("...on her lid"), FVector::Dist(Rest->GetActorLocation(), Saint->GetActorLocation()) < 300.0);
		TestTrue(TEXT("...her light the waking's for good"), Saint->LitWhen.WithFlags.Contains(GivenToTheWind::SaintAdaAwakeFlag()));
	}
	if (TestNotNull(TEXT("Abel's soul by the Reliquary"), AbelSoul) && Saint)
	{
		TestTrue(TEXT("...standing, near her, there once she wakes"), AbelSoul->bStanding && FVector::Dist(AbelSoul->GetActorLocation(), Saint->GetActorLocation()) < 600.0
			&& AbelSoul->ShownWhen.WithFlags.Contains(GivenToTheWind::SaintAdaAwakeFlag()) && AbelSoul->ActorHasTag(AAbelOnBoard::SpeakerTag));
	}
	TestNotNull(TEXT("Delia at the chapel door for the turn-in"), DeliaChapel);
	if (TestNotNull(TEXT("The west road's Company men"), Road))
	{
		TestTrue(TEXT("...during Main 7's walk home"), Road->ActiveWhen.DuringMission == MainSeven && Road->ActiveWhen.FromStep == GivenToTheWind::CarryStep);
	}
	if (TestNotNull(TEXT("Hob"), Hob))
	{
		TestTrue(TEXT("Hob has perches in Main 7"), Hob->Perches.ContainsByPredicate([](const FHobPerch& Perch) { return Perch.When.DuringMission == MainSeven; }));
	}
	return true;
}

#endif
