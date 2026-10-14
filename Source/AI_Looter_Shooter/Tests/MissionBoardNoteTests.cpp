#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/Interactable.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLine.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "UI/World/NoticeBoardWidget.h"
#include "World/MissionArtifact.h"
#include "World/NoticeBoard.h"
#include "World/WantedPoster.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Ruth Calder's note on the Rangers' board (the play-through of 2026-10-14): the paper (AWantedPoster, read for its words)
// could win the Interact key over the card laid on it (AMissionArtifact Artifact_CalderNote, which offers Calder's Caches),
// the board's screen said the job was "picked up elsewhere: Ruth Calder's note, at Main Street" under that very note, and
// a job turned in at a board said nothing.

using namespace MissionTestWorld;

namespace
{
	const FName BoardTag(TEXT("NoticeBoard_TestRangers"));
	const FName NoteGiver(TEXT("Artifact_TestNote"));
	const FName NoteJobId(TEXT("TestNoteJob"));
	const FName BountyId(TEXT("TestBountyJob"));
	const FName NoteDone(TEXT("Test.NoteJobDone"));
	const TCHAR* TallyLine = TEXT("Three tags on the tally.");

	/** The note's middle, straight ahead of the stand-in's eyes; the board's face toward the stand-in (yaw 180). */
	const FVector NoteAt(150.0, 0.0, InteractionTestWorld::EyeHeight);
	const FRotator Facing(0.0, 180.0, 0.0);
	/**
	 * Where the note hangs in the board's frame: SM_NoticeBoard's SOCKET_Decal (8.1 cm out of the posts' line, 1.36 m up,
	 * Boardwalk.py) and layout.json's calderNote offset (0, -40, 9).
	 */
	const FVector NoteOnBoard(8.1, -40.0, 145.0);
	/** The card as build_area_missionprops_valley.py lays it: CARD_PROUD in front of the paper, CARD_MARGIN wider each way. */
	constexpr double CardProud = 1.5;
	constexpr double CardMargin = 2.0;
	/** UInteractionComponent's: nothing farther is even asked. */
	constexpr double MaxReach = 400.0;

	struct FBoardScene
	{
		ANoticeBoard* Board = nullptr;
		AWantedPoster* Note = nullptr;
		AMissionArtifact* Card = nullptr;
		/** The card's size on the paper (half its width and height, cm). */
		FVector2D CardHalf = FVector2D::ZeroVector;

		bool IsWhole() const { return Board && Note && Card; }
	};

	/** The Rangers' board with Calder's note on its face and the card over the note, as the build scripts place them. */
	FBoardScene BuildBoard(UWorld* World)
	{
		FBoardScene Scene;
		Scene.Board = World->SpawnActor<ANoticeBoard>(NoteAt - Facing.RotateVector(NoteOnBoard), Facing);
		Scene.Note = World->SpawnActor<AWantedPoster>(NoteAt, Facing);
		Scene.Card = World->SpawnActor<AMissionArtifact>(NoteAt + Facing.Vector() * CardProud, Facing);
		if (!Scene.IsWhole())
		{
			return Scene;
		}
		// A town's board (build_area_board.py): its own tag, none of Skyreach's practice postings.
		Scene.Board->Postings.Reset();
		Scene.Board->MainPosting = NAME_None;
		Scene.Board->Tags.Add(BoardTag);
		Scene.Note->SetVariant(EWantedPosterVariant::CalderNote);
		// The card: an invisible engine cube a centimetre thick, the paper's size and the margin (place_artifact), tagged as
		// the giver and as the note.
		Scene.Card->Tags.Add(NoteGiver);
		Scene.Card->Tags.Add(AWantedPoster::NoteTag);
		Scene.Card->bTakeOnAccept = false;
		Scene.Card->Title = FText::FromString(TEXT("Ruth Calder's note"));
		Scene.Card->Prompt = FText::FromString(TEXT("Read the note"));
		const FVector Paper = Scene.Note->Hitbox->GetUnscaledBoxExtent();
		Scene.CardHalf = FVector2D(Paper.Y + CardMargin, Paper.Z + CardMargin);
		if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		{
			Scene.Card->Mesh->SetStaticMesh(Cube);
			Scene.Card->Mesh->SetRelativeScale3D(FVector(0.01, Scene.CardHalf.X * 2.0 / 100.0, Scene.CardHalf.Y * 2.0 / 100.0));
			Scene.Card->Mesh->SetHiddenInGame(true);
		}
		return Scene;
	}

	/** Calder's Caches' kind: offered by the note, turned in at the board with words of its own. */
	UMissionDefinition* MakeNoteJob(UObject* Outer, const TCHAR* Id = TEXT("TestNoteJob"))
	{
		UMissionDefinition* Job = NewMission(Outer, Id, EMissionKind::Side, EMissionStart::Offered, NAME_None);
		AddObjective<UMissionEventObjective>(Job, 0)->Event = NoteDone;
		Job->Offer.GiverTag = NoteGiver;
		Job->Offer.Briefing = FText::FromString(TEXT("RIM RANGERS. Three caches laid for any Ranger riding through."));
		Job->TurnIn.SpeakerTag = BoardTag;
		Job->TurnIn.GiverName = FText::FromString(TEXT("the Rangers' board"));
		Job->TurnIn.Lines = { FStoryLine::Make(FText::FromString(TEXT("Ellis")), FText::FromString(TallyLine)),
			FStoryLine::Make(FText::FromString(TEXT("Hob")), FText::FromString(TEXT("She'll still hang you, sunshine."))) };
		Job->Rewards.Experience = 100;
		return Job;
	}

	/** The sheriff's bounty's kind: the board's own posting. */
	UMissionDefinition* MakeBounty(UObject* Outer)
	{
		UMissionDefinition* Job = NewMission(Outer, TEXT("TestBountyJob"), EMissionKind::Side, EMissionStart::Offered, NAME_None);
		AddObjective<UMissionEventObjective>(Job, 0)->Event = TEXT("Test.Never");
		Job->Offer.GiverTag = BoardTag;
		Job->Offer.Briefing = FText::FromString(TEXT("BOUNTY."));
		Job->TurnIn.SpeakerTag = BoardTag;
		Job->Rewards.Experience = 100;
		return Job;
	}

	/**
	 * Something the key could focus, as UInteractionComponent::AddInteractable weighs it: at AtPoint when the key's line meets
	 * it there (it needs no sight check then), else at its own point. Left out beyond the component's farthest reach.
	 */
	void AddCandidate(TArray<FInteractionCandidate>& Out, const UInteractionComponent& User, const FInteractionView& View, AActor* Actor,
		const TOptional<FVector>& AtPoint)
	{
		const IInteractable* Interactable = Cast<IInteractable>(Actor);
		if (!Interactable)
		{
			return;
		}
		const FVector Point = AtPoint.IsSet() ? AtPoint.GetValue() : Interactable->GetInteractionLocation().Get(Actor->GetActorLocation());
		const double Distance = FVector::Dist(Point, View.ReachOrigin);
		if (Distance > MaxReach)
		{
			return;
		}
		FInteractionCandidate& Made = Out.AddDefaulted_GetRef();
		Made.Actor = Actor;
		Made.Location = Point;
		Made.Options = Interactable->GetInteractionOptions(User);
		Made.Reach = FMath::Min(Made.Options.Reach > 0.f ? Made.Options.Reach : User.Reach, static_cast<float>(MaxReach));
		Made.MinAim = InteractionFocus::NarrowingCone(User.InteractableNearAim, User.InteractableFarAim, static_cast<float>(Distance), Made.Reach);
		Made.bCheckSight = !AtPoint.IsSet();
	}

	/** Whether any words on the screen contain Words. */
	bool ScreenSays(const UUserWidget& Screen, const TCHAR* Words)
	{
		bool bFound = false;
		Screen.WidgetTree->ForEachWidget([&bFound, Words](UWidget* Each)
		{
			const UTextBlock* Text = Cast<UTextBlock>(Each);
			bFound = bFound || (Text && Text->GetText().ToString().Contains(Words));
		});
		return bFound;
	}

	const FNoticeBoardPosting* FindPosting(const TArray<FNoticeBoardPosting>& Postings, FName MissionId)
	{
		return Postings.FindByPredicate([MissionId](const FNoticeBoardPosting& Each) { return Each.MissionId == MissionId; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionBoardNoteFocusTest, "Looter.Missions.BoardNote.Focus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionBoardNoteFocusTest::RunTest(const FString& Parameters)
{
	// While the note's job is on offer, the card over the paper takes the key wherever on the paper the player looks, near
	// or far, straight on or from the side; the board's face elsewhere is the board's. Once the job is taken, the paper is
	// read again. A read through the card is a read of the note (it carries the note's tag): Side 1's step counts it.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	const FBoardScene Scene = BuildBoard(World);
	if (!Runner || !Player || !Interaction || !Scene.IsWhole())
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* NoteJob = MakeNoteJob(Scratch);
	// Side 1's step: read the note, by its tag alone (create_side_mission_assets.py).
	UMissionDefinition* ReadNote = NewMission(Scratch, TEXT("TestReadNote"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionInteractObjective>(ReadNote, 0)->Target.ActorTag = AWantedPoster::NoteTag;
	AddObjective<UMissionEventObjective>(ReadNote, 1)->Event = TEXT("Test.Never");
	Runner->BeginForTesting({ NoteJob, MakeBounty(Scratch), ReadNote }, Campaign, Player);
	Scene.Card->DispatchBeginPlay();
	Scene.Note->DispatchBeginPlay();
	Scene.Board->DispatchBeginPlay();
	Runner->Update(0.f);

	TestTrue(TEXT("The card offers the note's job"), Scene.Card->GetOffer() == NoteJob
		&& Scene.Card->GetInteractionOptions(*Interaction).CanUse());
	TestTrue(TEXT("...and reaches farther than the paper under it (the focus weighs distance by reach)"),
		Scene.Card->Reach > Interaction->Reach && Scene.Note->Reach <= 0.f);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Looking at the note: the card has the key, not the paper"), Interaction->GetFocusedActor() == Scene.Card);

	// From all round, near and far, at points all over the paper: the key's line meets the card first (it stands proud of
	// the paper and wider), and the card wins; with no line to meet it (the cone alone) it wins too, from 60 cm out.
	const FVector Out = Facing.Vector();
	const FVector Across = FVector::CrossProduct(FVector::UpVector, Out).GetSafeNormal();
	const FVector CardFront = NoteAt + Out * (CardProud + 0.5);
	int32 Looks = 0;
	int32 CardWon = 0;
	FString Missed;
	for (const double Distance : { 60.0, 120.0, 180.0, 240.0 })
	{
		for (const double Side : { -80.0, -60.0, 0.0, 60.0, 80.0 })
		{
			for (const double Height : { -60.0, -40.0, 0.0, 40.0, 60.0 })
			{
				for (const double U : { -11.0, 0.0, 11.0 })
				{
					for (const double V : { -13.0, 0.0, 13.0 })
					{
						FInteractionView View;
						View.Location = NoteAt + Out * Distance + Across * Side + FVector::UpVector * Height;
						View.ReachOrigin = View.Location;
						View.Direction = (NoteAt + Across * U + FVector::UpVector * V - View.Location).GetSafeNormal();
						const FVector Hit = View.Location + View.Direction
							* (FVector::DotProduct(CardFront - View.Location, Out) / FVector::DotProduct(View.Direction, Out));
						const FVector OnCard = Hit - CardFront;
						const bool bMeetsCard = FMath::Abs(FVector::DotProduct(OnCard, Across)) <= Scene.CardHalf.X
							&& FMath::Abs(OnCard.Z) <= Scene.CardHalf.Y;
						for (const bool bLine : { true, false })
						{
							TArray<FInteractionCandidate> Candidates;
							AddCandidate(Candidates, *Interaction, View, Scene.Card,
								bLine && bMeetsCard ? TOptional<FVector>(Hit) : TOptional<FVector>());
							AddCandidate(Candidates, *Interaction, View, Scene.Note, TOptional<FVector>());
							AddCandidate(Candidates, *Interaction, View, Scene.Board, TOptional<FVector>());
							const int32 Best = InteractionFocus::Select(View, Candidates, Interaction->AimThreshold,
								[](const FInteractionCandidate&) { return true; });
							++Looks;
							if (Candidates.IsValidIndex(Best) && Candidates[Best].Actor == Scene.Card)
							{
								++CardWon;
							}
							else if (Missed.IsEmpty())
							{
								Missed = FString::Printf(TEXT("%s from %.0f out, %.0f aside, %.0f up, at (%.0f, %.0f): %s"),
									bLine ? TEXT("line") : TEXT("cone"), Distance, Side, Height, U, V,
									Candidates.IsValidIndex(Best) ? *Candidates[Best].Actor->GetName() : TEXT("nothing"));
							}
						}
					}
				}
			}
		}
	}
	TestEqual(FString::Printf(TEXT("The card wins every look at the paper (first miss: %s)"), Missed.IsEmpty() ? TEXT("none") : *Missed),
		CardWon, Looks);

	// The board's face away from the note, met by the key's line: the board's (its screen offers the job too).
	{
		FInteractionView View;
		View.Location = InteractionTestWorld::Ahead(0.0);
		View.ReachOrigin = View.Location;
		const FVector OtherHalf = Scene.Board->GetActorTransform().TransformPosition(FVector(9.0, 40.0, 120.0));
		View.Direction = (OtherHalf - View.Location).GetSafeNormal();
		TArray<FInteractionCandidate> Candidates;
		AddCandidate(Candidates, *Interaction, View, Scene.Board, TOptional<FVector>(OtherHalf));
		AddCandidate(Candidates, *Interaction, View, Scene.Card, TOptional<FVector>());
		AddCandidate(Candidates, *Interaction, View, Scene.Note, TOptional<FVector>());
		const int32 Best = InteractionFocus::Select(View, Candidates, Interaction->AimThreshold, [](const FInteractionCandidate&) { return true; });
		TestTrue(TEXT("The board's other half: the board"), Candidates.IsValidIndex(Best) && Candidates[Best].Actor == Scene.Board);
	}

	// Read through the card while it offers: the note is read, as Side 1 asks.
	Runner->NotifyEvent(FMissionEvent::Interaction(Scene.Card, /*bHeld*/ false));
	TestEqual(TEXT("A read through the card counts as reading the note"), Runner->GetStep(TEXT("TestReadNote")), 1);

	// Taken: the card has nothing more to offer, and the paper is read for its words again.
	TestTrue(TEXT("The note's job is taken"), Runner->AcceptMission(NoteJobId));
	TestFalse(TEXT("...the card has nothing more to offer"), Scene.Card->GetInteractionOptions(*Interaction).CanUse());
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("...and the key goes to the paper"), Interaction->GetFocusedActor() == Scene.Note);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionBoardNotePostingTest, "Looter.Missions.BoardNote.Posting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionBoardNotePostingTest::RunTest(const FString& Parameters)
{
	// The board's screen: the note's job is up for the taking here (its card opens from the note) and says where it really
	// is, "Ruth Calder's note, pinned to this board"; the board's own bounty is its own. A note taken off the board leaves
	// the job offered elsewhere.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	const FBoardScene Scene = BuildBoard(World);
	if (!Runner || !Player || !Scene.IsWhole())
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* NoteJob = MakeNoteJob(Scratch);
	UMissionDefinition* Bounty = MakeBounty(Scratch);
	Runner->BeginForTesting({ NoteJob, Bounty }, Campaign, Player);
	Scene.Card->DispatchBeginPlay();
	Scene.Note->DispatchBeginPlay();
	Scene.Board->DispatchBeginPlay();
	Runner->Update(0.f);

	// What lies on the face: the card and the paper; not a step in front of the board, nor beside it.
	const FVector CardPoint = Scene.Card->GetInteractionLocation().Get(Scene.Card->GetActorLocation());
	TestTrue(TEXT("The card lies on the board's face"), Scene.Board->IsOnFace(CardPoint));
	TestTrue(TEXT("...so does the paper"), Scene.Board->IsOnFace(Scene.Note->GetActorLocation()));
	TestFalse(TEXT("A step in front of the board isn't on it"), Scene.Board->IsOnFace(CardPoint + Facing.Vector() * 40.0));
	TestFalse(TEXT("...nor a metre beside it"), Scene.Board->IsOnFace(CardPoint + FVector(0.0, -140.0, 0.0)));
	TestTrue(TEXT("The note's job: picked up from the card pinned here"), Scene.Board->FindPinnedGiver(*NoteJob) == Scene.Card);
	TestNull(TEXT("The board's own bounty: the board is its giver"), Scene.Board->FindPinnedGiver(*Bounty));

	const TArray<FNoticeBoardPosting> Postings = Scene.Board->GatherPostings(*Runner);
	const FNoticeBoardPosting* Note = FindPosting(Postings, NoteJobId);
	const FNoticeBoardPosting* Own = FindPosting(Postings, BountyId);
	TestTrue(TEXT("The note's job is up for the taking here, from the note"), Note && Note->State == ENoticePosting::Offered
		&& Note->bOfferedHere && Note->bTurnedInHere && Note->PinnedGiver.Get() == Scene.Card);
	TestTrue(TEXT("The bounty is the board's own"), Own && Own->bOfferedHere && !Own->PinnedGiver.IsValid());

	UNoticeBoardWidget* Screen = CreateWidget<UNoticeBoardWidget>(World, UNoticeBoardWidget::StaticClass());
	if (!TestNotNull(TEXT("The board's screen"), Screen))
	{
		return false;
	}
	Screen->Open(nullptr, Scene.Board);
	Screen->TakeWidget();
	TestTrue(TEXT("The screen names where the note really is"), ScreenSays(*Screen, TEXT("Ruth Calder's note, pinned to this board")));
	TestFalse(TEXT("...never at the street round it, or elsewhere"), ScreenSays(*Screen, TEXT("OFFERED ELSEWHERE"))
		|| ScreenSays(*Screen, TEXT("Offered elsewhere")) || ScreenSays(*Screen, TEXT("Main Street")));
	Screen->RemoveFromParent();

	// The note taken down and hung a few metres off: the job is offered elsewhere again (still turned in here).
	Scene.Card->SetActorLocation(Scene.Card->GetActorLocation() + FVector(-300.0, 0.0, 0.0));
	TestNull(TEXT("Off the board: nothing pinned"), Scene.Board->FindPinnedGiver(*NoteJob));
	const TArray<FNoticeBoardPosting> After = Scene.Board->GatherPostings(*Runner);
	const FNoticeBoardPosting* Elsewhere = FindPosting(After, NoteJobId);
	TestTrue(TEXT("...the job offered elsewhere, turned in here"), Elsewhere && Elsewhere->State == ENoticePosting::Offered
		&& !Elsewhere->bOfferedHere && Elsewhere->bTurnedInHere && !Elsewhere->PinnedGiver.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionBoardNoteTurnInTest, "Looter.Missions.BoardNote.TurnInLines",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionBoardNoteTurnInTest::RunTest(const FString& Parameters)
{
	// Turned in at the board, the job has its moment as a giver's has: its turn-in words play as captions (they wait under
	// the board's screen in the game). A board turn-in with no words says nothing.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	const FBoardScene Scene = BuildBoard(World);
	if (!Runner || !Captions || !Player || !Scene.IsWhole())
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* NoteJob = MakeNoteJob(Scratch);
	UMissionDefinition* Quiet = MakeNoteJob(Scratch, TEXT("TestQuietJob"));
	Quiet->TurnIn.Lines.Reset();
	Runner->BeginForTesting({ NoteJob, Quiet }, Campaign, Player);
	Scene.Board->DispatchBeginPlay();
	Runner->Update(0.f);

	TestTrue(TEXT("Both taken"), Runner->AcceptMission(NoteJobId) && Runner->AcceptMission(TEXT("TestQuietJob")));
	TestFalse(TEXT("Not ready: nothing to turn in"), Scene.Board->TurnIn(*Runner, NoteJobId));
	Runner->NotifyEvent(FMissionEvent::Named(NoteDone));
	TestTrue(TEXT("Done: ready to turn in at the board"), Runner->IsReadyToTurnIn(NoteJobId) && Runner->IsReadyToTurnIn(TEXT("TestQuietJob")));

	Captions->Clear();
	Captions->Update(1.f);
	TestTrue(TEXT("Turned in at the board"), Scene.Board->TurnIn(*Runner, NoteJobId) && Runner->IsCompleted(NoteJobId));
	const FCaptionEntry* Said = Captions->GetCurrent();
	TestTrue(TEXT("...its words said, the first first"), Said && Said->Line.Text.ToString() == TallyLine
		&& Said->Line.Speaker.ToString() == TEXT("Ellis"));
	TestEqual(TEXT("...and the next waiting"), Captions->GetQueue().Num(), 2);

	Captions->Clear();
	Captions->Update(5.f);
	TestTrue(TEXT("A job with no words turned in"), Scene.Board->TurnIn(*Runner, TEXT("TestQuietJob")));
	TestTrue(TEXT("...says nothing"), Captions->GetCurrent() == nullptr && Captions->GetQueue().IsEmpty());
	return true;
}

#endif
