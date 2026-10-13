#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The vigil stones (World/BossShrine; the user's 2026-10-12 call: "a pedestal with a menu that spawns next to the boss arenas
// after you kill the boss the first time so you can respawn the boss. 5 minute wait, or the boss respawns after a save and
// quit"): the stone appears only after its boss's first defeat, the five-minute rule, the Save & Quit rule (and a lair's
// boss back as the level loads), the call back starting the fight again with no story replayed (Abel: no second "Sit with
// Pa"), and the session keeping it all through a save.

#include "Bosses/AbelKeeper.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossTestSpider.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSpawner.h"
#include "Creatures/GravemotherCreature.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/SitWithPa.h"
#include "Session/BossRecallRecord.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Story/AbelOnBoard.h"
#include "Tests/AbelTestWorld.h"
#include "Tests/BossTestWorld.h"
#include "Tests/EncounterTestWorld.h"
#include "World/BossShrine.h"
#include "World/BossShrineRules.h"
#include "World/KeeperLanternPost.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

namespace
{
	constexpr float Frame = 0.05f;

	/** A vigil stone at Where, set up by Setup before its play begins (as a build script sets it), begun. */
	ABossShrine* SpawnShrine(UWorld* World, const FVector& Where, TFunctionRef<void(ABossShrine&)> Setup)
	{
		ABossShrine* Stone = World->SpawnActor<ABossShrine>(Where, FRotator::ZeroRotator);
		if (!Stone)
		{
			return nullptr;
		}
		Setup(*Stone);
		Stone->DispatchBeginPlay();
		return Stone;
	}

	/** Moves the stone on by Seconds, a frame at a time (a test level doesn't tick). */
	void RunShrine(ABossShrine* Stone, float Seconds)
	{
		for (float Done = 0.f; Stone && Done < Seconds - KINDA_SMALL_NUMBER; Done += Frame)
		{
			Stone->Advance(Frame);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossShrineAppearsTest, "Looter.Bosses.Shrine.Appears",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossShrineAppearsTest::RunTest(const FString& Parameters)
{
	// Until its boss is first beaten the stone isn't there: hidden, nothing to meet. Beaten, it rises once the body has
	// fallen, with the player near (out of the ground, then standing); a stone whose boss the story already counts beaten
	// (a save from before the stones) rises the first time the player comes near, and one far from the player waits.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(600.0, 0.0, 0.0));
	UBossComponent* Boss = BossTestSpider::Spawn(World, FVector::ZeroVector, 0.f);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Test boss"), Boss))
	{
		return false;
	}
	BossTestWorld::NoLoot(Boss->GetOwner());
	ABossShrine* Stone = SpawnShrine(World, FVector(0.0, 1500.0, 0.0), [Boss](ABossShrine& Setup)
	{
		Setup.BossKey = TEXT("TestBoss");
		Setup.CallKind = EBossShrineCall::Placed;
		Setup.PlacedBoss = Boss->GetCreature();
		Setup.BossClass = Boss->GetCreature()->GetClass();
	});
	if (!TestNotNull(TEXT("Its vigil stone"), Stone))
	{
		return false;
	}
	Stone->TestClock.PlayedSeconds = 100.0;
	Stone->Refresh();
	TestTrue(TEXT("Never beaten: the stone isn't there"), Stone->GetState() == EBossShrineState::Dormant && !Stone->IsRisen()
		&& Stone->IsHidden() && !Stone->GetActorEnableCollision());
	TestFalse(TEXT("...nor does it know its boss as beaten"), Stone->HasBossBeenBeaten());

	FEditorScriptExecutionGuard RunActorEvents;
	Boss->StartFight(Player);
	BossTestWorld::Kill(Boss->GetOwner());
	TestTrue(TEXT("Its boss beaten: the stone knows"), Boss->IsWon() && Stone->HasBossBeenBeaten());
	TestFalse(TEXT("...but doesn't rise before the body has fallen"), Stone->IsRisen());
	RunShrine(Stone, BossShrineRules::RiseDelay + 0.3f);
	TestTrue(TEXT("A few seconds on with the player near: it rises out of the ground"), Stone->IsRisen() && Stone->IsRising()
		&& !Stone->IsHidden());
	RunShrine(Stone, Stone->RiseSeconds + 0.5f);
	TestTrue(TEXT("...and stands, solid"), Stone->IsRisen() && !Stone->IsRising() && Stone->GetActorEnableCollision());
	TestTrue(TEXT("Beaten a moment ago: the call back waits"), Stone->GetState() == EBossShrineState::Waiting);
	TestFalse(TEXT("...its candles out"), Stone->AreCandlesLit());

	// Beaten before the stones kept count: the campaign's beaten bosses say so.
	UMissionRunner* Runner = UMissionRunner::Get(World);
	if (TestNotNull(TEXT("The test level's mission runner"), Runner))
	{
		Runner->GetCampaign().DefeatedBosses.AddUnique(TEXT("TestOldBoss"));
		ABossShrine* Old = SpawnShrine(World, FVector(1000.0, 0.0, 0.0), [](ABossShrine& Setup)
		{
			Setup.BossKey = TEXT("TestOldBoss");
			Setup.CallKind = EBossShrineCall::Placed;
		});
		ABossShrine* Far = SpawnShrine(World, FVector(90000.0, 0.0, 0.0), [](ABossShrine& Setup)
		{
			Setup.BossKey = TEXT("TestOldBoss");
			Setup.CallKind = EBossShrineCall::Placed;
		});
		if (TestTrue(TEXT("Two stones for a boss beaten before the stones"), Old && Far))
		{
			TestTrue(TEXT("The story counts it beaten: the near stone rises at once (its death was before this visit)"),
				Old->HasBossBeenBeaten() && Old->IsRisen());
			TestTrue(TEXT("...ready to call (no kept death to wait on)"), Old->GetState() == EBossShrineState::Ready);
			TestFalse(TEXT("A stone 900 m from the player waits for them to come near"), Far->IsRisen());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossShrineFiveMinutesTest, "Looter.Bosses.Shrine.FiveMinutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossShrineFiveMinutesTest::RunTest(const FString& Parameters)
{
	// Five minutes of play after its last death, its boss can be called back, and not before: the countdown (rounded up to
	// the second) and the button refusing until then; never while it's out there, nor while the story holds it.
	TestEqual(TEXT("Five minutes"), BossShrineRules::CallBackSeconds, 300.0);
	BossShrineRules::FInputs In;
	In.bEverBeaten = true;
	In.DefeatedAt = 1000.0;
	In.PlayedNow = 1000.0 + 299.5;
	BossShrineRules::FVerdict Verdict = BossShrineRules::Evaluate(In);
	TestTrue(TEXT("4:59.5 on: waiting, half a second left"), Verdict.State == EBossShrineState::Waiting
		&& FMath::IsNearlyEqual(Verdict.SecondsLeft, 0.5, 0.01));
	In.PlayedNow = 1300.0;
	TestTrue(TEXT("5:00 on: ready"), BossShrineRules::Evaluate(In).State == EBossShrineState::Ready);
	In.PlayedNow = 900.0;
	TestTrue(TEXT("Beaten \"after\" now (an odd save): ready rather than waiting it out"),
		BossShrineRules::Evaluate(In).State == EBossShrineState::Ready);
	In.PlayedNow = 1300.0;
	In.bBossPresent = true;
	TestTrue(TEXT("Out there now: nothing to call"), BossShrineRules::Evaluate(In).State == EBossShrineState::Present);
	In.bBossPresent = false;
	In.bStoryAllows = false;
	TestTrue(TEXT("The story holds it: not yet"), BossShrineRules::Evaluate(In).State == EBossShrineState::NotYet);
	In.bEverBeaten = false;
	TestTrue(TEXT("Never beaten: no stone"), BossShrineRules::Evaluate(In).State == EBossShrineState::Dormant);
	TestEqual(TEXT("The countdown: 5:00"), BossShrineRules::Countdown(300.0).ToString(), FString(TEXT("5:00")));
	TestEqual(TEXT("...4:10 with 249.2 s left (rounded up)"), BossShrineRules::Countdown(249.2).ToString(), FString(TEXT("4:10")));
	TestEqual(TEXT("...0:01 at the end"), BossShrineRules::Countdown(0.3).ToString(), FString(TEXT("0:01")));

	// In a level: the stone refuses while it waits, and calls its boss back once five minutes have passed.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(600.0, 0.0, 0.0));
	UBossComponent* Boss = BossTestSpider::Spawn(World, FVector::ZeroVector, 0.f);
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Test boss"), Boss))
	{
		return false;
	}
	ABossShrine* Stone = SpawnShrine(World, FVector(0.0, 1500.0, 0.0), [Boss](ABossShrine& Setup)
	{
		Setup.BossKey = TEXT("TestBoss");
		Setup.CallKind = EBossShrineCall::Placed;
		Setup.PlacedBoss = Boss->GetCreature();
	});
	if (!TestNotNull(TEXT("Its vigil stone"), Stone))
	{
		return false;
	}
	Stone->TestClock.PlayedSeconds = 600.0;
	FEditorScriptExecutionGuard RunActorEvents;
	Boss->StartFight(Player);
	BossTestWorld::Kill(Boss->GetOwner());
	Stone->Rise(/*bShow*/ false);
	Stone->TestClock.PlayedSeconds = 600.0 + 120.0;
	Stone->Refresh();
	const FBossShrineView Waiting = Stone->GetView();
	TestTrue(TEXT("Two minutes on: the menu counts down 3:00"), Waiting.State == EBossShrineState::Waiting
		&& BossShrineRules::Countdown(Waiting.SecondsLeft).ToString() == TEXT("3:00") && !Waiting.CanCall());
	TestTrue(TEXT("...its last kill and how many times it was beaten"), !Waiting.LastKill.IsEmpty() && Waiting.TimesBeaten == 1);
	TestFalse(TEXT("The button refuses while it waits"), Stone->CallBack(Player));
	TestTrue(TEXT("...and its boss stays beaten"), Boss->GetCreature()->IsDead() && Boss->IsWon());
	Stone->TestClock.PlayedSeconds = 600.0 + 300.0;
	Stone->Refresh();
	TestTrue(TEXT("Five minutes on: ready, the candles lit"), Stone->GetState() == EBossShrineState::Ready && Stone->AreCandlesLit());
	TestTrue(TEXT("Called back"), Stone->CallBack(Player));
	TestTrue(TEXT("...alive at its spot, its fight unwon and waiting, a rematch"), !Boss->GetCreature()->IsDead() && !Boss->IsWon()
		&& !Boss->IsFighting() && Boss->IsRematch()
		&& FVector::Dist(Boss->GetCreature()->GetActorLocation(), Boss->GetCreature()->GetHome().GetLocation()) < 1.0);
	TestTrue(TEXT("...and the stone says it's out there"), Stone->GetState() == EBossShrineState::Present);
	TestFalse(TEXT("Out there: the button refuses"), Stone->CallBack(Player));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossShrineSaveAndQuitTest, "Looter.Bosses.Shrine.SaveAndQuit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossShrineSaveAndQuitTest::RunTest(const FString& Parameters)
{
	// "Or the boss respawns after a save and quit": a death in an earlier load of the session is ready at once; a lair's
	// boss away for this visit comes back as a level loaded from the session begins after one, but not on a trip's arrival.
	BossShrineRules::FInputs In;
	In.bEverBeaten = true;
	In.DefeatedAt = 500.0;
	In.PlayedNow = 510.0;
	In.LoadsAtDefeat = 3;
	In.LoadsNow = 3;
	TestTrue(TEXT("10 s on, the same load: waiting 4:50"), BossShrineRules::Evaluate(In).State == EBossShrineState::Waiting
		&& FMath::IsNearlyEqual(BossShrineRules::Evaluate(In).SecondsLeft, 290.0, 0.01));
	In.LoadsNow = 4;
	TestTrue(TEXT("Save & Quit, then Continue, 10 s of play on: ready at once"), BossShrineRules::Evaluate(In).State == EBossShrineState::Ready);
	TestTrue(TEXT("A lair's boss away this visit after a Save & Quit: back"), BossShrineRules::ComesBackOnLoad(true, 500.0, 3, 4));
	TestFalse(TEXT("...not on a trip's arrival in the same load"), BossShrineRules::ComesBackOnLoad(true, 500.0, 3, 3));
	TestFalse(TEXT("...nothing to do when it isn't away"), BossShrineRules::ComesBackOnLoad(false, 500.0, 3, 4));
	TestTrue(TEXT("...and a death the stone never kept (a save from before the stones) counts as before the load"),
		BossShrineRules::ComesBackOnLoad(true, -1.0, 0, 0));

	// In a level: the Gravemother's lair, her death kept by her stone, her lair away (as the next arrival within 20 minutes
	// has it); a trip's arrival leaves her away, a load after a Save & Quit brings her back as a called-back fight.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(2000.0, 0.0, 0.0));
	AEncounterSpawner* Lair = EncounterTestWorld::SpawnSpawner(World, FVector::ZeroVector, [](AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("TestShrineLair");
		Setup.LegendaryId = TEXT("TestShrineMother");
		Setup.Groups = { EncounterTestWorld::MakeGroup(AGravemotherCreature::StaticClass(), 1, ECreatureRank::Legendary) };
		Setup.bSpawnOnApproach = false;
		Setup.GiveUpRadius = 3600.f;
	});
	ABossShrine* Stone = Lair ? SpawnShrine(World, FVector(4500.0, 0.0, 0.0), [Lair](ABossShrine& Setup)
	{
		Setup.BossKey = TEXT("TestShrineMother");
		Setup.BossClass = AGravemotherCreature::StaticClass();
		Setup.CallKind = EBossShrineCall::Lair;
		Setup.Lair = Lair;
	}) : nullptr;
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Her lair"), Lair) || !TestNotNull(TEXT("Her stone"), Stone))
	{
		return false;
	}
	Stone->TestClock.PlayedSeconds = 100.0;
	Stone->TestClock.Loads = 0;
	TestTrue(TEXT("Her lair brings her"), Lair->TriggerWave(/*bForce*/ true) && Lair->NumAlive() == 1);
	EncounterTestWorld::KillAll(*Lair);
	Lair->UpdateEncounter(0.5f);
	TestTrue(TEXT("Killed: her stone kept it"), Stone->HasBossBeenBeaten() && Lair->GetState() == EEncounterState::Cleared);
	Lair->SendLegendaryAway();
	TestFalse(TEXT("A trip's arrival in the same load: she stays away"), Stone->BringBackAfterLoad());
	TestEqual(TEXT("...nothing in her den"), Lair->NumAlive(), 0);
	Stone->TestClock.Loads = 1;
	TestTrue(TEXT("Loaded again after a Save & Quit: she's back"), Stone->BringBackAfterLoad());
	const TArray<ACreatureBase*> Back = Lair->GetAliveCreatures();
	const UBossComponent* HerFight = Back.Num() == 1 ? Back[0]->FindComponentByClass<UBossComponent>() : nullptr;
	TestTrue(TEXT("...the Gravemother, as a called-back fight (a little less luck)"), Back.Num() == 1 && Back[0]->IsA<AGravemotherCreature>()
		&& HerFight && HerFight->IsRematch());
	Stone->Refresh();
	TestTrue(TEXT("...and her stone says she's out there"), Stone->GetState() == EBossShrineState::Present);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossShrineRematchTest, "Looter.Bosses.Shrine.Rematch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossShrineRematchTest::RunTest(const FString& Parameters)
{
	// Abel, called back: the Keeper walks the boards again while Pa's board stands empty, unhurt until his fight starts as it
	// first did (a player on the deck); beaten again he kneels, and is back on his board before "Sit with Pa" could play a
	// second time; Main 6's story (his FightWhen) back as it was.
	AbelTestWorld::FScenesSetting Scenes(2);
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	TArray<AKeeperLanternPost*> Posts;
	AAbelOnBoard* OnBoard = nullptr;
	AAbelKeeper* Abel = AbelTestWorld::SpawnAbel(World, false, &Posts, &OnBoard);
	USceneSubsystem* Played = World->GetSubsystem<USceneSubsystem>();
	ACharacter* Player = nullptr;
	UBossComponent* Boss = Abel && Played ? AbelTestWorld::StartFight(*this, World, Abel, Player) : nullptr;
	if (!Boss || !OnBoard)
	{
		return false;
	}
	ABossShrine* Stone = SpawnShrine(World, FVector(-2600.0, 0.0, 0.0), [Abel](ABossShrine& Setup)
	{
		Setup.BossKey = AAbelKeeper::BossId;
		Setup.BossClass = AAbelKeeper::StaticClass();
		Setup.CallKind = EBossShrineCall::Placed;
		Setup.PlacedBoss = Abel;
	});
	if (!TestNotNull(TEXT("His vigil stone"), Stone))
	{
		return false;
	}
	Stone->TestClock.PlayedSeconds = 0.0;

	// The first fight: won, the scene plays (skipped), Pa on his board.
	FEditorScriptExecutionGuard RunActorEvents;
	AbelTestWorld::Run(Abel, 3.f);
	BossTestWorld::Kill(Abel);
	AbelTestWorld::Run(Abel, 2.5f);
	TestTrue(TEXT("First defeat: Sit with Pa plays"), Played->IsPlaying() && Played->GetPlayingName() == SitWithPa::SceneName());
	Played->SkipScene();
	TestTrue(TEXT("...and Pa sits on his board"), Abel->IsEndingDone() && OnBoard->IsShown() && !Abel->IsPresent());
	RunShrine(Stone, BossShrineRules::RiseDelay + Stone->RiseSeconds + 1.f);
	TestTrue(TEXT("The scene over, his stone stands outside the gate"), Stone->IsRisen() && !Stone->IsRising());

	// Five minutes on, called back, with Main 6's own FightWhen on him (kept to its fight step).
	FStoryCondition MainSix;
	MainSix.DuringMission = AAbelKeeper::Mission;
	MainSix.FromStep = AAbelKeeper::FightStep;
	Abel->FightWhen = MainSix;
	Stone->TestClock.PlayedSeconds = 301.0;
	Stone->Refresh();
	TestTrue(TEXT("Five minutes on: ready"), Stone->GetState() == EBossShrineState::Ready);
	TestTrue(TEXT("Called back"), Stone->CallBack(Player));
	UHealthComponent* Health = Abel->FindComponentByClass<UHealthComponent>();
	TestTrue(TEXT("The Keeper walks the boards again, alive, his fight unwon, a rematch"), Abel->IsPresent() && !Abel->IsDead()
		&& !Abel->IsEndingDone() && !Boss->IsWon() && Boss->IsRematch());
	TestFalse(TEXT("Pa's board stands empty meanwhile"), OnBoard->IsShown());
	TestTrue(TEXT("Unhurt until his fight starts"), Health && Health->bInvulnerable);
	TestTrue(TEXT("His fight starts as it first did: a player on the deck"), FMath::IsNearlyEqual(Boss->EngageRadius, Abel->DeckRadius));
	TestTrue(TEXT("The stone says he's out there"), Stone->GetState() == EBossShrineState::Present);
	Boss->StartFight(Player);
	TestTrue(TEXT("His fight again: his first phase, hurtable"), Boss->IsFighting() && Boss->GetPhase() == 0 && Health && !Health->bInvulnerable);

	// Beaten again: no scene, Pa back on his board, the Keeper's Lantern lit, the story as it was.
	AbelTestWorld::Run(Abel, 1.f);
	BossTestWorld::Kill(Abel);
	TestTrue(TEXT("Beaten again: he kneels"), Boss->IsWon() && Abel->GetMove() == EAbelMove::Kneel);
	bool bScenePlayed = false;
	for (float Time = 0.f; Time < 4.f; Time += Frame)
	{
		Abel->GetBoss()->TickFight(Frame);
		Abel->Tick(Frame);
		Stone->Advance(Frame);
		bScenePlayed |= Played->IsPlaying();
	}
	TestFalse(TEXT("Sit with Pa never plays again"), bScenePlayed);
	TestTrue(TEXT("Pa back on his board, the Keeper gone from the boards"), OnBoard->IsShown() && Abel->IsEndingDone() && !Abel->IsPresent()
		&& Abel->IsHidden());
	AKeeperLanternPost* Keepers = Abel->GetKeepersPost();
	TestTrue(TEXT("The Keeper's Lantern lit on its post"), Keepers && Keepers->IsKeepersLanternLit());
	TestTrue(TEXT("Main 6's FightWhen back on him"), Abel->FightWhen.DuringMission == AAbelKeeper::Mission
		&& Abel->FightWhen.FromStep == AAbelKeeper::FightStep);
	TestTrue(TEXT("His stone waits five minutes again"), Stone->GetState() == EBossShrineState::Waiting);

	// A boss of any other kind (the test boss): readied at its spot, its phases from the first again. Four phases, hurt only
	// into the second (Old Hunger, 75%): at half health it goes Under Silk, untargetable, and the killing blow wouldn't land.
	ACharacter* Other = BossTestWorld::SpawnPlayer(World, FVector(600.0, 5000.0, 0.0));
	UBossComponent* Spider = BossTestSpider::Spawn(World, FVector(0.0, 5000.0, 0.0), 0.f, 4);
	if (TestNotNull(TEXT("Test boss"), Spider) && Other)
	{
		BossTestWorld::NoLoot(Spider->GetOwner());
		Spider->StartFight(Other);
		BossTestWorld::HurtTo(*Spider, 0.6f);
		TestFalse(TEXT("...hurtable when it's killed"), Spider->IsUntargetable());
		const int32 PhaseReached = Spider->GetPhase();
		BossTestWorld::Kill(Spider->GetOwner());
		TestTrue(TEXT("The test boss readied again"), Spider->PrepareRematch() && !Spider->IsWon() && !Spider->GetCreature()->IsDead());
		Spider->StartFight(Other);
		TestTrue(TEXT("...its fight from its first phase again"), PhaseReached > 0 && Spider->IsFighting() && Spider->GetPhase() == 0);
		TestFalse(TEXT("...and a fight under way can't be readied"), Spider->PrepareRematch());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBossShrineSaveTest, "Looter.Bosses.Shrine.SaveRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBossShrineSaveTest::RunTest(const FString& Parameters)
{
	// The session keeps each stone's boss (its last death in time played and on the clock, the load it was in, how many
	// times, its level, whether its stone has risen) and how many times it was loaded, through the save format and back; a
	// save without them reads as none kept and never loaded.
	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->SessionLoads = 3;
	FBossRecallRecord& Kept = Save->BossRecalls.Add(TEXT("Gravemother"));
	Kept.DefeatedAt = 1234.5;
	Kept.DefeatedOn = FDateTime(2026, 10, 12, 14, 32, 0);
	Kept.LoadsAtDefeat = 2;
	Kept.Defeats = 4;
	Kept.Level = 7;
	Kept.bRisen = true;
	TArray<uint8> Bytes;
	const ULooterSessionSave* ReadBack = UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	if (!TestNotNull(TEXT("The session read back"), ReadBack))
	{
		return false;
	}
	const FBossRecallRecord* Back = ReadBack->BossRecalls.Find(TEXT("Gravemother"));
	TestTrue(TEXT("Its loads kept"), ReadBack->SessionLoads == 3);
	TestTrue(TEXT("Her record kept, every field"), Back && FMath::IsNearlyEqual(Back->DefeatedAt, 1234.5) && Back->DefeatedOn == Kept.DefeatedOn
		&& Back->LoadsAtDefeat == 2 && Back->Defeats == 4 && Back->Level == 7 && Back->bRisen);
	BossShrineRules::FInputs In;
	In.bEverBeaten = Back != nullptr;
	In.DefeatedAt = Back ? Back->DefeatedAt : -1.0;
	In.PlayedNow = 1240.0;
	In.LoadsAtDefeat = Back ? Back->LoadsAtDefeat : 0;
	In.LoadsNow = ReadBack->SessionLoads + 1;
	TestTrue(TEXT("Continued (one more load) 6 s of play after her death: ready"), BossShrineRules::Evaluate(In).State == EBossShrineState::Ready);

	ULooterSessionSave* Plain = NewObject<ULooterSessionSave>();
	Plain->Version = ULooterSessionSave::CurrentVersion;
	TArray<uint8> PlainBytes;
	const ULooterSessionSave* PlainBack = UGameplayStatics::SaveGameToMemory(Plain, PlainBytes) ? USessionSubsystem::ReadSave(PlainBytes) : nullptr;
	TestTrue(TEXT("A save without them: none kept, never loaded"), PlainBack && PlainBack->BossRecalls.IsEmpty() && PlainBack->SessionLoads == 0);
	const FBossRecallRecord Fresh;
	TestTrue(TEXT("A record never kept has no death"), !Fresh.HasDefeat() && !Fresh.bRisen);
	return true;
}

#endif
