#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/HouseLights.h"
#include "World/LightFlickerRules.h"
#include "World/LightFlickerSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr float Frame = 1.f / 30.f;
	constexpr float Tolerance = 1e-4f;

	/** A lantern: a movable point light on an actor of its own at Where, at Intensity, tagged Flicker if asked. */
	UPointLightComponent* SpawnLantern(UWorld* World, const FVector& Where, float Intensity, bool bTagged)
	{
		AActor* Lantern = World->SpawnActor<AActor>();
		if (!Lantern)
		{
			return nullptr;
		}
		UPointLightComponent* Light = NewObject<UPointLightComponent>(Lantern, TEXT("Light"));
		Light->SetMobility(EComponentMobility::Movable);
		Lantern->SetRootComponent(Light);
		Light->RegisterComponent();
		Light->SetWorldLocation(Where);
		Light->SetIntensity(Intensity);
		if (bTagged)
		{
			Light->ComponentTags.Add(ULightFlickerSubsystem::FlickerTag);
		}
		return Light;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickerBandTest, "Looter.World.Flicker.Band",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlickerBandTest::RunTest(const FString& Parameters)
{
	// A flame breathes a few percent either way, smoothly, gutters now and then (a small dip), and only a house lamp is
	// carried off and back.
	using namespace LightFlickerRules;
	FRandomStream Random(20261014);
	FFlameFlicker Flame;
	Flame.Start(Random, /*bCarried*/ false);
	float Lowest = 2.f;
	float BreathLow = 2.f;
	float BreathHigh = 0.f;
	float LargestStep = 0.f;
	float Last = Flame.GetFlame();
	int32 Gutters = 0;
	bool bWasGuttering = false;
	bool bEverCarried = false;
	constexpr float Minutes = 10.f;
	for (float Seconds = 0.f; Seconds < Minutes * 60.f; Seconds += Frame)
	{
		Flame.Advance(Frame, Random);
		const float Now = Flame.GetFlame();
		Lowest = FMath::Min(Lowest, Now);
		if (!Flame.IsGuttering())
		{
			BreathLow = FMath::Min(BreathLow, Now);
			BreathHigh = FMath::Max(BreathHigh, Now);
			if (!bWasGuttering)
			{
				LargestStep = FMath::Max(LargestStep, FMath::Abs(Now - Last));
			}
		}
		Gutters += Flame.IsGuttering() && !bWasGuttering ? 1 : 0;
		bWasGuttering = Flame.IsGuttering();
		bEverCarried |= Flame.GetCarry() != 1.f;
		Last = Now;
	}
	TestTrue(FString::Printf(TEXT("The breath stays within +-%.0f%% (%.3f..%.3f)"), Breath * 100.f, BreathLow, BreathHigh),
		BreathLow >= 1.f - Breath - Tolerance && BreathHigh <= 1.f + Breath + Tolerance);
	TestTrue(TEXT("...and it does breathe: 6% or more between its lows and highs"), BreathHigh - BreathLow >= 0.06f);
	TestTrue(FString::Printf(TEXT("Soft, never a strobe: at most 3%% from one frame to the next (%.4f)"), LargestStep), LargestStep <= 0.03f);
	TestTrue(FString::Printf(TEXT("A gutter dips it a little more, never out (lowest %.3f)"), Lowest),
		Lowest >= 1.f - Breath - GutterDepth - Tolerance && Lowest < 1.f - Breath);
	const float Seconds = Minutes * 60.f;
	TestTrue(FString::Printf(TEXT("Gutters are rare: one every %.0f-%.0f s (%d in %.0f minutes)"), GutterGapMin, GutterGapMax, Gutters, Minutes),
		Gutters >= FMath::FloorToInt(Seconds / (GutterGapMax + GutterSeconds)) - 1 && Gutters <= FMath::CeilToInt(Seconds / GutterGapMin) + 1);
	TestFalse(TEXT("A lantern is never carried off"), bEverCarried);

	// A house lamp is carried off now and then: it and its windows dim slowly to their low and come back.
	FFlameFlicker Lamp;
	Lamp.Start(Random, /*bCarried*/ true);
	int32 Carries = 0;
	bool bWasCarried = false;
	float CarryLow = 1.f;
	float GlassLow = 2.f;
	float GlassHigh = 0.f;
	float CarryStep = 0.f;
	float LastCarry = 1.f;
	for (float Time = 0.f; Time < 20.f * 60.f; Time += Frame)
	{
		Lamp.Advance(Frame, Random);
		Carries += Lamp.IsCarried() && !bWasCarried ? 1 : 0;
		bWasCarried = Lamp.IsCarried();
		CarryLow = FMath::Min(CarryLow, Lamp.GetCarry());
		CarryStep = FMath::Max(CarryStep, FMath::Abs(Lamp.GetCarry() - LastCarry));
		LastCarry = Lamp.GetCarry();
		GlassLow = FMath::Min(GlassLow, Lamp.GetGlassFactor());
		GlassHigh = FMath::Max(GlassHigh, Lamp.GetGlassFactor());
	}
	TestTrue(FString::Printf(TEXT("Carried now and then (%d times in 20 minutes)"), Carries), Carries >= 5 && Carries <= 30);
	TestTrue(FString::Printf(TEXT("...never darker than its low (%.2f)"), CarryLow), CarryLow >= CarryLowMin - Tolerance && CarryLow <= CarryLowMax);
	TestTrue(TEXT("...and slowly: no sudden drop"), CarryStep <= 0.02f);
	TestTrue(FString::Printf(TEXT("The windows follow, a little softer (%.3f..%.3f)"), GlassLow, GlassHigh),
		GlassHigh <= 1.f + GlassShare * Breath + Tolerance && GlassLow >= CarryLowMin * (1.f - GlassShare * (Breath + GutterDepth)) - Tolerance);

	// Two lamps side by side never breathe together.
	FFlameFlicker Other;
	Other.Start(Random, false);
	FFlameFlicker First;
	First.Start(Random, false);
	float Apart = 0.f;
	for (float Time = 0.f; Time < 10.f; Time += Frame)
	{
		First.Advance(Frame, Random);
		Other.Advance(Frame, Random);
		Apart = FMath::Max(Apart, FMath::Abs(First.GetFlame() - Other.GetFlame()));
	}
	TestTrue(TEXT("Two flames, two breaths"), Apart > 0.03f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickerNearTest, "Looter.World.Flicker.NearOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlickerNearTest::RunTest(const FString& Parameters)
{
	// Lights tagged Flicker breathe only near the view; far off they rest at their own intensity and nothing runs. Whatever
	// else sets a light (dusk) is its own from then on; a hidden light is left alone.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ULightFlickerSubsystem* Flicker = ULightFlickerSubsystem::Get(World);
	if (!TestNotNull(TEXT("The flicker"), Flicker))
	{
		return false;
	}
	Flicker->SetRandomSeed(20261014);
	const FVector Where(0.0, 0.0, 300.0);
	UPointLightComponent* Lantern = SpawnLantern(World, Where, 10.f, /*bTagged*/ true);
	UPointLightComponent* Steady = SpawnLantern(World, Where + FVector(200.0, 0.0, 0.0), 10.f, /*bTagged*/ false);
	if (!TestNotNull(TEXT("A lantern"), Lantern) || !TestNotNull(TEXT("A steady light"), Steady))
	{
		return false;
	}
	Flicker->GatherTagged();
	TestEqual(TEXT("The tagged light joins, the other doesn't"), Flicker->NumLights(), 1);
	TestEqual(TEXT("...at its own intensity"), Flicker->GetOwnIntensity(Lantern), 10.f);

	// Far off: nothing near, nothing ticks, nothing written.
	const FVector Far = Where + FVector(LightFlickerRules::Radius + 500.0, 0.0, 0.0);
	Flicker->Look(Far);
	TestEqual(TEXT("Far: none near"), Flicker->NumNear(), 0);
	TestFalse(TEXT("...the subsystem doesn't tick"), Flicker->IsTickable());
	for (int32 Step = 0; Step < 60; ++Step)
	{
		Flicker->Step(Frame);
	}
	TestEqual(TEXT("...and the light is as it was"), Lantern->Intensity, 10.f);

	// Near: it breathes within its band.
	const FVector Near = Where + FVector(1000.0, 0.0, -150.0);
	Flicker->Look(Near);
	TestEqual(TEXT("Near: it flickers"), Flicker->NumNear(), 1);
	TestTrue(TEXT("...and the subsystem ticks"), Flicker->IsTickable());
	float Low = 100.f;
	float High = 0.f;
	for (int32 Step = 0; Step < 30 * 20; ++Step)
	{
		Flicker->Step(Frame);
		Low = FMath::Min(Low, Lantern->Intensity);
		High = FMath::Max(High, Lantern->Intensity);
	}
	using namespace LightFlickerRules;
	TestTrue(FString::Printf(TEXT("Within its band about its own (%.2f..%.2f cd)"), Low, High),
		Low >= 10.f * (1.f - Breath - GutterDepth) - Tolerance && High <= 10.f * (1.f + Breath) + Tolerance);
	TestTrue(TEXT("...and it moves"), High - Low > 0.2f);
	TestEqual(TEXT("The untagged light never moves"), Steady->Intensity, 10.f);

	// Dusk (somebody else) sets it brighter: that's its own now.
	Lantern->SetIntensity(20.f);
	Flicker->Step(Frame * 2.f);
	TestEqual(TEXT("Set by the lighting: its own from now on"), Flicker->GetOwnIntensity(Lantern), 20.f);
	TestTrue(TEXT("...and it breathes about that"), Lantern->Intensity >= 20.f * (1.f - Breath - GutterDepth) - Tolerance
		&& Lantern->Intensity <= 20.f * (1.f + Breath) + Tolerance);

	// Going far: back to exactly its own.
	Flicker->Look(Far);
	TestEqual(TEXT("Gone far: at its own again"), Lantern->Intensity, 20.f);
	TestFalse(TEXT("...and nothing ticks"), Flicker->IsTickable());

	// Hidden (the Foreman's dark): left alone, even near.
	Lantern->SetVisibility(false);
	Flicker->Look(Near);
	TestEqual(TEXT("A hidden light isn't flickered"), Flicker->NumNear(), 0);
	Lantern->SetVisibility(true);

	// Looter.Flicker 0: everything still.
	if (IConsoleVariable* Switch = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Flicker")))
	{
		Switch->Set(0, ECVF_SetByCode);
		Flicker->Look(Near);
		TestEqual(TEXT("Looter.Flicker 0: none flicker"), Flicker->NumNear(), 0);
		Switch->Set(1, ECVF_SetByCode);
		Flicker->Look(Near);
		TestEqual(TEXT("Looter.Flicker 1: back"), Flicker->NumNear(), 1);
	}

	// Taken out: back to its own, and gone.
	Flicker->Step(Frame * 2.f);
	Flicker->RemoveLight(Lantern);
	TestEqual(TEXT("Removed: none left"), Flicker->NumLights(), 0);
	TestEqual(TEXT("...at its own"), Lantern->Intensity, 20.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickerHouseLampTest, "Looter.World.Flicker.HouseLamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFlickerHouseLampTest::RunTest(const FString& Parameters)
{
	// A lived-in house's lamp is a flame: tagged Flicker, its windows its glass (they follow it), carried about now and
	// then; at dusk it flickers about its dusk light and glow.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ULightFlickerSubsystem* Flicker = ULightFlickerSubsystem::Get(World);
	AStaticMeshActor* House = World->SpawnActor<AStaticMeshActor>();
	AHouseLights* Lights = World->SpawnActor<AHouseLights>(FVector(0.0, 0.0, 200.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The flicker"), Flicker) || !TestNotNull(TEXT("A house"), House) || !TestNotNull(TEXT("Its lights"), Lights))
	{
		return false;
	}
	Flicker->SetRandomSeed(7);
	TestTrue(TEXT("The lamp is tagged Flicker"), Lights->Lamp->ComponentHasTag(ULightFlickerSubsystem::FlickerTag));
	UStaticMesh* HouseMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_RR_Home_A.SM_RR_Home_A"));
	if (!TestNotNull(TEXT("SM_RR_Home_A exists (Art/Models/Buildings/Homestead.py)"), HouseMesh))
	{
		return false;
	}
	House->GetStaticMeshComponent()->SetStaticMesh(HouseMesh);
	Lights->House = House;
	Lights->DispatchBeginPlay();
	UMaterialInstanceDynamic* Windows = Lights->GetWindows();
	if (!TestNotNull(TEXT("The windows' own instance"), Windows))
	{
		return false;
	}
	TestEqual(TEXT("The lamp flickers"), Flicker->NumLights(), 1);
	TestEqual(TEXT("...its windows are its glass, at their day glow"), Flicker->GetOwnGlow(Lights->Lamp), Lights->DayGlow);
	const FFlameFlicker* Flame = Flicker->GetFlame(Lights->Lamp);
	TestTrue(TEXT("...and it's carried about now and then"), Flame && Flame->bCarries);

	Lights->ApplyState(FName(TEXT("Dusk")));
	Flicker->Look(Lights->GetActorLocation() + FVector(1200.0, 0.0, 0.0));
	float GlowLow = 100.f;
	float GlowHigh = 0.f;
	for (int32 Step = 0; Step < 30 * 10; ++Step)
	{
		Flicker->Step(1.f / 30.f);
		const float Glow = Windows->K2_GetScalarParameterValue(Lights->GlowParameter);
		GlowLow = FMath::Min(GlowLow, Glow);
		GlowHigh = FMath::Max(GlowHigh, Glow);
	}
	using namespace LightFlickerRules;
	TestEqual(TEXT("At dusk: its own light is the dusk lamp"), Flicker->GetOwnIntensity(Lights->Lamp), Lights->DuskLamp);
	TestEqual(TEXT("...and its glass the dusk glow"), Flicker->GetOwnGlow(Lights->Lamp), Lights->DuskGlow);
	TestTrue(FString::Printf(TEXT("The windows breathe with it (%.2f..%.2f)"), GlowLow, GlowHigh),
		GlowHigh <= Lights->DuskGlow * (1.f + GlassShare * Breath) + 0.01f
		&& GlowLow >= Lights->DuskGlow * CarryLowMin * (1.f - GlassShare * (Breath + GutterDepth)) - 0.01f && GlowHigh - GlowLow > 0.2f);

	Flicker->Look(Lights->GetActorLocation() + FVector(LightFlickerRules::Radius + 1000.0, 0.0, 0.0));
	TestEqual(TEXT("Gone far: the lamp at its dusk light"), Lights->Lamp->Intensity, Lights->DuskLamp);
	TestEqual(TEXT("...the windows at their dusk glow"), Windows->K2_GetScalarParameterValue(Lights->GlowParameter), Lights->DuskGlow);

	// Back to day while far: the house's own values, and the flicker takes them as the lamp's own when it comes near.
	Lights->ApplyState(FName(TEXT("Day")));
	Flicker->Look(Lights->GetActorLocation() + FVector(1200.0, 0.0, 0.0));
	Flicker->Step(1.f / 15.f);
	TestEqual(TEXT("Day again: its own light is the day lamp"), Flicker->GetOwnIntensity(Lights->Lamp), Lights->DayLamp);
	return true;
}

#endif
