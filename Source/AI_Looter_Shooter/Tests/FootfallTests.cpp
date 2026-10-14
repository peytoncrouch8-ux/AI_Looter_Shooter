#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/FootfallDust.h"
#include "Player/PlayerFootfallComponent.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/WeaponFX.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// The player's feet throwing dust (concept F, "your feet"; Looter.Player.Footfall.*): how much each surface throws for a
// sprinting step and a landing (water splashes, wood and stone almost nothing), the cap (a budget that refills), and the
// real character (BP_LooterCharacter) landing on dirt, wood and water in a test level.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootfallRulesTest, "Looter.Player.Footfall.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFootfallRulesTest::RunTest(const FString& Parameters)
{
	// A sprinting step: dirt and sand a puff or two, grass one, wood, stone, metal and glass none, water a splash.
	TestEqual(TEXT("Step on dirt"), FFootfallDust::PuffsFor(EFootfall::Step, EImpactMaterial::Dirt, 0.f), 2);
	TestEqual(TEXT("Step on sand"), FFootfallDust::PuffsFor(EFootfall::Step, EImpactMaterial::Sand, 0.f), 2);
	TestEqual(TEXT("Step on grass: a little"), FFootfallDust::PuffsFor(EFootfall::Step, EImpactMaterial::Grass, 0.f), 1);
	TestEqual(TEXT("Step on wood: nothing"), FFootfallDust::PuffsFor(EFootfall::Step, EImpactMaterial::Wood, 0.f), 0);
	TestEqual(TEXT("Step on stone: nothing"), FFootfallDust::PuffsFor(EFootfall::Step, EImpactMaterial::Stone, 0.f), 0);
	TestEqual(TEXT("Step on metal: nothing"), FFootfallDust::PuffsFor(EFootfall::Step, EImpactMaterial::Metal, 0.f), 0);
	TestEqual(TEXT("Step on glass: nothing"), FFootfallDust::PuffsFor(EFootfall::Step, EImpactMaterial::Glass, 0.f), 0);
	TestTrue(TEXT("Gravel flicks grit; dirt's step doesn't"), FFootfallDust::GritFor(EFootfall::Step, EImpactMaterial::Gravel, 0.f) > 0
		&& FFootfallDust::GritFor(EFootfall::Step, EImpactMaterial::Dirt, 0.f) == 0);
	TestTrue(TEXT("Water splashes, no dust"), FFootfallDust::Splashes(EImpactMaterial::Water)
		&& FFootfallDust::PuffsFor(EFootfall::Land, EImpactMaterial::Water, 1.f) == 0);

	// A landing: more the harder it was, most on dirt; a tiny one at most on stone and wood.
	const int32 Hop = FFootfallDust::PuffsFor(EFootfall::Land, EImpactMaterial::Dirt, 0.f);
	const int32 Hard = FFootfallDust::PuffsFor(EFootfall::Land, EImpactMaterial::Dirt, 1.f);
	TestTrue(FString::Printf(TEXT("A hard landing throws more than a hop (%d, %d)"), Hard, Hop), Hop > 0 && Hard > Hop);
	TestTrue(TEXT("...dirt more than grass"), FFootfallDust::PuffsFor(EFootfall::Land, EImpactMaterial::Grass, 1.f) < Hard);
	TestTrue(TEXT("...stone and wood a wisp at most"), FFootfallDust::PuffsFor(EFootfall::Land, EImpactMaterial::Stone, 1.f) <= 2
		&& FFootfallDust::PuffsFor(EFootfall::Land, EImpactMaterial::Wood, 1.f) <= 1);
	TestTrue(TEXT("...a hard one within the budget"), Hard <= FMath::FloorToInt32(FFootfallDust::BudgetCap));
	TestTrue(TEXT("Every surface's dust has a colour"), FFootfallDust::DustAlbedo(EImpactMaterial::Sand).GetLuminance() > FFootfallDust::DustAlbedo(EImpactMaterial::Dirt).GetLuminance());

	// How hard a landing was, from the speed it came down at.
	TestEqual(TEXT("A hop's worth is soft"), UPlayerFootfallComponent::LandHardness(200.f), 0.f);
	TestEqual(TEXT("A long fall is hard"), UPlayerFootfallComponent::LandHardness(2000.f), 1.f);
	const float Between = UPlayerFootfallComponent::LandHardness(650.f);
	TestTrue(TEXT("...and between, between"), Between > 0.f && Between < 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootfallCapTest, "Looter.Player.Footfall.Cap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFootfallCapTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FWeaponFX Effects;
	Effects.Initialize(TestLevel.GetTestWorld());
	FFootfallDust Dust;
	FFootfallSpot Spot;
	Spot.Along = FVector::ForwardVector;
	Spot.Speed = 700.f;
	Spot.Hardness = 1.f;

	// A scramble of hard landings in a moment: the budget holds it to its cap.
	int32 Thrown = 0;
	for (int32 Landing = 0; Landing < 10; ++Landing)
	{
		Thrown += Dust.Throw(Effects, EFootfall::Land, EImpactMaterial::Dirt, Spot, FLinearColor::White);
	}
	TestTrue(FString::Printf(TEXT("Ten hard landings at once throw no more than the cap (%d)"), Thrown),
		Thrown > 0 && Thrown <= FMath::FloorToInt32(FFootfallDust::BudgetCap));
	TestTrue(TEXT("...drawn by the pooled effects"), Effects.NumParticles() > 0);
	TestEqual(TEXT("...nothing more now"), Dust.Throw(Effects, EFootfall::Land, EImpactMaterial::Dirt, Spot, FLinearColor::White), 0);
	Dust.Refill(1.f);
	TestTrue(TEXT("A moment later there's dust again"), Dust.Throw(Effects, EFootfall::Land, EImpactMaterial::Dirt, Spot, FLinearColor::White) > 0);

	// Water: a splash, paid from the same budget.
	FFootfallDust Wet;
	TestEqual(TEXT("Into water: a splash"), Wet.Throw(Effects, EFootfall::Land, EImpactMaterial::Water, Spot, FLinearColor::White), 1);
	TestTrue(TEXT("...counted as one, no dust"), Wet.GetSplashCount() == 1 && Wet.GetPuffsThrown() == 0);
	// Wood: a sprinting step throws nothing at all.
	TestEqual(TEXT("A step on wood: nothing"), Wet.Throw(Effects, EFootfall::Step, EImpactMaterial::Wood, Spot, FLinearColor::White), 0);
	Effects.Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootfallSurfacesTest, "Looter.Player.Footfall.Surfaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFootfallSurfacesTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();

	struct FFootfallGround
	{
		const TCHAR* Name;
		const TCHAR* Tag;
		EImpactMaterial Expected;
		FVector Feet;
	};
	const FFootfallGround Grounds[] = {
		{ TEXT("Dirt"), TEXT("Surface.Dirt"), EImpactMaterial::Dirt, FVector::ZeroVector },
		{ TEXT("Wood"), TEXT("Surface.Wood"), EImpactMaterial::Wood, FVector(0.0, 2000.0, 0.0) },
		{ TEXT("Water"), TEXT("Surface.Water"), EImpactMaterial::Water, FVector(0.0, 4000.0, 0.0) },
	};
	int32 Puffs[UE_ARRAY_COUNT(Grounds)] = {};
	int32 Splashes[UE_ARRAY_COUNT(Grounds)] = {};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Grounds); ++Index)
	{
		const FFootfallGround& Ground = Grounds[Index];
		AStaticMeshActor* Block = LocomotionTestWorld::SpawnBlock(World, Ground.Feet - FVector(0.0, 0.0, 50.0), FVector(1500.0, 1500.0, 100.0));
		if (!TestNotNull(FString::Printf(TEXT("%s: the ground"), Ground.Name), Block))
		{
			return false;
		}
		Block->Tags.Add(FName(Ground.Tag));
		UPlayerLocomotionComponent* Locomotion = LocomotionTestWorld::SpawnPlayer(World, Ground.Feet);
		ACharacter* Body = Locomotion ? LocomotionTestWorld::BodyOf(Locomotion) : nullptr;
		UPlayerFootfallComponent* Feet = Body ? Body->FindComponentByClass<UPlayerFootfallComponent>() : nullptr;
		if (!TestNotNull(FString::Printf(TEXT("%s: the player's feet"), Ground.Name), Feet))
		{
			continue;
		}
		// A hard landing on it.
		Puffs[Index] = Feet->Footfall(EFootfall::Land, 1.f);
		Splashes[Index] = Feet->GetDust().GetSplashCount();
		TestTrue(FString::Printf(TEXT("%s: the feet know the ground"), Ground.Name), Feet->GetLastSurface() == Ground.Expected);
		AddInfo(FString::Printf(TEXT("%s: %d puffs, %d grains, %d splashes"), Ground.Name, Feet->GetDust().GetPuffsThrown(),
			Feet->GetDust().GetGritThrown(), Splashes[Index]));
	}
	TestTrue(FString::Printf(TEXT("Dirt: a ring of dust (%d puffs)"), Puffs[0]), Puffs[0] >= 8 && Splashes[0] == 0);
	TestTrue(FString::Printf(TEXT("Wood: a wisp at most (%d)"), Puffs[1]), Puffs[1] <= 1 && Splashes[1] == 0);
	TestTrue(FString::Printf(TEXT("Water: a splash, no dust (%d splashes)"), Splashes[2]), Splashes[2] == 1);
	return true;
}

#endif
