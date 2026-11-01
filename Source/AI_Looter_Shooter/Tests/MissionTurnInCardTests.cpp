#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/CaptionQueue.h"
#include "Story/CaptionSubsystem.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Story/StoryLine.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "UI/World/MissionOfferWidget.h"
#include "World/MissionArtifact.h"
#include "World/NoticeBoard.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The turn-in card (the user's call, 2026-10-31: every mission turned in at someone, a board or an artifact, "Card, then
// talk"): the card says what was done and what it pays; its Turn In turns the mission in at the giver and only then are
// the giver's words said (and what they offer next waits for them); Not now leaves it ready; a board and an artifact turn in
// through it too; a choice of two keeps its choice card; with no HUD (tests, a headless run) a talk still turns in at once.

using namespace MissionTestWorld;

namespace
{
	const FName DeliaTag(TEXT("Speaker_TestCardDelia"));
	const FName BoardTag(TEXT("NoticeBoard_TestCard"));
	const FName TallyTag(TEXT("Artifact_TestCardTally"));
	const FName DeckEvent(TEXT("Test.Card.Deck"));
	const FName HatEvent(TEXT("Test.Card.Hat"));
	const FName RopeEvent(TEXT("Test.Card.Rope"));
	const FName PorchEvent(TEXT("Test.Card.Porch"));
	const FName KeyEvent(TEXT("Test.Card.Key"));
	const TCHAR* TurnInWords = TEXT("They took him through the orchard.");

	FText Words(const TCHAR* Text)
	{
		return FText::FromString(Text);
	}

	UMissionEventObjective* AddCardEvent(UMissionDefinition* Mission, int32 Step, FName Event, const TCHAR* Text, bool bOptional = false)
	{
		UMissionEventObjective* Objective = AddObjective<UMissionEventObjective>(Mission, Step);
		Objective->Event = Event;
		Objective->Text = Words(Text);
		Objective->bOptional = bOptional;
		return Objective;
	}

	/**
	 * Landfall's shape in small: a fight; the lines cut with a hat to pick up on the way (optional); a scene watched; the porch
	 * held with a spare key to find (optional). Turned in at Tag ("Delia Ransom") for 60 XP and 20 silver, 10 per optional.
	 */
	UMissionDefinition* MakeJob(UObject* Outer, const TCHAR* Id, FName Tag)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, EMissionKind::Main, EMissionStart::Automatic, NAME_None);
		AddCardEvent(Mission, 0, DeckEvent, TEXT("Fight off the boarders"));
		AddCardEvent(Mission, 1, RopeEvent, TEXT("Cut the grapple lines"));
		AddCardEvent(Mission, 1, HatEvent, TEXT("Pick up Abel's hat"), true);
		AddObjective<UMissionSceneObjective>(Mission, 2)->Scene = TEXT("TestCardLanding");
		AddCardEvent(Mission, 3, PorchEvent, TEXT("Help Delia hold the porch"));
		AddCardEvent(Mission, 3, KeyEvent, TEXT("Find the spare key"), true);
		Mission->TurnIn.SpeakerTag = Tag;
		Mission->TurnIn.GiverName = Words(TEXT("Delia Ransom"));
		Mission->TurnIn.Lines = { FStoryLine::Make(FText::GetEmpty(), Words(TurnInWords), 3.f) };
		Mission->Rewards.Experience = 60;
		Mission->Rewards.Silver = 20;
		Mission->Rewards.OptionalSilver = 10;
		return Mission;
	}

	/** One step (an event), turned in at Tag with the given words. */
	UMissionDefinition* MakeErrand(UObject* Outer, const TCHAR* Id, FName Tag, FName Event, const TCHAR* Line)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, EMissionKind::Side, EMissionStart::Automatic, NAME_None);
		AddCardEvent(Mission, 0, Event, TEXT("Do the errand"));
		Mission->TurnIn.SpeakerTag = Tag;
		Mission->TurnIn.Lines = { FStoryLine::Make(FText::GetEmpty(), Words(Line), 3.f) };
		Mission->Rewards.Experience = 10;
		return Mission;
	}

	/** Plays the job to its turn-in: the hat picked up on the way, the spare key not found. */
	void PlayToTurnIn(UMissionRunner& Runner, FName Id)
	{
		Runner.NotifyEvent(FMissionEvent::Named(DeckEvent));
		Runner.NotifyEvent(FMissionEvent::Named(HatEvent));
		Runner.NotifyEvent(FMissionEvent::Named(RopeEvent));
		// The scene: watched (a test level plays none).
		if (Runner.GetStep(Id) == 2)
		{
			Runner.CompleteStep(Id);
		}
		Runner.NotifyEvent(FMissionEvent::Named(PorchEvent));
	}

	ASpeakerPoint* PlaceDelia(UWorld* World)
	{
		ASpeakerPoint* Point = World->SpawnActor<ASpeakerPoint>(InteractionTestWorld::Ahead(200.0), FRotator(0.0, 180.0, 0.0));
		if (!Point)
		{
			return nullptr;
		}
		Point->SpeakerPoint->SetRelativeLocation(FVector::ZeroVector);
		Point->SpeakerPoint->SpeakerName = Words(TEXT("Delia"));
		Point->SpeakerPoint->Lines = { FStoryLine::Make(FText::GetEmpty(), Words(TEXT("Mind the porch.")), 3.f) };
		Point->Tags.Add(DeliaTag);
		Point->DispatchBeginPlay();
		return Point;
	}

	FString OnScreen(const UCaptionSubsystem& Captions)
	{
		const FCaptionEntry* Current = Captions.GetCurrent();
		return Current ? Current->Line.Speaker.ToString() + TEXT("|") + Current->Line.Text.ToString() : FString(TEXT("none"));
	}

	/** The card as the HUD opens it, built (its layout made) and armed (a moment after opening). */
	UMissionOfferWidget* OpenCard(UWorld* World, const UMissionDefinition* Mission, AActor* Giver, bool bOverScreen = false)
	{
		UMissionOfferWidget* Card = CreateWidget<UMissionOfferWidget>(World, UMissionOfferWidget::StaticClass());
		if (Card)
		{
			Card->OpenTurnIn(nullptr, Mission, Giver, bOverScreen);
			Card->TakeWidget();
		}
		return Card;
	}

	void Arm(UMissionOfferWidget& Card)
	{
		Card.Advance(UMissionOfferWidget::ArmSeconds + 0.01f);
	}

	float StampSeconds()
	{
		return UMissionOfferWidget::StampSlamSeconds + UMissionOfferWidget::TurnInStampHoldSeconds + UMissionOfferWidget::StampOutSeconds;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInCardWordsTest, "Looter.Missions.TurnInCard.Card",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInCardWordsTest::RunTest(const FString& Parameters)
{
	// What the card says: the mission done, who to and where, every step's deed checked in order (the scene left out), the
	// optional one done as a bonus and the one still open as open, the rewards with the bonus earned, Turn In and Not now.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	ASpeakerPoint* Delia = PlaceDelia(World);
	if (!Runner || !Player || !Delia)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Job = MakeJob(Scratch, TEXT("TestCardJob"), DeliaTag);
	const FName Id = Job->GetMissionId();
	Runner->BeginForTesting({ Job }, Campaign, Player);
	Runner->Update(0.f);
	PlayToTurnIn(*Runner, Id);
	if (!TestTrue(TEXT("Played to its turn-in"), Runner->IsReadyToTurnIn(Id) && Runner->IsRunning(Id)))
	{
		return false;
	}
	TestTrue(TEXT("A turn-in with no choice has the card"), FMissionOfferCard::HasTurnInCard(*Job));

	const FMissionOfferCard Card = FMissionOfferCard::MakeTurnIn(*Job, Delia, World);
	TestTrue(TEXT("Main: gold"), Card.bMain);
	TestEqual(TEXT("The kind line"), Card.Kind.ToString(), FString(TEXT("Mission complete")));
	TestEqual(TEXT("The title"), Card.Title.ToString(), FString(TEXT("Test TestCardJob")));
	TestTrue(TEXT("Who it's turned in to, by the turn-in's own name"), Card.From.ToString().StartsWith(TEXT("Turn in to Delia Ransom")));
	TestTrue(TEXT("Someone's"), Card.GiverKind == EMissionGiverKind::Someone);
	if (TestEqual(TEXT("Five deeds: the scene isn't one"), Card.Done.Num(), 5))
	{
		TestEqual(TEXT("...in order: the fight"), Card.Done[0].Text, FString(TEXT("Fight off the boarders")));
		TestTrue(TEXT("...done, not optional"), Card.Done[0].bDone && !Card.Done[0].bOptional);
		TestEqual(TEXT("...the lines cut"), Card.Done[1].Text, FString(TEXT("Cut the grapple lines")));
		TestTrue(TEXT("...the hat: a bonus done"), Card.Done[2].Text == TEXT("Pick up Abel's hat") && Card.Done[2].bOptional && Card.Done[2].bDone);
		TestEqual(TEXT("...the porch held"), Card.Done[3].Text, FString(TEXT("Help Delia hold the porch")));
		TestTrue(TEXT("...the spare key: still open"), Card.Done[4].Text == TEXT("Find the spare key") && Card.Done[4].bOptional && !Card.Done[4].bDone);
	}
	TestTrue(TEXT("Something can still be done first"), Card.bOptionalOpen);
	TestTrue(TEXT("The experience"), Card.Rewards.ContainsByPredicate([](const FMissionOfferReward& Each)
	{
		return Each.Label.ToString() == TEXT("Experience") && Each.Value.ToString().Contains(TEXT("60"));
	}));
	TestTrue(TEXT("The silver"), Card.Rewards.ContainsByPredicate([](const FMissionOfferReward& Each)
	{
		return Each.Label.ToString() == TEXT("Silver") && Each.Value.ToString().Contains(TEXT("20"));
	}));
	TestTrue(TEXT("The optional bonus earned"), Card.Rewards.ContainsByPredicate([](const FMissionOfferReward& Each)
	{
		return Each.Label.ToString() == TEXT("Bonus") && Each.Value.ToString().Contains(TEXT("1 optional done"));
	}));
	TestTrue(TEXT("The buttons"), Card.Accept.ToString() == TEXT("Turn in") && Card.Decline.ToString() == TEXT("Not now"));

	// Not every mission has one: an automatic end, or a choice of two (its own card).
	UMissionDefinition* ByItself = NewMission(Scratch, TEXT("TestCardAuto"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	ByItself->TurnIn.bAutomatic = true;
	TestFalse(TEXT("An automatic turn-in has no card"), FMissionOfferCard::HasTurnInCard(*ByItself));
	UMissionDefinition* Choice = MakeJob(Scratch, TEXT("TestCardChoice"), DeliaTag);
	Choice->TurnIn.Choices.AddDefaulted(2);
	TestFalse(TEXT("A choice of two keeps its choice card"), FMissionOfferCard::HasTurnInCard(*Choice));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInCardFirstTest, "Looter.Missions.TurnInCard.CardThenWords",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInCardFirstTest::RunTest(const FString& Parameters)
{
	// Card first: while it's up nothing is turned in and nothing said; Turn In turns it in at Delia, her turn-in words come
	// in her voice, what she offers next waits behind them, and the stamp holds before the card goes.
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
	ASpeakerPoint* Delia = PlaceDelia(World);
	if (!Runner || !Captions || !Player || !Delia)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Job = MakeJob(Scratch, TEXT("TestCardFirst"), DeliaTag);
	// What she has next, offered once the turn-in is made.
	UMissionDefinition* Next = NewMission(Scratch, TEXT("TestCardNext"), EMissionKind::Side, EMissionStart::Offered, NAME_None);
	AddCardEvent(Next, 0, TEXT("Test.Card.Never"), TEXT("Something more"));
	Next->Prerequisites = { Job->GetMissionId() };
	Next->Offer.GiverTag = DeliaTag;
	Next->Offer.Lines = { FStoryLine::Make(FText::GetEmpty(), Words(TEXT("One more thing.")), 3.f) };
	const FName Id = Job->GetMissionId();
	Runner->BeginForTesting({ Job, Next }, Campaign, Player);
	Runner->Update(0.f);
	PlayToTurnIn(*Runner, Id);
	Captions->Update(20.f);
	if (!TestTrue(TEXT("Ready to turn in, nothing said"), Runner->IsReadyToTurnIn(Id) && OnScreen(*Captions) == TEXT("none")))
	{
		return false;
	}

	UMissionOfferWidget* Card = OpenCard(World, Job, Delia);
	if (!TestNotNull(TEXT("The card"), Card))
	{
		return false;
	}
	TestTrue(TEXT("In its turn-in mode"), Card->GetMode() == EMissionCardMode::TurnIn && Card->GetCard().Kind.ToString() == TEXT("Mission complete"));
	TestFalse(TEXT("The key that opened it can't turn it in unread"), Card->TurnIn());
	TestTrue(TEXT("...nothing turned in, nothing said"), Runner->IsReadyToTurnIn(Id) && !Campaign.HasCompleted(Id) && OnScreen(*Captions) == TEXT("none"));

	Arm(*Card);
	TestTrue(TEXT("Turn In"), Card->TurnIn());
	TestTrue(TEXT("...it's turned in"), Campaign.HasCompleted(Id) && !Runner->IsRunning(Id));
	TestEqual(TEXT("...her turn-in words, in her voice"), OnScreen(*Captions), FString::Printf(TEXT("Delia|%s"), TurnInWords));
	TestEqual(TEXT("...what she has next waits for them"), Delia->SpeakerPoint->GetPendingOffer(), Next->GetMissionId());
	TestTrue(TEXT("...the stamp is down and the card on its way"), Card->WasTurnedIn() && Card->IsClosing() && !Card->WasAccepted());
	TestFalse(TEXT("...and it takes no second press"), Card->TurnIn());
	Card->Advance(StampSeconds() * 0.5f);
	TestTrue(TEXT("The stamp holds a while"), Card->IsClosing());
	Card->Advance(StampSeconds());
	TestFalse(TEXT("A turned-in mission isn't the card's to turn in again"), UMissionOfferWidget::TurnInAtGiver(World, *Job, Delia, Player));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInCardNotNowTest, "Looter.Missions.TurnInCard.NotNow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInCardNotNowTest::RunTest(const FString& Parameters)
{
	// Not now: the card goes, the mission stays ready at Delia with nothing said; the next visit's card turns it in.
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
	ASpeakerPoint* Delia = PlaceDelia(World);
	if (!Runner || !Captions || !Player || !Delia)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UMissionDefinition* Job = MakeJob(CreatePackage(nullptr), TEXT("TestCardLater"), DeliaTag);
	const FName Id = Job->GetMissionId();
	Runner->BeginForTesting({ Job }, Campaign, Player);
	Runner->Update(0.f);
	PlayToTurnIn(*Runner, Id);
	Captions->Update(20.f);

	UMissionOfferWidget* Card = OpenCard(World, Job, Delia);
	if (!TestNotNull(TEXT("The card"), Card))
	{
		return false;
	}
	Arm(*Card);
	Card->Decline();
	TestTrue(TEXT("Not now: on its way out, not turned in"), Card->IsClosing() && !Card->WasTurnedIn());
	TestTrue(TEXT("...still ready at Delia"), Runner->IsReadyToTurnIn(Id) && Runner->FindTurnInAt(Delia) == Job && !Campaign.HasCompleted(Id));
	TestEqual(TEXT("...and nothing said"), OnScreen(*Captions), FString(TEXT("none")));
	TestFalse(TEXT("...a press on its way out does nothing"), Card->TurnIn());
	Card->Advance(UMissionOfferWidget::DeclineSeconds + 0.05f);

	// The next visit: the same card (the HUD keeps one), opened afresh.
	Card->OpenTurnIn(nullptr, Job, Delia, false);
	TestFalse(TEXT("Opened afresh, not closing"), Card->IsClosing());
	Arm(*Card);
	TestTrue(TEXT("Turned in on the next visit"), Card->TurnIn() && Campaign.HasCompleted(Id));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInCardBoardTest, "Looter.Missions.TurnInCard.Board",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInCardBoardTest::RunTest(const FString& Parameters)
{
	// At a notice board the card says "Turn in at" the board; its Turn In turns it in there, the board's words as written.
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
	ANoticeBoard* Board = World->SpawnActor<ANoticeBoard>(FVector(600.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!Runner || !Captions || !Player || !Board)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Board->Postings.Reset();
	Board->MainPosting = NAME_None;
	Board->Tags.Add(BoardTag);
	Board->DispatchBeginPlay();
	UMissionDefinition* Bounty = MakeErrand(CreatePackage(nullptr), TEXT("TestCardBounty"), BoardTag, TEXT("Test.Card.Bounty"),
		TEXT("The tags go up on the tally."));
	const FName Id = Bounty->GetMissionId();
	Runner->BeginForTesting({ Bounty }, Campaign, Player);
	Runner->Update(0.f);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Card.Bounty")));
	Captions->Update(20.f);
	if (!TestTrue(TEXT("Ready at the board"), Runner->FindTurnInAt(Board) == Bounty))
	{
		return false;
	}
	const FMissionOfferCard Said = FMissionOfferCard::MakeTurnIn(*Bounty, Board, World);
	TestTrue(TEXT("The board's card"), Said.GiverKind == EMissionGiverKind::Board);
	TestTrue(TEXT("...turned in at the board, by its title"), Said.From.ToString().StartsWith(TEXT("Turn in at Notice Board")));
	TestTrue(TEXT("...a side job: white"), !Said.bMain);

	UMissionOfferWidget* Card = OpenCard(World, Bounty, Board, /*bOverScreen*/ true);
	if (!TestNotNull(TEXT("The card over the board's screen"), Card))
	{
		return false;
	}
	Arm(*Card);
	TestTrue(TEXT("Turned in at the board"), Card->TurnIn() && Campaign.HasCompleted(Id));
	TestEqual(TEXT("...its words as written (a board has no voice)"), OnScreen(*Captions), FString(TEXT("|The tags go up on the tally.")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInCardArtifactTest, "Looter.Missions.TurnInCard.Artifact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInCardArtifactTest::RunTest(const FString& Parameters)
{
	// At a found thing: the card names it ("Turn in at Ruth's tally") and its Turn In turns the mission in there with its
	// words; without a HUD a pick-up turns one in at once.
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
	AMissionArtifact* Tally = World->SpawnActor<AMissionArtifact>(FVector(150.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!Runner || !Captions || !Player || !Tally)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Tally->Tags.Add(TallyTag);
	Tally->Title = FText::FromString(TEXT("Ruth's tally"));
	Tally->bTakeOnAccept = false;
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* First = MakeErrand(Scratch, TEXT("TestCardTagsA"), TallyTag, TEXT("Test.Card.TagsA"), TEXT("Eleven names, scratched off."));
	UMissionDefinition* Second = MakeErrand(Scratch, TEXT("TestCardTagsB"), TallyTag, TEXT("Test.Card.TagsB"), TEXT("Twelve now."));
	Runner->BeginForTesting({ First, Second }, Campaign, Player);
	Tally->DispatchBeginPlay();
	Runner->Update(0.f);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Card.TagsA")));
	Captions->Update(20.f);

	const FMissionOfferCard Said = FMissionOfferCard::MakeTurnIn(*First, Tally, World);
	TestTrue(TEXT("The artifact's card"), Said.GiverKind == EMissionGiverKind::Artifact
		&& Said.From.ToString().StartsWith(TEXT("Turn in at Ruth's tally")));
	UMissionOfferWidget* Card = OpenCard(World, First, Tally);
	if (!TestNotNull(TEXT("The card"), Card))
	{
		return false;
	}
	Arm(*Card);
	TestTrue(TEXT("Turned in at the tally"), Card->TurnIn() && Campaign.HasCompleted(First->GetMissionId()));
	TestEqual(TEXT("...its words as written"), OnScreen(*Captions), FString(TEXT("|Eleven names, scratched off.")));
	TestTrue(TEXT("...the other job isn't done yet, so it's still running"), Runner->IsRunning(Second->GetMissionId()));

	// No HUD to show the card (a test, a headless run): the pick-up turns it in at once, as before the card.
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Card.TagsB")));
	Captions->Update(20.f);
	TestTrue(TEXT("Picked up with no HUD: turned in at once"), Tally->Use(Player) && Campaign.HasCompleted(Second->GetMissionId()));
	TestEqual(TEXT("...with its words"), OnScreen(*Captions), FString(TEXT("|Twelve now.")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInCardChoiceTest, "Looter.Missions.TurnInCard.ChoiceUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInCardChoiceTest::RunTest(const FString& Parameters)
{
	// A turn-in with a choice of two keeps its choice card after the words: the turn-in card never settles it.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	ASpeakerPoint* Delia = PlaceDelia(World);
	if (!Runner || !Player || !Delia)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UMissionDefinition* Choice = MakeErrand(CreatePackage(nullptr), TEXT("TestCardSettle"), DeliaTag, TEXT("Test.Card.Settle"), TEXT("Well?"));
	FMissionTurnInChoice& Pay = Choice->TurnIn.Choices.AddDefaulted_GetRef();
	Pay.Title = Words(TEXT("Pay them"));
	FMissionTurnInChoice& Keep = Choice->TurnIn.Choices.AddDefaulted_GetRef();
	Keep.Title = Words(TEXT("Keep it"));
	const FName Id = Choice->GetMissionId();
	Runner->BeginForTesting({ Choice }, Campaign, Player);
	Runner->Update(0.f);
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.Card.Settle")));

	UMissionOfferWidget* Card = OpenCard(World, Choice, Delia);
	if (!TestNotNull(TEXT("A card"), Card))
	{
		return false;
	}
	Arm(*Card);
	TestFalse(TEXT("The turn-in card never settles a choice"), Card->TurnIn());
	TestTrue(TEXT("...it's still ready"), Runner->IsReadyToTurnIn(Id) && !Campaign.HasCompleted(Id));
	TestTrue(TEXT("Talked to: the words, then the choice card"), Delia->SpeakerPoint->Talk(Player) && Delia->SpeakerPoint->IsChoicePending()
		&& !Campaign.HasCompleted(Id));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionTurnInCardNoHUDTest, "Looter.Missions.TurnInCard.NoHUD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionTurnInCardNoHUDTest::RunTest(const FString& Parameters)
{
	// No HUD for the listener (tests, a headless run): the talk turns the mission in at once with her words, as before.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(World);
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	ASpeakerPoint* Delia = PlaceDelia(World);
	if (!Runner || !Captions || !Player || !Delia)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UMissionDefinition* Job = MakeJob(CreatePackage(nullptr), TEXT("TestCardHeadless"), DeliaTag);
	const FName Id = Job->GetMissionId();
	Runner->BeginForTesting({ Job }, Campaign, Player);
	Runner->Update(0.f);
	PlayToTurnIn(*Runner, Id);
	Captions->Update(20.f);
	TestTrue(TEXT("Talked to with no HUD"), Delia->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("...turned in at once"), Campaign.HasCompleted(Id));
	TestEqual(TEXT("...her words, in her voice"), OnScreen(*Captions), FString::Printf(TEXT("Delia|%s"), TurnInWords));
	return true;
}

#endif
