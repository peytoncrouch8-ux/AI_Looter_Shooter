#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/HouseLights.h"
#include "World/LightingStates.h"
#include "World/TownFire.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

namespace
{
	/** Every lighting state the town lights itself for (layout.json level.environment.states). */
	const TCHAR* const States[] = { TEXT("Day"), TEXT("Smog"), TEXT("SmogThin"), TEXT("Dusk"), TEXT("DuskSmog") };

	/** What a light should be in a state: its lamp (candelas) and, with windows, their Glow. */
	struct FExpected
	{
		float Lamp = 0.f;
		float Glow = 0.f;
	};

	/** A town lamp as town.json and the level scripts set it: day, gloom and dusk candelas, its reach (cm). */
	AHouseLights* SpawnLamp(UWorld* World, const FVector& Where, float Day, float Gloom, float Dusk, float Reach)
	{
		AHouseLights* Lights = World->SpawnActor<AHouseLights>(Where, FRotator::ZeroRotator);
		if (Lights)
		{
			Lights->DayLamp = Day;
			Lights->GloomLamp = Gloom;
			Lights->DuskLamp = Dusk;
			Lights->Lamp->SetIntensity(Day);
			Lights->Lamp->SetAttenuationRadius(Reach);
		}
		return Lights;
	}

	/** Lights up as State has it and checks the lamp (shown only above 0 candelas) and, if it has windows, their Glow. */
	void CheckState(FAutomationTestBase& Test, AHouseLights* Lights, const TCHAR* What, const TCHAR* State, const FExpected& Expected,
		bool bWindows)
	{
		Lights->ApplyState(FName(State));
		Test.TestEqual(FString::Printf(TEXT("%s in %s: the lamp"), What, State), Lights->Lamp->Intensity, Expected.Lamp);
		Test.TestTrue(FString::Printf(TEXT("%s in %s: shown only when it gives light"), What, State), Lights->Lamp->IsVisible() == (Expected.Lamp > 0.f));
		Test.TestTrue(FString::Printf(TEXT("%s in %s: IsLampLit says so"), What, State), Lights->IsLampLit() == (Expected.Lamp > 0.f));
		if (bWindows && Lights->GetWindows())
		{
			Test.TestEqual(FString::Printf(TEXT("%s in %s: the windows"), What, State),
				Lights->GetWindows()->K2_GetScalarParameterValue(Lights->GlowParameter), Expected.Glow);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHouseLightsTest, "Looter.World.HouseLights.Lamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHouseLightsTest::RunTest(const FString& Parameters)
{
	// Delia's farmhouse lights up at dusk: the hall lamp brightens and the windows glow, through an instance of their own
	// so no other house changes. By day the lamp is a glimmer, and it never reaches past the hall.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AStaticMeshActor* House = World->SpawnActor<AStaticMeshActor>();
	AHouseLights* Lights = World->SpawnActor<AHouseLights>();
	if (!TestNotNull(TEXT("A house"), House) || !TestNotNull(TEXT("The house's lights"), Lights))
	{
		return false;
	}
	UStaticMesh* HouseMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_Farmhouse_Ransom.SM_Farmhouse_Ransom"));
	if (TestNotNull(TEXT("SM_Farmhouse_Ransom exists (Art/Models/Buildings/Farmhouse.py)"), HouseMesh))
	{
		House->GetStaticMeshComponent()->SetStaticMesh(HouseMesh);
	}
	Lights->House = House;
	Lights->DispatchBeginPlay();

	TestEqual(TEXT("By day the lamp is a glimmer"), Lights->Lamp->Intensity, Lights->DayLamp);
	TestFalse(TEXT("...unshadowed"), Lights->Lamp->CastShadows != 0);
	TestTrue(TEXT("...and it stays in the hall"), Lights->Lamp->AttenuationRadius < 180.f);

	Lights->ApplyState(FName(TEXT("Dusk")));
	TestEqual(TEXT("At dusk the lamp brightens"), Lights->Lamp->Intensity, Lights->DuskLamp);
	TestTrue(TEXT("...and it knows it's dusk"), Lights->IsDusk() && !Lights->IsGloom());
	if (HouseMesh && TestNotNull(TEXT("The windows have an instance of their own (the WindowGlow slot)"), Lights->GetWindows()))
	{
		TestEqual(TEXT("...and the windows glow"), Lights->GetWindows()->K2_GetScalarParameterValue(Lights->GlowParameter), Lights->DuskGlow);
	}

	Lights->ApplyState(FName(TEXT("Day")));
	TestEqual(TEXT("Back to day: the lamp"), Lights->Lamp->Intensity, Lights->DayLamp);
	TestTrue(TEXT("...and it's neither dusk nor gloom"), !Lights->IsDusk() && !Lights->IsGloom());
	if (UMaterialInstanceDynamic* Windows = Lights->GetWindows())
	{
		TestEqual(TEXT("...and the windows"), Windows->K2_GetScalarParameterValue(Lights->GlowParameter), Lights->DayGlow);
	}

	// A house that sets no Gloom values (the depot, the workshop, Whitlock's barn) is lit in the smog as it always was.
	Lights->ApplyState(FName(TEXT("Smog")));
	TestEqual(TEXT("In the smog, with no gloom values: the day lamp"), Lights->Lamp->Intensity, Lights->DayLamp);
	if (UMaterialInstanceDynamic* Windows = Lights->GetWindows())
	{
		TestEqual(TEXT("...and the day glow"), Windows->K2_GetScalarParameterValue(Lights->GlowParameter), Lights->DayGlow);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHouseLightsKindsTest, "Looter.World.HouseLights.Kinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHouseLightsKindsTest::RunTest(const FString& Parameters)
{
	// One helper names how lit the town is in a state, for the lamps, the window life and the air: the bug it ends was three
	// places that each knew only "Dusk", so Mains 6-7 (DuskSmog) left every lamp of the town out.
	using namespace LightingStates;
	TestTrue(TEXT("Day is Day"), Kind(FName(TEXT("Day"))) == ELightKind::Day);
	TestTrue(TEXT("A level without states (no name) is Day"), Kind(NAME_None) == ELightKind::Day);
	TestTrue(TEXT("A state the town has no rule for is Day"), Kind(FName(TEXT("Night"))) == ELightKind::Day);
	TestTrue(TEXT("Smog is Gloom (Mains 1-6)"), Kind(FName(TEXT("Smog"))) == ELightKind::Gloom);
	TestTrue(TEXT("SmogThin is Gloom (after Main 5)"), Kind(FName(TEXT("SmogThin"))) == ELightKind::Gloom);
	TestTrue(TEXT("Dusk is Dusk"), Kind(FName(TEXT("Dusk"))) == ELightKind::Dusk);
	TestTrue(TEXT("DuskSmog is lit as Dusk (Mains 6-7)"), Kind(FName(TEXT("DuskSmog"))) == ELightKind::Dusk);
	TestTrue(TEXT("...names ignore case, as the console's do"), Kind(FName(TEXT("dusksmog"))) == ELightKind::Dusk
		&& Kind(FName(TEXT("SMOG"))) == ELightKind::Gloom);
	for (const TCHAR* State : { TEXT("Day"), TEXT("Smog"), TEXT("SmogThin"), TEXT("Dusk"), TEXT("DuskSmog") })
	{
		TestTrue(FString::Printf(TEXT("%s has a kind with a name"), State), FCString::Strlen(KindName(Kind(FName(State)))) > 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHouseLightsTableTest, "Looter.World.HouseLights.Table",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHouseLightsTableTest::RunTest(const FString& Parameters)
{
	// The towns plan's lights table (Docs/Work/2026-11-04-Towns/PLAN.md 4.1): each state lights what the table says.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();

	// Street lamps (LampPost, _B, _C): hidden by day, 3 cd in the smog, 8 at dusk; DuskSmog is Dusk.
	AHouseLights* Street = SpawnLamp(World, FVector(0.0, 0.0, 220.0), 0.f, 3.f, 8.f, 650.f);
	// Porch lanterns: hidden by day, 2 in the smog, 4 at dusk.
	AHouseLights* Porch = SpawnLamp(World, FVector(500.0, 0.0, 220.0), 0.f, 2.f, 4.f, 350.f);
	if (!TestNotNull(TEXT("A street lamp"), Street) || !TestNotNull(TEXT("A porch lantern"), Porch))
	{
		return false;
	}
	const FExpected StreetBy[] = { {0.f}, {3.f}, {3.f}, {8.f}, {8.f} };
	const FExpected PorchBy[] = { {0.f}, {2.f}, {2.f}, {4.f}, {4.f} };
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(States)); ++Index)
	{
		CheckState(*this, Street, TEXT("Street lamp"), States[Index], StreetBy[Index], false);
		CheckState(*this, Porch, TEXT("Porch lantern"), States[Index], PorchBy[Index], false);
	}

	// The town light rules: no shadow, no light function, no translucency lighting, drawn out to 45 m, a warm fire of a lamp.
	for (const AHouseLights* Each : { Street, Porch })
	{
		const UPointLightComponent* Lamp = Each->Lamp;
		TestFalse(TEXT("No shadow"), Lamp->CastShadows != 0);
		TestTrue(TEXT("No light function"), Lamp->LightFunctionMaterial == nullptr);
		TestFalse(TEXT("No lighting of translucent things"), Lamp->bAffectTranslucentLighting != 0);
		TestEqual(TEXT("Drawn out to 45 m"), Lamp->MaxDrawDistance, 4500.f);
		TestEqual(TEXT("...fading over the last 10 m"), Lamp->MaxDistanceFadeRange, 1000.f);
		TestTrue(TEXT("Warm"), Lamp->bUseTemperature != 0 && Lamp->Temperature < 3000.f);
	}
	TestTrue(TEXT("The street lamp reaches 6.5 m at most"), Street->Lamp->AttenuationRadius <= 650.f);
	TestTrue(TEXT("The porch lantern 3.5 m"), Porch->Lamp->AttenuationRadius <= 450.f);

	// In the smog (the 120 fps line: MainStreet 8.37 ms with the lamps at full reach) a dim lamp is a small pool, drawn near only.
	Street->ApplyState(FName(TEXT("Smog")));
	TestEqual(TEXT("A street lamp in the smog reaches 4 m"), Street->Lamp->AttenuationRadius, 400.f);
	TestEqual(TEXT("...is drawn out to 25 m"), Street->Lamp->MaxDrawDistance, AHouseLights::GloomDrawDistance);
	TestEqual(TEXT("...fading over 8 m"), Street->Lamp->MaxDistanceFadeRange, AHouseLights::GloomFadeRange);
	Porch->ApplyState(FName(TEXT("SmogThin")));
	TestTrue(TEXT("A porch lantern keeps its own, shorter reach"), Porch->Lamp->AttenuationRadius == 350.f && Porch->Lamp->MaxDrawDistance == AHouseLights::GloomDrawDistance);
	Street->ApplyState(FName(TEXT("DuskSmog")));
	TestTrue(TEXT("Back at dusk: the full reach and 45 m"), Street->Lamp->AttenuationRadius == 650.f
		&& Street->Lamp->MaxDrawDistance == AHouseLights::LampDrawDistance && Street->Lamp->MaxDistanceFadeRange == AHouseLights::LampFadeRange);

	// Lived-in homes (the farmhouse's windows): the hall lamp keeps its day value in the smog, the windows warm up (4); a
	// shop (the store, the sheriff, the undertaker) glows 6; at dusk both are the dusk values.
	AStaticMeshActor* HomeMesh = World->SpawnActor<AStaticMeshActor>();
	AStaticMeshActor* ShopMesh = World->SpawnActor<AStaticMeshActor>();
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Buildings/SM_Farmhouse_Ransom.SM_Farmhouse_Ransom"));
	if (!TestNotNull(TEXT("SM_Farmhouse_Ransom exists"), Mesh) || !TestNotNull(TEXT("Houses"), HomeMesh) || !TestNotNull(TEXT("..."), ShopMesh))
	{
		return false;
	}
	HomeMesh->GetStaticMeshComponent()->SetStaticMesh(Mesh);
	ShopMesh->GetStaticMeshComponent()->SetStaticMesh(Mesh);
	AHouseLights* Home = SpawnLamp(World, FVector(1000.0, 0.0, 200.0), 3.f, -1.f, 16.f, 400.f);
	AHouseLights* Shop = SpawnLamp(World, FVector(1500.0, 0.0, 200.0), 3.5f, -1.f, 30.f, 350.f);
	if (!TestNotNull(TEXT("A lived-in home"), Home) || !TestNotNull(TEXT("A shop"), Shop))
	{
		return false;
	}
	Home->House = HomeMesh;
	Shop->House = ShopMesh;
	Home->DispatchBeginPlay();
	Shop->DispatchBeginPlay();
	Home->GloomGlow = 4.f;
	Shop->GloomGlow = 6.f;
	if (!TestNotNull(TEXT("The home's windows"), Home->GetWindows()) || !TestNotNull(TEXT("The shop's windows"), Shop->GetWindows()))
	{
		return false;
	}
	const FExpected HomeBy[] = { {3.f, 1.f}, {3.f, 4.f}, {3.f, 4.f}, {16.f, 14.f}, {16.f, 14.f} };
	const FExpected ShopBy[] = { {3.5f, 1.f}, {3.5f, 6.f}, {3.5f, 6.f}, {30.f, 14.f}, {30.f, 14.f} };
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(States)); ++Index)
	{
		CheckState(*this, Home, TEXT("Lived-in home"), States[Index], HomeBy[Index], true);
		CheckState(*this, Shop, TEXT("Shop"), States[Index], ShopBy[Index], true);
	}

	// A lamp with no Gloom value of its own (a hall lamp) is lit in the smog as it always was: its reach, 45 m.
	Home->ApplyState(FName(TEXT("Smog")));
	TestTrue(TEXT("A hall lamp in the smog: its own reach, drawn to 45 m"), Home->Lamp->AttenuationRadius == 400.f
		&& Home->Lamp->MaxDrawDistance == AHouseLights::LampDrawDistance);

	// DuskSmog is lit exactly as Dusk, and the smog is not dusk.
	Home->ApplyState(FName(TEXT("DuskSmog")));
	TestTrue(TEXT("In DuskSmog the home is lit for dusk"), Home->IsDusk() && Home->GetKind() == ELightKind::Dusk);
	Home->ApplyState(FName(TEXT("Smog")));
	TestTrue(TEXT("In the smog it is Gloom, not dusk"), Home->IsGloom() && !Home->IsDusk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHouseLightsBudgetTest, "Looter.World.HouseLights.Budget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FHouseLightsBudgetTest::RunTest(const FString& Parameters)
{
	// The towns plan's count (4.1): the worst view, Main Street from its middle, has 16 lights within 45 m today (9 street
	// lamps, 2 porch lanterns, 5 house lamps); the plan adds porch lamps on 3 lived homes and the boarding house, 2 Pen Lane
	// lamps and 2 fires: 24, never more than 26 in any view, and a lamp past 45 m is no part of the view.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	constexpr int32 MaxInView = 26;
	TArray<AHouseLights*> Made;
	auto Add = [&Made, World](const FVector& Where, float Day, float Gloom, float Dusk, float Reach)
	{
		Made.Add(SpawnLamp(World, Where, Day, Gloom, Dusk, Reach));
	};
	// Today: nine street lamps down the street (every 9 m), two porch lanterns, five house lamps (sheriff, Haskett, Mercer, store, undertaker).
	for (int32 Index = 0; Index < 9; ++Index)
	{
		Add(FVector(-4000.0 + 900.0 * Index, Index % 2 ? 500.0 : -500.0, 220.0), 0.f, 3.f, 8.f, 650.f);
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Add(FVector(-1500.0 + 1800.0 * Index, 1100.0, 200.0), 0.f, 2.f, 4.f, 350.f);
	}
	const float HouseDay[] = { 1.5f, 3.f, 3.f, 3.5f, 1.75f };
	const float HouseDusk[] = { 6.f, 16.f, 16.f, 30.f, 5.5f };
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Add(FVector(-3000.0 + 1500.0 * Index, 1500.0, 180.0), HouseDay[Index], -1.f, HouseDusk[Index], 350.f);
	}
	const int32 Today = Made.Num();
	TestEqual(TEXT("Main Street's lights today"), Today, 16);
	// The plan's additions: porch lamps (3 lived homes, the boarding house), two Pen Lane lamps.
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Add(FVector(-2500.0 + 1700.0 * Index, -1300.0, 200.0), 0.f, 2.f, 4.f, 350.f);
	}
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Add(FVector(-1000.0 + 2000.0 * Index, -2200.0, 220.0), 0.f, 3.f, 8.f, 650.f);
	}
	// One lamp well past 45 m, out of this view.
	Add(FVector(11000.0, 0.0, 220.0), 0.f, 3.f, 8.f, 650.f);
	// Two fires: the Company's barrel fire at the gate (not by day) and a forge (also by day).
	ATownFire* Barrel = World->SpawnActor<ATownFire>(FVector(-3800.0, -1800.0, 100.0), FRotator::ZeroRotator);
	ATownFire* Forge = World->SpawnActor<ATownFire>(FVector(2800.0, -1800.0, 100.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("The barrel fire"), Barrel) || !TestNotNull(TEXT("The forge"), Forge))
	{
		return false;
	}
	Forge->bBurnsByDay = true;
	for (const AHouseLights* Each : Made)
	{
		if (!TestNotNull(TEXT("A lamp"), Each))
		{
			return false;
		}
	}

	const FVector Eyes(0.0, 0.0, 170.0);
	struct FBudget
	{
		const TCHAR* State;
		int32 Lit;
	};
	// Day: the five house lamps (a glimmer) and the forge; the streets, the porches and the barrel fire are out.
	// Dusk: all 24 (the 16, the 6 added lamps, the 2 fires) and not the far one.
	// The smog: the dim street and porch lamps are drawn to 25 m only. Within 25 m of the middle are 6 of the 9 street lamps, both
	// lanterns, 2 of the 4 added porch lamps and both Pen Lane lamps (12); the 5 house lamps and the 2 fires keep 45 m: 19 in all.
	const FBudget Budgets[] = { { TEXT("Day"), 6 }, { TEXT("Smog"), 19 }, { TEXT("SmogThin"), 19 }, { TEXT("Dusk"), 24 }, { TEXT("DuskSmog"), 24 } };
	for (const FBudget& Each : Budgets)
	{
		for (AHouseLights* Lamp : Made)
		{
			Lamp->ApplyState(FName(Each.State));
		}
		Barrel->ApplyState(FName(Each.State));
		Forge->ApplyState(FName(Each.State));
		const int32 Counted = ATownFire::CountLitInView(World, Eyes);
		TestEqual(FString::Printf(TEXT("Lit lights in view in %s"), Each.State), Counted, Each.Lit);
		TestTrue(FString::Printf(TEXT("...never more than %d (%d)"), MaxInView, Counted), Counted <= MaxInView);
	}
	return true;
}

#endif
