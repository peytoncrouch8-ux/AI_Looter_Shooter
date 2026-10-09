#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Creatures/CreatureBase.h"
#include "Loot/WeaponRack.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "Tutorial/TutorialDirector.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "World/NoticeBoard.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	/**
	 * An objective's rule in a line (its kind and every setting that decides when it's done and where it points) and its
	 * words (in full, and the tracker's short line and key hint), to compare two copies.
	 */
	FString RuleOf(const UMissionObjective* Objective)
	{
		if (!Objective)
		{
			return TEXT("none");
		}
		FString Rule = FString::Printf(TEXT("%s \"%s\" waypoint=%d on=%s pass=%d count=%d"), *Objective->GetClass()->GetName(), *Objective->Text.ToString(),
			static_cast<int32>(Objective->Waypoint), *GetNameSafe(Objective->WaypointActor.ActorClass.Get()),
			static_cast<int32>(Objective->bPassWithoutTargets), static_cast<int32>(Objective->bShowCount));
		Rule += FString::Printf(TEXT(" short=\"%s\" hint=%s \"%s\""), *Objective->ShortText.ToString(), *Objective->HintAction.ToString(),
			*Objective->HintText.ToString());
		if (const UMissionTravelObjective* Travel = Cast<UMissionTravelObjective>(Objective))
		{
			Rule += FString::Printf(TEXT(" distance=%.0f"), Travel->Distance);
		}
		else if (const UMissionReachObjective* Reach = Cast<UMissionReachObjective>(Objective))
		{
			Rule += FString::Printf(TEXT(" place=%s radius=%.0f flat=%d"), *GetNameSafe(Reach->Place.Actor.ActorClass.Get()), Reach->Place.Radius,
				static_cast<int32>(Reach->Place.bIgnoreHeight));
		}
		else if (const UMissionCollectObjective* Collect = Cast<UMissionCollectObjective>(Objective))
		{
			Rule += FString::Printf(TEXT(" what=%d count=%d"), static_cast<int32>(Collect->What), Collect->Count);
		}
		else if (const UMissionHitObjective* Hit = Cast<UMissionHitObjective>(Objective))
		{
			Rule += FString::Printf(TEXT(" target=%s count=%d player=%d"), *GetNameSafe(Hit->Target.ActorClass.Get()), Hit->Count,
				static_cast<int32>(Hit->bPlayerHitsOnly));
		}
		else if (const UMissionKillObjective* Kill = Cast<UMissionKillObjective>(Objective))
		{
			Rule += FString::Printf(TEXT(" target=%s count=%d player=%d"), *GetNameSafe(Kill->Target.ActorClass.Get()), Kill->Count,
				static_cast<int32>(Kill->bPlayerKillsOnly));
		}
		else if (const UMissionOpenPageObjective* Open = Cast<UMissionOpenPageObjective>(Objective))
		{
			Rule += FString::Printf(TEXT(" page=%d"), static_cast<int32>(Open->Page));
		}
		else if (const UMissionEventObjective* Event = Cast<UMissionEventObjective>(Objective))
		{
			Rule += FString::Printf(TEXT(" event=%s count=%d"), *Event->Event.ToString(), Event->Count);
		}
		return Rule;
	}

	/** The project's mission with this id, or null. */
	UMissionDefinition* FindMissionAsset(FName MissionId)
	{
		for (UMissionDefinition* Mission : UMissionDefinition::LoadAll())
		{
			if (Mission->GetMissionId() == MissionId)
			{
				return Mission;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTrackingTest, "Looter.Missions.Tracking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTrackingTest::RunTest(const FString& Parameters)
{
	// The subsystem's bookkeeping, without a world: what gets tracked as missions come and go.
	FMissionBook Book;
	TestTrue(TEXT("Nothing tracked at first"), Book.GetTracked() == INDEX_NONE);
	TestTrue(TEXT("Unknown id finds nothing"), Book.Find(1) == nullptr);

	const int32 First = Book.Add(FText::FromString(TEXT("First")));
	const int32 Second = Book.Add(FText::FromString(TEXT("Second")));
	const int32 Third = Book.Add(FText::FromString(TEXT("Third")));
	TestTrue(TEXT("Ids differ"), First != Second && Second != Third && First != Third);
	TestTrue(TEXT("Ids are real"), First != INDEX_NONE);
	TestTrue(TEXT("The first mission is tracked by itself"), Book.GetTracked() == First);
	TestTrue(TEXT("Later ones don't take over"), Book.GetMissions().Num() == 3 && Book.GetTracked() == First);
	TestTrue(TEXT("Listed in the order added"), Book.GetMissions()[1].Id == Second);
	TestTrue(TEXT("Title kept"), Book.Find(Second) && Book.Find(Second)->Title.ToString() == TEXT("Second"));

	// Picking another (a mission log would).
	TestTrue(TEXT("Track another"), Book.Track(Third));
	TestTrue(TEXT("It's tracked"), Book.GetTracked() == Third);
	TestFalse(TEXT("Tracking it again changes nothing"), Book.Track(Third));
	TestFalse(TEXT("Unknown id is ignored"), Book.Track(999));
	TestTrue(TEXT("Still tracked after an unknown id"), Book.GetTracked() == Third);

	// Objectives: text and a place. Only what a list shows (text, a waypoint coming or going) counts as a change.
	TestTrue(TEXT("New objective is a change"), Book.SetObjective(Third, FText::FromString(TEXT("Go")), FVector(100.f, 0.f, 0.f)));
	TestTrue(TEXT("Objective text kept"), Book.Find(Third)->Objective.ToString() == TEXT("Go"));
	TestTrue(TEXT("Waypoint kept"), Book.Find(Third)->Waypoint.IsSet() && Book.Find(Third)->Waypoint->Equals(FVector(100.f, 0.f, 0.f)));
	TestFalse(TEXT("Same again is no change"), Book.SetObjective(Third, FText::FromString(TEXT("Go")), FVector(100.f, 0.f, 0.f)));
	TestFalse(TEXT("A moving waypoint is no change"), Book.SetObjective(Third, FText::FromString(TEXT("Go")), FVector(150.f, 0.f, 0.f)));
	TestTrue(TEXT("But it moved"), Book.Find(Third)->Waypoint->Equals(FVector(150.f, 0.f, 0.f)));
	TestTrue(TEXT("Losing the waypoint is a change"), Book.SetObjective(Third, FText::FromString(TEXT("Go")), TOptional<FVector>()));
	TestFalse(TEXT("No waypoint now"), Book.Find(Third)->Waypoint.IsSet());
	TestTrue(TEXT("New text is a change"), Book.SetObjective(Third, FText::FromString(TEXT("Come back")), TOptional<FVector>()));
	TestFalse(TEXT("Unknown mission's objective is ignored"), Book.SetObjective(999, FText::FromString(TEXT("Nope")), FVector::ZeroVector));

	// The HUD tracker's parts: new ones are a change to show, the same ones again aren't, and the words count to the letter.
	const FText ComeBack = FText::FromString(TEXT("Come back"));
	FMissionTrackerParts Parts;
	Parts.Line = TEXT("Come back");
	Parts.Count = TEXT("0 / 2");
	Parts.CountDone = TEXT("2 / 2");
	Parts.Required = 2;
	Parts.Step = 1;
	Parts.StepCount = 3;
	TestTrue(TEXT("New tracker parts are a change"), Book.SetObjective(Third, ComeBack, TOptional<FVector>(), Parts));
	TestTrue(TEXT("The parts are kept"), Book.Find(Third)->Tracker.SameAs(Parts));
	TestFalse(TEXT("The same parts again are no change"), Book.SetObjective(Third, ComeBack, TOptional<FVector>(), Parts));
	Parts.Count = TEXT("1 / 2");
	Parts.Progress = 1;
	TestTrue(TEXT("A count that rose is a change, the words the same"), Book.SetObjective(Third, ComeBack, TOptional<FVector>(), Parts));
	Parts.Line = TEXT("come back");
	TestTrue(TEXT("A line changed only in its case is a change"), Book.SetObjective(Third, ComeBack, TOptional<FVector>(), Parts));
	Parts.HintKey = TEXT("R");
	TestTrue(TEXT("A new hint is a change"), Book.SetObjective(Third, ComeBack, TOptional<FVector>(), Parts));
	TestEqual(TEXT("and kept"), Book.Find(Third)->Tracker.HintKey, FString(TEXT("R")));
	Parts.ObjectiveIndex = 1;
	TestTrue(TEXT("The step's next objective is a change, the words the same"), Book.SetObjective(Third, ComeBack, TOptional<FVector>(), Parts));
	TestEqual(TEXT("and kept"), Book.Find(Third)->Tracker.ObjectiveIndex, 1);
	Parts.bCountIsTime = true;
	TestTrue(TEXT("A count that turns out to be seconds is a change"), Book.SetObjective(Third, ComeBack, TOptional<FVector>(), Parts));
	TestTrue(TEXT("and kept"), Book.Find(Third)->Tracker.bCountIsTime);

	// Removing: an untracked one leaves tracking alone; the tracked one hands over to the mission that took its place.
	TestTrue(TEXT("Remove untracked"), Book.Remove(First));
	TestTrue(TEXT("Tracking unchanged"), Book.GetTracked() == Third);
	TestFalse(TEXT("Removing it again does nothing"), Book.Remove(First));
	const int32 Fourth = Book.Add(FText::FromString(TEXT("Fourth")));
	TestTrue(TEXT("The middle one is tracked"), Book.GetTracked() == Third);
	TestTrue(TEXT("Remove the tracked one"), Book.Remove(Third));
	TestTrue(TEXT("The next one is tracked"), Book.GetTracked() == Fourth);
	TestTrue(TEXT("Remove the last, tracked"), Book.Remove(Fourth));
	TestTrue(TEXT("The one before it is tracked"), Book.GetTracked() == Second);
	TestTrue(TEXT("Remove the only one"), Book.Remove(Second));
	TestTrue(TEXT("Nothing tracked when none are left"), Book.GetTracked() == INDEX_NONE && Book.GetMissions().IsEmpty());

	// Tracking none, then adding: the new mission is what the player does now.
	const int32 Fifth = Book.Add(FText::FromString(TEXT("Fifth")));
	TestTrue(TEXT("Track none"), Book.Track(INDEX_NONE));
	TestTrue(TEXT("None tracked"), Book.GetTracked() == INDEX_NONE);
	const int32 Sixth = Book.Add(FText::FromString(TEXT("Sixth")));
	TestTrue(TEXT("Added while none tracked: tracked"), Book.GetTracked() == Sixth && Sixth != Fifth);
	TestTrue(TEXT("Ids aren't reused"), Sixth > Fourth);

	// The tutorial has a name to show as a mission.
	TestFalse(TEXT("Tutorial mission title"), GetDefault<ATutorialDirector>()->MissionTitle.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMinimapMissionArrowTest, "Looter.UI.Minimap.MissionArrow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMinimapMissionArrowTest::RunTest(const FString& Parameters)
{
	// 1 px per meter: a waypoint within 70 px of the center shows on the map, a farther one as an arrow 80 px out.
	const float PixelsPerCm = 0.01f;
	const float Inside = 70.f;
	const float Rim = 80.f;

	// Far ahead (north, facing north): at the top of the rim, pointing up.
	const FMinimapWaypoint Ahead = UHudMinimapWidget::PlaceWaypoint(FVector(20000.f, 0.f, 0.f), 0.f, PixelsPerCm, Inside, Rim);
	TestFalse(TEXT("Far ahead is past the rim"), Ahead.bInside);
	TestTrue(TEXT("Far ahead sits at the top of the rim"), Ahead.Position.Equals(FVector2D(0.0, -80.0), 1e-3));
	TestTrue(TEXT("Far ahead points up"), FMath::IsNearlyEqual(Ahead.Angle, 0.f, 1e-3f));

	// Far to the east while facing north: right, pointing right (90 degrees clockwise).
	const FMinimapWaypoint Right = UHudMinimapWidget::PlaceWaypoint(FVector(0.f, 20000.f, 0.f), 0.f, PixelsPerCm, Inside, Rim);
	TestFalse(TEXT("Far right is past the rim"), Right.bInside);
	TestTrue(TEXT("Far right sits on the right of the rim"), Right.Position.Equals(FVector2D(80.0, 0.0), 1e-3));
	TestTrue(TEXT("Far right points right"), FMath::IsNearlyEqual(Right.Angle, 90.f, 1e-3f));

	// Behind and to the left.
	const FMinimapWaypoint Behind = UHudMinimapWidget::PlaceWaypoint(FVector(-20000.f, 0.f, 0.f), 0.f, PixelsPerCm, Inside, Rim);
	TestTrue(TEXT("Behind points down"), FMath::IsNearlyEqual(FMath::Abs(Behind.Angle), 180.f, 1e-3f));
	const FMinimapWaypoint Left = UHudMinimapWidget::PlaceWaypoint(FVector(0.f, -20000.f, 0.f), 0.f, PixelsPerCm, Inside, Rim);
	TestTrue(TEXT("Left points left"), FMath::IsNearlyEqual(Left.Angle, -90.f, 1e-3f));

	// The map turns with the view: facing east, east is ahead.
	const FMinimapWaypoint TurnedEast = UHudMinimapWidget::PlaceWaypoint(FVector(0.f, 20000.f, 0.f), 90.f, PixelsPerCm, Inside, Rim);
	TestTrue(TEXT("Facing it, it points up"), FMath::IsNearlyEqual(TurnedEast.Angle, 0.f, 1e-3f));
	TestTrue(TEXT("Facing it, it's at the top"), TurnedEast.Position.Equals(FVector2D(0.0, -80.0), 1e-3));

	// Close by: on the map itself, where it is.
	const FMinimapWaypoint Near = UHudMinimapWidget::PlaceWaypoint(FVector(3000.f, 0.f, 500.f), 0.f, PixelsPerCm, Inside, Rim);
	TestTrue(TEXT("Near is inside"), Near.bInside);
	TestTrue(TEXT("Near sits where it is"), Near.Position.Equals(FVector2D(0.0, -30.0), 1e-3));
	TestTrue(TEXT("Just short of the edge is still inside"),
		UHudMinimapWidget::PlaceWaypoint(FVector(0.f, 6950.f, 0.f), 0.f, PixelsPerCm, Inside, Rim).bInside);
	TestTrue(TEXT("Standing on it is inside"), UHudMinimapWidget::PlaceWaypoint(FVector::ZeroVector, 45.f, PixelsPerCm, Inside, Rim).bInside);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionDefinitionsTest, "Looter.Missions.Definitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionDefinitionsTest::RunTest(const FString& Parameters)
{
	// The project's missions (DA_Mission_* in /Game/Data/Missions; Tools/Unreal/create_mission_assets.py makes the first two),
	// now and later: each whole, its id its own, its prerequisites missions that exist, its area an area that exists.
	const TArray<UMissionDefinition*> Missions = UMissionDefinition::LoadAll();
	const UMissionDefinition* Tutorial = FindMissionAsset(TEXT("Tutorial"));
	const UMissionDefinition* Test = FindMissionAsset(TEXT("Test"));
	if (!Tutorial || !Test)
	{
		AddError(TEXT("DA_Mission_Tutorial and DA_Mission_Test belong in /Game/Data/Missions: run Tools/Unreal/create_mission_assets.py in the editor."));
		return false;
	}
	TSet<FName> AreaIds;
	for (const UAreaDefinition* Area : UAreaDefinition::LoadAll())
	{
		AreaIds.Add(Area->GetAreaId());
	}
	TSet<FName> MissionIds;
	for (const UMissionDefinition* Mission : Missions)
	{
		MissionIds.Add(Mission->GetMissionId());
	}
	TSet<FName> Seen;
	for (const UMissionDefinition* Mission : Missions)
	{
		const FString Label = Mission->GetName();
		const FName MissionId = Mission->GetMissionId();
		bool bTaken = false;
		Seen.Add(MissionId, &bTaken);
		TestFalse(*FString::Printf(TEXT("%s's id %s is its own"), *Label, *MissionId.ToString()), bTaken);
		TestFalse(*FString::Printf(TEXT("%s has a title"), *Label), Mission->Title.IsEmpty());
		TestTrue(*FString::Printf(TEXT("%s has steps"), *Label), Mission->Steps.Num() > 0);
		for (int32 StepIndex = 0; StepIndex < Mission->Steps.Num(); ++StepIndex)
		{
			const TArray<TObjectPtr<UMissionObjective>>& Objectives = Mission->Steps[StepIndex].Objectives;
			TestTrue(*FString::Printf(TEXT("%s's step %d has objectives"), *Label, StepIndex + 1), Objectives.Num() > 0);
			for (const TObjectPtr<UMissionObjective>& Objective : Objectives)
			{
				TestTrue(*FString::Printf(TEXT("%s's step %d: every objective is there (the asset loaded them)"), *Label, StepIndex + 1), Objective != nullptr);
			}
		}
		for (const FName Needed : Mission->Prerequisites)
		{
			TestTrue(*FString::Printf(TEXT("%s's prerequisite %s is a mission"), *Label, *Needed.ToString()), MissionIds.Contains(Needed) && Needed != MissionId);
		}
		TestTrue(*FString::Printf(TEXT("%s's area %s is an area"), *Label, *Mission->Area.ToString()), Mission->Area.IsNone() || AreaIds.Contains(Mission->Area));
		TestTrue(*FString::Printf(TEXT("%s starts on an event it names"), *Label), Mission->Start != EMissionStart::OnEvent || !Mission->StartEvent.IsNone());

		// Turned in to someone, or finishing by itself, said so: a mission that isn't automatic names its giver.
		const FMissionTurnIn& TurnIn = Mission->TurnIn;
		TestTrue(*FString::Printf(TEXT("%s is turned in to a giver, or automatic"), *Label), TurnIn.bAutomatic || !TurnIn.SpeakerTag.IsNone());
		if (Mission->NeedsTurnIn() && !Mission->Steps.IsEmpty())
		{
			TestFalse(*FString::Printf(TEXT("%s's giver has a name for the tracker"), *Label), Mission->GetGiverName().IsEmpty());
			// Their talk can't also be its last objective: that talk is the turn-in, so the player never talks twice.
			bool bLastTalksToGiver = false;
			for (const TObjectPtr<UMissionObjective>& Objective : Mission->Steps.Last().Objectives)
			{
				const UMissionTalkObjective* Talk = Cast<UMissionTalkObjective>(Objective);
				bLastTalksToGiver |= Talk && Talk->SpeakerTag == TurnIn.SpeakerTag;
			}
			TestFalse(*FString::Printf(TEXT("%s's last objective isn't talking to its giver (that talk is the turn-in)"), *Label), bLastTalksToGiver);
		}
		// Skyreach's missions are its own, outside the story: they finish by themselves or are turned in at the town's
		// notice board (no one else is there to give them), and they give no experience.
		if (Mission->Kind == EMissionKind::Tutorial)
		{
			TestTrue(*FString::Printf(TEXT("%s, on Skyreach, finishes by itself or at the notice board"), *Label),
				TurnIn.bAutomatic || TurnIn.SpeakerTag == ANoticeBoard::GiverTag);
			TestFalse(*FString::Printf(TEXT("%s, on Skyreach, gives no experience"), *Label), Mission->Rewards.GivesExperience());
		}
	}

	// Ransom's Rest's story missions are turned in (the user's call, 2026-10-08), but Main 6, which ends in its own scene
	// with Pa (his board, the lantern lit and the train's steam follow it finished).
	const TCHAR* const TurnedIn[] = { TEXT("Main1"), TEXT("Main2"), TEXT("Main3"), TEXT("Main4"), TEXT("Main5"), TEXT("Main7"),
		TEXT("Side1"), TEXT("Side2") };
	for (const TCHAR* Id : TurnedIn)
	{
		if (const UMissionDefinition* Story = FindMissionAsset(Id))
		{
			TestTrue(*FString::Printf(TEXT("%s is turned in to someone"), Id), Story->NeedsTurnIn());
		}
	}

	// The test mission: kill, reach and interact objectives and some experience, started only by hand (never in play).
	bool bKill = false;
	bool bReach = false;
	bool bInteract = false;
	for (const FMissionStep& Step : Test->Steps)
	{
		for (const TObjectPtr<UMissionObjective>& Objective : Step.Objectives)
		{
			bKill |= Objective && Objective->IsA<UMissionKillObjective>();
			bReach |= Objective && Objective->IsA<UMissionReachObjective>();
			bInteract |= Objective && Objective->IsA<UMissionInteractObjective>();
		}
	}
	TestTrue(TEXT("The test mission kills, reaches and interacts"), bKill && bReach && bInteract);
	TestTrue(TEXT("It gives experience"), Test->Rewards.ExperienceShare > 0.f);
	TestTrue(TEXT("It starts only by hand"), Test->Start == EMissionStart::Manual);
	TestTrue(TEXT("The tutorial is outside the story, started by its director"), Tutorial->Kind == EMissionKind::Tutorial && Tutorial->Start == EMissionStart::Manual);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialMissionTest, "Looter.Missions.TutorialMission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTutorialMissionTest::RunTest(const FString& Parameters)
{
	// The tutorial's first goal is a mission as data. Its built-in steps make the mission the asset holds (an objective per
	// goal, pointing where the step points); the asset says the same; and played by a mission runner, a gun carried and the
	// board read finish it, and the board's postings, which wait on it, go up.
	const ATutorialDirector* Director = GetDefault<ATutorialDirector>();
	UMissionDefinition* BuiltIn = Director->MakeBuiltInMission(GetTransientPackage());
	if (!TestNotNull(TEXT("The built-in steps make a mission"), BuiltIn))
	{
		return false;
	}
	TestTrue(TEXT("Its id is the director's"), BuiltIn->GetMissionId() == Director->MissionId);
	TestEqual(TEXT("Its title is the tutorial's"), BuiltIn->Title.ToString(), Director->MissionTitle);
	TestTrue(TEXT("Outside the story, started by the director, finished by itself"), BuiltIn->Kind == EMissionKind::Tutorial
		&& BuiltIn->Start == EMissionStart::Manual && BuiltIn->TurnIn.bAutomatic);
	if (!TestEqual(TEXT("A step for each of the director's steps"), BuiltIn->Steps.Num(), Director->Steps.Num()))
	{
		return false;
	}
	for (int32 Index = 0; Index < Director->Steps.Num(); ++Index)
	{
		const FTutorialStep& Step = Director->Steps[Index];
		const UMissionObjective* Objective = BuiltIn->GetObjective(Index, 0);
		const FString What = FString::Printf(TEXT("Step %d"), Index + 1);
		TestEqual(*(What + TEXT(": one objective")), BuiltIn->Steps[Index].Objectives.Num(), 1);
		if (!TestNotNull(*(What + TEXT(": has its objective")), Objective))
		{
			continue;
		}
		TestEqual(*(What + TEXT(": the same words")), Objective->Text.ToString(), Step.Text);
		TestEqual(*(What + TEXT(": the tracker's short line")), Objective->ShortText.ToString(), Step.ShortText);
		TestTrue(*(What + TEXT(": no key on the tracker")), Objective->HintAction.IsNone() && Objective->HintText.IsEmpty());
		const int32 Amount = FMath::RoundToInt32(Step.Amount);
		switch (Step.Goal)
		{
		case ETutorialGoal::HoldWeapon:
		{
			const UMissionCollectObjective* Collect = Cast<UMissionCollectObjective>(Objective);
			TestTrue(*(What + TEXT(": carry a gun, the arrow on the rack's")), Collect && Collect->What == EMissionCollect::Weapons
				&& Collect->Count == Amount && Collect->Waypoint == EMissionWaypoint::Actor
				&& Collect->WaypointActor.ActorClass == AWeaponRack::StaticClass());
			break;
		}
		case ETutorialGoal::ReadBoard:
		{
			const UMissionEventObjective* Read = Cast<UMissionEventObjective>(Objective);
			TestTrue(*(What + TEXT(": read the board, no count, the arrow on it")), Read && Read->Event == ANoticeBoard::ReadEvent
				&& Read->Count == Amount && !Read->bShowCount && Read->Waypoint == EMissionWaypoint::Actor
				&& Read->WaypointActor.ActorClass == ANoticeBoard::StaticClass());
			break;
		}
		}
	}

	// The asset (create_mission_assets.py) holds the same mission.
	UMissionDefinition* Asset = FindMissionAsset(Director->MissionId);
	if (!Asset)
	{
		AddError(TEXT("DA_Mission_Tutorial belongs in /Game/Data/Missions: run Tools/Unreal/create_mission_assets.py in the editor."));
	}
	else if (TestEqual(TEXT("The asset has the first goal's steps"), Asset->Steps.Num(), BuiltIn->Steps.Num()))
	{
		TestEqual(TEXT("and its title"), Asset->Title.ToString(), BuiltIn->Title.ToString());
		TestTrue(TEXT("and its kind, start and turn-in"), Asset->Kind == BuiltIn->Kind && Asset->Start == BuiltIn->Start
			&& Asset->TurnIn.bAutomatic == BuiltIn->TurnIn.bAutomatic);
		for (int32 Index = 0; Index < BuiltIn->Steps.Num(); ++Index)
		{
			TestEqual(*FString::Printf(TEXT("The asset's step %d"), Index + 1), RuleOf(Asset->GetObjective(Index, 0)), RuleOf(BuiltIn->GetObjective(Index, 0)));
		}
	}

	// Played in a test level (the asset when there is one): a gun rack 20 m ahead, the notice board past it, and a posting
	// that waits on the first goal, as the board's do.
	FCampaignRecord Campaign;
	int32 Finished = 0;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	AWeaponRack* Rack = World->SpawnActor<AWeaponRack>(FVector(2000.0, 0.0, 0.0), FRotator::ZeroRotator);
	ANoticeBoard* Board = World->SpawnActor<ANoticeBoard>(FVector(2500.0, 800.0, 0.0), FRotator::ZeroRotator);
	if (!TestTrue(TEXT("Runner, display, stand-in, rack and board"), Runner && Display && Player && Rack && Board))
	{
		return false;
	}
	Board->DispatchBeginPlay();
	UMissionDefinition* Played = Asset ? Asset : BuiltIn;
	const FName TutorialId = Played->GetMissionId();
	UMissionDefinition* Posting = MissionTestWorld::NewMission(CreatePackage(nullptr), TEXT("WebHollow"), EMissionKind::Tutorial,
		EMissionStart::Automatic, TEXT("Skyreach"));
	MissionTestWorld::AddObjective<UMissionEventObjective>(Posting, 0)->Event = TEXT("Test.Later");
	Posting->Prerequisites = { TutorialId };
	Runner->OnMissionFinished.AddLambda([&Finished, TutorialId](const UMissionDefinition& Mission, bool) { Finished += Mission.GetMissionId() == TutorialId ? 1 : 0; });
	Runner->BeginForTesting({ Played, Posting }, Campaign, Player, TEXT("Skyreach"));
	Runner->Update(0.f);
	TestFalse(TEXT("It waits for its director"), Runner->IsRunning(TutorialId));
	TestFalse(TEXT("...and the posting waits for it"), Runner->IsRunning(Posting->GetMissionId()));
	TestTrue(TEXT("The director starts it"), Runner->StartMission(TutorialId, 0, /*bForce*/ true) && Runner->GetStep(TutorialId) == 0);
	auto Waypoint = [Display]() { const FMission* Shown = Display->GetTracked(); return Shown ? Shown->Waypoint : TOptional<FVector>(); };
	auto IsAt = [&Waypoint](const FVector& Where) { const TOptional<FVector> Spot = Waypoint(); return Spot.IsSet() && Spot->Equals(Where, 1.0); };
	TestTrue(TEXT("Find a gun: the arrow on the gun rack (no rifle lies on it in a test level)"), IsAt(Rack->GetActorLocation()));

	// Reading the board before the gun is taken doesn't count: that step isn't up yet.
	Board->Read(*Runner);
	TestTrue(TEXT("Read too soon: still finding a gun"), Runner->GetStep(TutorialId) == 0 && !Runner->IsRunning(Posting->GetMissionId()));

	// The stand-in carries no guns; the rifle is taken as if it had been.
	Runner->CompleteStep(TutorialId);
	TestTrue(TEXT("Then the notice board"), Runner->GetStep(TutorialId) == 1);
	TestTrue(TEXT("...the arrow on it"), IsAt(Board->GetActorLocation()));
	Board->Read(*Runner);
	TestFalse(TEXT("Read: the first goal is done"), Runner->IsRunning(TutorialId));
	TestEqual(TEXT("Finished once"), Finished, 1);
	TestTrue(TEXT("The campaign has it finished"), Campaign.HasCompleted(TutorialId));
	TestTrue(TEXT("It was never the campaign's main mission"), Campaign.ActiveMission.IsNone());
	TestTrue(TEXT("The posting waiting on it is up, and the board has the minimap follow it"), Runner->IsRunning(Posting->GetMissionId())
		&& Runner->GetTrackedMission() == Posting->GetMissionId());
	return true;
}

#endif
