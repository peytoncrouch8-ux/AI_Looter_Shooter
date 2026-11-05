#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Session/CampaignRecord.h"
#include "Story/StoryCondition.h"
#include "World/HouseLights.h"
#include "World/LightingStates.h"
#include "World/TownFire.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const FName Main1(TEXT("Main1"));
	const FName Main3(TEXT("Main3"));

	/** The Company's barrel fire at the gate as the level script sets it: the Company's from Main 1, the checkpoint broken after Main 3. */
	void MakeGateFire(ATownFire& Fire)
	{
		FStoryCondition AfterMain1;
		AfterMain1.AfterMissions = { Main1 };
		FStoryCondition AfterMain3;
		AfterMain3.AfterMissions = { Main3 };
		Fire.ShownWhen = { AfterMain1 };
		Fire.HiddenWhen = { AfterMain3 };
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTownFireLightTest, "Looter.World.TownFire.Light",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTownFireLightTest::RunTest(const FString& Parameters)
{
	// A town fire burns by the light of the moment: out by day (a forge burns by day too), lit in the smog's Gloom and at
	// Dusk, DuskSmog as Dusk; its light follows the town's rules and is hidden while it is out.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ATownFire* Barrel = World->SpawnActor<ATownFire>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator);
	ATownFire* Forge = World->SpawnActor<ATownFire>(FVector(2000.0, 0.0, 100.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The barrel fire"), Barrel) || !TestNotNull(TEXT("A forge"), Forge))
	{
		return false;
	}
	Forge->bBurnsByDay = true;
	Barrel->DispatchBeginPlay();
	Forge->DispatchBeginPlay();

	// The light's rules (the towns plan 4.1): a few candelas, a reach under 4.6 m, 2400 K, no shadow, no light function, no
	// translucency lighting, drawn out to 45 m.
	const UPointLightComponent* Light = Barrel->FireLight;
	TestTrue(TEXT("3 to 5 candelas"), Barrel->LightCandela >= 3.f && Barrel->LightCandela <= 5.f);
	TestTrue(TEXT("...reaching 4.5 m at most"), Light->AttenuationRadius <= 450.f);
	TestTrue(TEXT("...a 2400 K light"), Light->bUseTemperature != 0 && FMath::IsNearlyEqual(Light->Temperature, 2400.f, 1.f));
	TestFalse(TEXT("...no shadow"), Light->CastShadows != 0);
	TestTrue(TEXT("...no light function"), Light->LightFunctionMaterial == nullptr);
	TestFalse(TEXT("...no lighting of translucent things"), Light->bAffectTranslucentLighting != 0);
	TestEqual(TEXT("...drawn out to 45 m"), Light->MaxDrawDistance, AHouseLights::LampDrawDistance);
	TestEqual(TEXT("...fading over the last 10 m"), Light->MaxDistanceFadeRange, AHouseLights::LampFadeRange);
	TestFalse(TEXT("The flames cast no shadow"), Barrel->Flame->CastShadow != 0);

	struct FBurns
	{
		const TCHAR* State;
		bool bBarrel;
		bool bForge;
	};
	const FBurns Table[] = {
		{ TEXT("Day"), false, true }, { TEXT("Smog"), true, true }, { TEXT("SmogThin"), true, true },
		{ TEXT("Dusk"), true, true }, { TEXT("DuskSmog"), true, true },
	};
	for (const FBurns& Each : Table)
	{
		Barrel->ApplyState(FName(Each.State));
		Forge->ApplyState(FName(Each.State));
		// A fire lit as the level plays catches from nothing; a second on, it burns at its own.
		Barrel->Advance(1.f);
		Forge->Advance(1.f);
		TestTrue(FString::Printf(TEXT("The barrel fire in %s"), Each.State), Barrel->IsBurning() == Each.bBarrel);
		TestTrue(FString::Printf(TEXT("The forge in %s"), Each.State), Forge->IsBurning() == Each.bForge);
		TestTrue(FString::Printf(TEXT("...the light shown only while it burns, in %s"), Each.State),
			Barrel->FireLight->IsVisible() == Each.bBarrel && Forge->FireLight->IsVisible() == Each.bForge);
		TestTrue(FString::Printf(TEXT("...the flames and the smoke too, in %s"), Each.State),
			Barrel->Flame->IsVisible() == (Each.bBarrel && Barrel->CardMesh != nullptr)
			&& Barrel->Smoke->IsVisible() == (Each.bBarrel && Barrel->SmokeMesh != nullptr));
		TestTrue(FString::Printf(TEXT("...dark when out, a few candelas when lit, in %s"), Each.State),
			(Each.bBarrel ? Barrel->GetLightCandela() > 2.f && Barrel->GetLightCandela() < 5.f : Barrel->GetLightCandela() == 0.f));
	}
	TestTrue(TEXT("DuskSmog is the town's dusk"), Barrel->GetKind() == ELightKind::Dusk);

	// Burning, it flickers about its own: a band, never still.
	Barrel->ApplyState(FName(TEXT("Dusk")));
	Barrel->Advance(2.f);
	float Low = 100.f;
	float High = 0.f;
	float FlameLow = 100.f;
	float FlameHigh = 0.f;
	for (float Seconds = 0.f; Seconds < 10.f; Seconds += 1.f / 30.f)
	{
		Barrel->Advance(1.f / 30.f);
		Low = FMath::Min(Low, Barrel->GetLightCandela());
		High = FMath::Max(High, Barrel->GetLightCandela());
		FlameLow = FMath::Min(FlameLow, Barrel->GetFlameScale());
		FlameHigh = FMath::Max(FlameHigh, Barrel->GetFlameScale());
	}
	TestTrue(FString::Printf(TEXT("The light breathes about its candelas (%.2f..%.2f)"), Low, High),
		Low >= Barrel->LightCandela * 0.9f && High <= Barrel->LightCandela * 1.05f && High - Low > 0.05f);
	TestTrue(FString::Printf(TEXT("...and the flames rise and fall (%.2f..%.2f)"), FlameLow, FlameHigh),
		FlameLow > 0.75f && FlameHigh < 1.25f && FlameHigh - FlameLow > 0.1f);

	// Lit while the level plays, it catches from nothing; dark again, it ticks no more.
	Barrel->ApplyState(FName(TEXT("Day")));
	TestFalse(TEXT("Out by day again"), Barrel->IsBurning());
	Barrel->ApplyState(FName(TEXT("Smog")));
	Barrel->Advance(0.1f);
	const float Early = Barrel->GetLightCandela();
	Barrel->Advance(1.f);
	TestTrue(TEXT("It catches from nothing in a moment"), Early < Barrel->GetLightCandela() * 0.6f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTownFireStoryTest, "Looter.World.TownFire.Story",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTownFireStoryTest::RunTest(const FString& Parameters)
{
	// The Company's barrel fire at the gate: there from Main 1 (the Company arrives) and out once Main 3 breaks the
	// checkpoint; and out when the story hides it, whatever the light.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ATownFire* Fire = World->SpawnActor<ATownFire>(FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The barrel fire"), Fire))
	{
		return false;
	}
	MakeGateFire(*Fire);
	FCampaignRecord Campaign;
	TestFalse(TEXT("Before Main 1: the gate has no fire"), Fire->ShouldShow(&Campaign, nullptr));
	Campaign.Complete(Main1);
	TestTrue(TEXT("After Main 1: the Company's fire"), Fire->ShouldShow(&Campaign, nullptr));
	Campaign.Complete(Main3);
	TestFalse(TEXT("After Main 3: the checkpoint is broken, the fire out"), Fire->ShouldShow(&Campaign, nullptr));
	TestFalse(TEXT("With no campaign at all it is not there (a story fire needs its story)"), Fire->ShouldShow(nullptr, nullptr));

	ATownFire* Plain = World->SpawnActor<ATownFire>(FVector(500.0, 0.0, 100.0), FRotator::ZeroRotator);
	if (TestNotNull(TEXT("A forge, with no story"), Plain))
	{
		TestTrue(TEXT("A fire with no story conditions is always there"), Plain->ShouldShow(nullptr, nullptr));
	}

	// The story's word is the fire's, whatever the light says.
	Fire->DispatchBeginPlay();
	Fire->ApplyState(FName(TEXT("DuskSmog")));
	Fire->SetShown(true);
	TestTrue(TEXT("Shown and at dusk: burning"), Fire->IsBurning() && Fire->FireLight->IsVisible());
	Fire->SetShown(false);
	TestTrue(TEXT("Hidden by the story at dusk: out"), !Fire->IsBurning() && !Fire->FireLight->IsVisible() && !Fire->Smoke->IsVisible());
	return true;
}

#endif
