#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bounties/BountyArena.h"
#include "Bounties/BountyBoard.h"
#include "Bounties/BountyBoardSubsystem.h"
#include "Bounties/BountyChest.h"
#include "Bounties/BountyFight.h"
#include "Bounties/BountyRules.h"
#include "Bounties/BountyTuning.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/SpiderCreature.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// The bounty board played in a test level (Docs/Work/2026-10-22-Next/BOUNTY.md): a board, an arena and a test campaign;
// posted, read, taken, its fight springing and pushing, the named mark with its traits, won, its strongbox dropped and
// owed until looted, paid, its slot filling again later, the board rotating, a bounty given up; and its record saved.

namespace
{
	const FName TestArenaId(TEXT("TestArena"));
	const FName TestBoardId(TEXT("TestBoard"));

	/** A spiders' arena at Where: its ground, its spots (well clear of the player's way in), its strongbox's spot. */
	ABountyArena* SpawnArena(UWorld* World, const FVector& Where)
	{
		ABountyArena* Arena = World->SpawnActor<ABountyArena>(Where, FRotator::ZeroRotator);
		if (Arena)
		{
			Arena->ArenaId = TestArenaId;
			Arena->DisplayName = FText::FromString(TEXT("the test ground"));
			Arena->Kinds = { TEXT("Spider") };
			Arena->GiveUpRadius = 5000.f;
			Arena->SpringRadius = 1500.f;
			Arena->PushSpots = { FVector(800.0, 600.0, 0.0), FVector(800.0, -600.0, 0.0), FVector(1100.0, 0.0, 0.0), FVector(600.0, 0.0, 0.0) };
			Arena->FlankSpots = { FVector(0.0, 1200.0, 0.0), FVector(0.0, -1200.0, 0.0) };
			Arena->MarkSpots = { FVector(1300.0, 0.0, 0.0), FVector(1300.0, 500.0, 0.0), FVector(1300.0, -500.0, 0.0) };
			Arena->RewardSpot = FVector(400.0, 0.0, 0.0);
		}
		return Arena;
	}

	/** The board, open from the start, Wanted bounties only (its harder tiers wait on a mission never done). */
	ABountyBoard* SpawnBoard(UWorld* World, const FVector& Where)
	{
		ABountyBoard* Board = World->SpawnActor<ABountyBoard>(Where, FRotator::ZeroRotator);
		if (Board)
		{
			Board->BoardId = TestBoardId;
			Board->OpenWhen = FStoryCondition();
			Board->DangerousWhen = FStoryCondition();
			Board->DangerousWhen.AfterMissions = { TEXT("TestNeverDone") };
			Board->DeadlyWhen = Board->DangerousWhen;
			Board->DispatchBeginPlay();
		}
		return Board;
	}

	const FMission* FindTracked(UWorld* World)
	{
		const UMissionSubsystem* Missions = World->GetSubsystem<UMissionSubsystem>();
		return Missions ? Missions->GetTracked() : nullptr;
	}

	/** Moves a strongbox's drop on for Seconds, a frame at a time (a test level never ticks). */
	void RunDrop(ABountyChest& Chest, float Seconds)
	{
		constexpr float Frame = 1.f / 60.f;
		for (float Done = 0.f; Done < Seconds; Done += Frame)
		{
			Chest.AdvanceDrop(Frame);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBountyPlayTest, "Looter.Bounty.Play", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBountyPlayTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UBountyBoardSubsystem* Bounties = World->GetSubsystem<UBountyBoardSubsystem>();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector::ZeroVector);
	if (!TestNotNull(TEXT("A mission runner"), Runner) || !TestNotNull(TEXT("The bounty board's subsystem"), Bounties)
		|| !TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player);
	Bounties->SetTestClock(0.0);
	ABountyArena* Arena = SpawnArena(World, FVector(4000.0, 0.0, 0.0));
	ABountyBoard* Board = SpawnBoard(World, FVector(0.0, -600.0, 0.0));
	if (!TestNotNull(TEXT("An arena"), Arena) || !TestNotNull(TEXT("A board"), Board) || !TestTrue(TEXT("...the level's"), Bounties->GetBoard() == Board))
	{
		return false;
	}

	// A fresh board posts at once: one arena, so one posting; the other slots wait on more arenas.
	Bounties->Refresh();
	const FBountyBoardRecord* Record = Campaign.Bounties.Find(TestBoardId);
	if (!TestNotNull(TEXT("The board's record, in the campaign"), Record))
	{
		return false;
	}
	TestFalse(TEXT("It has posted"), Record->IsFresh());
	TestEqual(TEXT("Three slots"), Record->Slots.Num(), BountyRules::SlotCount);
	const FBountyPosting First = Record->Slots[0];
	TestTrue(TEXT("A Wanted spider at the arena in the first slot"), First.IsPosting() && First.Arena == TestArenaId
		&& First.Kind == FName(TEXT("Spider")) && First.Tier == EBountyTier::Wanted);
	// One arena open, three slots: the board doubles up there rather than leave cards empty (R2 play-check 2026-10-23).
	TestTrue(TEXT("...the others up at the same arena, the next serials, other names"), Record->Slots[1].IsPosting() && Record->Slots[2].IsPosting()
		&& Record->Slots[1].Arena == TestArenaId && Record->Slots[2].Arena == TestArenaId
		&& Record->Slots[1].Serial == First.Serial + 1 && Record->Slots[2].Serial == First.Serial + 2
		&& Record->Slots[0].NameIndex != Record->Slots[1].NameIndex && Record->Slots[1].NameIndex != Record->Slots[2].NameIndex
		&& Record->Slots[0].NameIndex != Record->Slots[2].NameIndex);
	TestTrue(TEXT("Its sheet is up and the board shows ?"), Board->IsSheetShown(0) && Board->IsSheetShown(1) && Board->IsSheetShown(2)
		&& Board->GetShownMark() == ABountyBoard::EShownMark::Offer);
	const TArray<FBountyCardView> Cards = Bounties->MakeCards();
	TestTrue(TEXT("Its card is new and takeable"), Cards.Num() == 3 && Cards[0].bNew && Cards[0].bCanTake && !Cards[1].bEmpty && !Cards[2].bEmpty);

	// Read: the ? goes.
	Bounties->MarkRead();
	TestTrue(TEXT("Read, the board's ? goes"), Board->GetShownMark() == ABountyBoard::EShownMark::None);

	// Taken: its fight waits at the arena, the bounty is tracked.
	TestTrue(TEXT("Taken"), Bounties->Take(0));
	TestFalse(TEXT("One at a time"), Bounties->Take(1));
	ABountyFight* Fight = Bounties->GetFight();
	if (!TestNotNull(TEXT("Its fight at the arena"), Fight))
	{
		return false;
	}
	TestTrue(TEXT("...waiting for the player"), Fight->GetState() == EEncounterState::Waiting && !Fight->HasSprung());
	TestFalse(TEXT("The taken sheet is gone from the board"), Board->IsSheetShown(0));
	const FMission* Tracked = FindTracked(World);
	TestTrue(TEXT("Tracked, its title the bounty's"), Tracked && Tracked->Title.ToString() == BountyRules::TitleFor(First).ToString()
		&& Tracked->Waypoint.IsSet());

	// The player comes near: it springs, its first push from ahead.
	Player->SetActorLocation(FVector(3000.0, 0.0, 0.0));
	Fight->UpdateEncounter(0.5f);
	TestTrue(TEXT("Sprung: the first push's four"), Fight->HasSprung() && Fight->NumAlive() == 4);
	TestNull(TEXT("...no mark yet"), Fight->GetMark());
	Bounties->Refresh();
	TestTrue(TEXT("The tracker: put it down"), FindTracked(World) && FindTracked(World)->Tracker.Step == 1);

	// Two left: the mark's push comes on top of them, the mark named, its traits as posted.
	TArray<ACreatureBase*> Push = Fight->GetAliveCreatures();
	BossTestWorld::Kill(Push[0]);
	BossTestWorld::Kill(Push[1]);
	Fight->UpdateEncounter(0.5f);
	TestEqual(TEXT("The mark's push came"), Fight->GetWavesStarted(), 2);
	ACreatureBase* Mark = Fight->GetMark();
	if (!TestNotNull(TEXT("The mark is out"), Mark))
	{
		return false;
	}
	TestEqual(TEXT("...named"), Mark->DisplayName.ToString(), BountyRules::MarkName(First.Kind, First.NameIndex).ToString());
	TestTrue(TEXT("...Gravebound, tagged"), Mark->GetRank() == ECreatureRank::Epic && Mark->Tags.Contains(ABountyFight::MarkTag));
	const UCreatureTraitComponent* Traits = UCreatureTraitComponent::Find(Mark);
	bool bTraitsAsPosted = Traits && Traits->GetTraits().Num() == First.MarkTraits.Num();
	for (const ECreatureTrait Trait : First.MarkTraits)
	{
		bTraitsAsPosted &= Traits && Traits->HasTrait(Trait);
	}
	TestTrue(TEXT("...its traits exactly the posting's"), bTraitsAsPosted);

	// The mark down: won (saved), its strongbox owed; the fight goes on with its posse.
	BossTestWorld::Kill(Mark);
	Record = Campaign.Bounties.Find(TestBoardId);
	TestTrue(TEXT("Won as the mark dies"), Record && Record->Slots[0].State == EBountyState::Won);
	TestTrue(TEXT("...its strongbox owed"), Record && Record->OwedChests.Num() == 1 && Record->OwedChests[0].Serial == First.Serial);
	TestNull(TEXT("...not dropped while the fight goes on"), Bounties->GetChest(First.Serial));
	Bounties->Refresh();
	TestTrue(TEXT("The tracker: collect it at the board, open its box"), FindTracked(World) && FindTracked(World)->Tracker.bTurnIn
		&& FindTracked(World)->Tracker.Optionals.Num() == 1);
	TestTrue(TEXT("The board shows !"), Board->GetShownMark() == ABountyBoard::EShownMark::TurnIn);
	TestFalse(TEXT("A won bounty can't be given up"), Bounties->GiveUp());

	// The posse down: the strongbox drops where the fight ended, and opens only once it has landed.
	EncounterTestWorld::KillAll(*Fight);
	Fight->UpdateEncounter(0.5f);
	ABountyChest* Chest = Bounties->GetChest(First.Serial);
	if (!TestNotNull(TEXT("The fight over: its strongbox"), Chest))
	{
		return false;
	}
	TestTrue(TEXT("...dropping in, at the arena's spot"), Chest->IsLanding()
		&& FVector::Dist2D(Chest->GetActorLocation(), Arena->GetRewardLocation()) < 1.0);
	TestTrue(TEXT("...a Strongbox with the bounty's loot"), Chest->Kind == EChestKind::Strongbox && Chest->LootOverride.bOverride
		&& Chest->LootOverride.Guns == FBountyTuning().GunsFor(EBountyTier::Wanted));
	RunDrop(*Chest, 2.f);
	TestFalse(TEXT("...landed"), Chest->IsLanding());

	// Paid at the board: experience and silver; the slot fills again ten minutes of play later.
	FMissionRewardsGiven Given;
	TestTrue(TEXT("Paid"), Bounties->Collect(Player, &Given));
	TestTrue(TEXT("...its experience and purse"), Given.Experience > 0 && Given.Silver == BountyRules::PurseFor(EBountyTier::Wanted, First.LevelHigh, FBountyTuning()));
	Record = Campaign.Bounties.Find(TestBoardId);
	TestTrue(TEXT("...its slot empty, one paid, the box still owed"), Record && !Record->Slots[0].IsPosting() && Record->Paid == 1
		&& Record->OwedChests.Num() == 1);
	TestNull(TEXT("...its fight gone"), Bounties->GetFight());
	// The tracker says it's paid (not its old "Collect it at the board") for a moment, then the entry goes.
	const FMission* PaidLine = FindTracked(World);
	TestTrue(TEXT("...the tracker says it is paid, no arrow"), PaidLine && PaidLine->Tracker.Line == TEXT("Bounty paid") && !PaidLine->Tracker.bTurnIn
		&& !PaidLine->Waypoint.IsSet());
	Bounties->Refresh();
	TestNotNull(TEXT("...still there a moment after"), FindTracked(World));
	const double Refill = FBountyTuning().RefillMinutes * 60.0;
	const int32 NextSerial = Campaign.Bounties.Find(TestBoardId)->NextSerial;
	Bounties->SetTestClock(Refill - 1.0);
	Bounties->Refresh();
	TestNull(TEXT("...gone once its moment is over"), FindTracked(World));
	TestFalse(TEXT("Not refilled before its time"), Campaign.Bounties.Find(TestBoardId)->Slots[0].IsPosting());
	Bounties->SetTestClock(Refill);
	Bounties->Refresh();
	const FBountyPosting Second = Campaign.Bounties.Find(TestBoardId)->Slots[0];
	TestTrue(TEXT("Refilled at its time, the next serial"), Second.IsPosting() && Second.Serial == NextSerial);

	// The strongbox opened: no longer owed.
	Chest->Open(nullptr, /*bForce*/ true);
	for (int32 Frame = 0; Frame < 240; ++Frame)
	{
		Chest->Advance(1.f / 60.f);
	}
	Bounties->Refresh();
	TestTrue(TEXT("Looted: no longer owed"), Chest->HasGivenLoot() && Campaign.Bounties.Find(TestBoardId)->OwedChests.IsEmpty());

	// Given up: back on the board, its fight gone.
	TestTrue(TEXT("The new one taken"), Bounties->Take(0) && Bounties->GetFight() != nullptr);
	TestTrue(TEXT("...given up"), Bounties->GiveUp());
	TestTrue(TEXT("...posted again, its fight gone"), Campaign.Bounties.Find(TestBoardId)->Slots[0].State == EBountyState::Posted
		&& Bounties->GetFight() == nullptr);

	// The rotation: two hours of play on, the untaken posting comes down and a new one goes up.
	Bounties->SetTestClock(Refill + FBountyTuning().RotateMinutes * 60.0 + 1.0);
	Bounties->Refresh();
	const FBountyBoardRecord* Rotated = Campaign.Bounties.Find(TestBoardId);
	TestTrue(TEXT("Rotated: a new posting"), Rotated && Rotated->Rotation == 2 && Rotated->Slots[0].IsPosting()
		&& Rotated->Slots[0].Serial > Second.Serial);

	// A swift posse runs Fleet; a mark with a twist has its trait.
	FBountyPosting Swift;
	Swift.Kind = TEXT("Spider");
	Swift.Tier = EBountyTier::Wanted;
	Swift.Arena = TestArenaId;
	Swift.Twists = { EBountyTwist::Warded, EBountyTwist::SwiftPosse };
	TestTrue(TEXT("A chosen posting put up"), Bounties->PostChosen(0, Swift) && Bounties->Take(0));
	Fight = Bounties->GetFight();
	if (TestNotNull(TEXT("...its fight"), Fight))
	{
		Fight->UpdateEncounter(0.5f);
		bool bAllFleet = Fight->NumAlive() > 0;
		for (const ACreatureBase* Each : Fight->GetAliveCreatures())
		{
			const UCreatureTraitComponent* Its = UCreatureTraitComponent::Find(Each);
			bAllFleet &= Its && Its->HasTrait(ECreatureTrait::Fleet);
		}
		TestTrue(TEXT("...its posse Fleet"), bAllFleet);
	}
	TestTrue(TEXT("Given up again"), Bounties->GiveUp());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBountySaveTest, "Looter.Bounty.Save", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBountySaveTest::RunTest(const FString& Parameters)
{
	// The board's record rides in the campaign record: through a session save and back, every field kept.
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	FBountyBoardRecord& Written = Save->Campaign.Bounties.FindOrAdd(TEXT("RansomsRest"));
	Written.Slots.SetNum(BountyRules::SlotCount);
	Written.RefillAt = { 0.0, 1234.5, 0.0 };
	FBountyPosting& Posting = Written.Slots[0];
	Posting.Serial = 12;
	Posting.Kind = TEXT("DeadMiner");
	Posting.Tier = EBountyTier::Deadly;
	Posting.Arena = TEXT("OldQuarry");
	Posting.LevelLow = 5;
	Posting.LevelHigh = 6;
	Posting.NameIndex = 3;
	Posting.Twists = { EBountyTwist::Vengeful, EBountyTwist::BigPosse };
	Posting.MarkTraits = { ECreatureTrait::Vengeful, ECreatureTrait::Phasing, ECreatureTrait::Warded, ECreatureTrait::Mending };
	Posting.State = EBountyState::Won;
	Posting.PostedAt = 3600.0;
	Written.NextSerial = 13;
	Written.Rotation = 4;
	Written.RotatedAt = 7200.0;
	Written.SeenUpTo = 11;
	Written.Paid = 6;
	FBountyChestOwed& Owed = Written.OwedChests.AddDefaulted_GetRef();
	Owed.Serial = 12;
	Owed.Arena = TEXT("OldQuarry");
	Owed.Tier = EBountyTier::Deadly;

	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("Written"), UGameplayStatics::SaveGameToMemory(Save, Bytes)))
	{
		return false;
	}
	const ULooterSessionSave* Read = USessionSubsystem::ReadSave(Bytes);
	const FBountyBoardRecord* Back = Read ? Read->Campaign.Bounties.Find(TEXT("RansomsRest")) : nullptr;
	if (!TestNotNull(TEXT("Read back, the board's record in it"), Back))
	{
		return false;
	}
	const FBountyPosting& Got = Back->Slots[0];
	TestTrue(TEXT("The posting whole"), Got.Serial == 12 && Got.Kind == Posting.Kind && Got.Tier == EBountyTier::Deadly && Got.Arena == Posting.Arena
		&& Got.LevelLow == 5 && Got.LevelHigh == 6 && Got.NameIndex == 3 && Got.Twists == Posting.Twists && Got.MarkTraits == Posting.MarkTraits
		&& Got.State == EBountyState::Won && FMath::IsNearlyEqual(Got.PostedAt, 3600.0));
	TestTrue(TEXT("The board's clock and counts"), Back->NextSerial == 13 && Back->Rotation == 4 && FMath::IsNearlyEqual(Back->RotatedAt, 7200.0)
		&& Back->SeenUpTo == 11 && Back->Paid == 6 && Back->RefillAt.Num() == 3 && FMath::IsNearlyEqual(Back->RefillAt[1], 1234.5));
	TestTrue(TEXT("The strongbox owed"), Back->OwedChests.Num() == 1 && Back->OwedChests[0].Serial == 12 && Back->OwedChests[0].Tier == EBountyTier::Deadly);
	TestEqual(TEXT("The taken or won one found"), Back->FindActiveSlot(), 0);

	// A session from before the board has none: a fresh board.
	FCampaignRecord Old;
	TestTrue(TEXT("An older campaign: no board yet"), Old.Bounties.IsEmpty() && FBountyBoardRecord().IsFresh());
	return true;
}

#endif
