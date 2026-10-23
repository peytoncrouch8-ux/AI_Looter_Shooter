#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "UI/Inventory/MapPins.h"
#include "UI/World/GiverMarkActor.h"
#include "UI/World/GiverMarkSubsystem.h"
#include "UI/World/MissionOfferWidget.h"
#include "World/MissionArtifact.h"
#include "World/NoticeBoard.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// What the player sees of missions picked up from their givers: the map's offer and turn-in pins, the marks over the
// givers and their chime, the minimap's share of markers, a bounty board's postings, and the offer card's words. The
// words below say ! for an offer and ? for a turn-in, as first built; since the user's call (2026-10-13) the glyphs are
// the other way round (MissionMarkIcons): the tests check the kinds, which didn't change.

using namespace MissionTestWorld;

namespace
{
	const FName WrenTag(TEXT("Speaker_Wren"));
	const FName TillyTag(TEXT("Speaker_Tilly"));
	const FName BoardTag(TEXT("NoticeBoard_Test"));
	const FName ArtifactTag(TEXT("Artifact_TestTag"));
	const FName FirstEvent(TEXT("Test.First"));
	const FName SecondEvent(TEXT("Test.Second"));

	UMissionDefinition* MakeOffered(UObject* Outer, const TCHAR* Id, EMissionKind Kind, FName Giver)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, Kind, EMissionStart::Offered, NAME_None);
		AddObjective<UMissionEventObjective>(Mission, 0)->Event = FirstEvent;
		AddObjective<UMissionEventObjective>(Mission, 1)->Event = SecondEvent;
		Mission->Offer.GiverTag = Giver;
		Mission->TurnIn.bAutomatic = true;
		return Mission;
	}

	/** A main mission that starts by itself and waits to be turned in to Tilly. */
	UMissionDefinition* MakeErrand(UObject* Outer, const TCHAR* Id)
	{
		UMissionDefinition* Mission = NewMission(Outer, Id, EMissionKind::Main, EMissionStart::Automatic, NAME_None);
		AddObjective<UMissionEventObjective>(Mission, 0)->Event = FirstEvent;
		Mission->TurnIn.SpeakerTag = TillyTag;
		Mission->TurnIn.GiverName = FText::FromString(TEXT("Tilly"));
		return Mission;
	}

	const FGiverMark* MarkOn(const TArray<FGiverMark>& Marks, const AActor* Giver)
	{
		return Marks.FindByPredicate([Giver](const FGiverMark& Each) { return Each.Giver.Get() == Giver; });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGiverMapPinsTest, "Looter.Missions.Givers.MapPins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGiverMapPinsTest::RunTest(const FString& Parameters)
{
	// The map page: a ! at each giver with a mission to offer (one pin a giver, the story's gold), a ? where one waits to
	// be turned in; nothing for what isn't offered yet.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(1500.0, 0.0, 0.0), WrenTag);
	AActor* Tilly = SpawnMarker(World, FVector(-1500.0, 800.0, 0.0), TillyTag);
	if (!Runner || !Player || !Wren || !Tilly)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Story = MakeOffered(Scratch, TEXT("TestStory"), EMissionKind::Main, WrenTag);
	Story->Offer.GiverName = FText::FromString(TEXT("Wren"));
	// A main mission is never offered: one waiting for its giver to be met is the one that still shows at her.
	Story->Offer.bMeetFirst = true;
	UMissionDefinition* Chore = MakeOffered(Scratch, TEXT("TestChore"), EMissionKind::Side, WrenTag);
	UMissionDefinition* Locked = MakeOffered(Scratch, TEXT("TestLocked"), EMissionKind::Side, TillyTag);
	Locked->Prerequisites = { FName(TEXT("TestNever")) };
	UMissionDefinition* Errand = MakeErrand(Scratch, TEXT("TestErrand"));
	Runner->BeginForTesting({ Story, Chore, Locked, Errand }, Campaign, Player);
	Runner->Update(0.f);
	Runner->CompleteStep(TEXT("TestErrand"));

	FMapPinSources Sources;
	Sources.World = World;
	Sources.Campaign = &Campaign;
	Sources.Runner = Runner;
	const TArray<FMapPin> Pins = MapPins::Gather(Sources);
	TestEqual(TEXT("One ! pin: Wren's two offers share it"), MapPins::Count(Pins, EMapPinKind::Offer), 1);
	const FMapPin* Offer = Pins.FindByPredicate([](const FMapPin& Pin) { return Pin.Kind == EMapPinKind::Offer; });
	if (TestNotNull(TEXT("Wren's pin"), Offer))
	{
		TestTrue(TEXT("...where she stands"), Offer->Location.Equals(Wren->GetActorLocation(), 1.0));
		TestTrue(TEXT("...the story's mission on it, gold"), Offer->Id == FName(TEXT("TestStory")) && Offer->bMain);
		TestEqual(TEXT("...who and how many"), Offer->Detail.ToString(), FString(TEXT("Offered by Wren, and 1 more")));
	}
	const FMapPin* TurnIn = Pins.FindByPredicate([](const FMapPin& Pin) { return Pin.Kind == EMapPinKind::TurnIn; });
	TestTrue(TEXT("A ? at Tilly for the errand waiting for her, gold"), TurnIn && TurnIn->Location.Equals(Tilly->GetActorLocation(), 1.0)
		&& TurnIn->bMain);
	TestFalse(TEXT("The kind has a legend name"), MapPins::KindName(EMapPinKind::Offer).IsEmpty());

	// The story taken: the pin shows the side job, white; both taken, no ! at all.
	Runner->AcceptMission(TEXT("TestStory"));
	const TArray<FMapPin> AfterStory = MapPins::Gather(Sources);
	const FMapPin* Side = AfterStory.FindByPredicate([](const FMapPin& Pin) { return Pin.Kind == EMapPinKind::Offer; });
	TestTrue(TEXT("Story taken: the side job's white ! alone"), Side && Side->Id == FName(TEXT("TestChore")) && !Side->bMain
		&& !Side->Detail.ToString().Contains(TEXT("more")));
	Runner->AcceptMission(TEXT("TestChore"));
	TestEqual(TEXT("Both taken: no ! on the map"), MapPins::Count(MapPins::Gather(Sources), EMapPinKind::Offer), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGiverMarksTest, "Looter.Missions.Givers.Marks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGiverMarksTest::RunTest(const FString& Parameters)
{
	// The marks over the givers: ! for an offer (gold over white), ? over ! where something waits to be turned in, none
	// over a hidden giver or one with nothing. The chime: once per mission newly offered within 40 m, not for what was
	// offered nearby as the level began, and not for a giver far off until the player comes near.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UGiverMarkSubsystem* Marks = UGiverMarkSubsystem::Get(World);
	UInteractionSubsystem* Registry = UInteractionSubsystem::Get(World);
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	AActor* Wren = SpawnMarker(World, FVector(800.0, 0.0, 0.0), WrenTag);
	AActor* Tilly = SpawnMarker(World, FVector(-800.0, 0.0, 0.0), TillyTag);
	AActor* Faraway = SpawnMarker(World, FVector(9000.0, 0.0, 0.0), TEXT("Speaker_Faraway"));
	if (!Runner || !Marks || !Registry || !Player || !Wren || !Tilly || !Faraway)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	// Givers are things the player uses: they're among the level's interactables.
	for (AActor* Giver : { Wren, Tilly, Faraway })
	{
		Registry->Register(Giver);
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Story = MakeOffered(Scratch, TEXT("TestStory"), EMissionKind::Main, WrenTag);
	// A main mission is never offered: one waiting for its giver to be met still has her gold mark.
	Story->Offer.bMeetFirst = true;
	UMissionDefinition* Chore = MakeOffered(Scratch, TEXT("TestChore"), EMissionKind::Side, WrenTag);
	UMissionDefinition* TillyJob = MakeOffered(Scratch, TEXT("TestTillyJob"), EMissionKind::Side, TillyTag);
	UMissionDefinition* Errand = MakeErrand(Scratch, TEXT("TestErrand"));
	UMissionDefinition* Later = MakeOffered(Scratch, TEXT("TestLater"), EMissionKind::Side, WrenTag);
	Later->Prerequisites = { FName(TEXT("TestErrand")) };
	UMissionDefinition* FarJob = MakeOffered(Scratch, TEXT("TestFarJob"), EMissionKind::Side, TEXT("Speaker_Faraway"));
	UMissionDefinition* AfterChore = MakeOffered(Scratch, TEXT("TestAfterChore"), EMissionKind::Side, WrenTag);
	AfterChore->Prerequisites = { FName(TEXT("TestChore")) };
	Runner->BeginForTesting({ Story, Chore, TillyJob, Errand, Later, FarJob, AfterChore }, Campaign, Player);
	Runner->Update(0.f);

	// As the level begins: what's offered nearby is known already.
	Marks->Advance(UGiverMarkSubsystem::RefreshInterval);
	TestEqual(TEXT("No chime for what was there as the level began"), Marks->GetChimeCount(), 0);
	TestTrue(TEXT("...though it counts as seen"), Marks->HasChimed(TEXT("TestStory")) && Marks->HasChimed(TEXT("TestTillyJob")));
	TestFalse(TEXT("...not the job far off"), Marks->HasChimed(TEXT("TestFarJob")));

	const FGiverMark* OverWren = MarkOn(Marks->GetMarks(), Wren);
	TestTrue(TEXT("A gold ! over Wren for the story"), OverWren && OverWren->Mark == MissionMarkIcons::EMark::Offer && OverWren->bMain
		&& OverWren->MissionId == FName(TEXT("TestStory")) && OverWren->Offers.Num() == 2);
	TestTrue(TEXT("...floating over her"), OverWren && OverWren->Location.Z > Wren->GetActorLocation().Z);

	// Tilly: something waits to be turned in to her, and she has a job: the ? comes first.
	Runner->CompleteStep(TEXT("TestErrand"));
	Marks->Refresh();
	const FGiverMark* OverTilly = MarkOn(Marks->GetMarks(), Tilly);
	TestTrue(TEXT("A ? over Tilly, over her !"), OverTilly && OverTilly->Mark == MissionMarkIcons::EMark::TurnIn && OverTilly->bMain
		&& OverTilly->MissionId == FName(TEXT("TestErrand")));

	// The turn-in opens a new job at Wren, near: one chime as its ! is seen, never twice.
	Marks->Advance(UGiverMarkSubsystem::SettleSeconds);
	Runner->NotifyEvent(FMissionEvent::Named(FMissionEvent::Talk, nullptr, TillyTag));
	TestTrue(TEXT("Turned in: the new job is offered"), Runner->IsAvailable(TEXT("TestLater")));
	Marks->Advance(UGiverMarkSubsystem::RefreshInterval);
	TestEqual(TEXT("...and it chimes once"), Marks->GetChimeCount(), 1);
	Marks->Advance(UGiverMarkSubsystem::RefreshInterval);
	TestEqual(TEXT("...only once"), Marks->GetChimeCount(), 1);
	const FGiverMark* TillyAfter = MarkOn(Marks->GetMarks(), Tilly);
	TestTrue(TEXT("Tilly's ? gone: her job's white ! now"), TillyAfter && TillyAfter->Mark == MissionMarkIcons::EMark::Offer && !TillyAfter->bMain);

	// Far off, nothing chimes; coming near, it does.
	TestFalse(TEXT("The far job hasn't chimed"), Marks->HasChimed(TEXT("TestFarJob")));
	Player->SetActorLocation(FVector(8000.0, 0.0, 0.0));
	Marks->Advance(UGiverMarkSubsystem::RefreshInterval);
	TestTrue(TEXT("Near it now: it chimes"), Marks->HasChimed(TEXT("TestFarJob")) && Marks->GetChimeCount() == 2);

	// Under a menu or a scene a new offer waits to chime until it can be seen.
	Player->SetActorLocation(FVector::ZeroVector);
	Marks->SetLabelsShown(false);
	Runner->CompleteMission(TEXT("TestChore"));
	TestTrue(TEXT("The chore done: the next job offered"), Runner->IsAvailable(TEXT("TestAfterChore")));
	Marks->Advance(UGiverMarkSubsystem::RefreshInterval);
	TestTrue(TEXT("A menu up: no chime yet"), Marks->GetChimeCount() == 2 && !Marks->HasChimed(TEXT("TestAfterChore")));
	Marks->SetLabelsShown(true);
	Marks->Advance(UGiverMarkSubsystem::RefreshInterval);
	TestTrue(TEXT("...it chimes once the ! can be seen"), Marks->GetChimeCount() == 3 && Marks->HasChimed(TEXT("TestAfterChore")));

	// Hidden or empty-handed: no mark.
	Wren->SetActorHiddenInGame(true);
	Marks->Refresh();
	TestNull(TEXT("No mark over a hidden giver"), MarkOn(Marks->GetMarks(), Wren));
	TestNull(TEXT("None over the player"), MarkOn(Marks->GetMarks(), Player));

	// How a mark over the world eases with distance, and pops in.
	TestTrue(TEXT("Whole near, gone past 40 m"), FMath::IsNearlyEqual(AGiverMarkActor::AlphaAt(500.f), 1.f)
		&& FMath::IsNearlyEqual(AGiverMarkActor::AlphaAt(4100.f), 0.f) && AGiverMarkActor::AlphaAt(3700.f) > 0.f && AGiverMarkActor::AlphaAt(3700.f) < 1.f);
	TestTrue(TEXT("Smaller far off"), AGiverMarkActor::ScaleAt(3900.f) < AGiverMarkActor::ScaleAt(500.f));
	TestTrue(TEXT("Pops past its size, then settles"), AGiverMarkActor::PopAt(0.f) < 0.1f && AGiverMarkActor::PopAt(0.24f) > 1.f
		&& FMath::IsNearlyEqual(AGiverMarkActor::PopAt(1.f), 1.f));

	// The minimap: the hostiles first, then the givers, then the loot.
	const FMinimapMarkerSplit Split = UHudMinimapWidget::SplitMarkers(10, 4, 3, 5, 5);
	TestTrue(TEXT("Minimap: hostiles, givers, then guns"), Split.Hostiles == 4 && Split.Givers == 3 && Split.Guns == 3 && Split.Ammo == 0);
	const FMinimapMarkerSplit Crowded = UHudMinimapWidget::SplitMarkers(4, 6, 2, 1, 1);
	TestTrue(TEXT("...creatures before givers when the pool is short"), Crowded.Hostiles == 4 && Crowded.Givers == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGiverBoardTest, "Looter.Missions.Givers.Board",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGiverBoardTest::RunTest(const FString& Parameters)
{
	// Any board is a bounty board: the jobs its tags offer, up for the taking first; taken, under way; what isn't offered
	// yet stays off it.
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
	UMissionDefinition* Bounty = MakeOffered(Scratch, TEXT("TestBounty"), EMissionKind::Side, BoardTag);
	UMissionDefinition* Running = MakeOffered(Scratch, TEXT("TestRunning"), EMissionKind::Side, BoardTag);
	UMissionDefinition* Locked = MakeOffered(Scratch, TEXT("TestLocked"), EMissionKind::Side, BoardTag);
	Locked->Prerequisites = { FName(TEXT("TestNever")) };
	UMissionDefinition* Elsewhere = MakeOffered(Scratch, TEXT("TestElsewhere"), EMissionKind::Side, WrenTag);
	Runner->BeginForTesting({ Running, Bounty, Locked, Elsewhere }, Campaign, Player);
	Runner->Update(0.f);
	Runner->AcceptMission(TEXT("TestRunning"));

	TArray<FNoticeBoardPosting> Postings = Board->GatherPostings(*Runner);
	TestEqual(TEXT("Two postings: the job up for the taking and the one under way"), Postings.Num(), 2);
	TestTrue(TEXT("...the job to take first, offered here"), Postings.Num() == 2 && Postings[0].MissionId == FName(TEXT("TestBounty"))
		&& Postings[0].State == ENoticePosting::Offered && Postings[0].bOfferedHere);
	TestTrue(TEXT("...then the one under way"), Postings.Num() == 2 && Postings[1].State == ENoticePosting::Open);
	TestTrue(TEXT("The board offers it"), Board->Offers(*Bounty) && !Board->Offers(*Elsewhere) && Runner->GetOffersAt(Board).Contains(Bounty));

	Runner->AcceptMission(TEXT("TestBounty"));
	Postings = Board->GatherPostings(*Runner);
	const FNoticeBoardPosting* Taken = Postings.FindByPredicate([](const FNoticeBoardPosting& Each) { return Each.MissionId == FName(TEXT("TestBounty")); });
	TestTrue(TEXT("Taken: under way on the board"), Taken && Taken->State == ENoticePosting::Open);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionGiverCardTest, "Looter.Missions.Givers.Card",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMissionGiverCardTest::RunTest(const FString& Parameters)
{
	// The offer card's words: the kind, the title, who offers it (someone, a board, a found thing), the story, the first
	// step's objectives only, the rewards, and the two choices; and where to pick it up for the Missions page.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AActor* Wren = SpawnMarker(World, FVector(600.0, 0.0, 0.0), WrenTag);
	ANoticeBoard* Board = World->SpawnActor<ANoticeBoard>(FVector(-600.0, 0.0, 0.0), FRotator::ZeroRotator);
	AMissionArtifact* Artifact = World->SpawnActor<AMissionArtifact>(FVector(0.0, 600.0, 0.0), FRotator::ZeroRotator);
	if (!Wren || !Board || !Artifact)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* Story = MakeOffered(Scratch, TEXT("TestStory"), EMissionKind::Main, WrenTag);
	Story->Title = FText::FromString(TEXT("The Well"));
	Story->Offer.GiverName = FText::FromString(TEXT("Wren"));
	Story->Offer.Briefing = FText::FromString(TEXT("The well's gone foul. Find out why."));
	Story->Steps[0].Objectives[0]->Text = FText::FromString(TEXT("Go down the well."));
	AddObjective<UMissionEventObjective>(Story, 0)->Text = FText::FromString(TEXT("Bring a lamp."));
	Story->Rewards.Experience = 25;
	Story->Rewards.bGun = true;
	Story->Rewards.GunRarityFloor = EWeaponRarity::Rare;
	Story->Rewards.UnlockAreas = { FName(TEXT("TestArea")) };

	const FMissionOfferCard Card = FMissionOfferCard::Make(*Story, Wren, World);
	TestTrue(TEXT("The story's: gold, its kind"), Card.bMain && Card.Kind.ToString() == TEXT("Main mission"));
	TestEqual(TEXT("Its title"), Card.Title.ToString(), FString(TEXT("The Well")));
	TestTrue(TEXT("From someone"), Card.GiverKind == EMissionGiverKind::Someone && Card.From.ToString() == TEXT("From Wren"));
	TestEqual(TEXT("The story"), Card.Briefing.ToString(), FString(TEXT("The well's gone foul. Find out why.")));
	TestFalse(TEXT("...said, not a found page"), Card.bDocument);
	TestTrue(TEXT("The first step's objectives only"), Card.Objectives.Num() == 2 && Card.Objectives[0] == TEXT("Go down the well.")
		&& Card.Objectives[1] == TEXT("Bring a lamp."));
	TestEqual(TEXT("...with one more step after"), Card.MoreSteps, 1);
	const FMissionOfferReward* XP = Card.Rewards.FindByPredicate([](const FMissionOfferReward& Each) { return Each.Label.ToString() == TEXT("Experience"); });
	TestTrue(TEXT("Rewards: the experience"), XP && XP->Value.ToString() == TEXT("+25 XP"));
	const FMissionOfferReward* Gun = Card.Rewards.FindByPredicate([](const FMissionOfferReward& Each) { return Each.Label.ToString() == TEXT("Gun"); });
	TestTrue(TEXT("...the gun and its rarity"), Gun && Gun->Value.ToString() == TEXT("Rare or better"));
	TestTrue(TEXT("...the area opened"), Card.Rewards.ContainsByPredicate([](const FMissionOfferReward& Each) { return Each.Label.ToString() == TEXT("Opens"); }));
	TestTrue(TEXT("The choices"), Card.Accept.ToString() == TEXT("Accept") && Card.Decline.ToString() == TEXT("Not now"));

	// A board's posting: posted on the board, by its title.
	UMissionDefinition* Bounty = MakeOffered(Scratch, TEXT("TestBounty"), EMissionKind::Side, BoardTag);
	const FMissionOfferCard BoardCard = FMissionOfferCard::Make(*Bounty, Board, World);
	TestTrue(TEXT("A side job"), !BoardCard.bMain && BoardCard.Kind.ToString() == TEXT("Side mission"));
	TestTrue(TEXT("Posted on the board"), BoardCard.GiverKind == EMissionGiverKind::Board && BoardCard.From.ToString() == TEXT("Posted on Notice Board"));
	TestTrue(TEXT("No briefing: what it's about"), BoardCard.Briefing.EqualTo(Bounty->Summary));

	// A found thing: its own words, as a page.
	UMissionDefinition* Found = MakeOffered(Scratch, TEXT("TestFound"), EMissionKind::Side, ArtifactTag);
	Found->Offer.Briefing = FText::FromString(TEXT("The offer's words."));
	Artifact->Title = FText::FromString(TEXT("A miner's tag"));
	Artifact->Text = FText::FromString(TEXT("E. Shaw, No. 4 shaft. Tell my girl."));
	const FMissionOfferCard FoundCard = FMissionOfferCard::Make(*Found, Artifact, World);
	TestTrue(TEXT("Found: the thing"), FoundCard.GiverKind == EMissionGiverKind::Artifact && FoundCard.From.ToString() == TEXT("Found: A miner's tag"));
	TestTrue(TEXT("...its own words, as a page"), FoundCard.bDocument && FoundCard.Briefing.ToString() == TEXT("E. Shaw, No. 4 shaft. Tell my girl."));

	// Where to pick them up, for the Missions page (a test level has no names on its map).
	TestEqual(TEXT("Here: offered by Wren"), FMissionOfferCard::WhereToGet(*Story, Wren, World, FText::GetEmpty()).ToString(),
		FString(TEXT("Offered by Wren")));
	TestEqual(TEXT("In another area: named with it"), FMissionOfferCard::WhereToGet(*Story, nullptr, World, FText::FromString(TEXT("Far Away"))).ToString(),
		FString(TEXT("Offered by Wren, in Far Away")));
	TestTrue(TEXT("A board's, from its tag when it isn't here"), FMissionOfferCard::GiverKindOf(*Bounty, nullptr) == EMissionGiverKind::Board);
	TestTrue(TEXT("An artifact's, from its tag"), FMissionOfferCard::GiverKindOf(*Found, nullptr) == EMissionGiverKind::Artifact);
	TestEqual(TEXT("A giver named by its tag alone"), Bounty->GetOfferGiverName().ToString(), FString(TEXT("Test")));
	return true;
}

#endif
