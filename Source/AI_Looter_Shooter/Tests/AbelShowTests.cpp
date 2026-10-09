#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Abel's show (AbelKeeperShow.cpp, AbelRules::MakeShow): his entrance with his lantern raised, a stagger putting him on a
// knee with his coal open (never out in the fog), his shots from the fog, his barrage in the wind, and his loot's shower
// waiting for the scene.

#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelRules.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossRules.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Tests/AbelTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

using namespace AbelTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbelShowTest, "Looter.Bosses.Abel.Show",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAbelShowTest::RunTest(const FString& Parameters)
{
	// His bar's show and his loot as data: his title, his cries, the shower after the scene from the deck's middle.
	const FBossShow Show = AbelRules::MakeShow();
	TestEqual(TEXT("His title"), Show.Title.ToString(), FString(TEXT("Waiting on a Dark Saint")));
	TestTrue(TEXT("...his cries at the start, each phase, a stagger and his death"), !Show.IntroCue.IsNone() && !Show.PhaseCue.IsNone()
		&& !Show.StaggerCue.IsNone() && !Show.DeathCue.IsNone());
	const FBossLootShowerSettings Shower = AbelRules::MakeLootShower();
	TestTrue(TEXT("His loot waits for the scene, and bursts from the deck's middle (not the open end)"), Shower.bAfterScene && Shower.bFromSpot
		&& Shower.Delay >= 2.f && Shower.BonusAmmo > 0);
	const TArray<FBossPhase> Phases = BossRules::Ordered(AbelRules::MakePhases());
	TestTrue(TEXT("\"The bell\" has his shots from the fog"), Phases.Num() == 3 && Phases[1].Events.ContainsByPredicate([](const FBossPhaseEvent& Event)
	{
		return Event.Kind == EBossEventKind::Custom && Event.Name == AbelRules::FogShotEvent() && Event.RepeatEvery > 0.f;
	}));

	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AAbelKeeper* Abel = SpawnAbel(World);
	UEnemyProjectileSubsystem* Shots = World->GetSubsystem<UEnemyProjectileSubsystem>();
	ACharacter* Player = nullptr;
	UBossComponent* Boss = Abel && Shots ? StartFight(*this, World, Abel, Player) : nullptr;
	if (!Boss)
	{
		return false;
	}

	// His entrance: turned to the player, his lantern raised; then he fights.
	TestTrue(TEXT("As his fight starts he raises his lantern"), Abel->GetMove() == EAbelMove::Intro);
	Run(Abel, Frame);
	TestTrue(TEXT("...in his flare's pose"), Abel->GetPoseNow() == EAbelPose::Flare);
	const float IntroOver = RunUntil(Abel, Abel->ShowRules.IntroSeconds + 0.5f, [Abel]() { return Abel->GetMove() != EAbelMove::Intro; });
	TestTrue(FString::Printf(TEXT("...for under two seconds (%.2f s)"), IntroOver), IntroOver > 0.f && IntroOver <= Abel->ShowRules.IntroSeconds + Frame);

	// Staggered while he grieves: down on a knee facing the player, his coal still open; then up again.
	if (TestTrue(TEXT("He grieves"), Abel->Grieve()))
	{
		TestTrue(TEXT("Crits on his open coal stagger him"), Boss->BeginStagger() && Abel->GetMove() == EAbelMove::Staggered);
		Run(Abel, Frame);
		TestTrue(TEXT("...down on a knee, his coal open"), Abel->GetPoseNow() == EAbelPose::Kneel && Abel->IsCoalOpen());
		Run(Abel, Boss->Stagger.Seconds + 0.2f);
		TestTrue(TEXT("...then up again"), !Boss->IsStaggered() && Abel->GetMove() != EAbelMove::Staggered);
	}

	// Out in the fog: no stagger, and his lantern flares over the canyon before he fires from it.
	HurtTo(Abel, AbelRules::BellShare);
	TestTrue(TEXT("At 60% he drifts out"), Abel->IsOutInFog());
	TestFalse(TEXT("...where nothing staggers him"), Boss->BeginStagger());
	Run(Abel, Abel->DriftSeconds + 0.1f);
	const float FlareAt = RunUntil(Abel, AbelRules::FogShotFirst + 1.f, [Abel]()
	{
		return Abel->GetMove() == EAbelMove::InFog && Abel->GetPoseNow() == EAbelPose::Flare;
	});
	TestTrue(FString::Printf(TEXT("In the fog his lantern flares (%.2f s in)"), FlareAt), FlareAt >= 0.f);
	const int32 Before = Shots->NumShotsFrom(Abel);
	Run(Abel, Abel->ShowRules.FogFlareSeconds + 0.1f);
	TestEqual(TEXT("...then his buckshot flies from the fog"), Shots->NumShotsFrom(Abel), Before + Abel->Buckshot.Pellets);
	TestTrue(TEXT("...and he's still out there"), Abel->IsOutInFog() && Boss->IsUntargetable());

	// In the wind his buckshot is a barrage: three shots after the flare, a gap apart.
	FTestWorldWrapper WindWrapper;
	if (!TestTrue(TEXT("Second test world created"), WindWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* Windy = WindWrapper.GetTestWorld();
	AAbelKeeper* InWind = SpawnAbel(Windy);
	UEnemyProjectileSubsystem* WindShots = Windy->GetSubsystem<UEnemyProjectileSubsystem>();
	ACharacter* WindPlayer = nullptr;
	if (!InWind || !WindShots || !StartFight(*this, Windy, InWind, WindPlayer))
	{
		return false;
	}
	Run(InWind, InWind->ShowRules.IntroSeconds + 0.1f);
	InWind->StartWind();
	if (TestTrue(TEXT("In the wind he flares"), InWind->StartFlare()))
	{
		Run(InWind, InWind->FlareSeconds + Frame);
		TestEqual(TEXT("...the barrage's first shot"), WindShots->NumShotsFrom(InWind), InWind->ShowRules.BarragePellets);
		Run(InWind, InWind->ShowRules.BarrageGap * (InWind->ShowRules.BarrageShots - 1) + Frame);
		TestEqual(TEXT("...then the rest, a gap apart"), WindShots->NumShotsFrom(InWind), InWind->ShowRules.BarragePellets * InWind->ShowRules.BarrageShots);
	}
	return true;
}

#endif
