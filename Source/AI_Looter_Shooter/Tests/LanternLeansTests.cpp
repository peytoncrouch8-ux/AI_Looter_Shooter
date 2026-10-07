#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Areas/StationBoard.h"
#include "Bestiary/BestiaryEntry.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/DoorHandoff.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLineSet.h"
#include "Tests/LanternLeansTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "World/KeeperLanternPost.h"
#include "World/LanternFlame.h"
#include "World/TrainStation.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Main 7, "The Lantern Leans" (Docs/Areas/RansomsRest.md): the lantern's flame leaning north-east after Main 6, home to
// Delia, who hands Heirloom out through the door once, the depot where Tilly's hearse car waits, and the station board,
// which lists the Gilded Lily once it's read and Ned's page in the Ledger filled in. The train itself is TrainTests.cpp's;
// the Lily's area asset and Main 7's pieces in the built level are LanternLeansPlacedTests.cpp's.

using namespace LanternLeansTestWorld;

namespace
{
	const TCHAR* IslandMap = TEXT("/Game/Maps/Lvl_TutorialIsland");

	/** Where the test level has its pieces (cm): the player at the origin, Delia's door ahead, the depot far off. */
	const FVector DoorAt(300.0, 0.0, 150.0);
	const FVector HandoffAt(290.0, 45.0, 100.0);
	const FVector PlatformAt(5400.0, 0.0, 90.0);

	/** The Heirlooms in the world (loot, or in hand). */
	int32 CountHeirlooms(UWorld* World, const UNamedWeaponDefinition* Heirloom)
	{
		int32 Found = 0;
		for (TActorIterator<AWeaponBase> It(World); It; ++It)
		{
			Found += !It->IsActorBeingDestroyed() && Heirloom && It->GetInstance().Named == Heirloom ? 1 : 0;
		}
		return Found;
	}

	/** Moves a hand-off's beat on for Seconds in tenths. */
	void AdvanceBeat(ADoorHandoff& Handoff, float Seconds)
	{
		for (float Done = 0.f; Done < Seconds - KINDA_SMALL_NUMBER; Done += 0.1f)
		{
			Handoff.Advance(FMath::Min(0.1f, Seconds - Done));
		}
	}

	/** A line set the story script makes, when it's in this checkout. */
	const UStoryLineSet* LoadLines(const TCHAR* Name)
	{
		const FString Package = FString(TEXT("/Game/Data/Story/")) + Name;
		return FPackageName::DoesPackageExist(Package) ? LoadObject<UStoryLineSet>(nullptr, *(Package + TEXT(".") + Name)) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanternLeansMissionTest, "Looter.Story.LanternLeans.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLanternLeansMissionTest::RunTest(const FString& Parameters)
{
	// Main 7 as its asset has it: id exactly Main7, after Main 6 on Ransom's Rest, starting by itself; Delia's door, the
	// depot's place by the hearse car's door, the station board read; 30% of a level, the Lily opened, Heirloom handed over
	// in the story rather than dropped.
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main7")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main7.DA_Mission_Main7"));
		if (TestNotNull(TEXT("DA_Mission_Main7 loads"), Asset))
		{
			TestTrue(TEXT("Main7, a main mission starting by itself on Ransom's Rest, after Main6"), Asset->GetMissionId() == MainSeven
				&& Asset->Kind == EMissionKind::Main && Asset->Start == EMissionStart::Automatic && Asset->Area == FName(TEXT("RansomsRest"))
				&& Asset->Prerequisites == TArray<FName>({ MainSix }));
			TestEqual(TEXT("Its title"), Asset->Title.ToString(), FString(TEXT("The Lantern Leans")));
			TestEqual(TEXT("Three steps"), Asset->Steps.Num(), MainSevenSteps);
			const UMissionTalkObjective* Home = Cast<UMissionTalkObjective>(Asset->GetObjective(0, 0));
			const UMissionReachObjective* Depot = Cast<UMissionReachObjective>(Asset->GetObjective(1, 0));
			const UMissionEventObjective* Board = Cast<UMissionEventObjective>(Asset->GetObjective(2, 0));
			TestTrue(TEXT("1: home to Delia, at her screen door"), Home && Home->SpeakerTag == DeliasDoor);
			TestTrue(TEXT("2: the depot, by the hearse car's door"), Depot && Depot->Place.Actor.ActorTag == DepotPlace && Depot->Place.Radius <= 1200.f);
			TestTrue(TEXT("3: the station board read, the arrow on the station"), Board && Board->Event == StationBoard::ReadEvent()
				&& Board->Waypoint == EMissionWaypoint::Actor && Board->WaypointActor.ActorClass == ATrainStation::StaticClass());
			const FMissionRewards& Rewards = Asset->Rewards;
			TestTrue(TEXT("Its reward: 30% of a level and the Lily opened"), FMath::IsNearlyEqual(Rewards.ExperienceShare, 0.3f)
				&& Rewards.UnlockAreas == TArray<FName>({ LilyId }));
			TestTrue(TEXT("...and Heirloom, handed over by Delia, never dropped"), Rewards.NamedGun == HeirloomId && Rewards.bNamedGunByHand && !Rewards.bGun);
		}
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Main7 isn't made yet: run Tools/Unreal/create_mission_assets.py. The flow below runs on a copy."));
	}

	// The words, as the story script makes them (create_story_lines.py): Delia's line word for word, Hob's at the lean.
	const TCHAR* const LineSets[] = { TEXT("DA_Lines_DeliaMain7"), TEXT("DA_Lines_DeliaMain7After"), TEXT("DA_Lines_HobMain7Lean"),
		TEXT("DA_Lines_HobMain7Depot"), TEXT("DA_Lines_HobMain7Board"), TEXT("DA_Lines_HobMain7"), TEXT("DA_Lines_TillyMain7"),
		TEXT("DA_Lines_TillyAfterMain7") };
	int32 LinesMade = 0;
	for (const TCHAR* Name : LineSets)
	{
		LinesMade += FPackageName::DoesPackageExist(FString(TEXT("/Game/Data/Story/")) + Name) ? 1 : 0;
	}
	if (LinesMade == 0)
	{
		AddWarning(TEXT("Main 7's line sets aren't made yet: run Tools/Unreal/create_story_lines.py."));
	}
	else
	{
		TestEqual(TEXT("Main 7's line sets are made, every one"), LinesMade, static_cast<int32>(UE_ARRAY_COUNT(LineSets)));
		if (const UStoryLineSet* Delia = LoadLines(TEXT("DA_Lines_DeliaMain7")))
		{
			TArray<FString> Said;
			for (const FStoryLine& Line : Delia->Lines)
			{
				Said.Add(Line.Text.ToString());
			}
			TArray<FString> Written;
			for (const TCHAR* Words : DeliasWords)
			{
				Written.Add(Words);
			}
			TestTrue(TEXT("Delia's words, the doc's own"), Said == Written);
		}
		if (const UStoryLineSet* Hob = LoadLines(TEXT("DA_Lines_HobMain7Lean")))
		{
			TestTrue(TEXT("Hob at the lean, the doc's line first"), !Hob->Lines.IsEmpty() && Hob->Lines[0].Text.ToString() == HobsLean);
		}
	}

	// Played through in a test level: Main 6 done, Main 7 starts; Delia's door hands Heirloom out once; the depot; the
	// station board read finishes it, opens the Lily and fills in Ned's page.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(0.0, 0.0, 90.0));
	ASpeakerPoint* Door = PlaceDeliasDoor(World, DoorAt, 180.0);
	AActor* ScreenDoor = MissionTestWorld::SpawnMarker(World, FVector(300.0, -45.0, 60.0));
	ADoorHandoff* Handoff = PlaceHandoff(World, HandoffAt, 180.0, ScreenDoor);
	ATrainStation* Station = World->SpawnActor<ATrainStation>(FVector(6000.0, 0.0, 0.0), FRotator::ZeroRotator);
	AActor* Platform = MissionTestWorld::SpawnMarker(World, PlatformAt, DepotPlace);
	if (!Runner || !Captions || !Player || !Door || !ScreenDoor || !Handoff || !Station || !Platform)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* MainSevenMission = MakeMainSeven(Scratch);
	Runner->BeginForTesting({ MakeMainSix(Scratch), MainSevenMission }, Campaign, Player, Valley);
	Handoff->DispatchBeginPlay();
	const UNamedWeaponDefinition* Heirloom = UNamedWeaponDefinition::FindByName(HeirloomId.ToString());
	if (!Heirloom)
	{
		AddWarning(TEXT("DA_Named_Heirloom isn't made (Tools/Unreal/create_named_weapons.py): the hand-off's gun isn't checked."));
	}

	Runner->Update(0.f);
	TestTrue(TEXT("Main 6 first; Main 7 waits for it"), Runner->IsRunning(MainSix) && Runner->GetStatus(*MainSevenMission) == EMissionStatus::Locked);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainSixDone")));
	TestTrue(TEXT("Main 6 done: Main 7 starts by itself, home to Delia"), Campaign.HasCompleted(MainSix) && Runner->GetStep(MainSeven) == 0);
	TestTrue(TEXT("...the hand-off waits for her"), Handoff->GetState() == EDoorHandoffState::Waiting && !Handoff->GetGun());
	TestTrue(TEXT("...it hands out the mission's named gun"), Handoff->FindGunId() == HeirloomId);

	TestTrue(TEXT("Talked to at her door"), Door->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...on to the depot"), Runner->GetStep(MainSeven), 1);
	if (Heirloom)
	{
		AWeaponBase* Gun = Handoff->GetGun();
		TestTrue(TEXT("Her step done: the hand-off begins"), Handoff->GetState() == EDoorHandoffState::Handing);
		TestTrue(TEXT("...Heirloom in the world at once, as loot, unseen at the crack"), Gun && Gun->IsPickup() && Gun->IsHidden()
			&& Gun->GetInstance().Named == Heirloom && Gun->GetActorLocation().Equals(HandoffAt, 1.0));
		TestEqual(TEXT("...one Heirloom"), CountHeirlooms(World, Heirloom), 1);
		TestEqual(TEXT("The door shut as she starts"), Handoff->GetDoorAngle(), 0.f);
		AdvanceBeat(*Handoff, Handoff->OpenAfter + Handoff->SwingSeconds + 0.1f);
		TestNearlyEqual(TEXT("...then open a crack as she says she kept it back"), Handoff->GetDoorAngle(), Handoff->CrackDegrees, 0.01f);
		TestNearlyEqual(TEXT("...the screen door turned on its hinge"), static_cast<float>(FRotator::NormalizeAxis(ScreenDoor->GetActorRotation().Yaw)),
			Handoff->CrackDegrees, 0.5f);
		TestTrue(TEXT("...the gun not out yet"), Gun && Gun->IsHidden() && !Handoff->IsHeldOut());
		AdvanceBeat(*Handoff, Handoff->GunOutAfter - Handoff->OpenAfter - Handoff->SwingSeconds);
		TestTrue(TEXT("Held out through the crack: seen, loot, where she holds it (not dropped)"), Handoff->GetState() == EDoorHandoffState::Holding
			&& Handoff->IsHeldOut() && Gun && !Gun->IsHidden() && Gun->GetActorLocation().Equals(HandoffAt, 1.0));
		TestFalse(TEXT("...and it waits without ticking"), Handoff->IsActorTickEnabled());

		// Talked to again: her words, and nothing more handed out.
		Captions->Update(30.f);
		Door->SpeakerPoint->Talk(Player);
		TestEqual(TEXT("Talked to again: still one Heirloom"), CountHeirlooms(World, Heirloom), 1);

		// Taken (the backpack takes it: the loot goes): the door eases shut.
		Gun->Destroy();
		Runner->NotifyEvent(FMissionEvent::Interaction(Gun, /*bHeld*/ false));
		TestTrue(TEXT("Taken: the door starts to shut"), Handoff->GetState() == EDoorHandoffState::Closing);
		AdvanceBeat(*Handoff, Handoff->CloseAfterTaken + Handoff->SwingSeconds + 0.1f);
		TestTrue(TEXT("...and shuts"), Handoff->GetState() == EDoorHandoffState::Done && FMath::IsNearlyZero(Handoff->GetDoorAngle())
			&& FMath::IsNearlyZero(FRotator::NormalizeAxis(ScreenDoor->GetActorRotation().Yaw), 0.5));
	}

	// The depot: Tilly's hearse car's door on the platform.
	Runner->Update(0.2f);
	TestEqual(TEXT("Far from the depot, still on the way"), Runner->GetStep(MainSeven), 1);
	Player->SetActorLocation(PlatformAt + FVector(300.0, 0.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("At the hearse car's door: read the station board"), Runner->GetStep(MainSeven), 2);

	// The board read: finished, the Lily open, no second Heirloom from the reward.
	Runner->NotifyEvent(FMissionEvent::Named(StationBoard::ReadEvent(), Station));
	TestTrue(TEXT("Read: Main 7 is finished"), Campaign.HasCompleted(MainSeven) && !Runner->IsRunning(MainSeven));
	TestTrue(TEXT("...the Gilded Lily opened"), Campaign.IsAreaOpen(LilyId));
	if (Heirloom)
	{
		TestEqual(TEXT("...and no Heirloom dropped: Delia's was the reward"), CountHeirlooms(World, Heirloom), 0);
	}

	// Ned's page in the Ledger: his whereabouts, blank until now.
	const FString NedPath = TEXT("/Game/Data/Bestiary/DA_Bestiary_Ned.DA_Bestiary_Ned");
	if (const UBestiaryEntry* Ned = FPackageName::DoesPackageExist(TEXT("/Game/Data/Bestiary/DA_Bestiary_Ned")) ? LoadObject<UBestiaryEntry>(nullptr, *NedPath) : nullptr)
	{
		TestFalse(TEXT("Ned's whereabouts blank on a new story"), Ned->IsFound(FCampaignRecord()));
		TestTrue(TEXT("...written in once Main 7 is done: the Gilded Lily"), Ned->IsFound(Campaign)
			&& Ned->Habitat.ToString().Contains(TEXT("Gilded Lily")));
	}
	else
	{
		AddWarning(TEXT("DA_Bestiary_Ned isn't made: run Tools/Unreal/create_bestiary_pages.py."));
	}

	// The depot's board now: Ransom's Rest (here), the Lily (its level not in the game), Skyreach (practice).
	UAreaDefinition* Skyreach = NewArea(Scratch, TEXT("DA_Area_Skyreach"), TEXT("Skyreach"), IslandMap, TEXT("Landing_Jetty"), true);
	UAreaDefinition* Rest = NewArea(Scratch, TEXT("DA_Area_RansomsRest"), TEXT("Ransom's Rest"), IslandMap, TEXT("Landing_Depot"), false);
	UAreaDefinition* Lily = NewLily(Scratch);
	StationBoard::RecordFirstCastOff(Campaign);
	const TArray<FStationBoardLine> Lines = StationBoard::BuildLines(Campaign, { Skyreach, Rest, Lily }, { MainSevenMission }, Rest->GetAreaId());
	if (TestEqual(TEXT("The depot's board: three lines"), Lines.Num(), 3))
	{
		TestTrue(TEXT("Ransom's Rest, here"), Lines[0].AreaId == Rest->GetAreaId() && Lines[0].bHere);
		TestTrue(TEXT("The Gilded Lily, a destination whose level isn't in the game yet"), Lines[1].AreaId == LilyId && Lines[1].IsDestination()
			&& !Lines[1].bLevelBuilt && Lines[1].Name.ToString() == TEXT("The Gilded Lily"));
		TestTrue(TEXT("...beside Skyreach (practice)"), Lines[2].Kind == EStationLine::Practice && Lines[2].Name.ToString() == TEXT("Skyreach (practice)"));
		TestEqual(TEXT("Choosing the Lily says so, and nobody goes"), StationBoard::NotOpenText(FStationBoardWords::Station(), Lines[1]).ToString(),
			FString(TEXT("The line to the Lily isn't open yet.")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanternLeansHandoffTest, "Looter.Story.LanternLeans.Handoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLanternLeansHandoffTest::RunTest(const FString& Parameters)
{
	// Heirloom is handed out once: a session that comes back past Delia's step gets nothing more (its gun is in hand, or on
	// the porch where it was saved), and Main 7 finished from her step (the console) hands it out once, with no drop.
	const UNamedWeaponDefinition* Heirloom = UNamedWeaponDefinition::FindByName(HeirloomId.ToString());
	if (!Heirloom)
	{
		AddWarning(TEXT("DA_Named_Heirloom isn't made (Tools/Unreal/create_named_weapons.py): skipped."));
		return true;
	}
	for (const bool bResumedPast : { true, false })
	{
		const FString Case = bResumedPast ? TEXT("Resumed on the depot step") : TEXT("Finished from Delia's step");
		FCampaignRecord Campaign;
		Campaign.Complete(MainSix);
		if (bResumedPast)
		{
			Campaign.ActiveMission = MainSeven;
			Campaign.ActiveMissionStep = 1;
		}
		FTestWorldWrapper TestLevel;
		if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
		{
			return false;
		}
		UWorld* World = TestLevel.GetTestWorld();
		UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
		AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(0.0, 0.0, 90.0));
		ADoorHandoff* Handoff = PlaceHandoff(World, HandoffAt, 180.0, nullptr);
		if (!Runner || !Player || !Handoff)
		{
			AddError(TEXT("The test level isn't whole."));
			return false;
		}
		UPackage* Scratch = CreatePackage(nullptr);
		Runner->BeginForTesting({ MakeMainSix(Scratch), MakeMainSeven(Scratch) }, Campaign, Player, Valley);
		Handoff->DispatchBeginPlay();
		Runner->Update(0.f);
		if (bResumedPast)
		{
			TestEqual(Case + TEXT(": Main 7 goes on from the depot"), Runner->GetStep(MainSeven), 1);
			TestTrue(Case + TEXT(": nothing handed out"), Handoff->GetState() == EDoorHandoffState::Done && !Handoff->GetGun()
				&& CountHeirlooms(World, Heirloom) == 0);
			Runner->CompleteMission(MainSeven);
			TestEqual(Case + TEXT(": finished, still no Heirloom (the session has it)"), CountHeirlooms(World, Heirloom), 0);
		}
		else
		{
			TestEqual(Case + TEXT(": Main 7 at Delia's door"), Runner->GetStep(MainSeven), 0);
			Runner->CompleteMission(MainSeven);
			TestTrue(Case + TEXT(": finished, Heirloom handed out"), Campaign.HasCompleted(MainSeven) && Handoff->GetState() == EDoorHandoffState::Handing);
			TestEqual(Case + TEXT(": one Heirloom, none dropped beside it"), CountHeirlooms(World, Heirloom), 1);
			TestFalse(Case + TEXT(": no second one"), Handoff->HandOut());
			AdvanceBeat(*Handoff, Handoff->GunOutAfter + 0.1f);
			TestTrue(Case + TEXT(": held out in the end, with no door to open"), Handoff->IsHeldOut());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanternLeansLeanTest, "Looter.Story.LanternLeans.Lean",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLanternLeansLeanTest::RunTest(const FString& Parameters)
{
	// The lean as a rule: north-east is as far north (+X) as east (+Y), off upright by its angle, the tip up.
	const FVector Axis = LanternLean::Axis(45.f, 38.f);
	TestTrue(TEXT("North-east: as far north as east"), Axis.X > 0.0 && FMath::IsNearlyEqual(Axis.X, Axis.Y, 1e-4));
	TestNearlyEqual(TEXT("...38 degrees off upright"), static_cast<float>(FMath::RadiansToDegrees(FMath::Acos(Axis.Z))), 38.f, 0.01f);
	TestTrue(TEXT("No lean: upright"), LanternLean::Axis(45.f, 0.f).Equals(FVector::UpVector, 1e-4));
	TestTrue(TEXT("East is +Y"), LanternLean::Axis(90.f, 90.f).Equals(FVector(0.0, 1.0, 0.0), 1e-4));
	TestTrue(TEXT("The turn stands a flame on it"), LanternLean::Turn(45.f, 38.f).RotateVector(FVector::UpVector).Equals(Axis, 1e-4));

	// Hung on a lantern turned any way: out until Main 6 is done, then lit and leaning north-east in the world.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector(0.0, 0.0, 90.0));
	AActor* Lantern = MissionTestWorld::SpawnMarker(World, FVector(400.0, 0.0, 200.0));
	ALanternFlame* Flame = World->SpawnActor<ALanternFlame>(FVector(400.0, 0.0, 212.0), FRotator::ZeroRotator);
	if (!Runner || !Player || !Lantern || !Flame)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Lantern->SetActorRotation(FRotator(0.0, 137.0, 0.0));
	Flame->AttachToActor(Lantern, FAttachmentTransformRules::KeepWorldTransform);
	UPackage* Scratch = CreatePackage(nullptr);
	Runner->BeginForTesting({ MakeMainSix(Scratch), MakeMainSeven(Scratch) }, Campaign, Player, Valley);
	Flame->DispatchBeginPlay();
	Runner->Update(0.f);
	TestFalse(TEXT("It never ticks"), Flame->PrimaryActorTick.bCanEverTick);
	TestTrue(TEXT("North-east by default (toward the Lily), shown after Main 6"), FMath::IsNearlyEqual(Flame->Bearing, 45.f)
		&& Flame->ShownWhen.AfterMissions == TArray<FName>({ MainSix }));
	TestTrue(TEXT("During Main 6: out"), !Flame->IsLit() && !Flame->Flame->IsVisible());
	TestTrue(TEXT("...a flame of crossed cards, one draw"), Flame->Flame->GetInstanceCount() >= 3);

	// Hung in the keeper's post's lantern: lit the moment Abel lights it in his scene, before Main 6 is over.
	AKeeperLanternPost* Post = World->SpawnActor<AKeeperLanternPost>(FVector(-400.0, 0.0, 0.0), FRotator::ZeroRotator);
	ALanternFlame* PostFlame = World->SpawnActor<ALanternFlame>(FVector(-400.0, 0.0, 250.0), FRotator::ZeroRotator);
	if (TestTrue(TEXT("A keeper's post, a flame in its lantern"), Post && PostFlame))
	{
		Post->bKeepersPost = true;
		PostFlame->AttachToActor(Post, FAttachmentTransformRules::KeepWorldTransform);
		Post->DispatchBeginPlay();
		PostFlame->DispatchBeginPlay();
		TestFalse(TEXT("...out while the lantern is dark"), PostFlame->IsLit());
		Post->LightKeepersLantern();
		TestTrue(TEXT("Abel lights the lantern: its flame with it, Main 6 not over yet"), PostFlame->IsLit() && !Campaign.HasCompleted(MainSix));
		PostFlame->RefreshStory();
		TestTrue(TEXT("...and the story doesn't put it out while the lantern burns"), PostFlame->IsLit());
	}

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainSixDone")));
	TestTrue(TEXT("Main 6 done: lit"), Flame->IsLit() && Flame->Flame->IsVisible());
	TestTrue(TEXT("...leaning north-east, whichever way its lantern hangs"), Flame->GetFlameAxis().Equals(LanternLean::Axis(45.f, Flame->LeanDegrees), 1e-3));
	Lantern->SetActorRotation(FRotator(0.0, -20.0, 0.0));
	Flame->Lean();
	TestTrue(TEXT("...and still north-east once the lantern has turned"), Flame->GetFlameAxis().Equals(LanternLean::Axis(45.f, Flame->LeanDegrees), 1e-3));
	return true;
}

#endif
