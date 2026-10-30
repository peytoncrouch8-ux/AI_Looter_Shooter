#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/FoliageBlastSubsystem.h"

// The blast through the grass (UFoliageBlastSubsystem::StateAt): its front rolls out to its reach while it blows at full
// strength, then the plants spring back past upright and settle to nothing.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFoliageBlastShapeTest, "Looter.World.FoliageBlast.Shape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFoliageBlastShapeTest::RunTest(const FString& Parameters)
{
	constexpr float Radius = 690.f;
	constexpr float Grow = 0.6f;

	const FFoliageBlastState Start = UFoliageBlastSubsystem::StateAt(0.f, Radius, Grow, 1.f);
	TestEqual(TEXT("It starts at its middle"), Start.Front, 0.f);
	TestEqual(TEXT("It blows at full strength from the start"), Start.Strength, 1.f);

	const FFoliageBlastState Half = UFoliageBlastSubsystem::StateAt(0.3f, Radius, Grow, 1.f);
	TestTrue(TEXT("Halfway through its roll the front is past half its reach (it slows as it spends itself)"), Half.Front > Radius * 0.5f && Half.Front < Radius);

	const FFoliageBlastState Reached = UFoliageBlastSubsystem::StateAt(Grow, Radius, Grow, 0.8f);
	TestEqual(TEXT("At its reach the front stops"), Reached.Front, Radius);
	TestEqual(TEXT("And still pushes at its strength"), Reached.Strength, 0.8f);

	bool bSwungPast = false;
	for (float After = 0.f; After < UFoliageBlastSubsystem::SettleSeconds; After += 0.02f)
	{
		bSwungPast |= UFoliageBlastSubsystem::StateAt(Grow + After, Radius, Grow, 1.f).Strength < -0.05f;
	}
	TestTrue(TEXT("The plants spring back a little past upright"), bSwungPast);

	const FFoliageBlastState Done = UFoliageBlastSubsystem::StateAt(Grow + UFoliageBlastSubsystem::SettleSeconds, Radius, Grow, 1.f);
	TestEqual(TEXT("Then they are still"), Done.Strength, 0.f);
	TestEqual(TEXT("No blast without strength"), UFoliageBlastSubsystem::StateAt(0.1f, Radius, Grow, 0.f).Strength, 0.f);
	return true;
}

#endif
