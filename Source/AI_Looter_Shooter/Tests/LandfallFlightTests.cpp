#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/StationBoard.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Scenes/HardLanding.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionSubsystem.h"
#include "Story/NewGameIntro.h"
#include "Story/VaneBroadcast.h"
#include "Tests/MissionTestWorld.h"
#include "World/CompanyTender.h"
#include "World/LamplighterCourse.h"
#include "World/LamplighterRules.h"
#include "World/LamplighterShip.h"
#include "World/LandfallGrapple.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	constexpr float LandfallFrame = 1.f / 30.f;

	/** Looter.Scenes set for a test (2: on, whatever the command line says), and put back after it. */
	class FLandfallScenesOn
	{
	public:
		FLandfallScenesOn()
			: Variable(IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Scenes")))
		{
			if (Variable)
			{
				Previous = Variable->GetInt();
				Variable->Set(2, ECVF_SetByConsole);
			}
		}

		~FLandfallScenesOn()
		{
			if (Variable)
			{
				Variable->Set(Previous, ECVF_SetByConsole);
			}
		}

	private:
		IConsoleVariable* Variable = nullptr;
		int32 Previous = 1;
	};

	/** A course like WORLD's: from far out west, low over the plains, a turn into the canyon, the stop over open air. */
	TArray<FVector> TestCourse()
	{
		return { FVector(-35000.0, 0.0, 3000.0), FVector(-22000.0, 1500.0, 2200.0), FVector(-12000.0, 4000.0, 1500.0),
			FVector(-6000.0, 4000.0, 1200.0), FVector(-2000.0, 2500.0, 900.0) };
	}

	/** The ledge she lands on: low, ahead of the stop, turned a little. */
	FTransform TestMoored()
	{
		return FTransform(FRotator(0.0, 20.0, 0.0), FVector(4000.0, 3000.0, 0.0));
	}

	/** A Lamplighter in a test level with no crew and empty boarding waves (the fight's rules without DREDGER's men). */
	ALamplighterShip* SpawnTestShip(UWorld* World)
	{
		const FTransform Moored = TestMoored();
		ALamplighterShip* Ship = World->SpawnActorDeferred<ALamplighterShip>(ALamplighterShip::StaticClass(), Moored);
		if (!Ship)
		{
			return nullptr;
		}
		Ship->Course = TestCourse();
		Ship->GrappleStop = 3;
		Ship->Moored = Moored;
		Ship->bSpawnCrew = false;
		Ship->bSpawnBoarders = false;
		Ship->FinishSpawning(Moored);
		Ship->DispatchBeginPlay();
		return Ship;
	}

	/** Landfall's fight and landing steps, made in code: the deck clear, the two cuts (held), the hard landing. */
	UMissionDefinition* MakeFlightMission(UObject* Outer, FName Area)
	{
		UMissionDefinition* Mission = MissionTestWorld::NewMission(Outer, TEXT("TestLandfall"), EMissionKind::Main, EMissionStart::Automatic, Area);
		MissionTestWorld::AddObjective<UMissionEventObjective>(Mission, 0)->Event = LandfallRules::DeckClearEvent();
		UMissionInteractObjective* Cut = MissionTestWorld::AddObjective<UMissionInteractObjective>(Mission, 1);
		Cut->Target.ActorTag = LandfallRules::GrappleTag();
		Cut->Count = 2;
		Cut->bHold = true;
		MissionTestWorld::AddObjective<UMissionSceneObjective>(Mission, 2)->Scene = HardLanding::SceneName();
		return Mission;
	}

	void AdvanceShip(ALamplighterShip& Ship, float Seconds)
	{
		for (float Done = 0.f; Done < Seconds - KINDA_SMALL_NUMBER; Done += LandfallFrame)
		{
			Ship.Advance(FMath::Min(LandfallFrame, Seconds - Done));
		}
	}

	/** The ship and the level's scenes on together, frame by frame (the scene moves her while it plays). */
	void AdvanceWithScenes(ALamplighterShip& Ship, USceneSubsystem& Scenes, float Seconds)
	{
		for (float Done = 0.f; Done < Seconds - KINDA_SMALL_NUMBER; Done += LandfallFrame)
		{
			const float Step = FMath::Min(LandfallFrame, Seconds - Done);
			Ship.Advance(Step);
			Scenes.Tick(Step);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandfallFlightCourseTest, "Looter.Story.Landfall.Flight.Course",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLandfallFlightCourseTest::RunTest(const FString& Parameters)
{
	// Her course: through every point from the first to the last, heading along it; she cruises, slows into the creep at the
	// grapple stop, creeps at most CreepLimit past it and holds; she banks and pitches only gently, and her calm pose is level.
	FLamplighterCourse Course;
	TestFalse(TEXT("One point is no course"), Course.Build({ FVector::ZeroVector }));
	const TArray<FVector> Points = TestCourse();
	if (!TestTrue(TEXT("The course builds"), Course.Build(Points)))
	{
		return false;
	}
	TestTrue(TEXT("It starts on its first point"), Course.LocationAt(0.f).Equals(Points[0], 1.0));
	TestTrue(TEXT("...and ends on its last"), Course.LocationAt(Course.GetLength()).Equals(Points.Last(), 1.0));
	TestTrue(TEXT("Its points lie along it in order"), Course.DistanceAtPoint(1) > 0.f && Course.DistanceAtPoint(3) > Course.DistanceAtPoint(1)
		&& Course.DistanceAtPoint(4) <= Course.GetLength() + 1.f);
	TestTrue(TEXT("It heads east at its start"), Course.HeadingAt(0.f).X > 0.9);

	const float Stop = Course.DistanceAtPoint(3);
	TestEqual(TEXT("Far from the stop: the cruise"), Course.TargetSpeed(0.f, Stop), FLamplighterCourse::CruiseSpeed);
	TestNearlyEqual(TEXT("At the stop: the creep"), Course.TargetSpeed(Stop, Stop), FLamplighterCourse::CreepSpeed, 1.f);
	TestEqual(TEXT("Past the creep's end: held"), Course.TargetSpeed(Stop + FLamplighterCourse::CreepLimit + 10.f, Stop), 0.f);
	TestNearlyEqual(TEXT("Speed changes no faster than her pull"), FLamplighterCourse::SpeedToward(0.f, 900.f, 0.5f),
		FLamplighterCourse::Acceleration * 0.5f, 0.01f);

	// Sailed by the clock: she gets to the stop, slows, never passes the creep's end, never speeds past the cruise.
	float Distance = 0.f;
	float Speed = FLamplighterCourse::CruiseSpeed;
	float Fastest = 0.f;
	float ReachedStopAt = -1.f;
	bool bGentle = true;
	for (float Clock = 0.f; Clock < 240.f; Clock += LandfallFrame)
	{
		Speed = FLamplighterCourse::SpeedToward(Speed, Course.TargetSpeed(Distance, Stop), LandfallFrame);
		Distance = FMath::Min(Distance + Speed * LandfallFrame, Course.HoldDistance(Stop));
		Fastest = FMath::Max(Fastest, Speed);
		if (ReachedStopAt < 0.f && Distance >= Stop)
		{
			ReachedStopAt = Clock;
			TestTrue(TEXT("She comes to the stop at the creep"), Speed <= FLamplighterCourse::CreepSpeed + 25.f);
		}
		const FRotator Pose = Course.PoseAt(Distance, Speed, Clock, 1.f).Rotator();
		bGentle = bGentle && FMath::Abs(Pose.Roll) <= FLamplighterCourse::MaxBankDegrees + FLamplighterCourse::RollDegrees + 0.1
			&& FMath::Abs(Pose.Pitch) <= FLamplighterCourse::MaxPitchDegrees + FLamplighterCourse::PitchDegrees + 0.1;
	}
	TestTrue(TEXT("She reaches the grapple stop"), ReachedStopAt > 0.f);
	TestTrue(TEXT("...and holds within the creep's reach past it"), Distance <= Stop + FLamplighterCourse::CreepLimit + 1.f && Speed < 1.f);
	TestTrue(TEXT("Never faster than the cruise"), Fastest <= FLamplighterCourse::CruiseSpeed + 0.01f);
	TestTrue(TEXT("Her bank and pitch stay gentle"), bGentle);
	const FRotator Calm = Course.CalmPoseAt(Stop).Rotator();
	TestTrue(TEXT("Her calm pose is level (what the tender keeps station on)"), FMath::IsNearlyZero(Calm.Roll, 0.01) && FMath::IsNearlyZero(Calm.Pitch, 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandfallFlightOldSaveTest, "Looter.Story.Landfall.Flight.OldSave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLandfallFlightOldSaveTest::RunTest(const FString& Parameters)
{
	const FName Main = LandfallRules::MissionId();
	// A new story flies, and passes the old opening for good.
	FCampaignRecord New;
	TestTrue(TEXT("A new story flies"), LandfallRules::StartFor(New) == ELandfallStart::Flight);
	LandfallRules::ApplyStart(New, ELandfallStart::Flight);
	TestTrue(TEXT("...the cold open, the grave and the Rest's intro behind it"), New.bColdOpenSeen && New.HasFlag(NewGameIntro::SeenFlag())
		&& New.HasFlag(LandfallRules::BegunFlag()));

	// A session saved mid-flight flies again from the start.
	FCampaignRecord MidAir = New;
	MidAir.ActiveMission = Main;
	MidAir.ActiveMissionStep = LandfallRules::StepGrapples;
	TestTrue(TEXT("Saved mid-flight: it flies again"), LandfallRules::StartFor(MidAir) == ELandfallStart::Flight);
	LandfallRules::ApplyStart(MidAir, ELandfallStart::Flight);
	TestEqual(TEXT("...Main 1 back at its first step"), MidAir.ActiveMissionStep, LandfallRules::StepProw);

	// Down already: moored, the anchor down only once it's been wound or Main 1 is past it.
	FCampaignRecord Down = New;
	Down.ActiveMission = Main;
	Down.ActiveMissionStep = LandfallRules::StepAnchor;
	TestTrue(TEXT("Saved on the ledge: moored"), LandfallRules::StartFor(Down) == ELandfallStart::Moored);
	TestFalse(TEXT("...the anchor not down yet"), LandfallRules::IsAnchoredIn(Down));
	Down.ActiveMissionStep = LandfallRules::StepPoint;
	TestTrue(TEXT("Past the anchor: it's down"), LandfallRules::IsAnchoredIn(Down));

	// A save from before Landfall, Main 1 running at an old step: moored, anchored, Main 1 at the climb to the Point.
	FCampaignRecord Old;
	Old.bFirstCastOff = true;
	Old.bColdOpenSeen = true;
	Old.ActiveMission = Main;
	Old.ActiveMissionStep = 2;
	Old.MarkOptionalDone(Main, 7, 0);
	TestTrue(TEXT("An old save is moored"), LandfallRules::StartFor(Old) == ELandfallStart::MooredOldSave);
	LandfallRules::ApplyStart(Old, ELandfallStart::MooredOldSave);
	TestEqual(TEXT("...Main 1 moved to the Point"), Old.ActiveMissionStep, LandfallRules::OldSaveStep);
	TestTrue(TEXT("...anchored and begun, its old optionals forgotten"), LandfallRules::IsAnchoredIn(Old) && Old.HasFlag(LandfallRules::BegunFlag())
		&& Old.CountOptionalDone(Main) == 0);
	TestTrue(TEXT("...and it stays moored on the next load"), LandfallRules::StartFor(Old) == ELandfallStart::Moored);

	// Main 1 done: moored, anchored.
	FCampaignRecord Done;
	Done.CompletedMissions.Add(Main);
	TestTrue(TEXT("Main 1 done: moored and anchored"), LandfallRules::StartFor(Done) == ELandfallStart::Moored && LandfallRules::IsAnchoredIn(Done));

	// The story's first arrival is her deck.
	TestEqual(TEXT("The first arrival is the Lamplighter"), StationBoard::FirstArrivalLanding(), LandfallRules::LandingTag());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandfallFlightLandingPathTest, "Looter.Story.Landfall.Flight.LandingPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLandfallFlightLandingPathTest::RunTest(const FString& Parameters)
{
	// The fall from where she broke free to the ledge: from there, faster and faster down, listing to her torn side, never
	// jumping a frame, exactly at Moored at the end.
	const FTransform From(FRotator(0.0, 5.0, 0.0), FVector(-6000.0, 4000.0, 1200.0));
	const FTransform Moored = TestMoored();
	TestTrue(TEXT("It starts where she broke free"), HardLanding::PoseAt(From, Moored, 0.f, 1.f).Equals(From, 0.5));
	TestTrue(TEXT("It ends at Moored"), HardLanding::PoseAt(From, Moored, HardLanding::TotalSeconds, 1.f).Equals(Moored, 0.01));
	TestTrue(TEXT("The scene takes up where her breaking leaves off"), HardLanding::SceneSeconds > 4.f && HardLanding::SceneSeconds < 7.f);
	TestTrue(TEXT("Slow at first, all the way down at the hit"), HardLanding::DropAt(1.f) < 0.05f && FMath::IsNearlyEqual(HardLanding::DropAt(HardLanding::TouchAt), 1.f));
	TestTrue(TEXT("Across with the skid's end"), FMath::IsNearlyEqual(HardLanding::GlideAt(HardLanding::TouchAt + HardLanding::SkidSeconds), 1.f));
	FTransform Before = From;
	double Biggest = 0.0;
	bool bLists = false;
	for (float Seconds = LandfallFrame; Seconds <= HardLanding::TotalSeconds + LandfallFrame; Seconds += LandfallFrame)
	{
		const FTransform Pose = HardLanding::PoseAt(From, Moored, Seconds, 1.f);
		Biggest = FMath::Max(Biggest, FVector::Distance(Pose.GetLocation(), Before.GetLocation()));
		// Listing to starboard (+Y) dips it: her right vector points down.
		bLists = bLists || (Seconds < HardLanding::TouchAt && Pose.GetRotation().GetRightVector().Z < -0.1);
		Before = Pose;
	}
	TestTrue(TEXT("No frame jumps more than a hard fall's step (under 2.5 m at 30 fps)"), Biggest < 250.0);
	TestTrue(TEXT("She lists to her torn side as she falls"), bLists);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandfallFlightSequenceTest, "Looter.Story.Landfall.Flight.Sequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLandfallFlightSequenceTest::RunTest(const FString& Parameters)
{
	// In a test level: she sails her course, the grapples pin her dead still, the waves come and the deck clears (the
	// mission hears Landfall.DeckClear), the lines can't be cut before that, two cuts break her free, the hard landing plays
	// and leaves her exactly at Moored, the missions hearing Scene.HardLanding.
	FLandfallScenesOn On;
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	USceneSubsystem* Scenes = World->GetSubsystem<USceneSubsystem>();
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	ALamplighterShip* Ship = SpawnTestShip(World);
	if (!TestNotNull(TEXT("Missions"), Runner) || !TestNotNull(TEXT("Scenes"), Scenes) || !Player || !TestNotNull(TEXT("The ship"), Ship))
	{
		return false;
	}
	const FName Area(TEXT("TestRest"));
	UMissionDefinition* Mission = MakeFlightMission(CreatePackage(nullptr), Area);
	Runner->BeginForTesting({ Mission }, Campaign, Player, Area);
	Runner->Update(0.f);

	TestTrue(TEXT("Before the level begins her: moored where she was placed"), Ship->GetActorTransform().Equals(TestMoored(), 1.0));
	TestTrue(TEXT("A new story: the flight begins"), Ship->BeginLandfall() && Ship->GetFlight() == EShipFlight::Sailing);
	TestTrue(TEXT("...the old opening passed in the record"), Campaign.bColdOpenSeen && Campaign.HasFlag(LandfallRules::BegunFlag()));
	TestTrue(TEXT("...at her course's start"), Ship->GetActorLocation().Equals(TestCourse()[0], 30.0));
	bool bProwTagged = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		bProwTagged |= It->ActorHasTag(LandfallRules::ProwTag()) && It->GetAttachParentActor() == Ship;
	}
	TestTrue(TEXT("Her prow's marker rides with her"), bProwTagged);

	AdvanceShip(*Ship, 5.f);
	TestTrue(TEXT("Five seconds on she's sailed her course"), Ship->GetDistance() > 3000.f && Ship->GetFlight() == EShipFlight::Sailing);

	Ship->GrappleNow();
	TestTrue(TEXT("Grappled: pinned"), Ship->GetFlight() == EShipFlight::Grappled && Ship->CountGrapples(true) == 2);
	TestNotNull(TEXT("...the tender alongside"), Ship->GetTender());
	AdvanceShip(*Ship, 2.f);
	const FTransform Still = Ship->GetActorTransform();
	AdvanceShip(*Ship, 1.f);
	TestTrue(TEXT("After the lurch she's dead still"), Ship->GetActorTransform().Equals(Still, 0.001));
	TArray<ALandfallGrapple*> Lines;
	for (TActorIterator<ALandfallGrapple> It(World); It; ++It)
	{
		Lines.Add(*It);
	}
	if (!TestEqual(TEXT("Two grapples, tagged for the mission"), Lines.Num(), 2) || !Lines[0]->ActorHasTag(LandfallRules::GrappleTag()))
	{
		return false;
	}
	TestFalse(TEXT("Not cut while the deck's held"), Ship->CanCutGrapples() || Lines[0]->Cut());

	// The waves (empty here) come and go: the deck clears.
	AdvanceShip(*Ship, 6.f);
	TestTrue(TEXT("The deck is clear"), Ship->IsDeckClear() && Ship->CanCutGrapples());
	Runner->Update(0.f);
	TestEqual(TEXT("...and the mission heard it"), Runner->GetStep(TEXT("TestLandfall")), 1);

	for (ALandfallGrapple* Line : Lines)
	{
		TestTrue(TEXT("A line cut"), Line->Cut());
		// The player's interaction component tells the missions of a held use; here the test does.
		Runner->NotifyEvent(FMissionEvent::Interaction(Line, /*bHeld*/ true));
	}
	TestTrue(TEXT("Both cut: she breaks free"), Ship->GetFlight() == EShipFlight::Breaking);
	TestTrue(TEXT("...the tender falling away"), Ship->GetTender() && Ship->GetTender()->IsFalling());
	Runner->Update(0.f);
	TestEqual(TEXT("...the mission on to the landing"), Runner->GetStep(TEXT("TestLandfall")), 2);

	AdvanceShip(*Ship, HardLanding::BreakSeconds - 0.5f);
	const FTransform OnPath = HardLanding::PoseAt(Ship->GetBreakFrom(), TestMoored(), HardLanding::BreakSeconds - 0.5f, /*TornSide*/ 1.f);
	TestTrue(TEXT("Breaking free she follows the landing's path"), Ship->GetFlight() == EShipFlight::Breaking
		&& Ship->GetActorLocation().Equals(OnPath.GetLocation(), 3.0));
	AdvanceShip(*Ship, 0.6f);
	TestTrue(TEXT("Then the hard landing plays"), Ship->GetFlight() == EShipFlight::Landing && Scenes->GetPlayingName() == HardLanding::SceneName());
	AdvanceWithScenes(*Ship, *Scenes, HardLanding::SceneSeconds + 0.5f);
	TestFalse(TEXT("It's over"), Scenes->IsPlaying());
	TestTrue(TEXT("She lies moored, exactly at Moored"), Ship->GetFlight() == EShipFlight::Moored && Ship->GetActorTransform().Equals(TestMoored(), 0.5));
	TestTrue(TEXT("...the scene played for the missions"), Scenes->HasPlayed(HardLanding::SceneName()));
	Runner->Update(0.f);
	TestTrue(TEXT("...and the mission's through its steps"), Campaign.HasCompleted(TEXT("TestLandfall")) || Runner->IsReadyToTurnIn(TEXT("TestLandfall")));
	TestFalse(TEXT("Not anchored yet"), Ship->IsAnchored());
	Ship->SetAnchored(false);
	TestTrue(TEXT("Anchored: kept in the story"), Ship->IsAnchored() && Campaign.HasFlag(LandfallRules::AnchoredFlag()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandfallFlightSkipTest, "Looter.Story.Landfall.Flight.Skip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLandfallFlightSkipTest::RunTest(const FString& Parameters)
{
	// The hard landing skipped: she lies at Moored all the same, and it counts as played. Vane's broadcast cut off with it.
	FLandfallScenesOn On;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	USceneSubsystem* Scenes = World->GetSubsystem<USceneSubsystem>();
	ALamplighterShip* Ship = SpawnTestShip(World);
	if (!TestNotNull(TEXT("Scenes"), Scenes) || !TestNotNull(TEXT("The ship"), Ship))
	{
		return false;
	}
	Ship->SetFlight(EShipFlight::Landing);
	TestTrue(TEXT("The jump to the landing plays it"), Scenes->GetPlayingName() == HardLanding::SceneName());
	AdvanceWithScenes(*Ship, *Scenes, 1.f);
	Scenes->SkipScene();
	TestTrue(TEXT("Skipped: moored at Moored"), !Scenes->IsPlaying() && Ship->GetFlight() == EShipFlight::Moored
		&& Ship->GetActorTransform().Equals(TestMoored(), 0.5));
	TestTrue(TEXT("...and played, for the missions"), Scenes->HasPlayed(HardLanding::SceneName()));

	UVaneBroadcast* Radio = UVaneBroadcast::Get(World);
	if (TestNotNull(TEXT("Vane's radio"), Radio))
	{
		UVaneBroadcast::Say(Ship, { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("A test of the company's horn."))) });
		TestTrue(TEXT("On the air"), UVaneBroadcast::IsSpeaking(Ship));
		UVaneBroadcast::Stop(Ship);
		TestFalse(TEXT("Cut off"), UVaneBroadcast::IsSpeaking(Ship));
		UVaneBroadcast::Say(Ship, { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("Short."))) });
		for (int32 Frame = 0; Frame < 30 * 20 && Radio->IsOnAir(); ++Frame)
		{
			Radio->Advance(LandfallFrame);
			if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World))
			{
				Captions->Update(LandfallFrame);
			}
		}
		TestFalse(TEXT("A broadcast ends by itself once he's said it"), Radio->IsOnAir());
	}
	return true;
}

#endif
