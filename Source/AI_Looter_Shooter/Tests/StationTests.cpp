#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/AreaLandings.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Areas/StationBoard.h"
#include "Creatures/SpiderCreature.h"
#include "Loot/LootLibrary.h"
#include "Loot/LootTable.h"
#include "Missions/MissionDefinition.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/MissionTestWorld.h"
#include "Weapons/WeaponDefinition.h"
#include "World/SkiffJetty.h"
#include "World/TrainStation.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* IslandMap = TEXT("/Game/Maps/Lvl_TutorialIsland");
	const TCHAR* UnbuiltMap = TEXT("/Game/Maps/Lvl_NotBuiltYet");

	/** An area made in code, in a scratch package so it never meets the project's assets. Its id is its asset name's end. */
	UAreaDefinition* NewArea(UPackage* Scratch, const TCHAR* AssetName, const TCHAR* Name, const TCHAR* Map, const TCHAR* Landing, bool bPractice)
	{
		UAreaDefinition* Area = NewObject<UAreaDefinition>(Scratch, AssetName, RF_Transient);
		Area->DisplayName = FText::FromString(Name);
		Area->Map = FSoftObjectPath(FString::Printf(TEXT("%s.%s"), Map, *FPackageName::GetShortName(Map)));
		Area->Landings = { FName(Landing) };
		Area->bPractice = bPractice;
		return Area;
	}

	/** A main mission that opens an area, made in code. */
	UMissionDefinition* NewOpener(UPackage* Scratch, const TCHAR* Id, const TCHAR* Title, int32 SortOrder, FName Opens)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Scratch, Id, EMissionKind::Main, EMissionStart::Automatic, NAME_None);
		Mission->Title = FText::FromString(Title);
		Mission->SortOrder = SortOrder;
		Mission->Rewards.UnlockAreas = { Opens };
		return Mission;
	}

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationBoardLinesTest, "Looter.Station.BoardLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStationBoardLinesTest::RunTest(const FString& Parameters)
{
	// A station board's lines as the story moves on (Docs/Areas/RansomsRest.md, "The exit and the unlock"), from areas
	// and missions made in code: Skyreach for practice, Ransom's Rest (the story's first area), the Lily (not built yet)
	// opened by Main 7, Dustwater by Main 14.
	UPackage* Scratch = CreatePackage(nullptr);
	UAreaDefinition* Skyreach = NewArea(Scratch, TEXT("DA_Area_Skyreach"), TEXT("Skyreach"), IslandMap, TEXT("Landing_Jetty"), true);
	UAreaDefinition* Rest = NewArea(Scratch, TEXT("DA_Area_RansomsRest"), TEXT("Ransom's Rest"), IslandMap, TEXT("Landing_Depot"), false);
	Rest->MinLevel = 1;
	Rest->MaxLevel = 10;
	UAreaDefinition* Lily = NewArea(Scratch, TEXT("DA_Area_TestLily"), TEXT("The Gilded Lily"), UnbuiltMap, TEXT("Landing_Platform"), false);
	const TArray<UAreaDefinition*> Areas = { Skyreach, Rest, Lily };
	const TArray<UMissionDefinition*> Missions = {
		NewOpener(Scratch, TEXT("TestMain7"), TEXT("The Lantern Leans"), 7, TEXT("TestLily")),
		NewOpener(Scratch, TEXT("TestMain14"), TEXT("Dust to Dust"), 14, TEXT("TestDustwater")),
	};
	TestTrue(TEXT("Ransom's Rest is the story's first area"), Rest->GetAreaId() == StationBoard::FirstAreaId());

	// Before the first cast-off, Skyreach's jetty offers one trip: to the story's first arrival, the family plot's grave.
	FCampaignRecord Fresh;
	TArray<FStationBoardLine> Lines = StationBoard::BuildLines(Fresh, Areas, Missions, Skyreach->GetAreaId());
	if (TestEqual(TEXT("Before the first cast-off: one line"), Lines.Num(), 1))
	{
		const FStationBoardLine& Leave = Lines[0];
		TestTrue(TEXT("It's the first cast-off, to Ransom's Rest"), Leave.bFirstCastOff && Leave.AreaId == Rest->GetAreaId() && Leave.IsDestination());
		TestTrue(TEXT("It arrives at the family plot"), Leave.Landing == FName(TEXT("Landing_FamilyPlot")));
		TestTrue(TEXT("Its level is in the game (the stand-in's)"), Leave.bLevelBuilt);
		TestEqual(TEXT("Its name"), Leave.Name.ToString(), FString(TEXT("Ransom's Rest")));
	}

	// The first cast-off opens the story: Ransom's Rest is open, the Lily isn't.
	FCampaignRecord Story;
	StationBoard::RecordFirstCastOff(Story);
	TestTrue(TEXT("Cast off, and Ransom's Rest open"), Story.bFirstCastOff && StationBoard::IsAreaOpen(Story, Rest->GetAreaId())
		&& Story.IsAreaOpen(Rest->GetAreaId()));
	TestFalse(TEXT("The Lily isn't open"), StationBoard::IsAreaOpen(Story, Lily->GetAreaId()));
	FCampaignRecord FlagOnly;
	FlagOnly.bFirstCastOff = true;
	TestTrue(TEXT("A record with only the flag still has the first area open"), StationBoard::IsAreaOpen(FlagOnly, Rest->GetAreaId()));

	// At the depot on Ransom's Rest: the area itself (here), the blank line naming Main 7, and Skyreach for practice.
	Lines = StationBoard::BuildLines(Story, Areas, Missions, Rest->GetAreaId());
	if (TestEqual(TEXT("The depot's board: three lines"), Lines.Num(), 3))
	{
		TestTrue(TEXT("1: Ransom's Rest, here: it goes nowhere"), Lines[0].AreaId == Rest->GetAreaId() && Lines[0].bHere && !Lines[0].IsDestination()
			&& Lines[0].Kind == EStationLine::Area);
		TestTrue(TEXT("2: the blank line, naming the mission that opens the next area"), Lines[1].Kind == EStationLine::NextMission
			&& Lines[1].MissionId == FName(TEXT("TestMain7")) && !Lines[1].IsDestination());
		TestEqual(TEXT("Its words"), Lines[1].Name.ToString(), FString(TEXT("Finish ‘The Lantern Leans’")));
		TestTrue(TEXT("3: Skyreach for practice, to its jetty"), Lines[2].Kind == EStationLine::Practice && Lines[2].AreaId == Skyreach->GetAreaId()
			&& Lines[2].Landing == FName(TEXT("Landing_Jetty")) && Lines[2].IsDestination() && !Lines[2].bFirstCastOff);
		TestEqual(TEXT("Its name"), Lines[2].Name.ToString(), FString(TEXT("Skyreach (practice)")));
	}
	TestFalse(TEXT("Later areas aren't shown"), Lines.ContainsByPredicate([Lily](const FStationBoardLine& Line) { return Line.AreaId == Lily->GetAreaId(); }));

	// Back on Skyreach's jetty: every opened area, each a trip to its station, and no practice line for itself.
	Lines = StationBoard::BuildLines(Story, Areas, Missions, Skyreach->GetAreaId());
	if (TestEqual(TEXT("The jetty's board after the first cast-off: two lines"), Lines.Num(), 2))
	{
		TestTrue(TEXT("Ransom's Rest, to its depot, a plain trip"), Lines[0].AreaId == Rest->GetAreaId() && Lines[0].IsDestination()
			&& Lines[0].Landing == FName(TEXT("Landing_Depot")) && !Lines[0].bFirstCastOff);
		TestTrue(TEXT("Then the blank line"), Lines[1].Kind == EStationLine::NextMission);
	}

	// Main 7 done: the Lily opens (its level not built yet: listed, and a trip there stays), the blank line moves on.
	Story.Complete(TEXT("TestMain7"));
	Story.OpenArea(TEXT("TestLily"));
	Lines = StationBoard::BuildLines(Story, Areas, Missions, Rest->GetAreaId());
	const FStationBoardLine* LilyLine = Lines.FindByPredicate([Lily](const FStationBoardLine& Line) { return Line.AreaId == Lily->GetAreaId(); });
	TestTrue(TEXT("The Lily is listed, a destination whose level isn't in the game"), LilyLine && LilyLine->IsDestination() && !LilyLine->bLevelBuilt);
	const FStationBoardLine* Blank = Lines.FindByPredicate([](const FStationBoardLine& Line) { return Line.Kind == EStationLine::NextMission; });
	TestTrue(TEXT("The blank line names Main 14 now"), Blank && Blank->MissionId == FName(TEXT("TestMain14")));
	TestEqual(TEXT("What the board says of the Lily"), FText::Format(FStationBoardWords::Station().NotOpen, Lily->DisplayName).ToString(),
		FString(TEXT("The line to The Gilded Lily isn't open yet.")));

	// Everything opened: no blank line.
	Story.Complete(TEXT("TestMain14"));
	Story.OpenArea(TEXT("TestDustwater"));
	Lines = StationBoard::BuildLines(Story, Areas, Missions, Rest->GetAreaId());
	TestFalse(TEXT("Nothing left to open: no blank line"), Lines.ContainsByPredicate([](const FStationBoardLine& Line) { return Line.Kind == EStationLine::NextMission; }));

	// The boards' words.
	TestEqual(TEXT("A station travels"), FStationBoardWords::Station().Depart.ToString(), FString(TEXT("Travel")));
	TestEqual(TEXT("The jetty casts off"), FStationBoardWords::Jetty().Depart.ToString(), FString(TEXT("Cast off")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationPracticeRulesTest, "Looter.Station.PracticeRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStationPracticeRulesTest::RunTest(const FString& Parameters)
{
	// Skyreach is practice: its kills give no experience, and once the player has left it they drop only ammo.
	UPackage* Scratch = CreatePackage(nullptr);
	UAreaDefinition* Practice = NewArea(Scratch, TEXT("DA_Area_TestPractice"), TEXT("Practice"), IslandMap, TEXT("Landing_Jetty"), true);
	UAreaDefinition* StoryArea = NewArea(Scratch, TEXT("DA_Area_TestValley"), TEXT("Valley"), IslandMap, TEXT("Landing_Depot"), false);
	FCampaignRecord Before;
	FCampaignRecord After;
	After.bFirstCastOff = true;

	TestFalse(TEXT("Practice: no experience"), Practice->GivesKillExperience() || UAreaRulesSubsystem::GivesKillExperienceIn(Practice));
	TestTrue(TEXT("The story's areas give it"), StoryArea->GivesKillExperience() && UAreaRulesSubsystem::GivesKillExperienceIn(StoryArea));
	TestTrue(TEXT("A level that is no area's gives it"), UAreaRulesSubsystem::GivesKillExperienceIn(nullptr));
	TestTrue(TEXT("Practice before the first cast-off (the tutorial's kills): guns drop"), UAreaRulesSubsystem::DropsGunsIn(Practice, &Before)
		&& UAreaRulesSubsystem::DropsGunsIn(Practice, nullptr));
	TestFalse(TEXT("Practice after it: ammo only"), UAreaRulesSubsystem::DropsGunsIn(Practice, &After));
	TestTrue(TEXT("The story's areas drop guns after it"), UAreaRulesSubsystem::DropsGunsIn(StoryArea, &After) && UAreaRulesSubsystem::DropsGunsIn(nullptr, &After));

	// A level 1 spider's experience (its share of its 10, Docs/Progression.md), none on Skyreach. A class default is in no
	// level: as it always was.
	const ASpiderCreature* Spider = GetDefault<ASpiderCreature>();
	const int64 SpiderXP = UPlayerProgressionSubsystem::GetLevelRules().KillXP(Spider->XPReward, 1, 1);
	TestTrue(TEXT("A level 1 spider is worth something"), SpiderXP > 0);
	TestEqual(TEXT("In the story: its experience"), UPlayerProgressionSubsystem::KillXPIn(StoryArea, Spider, 1), SpiderXP);
	TestEqual(TEXT("In practice: none"), UPlayerProgressionSubsystem::KillXPIn(Practice, Spider, 1), int64(0));
	TestEqual(TEXT("No area: its experience"), UPlayerProgressionSubsystem::KillXPIn(nullptr, Spider, 1), SpiderXP);
	TestEqual(TEXT("Asked of the spider itself, in no level: its experience"), UPlayerProgressionSubsystem::KillXP(Spider, 1), SpiderXP);
	TestTrue(TEXT("Asked of no level at all: yes, and yes"), UAreaRulesSubsystem::GivesKillExperienceAt(Spider) && UAreaRulesSubsystem::DropsGunsAt(Spider));

	// A kill that always drops a gun drops none when guns are off, and the very same ammo from the same stream.
	UWeaponDefinition* Rifle = USessionSubsystem::LoadBullpup();
	if (!TestNotNull(TEXT("The rifle's definition"), Rifle))
	{
		return false;
	}
	ULootTable* Table = NewObject<ULootTable>(Scratch, TEXT("TestAlwaysAGun"), RF_Transient);
	FLootTableEntry& Entry = Table->Entries.AddDefaulted_GetRef();
	Entry.Weapon = Rifle;
	Table->WeaponDropChance = 1.f;
	Table->AmmoDropChance = 1.f;
	for (int32 Seed = 1; Seed <= 20; ++Seed)
	{
		FRandomStream WithGuns(Seed);
		FRandomStream AmmoOnly(Seed);
		const FLootRoll Full = ULootLibrary::RollLoot(Table, 1, 0.f, WithGuns);
		const FLootRoll Practiced = ULootLibrary::RollLoot(Table, 1, 0.f, AmmoOnly, {}, /*bWeapons*/ false);
		bool bSameAmmo = Full.Ammo.Num() == Practiced.Ammo.Num();
		for (int32 Index = 0; bSameAmmo && Index < Full.Ammo.Num(); ++Index)
		{
			bSameAmmo = Full.Ammo[Index].Type == Practiced.Ammo[Index].Type && Full.Ammo[Index].Amount == Practiced.Ammo[Index].Amount;
		}
		if (!TestTrue(FString::Printf(TEXT("Seed %d: a gun with guns on, none without, the same ammo"), Seed),
			Full.Weapons.Num() == 1 && Practiced.Weapons.IsEmpty() && bSameAmmo && Practiced.Ammo.Num() > 0))
		{
			break;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationSkipTutorialTest, "Looter.Station.SkipTutorial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStationSkipTutorialTest::RunTest(const FString& Parameters)
{
	// "Skip the tutorial" in a new game's save: it counts as the first cast-off, the tutorial is done (a practice visit
	// never strands the player), a Common Bullpup is in hand ("in the coffin"), and the session opens on the story's first
	// arrival at level 1. Read back through the save format, as the level reads it.
	UWeaponDefinition* Bullpup = USessionSubsystem::LoadBullpup();
	UPackage* Scratch = CreatePackage(nullptr);
	UAreaDefinition* First = NewArea(Scratch, TEXT("DA_Area_RansomsRest"), TEXT("Ransom's Rest"), TEXT("/Game/Maps/Lvl_RansomsRest"),
		TEXT("Landing_Depot"), false);
	if (!TestNotNull(TEXT("The Bullpup's definition"), Bullpup))
	{
		return false;
	}
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	TestTrue(TEXT("Applied, armed"), USessionSubsystem::ApplyTutorialSkip(*Save, *First, Bullpup));

	const ULooterSessionSave* Read = WriteAndRead(Save);
	if (!TestNotNull(TEXT("Read back"), Read))
	{
		return false;
	}
	TestTrue(TEXT("The tutorial is done"), Read->Progress.bTutorialDone);
	TestTrue(TEXT("It counts as the first cast-off, and the story's first area is open"), Read->Campaign.bFirstCastOff
		&& Read->Campaign.IsAreaOpen(StationBoard::FirstAreaId()));
	TestEqual(TEXT("Level 1, as every new session arrives"), Read->Progress.Level, 1);
	TestTrue(TEXT("It carries what the save says"), Read->bHasInventory);
	if (TestEqual(TEXT("One gun"), Read->Inventory.Equipped.Num(), 1))
	{
		const FWeaponInstanceData& Gun = Read->Inventory.Equipped[0];
		TestTrue(TEXT("A Bullpup"), Gun.Definition == Bullpup);
		TestTrue(TEXT("Common"), Gun.Rarity == EWeaponRarity::Common);
		TestEqual(TEXT("Level 1"), Gun.Level, 1);
		TestTrue(TEXT("Built from its parts"), Gun.Parts.Num() > 0);
		const int32 Rounds = Read->Inventory.Ammo.IsValidIndex(static_cast<int32>(Bullpup->AmmoType)) ? Read->Inventory.Ammo[static_cast<int32>(Bullpup->AmmoType)] : 0;
		TestEqual(TEXT("Its starting magazines"), Rounds, Gun.Stats.MagazineSize * Bullpup->StartingReserveMagazines);
	}
	TestEqual(TEXT("In hand"), Read->Inventory.ActiveSlot, 0);
	TestTrue(TEXT("Nothing in the backpack"), Read->Inventory.Backpack.IsEmpty());
	TestEqual(TEXT("It continues on the first area's level"), Read->Map, FString(TEXT("/Game/Maps/Lvl_RansomsRest")));
	TestTrue(TEXT("At the family plot, with no spot of its own"), !Read->bHasPlayerSpot
		&& Read->GetArrivalOn(TEXT("/Game/Maps/Lvl_RansomsRest")) == StationBoard::FirstArrivalLanding());

	// On a practice visit later: the gangplank is down and "Board the skiff" never shows.
	TestTrue(TEXT("Skyreach's gangplank is down for them"), ASkiffJetty::IsGangplankDownFor(Read->Progress.bTutorialDone, Read->Campaign.bFirstCastOff));
	TestFalse(TEXT("No boarding mission for them"), ASkiffJetty::ShowsBoardingFor(Read->Progress.bTutorialDone, Read->Campaign.bFirstCastOff));

	// Without the Bullpup's kind the rest still holds, unarmed.
	ULooterSessionSave* Unarmed = NewObject<ULooterSessionSave>();
	TestFalse(TEXT("No Bullpup: not armed"), USessionSubsystem::ApplyTutorialSkip(*Unarmed, *First, nullptr));
	TestTrue(TEXT("... but skipped all the same"), Unarmed->Progress.bTutorialDone && Unarmed->Campaign.bFirstCastOff && !Unarmed->bHasInventory);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationArrivalsTest, "Looter.Station.Arrivals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStationArrivalsTest::RunTest(const FString& Parameters)
{
	// Where trips arrive, and how: the first cast-off at the family plot behind the white with the game's title, every
	// other trip at its station (a jetty and a station carry theirs), the white plain.
	TestEqual(TEXT("The first arrival's title"), UAreaTravelSubsystem::ArrivalTitle(StationBoard::FirstArrivalLanding()).ToString(), FString(TEXT("REVENANT")));
	TestTrue(TEXT("Any other arrival's: none"), UAreaTravelSubsystem::ArrivalTitle(TEXT("Landing_Depot")).IsEmpty()
		&& UAreaTravelSubsystem::ArrivalTitle(TEXT("Landing_Jetty")).IsEmpty() && UAreaTravelSubsystem::ArrivalTitle(NAME_None).IsEmpty());

	// The first cast-off's trip, as CompleteFirstCastOff writes it: from Skyreach, Skyreach's world kept, to the family plot.
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->Map = IslandMap;
	Save->bHasPlayerSpot = true;
	Save->FindOrAddWorld(IslandMap).AmmoPickups.AddDefaulted_GetRef().Amount = 24;
	StationBoard::RecordFirstCastOff(Save->Campaign);
	Save->PrepareTrip(TEXT("/Game/Maps/Lvl_RansomsRest"), StationBoard::FirstArrivalLanding());
	const ULooterSessionSave* Arriving = WriteAndRead(Save);
	TestTrue(TEXT("It arrives at the family plot, the story begun, Skyreach's world kept"), Arriving
		&& Arriving->GetArrivalOn(TEXT("/Game/Maps/Lvl_RansomsRest")) == FName(TEXT("Landing_FamilyPlot")) && Arriving->Campaign.bFirstCastOff
		&& Arriving->FindWorld(IslandMap) && Arriving->FindWorld(IslandMap)->AmmoPickups.Num() == 1);

	// The project's areas: a trip to Ransom's Rest arrives at its depot, one to Skyreach at its jetty.
	if (const UAreaDefinition* Rest = UAreaDefinition::FindByName(StationBoard::FirstAreaId().ToString()))
	{
		TestTrue(TEXT("Ransom's Rest's trips arrive at the depot"), Rest->GetDefaultLanding() == FName(TEXT("Landing_Depot")));
	}
	if (const UAreaDefinition* Skyreach = UAreaDefinition::FindByName(TEXT("Skyreach")))
	{
		TestTrue(TEXT("Skyreach's at the jetty"), Skyreach->GetDefaultLanding() == FName(TEXT("Landing_Jetty")));
	}

	// The landings they carry: found by name, the player put on the spot each carries.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ATrainStation* Station = World->SpawnActor<ATrainStation>(FVector(5000.0, 0.0, 0.0), FRotator(0.0, 90.0, 0.0));
	ASkiffJetty* Jetty = World->SpawnActor<ASkiffJetty>(FVector(-5000.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!TestTrue(TEXT("A station and a jetty placed"), Station && Jetty))
	{
		return false;
	}
	TestTrue(TEXT("The station is Landing_Depot"), AreaLandings::Find(World, TEXT("Landing_Depot")) == Station && AreaLandings::IsLanding(Station));
	const FTransform StationSpot = AreaLandings::GetSpot(Station, TEXT("Landing_Depot"));
	const FTransform PlatformSpot = Station->LandingTransform * Station->GetActorTransform();
	TestTrue(TEXT("... arriving on its platform spot, not its middle"), StationSpot.GetLocation().Equals(PlatformSpot.GetLocation(), 0.5)
		&& StationSpot.GetLocation().Equals(Station->Landing->GetComponentLocation(), 0.1)
		&& !StationSpot.GetLocation().Equals(Station->GetActorLocation(), 1.0));
	TestTrue(TEXT("... facing the way it's set"), FMath::IsNearlyEqual(StationSpot.Rotator().Yaw, PlatformSpot.Rotator().Yaw, 0.5));
	TestTrue(TEXT("The jetty is Landing_Jetty"), AreaLandings::Find(World, TEXT("Landing_Jetty")) == Jetty);
	const FTransform JettySpot = AreaLandings::GetSpot(Jetty, TEXT("Landing_Jetty"));
	if (Jetty->Jetty->DoesSocketExist(Jetty->LandingSocket))
	{
		TestTrue(TEXT("... arriving on its deck's landing socket"), JettySpot.GetLocation().Equals(Jetty->Jetty->GetSocketLocation(Jetty->LandingSocket), 0.5));
		TestTrue(TEXT("... facing inland, away from the drop"), FVector::DotProduct(JettySpot.GetRotation().GetForwardVector(), Jetty->GetActorForwardVector()) < -0.9);
	}
	else
	{
		AddError(TEXT("SM_SkiffJetty has no Landing socket: Art/Models/Props/SkiffJetty.py makes it."));
	}
	TestTrue(TEXT("A landing nobody carries is the actor's own place"), AreaLandings::GetSpot(Station, TEXT("Landing_Nowhere")).GetLocation()
		.Equals(Station->GetActorLocation(), 0.1));
	return true;
}

#endif
