#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/Animation/BodySwing.h"

// Crane's toll-chain hook against Ellis's lantern (FBodySwing::SettingsFor "hook" and "lantern"), as plain math: the
// hook swings less and settles sooner when the body starts and stops.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingHookTest, "Looter.Player.BodySwing.Hook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingHookTest::RunTest(const FString& Parameters)
{
	// A 22 cm hook hanging from a body that runs off at 450 cm/s in 0.15 s (about 3 g, as play moves it), runs, and stops as
	// sharply: with the lantern's settings it swings out near level; the hook's swing far less and settle.
	const FVector Start(0.0, 0.0, 150.0);
	const float Length = 22.f;
	auto Swing = [&](const FBodySwingSettings& Settings, float& OutSettled)
	{
		FBodySwingPoints Sim;
		float Widest = 0.f;
		OutSettled = 0.f;
		const float Fps = 60.f;
		FVector Anchor = Start;
		float Speed = 0.f;
		for (int32 Frame = 0; Frame < 180; ++Frame)
		{
			const float Time = static_cast<float>(Frame) / Fps;
			// Off from 0.2 s, at full speed by 0.35 s, stopping from 1.2 s to 1.35 s.
			const float Target = Time >= 0.2f && Time < 1.2f ? 450.f : 0.f;
			Speed = FMath::FInterpConstantTo(Speed, Target, 1.f / Fps, 3000.f);
			Anchor.X += Speed / Fps;
			const TArray<FVector> Targets({ Anchor + FVector(0.0, 0.0, -Length) });
			Sim.Step(Anchor, Targets, TArray<float>({ Length }), {}, Settings, 1.f / Fps);
			const FVector Hang = (Sim.Points[0] - Anchor).GetSafeNormal();
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(static_cast<float>(-Hang.Z), -1.f, 1.f)));
			Widest = FMath::Max(Widest, Angle);
			if (Time >= 2.7f)
			{
				OutSettled = FMath::Max(OutSettled, Angle);
			}
		}
		return Widest;
	};
	float LanternLeft = 0.f;
	float HookLeft = 0.f;
	const float LanternWidest = Swing(FBodySwing::SettingsFor(TEXT("lantern")), LanternLeft);
	const float HookWidest = Swing(FBodySwing::SettingsFor(TEXT("hook")), HookLeft);
	TestTrue(FString::Printf(TEXT("The hook swings at most half the lantern's way out (%.0f against %.0f degrees)"), HookWidest, LanternWidest),
		HookWidest < LanternWidest * 0.5f);
	TestTrue(FString::Printf(TEXT("...and still swings (%.0f degrees)"), HookWidest), HookWidest > 5.f);
	TestTrue(FString::Printf(TEXT("...and has settled a second and a half after the stop (%.1f degrees off)"), HookLeft), HookLeft < 3.f);
	return true;
}

#endif
