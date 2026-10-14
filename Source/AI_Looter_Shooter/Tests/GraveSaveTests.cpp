#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/SceneTimeline.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/MissionTestWorld.h"
#include "World/GraveMarkRules.h"
#include "World/GraveTravelRules.h"
#include "World/GraveTravelSubsystem.h"
#include "World/RespawnMarker.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Respawn graves as save points (the user's call, 2026-10-13): walking up to a grave marks it, the last one marked in an
// area is where Continue wakes. The rules alone, a walk up to graves in a test level, and Continue's placement.

using namespace MissionTestWorld;

namespace
{
	const FName Valley(TEXT("TestValley"));

	/** A grave in a test level, named, with the way it opens (from the start, with a mission) and its walk-up gate. */
	ARespawnMarker* PlaceSaveGrave(UWorld* World, FName Id, const FVector& Where, double Yaw, bool bOpenFromStart, FName OpenedBy = NAME_None,
		FName MarkedAfter = NAME_None)
	{
		ARespawnMarker* Grave = World->SpawnActor<ARespawnMarker>(Where, FRotator(0.0, Yaw, 0.0));
		if (Grave)
		{
			Grave->MarkerId = Id;
			Grave->bStartActive = bOpenFromStart;
			Grave->ActiveAfterMission = OpenedBy;
			Grave->MarkAfterMission = MarkedAfter;
		}
		return Grave;
	}

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}

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
				Variable->Set(Wanted, ECVF_SetByConsole);
			}
		}

		~FScenesSetting()
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraveSaveRulesTest, "Looter.Grave.Save.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGraveSaveRulesTest::RunTest(const FString& Parameters)
{
	// When a walk up to a grave marks it, in the order the rules ask; what a mark does; how the record keeps it, per area.
	using namespace GraveMarkRules;
	FGraveMarkSenses Near;
	Near.bHasPlayer = true;
	Near.Distance = 300.0;
	Near.Radius = 600.0;
	auto With = [&Near](TFunctionRef<void(FGraveMarkSenses&)> Change)
	{
		FGraveMarkSenses Senses = Near;
		Change(Senses);
		return Check(Senses);
	};

	TestTrue(TEXT("Nobody there: nothing to mark it"), Check(FGraveMarkSenses()) == EGraveMarkBlock::NoPlayer);
	TestTrue(TEXT("A living player in the radius with a clear line: it marks"), Check(Near) == EGraveMarkBlock::None);
	TestTrue(TEXT("Not marking on approach: off"), With([](FGraveMarkSenses& S) { S.bMarkOnApproach = false; }) == EGraveMarkBlock::Off);
	TestTrue(TEXT("A trip's landing: never"), With([](FGraveMarkSenses& S) { S.bLanding = true; }) == EGraveMarkBlock::Landing);
	TestTrue(TEXT("Down: no"), With([](FGraveMarkSenses& S) { S.bDying = true; }) == EGraveMarkBlock::Dying);
	TestTrue(TEXT("In a scene: no"), With([](FGraveMarkSenses& S) { S.bInScene = true; }) == EGraveMarkBlock::Scene);
	TestTrue(TEXT("A trip or fast travel under way: no"), With([](FGraveMarkSenses& S) { S.bTravelling = true; }) == EGraveMarkBlock::Travelling);
	TestTrue(TEXT("Its mission not finished: not yet"), With([](FGraveMarkSenses& S) { S.bStoryAllows = false; }) == EGraveMarkBlock::NotYet);
	TestTrue(TEXT("Just inside the radius: yes"), With([](FGraveMarkSenses& S) { S.Distance = 600.0; }) == EGraveMarkBlock::None);
	TestTrue(TEXT("Just outside it: too far"), With([](FGraveMarkSenses& S) { S.Distance = 600.5; }) == EGraveMarkBlock::TooFar);
	TestTrue(TEXT("A wall in the way: no line"), With([](FGraveMarkSenses& S) { S.bClearLine = false; }) == EGraveMarkBlock::NoSight);
	TestTrue(TEXT("A dead player outranks a scene"), With([](FGraveMarkSenses& S) { S.bDying = true; S.bInScene = true; }) == EGraveMarkBlock::Dying);
	TestTrue(TEXT("The story's wait is named before the distance"), With([](FGraveMarkSenses& S) { S.bStoryAllows = false; S.Distance = 5000.0; })
		== EGraveMarkBlock::NotYet);
	TestTrue(TEXT("The distance before the line"), With([](FGraveMarkSenses& S) { S.Distance = 5000.0; S.bClearLine = false; }) == EGraveMarkBlock::TooFar);
	TestTrue(TEXT("No words when it marks"), Describe(EGraveMarkBlock::None).IsEmpty());
	for (const EGraveMarkBlock Block : { EGraveMarkBlock::Off, EGraveMarkBlock::Landing, EGraveMarkBlock::NoPlayer, EGraveMarkBlock::Dying, EGraveMarkBlock::Scene,
		EGraveMarkBlock::Travelling, EGraveMarkBlock::NotYet, EGraveMarkBlock::TooFar, EGraveMarkBlock::NoSight })
	{
		TestFalse(TEXT("A word for every block"), Describe(Block).IsEmpty());
	}

	// What a walk up to a grave does.
	TestTrue(TEXT("Never marked: a first mark"), Outcome(false, false) == EGraveMark::First);
	TestTrue(TEXT("Never marked, though already the save point (the console set it): still its first"), Outcome(false, true) == EGraveMark::First);
	TestTrue(TEXT("Marked before and another is the save point: the save point moves back"), Outcome(true, false) == EGraveMark::Again);
	TestTrue(TEXT("The save point already: nothing"), Outcome(true, true) == EGraveMark::None);

	// The record: the last marked per area, and every grave ever marked.
	const FName Other(TEXT("TestElsewhere"));
	FCampaignRecord Campaign;
	TestFalse(TEXT("A new story has marked nothing"), IsMarked(Campaign, TEXT("FamilyPlot")) || IsSavePoint(Campaign, Valley, TEXT("FamilyPlot")));
	TestTrue(TEXT("The family plot: a first mark"), Record(Campaign, Valley, TEXT("FamilyPlot")) == EGraveMark::First);
	TestTrue(TEXT("...marked, and the valley's save point"), IsMarked(Campaign, TEXT("FamilyPlot")) && IsSavePoint(Campaign, Valley, TEXT("FamilyPlot"))
		&& Campaign.GetLastGrave(Valley) == FName(TEXT("FamilyPlot")));
	TestTrue(TEXT("Marking it again: nothing"), Record(Campaign, Valley, TEXT("FamilyPlot")) == EGraveMark::None);
	TestTrue(TEXT("The chapel yard: a first mark"), Record(Campaign, Valley, TEXT("ChapelYard")) == EGraveMark::First);
	TestTrue(TEXT("...the save point moves to it, the family plot stays marked"), Campaign.GetLastGrave(Valley) == FName(TEXT("ChapelYard"))
		&& IsMarked(Campaign, TEXT("FamilyPlot")) && !IsSavePoint(Campaign, Valley, TEXT("FamilyPlot")));
	TestTrue(TEXT("Back at the family plot: marked again, not a first"), Record(Campaign, Valley, TEXT("FamilyPlot")) == EGraveMark::Again
		&& Campaign.GetLastGrave(Valley) == FName(TEXT("FamilyPlot")));
	TestTrue(TEXT("Another area has its own save point"), Record(Campaign, Other, TEXT("Lily")) == EGraveMark::First
		&& Campaign.GetLastGrave(Other) == FName(TEXT("Lily")) && Campaign.GetLastGrave(Valley) == FName(TEXT("FamilyPlot")));
	TestTrue(TEXT("No name: nothing marked"), Record(Campaign, Valley, NAME_None) == EGraveMark::None && Campaign.GetLastGrave(Valley) == FName(TEXT("FamilyPlot")));
	TestTrue(TEXT("Marked graves are a list of their own, apart from the save points"), Campaign.MarkedGraves.Num() == 3 && Campaign.LastGraves.Num() == 2);

	// Through the session's save and back.
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->Campaign = Campaign;
	const ULooterSessionSave* Loaded = WriteAndRead(Save);
	if (TestNotNull(TEXT("Saved and read"), Loaded))
	{
		const FCampaignRecord& Read = Loaded->Campaign;
		TestTrue(TEXT("Each area's save point is kept"), Read.GetLastGrave(Valley) == FName(TEXT("FamilyPlot")) && Read.GetLastGrave(Other) == FName(TEXT("Lily")));
		TestTrue(TEXT("...and which graves were marked"), IsMarked(Read, TEXT("FamilyPlot")) && IsMarked(Read, TEXT("ChapelYard")) && IsMarked(Read, TEXT("Lily")));
		TestFalse(TEXT("...and not one that wasn't"), IsMarked(Read, TEXT("BootHill")));
	}
	TestTrue(TEXT("A save from before save points reads as nothing marked"), GetDefault<ULooterSessionSave>()->Campaign.LastGraves.IsEmpty()
		&& GetDefault<ULooterSessionSave>()->Campaign.MarkedGraves.IsEmpty());

	// Taking a mark back (the console): the grave is plain again, and no longer its area's save point.
	TestTrue(TEXT("Forgotten"), Forget(Campaign, Valley, TEXT("FamilyPlot")));
	TestTrue(TEXT("...no longer marked, nor the save point"), !IsMarked(Campaign, TEXT("FamilyPlot")) && Campaign.GetLastGrave(Valley).IsNone());
	TestTrue(TEXT("...the others as they were"), IsMarked(Campaign, TEXT("ChapelYard")) && Campaign.GetLastGrave(Other) == FName(TEXT("Lily")));
	TestFalse(TEXT("Forgetting it twice does nothing"), Forget(Campaign, Valley, TEXT("FamilyPlot")));

	// The flare: nothing before it, up to its peak by the rise, then easing down to nothing at its end.
	TestTrue(TEXT("A first mark's flare outlasts a lifting flame"), FlareSeconds(EGraveMark::First) > FlareSeconds(EGraveMark::Again) && FlareSeconds(EGraveMark::None) == 0.f);
	for (const EGraveMark Kind : { EGraveMark::First, EGraveMark::Again })
	{
		const float Length = FlareSeconds(Kind);
		TestTrue(TEXT("No flare before or after"), FlarePulse(-0.1f, Kind) == 0.f && FlarePulse(0.f, Kind) == 0.f && FlarePulse(Length, Kind) == 0.f);
		TestTrue(TEXT("At its peak by the rise"), FMath::IsNearlyEqual(FlarePulse(FlareRiseSeconds, Kind), 1.f, 0.001f));
		float Last = 1.f;
		bool bEasing = true;
		for (float Age = FlareRiseSeconds; Age < Length; Age += 0.02f)
		{
			const float Pulse = FlarePulse(Age, Kind);
			bEasing = bEasing && Pulse <= Last + 0.0001f && Pulse >= 0.f;
			Last = Pulse;
		}
		TestTrue(TEXT("...then only eases down"), bEasing);
	}
	TestTrue(TEXT("Over about a second for a first mark"), FlareSeconds(EGraveMark::First) > 0.9f && FlareSeconds(EGraveMark::First) < 1.3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraveSaveWalkUpTest, "Looter.Grave.Save.WalkUp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGraveSaveWalkUpTest::RunTest(const FString& Parameters)
{
	// A walk up to graves in a test level: the radius, the story's gate, a first mark that opens a grave, the save point moving
	// back, a player who is down or in a scene or a grave that is a trip's landing, and the graves the story opens on their own.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UGraveTravelSubsystem* Travel = World->GetSubsystem<UGraveTravelSubsystem>();
	AActor* Player = SpawnMarker(World, FVector(8000.0, 8000.0, 0.0));
	// The family plot opens with Main 1; the chapel yard only opens when the player walks up to it; boot hill is open from the start.
	ARespawnMarker* FamilyPlot = PlaceSaveGrave(World, TEXT("FamilyPlot"), FVector(0.0, 0.0, 0.0), 90.0, false, TEXT("TestMain1"));
	ARespawnMarker* ChapelYard = PlaceSaveGrave(World, TEXT("ChapelYard"), FVector(3000.0, 0.0, 0.0), 0.0, false);
	ARespawnMarker* BootHill = PlaceSaveGrave(World, TEXT("BootHill"), FVector(0.0, 3000.0, 0.0), 0.0, true);
	if (!TestTrue(TEXT("The runner, the player and the graves"), Runner && Travel && Player && FamilyPlot && ChapelYard && BootHill))
	{
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Main1 = NewMission(Scratch, TEXT("TestMain1"), EMissionKind::Main, EMissionStart::Automatic, Valley);
	AddObjective<UMissionEventObjective>(Main1, 0)->Event = TEXT("Test.Done");
	Runner->BeginForTesting({ Main1 }, Campaign, Player, Valley);

	// Far from every grave: too far (the family plot's gate is named first).
	TestTrue(TEXT("Far from the chapel yard: too far"), ChapelYard->CheckMark(Player) == EGraveMarkBlock::TooFar);
	TestTrue(TEXT("The family plot waits for Main 1 first"), FamilyPlot->CheckMark(Player) == EGraveMarkBlock::NotYet);
	TestTrue(TEXT("Its gate is its opening mission"), FamilyPlot->GetMarkGate() == FName(TEXT("TestMain1")));
	TestTrue(TEXT("No mission, no gate"), ChapelYard->GetMarkGate().IsNone());
	TestTrue(TEXT("Nobody at all"), ChapelYard->CheckMark(nullptr) == EGraveMarkBlock::NoPlayer);

	// Beside the family plot before Main 1 is finished: nothing, and it isn't opened by the walk.
	Player->SetActorLocation(FVector(300.0, 0.0, 0.0));
	TestTrue(TEXT("Beside the family plot, Main 1 not done: not yet"), FamilyPlot->CheckMark(Player) == EGraveMarkBlock::NotYet);
	TestTrue(TEXT("...so nothing is marked"), FamilyPlot->TryMark(Player) == EGraveMark::None && !FamilyPlot->IsMarked(Campaign) && !FamilyPlot->IsActive(Campaign));

	// Main 1 finished: the grave opens by itself, but that marks nothing and moves no save point.
	Campaign.Complete(TEXT("TestMain1"));
	FamilyPlot->DispatchBeginPlay();
	TestTrue(TEXT("Main 1 finished: the family plot is open"), FamilyPlot->IsActive(Campaign) && Campaign.IsRespawnActive(TEXT("FamilyPlot")));
	TestTrue(TEXT("...unmarked, no save point, its lantern dark"), !FamilyPlot->IsMarked(Campaign) && Campaign.GetLastGrave(Valley).IsNone() && !FamilyPlot->IsLit());
	TestTrue(TEXT("Opened by the story, not by the walk"), FamilyPlot->CheckMark(Player) == EGraveMarkBlock::None);

	// The walk: a first mark.
	TestTrue(TEXT("Walking up to the family plot: its first mark"), FamilyPlot->TryMark(Player) == EGraveMark::First);
	TestTrue(TEXT("...marked and the valley's save point"), FamilyPlot->IsMarked(Campaign) && FamilyPlot->IsSavePoint(Campaign, Valley)
		&& Campaign.GetLastGrave(Valley) == FName(TEXT("FamilyPlot")));
	TestTrue(TEXT("...its lantern lit with a flare"), FamilyPlot->IsLit() && FamilyPlot->IsFlaring());
	TestTrue(TEXT("Standing there still: nothing again"), FamilyPlot->TryMark(Player) == EGraveMark::None);

	// The radius is the edge.
	Player->SetActorLocation(ChapelYard->GetActorLocation() + FVector(ChapelYard->MarkRadius + 1.f, 0.0, 0.0));
	TestTrue(TEXT("A step outside the chapel yard's radius: too far"), ChapelYard->CheckMark(Player) == EGraveMarkBlock::TooFar);
	TestTrue(TEXT("...and it isn't open"), ChapelYard->TryMark(Player) == EGraveMark::None && !ChapelYard->IsActive(Campaign));
	Player->SetActorLocation(ChapelYard->GetActorLocation() + FVector(ChapelYard->MarkRadius - 1.f, 0.0, 0.0));
	TestFalse(TEXT("Closed before the walk"), ChapelYard->IsActive(Campaign));
	TestTrue(TEXT("A step inside: its first mark"), ChapelYard->TryMark(Player) == EGraveMark::First);
	TestTrue(TEXT("...a first mark opens a grave the story hadn't"), ChapelYard->IsActive(Campaign) && Campaign.IsRespawnActive(TEXT("ChapelYard")));
	TestTrue(TEXT("...and fast travel can go there"), Travel->IsOpen(*ChapelYard));
	TestTrue(TEXT("...the save point moves to it"), Campaign.GetLastGrave(Valley) == FName(TEXT("ChapelYard")) && ChapelYard->IsSavePoint());
	TestTrue(TEXT("...the family plot stays marked and lit"), FamilyPlot->IsMarked(Campaign) && FamilyPlot->IsLit() && !FamilyPlot->IsSavePoint());

	// Back to the family plot: the save point moves back, the flame lifts, no first mark.
	Player->SetActorLocation(FVector(100.0, 100.0, 0.0));
	TestTrue(TEXT("Back at the family plot: marked again"), FamilyPlot->TryMark(Player) == EGraveMark::Again);
	TestTrue(TEXT("...the save point is the family plot, the chapel yard still marked"), Campaign.GetLastGrave(Valley) == FName(TEXT("FamilyPlot"))
		&& ChapelYard->IsMarked(Campaign) && ChapelYard->IsLit());
	TestTrue(TEXT("...the flame lifts"), FamilyPlot->IsFlaring());

	// A grave open from the start isn't marked until walked up to; one that doesn't mark on approach isn't, at all.
	Player->SetActorLocation(BootHill->GetActorLocation() + FVector(100.0, 0.0, 0.0));
	TestTrue(TEXT("Boot hill, open from the start: still unmarked"), BootHill->IsActive(Campaign) && !BootHill->IsMarked(Campaign) && !BootHill->IsLit());
	BootHill->bMarkOnApproach = false;
	TestTrue(TEXT("Not marking on approach: off"), BootHill->CheckMark(Player) == EGraveMarkBlock::Off && BootHill->TryMark(Player) == EGraveMark::None);
	TestTrue(TEXT("...though the console can"), BootHill->TryMark(Player, /*bForce*/ true) == EGraveMark::First);
	TestTrue(TEXT("...and it is the save point now"), Campaign.GetLastGrave(Valley) == FName(TEXT("BootHill")) && BootHill->IsLit());
	BootHill->bMarkOnApproach = true;
	TestTrue(TEXT("The console marks from anywhere, past a closed story"), FamilyPlot->TryMark(Player, true) == EGraveMark::Again);

	// A grave with its own gate, open from the start.
	ARespawnMarker* Gated = PlaceSaveGrave(World, TEXT("Gated"), FVector(-3000.0, 0.0, 0.0), 0.0, true, NAME_None, TEXT("TestMain9"));
	Player->SetActorLocation(FVector(-3000.0, 100.0, 0.0));
	TestTrue(TEXT("MarkAfterMission holds an open grave back"), Gated && Gated->GetMarkGate() == FName(TEXT("TestMain9")) && Gated->CheckMark(Player) == EGraveMarkBlock::NotYet);
	Campaign.Complete(TEXT("TestMain9"));
	TestTrue(TEXT("...until that mission is finished"), Gated->CheckMark(Player) == EGraveMarkBlock::None);

	// A trip's landing is never a save point.
	ARespawnMarker* Platform = PlaceSaveGrave(World, TEXT("Platform"), FVector(-6000.0, 0.0, 0.0), 0.0, true);
	Platform->Tags.Add(FName(TEXT("Landing_Depot")));
	Player->SetActorLocation(FVector(-6000.0, 100.0, 0.0));
	TestTrue(TEXT("A landing: never"), Platform->CheckMark(Player) == EGraveMarkBlock::Landing && Platform->TryMark(Player) == EGraveMark::None);

	// A player who is down marks nothing.
	ARespawnMarker* Quiet = PlaceSaveGrave(World, TEXT("Quiet"), FVector(0.0, -3000.0, 0.0), 0.0, true);
	ATargetDummy* Body = SpawnDummy(World, Quiet->GetActorLocation() + FVector(100.0, 0.0, 0.0));
	UHealthComponent* Health = Body ? Body->FindComponentByClass<UHealthComponent>() : nullptr;
	if (TestTrue(TEXT("A stand-in with health beside the quiet grave"), Quiet && Health))
	{
		Hurt(Body, Health->GetMaxHealth() * 10.f, nullptr);
		if (TestTrue(TEXT("...is down"), Health->IsDead()))
		{
			TestTrue(TEXT("A dead player: no"), Quiet->CheckMark(Body) == EGraveMarkBlock::Dying && Quiet->TryMark(Body) == EGraveMark::None && !Quiet->IsMarked(Campaign));
		}
	}

	// A scene playing holds it back until the scene is over.
	ARespawnMarker* Hushed = PlaceSaveGrave(World, TEXT("Hushed"), FVector(0.0, -6000.0, 0.0), 0.0, true);
	USceneSubsystem* Scenes = World->GetSubsystem<USceneSubsystem>();
	FScenesSetting On(2);
	Player->SetActorLocation(Hushed->GetActorLocation() + FVector(100.0, 0.0, 0.0));
	FScenePlay Scene;
	Scene.Name = TEXT("TestGraveScene");
	Scene.bLookOnly = false;
	Scene.Timeline.AddMoment(TEXT("End"), 30.f);
	if (TestTrue(TEXT("A scene plays"), Scenes && Scenes->Play(MoveTemp(Scene))))
	{
		TestTrue(TEXT("In a scene: no"), Hushed->CheckMark(Player) == EGraveMarkBlock::Scene && Hushed->TryMark(Player) == EGraveMark::None);
		Scenes->SkipScene();
		TestTrue(TEXT("The scene over: it marks"), Hushed->TryMark(Player) == EGraveMark::First);
	}

	// A level begun with a grave marked before stands lit from the start, without a flare.
	ARespawnMarker* Remembered = PlaceSaveGrave(World, TEXT("Remembered"), FVector(6000.0, 0.0, 0.0), 0.0, true);
	GraveMarkRules::Record(Campaign, Valley, TEXT("Remembered"));
	Remembered->DispatchBeginPlay();
	TestTrue(TEXT("Marked in an earlier visit: lit from the start, no flare"), Remembered->IsLit() && !Remembered->IsFlaring());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraveSaveContinueTest, "Looter.Grave.Save.Continue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGraveSaveContinueTest::RunTest(const FString& Parameters)
{
	// Where a Continue wakes the player: the area's last found roost when it stands here open (FindLastFound), else where
	// they were saved (never an open roost nearest it: the user's rule, 2026-10-14, is the last touched); never on a trip's
	// arrival. A death wakes at the same roost.
	FCampaignRecord Played;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Stand = SpawnMarker(World, FVector::ZeroVector);
	ARespawnMarker* FamilyPlot = PlaceSaveGrave(World, TEXT("FamilyPlot"), FVector(0.0, 0.0, 0.0), 90.0, true);
	ARespawnMarker* ChapelYard = PlaceSaveGrave(World, TEXT("ChapelYard"), FVector(6000.0, 0.0, 0.0), 180.0, true);
	ARespawnMarker* Shut = PlaceSaveGrave(World, TEXT("Shut"), FVector(-4000.0, 0.0, 0.0), 0.0, false, TEXT("TestNeverDone"));
	ARespawnMarker* Platform = PlaceSaveGrave(World, TEXT("Platform"), FVector(3000.0, 0.0, 0.0), 0.0, true);
	if (!TestTrue(TEXT("The runner and the graves"), Runner && Stand && FamilyPlot && ChapelYard && Shut && Platform))
	{
		return false;
	}
	Platform->Tags.Add(FName(TEXT("Landing_Depot")));
	Runner->BeginForTesting({}, Played, Stand, Valley);
	const FVector SavedNearChapel(5000.0, 0.0, 100.0);
	auto Wake = [&](const FCampaignRecord& Record) { return ARespawnMarker::FindLastFound(World, Record, Valley); };

	FCampaignRecord Record;
	TestNull(TEXT("Nothing found: no roost, though the chapel yard stands open near where they were saved"), Wake(Record));
	Record.SetLastGrave(Valley, TEXT("FamilyPlot"));
	TestTrue(TEXT("The family plot found last: there, though the chapel yard is nearer"), Wake(Record) == FamilyPlot);
	Record.SetLastGrave(Valley, TEXT("Shut"));
	TestNull(TEXT("The last found is closed: none (nothing else found)"), Wake(Record));
	Record.SetLastGrave(Valley, TEXT("Platform"));
	TestNull(TEXT("The last found is a trip's landing: never"), Wake(Record));
	Record.SetLastGrave(Valley, TEXT("Gone"));
	TestNull(TEXT("The last found isn't in this level: none"), Wake(Record));
	Record.LastGraves.Reset();
	Record.SetLastGrave(TEXT("TestElsewhere"), TEXT("FamilyPlot"));
	TestNull(TEXT("Another area's save point doesn't count"), Wake(Record));
	Record.SetLastGrave(Valley, TEXT("FamilyPlot"));

	// A death wakes at the same roost, wherever they fell (never the nearest open one).
	TestTrue(TEXT("A death near the chapel yard wakes at the save point, the family plot"),
		ARespawnMarker::ChooseWakeSpot(World, &Record).Grave == FamilyPlot);

	// The session loading: the player is put at the save point, facing its arrow, on its ground.
	ACharacter* Pawn = World->SpawnActor<ACharacter>(FVector(111.0, 222.0, 100.0), FRotator::ZeroRotator);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!TestTrue(TEXT("A player"), Pawn && Controller))
	{
		return false;
	}
	// A test world never readies its actors: the controller joins the world's list, as play would add it, and holds the pawn.
	World->AddController(Controller);
	Controller->SetPawn(Pawn);

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->Map = USessionSubsystem::MapOf(World);
	Save->bHasPlayerSpot = true;
	Save->PlayerLocation = SavedNearChapel;
	Save->PlayerView = FRotator(10.0, 45.0, 0.0);
	Save->Campaign.SetLastGrave(Valley, TEXT("FamilyPlot"));
	USessionSubsystem::RestoreWorld(World, *Save);
	const FVector Expected = GraveTravelRules::ArrivalLocation(FamilyPlot->GetActorLocation(), Pawn->GetDefaultHalfHeight());
	TestTrue(TEXT("Continue: standing on the save point's spot"), Pawn->GetActorLocation().Equals(Expected, 1.0));
	TestTrue(TEXT("...facing its arrow"), FMath::IsNearlyEqual(FRotator::NormalizeAxis(Pawn->GetActorRotation().Yaw), 90.0, 0.5));

	// A save with nothing found (roosts open all the same): where they stood, looking the way they looked.
	Pawn->SetActorLocation(FVector(111.0, 222.0, 100.0));
	Save->Campaign.LastGraves.Reset();
	USessionSubsystem::RestoreWorld(World, *Save);
	TestTrue(TEXT("Nothing found: where they were saved, not the open roost nearest it"), Pawn->GetActorLocation().Equals(SavedNearChapel, 1.0));
	TestTrue(TEXT("...with the view they had"), Controller->GetControlRotation().Equals(FRotator(10.0, 45.0, 0.0), 0.5));

	// A trip's arrival has no saved spot: the player stays where the trip put them (its landing), whatever was marked.
	Save->bHasPlayerSpot = false;
	Save->Campaign.SetLastGrave(Valley, TEXT("FamilyPlot"));
	Pawn->SetActorLocation(FVector(111.0, 222.0, 100.0));
	USessionSubsystem::RestoreWorld(World, *Save);
	TestTrue(TEXT("A trip's arrival: not moved to a grave"), Pawn->GetActorLocation().Equals(FVector(111.0, 222.0, 100.0), 1.0));
	return true;
}

// The grave's lantern became Hob's roost's beacon (wave 3, 2026-10-13): its look is tested in RoostPlayTests.cpp
// (Looter.Roost.Play.Look).

#endif
