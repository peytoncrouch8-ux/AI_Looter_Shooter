#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/StationBoard.h"
#include "Scenes/ColdOpenSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tutorial/TutorialDirector.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"
#include "World/SkiffJetty.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

// A new game (the user's call, 2026-10-13): it starts straight on the story, Ransom's Rest's cold open and then Ellis's
// grave, with the Common Bullpup Ellis was buried with; no "Start on Skyreach / Skip the tutorial" choice. The controls are
// taught there by the contextual hints (Tests/ControlHintOpeningTests.cpp). Skyreach is the story's third area now, so no
// station board lists it; sessions begun on Skyreach before this still continue where they were.

namespace
{
	const TCHAR* RestMap = TEXT("/Game/Maps/Lvl_RansomsRest");
	const TCHAR* IslandMap = TEXT("/Game/Maps/Lvl_TutorialIsland");

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewGameSaveTest, "Looter.NewGame.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNewGameSaveTest::RunTest(const FString& Parameters)
{
	// What a new game writes before its level opens (USessionSubsystem::ApplyNewGame): the story begun (the record's first
	// cast-off, so the cold open is due), a Common Bullpup in hand with its starting magazines ("in the coffin"), level 1,
	// arriving at the family plot's grave on Ransom's Rest. Read back through the save format, as the level reads it.
	UWeaponDefinition* Bullpup = USessionSubsystem::LoadBullpup();
	if (!TestNotNull(TEXT("The Bullpup's definition"), Bullpup))
	{
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UAreaDefinition* First = NewObject<UAreaDefinition>(Scratch, TEXT("DA_Area_RansomsRest"), RF_Transient);
	First->DisplayName = FText::FromString(TEXT("Ransom's Rest"));
	First->Map = FSoftObjectPath(FString::Printf(TEXT("%s.%s"), RestMap, *FPackageName::GetShortName(RestMap)));
	First->Landings = { FName(TEXT("Landing_Depot")) };
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	TestTrue(TEXT("Applied, armed"), USessionSubsystem::ApplyNewGame(*Save, *First, Bullpup));

	const ULooterSessionSave* Read = WriteAndRead(Save);
	if (!TestNotNull(TEXT("Read back"), Read))
	{
		return false;
	}
	TestTrue(TEXT("The story has begun, its first area open"), Read->Campaign.bFirstCastOff
		&& Read->Campaign.IsAreaOpen(StationBoard::FirstAreaId()) && StationBoard::IsAreaOpen(Read->Campaign, StationBoard::FirstAreaId()));
	TestTrue(TEXT("...so the cold open is due, once"), UColdOpenSubsystem::IsDue(Read->Campaign) && !Read->Campaign.bColdOpenSeen);
	TestTrue(TEXT("...and nothing of the story done yet"), Read->Campaign.CompletedMissions.IsEmpty() && Read->Campaign.ActiveMission.IsNone());
	TestEqual(TEXT("Level 1, as every new session arrives"), Read->Progress.Level, 1);
	TestTrue(TEXT("It carries what the save says"), Read->bHasInventory);
	if (TestEqual(TEXT("One gun"), Read->Inventory.Equipped.Num(), 1))
	{
		const FWeaponInstanceData& Gun = Read->Inventory.Equipped[0];
		TestTrue(TEXT("A Bullpup"), Gun.Definition == Bullpup);
		TestTrue(TEXT("Common"), Gun.Rarity == EWeaponRarity::Common);
		TestEqual(TEXT("Level 1"), Gun.Level, 1);
		TestTrue(TEXT("Built from its parts"), Gun.Parts.Num() > 0);
		const int32 AmmoIndex = static_cast<int32>(Bullpup->AmmoType);
		const int32 Rounds = Read->Inventory.Ammo.IsValidIndex(AmmoIndex) ? Read->Inventory.Ammo[AmmoIndex] : 0;
		TestEqual(TEXT("Its starting magazines"), Rounds, Gun.Stats.MagazineSize * Bullpup->StartingReserveMagazines);
	}
	TestEqual(TEXT("In hand"), Read->Inventory.ActiveSlot, 0);
	TestTrue(TEXT("Nothing in the backpack"), Read->Inventory.Backpack.IsEmpty());
	TestEqual(TEXT("It continues on the first area's level"), Read->Map, FString(RestMap));
	TestTrue(TEXT("At the family plot, with no spot of its own"), !Read->bHasPlayerSpot
		&& Read->GetArrivalOn(RestMap) == StationBoard::FirstArrivalLanding());

	// Skyreach comes later in the story: a visit there never runs its old tutorial or offers the skiff's first trip.
	TestTrue(TEXT("Skyreach's old tutorial counts as done"), Read->Progress.bTutorialDone
		&& ATutorialDirector::DecideStart(Read->Progress.bTutorialDone, Read->Campaign.HasCompleted(TEXT("Tutorial")),
			Read->Campaign.HasCompleted(TEXT("WebHollow"))) == ETutorialStart::Done);
	TestTrue(TEXT("...its jetty's gangplank down"), ASkiffJetty::IsGangplankDownFor(Read->Progress.bTutorialDone, Read->Campaign.bFirstCastOff));
	TestFalse(TEXT("...and no boarding mission"), ASkiffJetty::ShowsBoardingFor(Read->Progress.bTutorialDone, Read->Campaign.bFirstCastOff));

	// Without the Bullpup's kind the rest still holds, unarmed.
	ULooterSessionSave* Unarmed = NewObject<ULooterSessionSave>();
	TestFalse(TEXT("No Bullpup: not armed"), USessionSubsystem::ApplyNewGame(*Unarmed, *First, nullptr));
	TestTrue(TEXT("...but the story begun all the same"), Unarmed->Campaign.bFirstCastOff && Unarmed->Progress.bTutorialDone
		&& !Unarmed->bHasInventory && Unarmed->GetArrivalOn(RestMap) == StationBoard::FirstArrivalLanding());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNewGameStoryStartTest, "Looter.NewGame.StoryStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNewGameStoryStartTest::RunTest(const FString& Parameters)
{
	// The project as it is: the story's first area and its level are in the game, so an empty slot's New Game starts there
	// (USessionSubsystem::PlaySession hands it to StartNewGame), arriving at the family plot with the title.
	TestEqual(TEXT("The story's first area"), StationBoard::FirstAreaId(), FName(TEXT("RansomsRest")));
	TestEqual(TEXT("...its first arrival: Ellis's grave in the family plot"), StationBoard::FirstArrivalLanding(), FName(TEXT("Landing_FamilyPlot")));
	const UAreaDefinition* Rest = UAreaDefinition::FindByName(StationBoard::FirstAreaId().ToString());
	if (!TestNotNull(TEXT("DA_Area_RansomsRest (Tools/Unreal/create_area_assets.py)"), Rest))
	{
		return false;
	}
	TestTrue(TEXT("Its level is in the game"), Rest->HasMap() && Rest->GetMapPackage().Equals(RestMap, ESearchCase::IgnoreCase));
	TestTrue(TEXT("A new game can start on the story"), USessionSubsystem::CanStartStory());

	// The cold open plays once: a session continued after it (seen) or a level played without the story begun never has it.
	FCampaignRecord Begun;
	StationBoard::RecordFirstCastOff(Begun);
	TestTrue(TEXT("Due on a new game"), UColdOpenSubsystem::IsDue(Begun));
	FCampaignRecord Seen = Begun;
	Seen.bColdOpenSeen = true;
	TestFalse(TEXT("Not once seen (played or skipped)"), UColdOpenSubsystem::IsDue(Seen));
	TestFalse(TEXT("Not without the story begun (Looter.PlayLevel straight on the level)"), UColdOpenSubsystem::IsDue(FCampaignRecord()));

	// An old session saved mid-Skyreach continues there, as it was; a level that's gone continues on the game's first level.
	if (FPackageName::DoesPackageExist(IslandMap))
	{
		TestEqual(TEXT("A session saved on Skyreach continues on Skyreach"), USessionSubsystem::ContinueMap(IslandMap), FString(IslandMap));
	}
	TestEqual(TEXT("A session saved in a level that's gone continues on the game's first level"),
		USessionSubsystem::ContinueMap(TEXT("/Game/Maps/Lvl_NotBuiltYet")), USessionSubsystem::ContinueMap(FString()));

	// The map's order: 1 Ransom's Rest, 2 the Gilded Lily, 3 Skyreach (reached later from the Lily). No board lists Skyreach.
	const TArray<UAreaDefinition*> Areas = UAreaDefinition::LoadAll();
	const UAreaDefinition* Skyreach = UAreaDefinition::FindByName(TEXT("Skyreach"));
	auto ListsSkyreach = [Skyreach](const TArray<FStationBoardLine>& Lines)
	{
		return Skyreach && Lines.ContainsByPredicate([Skyreach](const FStationBoardLine& Line) { return Line.AreaId == Skyreach->GetAreaId(); });
	};
	const TArray<FStationBoardLine> AtTheDepot = StationBoard::BuildLines(Begun, Areas, {}, StationBoard::FirstAreaId());
	TestTrue(TEXT("The depot's board: Ransom's Rest, here"), AtTheDepot.ContainsByPredicate([](const FStationBoardLine& Line)
		{ return Line.AreaId == StationBoard::FirstAreaId() && Line.bHere; }));
	TestFalse(TEXT("...and no Skyreach"), ListsSkyreach(AtTheDepot));
	const FName LilyId(TEXT("GildedLily"));
	if (UAreaDefinition::FindByName(LilyId.ToString()))
	{
		FCampaignRecord LilyOpen = Begun;
		LilyOpen.OpenArea(LilyId);
		const TArray<FStationBoardLine> Later = StationBoard::BuildLines(LilyOpen, Areas, {}, StationBoard::FirstAreaId());
		const int32 RestAt = Later.IndexOfByPredicate([](const FStationBoardLine& Line) { return Line.AreaId == StationBoard::FirstAreaId(); });
		const int32 LilyAt = Later.IndexOfByPredicate([LilyId](const FStationBoardLine& Line) { return Line.AreaId == LilyId; });
		TestTrue(TEXT("Once the Lily opens: Ransom's Rest, then the Lily"), RestAt != INDEX_NONE && LilyAt != INDEX_NONE && RestAt < LilyAt);
		TestFalse(TEXT("...and still no Skyreach"), ListsSkyreach(Later));
	}
	else
	{
		AddWarning(TEXT("DA_Area_GildedLily isn't made: run Tools/Unreal/create_area_assets.py."));
	}
	return true;
}

#endif
