#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/HouseLights.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHouseLightsTest, "Looter.World.HouseLights",
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
	if (HouseMesh && TestNotNull(TEXT("The windows have an instance of their own (the WindowGlow slot)"), Lights->GetWindows()))
	{
		TestEqual(TEXT("...and the windows glow"), Lights->GetWindows()->K2_GetScalarParameterValue(Lights->GlowParameter), Lights->DuskGlow);
	}

	Lights->ApplyState(FName(TEXT("Day")));
	TestEqual(TEXT("Back to day: the lamp"), Lights->Lamp->Intensity, Lights->DayLamp);
	if (UMaterialInstanceDynamic* Windows = Lights->GetWindows())
	{
		TestEqual(TEXT("...and the windows"), Windows->K2_GetScalarParameterValue(Lights->GlowParameter), Lights->DayGlow);
	}
	return true;
}

#endif
