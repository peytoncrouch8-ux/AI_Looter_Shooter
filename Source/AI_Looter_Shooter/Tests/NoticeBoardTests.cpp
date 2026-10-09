#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Areas/AreaDefinition.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionPlaceObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/MissionTestWorld.h"
#include "Tutorial/MissionClearZoneObjective.h"
#include "Tutorial/TutorialDirector.h"
#include "UI/World/NoticeBoardWidget.h"
#include "Weapons/WeaponDefinition.h"
#include "World/NoticeBoard.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Skyreach's notice board as its mission board (Docs/Polish/TutorialRework.md): the postings up once the first goal is
// done, listed as they stand, tracked and turned in at the board for guns (and never experience), their state kept by the
// session's campaign record; and the zone objective that counts Web Hollow's spiders and the Wallow's slimes from the world.

using namespace MissionTestWorld;

namespace
{
	const FName SkyreachId(TEXT("Skyreach"));
	const FName HollowTag(TEXT("TestHollow"));
	const FName RangeTag(TEXT("TestRange"));
	const FName WallowTag(TEXT("TestWallow"));
	const FName LookoutTag(TEXT("Place_Lookout"));
	const FVector HollowAt(5000.0, 5000.0, 0.0);
	const FVector WallowAt(2000.0, -5000.0, 0.0);
	const FVector LookoutAt(-6000.0, 6000.0, 0.0);
	const TCHAR* const ShotgunPath = TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun");

	/** A posting as Skyreach's are made (create_mission_assets.py): automatic, after the first goal, on Skyreach. */
	UMissionDefinition* NewPosting(UObject* Outer, const TCHAR* Id, bool bTurnedInAtBoard)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, EMissionKind::Tutorial, EMissionStart::Automatic, SkyreachId);
		Mission->Prerequisites = { FName(TEXT("Tutorial")) };
		if (bTurnedInAtBoard)
		{
			Mission->TurnIn.SpeakerTag = ANoticeBoard::GiverTag;
			Mission->TurnIn.GiverName = FText::FromString(TEXT("the notice board"));
		}
		else
		{
			Mission->TurnIn.bAutomatic = true;
		}
		return Mission;
	}

	/** A zone of dummies standing in for a hollow's creatures (they die for good in a test level). */
	void AddClearZone(UMissionDefinition* Mission, FName Tag, const FVector& Where, int32 Count)
	{
		UMissionClearZoneObjective* Clear = AddObjective<UMissionClearZoneObjective>(Mission, 0);
		Clear->Target.ActorTag = Tag;
		Clear->Zone.Location = Where;
		Clear->Zone.Radius = 1500.f;
		Clear->Count = Count;
	}

	/** Skyreach's missions as the assets hold them, made in code: the first goal, the four postings and the skiff. */
	TArray<UMissionDefinition*> MakeSkyreach(UObject* Outer, UWeaponDefinition* Shotgun)
	{
		UMissionDefinition* FirstGoal = GetDefault<ATutorialDirector>()->MakeBuiltInMission(Outer);
		UMissionDefinition* Hollow = NewPosting(Outer, TEXT("WebHollow"), true);
		AddClearZone(Hollow, HollowTag, HollowAt, 2);
		Hollow->Rewards.bGun = Shotgun != nullptr;
		Hollow->Rewards.GunKind = Shotgun;
		Hollow->SortOrder = 1;
		UMissionDefinition* Range = NewPosting(Outer, TEXT("RangePractice"), false);
		UMissionHitObjective* Hits = AddObjective<UMissionHitObjective>(Range, 0);
		Hits->Target.ActorTag = RangeTag;
		Hits->Count = 3;
		Range->SortOrder = 2;
		UMissionDefinition* Wallow = NewPosting(Outer, TEXT("Wallow"), true);
		AddClearZone(Wallow, WallowTag, WallowAt, 1);
		Wallow->SortOrder = 3;
		UMissionDefinition* Lookout = NewPosting(Outer, TEXT("Lookout"), false);
		AddObjective<UMissionReachObjective>(Lookout, 0)->Place.Actor.ActorTag = LookoutTag;
		Lookout->SortOrder = 4;
		UMissionDefinition* Skiff = NewMission(Outer, TEXT("BoardSkiff"), EMissionKind::Tutorial, EMissionStart::Manual, SkyreachId);
		AddObjective<UMissionBoardObjective>(Skiff, 0);
		Skiff->TurnIn.bAutomatic = true;
		Skiff->SortOrder = 10;
		return { FirstGoal, Hollow, Range, Wallow, Lookout, Skiff };
	}

	/** What the runner's missions gave as they finished, as the board's screen hears it. */
	struct FHeard
	{
		int32 Rewarded = 0;
		int64 Experience = 0;
		TMap<FName, FMissionRewardsGiven> Given;
	};

	void Listen(UMissionRunner& Runner, FHeard& Heard)
	{
		Runner.OnMissionCompleted.AddLambda([&Heard](const UMissionDefinition& Mission, const FMissionRewardsGiven& Given)
		{
			Heard.Rewarded += Given.IsEmpty() ? 0 : 1;
			Heard.Experience += Given.Experience;
			Heard.Given.Add(Mission.GetMissionId(), Given);
		});
	}

	const FNoticeBoardPosting* FindPosting(const TArray<FNoticeBoardPosting>& Postings, const TCHAR* Id)
	{
		return Postings.FindByPredicate([Id](const FNoticeBoardPosting& Each) { return Each.MissionId == FName(Id); });
	}

	ENoticePosting StateOf(const TArray<FNoticeBoardPosting>& Postings, const TCHAR* Id)
	{
		const FNoticeBoardPosting* Posting = FindPosting(Postings, Id);
		return Posting ? Posting->State : ENoticePosting::Locked;
	}

	/** Kills Victim as the player's shot would. */
	void Kill(AActor* Victim, AController* By)
	{
		Hurt(Victim, 100000.f, By);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNoticeBoardFlowTest, "Looter.Tutorial.Board.Flow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNoticeBoardFlowTest::RunTest(const FString& Parameters)
{
	// The board's postings through a whole visit: locked until the first goal is done (the board says to bring a gun), up
	// with Web Hollow tracked, counted, turned in at the board for a gun and no experience (by its screen, or by a talk at
	// it as the console sends), the practice ones done by themselves, and the skiff listed once it's up.
	FCampaignRecord Campaign;
	FHeard Heard;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	APlayerController* Shooter = World->SpawnActor<APlayerController>();
	ANoticeBoard* Board = World->SpawnActor<ANoticeBoard>(FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator);
	ATargetDummy* HollowA = SpawnDummy(World, HollowAt + FVector(300.0, 0.0, 0.0), HollowTag);
	ATargetDummy* HollowB = SpawnDummy(World, HollowAt - FVector(300.0, 0.0, 0.0), HollowTag);
	ATargetDummy* RangeDummy = SpawnDummy(World, FVector(4000.0, -1500.0, 0.0), RangeTag);
	ATargetDummy* WallowDummy = SpawnDummy(World, WallowAt, WallowTag);
	AActor* Lookout = SpawnMarker(World, LookoutAt, LookoutTag);
	if (!Runner || !Player || !Shooter || !Board || !HollowA || !HollowB || !RangeDummy || !WallowDummy || !Lookout)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Board->DispatchBeginPlay();
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, ShotgunPath);
	Runner->BeginForTesting(MakeSkyreach(CreatePackage(nullptr), Shotgun), Campaign, Player, SkyreachId);
	Listen(*Runner, Heard);
	Runner->Update(0.f);
	Runner->StartMission(TEXT("Tutorial"), 0, /*bForce*/ true);

	// Before the first goal: four postings, all locked (the board's note says why); the skiff isn't on the board.
	TArray<FNoticeBoardPosting> Postings = Board->GatherPostings(*Runner);
	TestEqual(TEXT("Four postings before the skiff is up"), Postings.Num(), 4);
	TestTrue(TEXT("All locked"), !Postings.ContainsByPredicate([](const FNoticeBoardPosting& Each) { return Each.State != ENoticePosting::Locked; }));
	TestFalse(TEXT("The skiff isn't listed yet"), FindPosting(Postings, TEXT("BoardSkiff")) != nullptr);
	TestFalse(TEXT("The board has a word for it"), Board->LockedNote.IsEmpty());
	TestEqual(TEXT("Listed in the board's order: Web Hollow first"), Postings.IsEmpty() ? FName() : Postings[0].MissionId, FName(TEXT("WebHollow")));

	// A gun, then the board read: they're up, Web Hollow tracked.
	Runner->CompleteStep(TEXT("Tutorial"));
	Board->Read(*Runner);
	Postings = Board->GatherPostings(*Runner);
	TestTrue(TEXT("Read with a gun: the first goal done"), Campaign.HasCompleted(TEXT("Tutorial")));
	const FNoticeBoardPosting* Hollow = FindPosting(Postings, TEXT("WebHollow"));
	TestTrue(TEXT("Web Hollow open, tracked, counted"), Hollow && Hollow->State == ENoticePosting::Open && Hollow->bTracked
		&& Hollow->Progress == TEXT("0 / 2") && Hollow->bTurnedInHere);
	TestTrue(TEXT("The range open, finishing by itself"), StateOf(Postings, TEXT("RangePractice")) == ENoticePosting::Open
		&& !FindPosting(Postings, TEXT("RangePractice"))->bTurnedInHere);
	TestTrue(TEXT("The Wallow and the lookout open"), StateOf(Postings, TEXT("Wallow")) == ENoticePosting::Open
		&& StateOf(Postings, TEXT("Lookout")) == ENoticePosting::Open);

	// Web Hollow cleared: ready to turn in, at the board (its prompt says so); nothing given yet.
	Kill(HollowA, Shooter);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestEqual(TEXT("One down: 1 / 2"), FindPosting(Board->GatherPostings(*Runner), TEXT("WebHollow"))->Progress, FString(TEXT("1 / 2")));
	Kill(HollowB, Shooter);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("Both down: ready to turn in"), StateOf(Board->GatherPostings(*Runner), TEXT("WebHollow")) == ENoticePosting::Ready);
	TestTrue(TEXT("...to the board"), Runner->FindTurnInAt(Board) && Runner->FindTurnInAt(Board)->GetMissionId() == FName(TEXT("WebHollow")));
	TestEqual(TEXT("...nothing given yet"), Heard.Rewarded, 0);

	// Turned in at the board: done, its gun (when the shotgun's kind is in the project), never experience.
	TestFalse(TEXT("The lookout, still open, can't be turned in"), Board->TurnIn(*Runner, TEXT("Lookout")));
	TestTrue(TEXT("Web Hollow turned in at the board"), Board->TurnIn(*Runner, TEXT("WebHollow")));
	TestTrue(TEXT("...done"), Campaign.HasCompleted(TEXT("WebHollow")) && StateOf(Board->GatherPostings(*Runner), TEXT("WebHollow")) == ENoticePosting::Done);
	TestFalse(TEXT("...and not twice"), Board->TurnIn(*Runner, TEXT("WebHollow")));
	if (Shotgun)
	{
		const FMissionRewardsGiven* Given = Heard.Given.Find(TEXT("WebHollow"));
		TestTrue(TEXT("...its reward a gun"), Given && Given->bGun);
		TestEqual(TEXT("The card's reward line"), UNoticeBoardWidget::DescribeReward(MakeSkyreach(CreatePackage(nullptr), Shotgun)[1]->Rewards).ToString(),
			FString::Printf(TEXT("Reward: %s"), *Shotgun->DisplayName.ToString()));
	}
	else
	{
		AddWarning(TEXT("No DA_PumpShotgun: Web Hollow's gun wasn't tried."));
	}
	TestEqual(TEXT("Skyreach gives no experience"), Heard.Experience, static_cast<int64>(0));

	// The range: hits on its dummies finish it on the spot, nothing to turn in.
	for (int32 Shot = 0; Shot < 3; ++Shot)
	{
		Hurt(RangeDummy, 10.f, Shooter);
	}
	TestTrue(TEXT("Three hits: the range is done by itself"), Campaign.HasCompleted(TEXT("RangePractice")));
	TestFalse(TEXT("...and not turned in at the board"), Board->TurnIn(*Runner, TEXT("RangePractice")));

	// The Wallow: a talk at the board (the console's Looter.Mission.Event Talk Speaker_NoticeBoard) turns it in too.
	Kill(WallowDummy, Shooter);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("The bog cleared: ready"), Runner->IsReadyToTurnIn(TEXT("Wallow")));
	Runner->NotifyEvent(FMissionEvent::Talked(Board));
	TestTrue(TEXT("A talk at the board turns it in"), Campaign.HasCompleted(TEXT("Wallow")));

	// Tracking from the board, and the lookout reached.
	TestTrue(TEXT("Track the lookout"), Board->Track(*Runner, TEXT("Lookout")) && Runner->GetTrackedMission() == FName(TEXT("Lookout")));
	TestFalse(TEXT("A posting that's done can't be tracked"), Board->Track(*Runner, TEXT("WebHollow")));
	Player->SetActorLocation(LookoutAt + FVector(200.0, 0.0, 900.0));
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("Up at the lookout: done by itself"), Campaign.HasCompleted(TEXT("Lookout")));

	// The skiff, started by the jetty once Web Hollow is in: on the board from then on.
	Runner->StartMission(TEXT("BoardSkiff"));
	TestTrue(TEXT("The skiff listed once it's up"), StateOf(Board->GatherPostings(*Runner), TEXT("BoardSkiff")) == ENoticePosting::Open);
	TestEqual(TEXT("Still no experience, all told"), Heard.Experience, static_cast<int64>(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNoticeBoardSaveTest, "Looter.Tutorial.Board.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNoticeBoardSaveTest::RunTest(const FString& Parameters)
{
	// The postings' state is the session's campaign record: through the save's format and into a new level, the first goal
	// stays done (the postings go up without reading the board again), one ready to turn in comes back ready, one done stays
	// done, and one open starts over, its zone read from the world, so creatures already dead still count.
	UPackage* Scratch = CreatePackage(nullptr);
	FCampaignRecord Played;
	{
		FTestWorldWrapper TestLevel;
		if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
		{
			return false;
		}
		UWorld* World = TestLevel.GetTestWorld();
		UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
		AActor* Player = SpawnMarker(World, FVector::ZeroVector);
		APlayerController* Shooter = World->SpawnActor<APlayerController>();
		ANoticeBoard* Board = World->SpawnActor<ANoticeBoard>(FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator);
		ATargetDummy* HollowA = SpawnDummy(World, HollowAt, HollowTag);
		ATargetDummy* HollowB = SpawnDummy(World, HollowAt + FVector(200.0, 0.0, 0.0), HollowTag);
		ATargetDummy* RangeDummy = SpawnDummy(World, FVector(4000.0, -1500.0, 0.0), RangeTag);
		SpawnDummy(World, WallowAt, WallowTag);
		if (!Runner || !Player || !Shooter || !Board || !HollowA || !HollowB || !RangeDummy)
		{
			return false;
		}
		Runner->BeginForTesting(MakeSkyreach(Scratch, nullptr), Played, Player, SkyreachId);
		Runner->Update(0.f);
		Runner->StartMission(TEXT("Tutorial"), 0, /*bForce*/ true);
		Runner->CompleteStep(TEXT("Tutorial"));
		Board->Read(*Runner);
		Kill(HollowA, Shooter);
		Kill(HollowB, Shooter);
		for (int32 Shot = 0; Shot < 3; ++Shot)
		{
			Hurt(RangeDummy, 10.f, Shooter);
		}
		Runner->Update(UMissionRunner::UpdateInterval);
		TestTrue(TEXT("Web Hollow ready, the range done, the Wallow open"), Runner->IsReadyToTurnIn(TEXT("WebHollow"))
			&& Played.HasCompleted(TEXT("RangePractice")) && Runner->IsRunning(TEXT("Wallow")) && !Runner->IsReadyToTurnIn(TEXT("Wallow")));
	}

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->Campaign = Played;
	TArray<uint8> Bytes;
	const ULooterSessionSave* Read = UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	if (!TestNotNull(TEXT("Saved and read back"), Read))
	{
		return false;
	}
	TestTrue(TEXT("The save keeps the first goal and the range done, Web Hollow waiting"), Read->Campaign.HasCompleted(TEXT("Tutorial"))
		&& Read->Campaign.HasCompleted(TEXT("RangePractice")) && Read->Campaign.IsReadyToTurnIn(TEXT("WebHollow")));
	TestTrue(TEXT("...and no posting as the story's mission"), Read->Campaign.ActiveMission.IsNone());

	// The next visit: the Wallow's slime already dead as it begins (a level never brings the dead back mid-play).
	FCampaignRecord Loaded = Read->Campaign;
	FTestWorldWrapper NextLevel;
	if (!TestTrue(TEXT("Next test level made"), NextLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = NextLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	APlayerController* Shooter = World->SpawnActor<APlayerController>();
	ANoticeBoard* Board = World->SpawnActor<ANoticeBoard>(FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator);
	ATargetDummy* Slime = SpawnDummy(World, WallowAt, WallowTag);
	if (!Runner || !Player || !Shooter || !Board || !Slime)
	{
		return false;
	}
	Kill(Slime, Shooter);
	FHeard Heard;
	Runner->BeginForTesting(MakeSkyreach(CreatePackage(nullptr), nullptr), Loaded, Player, SkyreachId);
	Listen(*Runner, Heard);
	Runner->Update(0.f);
	TestFalse(TEXT("The first goal doesn't start again"), Runner->IsRunning(TEXT("Tutorial")));
	TestTrue(TEXT("Web Hollow back, waiting at the board"), Runner->IsReadyToTurnIn(TEXT("WebHollow")));
	TestFalse(TEXT("The range stays done"), Runner->IsRunning(TEXT("RangePractice")));
	TestTrue(TEXT("The Wallow up again by itself, its slime counted though it died before: ready"), Runner->IsReadyToTurnIn(TEXT("Wallow")));
	const TArray<FNoticeBoardPosting> Postings = Board->GatherPostings(*Runner);
	TestTrue(TEXT("The board says so"), StateOf(Postings, TEXT("WebHollow")) == ENoticePosting::Ready
		&& StateOf(Postings, TEXT("RangePractice")) == ENoticePosting::Done && StateOf(Postings, TEXT("Wallow")) == ENoticePosting::Ready);
	Runner->NotifyEvent(FMissionEvent::Talked(Board));
	TestTrue(TEXT("One talk at the board turns both in"), Loaded.HasCompleted(TEXT("WebHollow")) && Loaded.HasCompleted(TEXT("Wallow"))
		&& Loaded.ReadyMissions.IsEmpty());
	TestEqual(TEXT("...for no experience"), Heard.Experience, static_cast<int64>(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClearZoneObjectiveTest, "Looter.Tutorial.ClearZone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FClearZoneObjectiveTest::RunTest(const FString& Parameters)
{
	// Clear a zone: done once none of its own is left alive, counted from the world: one killed before it began counts,
	// one outside the zone doesn't, and the count stops short until the last is down.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UMissionSubsystem* Display = World->GetSubsystem<UMissionSubsystem>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	APlayerController* Shooter = World->SpawnActor<APlayerController>();
	ATargetDummy* Early = SpawnDummy(World, HollowAt + FVector(0.0, 400.0, 0.0), HollowTag);
	ATargetDummy* Near = SpawnDummy(World, HollowAt + FVector(-200.0, 0.0, 0.0), HollowTag);
	ATargetDummy* Far = SpawnDummy(World, HollowAt + FVector(600.0, 0.0, 0.0), HollowTag);
	ATargetDummy* Outside = SpawnDummy(World, HollowAt + FVector(5000.0, 0.0, 0.0), HollowTag);
	if (!Runner || !Display || !Player || !Shooter || !Early || !Near || !Far || !Outside)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	const FName Id(TEXT("TestClear"));
	UMissionDefinition* Mission = NewMission(CreatePackage(nullptr), TEXT("TestClear"), EMissionKind::Tutorial, EMissionStart::Manual, SkyreachId);
	AddClearZone(Mission, HollowTag, HollowAt, 3);
	Mission->TurnIn.bAutomatic = true;
	const UMissionClearZoneObjective* Clear = Cast<UMissionClearZoneObjective>(Mission->GetObjective(0, 0));
	TestTrue(TEXT("An empty zone is cleared, not missing its targets"), Clear && Clear->HasTargets(FMissionContext()));

	Kill(Early, Shooter);
	Runner->BeginForTesting({ Mission }, Campaign, Player, SkyreachId);
	Runner->Update(0.f);
	Runner->StartMission(Id, 0, /*bForce*/ true);
	TArray<FMissionObjectiveView> Views = Runner->GetObjectiveViews(Id);
	TestTrue(TEXT("Killed before it began: counted, 1 / 3"), Views.Num() == 1 && Views[0].Progress == TEXT("1 / 3"));
	FVector Waypoint = FVector::ZeroVector;
	TestTrue(TEXT("The arrow on the nearest still standing in the zone"), Display->GetTrackedWaypoint(Waypoint)
		&& Waypoint.Equals(Near->GetActorLocation(), 1.0));

	Kill(Outside, Shooter);
	Runner->Update(UMissionRunner::UpdateInterval);
	Views = Runner->GetObjectiveViews(Id);
	TestTrue(TEXT("One outside the zone doesn't count"), Views.Num() == 1 && Views[0].Progress == TEXT("1 / 3"));
	Kill(Near, Shooter);
	Runner->Update(UMissionRunner::UpdateInterval);
	Views = Runner->GetObjectiveViews(Id);
	TestTrue(TEXT("Two down: 2 / 3, still running"), Views.Num() == 1 && Views[0].Progress == TEXT("2 / 3") && Runner->IsRunning(Id));
	Kill(Far, Shooter);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("None left: done"), Campaign.HasCompleted(Id));

	// A zone holding more than its count reads short until it's empty.
	FMissionObjectiveState State;
	FMissionContext Context;
	Context.World = World;
	UMissionClearZoneObjective* Big = NewObject<UMissionClearZoneObjective>(GetTransientPackage());
	Big->Target.ActorTag = WallowTag;
	Big->Zone.Location = WallowAt;
	Big->Zone.Radius = 1500.f;
	Big->Count = 1;
	SpawnDummy(World, WallowAt, WallowTag);
	SpawnDummy(World, WallowAt + FVector(100.0, 0.0, 0.0), WallowTag);
	Big->Begin(Context, State);
	TestTrue(TEXT("Two alive for a count of one: 0, not done"), State.Count == 0 && Big->CountAlive(Context) == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNoticeBoardPostingsTest, "Looter.Tutorial.Board.Postings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNoticeBoardPostingsTest::RunTest(const FString& Parameters)
{
	// The project's postings (create_mission_assets.py): each on Skyreach, outside the story, up after the first goal,
	// worth no experience; the ones that pay turned in at the board (Web Hollow a shotgun, the Wallow a gun of Uncommon or
	// better), the practice ones done by themselves; and their words name nothing of the story.
	TMap<FName, const UMissionDefinition*> Found;
	for (const UMissionDefinition* Mission : UMissionDefinition::LoadAll())
	{
		Found.Add(Mission->GetMissionId(), Mission);
	}
	const ANoticeBoard* Board = GetDefault<ANoticeBoard>();
	for (const FName Id : Board->Postings)
	{
		if (!Found.Contains(Id))
		{
			AddError(FString::Printf(TEXT("DA_Mission_%s belongs in /Game/Data/Missions: run Tools/Unreal/create_mission_assets.py in the editor."), *Id.ToString()));
		}
	}
	// The story's names (Docs/Story.md), none of which may show on Skyreach.
	const TCHAR* const StoryNames[] = { TEXT("Ellis"), TEXT("Ransom"), TEXT("Delia"), TEXT("Tilly"), TEXT("Sexton"), TEXT("Hob"),
		TEXT("Abel"), TEXT("Aldana"), TEXT("Amos"), TEXT("Deacon"), TEXT("Dunne"), TEXT("Ned"), TEXT("Teropa"), TEXT("Reaches"),
		TEXT("Ranger"), TEXT("Gravewind"), TEXT("Unpaid"), TEXT("Saint"), TEXT("Keeper"), TEXT("Ledger") };
	for (const FName Id : Board->Postings)
	{
		const UMissionDefinition* const* Entry = Found.Find(Id);
		const UMissionDefinition* Mission = Entry ? *Entry : nullptr;
		if (!Mission)
		{
			continue;
		}
		const FString Label = Id.ToString();
		TestTrue(*FString::Printf(TEXT("%s: Skyreach's own"), *Label), Mission->Kind == EMissionKind::Tutorial && Mission->Area == FName(TEXT("Skyreach")));
		TestFalse(*FString::Printf(TEXT("%s: no experience"), *Label), Mission->Rewards.GivesExperience());
		TestFalse(*FString::Printf(TEXT("%s: a notice to read"), *Label), Mission->Summary.IsEmpty());
		if (Id != FName(TEXT("BoardSkiff")))
		{
			TestTrue(*FString::Printf(TEXT("%s: up once the first goal is done"), *Label), Mission->Start == EMissionStart::Automatic
				&& Mission->Prerequisites.Contains(FName(TEXT("Tutorial"))));
		}
		// Paid at the board, or done on the spot: nobody walks back for nothing.
		TestTrue(*FString::Printf(TEXT("%s: turned in at the board when it pays, else done by itself"), *Label),
			Mission->Rewards.IsEmpty() ? Mission->TurnIn.bAutomatic : Board->Gives(*Mission) || Mission->TurnIn.SpeakerTag == ANoticeBoard::GiverTag);
		TArray<FString> Words = { Mission->Title.ToString(), Mission->Summary.ToString() };
		for (const FMissionStep& Step : Mission->Steps)
		{
			for (const TObjectPtr<UMissionObjective>& Objective : Step.Objectives)
			{
				if (Objective)
				{
					Words.Add(Objective->Text.ToString());
					Words.Add(Objective->ShortText.ToString());
				}
			}
		}
		for (const FString& Said : Words)
		{
			for (const TCHAR* Name : StoryNames)
			{
				// Names as written, capitalized ("Ned", never the "ned" of "turned").
				TestFalse(*FString::Printf(TEXT("%s names nothing of the story (%s)"), *Label, Name), Said.Contains(Name, ESearchCase::CaseSensitive));
			}
		}
	}
	const UMissionDefinition* const* Hollow = Found.Find(TEXT("WebHollow"));
	if (Hollow && *Hollow)
	{
		const FMissionRewards& Rewards = (*Hollow)->Rewards;
		TestTrue(TEXT("Web Hollow pays a shotgun"), Rewards.bGun && Rewards.GunKind && Rewards.GunKind->GetPathName().Contains(TEXT("PumpShotgun")));
		TestTrue(TEXT("...turned in at the board"), (*Hollow)->TurnIn.SpeakerTag == ANoticeBoard::GiverTag);
		TestTrue(TEXT("...counted from the world (six spiders in the hollow)"), Cast<UMissionClearZoneObjective>((*Hollow)->GetObjective(0, 0)) != nullptr);
	}
	const UMissionDefinition* const* Wallow = Found.Find(TEXT("Wallow"));
	if (Wallow && *Wallow)
	{
		const FMissionRewards& Rewards = (*Wallow)->Rewards;
		TestTrue(TEXT("The Wallow pays a gun, Uncommon or better"), Rewards.bGun && static_cast<uint8>(Rewards.GunRarityFloor) >= static_cast<uint8>(EWeaponRarity::Uncommon));
		TestEqual(TEXT("...its card says so"), UNoticeBoardWidget::DescribeReward(Rewards).ToString(), FString(TEXT("Reward: a gun, Uncommon or better")));
	}
	TestEqual(TEXT("A posting with no reward says it's practice"), UNoticeBoardWidget::DescribeReward(FMissionRewards()).ToString(),
		FString(TEXT("No reward: it's practice")));
	return true;
}

#endif
