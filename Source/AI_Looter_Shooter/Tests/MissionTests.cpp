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
	// The tutorial is a mission as data and behaves as it always did. Its built-in steps make the mission the asset holds
	// (an objective per goal, finishing it as the goal did, pointing where the tutorial pointed); the asset says the same;
	// and played by a mission runner, the steps follow each other as the tutorial's did.
	const ATutorialDirector* Director = GetDefault<ATutorialDirector>();
	UMissionDefinition* BuiltIn = Director->MakeBuiltInMission(GetTransientPackage());
	if (!TestNotNull(TEXT("The built-in steps make a mission"), BuiltIn))
	{
		return false;
	}
	TestTrue(TEXT("Its id is the director's"), BuiltIn->GetMissionId() == Director->MissionId);
	TestEqual(TEXT("Its title is the tutorial's"), BuiltIn->Title.ToString(), Director->MissionTitle);
	TestTrue(TEXT("Outside the story, started by the director"), BuiltIn->Kind == EMissionKind::Tutorial && BuiltIn->Start == EMissionStart::Manual);
	if (!TestEqual(TEXT("A step for each tutorial step"), BuiltIn->Steps.Num(), Director->Steps.Num()))
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
		TestTrue(*(What + TEXT(": the tracker's key hint")), Objective->HintAction == Step.HintAction
			&& Objective->HintText.ToString() == Step.HintText);
		const int32 Amount = FMath::RoundToInt32(Step.Amount);
		switch (Step.Goal)
		{
		case ETutorialGoal::Move:
		{
			const UMissionTravelObjective* Travel = Cast<UMissionTravelObjective>(Objective);
			TestTrue(*(What + TEXT(": walk the distance, the arrow on the gun rack")), Travel && Travel->Distance == Step.Amount
				&& Travel->Waypoint == EMissionWaypoint::Actor && Travel->WaypointActor.ActorClass == AWeaponRack::StaticClass());
			break;
		}
		case ETutorialGoal::ReachRack:
		{
			const UMissionReachObjective* Reach = Cast<UMissionReachObjective>(Objective);
			TestTrue(*(What + TEXT(": reach the gun rack, on the map, passing without one")), Reach && Reach->Place.Actor.ActorClass == AWeaponRack::StaticClass()
				&& Reach->Place.Radius == Step.Amount && Reach->Place.bIgnoreHeight && Reach->bPassWithoutTargets);
			break;
		}
		case ETutorialGoal::HoldWeapon:
		{
			const UMissionCollectObjective* Collect = Cast<UMissionCollectObjective>(Objective);
			TestTrue(*(What + TEXT(": carry a gun, the arrow on the rack's")), Collect && Collect->What == EMissionCollect::Weapons
				&& Collect->Count == Amount && Collect->Waypoint == EMissionWaypoint::Actor);
			break;
		}
		case ETutorialGoal::HitDummies:
		{
			const UMissionHitObjective* Hit = Cast<UMissionHitObjective>(Objective);
			TestTrue(*(What + TEXT(": the player's hits on dummies, counted, the arrow on the training ground")), Hit && Hit->Target.ActorClass == ATargetDummy::StaticClass()
				&& Hit->Count == Amount && Hit->bPlayerHitsOnly && Hit->bPassWithoutTargets && Hit->bShowCount && Hit->Waypoint == EMissionWaypoint::TargetsCenter);
			break;
		}
		case ETutorialGoal::KillCreatures:
		{
			const UMissionKillObjective* Kill = Cast<UMissionKillObjective>(Objective);
			TestTrue(*(What + TEXT(": the player's kills of creatures, counted, the arrow on the nearest spider")), Kill && Kill->Target.ActorClass == ACreatureBase::StaticClass()
				&& Kill->Count == Amount && Kill->bPlayerKillsOnly && Kill->bPassWithoutTargets && Kill->bShowCount && Kill->Waypoint == EMissionWaypoint::Actor);
			break;
		}
		case ETutorialGoal::OpenInventory:
		{
			const UMissionOpenPageObjective* Open = Cast<UMissionOpenPageObjective>(Objective);
			TestTrue(*(What + TEXT(": open the inventory, no arrow")), Open && Open->Page == EMissionPage::Any && Open->Waypoint == EMissionWaypoint::None);
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
	else if (TestEqual(TEXT("The asset has the tutorial's steps"), Asset->Steps.Num(), BuiltIn->Steps.Num()))
	{
		TestEqual(TEXT("and its title"), Asset->Title.ToString(), BuiltIn->Title.ToString());
		TestTrue(TEXT("and its kind and start"), Asset->Kind == BuiltIn->Kind && Asset->Start == BuiltIn->Start);
		for (int32 Index = 0; Index < BuiltIn->Steps.Num(); ++Index)
		{
			TestEqual(*FString::Printf(TEXT("The asset's step %d"), Index + 1), RuleOf(Asset->GetObjective(Index, 0)), RuleOf(BuiltIn->GetObjective(Index, 0)));
		}
	}

	// Played in a test level (the asset when there is one): a gun rack 20 m ahead, two dummies past it, no creatures.
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
	APlayerController* Shooter = World->SpawnActor<APlayerController>();
	AWeaponRack* Rack = World->SpawnActor<AWeaponRack>(FVector(2000.0, 0.0, 0.0), FRotator::ZeroRotator);
	ATargetDummy* Left = MissionTestWorld::SpawnDummy(World, FVector(3000.0, 1000.0, 0.0));
	ATargetDummy* Right = MissionTestWorld::SpawnDummy(World, FVector(3000.0, -1000.0, 0.0));
	if (!TestTrue(TEXT("Runner, display, stand-in, shooter, rack and dummies"), Runner && Display && Player && Shooter && Rack && Left && Right))
	{
		return false;
	}
	UMissionDefinition* Played = Asset ? Asset : BuiltIn;
	const FName TutorialId = Played->GetMissionId();
	Runner->OnMissionFinished.AddLambda([&Finished](const UMissionDefinition&, bool) { ++Finished; });
	Runner->BeginForTesting({ Played }, Campaign, Player, TEXT("Skyreach"));
	Runner->Update(0.f);
	TestFalse(TEXT("It waits for its director"), Runner->IsRunning(TutorialId));
	TestTrue(TEXT("The director starts it"), Runner->StartMission(TutorialId, 0, /*bForce*/ true) && Runner->GetStep(TutorialId) == 0);
	auto Waypoint = [Display]() { const FMission* Shown = Display->GetTracked(); return Shown ? Shown->Waypoint : TOptional<FVector>(); };
	auto IsAt = [&Waypoint](const FVector& Where) { const TOptional<FVector> Spot = Waypoint(); return Spot.IsSet() && Spot->Equals(Where, 1.0); };
	TestTrue(TEXT("Moving: the arrow points down the road to the gun rack"), IsAt(Rack->GetActorLocation()));

	Player->SetActorLocation(FVector(500.0, 0.0, 0.0));
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("5 m isn't far enough"), Runner->GetStep(TutorialId) == 0);
	Player->SetActorLocation(FVector(700.0, 0.0, 0.0));
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("7 m is: on to the village"), Runner->GetStep(TutorialId) == 1);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("13 m from the rack isn't there yet"), Runner->GetStep(TutorialId) == 1);
	Player->SetActorLocation(FVector(1200.0, 0.0, 300.0));
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("8 m from the rack is: take the rifle"), Runner->GetStep(TutorialId) == 2);
	TestTrue(TEXT("No rifle lying on it in a test level: the arrow points to the rack"), IsAt(Rack->GetActorLocation()));

	// The stand-in carries no guns; the rifle is taken as if it had been.
	Runner->CompleteStep(TutorialId);
	TestTrue(TEXT("The dummies next"), Runner->GetStep(TutorialId) == 3);
	TestTrue(TEXT("The arrow points to the middle of the training ground"), IsAt(FVector(3000.0, 0.0, 0.0)));
	for (int32 Shot = 0; Shot < 4; ++Shot)
	{
		MissionTestWorld::Hurt(Shot % 2 == 0 ? Left : Right, 10.f, Shooter);
	}
	MissionTestWorld::Hurt(Left, 10.f, nullptr);
	TestTrue(TEXT("Four of the player's hits (and one not theirs) aren't enough"), Runner->GetStep(TutorialId) == 3);
	MissionTestWorld::Hurt(Right, 10.f, Shooter);
	TestTrue(TEXT("Five are; with no creatures in the level the hunt passes at once: open the inventory"), Runner->GetStep(TutorialId) == 5);
	TestFalse(TEXT("Opening the inventory has no arrow"), Waypoint().IsSet());
	Runner->CompleteStep(TutorialId);
	TestFalse(TEXT("The last step finishes it"), Runner->IsRunning(TutorialId));
	TestEqual(TEXT("Finished once"), Finished, 1);
	TestTrue(TEXT("The campaign has it finished"), Campaign.HasCompleted(TutorialId));
	TestTrue(TEXT("It was never the campaign's main mission"), Campaign.ActiveMission.IsNone());
	return true;
}

#endif
