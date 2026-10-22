#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/BossLootShower.h"
#include "Creatures/SetPieceRules.h"
#include "Creatures/SetPieceSpawner.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Tests/EncounterTestWorld.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// The set pieces (ASetPieceSpawner; FIGHT-LAYOUT, 2026-10-21: the user's few hand-made fights an area, announced and
// rewarded, once a session): their rules without a level, then one played through in a test level (its pushes from their
// own sides, a placed creature, the speakers it shuts, its flag and its shower).

namespace
{
	FEncounterGroup WaveGroup(TSubclassOf<ACreatureBase> Class, int32 Count, int32 Wave)
	{
		FEncounterGroup Made = EncounterTestWorld::MakeGroup(Class, Count);
		Made.FirstWave = Wave;
		Made.LastWave = Wave;
		return Made;
	}

	/** A set piece at Where, set up by Setup before its play begins, begun, with a fixed seed. */
	ASetPieceSpawner* SpawnSetPiece(UWorld* World, const FVector& Where, TFunctionRef<void(ASetPieceSpawner&)> Setup)
	{
		ASetPieceSpawner* Made = World->SpawnActor<ASetPieceSpawner>(Where, FRotator::ZeroRotator);
		if (!Made)
		{
			return nullptr;
		}
		Setup(*Made);
		Made->DispatchBeginPlay();
		Made->SetRandomSeed(20261021);
		return Made;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSetPieceRulesTest, "Looter.SetPiece.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSetPieceRulesTest::RunTest(const FString& Parameters)
{
	using namespace SetPieceRules;
	// A push comes on top of the last one's stragglers, only while it's fighting and a wave is left.
	TestTrue(TEXT("Three left of the first push, three to push at: the second comes"), IsPushDue(EEncounterState::Engaged, 1, 2, 3, 3));
	TestFalse(TEXT("Four left: not yet"), IsPushDue(EEncounterState::Engaged, 1, 2, 4, 3));
	TestFalse(TEXT("Every push come: none more"), IsPushDue(EEncounterState::Engaged, 2, 2, 0, 3));
	TestFalse(TEXT("Not sprung yet: the first is the spring's, not a push"), IsPushDue(EEncounterState::Waiting, 0, 2, 0, 3));
	TestFalse(TEXT("Taken away while the player is far: no push"), IsPushDue(EEncounterState::Waiting, 1, 2, 2, 3));
	TestTrue(TEXT("All of the last down at once: the next comes"), IsPushDue(EEncounterState::Engaged, 1, 3, 0, 0));
	TestEqual(TEXT("Its own push threshold..."), PushThreshold(3, -1), 3);
	TestEqual(TEXT("...or the tuning's when it sets one"), PushThreshold(3, 1), 1);
	TestEqual(TEXT("A negative threshold is none"), PushThreshold(-2, -1), 0);

	// Done: every wave's creatures down and every placed one; a lair cleared by its monster being away isn't a fight.
	TestTrue(TEXT("Cleared, its placed ones down: done"), IsDone(EEncounterState::Cleared, 2, 0, 0));
	TestFalse(TEXT("A placed one still up: not done"), IsDone(EEncounterState::Cleared, 2, 1, 0));
	TestFalse(TEXT("A placed one owed: not done"), IsDone(EEncounterState::Cleared, 2, 0, 1));
	TestFalse(TEXT("Cleared with no wave begun: not a fight"), IsDone(EEncounterState::Cleared, 0, 0, 0));
	TestFalse(TEXT("Still fighting: not done"), IsDone(EEncounterState::Engaged, 2, 0, 0));

	// The speakers it shuts: from its spring until it's done or its story ends.
	TestTrue(TEXT("Begun and on: hushed"), KeepsHushed(EEncounterState::Engaged, 1, false));
	TestTrue(TEXT("...and while the player's away and it waits"), KeepsHushed(EEncounterState::Waiting, 1, false));
	TestFalse(TEXT("Not begun: they talk"), KeepsHushed(EEncounterState::Waiting, 0, false));
	TestFalse(TEXT("Done: they talk"), KeepsHushed(EEncounterState::Cleared, 2, true));
	TestFalse(TEXT("Its story ended: they talk"), KeepsHushed(EEncounterState::Off, 1, false));

	// Held off while a spawner it waits on fights (no two fights stacked in the drift).
	const TArray<EEncounterState> NoneWaited;
	const TArray<EEncounterState> Quiet = { EEncounterState::Cleared, EEncounterState::Waiting, EEncounterState::Off };
	const TArray<EEncounterState> Fighting = { EEncounterState::Waiting, EEncounterState::Engaged };
	TestFalse(TEXT("No spawners waited on: free"), IsHeld(NoneWaited));
	TestFalse(TEXT("The waited one cleared or waiting: free"), IsHeld(Quiet));
	TestTrue(TEXT("The waited one fighting: held"), IsHeld(Fighting));

	// Beats by wave, shakes and dust as played.
	const TArray<int32> BeatWaves = { 1, 2 };
	TestEqual(TEXT("The second push's beat"), FindBeat(BeatWaves, 2), 1);
	TestEqual(TEXT("A push without one"), FindBeat(BeatWaves, 3), static_cast<int32>(INDEX_NONE));
	TestNearlyEqual(TEXT("A shake at the tuning's scale"), ShakeStrength(0.4f, 1.5f), 0.6f, 0.001f);
	TestNearlyEqual(TEXT("...never past full"), ShakeStrength(0.8f, 2.f), 1.f, 0.001f);
	TestNearlyEqual(TEXT("...none at scale 0"), ShakeStrength(0.8f, 0.f), 0.f, 0.001f);
	TestEqual(TEXT("Dust at the tuning's scale"), DustPuffs(10, 1.5f), 15);
	TestEqual(TEXT("...at most a beat's most"), DustPuffs(40, 1.f), MaxDustPuffs);
	TestEqual(TEXT("The reward's level is the fight's"), RewardLevel(6), 6);
	TestEqual(TEXT("...never under 1"), RewardLevel(0), 1);

	// The cvars' defaults are the tuning's own (a fresh game and the tests agree).
	const FSetPieceTuning Defaults;
	const FSetPieceTuning Now = FSetPieceTuning::Current();
	TestTrue(TEXT("The cvars start at the tuning's defaults"), Now.PushWhenLeft == Defaults.PushWhenLeft && Now.RewardGuns == Defaults.RewardGuns
		&& FMath::IsNearlyEqual(Now.ShakeScale, Defaults.ShakeScale) && Now.RewardAmmo == Defaults.RewardAmmo);
	TestTrue(TEXT("A reward is a Strongbox's worth: two guns at its luck"), Defaults.RewardGuns == 2 && FMath::IsNearlyEqual(Defaults.RewardLuck, 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSetPiecePlayTest, "Looter.SetPiece.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSetPiecePlayTest::RunTest(const FString& Parameters)
{
	// A set piece played through: it springs as the player nears it, its first push from the side ahead with a placed
	// creature of its own, a speaker shut; the second push comes on top of the first's last one, from its own side; done
	// only once its placed one is down too, then the flag (in the campaign record), the speaker open again, the shower;
	// and a set piece with that flag set stays off (once a session).
	FCampaignRecord Campaign;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UEncounterSubsystem* Encounters = UEncounterSubsystem::Get(World);
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector::ZeroVector);
	ASpeakerPoint* Window = World->SpawnActor<ASpeakerPoint>(FVector(0.0, 1500.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A mission runner"), Runner) || !TestNotNull(TEXT("Encounters"), Encounters) || !TestNotNull(TEXT("Player stand-in"), Player)
		|| !TestNotNull(TEXT("A speaker"), Window) || !TestNotNull(TEXT("...with its point"), Window->SpeakerPoint.Get()))
	{
		return false;
	}
	Window->Tags.Add(TEXT("Speaker_SetPieceTest"));
	Runner->BeginForTesting({}, Campaign, Player);
	const FName Flag(TEXT("SetPiece.Test"));

	ASetPieceSpawner* Piece = SpawnSetPiece(World, FVector(2000.0, 0.0, 0.0), [&Flag](ASetPieceSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("SetPieceTest");
		Setup.DoneFlag = Flag;
		Setup.Groups = { WaveGroup(ASpiderCreature::StaticClass(), 3, 1), WaveGroup(ASpiderCreature::StaticClass(), 2, 2) };
		Setup.NumWaves = 2;
		Setup.PushWhenLeft = 1;
		Setup.SpringRadius = 3000.f;
		Setup.GiveUpRadius = 5000.f;
		FSetPieceBeat Ahead;
		Ahead.Wave = 1;
		Ahead.Spots = { FVector(0.0, 500.0, 0.0), FVector(0.0, -500.0, 0.0), FVector(400.0, 0.0, 0.0) };
		Ahead.Line = FText::FromString(TEXT("Look out!"));
		FSetPieceBeat Behind;
		Behind.Wave = 2;
		Behind.Spots = { FVector(-1000.0, 700.0, 0.0), FVector(-1000.0, -700.0, 0.0) };
		Setup.Beats = { Ahead, Behind };
		FSetPiecePlaced Ringer;
		Ringer.CreatureClass = ASlimeCreature::StaticClass();
		Ringer.Location = FVector(2600.0, 1500.0, 0.0);
		Ringer.Wave = 1;
		Setup.Placed = { Ringer };
		Setup.HushSpeakerTags = { TEXT("Speaker_SetPieceTest") };
	});
	if (!TestNotNull(TEXT("The set piece"), Piece))
	{
		return false;
	}
	TestTrue(TEXT("Its flag keeps it off once set: in its condition's without-flags"), Piece->ActiveWhen.WithoutFlags.Contains(Flag));
	TestTrue(TEXT("Waiting for the player"), Piece->GetState() == EEncounterState::Waiting);

	// Sprung: the first push from ahead (its beat's spots, round the spawner's far side), the placed one, the speaker shut.
	Piece->UpdateEncounter(0.5f);
	TestEqual(TEXT("Sprung as the player comes near: the first push's three"), Piece->NumAlive(), 3);
	TestEqual(TEXT("...and its placed one"), Piece->NumPlacedAlive(), 1);
	bool bAhead = true;
	for (const ACreatureBase* Creature : Piece->GetAliveCreatures())
	{
		bAhead &= Creature->GetActorLocation().X > 1500.0;
	}
	TestTrue(TEXT("...the push from its own side (ahead)"), bAhead);
	TestTrue(TEXT("The speaker won't talk while it's on"), !Window->SpeakerPoint->bEnabled && Piece->IsHushing());

	// The second push comes on top of the first's last one, from behind.
	TArray<ACreatureBase*> First = Piece->GetAliveCreatures();
	BossTestWorld::Kill(First[0]);
	Piece->UpdateEncounter(0.5f);
	TestEqual(TEXT("Two left of three: no push yet"), Piece->GetWavesStarted(), 1);
	BossTestWorld::Kill(First[1]);
	Piece->UpdateEncounter(0.5f);
	TestEqual(TEXT("One left: the second push comes"), Piece->GetWavesStarted(), 2);
	int32 Behind = 0;
	for (const ACreatureBase* Creature : Piece->GetAliveCreatures())
	{
		Behind += Creature->GetActorLocation().X < 1500.0 ? 1 : 0;
	}
	TestEqual(TEXT("...its two from behind, on top of the one left"), Behind, 2);
	TestEqual(TEXT("...three up"), Piece->NumAlive(), 3);

	// Its own down with its placed one still up: not done.
	EncounterTestWorld::KillAll(*Piece);
	Piece->UpdateEncounter(0.5f);
	TestTrue(TEXT("Its waves cleared"), Piece->GetState() == EEncounterState::Cleared);
	TestFalse(TEXT("...but its placed one is up: not done"), Piece->IsDone() || Campaign.HasFlag(Flag));
	TestFalse(TEXT("...and the speaker stays shut"), Window->SpeakerPoint->bEnabled);

	// The placed one down: done, the flag kept, the speaker open, the shower thrown.
	for (TActorIterator<ASlimeCreature> It(World); It; ++It)
	{
		BossTestWorld::Kill(*It);
	}
	TestTrue(TEXT("Its placed one down: done"), Piece->CheckDone() && Piece->IsDone());
	TestTrue(TEXT("...its flag in the campaign record (saved as any story flag)"), Campaign.HasFlag(Flag));
	TestTrue(TEXT("...the speaker talks again"), Window->SpeakerPoint->bEnabled && !Piece->IsHushing());
	int32 Showers = 0;
	for (TActorIterator<ABossLootShower> It(World); It; ++It)
	{
		++Showers;
	}
	TestEqual(TEXT("...and its loot shower thrown"), Showers, 1);
	Encounters->RefreshStory();
	TestTrue(TEXT("Done, it stays cleared this visit"), Piece->GetState() == EEncounterState::Cleared);

	// Once a session: the same set piece beginning with its flag set (a later load) is off.
	ASetPieceSpawner* Again = SpawnSetPiece(World, FVector(-4000.0, 0.0, 0.0), [&Flag](ASetPieceSpawner& Setup)
	{
		Setup.DoneFlag = Flag;
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 2) };
		Setup.SpringRadius = 6000.f;
	});
	if (TestNotNull(TEXT("A second set piece with the flag"), Again))
	{
		Again->UpdateEncounter(0.5f);
		TestTrue(TEXT("...stays off, nothing spawned"), Again->GetState() == EEncounterState::Off && Again->NumAlive() == 0);
	}

	// Held: a set piece waiting on a spawner that's fighting doesn't spring until it's done.
	AEncounterSpawner* Shift = EncounterTestWorld::SpawnSpawner(World, FVector(0.0, -3000.0, 0.0), [](AEncounterSpawner& Setup)
	{
		Setup.SpawnerId = TEXT("SetPieceTestShift");
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1) };
		Setup.ActivationRadius = 5000.f;
	});
	ASetPieceSpawner* Drift = SpawnSetPiece(World, FVector(0.0, 3000.0, 0.0), [](ASetPieceSpawner& Setup)
	{
		Setup.DoneFlag = TEXT("SetPiece.TestDrift");
		Setup.Groups = { EncounterTestWorld::MakeGroup(ASpiderCreature::StaticClass(), 1) };
		Setup.SpringRadius = 5000.f;
		Setup.WaitForSpawners = { TEXT("SetPieceTestShift") };
	});
	if (TestNotNull(TEXT("A shift"), Shift) && TestNotNull(TEXT("A drift's set piece"), Drift))
	{
		Shift->UpdateEncounter(0.5f);
		Drift->UpdateEncounter(0.5f);
		TestTrue(TEXT("The shift fighting: the set piece holds off"), Shift->GetState() == EEncounterState::Engaged && Drift->IsHeld()
			&& Drift->NumAlive() == 0);
		EncounterTestWorld::KillAll(*Shift);
		Shift->UpdateEncounter(0.5f);
		Drift->UpdateEncounter(0.5f);
		TestTrue(TEXT("The shift down: it springs"), !Drift->IsHeld() && Drift->NumAlive() == 1);
	}
	return true;
}

#endif
