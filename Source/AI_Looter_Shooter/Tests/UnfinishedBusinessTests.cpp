#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/HuntingGround.h"
#include "Creatures/UnpaidCreature.h"
#include "Interaction/InteractionComponent.h"
#include "Missions/MissionCombatObjectives.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionLastingInteractObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Story/AmosWhitlock.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPointComponent.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "Tests/UnfinishedBusinessTestWorld.h"
#include "World/HayBale.h"
#include "World/InstancedProps.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Side 2, "Unfinished Business" (Docs/Areas/RansomsRest.md): Amos at his fence (only Ellis can see him), six hay bales
// loaded into his barn by holding Interact, his old hired hands driven off (6, one of them Restless), and Amos again, who
// sits on his fence to wait for the saint to come back. Amos himself (his topics, poses and the rail) is AmosTests.cpp's.

using namespace UnfinishedBusinessTestWorld;

namespace
{
	/** The first objective's progress of a running mission ("2 / 6"), or "not running". */
	FString ProgressOf(const UMissionRunner& Runner, FName MissionId)
	{
		const TArray<FMissionObjectiveView> Views = Runner.GetObjectiveViews(MissionId);
		return Views.IsEmpty() ? FString(TEXT("not running")) : Views[0].Progress;
	}

	FString OnScreen(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT("|") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}

	/** Lets whatever is being said play out to its end. */
	void PlayOut(UCaptionSubsystem& Captions)
	{
		for (int32 Line = 0; Line < 40 && Captions.GetCurrent(); ++Line)
		{
			Captions.Update(Captions.GetCurrent()->Seconds + 0.01f);
		}
	}

	/** The bales of a test level: a row out in the field (their middles at the stand-in's eye height), the stack far off. */
	FVector FieldSpot(int32 Index)
	{
		return FVector(1000.0 + 500.0 * Index, 3000.0, 46.0);
	}

	FVector StackSpot(int32 Index)
	{
		return FVector(-3000.0, -3000.0 + 100.0 * Index, 0.0);
	}

	/** Remarks as the build script gives them (from the line sets): Amos at the first and the last, Hob at the third. */
	TArray<FHayBaleRemark> TestRemarks()
	{
		auto Remark = [](int32 Count, const TCHAR* Speaker, const TCHAR* Words)
		{
			FHayBaleRemark Made;
			Made.LoadedCount = Count;
			Made.Lines = { FStoryLine::Make(FText::FromString(Speaker), FText::FromString(Words), 2.f) };
			return Made;
		};
		return { Remark(1, TEXT("Amos Whitlock"), TEXT("Test.First")), Remark(3, TEXT("Hob"), TEXT("Test.Third")),
			Remark(BaleCount, TEXT("Amos Whitlock"), TEXT("Test.Last")) };
	}

	/** Through the save format and back, read as the game reads a session; null when it failed. */
	ULooterSessionSave* WriteAndRead(ULooterSessionSave* Save)
	{
		TArray<uint8> Bytes;
		return UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	}

	/** How many creatures of Class at Rank a spawner's waves bring in all. */
	int32 CountBrought(const AEncounterSpawner& Spawner, const UClass* Class, ECreatureRank Rank)
	{
		const int32 Waves = FMath::Max(Spawner.NumWaves, 1);
		int32 Brought = 0;
		for (const FEncounterGroup& Group : Spawner.Groups)
		{
			if (!Group.CreatureClass || !Group.CreatureClass->IsChildOf(Class) || Group.RankRoll != EEncounterRankRoll::Fixed
				|| Group.Rank != Rank)
			{
				continue;
			}
			for (int32 Wave = 1; Wave <= Waves; ++Wave)
			{
				Brought += EncounterRules::JoinsWave(Group, Wave) ? Group.Count : 0;
			}
		}
		return Brought;
	}

	/** One of the layout's obstacle lines (x and y in cm): a path, or a polygon closed round. */
	struct FLayoutLine
	{
		TArray<FVector2D> Points;
		bool bClosed = false;
	};

	/** The layout's obstacle lines by id (Art/Levels/RansomsRest/layout.json), which build_area_dressing.py dresses. */
	TMap<FString, FLayoutLine> LayoutLines(FAutomationTestBase& Test)
	{
		TMap<FString, FLayoutLine> Lines;
		FString Text;
		const FString LayoutFile = FPaths::Combine(FPaths::ProjectDir(), TEXT("Art/Levels/RansomsRest/layout.json"));
		TSharedPtr<FJsonObject> Layout;
		const TArray<TSharedPtr<FJsonValue>>* Obstacles = nullptr;
		if (!Test.TestTrue(TEXT("layout.json reads"), FFileHelper::LoadFileToString(Text, *LayoutFile))
			|| !Test.TestTrue(TEXT("...as JSON with its obstacles"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Layout)
				&& Layout.IsValid() && Layout->TryGetArrayField(TEXT("obstacles"), Obstacles) && Obstacles))
		{
			return Lines;
		}
		for (const TSharedPtr<FJsonValue>& Each : *Obstacles)
		{
			const TSharedPtr<FJsonObject> Obstacle = Each->AsObject();
			FString Id;
			const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
			if (!Obstacle.IsValid() || !Obstacle->TryGetStringField(TEXT("id"), Id))
			{
				continue;
			}
			FLayoutLine Line;
			Line.bClosed = Obstacle->TryGetArrayField(TEXT("polygon"), Points);
			if (!Line.bClosed && !Obstacle->TryGetArrayField(TEXT("path"), Points))
			{
				continue;
			}
			for (const TSharedPtr<FJsonValue>& Point : *Points)
			{
				const TArray<TSharedPtr<FJsonValue>>& XY = Point->AsArray();
				if (XY.Num() >= 2)
				{
					Line.Points.Emplace(XY[0]->AsNumber(), XY[1]->AsNumber());
				}
			}
			Lines.Add(Id, MoveTemp(Line));
		}
		return Lines;
	}

	/** How far a point is from a layout line, seen from above (cm). */
	double DistanceToLine(const FVector& Point, const FLayoutLine& Line)
	{
		const int32 Count = Line.Points.Num();
		if (Count == 0)
		{
			return TNumericLimits<double>::Max();
		}
		const FVector2D At(Point.X, Point.Y);
		double Best = FVector2D::Distance(At, Line.Points[0]);
		for (int32 Index = 0; Index + 1 < Count + (Line.bClosed ? 1 : 0); ++Index)
		{
			const FVector2D A = Line.Points[Index];
			const FVector2D Along = Line.Points[(Index + 1) % Count] - A;
			const double LengthSquared = Along.SizeSquared();
			const double T = LengthSquared > 0.0 ? FMath::Clamp(FVector2D::DotProduct(At - A, Along) / LengthSquared, 0.0, 1.0) : 0.0;
			Best = FMath::Min(Best, FVector2D::Distance(At, A + Along * T));
		}
		return Best;
	}

	/** Where every instance of a mesh the level's dressing (AInstancedProps) places stands, relative to the level: a level
	 * loaded only to look at has no world transforms. A section's is its pivot, its first post. */
	TArray<FVector> DressingPivots(const ULevel& Level, const TCHAR* MeshName)
	{
		TArray<FVector> Pivots;
		for (const AActor* Actor : Level.Actors)
		{
			const AInstancedProps* Props = Cast<AInstancedProps>(Actor);
			const UStaticMesh* Mesh = Props && Props->Instances ? Props->Instances->GetStaticMesh() : nullptr;
			if (!Mesh || Mesh->GetName() != MeshName)
			{
				continue;
			}
			const FTransform Placed = Props->GetRootComponent()->GetRelativeTransform();
			for (int32 Index = 0; Index < Props->Instances->GetInstanceCount(); ++Index)
			{
				FTransform Instance;
				if (Props->Instances->GetInstanceTransform(Index, Instance, /*bWorldSpace=*/false))
				{
					Pivots.Add((Instance * Placed).GetLocation());
				}
			}
		}
		return Pivots;
	}

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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnfinishedBusinessMissionTest, "Looter.Story.UnfinishedBusiness.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnfinishedBusinessMissionTest::RunTest(const FString& Parameters)
{
	// Side 2 as its asset has it: after Main 4 on Ransom's Rest; talk to Amos, load six bales (held, counted from the world),
	// drive off the hands (their encounter cleared: six); turned in to Amos; 40 experience and a guaranteed Epic gun.
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Side2")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Side2.DA_Mission_Side2"));
		if (TestNotNull(TEXT("DA_Mission_Side2 loads"), Asset))
		{
			TestTrue(TEXT("Side2, a side mission starting by itself on Ransom's Rest, after Main4"), Asset->GetMissionId() == SideTwo
				&& Asset->Kind == EMissionKind::Side && Asset->Start == EMissionStart::Automatic && Asset->Area == FName(TEXT("RansomsRest"))
				&& Asset->Prerequisites == TArray<FName>({ MainFour }));
			TestEqual(TEXT("Three steps"), Asset->Steps.Num(), SideTwoSteps);
			const UMissionTalkObjective* Meet = Cast<UMissionTalkObjective>(Asset->GetObjective(0, 0));
			const UMissionLastingInteractObjective* Load = Cast<UMissionLastingInteractObjective>(Asset->GetObjective(1, 0));
			const UMissionClearObjective* Hands = Cast<UMissionClearObjective>(Asset->GetObjective(2, 0));
			TestTrue(TEXT("1: talk to Amos at his fence"), Meet && Meet->SpeakerTag == AAmosWhitlock::SpeakerTag);
			TestTrue(TEXT("2: six hay bales loaded, held, counted from the world"), Load && Load->Count == BaleCount && Load->bHold
				&& Load->Target.ActorTag == AHayBale::BaleTag);
			TestTrue(TEXT("3: the hands driven off (their encounter cleared, six)"), Hands && Hands->SpawnerId == HandsId
				&& Hands->Count == HandsBasic + 1);
			TestTrue(TEXT("Turned in to Amos (his thanks: the talk that was its fourth step)"), Asset->NeedsTurnIn()
				&& Asset->TurnIn.SpeakerTag == AAmosWhitlock::SpeakerTag);
			TestTrue(TEXT("Its reward: 40 experience and a guaranteed Epic gun"),
				Asset->Rewards.Experience == 40 && Asset->Rewards.bGun && Asset->Rewards.GunRarityFloor == EWeaponRarity::Epic);
		}
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Side2 isn't made yet: run Tools/Unreal/create_side_mission_assets.py. The flow below runs on a copy."));
	}

	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(-300.0, 0.0, 0.0));
	UInteractionComponent* Interaction = Player ? NewObject<UInteractionComponent>(Player, TEXT("Interaction")) : nullptr;
	if (Interaction)
	{
		Player->AddInstanceComponent(Interaction);
		Interaction->RegisterComponent();
	}
	// The hands' yard far off from Amos and the field.
	AEncounterSpawner* Hands = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, 9000.0, 0.0), SetUpHands);
	if (!Runner || !Captions || !Player || !Interaction || !Hands)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	Runner->BeginForTesting({ MakeMainFour(Scratch), MakeSideTwo(Scratch) }, Campaign, Player, Valley);
	AAmosWhitlock* Amos = PlaceAmos(World, FVector::ZeroVector);
	TArray<AHayBale*> Bales;
	for (int32 Index = 0; Index < BaleCount; ++Index)
	{
		Bales.Add(SpawnBale(World, FieldSpot(Index), StackSpot(Index)));
	}
	if (!TestTrue(TEXT("Amos and the bales placed"), Amos && !Bales.Contains(nullptr)))
	{
		return false;
	}
	for (AHayBale* Bale : Bales)
	{
		Bale->LoadRemarks = TestRemarks();
	}
	Amos->DispatchBeginPlay();

	// Before Main 4: no Amos, no Side 2, and the hay is only hay.
	Runner->Update(0.f);
	TestFalse(TEXT("Side 2 waits for Main 4"), Runner->IsRunning(SideTwo));
	TestFalse(TEXT("...Amos isn't at his fence yet"), Amos->IsShown());
	TestFalse(TEXT("...and the bales can't be loaded"), Bales[0]->CanLoad());

	// Main 4 done: Side 2 opens, and Amos leans on his fence.
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainFourDone")));
	TestTrue(TEXT("Main 4 done: Side 2 starts by itself, at talking to Amos"), Runner->IsRunning(SideTwo) && Runner->GetStep(SideTwo) == 0);
	TestTrue(TEXT("...Amos is at his fence, leaning on it"), Amos->IsShown() && Amos->GetSeat() == EAmosPose::Lean && !Amos->IsSitting());
	TestFalse(TEXT("...still, nothing of him ticks"), Amos->IsActorTickEnabled());
	TestFalse(TEXT("...the bales wait for him to ask"), Bales[0]->CanLoad());

	// 1: talk to him.
	TestTrue(TEXT("Amos talked to"), Amos->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("...he ticks while he talks"), Amos->IsActorTickEnabled());
	TestEqual(TEXT("Talked to: load the bales"), Runner->GetStep(SideTwo), 1);
	TestEqual(TEXT("...none loaded yet"), ProgressOf(*Runner, SideTwo), FString(TEXT("0 / 6")));
	PlayOut(*Captions);
	Amos->UpdatePose(5.f);
	TestFalse(TEXT("His words over and his head turned back: he doesn't tick"), Amos->IsActorTickEnabled());

	// 2: the first bale held in the field, as the player's Interact key does.
	const FVector First = Bales[0]->GetActorLocation();
	Player->SetActorLocationAndRotation(FVector(First.X - 150.0, First.Y, 0.0), FRotator::ZeroRotator);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The bale is what's looked at, to hold"), Interaction->GetFocusedActor() == Bales[0]
		&& Interaction->GetFocusedOptions().bHold && !Interaction->GetFocusedOptions().bTap
		&& Interaction->GetFocusedOptions().HoldPrompt.ToString() == TEXT("Load the hay bale"));
	Interaction->PressInteract();
	Interaction->UpdateInteraction(Bales[0]->LoadHoldSeconds * 0.5f);
	TestFalse(TEXT("Half a hold: still in the field"), Bales[0]->IsLoaded());
	Interaction->UpdateInteraction(Bales[0]->LoadHoldSeconds);
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Held: loaded"), Bales[0]->IsLoaded());
	TestTrue(TEXT("...gone from the field and standing in the stack"), Bales[0]->Bale->bHiddenInGame && !Bales[0]->Stacked->bHiddenInGame);
	TestEqual(TEXT("...counted at once"), ProgressOf(*Runner, SideTwo), FString(TEXT("1 / 6")));
	TestEqual(TEXT("...and Amos remarks"), OnScreen(*Captions), FString(TEXT("Amos Whitlock|Test.First")));
	TestFalse(TEXT("A loaded bale can't be loaded again"), Bales[0]->CanLoad() || Bales[0]->Load(Player));

	// Two by the console: counted on the next look.
	Bales[1]->Load(nullptr);
	Bales[2]->Load(nullptr);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestEqual(TEXT("Loaded with no event: counted on the next look"), ProgressOf(*Runner, SideTwo), FString(TEXT("3 / 6")));

	// A reload starts a side mission over from its first step: the barn kept its hay, so nothing is lost.
	TestTrue(TEXT("Started over"), Runner->StartMission(SideTwo, 0, /*bForce*/ true));
	PlayOut(*Captions);
	TestTrue(TEXT("...Amos talked to again"), Amos->SpeakerPoint->Talk(Player));
	TestEqual(TEXT("...still 3 of 6"), ProgressOf(*Runner, SideTwo), FString(TEXT("3 / 6")));
	for (int32 Index = 3; Index < BaleCount; ++Index)
	{
		Bales[Index]->Load(nullptr);
	}
	Runner->Update(UMissionRunner::UpdateInterval);
	TestEqual(TEXT("Six in: drive off the hands"), Runner->GetStep(SideTwo), 2);
	TestEqual(TEXT("...the stack holds all six"), AHayBale::CountLoaded(World), BaleCount);

	// 3: the hired hands, in the barn yard as the player comes into it.
	TestTrue(TEXT("The hands' fight is on"), Hands->IsStoryActive());
	Hands->UpdateEncounter(0.5f);
	TestEqual(TEXT("Far off, nothing comes yet"), Hands->NumAlive(), 0);
	Player->SetActorLocation(Hands->GetActorLocation() + FVector(1500.0, 800.0, 0.0));
	Hands->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Out = Hands->GetAliveCreatures();
	TestEqual(TEXT("In the yard: six of them"), Out.Num(), HandsBasic + 1);
	TestEqual(TEXT("...five Unpaid"), EncounterTestWorld::CountOf(Out, AUnpaidCreature::StaticClass(), ECreatureRank::Basic), HandsBasic);
	TestEqual(TEXT("...and one Restless"), EncounterTestWorld::CountOf(Out, AUnpaidCreature::StaticClass(), ECreatureRank::Rare), 1);
	TestFalse(TEXT("...coming for the player"), Out.ContainsByPredicate([Player](const ACreatureBase* Unpaid) { return Unpaid->GetTarget() != Player; }));
	EncounterTestWorld::KillAll(*Hands);
	Hands->UpdateEncounter(0.5f);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("Six down: the yard is clear"), Hands->GetState() == EEncounterState::Cleared && Hands->GetKilled() == HandsBasic + 1);
	TestEqual(TEXT("...talk to Amos (its step past the last)"), Runner->GetStep(SideTwo), 3);
	TestTrue(TEXT("...ready to turn in to him"), Runner->IsReadyToTurnIn(SideTwo) && !Campaign.HasCompleted(SideTwo));
	TestFalse(TEXT("...and the fight is over"), Hands->IsStoryActive());

	// Amos again: Side 2 is turned in, and once his words are over he settles onto his fence.
	PlayOut(*Captions);
	TestTrue(TEXT("Amos talked to"), Amos->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("Side 2 is turned in: finished"), Campaign.HasCompleted(SideTwo) && !Runner->IsRunning(SideTwo));
	Amos->UpdatePose(0.1f);
	TestTrue(TEXT("...he leans on while he's talking"), Amos->GetSeat() == EAmosPose::Lean);
	PlayOut(*Captions);
	Amos->UpdatePose(0.1f);
	TestTrue(TEXT("His words over: he settles onto the rail"), Amos->IsSettling() && Amos->GetSeat() == EAmosPose::Sit);
	Amos->UpdatePose(Amos->SitSeconds);
	Amos->UpdatePose(0.1f);
	TestTrue(TEXT("...and sits on his fence to wait for the saint"), Amos->IsSitting());
	TestFalse(TEXT("...still again, nothing of him ticks"), Amos->IsActorTickEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnfinishedBusinessBalesTest, "Looter.Story.UnfinishedBusiness.Bales",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnfinishedBusinessBalesTest::RunTest(const FString& Parameters)
{
	// The bales: loadable only once Amos has asked (the console forces one), each counted once from the world, a remark at
	// the first, the third and the last, kept by the session with the map's world (put back quietly), and all in the stack
	// once Side 2 is done whatever this level saw.
	FCampaignRecord Campaign;
	FTestWorldWrapper PlayedLevel;
	FTestWorldWrapper AgainLevel;
	if (!TestTrue(TEXT("Test levels made"), PlayedLevel.CreateTestWorld(EWorldType::EditorPreview)
		&& AgainLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* Played = PlayedLevel.GetTestWorld();
	UMissionRunner* Runner = Played->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(Played);
	AActor* Player = MissionTestWorld::SpawnMarker(Played, FVector::ZeroVector);
	if (!Runner || !Captions || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	Runner->BeginForTesting({ MakeMainFour(Scratch), MakeSideTwoAt(Scratch, 1) }, Campaign, Player, Valley);
	Runner->Update(0.f);
	TArray<AHayBale*> Bales;
	for (int32 Index = 0; Index < BaleCount; ++Index)
	{
		FActorSpawnParameters Params;
		Params.Name = FName(*FString::Printf(TEXT("Whitlock_HayBale_%d"), Index + 1));
		AHayBale* Bale = Played->SpawnActor<AHayBale>(FieldSpot(Index), FRotator::ZeroRotator, Params);
		if (Bale)
		{
			Bale->Stacked->SetWorldLocation(StackSpot(Index));
			Bale->LoadRemarks = TestRemarks();
			Bale->DispatchBeginPlay();
		}
		Bales.Add(Bale);
	}
	if (!TestFalse(TEXT("The bales placed"), Bales.Contains(nullptr)))
	{
		return false;
	}
	TestTrue(TEXT("Tagged for the missions"), Bales[0]->ActorHasTag(AHayBale::BaleTag));
	TestTrue(TEXT("Its story is the build script's: once Amos has asked, in the stack after Side 2"),
		Bales[0]->LoadWhen.DuringMission == SideTwo && Bales[0]->LoadWhen.FromStep == 1 && Bales[0]->LoadedWhen.AfterMissions.Contains(SideTwo));
	TestFalse(TEXT("Before Side 2 they can't be loaded"), Bales[0]->CanLoad() || Bales[0]->Load(nullptr));
	TestTrue(TEXT("...but the console forces one"), Bales[0]->Load(nullptr, /*bForce*/ true) && Bales[0]->IsUsedUp());
	Bales[0]->PutBack();
	TestTrue(TEXT("...and puts it back in the field"), !Bales[0]->IsLoaded() && !Bales[0]->Bale->bHiddenInGame && Bales[0]->Stacked->bHiddenInGame);
	PlayOut(*Captions);

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainFourDone")));
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Step0")));
	TestEqual(TEXT("Side 2 at its bales"), Runner->GetStep(SideTwo), 1);
	TestTrue(TEXT("Now they can be loaded"), Bales[0]->CanLoad());
	const FInteractionOptions Options = Bales[0]->GetInteractionOptions(*NewObject<UInteractionComponent>(Player));
	TestTrue(TEXT("...by a hold only, a heave longer than a poster's"), Options.bUsable && Options.bHold && !Options.bTap
		&& Options.HoldSeconds >= 1.f);
	TestTrue(TEXT("...found at the bale in the field"), Bales[0]->GetInteractionLocation().IsSet()
		&& FVector::Dist(*Bales[0]->GetInteractionLocation(), Bales[0]->GetActorLocation()) < 60.0);

	// Each one in: Amos at the first, Hob at the third, Amos at the last.
	const TCHAR* Said[] = { TEXT("Amos Whitlock|Test.First"), nullptr, TEXT("Hob|Test.Third"), nullptr, nullptr, TEXT("Amos Whitlock|Test.Last") };
	for (int32 Index = 0; Index < BaleCount; ++Index)
	{
		PlayOut(*Captions);
		TestTrue(*FString::Printf(TEXT("Bale %d loaded"), Index + 1), Bales[Index]->Load(Player));
		TestEqual(*FString::Printf(TEXT("...%d in the stack"), Index + 1), AHayBale::CountLoaded(Played), Index + 1);
		TestEqual(*FString::Printf(TEXT("...what's said at %d"), Index + 1), OnScreen(*Captions), FString(Said[Index] ? Said[Index] : TEXT("none")));
		TestTrue(TEXT("...standing in the stack, at its place there"), !Bales[Index]->Stacked->bHiddenInGame
			&& Bales[Index]->Stacked->GetComponentLocation().Equals(StackSpot(Index), 1.0));
		if (Index == 2)
		{
			Bales[3]->Load(nullptr);
			Runner->Update(UMissionRunner::UpdateInterval);
			TestEqual(TEXT("Counted from the world: three loaded, and one more with no event (the console)"),
				ProgressOf(*Runner, SideTwo), FString(TEXT("4 / 6")));
			Bales[3]->PutBack();
		}
	}
	Runner->Update(UMissionRunner::UpdateInterval);
	TestEqual(TEXT("Six in: the step is done"), Runner->GetStep(SideTwo), 2);

	// Kept with the map's world: two loaded, through the save file, put back quietly when the level is played again.
	for (int32 Index = 2; Index < BaleCount; ++Index)
	{
		Bales[Index]->PutBack();
	}
	TestEqual(TEXT("Two left in the stack"), AHayBale::CountLoaded(Played), 2);
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	USessionSubsystem::CaptureWorld(Played, *Save);
	const FString PlayedMap = USessionSubsystem::MapOf(Played);
	const FSavedMapWorld* Kept = Save->FindWorld(PlayedMap);
	TestTrue(TEXT("The two loaded are kept by name, and only they"), Kept && Kept->LoadedBales.Num() == 2
		&& Kept->LoadedBales.Contains(Bales[0]->GetFName()) && Kept->LoadedBales.Contains(Bales[1]->GetFName()));
	ULooterSessionSave* Back = WriteAndRead(Save);
	const FSavedMapWorld* Read = Back ? Back->FindWorld(PlayedMap) : nullptr;
	if (TestTrue(TEXT("Through the save file"), Read && Read->LoadedBales.Num() == 2))
	{
		// The level played again: its bales as built, all in the field; the session puts back what was loaded. (The
		// second test level stands in for the same map, so its world is filed under that map's name.)
		UWorld* Again = AgainLevel.GetTestWorld();
		TArray<AHayBale*> AgainBales;
		for (int32 Index = 0; Index < BaleCount; ++Index)
		{
			FActorSpawnParameters Params;
			Params.Name = Bales[Index]->GetFName();
			AHayBale* Bale = Again->SpawnActor<AHayBale>(FieldSpot(Index), FRotator::ZeroRotator, Params);
			if (Bale)
			{
				Bale->LoadRemarks = TestRemarks();
				Bale->DispatchBeginPlay();
			}
			AgainBales.Add(Bale);
		}
		if (TestFalse(TEXT("The level's bales again, in the field"), AgainBales.Contains(nullptr) || AHayBale::CountLoaded(Again) != 0))
		{
			// Copied first: adding the second map's entry may move the first's.
			const FSavedMapWorld PlayedWorld = *Read;
			Back->FindOrAddWorld(USessionSubsystem::MapOf(Again)) = PlayedWorld;
			USessionSubsystem::RestoreWorld(Again, *Back);
			const UCaptionSubsystem* AgainCaptions = UCaptionSubsystem::Get(Again);
			TestTrue(TEXT("The two in the stack again"), AgainBales[0]->IsLoaded() && AgainBales[1]->IsLoaded()
				&& AHayBale::CountLoaded(Again) == 2);
			TestTrue(TEXT("...quietly: nothing said"), !AgainCaptions || AgainCaptions->GetCurrent() == nullptr);
		}
	}

	// Side 2 done and turned in to Amos: the hay's in, whatever this level saw.
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Step2")));
	TestTrue(TEXT("Its objectives done: waiting for Amos"), Runner->IsReadyToTurnIn(SideTwo) && !Campaign.HasCompleted(SideTwo));
	Runner->NotifyEvent(FMissionEvent::Named(FMissionEvent::Talk, nullptr, AAmosWhitlock::SpeakerTag));
	TestTrue(TEXT("Side 2 finished"), Campaign.HasCompleted(SideTwo));
	TestEqual(TEXT("...every bale in the stack"), AHayBale::CountLoaded(Played), BaleCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnfinishedBusinessHandsTest, "Looter.Story.UnfinishedBusiness.Hands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnfinishedBusinessHandsTest::RunTest(const FString& Parameters)
{
	// "Drive off his old hired hands, Unpaid from the fever winter (6, one of them Restless)": on during Side 2's third step
	// only, appearing in the barn yard as the player comes within 22 m, inside its fence and coming for them, giving up at
	// the fence; cleared, the step is done.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(1500.0, 800.0, 0.0));
	AEncounterSpawner* Hands = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, SetUpHands);
	if (!Runner || !Player || !Hands)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	Runner->BeginForTesting({ MakeMainFour(Scratch), MakeSideTwoAt(Scratch, 2) }, Campaign, Player, Valley);
	Runner->Update(0.f);
	Hands->UpdateEncounter(0.5f);
	TestTrue(TEXT("Before Side 2 the yard is quiet"), Hands->GetState() == EEncounterState::Off && Hands->NumAlive() == 0);

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainFourDone")));
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Step0")));
	Hands->UpdateEncounter(0.5f);
	TestTrue(TEXT("While the bales are loaded, still quiet"), Runner->GetStep(SideTwo) == 1 && !Hands->IsStoryActive() && Hands->NumAlive() == 0);

	Player->SetActorLocation(FVector(-4500.0, 0.0, 0.0));
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Step1")));
	Hands->UpdateEncounter(0.5f);
	TestTrue(TEXT("The hay in: the fight is on, waiting for the player 45 m off"), Hands->IsStoryActive()
		&& Hands->GetState() == EEncounterState::Waiting && Hands->NumAlive() == 0);
	Player->SetActorLocation(FVector(1500.0, 800.0, 0.0));
	Hands->UpdateEncounter(0.5f);
	const TArray<ACreatureBase*> Out = Hands->GetAliveCreatures();
	TestEqual(TEXT("In the yard: six"), Out.Num(), HandsBasic + 1);
	TestEqual(TEXT("...five Basic"), EncounterTestWorld::CountOf(Out, AUnpaidCreature::StaticClass(), ECreatureRank::Basic), HandsBasic);
	TestEqual(TEXT("...one Restless"), EncounterTestWorld::CountOf(Out, AUnpaidCreature::StaticClass(), ECreatureRank::Rare), 1);
	TestFalse(TEXT("...each tagged for the fight"), Out.ContainsByPredicate([](const ACreatureBase* Unpaid) { return !Unpaid->ActorHasTag(HandsTag); }));
	const TArray<FVector> Fence(YardFence, UE_ARRAY_COUNT(YardFence));
	TestFalse(TEXT("...each inside the yard's fence"), Out.ContainsByPredicate([&Fence](const ACreatureBase* Unpaid)
	{
		return !FHuntingGround::IsInsidePolygon(Fence, FVector2D(Unpaid->GetActorLocation()));
	}));
	TestFalse(TEXT("...none within 8 m of the player"), Out.ContainsByPredicate([Player](const ACreatureBase* Unpaid)
	{
		return FVector::Dist2D(Unpaid->GetActorLocation(), Player->GetActorLocation()) < 800.0;
	}));
	TestFalse(TEXT("...coming for the player"), Out.ContainsByPredicate([Player](const ACreatureBase* Unpaid) { return Unpaid->GetTarget() != Player; }));
	const FHuntingGround Ground = Hands->MakeHuntingGround();
	TestTrue(TEXT("Their ground: the yard, and just past its fence"), Ground.Contains(FVector::ZeroVector, FVector::ZeroVector)
		&& Ground.Contains(FVector(2000.0, 0.0, 0.0), FVector::ZeroVector));
	TestFalse(TEXT("...not 5 m past it: they give up there"), Ground.Contains(FVector(2400.0, 0.0, 0.0), FVector::ZeroVector));

	EncounterTestWorld::KillAll(*Hands);
	Hands->UpdateEncounter(0.5f);
	Runner->Update(UMissionRunner::UpdateInterval);
	TestTrue(TEXT("Six down: cleared, and Side 2 moves on to Amos"), Hands->GetState() == EEncounterState::Cleared
		&& Runner->GetStep(SideTwo) == 3);
	TestFalse(TEXT("...the fight is over"), Hands->IsStoryActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnfinishedBusinessPlacedTest, "Looter.Story.UnfinishedBusiness.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnfinishedBusinessPlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (build_area_whitlock.py): Amos at his fence by its gate, his six bales with their stack by the
	// barn, the hired hands' fight in the barn yard; and Whitlock Fields' fences and wall (build_area_dressing.py's
	// instances, one AInstancedProps per mesh, theirs told from the rest by the layout's lines they stand on).
	const ULevel* Level = LoadRansomsRest(*this);
	if (!Level)
	{
		return true;
	}
	const AAmosWhitlock* Amos = nullptr;
	const AEncounterSpawner* Hands = nullptr;
	TArray<const AHayBale*> Bales;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		Amos = Amos ? Amos : Cast<AAmosWhitlock>(Actor);
		if (const AHayBale* Bale = Cast<AHayBale>(Actor))
		{
			Bales.Add(Bale);
		}
		if (const AEncounterSpawner* Spawner = Cast<AEncounterSpawner>(Actor); Spawner && Spawner->SpawnerId == HandsId)
		{
			Hands = Spawner;
		}
	}
	if (!Amos && Bales.IsEmpty() && !Hands)
	{
		AddWarning(TEXT("Side 2's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	// The sections standing on Whitlock Fields' lines: a section's pivot is a joint on its line, so within a hand of it.
	const TMap<FString, FLayoutLine> Lines = LayoutLines(*this);
	const auto StandingOn = [&Lines](const TArray<FVector>& Pivots, const TCHAR* LineId)
	{
		const FLayoutLine* Line = Lines.Find(LineId);
		return Line ? Pivots.FilterByPredicate([Line](const FVector& Pivot) { return DistanceToLine(Pivot, *Line) < 50.0; })
			: TArray<FVector>();
	};
	const TArray<FVector> Rails = DressingPivots(*Level, TEXT("SM_FenceRail"));
	const TArray<FVector> AmosRails = StandingOn(Rails, TEXT("amosFence"));
	const int32 YardRails = StandingOn(Rails, TEXT("barnYardFence")).Num();
	const int32 Walls = StandingOn(DressingPivots(*Level, TEXT("SM_StoneWall")), TEXT("fieldWall")).Num();
	if (TestNotNull(TEXT("Amos at his fence"), Amos))
	{
		TestTrue(TEXT("...tagged for the missions"), Amos->ActorHasTag(AAmosWhitlock::SpeakerTag));
		TestTrue(TEXT("...there from Main 4's finish, on the rail after Side 2"), Amos->ShownWhen.AfterMissions.Contains(MainFour)
			&& Amos->SitWhen.AfterMissions.Contains(SideTwo));
		TestTrue(TEXT("...his words at each of Side 2's steps"), Amos->SpeakerPoint->Topics.ContainsByPredicate([](const FSpeakerTopic& Topic)
		{
			return Topic.When.DuringMission == SideTwo && Topic.When.FromStep == 3 && Topic.LineSet != nullptr;
		}) && Amos->SpeakerPoint->Topics.ContainsByPredicate([](const FSpeakerTopic& Topic)
		{
			return Topic.When.DuringMission == SideTwo && Topic.When.FromStep == 0 && Topic.LineSet != nullptr;
		}));
		// In the middle of a span, 75 cm from a section's first post (the one past his gate: build_area_whitlock.amos_spot).
		const FVector At = Amos->GetRootComponent()->GetRelativeLocation();
		TestTrue(TEXT("...on his fence's line, between two of its posts"), AmosRails.ContainsByPredicate([&At](const FVector& Post)
		{
			return FVector::Dist2D(Post, At) < 100.0;
		}));
		if (!Amos->HasModel())
		{
			AddWarning(TEXT("Amos stands in plain shapes: SK_Amos isn't in this checkout (Art/Models/Creatures/Amos.py)."));
		}
	}
	TestEqual(TEXT("Six hay bales"), Bales.Num(), BaleCount);
	if (Bales.Num() == BaleCount)
	{
		// Where each one's place in the stack is, from the level as saved (relative transforms: a level loaded only to look
		// at has no world ones).
		const auto InStack = [](const AHayBale* Bale)
		{
			return Bale->GetRootComponent()->GetRelativeTransform().TransformPosition(Bale->Stacked->GetRelativeLocation());
		};
		FVector Stack = FVector::ZeroVector;
		for (const AHayBale* Bale : Bales)
		{
			Stack += InStack(Bale);
		}
		Stack /= Bales.Num();
		TestFalse(TEXT("...each tagged, loaded once Amos has asked"), Bales.ContainsByPredicate([](const AHayBale* Bale)
		{
			return !Bale->ActorHasTag(AHayBale::BaleTag) || Bale->LoadWhen.DuringMission != SideTwo || Bale->LoadWhen.FromStep != 1
				|| !Bale->LoadedWhen.AfterMissions.Contains(SideTwo);
		}));
		TestFalse(TEXT("...each with its place in one stack by the barn, out in the field from it"), Bales.ContainsByPredicate([&Stack, &InStack](const AHayBale* Bale)
		{
			return FVector::Dist2D(InStack(Bale), Stack) > 200.0 || FVector::Dist2D(Bale->GetRootComponent()->GetRelativeLocation(), Stack) < 800.0;
		}));
		TestTrue(TEXT("...remarked on"), !Bales[0]->LoadRemarks.IsEmpty());
	}
	if (TestNotNull(TEXT("The hired hands' fight"), Hands))
	{
		TestEqual(TEXT("...five Basic Unpaid"), CountBrought(*Hands, AUnpaidCreature::StaticClass(), ECreatureRank::Basic), HandsBasic);
		TestEqual(TEXT("...and one Restless"), CountBrought(*Hands, AUnpaidCreature::StaticClass(), ECreatureRank::Rare), 1);
		TestTrue(TEXT("...on Side 2's third step only"), Hands->ActiveWhen.DuringMission == SideTwo && Hands->ActiveWhen.FromStep == 2
			&& Hands->ActiveWhen.BeforeStep == 3);
		TestTrue(TEXT("...coming for the player as they appear, inside the yard's fence"), Hands->bHuntOnSpawn
			&& Hands->GroundCorners.Num() >= 3 && Hands->SpawnPoints.Num() >= HandsBasic + 1);
		TestTrue(TEXT("...tagged for the fight"), Hands->CreatureTags.Contains(HandsTag));
	}
	// Too few: the dressing isn't placed (Tools/Unreal/build_area.py RansomsRest dressing), or no longer lays these lines.
	TestTrue(TEXT("Whitlock Fields' fences (Amos's and the barn yard's), the dressing's"), AmosRails.Num() + YardRails >= 40);
	TestTrue(TEXT("...and its stone field wall"), Walls >= 10);
	return true;
}

#endif
