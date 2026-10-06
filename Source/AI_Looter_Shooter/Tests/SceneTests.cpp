#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/SceneTimeline.h"
#include "Scenes/SkiffRide.h"
#include "Scenes/TransitionScreen.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	constexpr float SceneFrame = 1.f / 30.f;

	/** Looter.Scenes set for a test, and put back after it. */
	class FScenesSetting
	{
	public:
		explicit FScenesSetting(int32 Wanted)
			: Variable(IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Scenes")))
		{
			if (Variable)
			{
				Previous = Variable->GetInt();
				Set(Wanted);
			}
		}

		~FScenesSetting()
		{
			Set(Previous);
		}

		void Set(int32 Wanted)
		{
			if (Variable)
			{
				Variable->Set(Wanted, ECVF_SetByConsole);
			}
		}

		bool IsFound() const { return Variable != nullptr; }

	private:
		IConsoleVariable* Variable = nullptr;
		int32 Previous = 1;
	};

	/** Moves Screen on for Seconds in frames of SceneFrame; how many times a held white timed out meanwhile. */
	int32 AdvanceScreen(FTransitionScreen& Screen, float Seconds)
	{
		int32 TimedOut = 0;
		for (float Done = 0.f; Done < Seconds - KINDA_SMALL_NUMBER; Done += SceneFrame)
		{
			TimedOut += Screen.Advance(FMath::Min(SceneFrame, Seconds - Done)) ? 1 : 0;
		}
		return TimedOut;
	}

	/** Ticks the level's scenes for Seconds in frames of SceneFrame, counting them on Clock. */
	void TickScenes(USceneSubsystem& Scenes, float Seconds, float& Clock)
	{
		for (float Done = 0.f; Done < Seconds - KINDA_SMALL_NUMBER; Done += SceneFrame)
		{
			const float Delta = FMath::Min(SceneFrame, Seconds - Done);
			Clock += Delta;
			Scenes.Tick(Delta);
		}
	}

	/** A skiff moored at Moored: the packet skiff's mesh when the project has it, else a bare hull with no deck socket. */
	AActor* SpawnSkiff(UWorld* World, const FTransform& Moored, UStaticMesh* Mesh)
	{
		AActor* Skiff = World->SpawnActor<AActor>();
		if (!Skiff)
		{
			return nullptr;
		}
		UStaticMeshComponent* Hull = NewObject<UStaticMeshComponent>(Skiff, TEXT("Hull"));
		Hull->SetMobility(EComponentMobility::Movable);
		Hull->SetStaticMesh(Mesh);
		Skiff->SetRootComponent(Hull);
		Hull->RegisterComponent();
		Skiff->SetActorTransform(Moored);
		return Skiff;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneTimelineTest, "Looter.Scenes.Timeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSceneTimelineTest::RunTest(const FString& Parameters)
{
	// A move from 0.25 s for a second, a cut at 2 s, and three moments: one at 0.5 s, two at 1 s (added in that order).
	FSceneTimeline Timeline;
	FString Heard;
	float MoveAlpha = -1.f;
	int32 MoveCalls = 0;
	float CutAlpha = -1.f;
	int32 CutCalls = 0;
	Timeline.AddMove(0.25f, 1.f, [&MoveAlpha, &MoveCalls](float Alpha) { MoveAlpha = Alpha; ++MoveCalls; });
	Timeline.AddMove(2.f, 0.f, [&CutAlpha, &CutCalls](float Alpha) { CutAlpha = Alpha; ++CutCalls; });
	Timeline.AddMoment(TEXT("Second"), 1.f, [&Heard]() { Heard += TEXT("(action) "); });
	Timeline.AddMoment(TEXT("Third"), 1.f);
	Timeline.AddMoment(TEXT("First"), 0.5f);
	Timeline.OnMoment = [&Heard](FName Moment) { Heard += Moment.ToString() + TEXT(" "); };
	TestNearlyEqual(TEXT("It lasts until its cut at 2 s"), Timeline.GetDuration(), 2.f, 0.001f);
	TestNearlyEqual(TEXT("A moment keeps its time"), Timeline.GetMomentTime(TEXT("Third")), 1.f, 0.001f);
	TestTrue(TEXT("A moment it doesn't have has no time"), Timeline.GetMomentTime(TEXT("Missing")) < 0.f);

	Timeline.Advance(0.2f);
	TestEqual(TEXT("Before 0.25 s the move hasn't begun"), MoveCalls, 0);
	TestTrue(TEXT("...and nothing has happened"), Heard.IsEmpty());

	Timeline.Advance(0.35f);
	TestNearlyEqual(TEXT("0.55 s in, the move is 30% through"), MoveAlpha, 0.3f, 0.001f);
	TestEqual(TEXT("First has happened"), Heard, FString(TEXT("First ")));

	// One long step, past the move's end and both moments at 1 s.
	Timeline.Advance(1.f);
	TestEqual(TEXT("The move ends exactly at its end"), MoveAlpha, 1.f);
	TestEqual(TEXT("The two at 1 s happen in the order they were added, each after its action"), Heard,
		FString(TEXT("First (action) Second Third ")));
	TestTrue(TEXT("Second and Third have happened"), Timeline.HasHappened(TEXT("Second")) && Timeline.HasHappened(TEXT("Third")));
	TestFalse(TEXT("It isn't finished before its cut"), Timeline.IsFinished());
	const int32 CallsAtEnd = MoveCalls;

	Timeline.Advance(0.3f);
	TestEqual(TEXT("A move at its end isn't applied again"), MoveCalls, CallsAtEnd);
	TestEqual(TEXT("The cut waits for 2 s"), CutCalls, 0);

	Timeline.Advance(0.5f);
	TestTrue(TEXT("The cut happens once, whole"), CutCalls == 1 && CutAlpha == 1.f);
	TestTrue(TEXT("Then it's finished"), Timeline.IsFinished());
	Timeline.Advance(5.f);
	Timeline.SkipToEnd();
	TestEqual(TEXT("Nothing happens twice, played on or skipped"), Heard, FString(TEXT("First (action) Second Third ")));
	TestEqual(TEXT("...not the cut either"), CutCalls, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneSkipTest, "Looter.Scenes.Skip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSceneSkipTest::RunTest(const FString& Parameters)
{
	// A timeline skipped halfway: every move at its end, every moment still to come in order, none that happened again.
	{
		FSceneTimeline Timeline;
		FString Heard;
		float Lift = 0.f;
		float Later = -1.f;
		Timeline.AddMove(0.f, 4.f, [&Lift](float Alpha) { Lift = Alpha; });
		Timeline.AddMove(5.f, 1.f, [&Later](float Alpha) { Later = Alpha; });
		Timeline.AddMoment(TEXT("Early"), 1.f);
		Timeline.AddMoment(TEXT("Late"), 3.f);
		Timeline.AddMoment(TEXT("End"), 6.f);
		Timeline.OnMoment = [&Heard](FName Moment) { Heard += Moment.ToString() + TEXT(" "); };
		Timeline.Advance(1.5f);
		Timeline.SkipToEnd();
		TestEqual(TEXT("A skip fires the moments still to come, in order, and none twice"), Heard, FString(TEXT("Early Late End ")));
		TestTrue(TEXT("...with every move at its end, even one that hadn't begun"), Lift == 1.f && Later == 1.f);
		TestTrue(TEXT("...and it's finished"), Timeline.IsFinished());
	}

	// The same through the level's scenes, which a mission is waiting on.
	FScenesSetting On(2);
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	USceneSubsystem* Scenes = World->GetSubsystem<USceneSubsystem>();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = MissionTestWorld::SpawnMarker(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("The level has scenes"), Scenes) || !TestNotNull(TEXT("...and a mission runner"), Runner) || !Player)
	{
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Watch = MissionTestWorld::NewMission(Scratch, TEXT("TestWatch"), EMissionKind::Side, EMissionStart::Automatic, TEXT("TestValley"));
	MissionTestWorld::AddObjective<UMissionSceneObjective>(Watch, 0)->Scene = TEXT("TestScene");
	Runner->BeginForTesting({ Watch }, Campaign, Player, TEXT("TestValley"));
	Runner->Update(0.f);
	TestTrue(TEXT("The mission waits for the scene"), Runner->IsRunning(TEXT("TestWatch")));

	FString Heard;
	const FDelegateHandle Listening = Scenes->OnSceneEvent.AddLambda([&Heard](FName Event) { Heard += Event.ToString() + TEXT(" "); });
	float Clock = 0.f;
	float Lift = 0.f;
	int32 AfterEnds = 0;
	FScenePlay Scene;
	Scene.Name = TEXT("TestScene");
	Scene.bLookOnly = false;
	Scene.Timeline.AddMove(0.f, 4.f, [&Lift](float Alpha) { Lift = Alpha; });
	Scene.Timeline.AddMoment(TEXT("Early"), 1.f);
	Scene.Timeline.AddMoment(TEXT("Late"), 3.f);
	Scene.Timeline.AddMoment(TEXT("End"), 4.f);
	Scene.AfterEnd = [&AfterEnds]() { ++AfterEnds; };
	TestTrue(TEXT("It plays"), Scenes->Play(MoveTemp(Scene)));
	TickScenes(*Scenes, 1.5f, Clock);
	TestEqual(TEXT("1.5 s in, Early has gone out"), Heard, FString(TEXT("Early ")));
	TestTrue(TEXT("...and it plays on"), Scenes->IsPlaying() && Scenes->GetPlayingName() == FName(TEXT("TestScene")));
	TestFalse(TEXT("The mission still waits"), Campaign.HasCompleted(TEXT("TestWatch")));

	Scenes->SkipScene();
	TestEqual(TEXT("Skipped: its end moments still go out, in order, none twice"), Heard, FString(TEXT("Early Late End ")));
	TestTrue(TEXT("...its moves at their ends"), Lift == 1.f);
	TestTrue(TEXT("...and it has ended, once"), !Scenes->IsPlaying() && AfterEnds == 1);
	Runner->Update(0.f);
	TestTrue(TEXT("For the missions a skipped scene has played"), Campaign.HasCompleted(TEXT("TestWatch")));

	// A skip asked from inside one of the scene's own moments happens once that moment is over.
	Heard.Reset();
	FScenePlay Asking;
	Asking.Name = TEXT("TestAsking");
	Asking.Timeline.AddMoment(TEXT("Ask"), 0.5f, [Scenes]() { Scenes->SkipScene(); });
	Asking.Timeline.AddMoment(TEXT("After"), 2.f);
	Asking.AfterEnd = [&AfterEnds]() { ++AfterEnds; };
	TestTrue(TEXT("The second scene plays"), Scenes->Play(MoveTemp(Asking)));
	TickScenes(*Scenes, 1.f, Clock);
	TestEqual(TEXT("Skipped from its own moment: the rest still go out, once"), Heard, FString(TEXT("Ask After ")));
	TestTrue(TEXT("...and it ended once"), !Scenes->IsPlaying() && AfterEnds == 2);
	Scenes->OnSceneEvent.Remove(Listening);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScenesOffInToursTest, "Looter.Scenes.OffInTours",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FScenesOffInToursTest::RunTest(const FString& Parameters)
{
	// What Tools/tour.ps1 and Tools/perf.ps1 put on the game's command line, and a plain game's.
	const TCHAR* TourRun = TEXT("\"C:/Dev/AI_Looter_Shooter/AI_Looter_Shooter.uproject\" /Game/Maps/Lvl_TutorialIsland -game -windowed -ResX=1920 ")
		TEXT("-ResY=1080 -nosplash -log=Tour.log -ExecCmds=\"Looter.Quality Medium,Looter.Tour Art/Levels/TutorialIsland/views.json quit\"");
	const TCHAR* PerfRun = TEXT("\"C:/Dev/AI_Looter_Shooter/AI_Looter_Shooter.uproject\" /Game/Maps/Lvl_TutorialIsland -game -windowed -ResX=1920 ")
		TEXT("-ResY=1080 -nosplash -csvCaptureFrames=1200 -ExitAfterCsvProfiling -log=Perf.log");
	const TCHAR* Asked = TEXT("\"C:/Dev/AI_Looter_Shooter/AI_Looter_Shooter.uproject\" -game -NoScenes");
	const TCHAR* Played = TEXT("\"C:/Dev/AI_Looter_Shooter/AI_Looter_Shooter.uproject\" -game -windowed");
	TestTrue(TEXT("A tour run turns scenes off"), USceneSubsystem::CommandLineTurnsScenesOff(TourRun));
	TestTrue(TEXT("A perf capture turns them off"), USceneSubsystem::CommandLineTurnsScenesOff(PerfRun));
	TestTrue(TEXT("-NoScenes turns them off"), USceneSubsystem::CommandLineTurnsScenesOff(Asked));
	TestFalse(TEXT("A game played leaves them on"), USceneSubsystem::CommandLineTurnsScenesOff(Played));
	TestFalse(TEXT("No command line leaves them on"), USceneSubsystem::CommandLineTurnsScenesOff(nullptr));

	FScenesSetting Setting(0);
	if (!TestTrue(TEXT("Looter.Scenes exists"), Setting.IsFound()))
	{
		return false;
	}
	TestFalse(TEXT("Looter.Scenes 0: off"), USceneSubsystem::AreScenesOn(nullptr));
	Setting.Set(2);
	TestTrue(TEXT("Looter.Scenes 2: on, even in a tour or perf run"), USceneSubsystem::AreScenesOn(nullptr));
	Setting.Set(1);
	TestTrue(TEXT("Looter.Scenes 1: as the command line says"),
		USceneSubsystem::AreScenesOn(nullptr) == !USceneSubsystem::CommandLineTurnsScenesOff(FCommandLine::Get()));

	// Off: nothing plays and the caller goes straight on (travels at once); no moment, no OnWhiteout.
	Setting.Set(0);
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	USceneSubsystem* Scenes = World->GetSubsystem<USceneSubsystem>();
	AActor* Skiff = SpawnSkiff(World, FTransform::Identity, nullptr);
	if (!TestNotNull(TEXT("The level has scenes"), Scenes) || !TestNotNull(TEXT("...and a skiff"), Skiff))
	{
		return false;
	}
	int32 Whiteouts = 0;
	int32 Moments = 0;
	Scenes->OnSceneEvent.AddLambda([&Moments](FName) { ++Moments; });
	TestFalse(TEXT("The ride doesn't play"), Scenes->PlaySkiffRide(Skiff, FOnSceneMoment::CreateLambda([&Whiteouts]() { ++Whiteouts; })));
	FScenePlay Scene;
	Scene.Name = TEXT("TestScene");
	Scene.Timeline.AddMoment(TEXT("Moment"), 0.f);
	TestFalse(TEXT("No scene plays"), Scenes->Play(MoveTemp(Scene)));
	float Clock = 0.f;
	TickScenes(*Scenes, 1.f, Clock);
	TestTrue(TEXT("Nothing is playing, nothing went out, and nothing waits on a white"), !Scenes->IsPlaying() && Moments == 0 && Whiteouts == 0);
	TestTrue(TEXT("The skiff stays moored"), Skiff->GetActorLocation().IsNearlyZero(0.1));
	Scenes->OnSceneEvent.Clear();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransitionScreenTest, "Looter.Scenes.TransitionScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTransitionScreenTest::RunTest(const FString& Parameters)
{
	FTransitionScreen Screen;
	TestFalse(TEXT("Clear to begin with"), Screen.IsShowing());

	// Held: full white at once, through a level load's one long frame and however long the arrival takes.
	Screen.Hold();
	TestTrue(TEXT("Held, fully white"), Screen.IsHeld() && Screen.GetWhite() == 1.f);
	TestFalse(TEXT("A level load's long frame doesn't time it out"), Screen.Advance(30.f));
	AdvanceScreen(Screen, 5.f);
	TestTrue(TEXT("Still held 5 s on"), Screen.IsHeld() && Screen.GetWhite() == 1.f);
	TestEqual(TEXT("No title while held"), Screen.GetTitleOpacity(), 0.f);

	// Revealed with the title: it rises through the white, then the white thins.
	Screen.Reveal(true);
	TestTrue(TEXT("Revealing begins fully white"), Screen.GetPhase() == ETransitionPhase::Revealing && Screen.GetWhite() == 1.f);
	AdvanceScreen(Screen, FTransitionScreen::TitleDelay + FTransitionScreen::TitleRiseSeconds);
	TestNearlyEqual(TEXT("The title has risen into place"), Screen.GetTitleRise(), 0.f, 0.02f);
	TestNearlyEqual(TEXT("...whole"), Screen.GetTitleOpacity(), 1.f, 0.02f);
	TestNearlyEqual(TEXT("...with its line drawing out"), Screen.GetTitleLine(), 0.875f, 0.05f);
	TestEqual(TEXT("...through a white still whole"), Screen.GetWhite(), 1.f);
	AdvanceScreen(Screen, FTransitionScreen::TitleHoldSeconds + 0.5f * FTransitionScreen::ThinSeconds);
	const float Thinning = Screen.GetWhite();
	TestTrue(TEXT("Then the white thins"), Thinning > 0.2f && Thinning < 0.8f);
	AdvanceScreen(Screen, FTransitionScreen::RevealSeconds(true));
	TestFalse(TEXT("...and is gone, title and all"), Screen.IsShowing());

	// Without a title it only thins, in a couple of seconds.
	Screen.Hold();
	Screen.Reveal(false);
	TestEqual(TEXT("No title"), Screen.GetTitleOpacity(), 0.f);
	TestTrue(TEXT("A couple of seconds"), FTransitionScreen::RevealSeconds(false) <= 3.f);
	AdvanceScreen(Screen, FTransitionScreen::RevealSeconds(false) + 0.1f);
	TestFalse(TEXT("Gone"), Screen.IsShowing());

	// Held with nothing to reveal it: it thins by itself after about 20 s, never white for good.
	Screen.Hold();
	const int32 Early = AdvanceScreen(Screen, FTransitionScreen::HoldTimeout - 0.5f);
	TestTrue(TEXT("Held 19.5 s"), Screen.IsHeld() && Early == 0);
	const int32 Late = AdvanceScreen(Screen, 1.f);
	TestEqual(TEXT("...then it times out, once"), Late, 1);
	TestTrue(TEXT("...and thins"), Screen.GetPhase() == ETransitionPhase::Revealing);
	AdvanceScreen(Screen, FTransitionScreen::RevealSeconds(false) + 0.1f);
	TestFalse(TEXT("...and is gone"), Screen.IsShowing());

	// A scene's own white, a fade up to a hold, and clearing.
	Screen.Drive(0.4f);
	TestTrue(TEXT("A scene's white is as set, and not held"), FMath::IsNearlyEqual(Screen.GetWhite(), 0.4f) && !Screen.IsHeld());
	Screen.Drive(0.f);
	TestFalse(TEXT("None at 0"), Screen.IsShowing());
	Screen.FadeIn(2.f);
	AdvanceScreen(Screen, 1.f);
	TestTrue(TEXT("Halfway up its fade"), Screen.GetWhite() > 0.2f && Screen.GetWhite() < 0.8f);
	AdvanceScreen(Screen, 1.1f);
	TestTrue(TEXT("Then held"), Screen.IsHeld());
	Screen.Clear();
	TestFalse(TEXT("Cleared at once"), Screen.IsShowing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSkiffRideTest, "Looter.Scenes.SkiffRide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSkiffRideTest::RunTest(const FString& Parameters)
{
	// The course as plain math: about 12 s to full white, the white over the last 2-3 s, still until the ropes slip,
	// easing out to a drift and never faster, a slow climb and a gentle turn to starboard.
	TestTrue(TEXT("Full white about 12 s in"), SkiffRide::FullWhiteTime >= 11.f && SkiffRide::FullWhiteTime <= 13.f);
	const float WhiteSeconds = SkiffRide::FullWhiteTime - SkiffRide::WhiteStart;
	TestTrue(TEXT("The white comes up over the last 2-3 s"), WhiteSeconds >= 2.f && WhiteSeconds <= 3.f);
	TestEqual(TEXT("No white before then"), SkiffRide::WhiteAt(SkiffRide::WhiteStart - 0.1f), 0.f);
	TestEqual(TEXT("Fully white at the end"), SkiffRide::WhiteAt(SkiffRide::FullWhiteTime), 1.f);
	// The skiff faces its +X, as every imported model does (Blender's -Y front); its gangplank, toward the jetty, is on its -Y.
	const FTransform Moored(FRotator(0.0, 30.0, 0.0), FVector(1000.0, -500.0, 2000.0));
	const FVector Bow = Moored.TransformVectorNoScale(FVector::ForwardVector);
	const FVector JettySide = Moored.TransformVectorNoScale(-FVector::RightVector);
	TestTrue(TEXT("Moored until the ropes slip"), SkiffRide::PoseAt(Moored, Bow, SkiffRide::CastOffTime).Equals(Moored, 0.01));
	float LastStep = 0.f;
	bool bEasesOut = true;
	for (float Seconds = SkiffRide::CastOffTime; Seconds < SkiffRide::FullWhiteTime; Seconds += 0.1f)
	{
		const float Covered = SkiffRide::DistanceAt(Seconds + 0.1f) - SkiffRide::DistanceAt(Seconds);
		bEasesOut = bEasesOut && Covered >= LastStep - 0.01f && Covered <= SkiffRide::CruiseSpeed * 0.1f + 0.01f;
		LastStep = Covered;
	}
	TestTrue(TEXT("It eases out of its moorings to a drift, never faster"), bEasesOut);
	const FTransform Ended = SkiffRide::PoseAt(Moored, Bow, SkiffRide::FullWhiteTime);
	const FVector Moved = Ended.GetLocation() - Moored.GetLocation();
	TestTrue(TEXT("40-80 m out along its bow by full white"), FVector::DotProduct(Moved, Bow) > 4000.0 && FVector::DotProduct(Moved, Bow) < 8000.0);
	TestTrue(TEXT("3-12 m higher"), Moved.Z > 300.0 && Moved.Z < 1200.0);
	const double Turned = FRotator::NormalizeAxis(Ended.Rotator().Yaw - Moored.Rotator().Yaw);
	TestTrue(TEXT("Turned gently to starboard"), Turned > 5.0 && Turned < 25.0);
	TestTrue(TEXT("...away from the jetty, on its port side"), FVector::DotProduct(Moved, JettySide) < 0.0);

	// The packet skiff's Deck socket, where the player stands: 1.2 m toward the bow from the middle, on the deck (Skiff.py's
	// (0, -1.2 m, 0.43 m) in Blender, which the importer turns to +X).
	UStaticMesh* Packet = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Vehicles/SM_Skiff_A_Packet.SM_Skiff_A_Packet"));
	if (Packet)
	{
		const UStaticMeshSocket* Deck = Packet->FindSocket(SkiffRide::DeckSocket());
		if (TestNotNull(TEXT("The packet skiff has its Deck socket"), Deck))
		{
			TestTrue(TEXT("...1.2 m toward the bow and 0.43 m up"), Deck->RelativeLocation.Equals(FVector(120.0, 0.0, 43.0), 5.0));
		}
	}

	// The ride in a test level (no player there to hold): the ropes slip, about 12 s to full white, OnWhiteout once.
	FScenesSetting On(2);
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	USceneSubsystem* Scenes = World->GetSubsystem<USceneSubsystem>();
	AActor* Skiff = SpawnSkiff(World, Moored, Packet);
	if (!TestNotNull(TEXT("The level has scenes"), Scenes) || !TestNotNull(TEXT("...and a skiff"), Skiff))
	{
		return false;
	}
	TestTrue(TEXT("The skiff's bow is its +X"), SkiffRide::BowOf(*Skiff).Equals(Bow, 0.001));
	float Clock = 0.f;
	float CastOffAt = -1.f;
	float WhiteoutAt = -1.f;
	int32 Whiteouts = 0;
	TArray<FName> Events;
	const FDelegateHandle Listening = Scenes->OnSceneEvent.AddLambda([&Events, &CastOffAt, &Clock](FName Event)
	{
		Events.Add(Event);
		if (Event == SkiffRide::CastOffMoment())
		{
			CastOffAt = Clock;
		}
	});
	auto CountWhiteout = [&Whiteouts, &WhiteoutAt, &Clock]()
	{
		return FOnSceneMoment::CreateLambda([&Whiteouts, &WhiteoutAt, &Clock]() { ++Whiteouts; WhiteoutAt = Clock; });
	};
	// What this test does wrong on purpose, and the ride's own warning when no trip follows it.
	AddExpectedMessagePlain(TEXT("has no skiff"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessagePlain(TEXT("can't play while"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AddExpectedMessagePlain(TEXT("no trip came"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	TestFalse(TEXT("No skiff, no ride"), Scenes->PlaySkiffRide(nullptr, CountWhiteout()));
	TestTrue(TEXT("The ride plays"), Scenes->PlaySkiffRide(Skiff, CountWhiteout()));
	TestTrue(TEXT("...as the scene playing"), Scenes->IsPlaying() && Scenes->GetPlayingName() == SkiffRide::SceneName());
	TestFalse(TEXT("One scene at a time"), Scenes->PlaySkiffRide(Skiff, CountWhiteout()));
	while (Scenes->IsPlaying() && Clock < 20.f)
	{
		TickScenes(*Scenes, SceneFrame, Clock);
	}
	TestEqual(TEXT("OnWhiteout fired once"), Whiteouts, 1);
	TestNearlyEqual(TEXT("...at full white, 12 s in"), WhiteoutAt, SkiffRide::FullWhiteTime, 0.05f);
	TestNearlyEqual(TEXT("The ropes slipped 0.75 s in"), CastOffAt, SkiffRide::CastOffTime, 0.05f);
	TestTrue(TEXT("CastOff, then Whiteout, each once"), Events.Num() == 2 && Events[0] == SkiffRide::CastOffMoment()
		&& Events[1] == SkiffRide::WhiteoutMoment());
	TestTrue(TEXT("The skiff is where the course ends"), Skiff->GetActorTransform().Equals(Ended, 1.0));

	// No trip came (a test level has none): the skiff goes back to its moorings by itself.
	TickScenes(*Scenes, USceneSubsystem::TravelGraceSeconds + 0.5f, Clock);
	TestTrue(TEXT("With no trip, the skiff is back at its moorings"), Skiff->GetActorLocation().Equals(Moored.GetLocation(), 0.1));

	// Skipped before the ropes slip: OnWhiteout at once, its moments still going out, the skiff at the course's end.
	Events.Reset();
	TestTrue(TEXT("The ride plays again"), Scenes->PlaySkiffRide(Skiff, CountWhiteout()));
	TickScenes(*Scenes, 0.3f, Clock);
	Scenes->SkipScene();
	TestEqual(TEXT("A skip fires OnWhiteout at once"), Whiteouts, 2);
	TestTrue(TEXT("...and still slips the ropes first"), Events.Num() == 2 && Events[0] == SkiffRide::CastOffMoment()
		&& Events[1] == SkiffRide::WhiteoutMoment());
	TestTrue(TEXT("...leaving the skiff where the ride ends"), Skiff->GetActorTransform().Equals(Ended, 1.0));
	TestFalse(TEXT("...and nothing is playing"), Scenes->IsPlaying());
	Scenes->ReturnFromScene();
	TestTrue(TEXT("Returned, the skiff is moored again"), Skiff->GetActorLocation().Equals(Moored.GetLocation(), 0.1));
	Scenes->OnSceneEvent.Remove(Listening);
	return true;
}

#endif
