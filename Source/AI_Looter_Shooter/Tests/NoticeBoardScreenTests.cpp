#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/EconomyRules.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "UI/World/MissionOfferWidget.h"
#include "UI/World/NoticeBoardWidget.h"
#include "World/NoticeBoard.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// A notice board's screen on the real boards (the play-through of 2026-10-14): each posting's reward line says what the job
// pays, the offer card's numbers, and only Skyreach's practice postings say they're practice; a click chooses a card and a
// click on the chosen one (or a double click) acts on it, as the keys do.

using namespace MissionTestWorld;

namespace
{
	const FName BoardTag(TEXT("NoticeBoard_TestScreen"));

	/** A bounty board's job as the real boards post them: offered by the board's tag, one step that waits. */
	UMissionDefinition* MakeJob(UObject* Outer, const TCHAR* Id)
	{
		UMissionDefinition* Job = NewMission(Outer, Id, EMissionKind::Side, EMissionStart::Offered, NAME_None);
		AddObjective<UMissionEventObjective>(Job, 0)->Event = TEXT("Test.Never");
		Job->Offer.GiverTag = BoardTag;
		Job->TurnIn.bAutomatic = true;
		Job->Rewards.Experience = 100;
		return Job;
	}

	int32 IndexOf(const TArray<FNoticeBoardPosting>& Postings, FName MissionId)
	{
		return Postings.IndexOfByPredicate([MissionId](const FNoticeBoardPosting& Each) { return Each.MissionId == MissionId; });
	}

	/** Whether any words on the screen contain Words (as shown: the reward lines are in capitals). */
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNoticeBoardRewardLineTest, "Looter.UI.NoticeBoard.Rewards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNoticeBoardRewardLineTest::RunTest(const FString& Parameters)
{
	UPackage* Scratch = CreatePackage(nullptr);
	const FString Separator = FString(TEXT(" · "));

	// A Rim Rangers' job (Calder's Caches' kind): experience and silver, with the numbers its offer card shows.
	UMissionDefinition* Caches = NewMission(Scratch, TEXT("TestCaches"), EMissionKind::Side, EMissionStart::Offered, NAME_None);
	Caches->Rewards.Experience = 1300;
	Caches->Rewards.Silver = 60;
	const FString XP = FText::AsNumber(1300).ToString();
	const FString Silver = LooterEconomy::Format(60);
	const FString Line = UNoticeBoardWidget::DescribeReward(*Caches).ToString();
	TestEqual(TEXT("A job's reward line: its experience and its silver"), Line,
		FString::Printf(TEXT("Reward: +%s XP%s+%s silver"), *XP, *Separator, *Silver));
	TestFalse(TEXT("...never practice"), Line.Contains(TEXT("practice")));
	const FMissionOfferCard Card = FMissionOfferCard::Make(*Caches, nullptr, nullptr);
	TestTrue(TEXT("...the numbers on its offer card"), Card.Rewards.Num() >= 2 && Card.Rewards[0].Value.ToString().Contains(XP)
		&& Card.Rewards[1].Value.ToString().Contains(Silver));

	// A giver's purse kept by mission id (Amos's wages, Side 2), as its offer card finds it; a share of a level.
	UMissionDefinition* Wages = NewMission(Scratch, TEXT("Side2"), EMissionKind::Side, EMissionStart::Offered, NAME_None);
	Wages->Rewards.ExperienceShare = 0.3f;
	TestEqual(TEXT("A purse by mission id and a share of a level"), UNoticeBoardWidget::DescribeReward(*Wages).ToString(),
		FString::Printf(TEXT("Reward: +30%% of a level%s+%s silver"), *Separator, *LooterEconomy::Format(MissionRewards::SilverFor(TEXT("Side2")))));

	// The Gravemother's posting: experience, no purse (her loot is the reward), and a gun with a floor.
	UMissionDefinition* Fang = NewMission(Scratch, TEXT("TestFang"), EMissionKind::Side, EMissionStart::Offered, NAME_None);
	Fang->Rewards.Experience = 500;
	Fang->Rewards.bGun = true;
	Fang->Rewards.GunRarityFloor = EWeaponRarity::Uncommon;
	TestEqual(TEXT("Experience and a gun with its floor"), UNoticeBoardWidget::DescribeReward(*Fang).ToString(),
		FString::Printf(TEXT("Reward: +%s XP%sa gun, Uncommon or better"), *FText::AsNumber(500).ToString(), *Separator));

	// A story job that pays nothing says nothing; only a practice posting (Skyreach's, outside the story) says it's practice.
	UMissionDefinition* Errand = NewMission(Scratch, TEXT("TestErrand"), EMissionKind::Main, EMissionStart::Offered, NAME_None);
	TestTrue(TEXT("A story job with nothing to give: no line"), UNoticeBoardWidget::DescribeReward(*Errand).IsEmpty());
	UMissionDefinition* Range = NewMission(Scratch, TEXT("TestRange"), EMissionKind::Tutorial, EMissionStart::Automatic, NAME_None);
	TestEqual(TEXT("A practice posting with nothing to give: practice"), UNoticeBoardWidget::DescribeReward(*Range).ToString(),
		FString(TEXT("No reward: it's practice")));
	TestTrue(TEXT("From the rewards alone, not practice: no line"), UNoticeBoardWidget::DescribeReward(FMissionRewards(), NAME_None, false).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNoticeBoardClicksTest, "Looter.UI.NoticeBoard.Clicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FNoticeBoardClicksTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	ANoticeBoard* Board = World->SpawnActor<ANoticeBoard>(FVector(600.0, 0.0, 0.0), FRotator::ZeroRotator);
	if (!Runner || !Player || !Board)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	// A town's board as build_area_board.py places one: its own tag, none of Skyreach's practice postings.
	Board->Postings.Reset();
	Board->MainPosting = NAME_None;
	Board->Tags.Add(BoardTag);
	Board->DispatchBeginPlay();
	UPackage* Scratch = CreatePackage(nullptr);
	const TArray<UMissionDefinition*> Jobs = { MakeJob(Scratch, TEXT("TestJobA")), MakeJob(Scratch, TEXT("TestJobB")),
		MakeJob(Scratch, TEXT("TestJobC")) };
	// Calder's Caches' kind: offered by a note found elsewhere, only turned in at this board (the play-through's card that
	// took no click: it had no button, being nothing this board can act on before it's taken).
	UMissionDefinition* NoteJob = NewMission(Scratch, TEXT("TestNoteJob"), EMissionKind::Side, EMissionStart::Offered, NAME_None);
	AddObjective<UMissionEventObjective>(NoteJob, 0)->Event = TEXT("Test.Never");
	NoteJob->Offer.GiverTag = TEXT("Artifact_TestNote");
	NoteJob->TurnIn.SpeakerTag = BoardTag;
	Runner->BeginForTesting({ Jobs[0], Jobs[1], Jobs[2], NoteJob }, Campaign, Player);
	Runner->Update(0.f);
	for (const UMissionDefinition* Job : Jobs)
	{
		Runner->AcceptMission(Job->GetMissionId());
	}
	// The three jobs under way and none followed: each card can be tracked from the board (acted on without a HUD).
	Runner->TrackMission(NAME_None);

	UNoticeBoardWidget* Screen = CreateWidget<UNoticeBoardWidget>(World, UNoticeBoardWidget::StaticClass());
	if (!TestNotNull(TEXT("The board's screen"), Screen))
	{
		return false;
	}
	Screen->Open(nullptr, Board);
	Screen->TakeWidget();
	if (!TestEqual(TEXT("Three jobs under way and one offered elsewhere on the board"), Screen->GetPostings().Num(), 4))
	{
		return false;
	}
	const FName NoteId = NoteJob->GetMissionId();
	const int32 Note = IndexOf(Screen->GetPostings(), NoteId);
	TestTrue(TEXT("The note's job is pinned up as offered, not here"), Note != INDEX_NONE
		&& Screen->GetPostings()[Note].State == ENoticePosting::Offered && !Screen->GetPostings()[Note].bOfferedHere);
	TestEqual(TEXT("The first job that can be acted on is chosen as it opens"), Screen->GetSelected(), IndexOf(Screen->GetPostings(), Jobs[0]->GetMissionId()));
	TestTrue(TEXT("Each job's card says what it pays"), ScreenSays(*Screen, TEXT("REWARD: +100 XP")));
	TestFalse(TEXT("...and none calls a real job practice"), ScreenSays(*Screen, TEXT("PRACTICE")));
	TestTrue(TEXT("The note's card says it's offered elsewhere"), ScreenSays(*Screen, TEXT("OFFERED ELSEWHERE")));

	// A card that can't be acted on here is chosen by a click all the same; the next click says why, and takes nothing.
	TestTrue(TEXT("A click on the note's card chooses it"), Screen->ClickCard(Note) && Screen->GetSelected() == Note);
	TestFalse(TEXT("...a second click can't take it here"), Screen->ClickCard(Note));
	TestFalse(TEXT("...it isn't taken"), Runner->IsRunning(NoteId));
	TestTrue(TEXT("...and the board says where it's picked up"), ScreenSays(*Screen, TEXT("is picked up elsewhere")));

	// A click on a card not chosen chooses it (as S would), and does nothing more yet.
	const FName ThirdId = Jobs[2]->GetMissionId();
	const int32 Third = IndexOf(Screen->GetPostings(), ThirdId);
	TestTrue(TEXT("A click on another card chooses it"), Screen->ClickCard(Third) && Screen->GetSelected() == Third);
	TestTrue(TEXT("...and acts on nothing yet"), Runner->GetTrackedMission().IsNone());
	// A click on the chosen card acts on it, as E does.
	TestTrue(TEXT("A click on the chosen card acts on it: tracked"), Screen->ClickCard(Third) && Runner->GetTrackedMission() == ThirdId);
	const int32 Tracked = IndexOf(Screen->GetPostings(), ThirdId);
	TestTrue(TEXT("...and the choice moved on to one that can be acted on"), Screen->GetSelected() == IndexOf(Screen->GetPostings(), Jobs[0]->GetMissionId()));
	TestTrue(TEXT("Tracked now, its card is still chosen by a click"), Screen->ClickCard(Tracked) && Screen->GetSelected() == Tracked);
	TestFalse(TEXT("...but can't be acted on again"), Screen->ClickCard(Tracked));
	TestTrue(TEXT("...and the board says why"), ScreenSays(*Screen, TEXT("is tracked already")));

	// A double click (two clicks) on a card not chosen: chosen, then acted on.
	const FName SecondId = Jobs[1]->GetMissionId();
	const int32 Second = IndexOf(Screen->GetPostings(), SecondId);
	TestTrue(TEXT("A double click on a card not chosen: chosen, then tracked"), Screen->ClickCard(Second) && Screen->ClickCard(Second)
		&& Runner->GetTrackedMission() == SecondId);
	TestFalse(TEXT("A card that isn't there: nothing"), Screen->ClickCard(7));
	return true;
}

#endif
