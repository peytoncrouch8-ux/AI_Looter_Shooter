#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The Gravemother's headstone (the user's pick over plain armour, 2026-10-19: "it has its own health; shots on it hurt the
// stone, not her. Broken, it cracks off and her pale Bone abdomen is bare. It also cracks off by itself at her 33% if it
// still stands"; Main's numbers: 20% of her health, cracked at 66% of its own, shattered at 33%, an early break staggers
// her but doesn't bring her last third). AGravemotherStone, GravemotherCreatureStone.cpp, the boss bar's stone bar.

#include "Bosses/BossComponent.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/GravemotherCreature.h"
#include "Creatures/GravemotherStone.h"
#include "Tests/BossTestWorld.h"
#include "Tests/GravemotherTestWorld.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

using namespace GravemotherTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherStoneHealthTest, "Looter.Creatures.Gravemother.Stone.Health",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherStoneHealthTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector, 6);
	AGravemotherStone* Stone = Mother ? Mother->GetStone() : nullptr;
	if (!TestNotNull(TEXT("The Gravemother's headstone"), Stone))
	{
		return false;
	}
	const UHealthComponent* Hers = Mother->FindComponentByClass<UHealthComponent>();
	const UHealthComponent* Its = Stone->GetHealthComponent();
	TestNearlyEqual(*FString::Printf(TEXT("Its health is a fifth of hers (%.0f of %.0f)"), Its->GetMaxHealth(), Hers->GetMaxHealth()),
		Its->GetMaxHealth(), Hers->GetMaxHealth() * 0.2f, 1.f);
	TestTrue(TEXT("Intact, shootable by bullets only, showing its own hits"), Stone->GetState() == EGravemotherStone::Intact && Stone->GetShownMesh() == 0
		&& Stone->GetSlab()->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block
		&& Stone->GetSlab()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore
		&& Stone->GetSlab()->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore);

	// Shots on it hurt it, not her; it cracks at 66% of its health and shatters at 33%.
	const float Before = Hers->GetHealth();
	HurtTo(Stone, 0.8f);
	TestNearlyEqual(TEXT("A hit on the stone leaves her whole"), Hers->GetHealth(), Before, 0.01f);
	TestEqual(TEXT("At 80% it's intact"), Stone->GetState(), EGravemotherStone::Intact);
	HurtTo(Stone, 0.6f);
	TestTrue(TEXT("At 60% it's cracked"), Stone->GetState() == EGravemotherStone::Cracked && Stone->GetShownMesh() == 1);
	HurtTo(Stone, 0.3f);
	TestTrue(TEXT("At 30% it's shattered"), Stone->GetState() == EGravemotherStone::Shattered && Stone->GetShownMesh() == 2);
	TestTrue(TEXT("...still on"), Mother->IsStoneOn());

	// A life starting over (her fight's reset, a rematch): whole again.
	Mother->ResetToHome();
	TestTrue(TEXT("Back home, her stone is whole"), Stone->GetState() == EGravemotherStone::Intact && FMath::IsNearlyEqual(Stone->GetHealthShare(), 1.f)
		&& Mother->GetStone() == Stone);

	// The boss bar's stone bar: shown at its share while the stone stands, gone when it breaks.
	UHudBossBarWidget* Bar = CreateWidget<UHudBossBarWidget>(World, UHudBossBarWidget::StaticClass());
	if (TestNotNull(TEXT("The boss bar"), Bar))
	{
		Bar->SetBoss(FText::FromString(TEXT("The Gravemother")), 8, UCreatureRankSettings::Get(ECreatureRank::Legendary).Color, { 1.f, 0.66f, 0.33f });
		Bar->TakeWidget();
		TestFalse(TEXT("No stone bar until a boss asks"), Bar->IsArmourShown());
		Bar->SetArmour(true, 0.6f);
		TestTrue(TEXT("Its stone bar shows the stone's share"), Bar->IsArmourShown() && FMath::IsNearlyEqual(Bar->GetArmourFraction(), 0.6f));
		Bar->SetArmour(false, 0.f);
		TestFalse(TEXT("...and goes when it breaks"), Bar->IsArmourShown());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravemotherStoneBreakTest, "Looter.Creatures.Gravemother.Stone.Break",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravemotherStoneBreakTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	SpawnFloor(World, FVector::ZeroVector, 2000.0);
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(1000.0, 0.0, 90.0));
	AGravemotherCreature* Mother = SpawnMother(World, FVector::ZeroVector);
	AGravemotherStone* Stone = Mother ? Mother->GetStone() : nullptr;
	if (!TestNotNull(TEXT("Player stand-in"), Player) || !TestNotNull(TEXT("Her headstone"), Stone))
	{
		return false;
	}
	UBossComponent* Boss = Mother->GetBoss();
	Boss->StartFight(Player);
	const float ChaseBefore = Mother->ChaseSpeed;

	// Shot off early: it cracks off and she staggers (her long stagger), but her last third doesn't come early.
	HurtTo(Stone, 0.f);
	TestTrue(TEXT("Shot to nothing: it cracks off"), Stone->GetState() == EGravemotherStone::Broken && !Mother->IsStoneOn()
		&& Stone->GetSlab()->bHiddenInGame && Stone->GetSlab()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestTrue(TEXT("...staggering her: her boss's own stagger"), Boss->IsStaggered() && Mother->IsReeling() && !Mother->IsStunned());
	TestTrue(TEXT("...but not her last third"), Boss->GetPhase() == 0 && Mother->GetPacePhase() == 0);
	TestEqual(TEXT("Its five pieces fall away"), Stone->NumShardsShown(), 5);
	Stone->Advance(0.1f);
	TestEqual(TEXT("...splitting first, all still up"), Stone->NumShardsFalling(), 5);
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		Stone->Advance(Step);
	}
	TestEqual(TEXT("Three seconds on, they lie on the floor"), Stone->NumShardsFalling(), 0);
	for (float Time = 0.f; Time < Mother->StoneSettings.ShardLieSeconds + Mother->StoneSettings.ShardSinkSeconds; Time += Step)
	{
		Stone->Advance(Step);
	}
	TestEqual(TEXT("...then sink away"), Stone->NumShardsShown(), 0);
	Boss->TickFight(Boss->Stagger.Seconds + 0.1f);
	TestTrue(TEXT("Up again after her stagger, at her own pace"), !Mother->IsReeling() && FMath::IsNearlyEqual(Mother->ChaseSpeed, ChaseBefore));

	// At her 33% it cracks off by itself, with no stagger, and she runs a quarter faster.
	ACharacter* Other = BossTestWorld::SpawnPlayer(World, FVector(1000.0, 1500.0, 90.0));
	AGravemotherCreature* Second = SpawnMother(World, FVector(0.0, 1500.0, 0.0));
	if (TestNotNull(TEXT("Second Gravemother"), Second) && TestNotNull(TEXT("Second stand-in"), Other))
	{
		UBossComponent* SecondBoss = Second->GetBoss();
		SecondBoss->StartFight(Other);
		const float OwnChase = Second->ChaseSpeed;
		HurtTo(Second, 0.32f);
		TestTrue(TEXT("At her 33% the stone cracks off by itself"), !Second->IsStoneOn());
		TestFalse(TEXT("...no stagger"), SecondBoss->IsStaggered());
		TestTrue(*FString::Printf(TEXT("...her last third: a quarter faster (%.0f to %.0f)"), OwnChase, Second->ChaseSpeed),
			SecondBoss->GetPhase() == 2 && FMath::IsNearlyEqual(Second->ChaseSpeed, OwnChase * 1.25f, 0.5f)
			&& FMath::IsNearlyEqual(Second->GetPace().SpeedScale, 1.25f));
	}
	return true;
}

#endif
