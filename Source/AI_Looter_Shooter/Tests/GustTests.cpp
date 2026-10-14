#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/GustClock.h"
#include "World/GustSubsystem.h"
#include "World/SwingComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

namespace GustTestKit
{
	/** One frame of the gust tests (s). */
	inline constexpr float Frame = 1.f / 30.f;

	/** Looter.Air set for a test (the gusts are still while it's 0), and put back after it. */
	class FAirSetting
	{
	public:
		explicit FAirSetting(int32 Wanted)
			: Variable(IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Air")))
		{
			if (Variable)
			{
				Previous = Variable->GetInt();
				Variable->Set(Wanted, ECVF_SetByConsole);
			}
		}

		~FAirSetting()
		{
			if (Variable)
			{
				Variable->Set(Previous, ECVF_SetByConsole);
			}
		}

	private:
		IConsoleVariable* Variable = nullptr;
		int32 Previous = 1;
	};
}

using namespace GustTestKit;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGustScheduleTest, "Looter.World.Gust.Schedule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGustScheduleTest::RunTest(const FString& Parameters)
{
	// Gusts come in their interval (the first sooner), each from the prevailing quarter, as strong and as long as the rules
	// say, pushing only while they blow, and the same seed plays the same gusts.
	auto Play = [](int32 Seed, float Seconds, TArray<double>& OutStarts, TArray<FGustEvent>& OutGusts, bool& bPushOk)
	{
		FGustClock Clock;
		Clock.Reset(Seed, 19.f);
		bPushOk = true;
		double Now = 0.0;
		while (Now < Seconds)
		{
			const bool bStarted = Clock.Advance(Frame);
			Now += Frame;
			if (bStarted)
			{
				OutStarts.Add(Now - Clock.GetAge());
				OutGusts.Add(Clock.GetCurrent());
			}
			const float Push = Clock.GetStrengthNow();
			bPushOk &= Clock.IsBlowing() ? (Push >= 0.f && Push <= Clock.GetCurrent().Strength + KINDA_SMALL_NUMBER) : Push == 0.f;
		}
		return Clock.GetCount();
	};

	TArray<double> Starts;
	TArray<FGustEvent> Gusts;
	bool bPushOk = false;
	const int32 Count = Play(1234, 2000.f, Starts, Gusts, bPushOk);
	TestEqual(TEXT("Every gust begun is counted"), Count, Starts.Num());
	TestTrue(TEXT("Over 2000 s, between one each 120 s and one each 40 s"), Count >= 2000 / 120 - 1 && Count <= 2000 / 40 + 1);
	if (!TestTrue(TEXT("Gusts came"), Starts.Num() > 2))
	{
		return false;
	}
	const double Slack = Frame + 0.001;
	TestTrue(TEXT("The first comes 20 to 60 s in"), Starts[0] >= FGustClock::FirstIntervalMin - Slack && Starts[0] <= FGustClock::FirstIntervalMax + Slack);
	bool bGapsOk = true;
	for (int32 Index = 1; Index < Starts.Num(); ++Index)
	{
		const double Gap = Starts[Index] - Starts[Index - 1];
		bGapsOk &= Gap >= FGustClock::IntervalMin - Slack && Gap <= FGustClock::IntervalMax + Slack;
	}
	TestTrue(TEXT("...then one every 40 to 120 s, start to start"), bGapsOk);
	bool bEachOk = true;
	for (const FGustEvent& Gust : Gusts)
	{
		const float Off = static_cast<float>(FMath::Abs(FRotator::NormalizeAxis(Gust.Direction.Rotation().Yaw - 19.0)));
		bEachOk &= Gust.Strength >= FGustClock::StrengthMin && Gust.Strength <= FGustClock::StrengthMax;
		bEachOk &= Gust.Duration >= FGustClock::DurationMin && Gust.Duration <= FGustClock::DurationMax;
		bEachOk &= Off <= FGustClock::DirectionVariance + 0.01f && FMath::IsNearlyEqual(Gust.Direction.Size(), 1.0, 0.001);
		bEachOk &= FMath::IsNearlyZero(Gust.Direction.Z);
	}
	TestTrue(TEXT("Each 0.4 to 1 strong, 2 to 5 s long, level, from within 25 degrees of the prevailing wind"), bEachOk);
	TestTrue(TEXT("A gust pushes only while it blows, never past its strength"), bPushOk);

	// Seeded: the same seed plays the same gusts; another seed others.
	TArray<double> Again, Other;
	TArray<FGustEvent> GustsAgain, GustsOther;
	bool bIgnored = false;
	Play(1234, 2000.f, Again, GustsAgain, bIgnored);
	Play(98765, 2000.f, Other, GustsOther, bIgnored);
	TestTrue(TEXT("The same seed: the same gusts at the same times"), Again == Starts && GustsAgain.Num() == Gusts.Num()
		&& GustsAgain[0].Strength == Gusts[0].Strength && GustsAgain.Last().Duration == Gusts.Last().Duration);
	TestTrue(TEXT("Another seed: other times"), Other.Num() == 0 || Other[0] != Starts[0]);

	// Its rise and fall: nothing at its ends, full in its middle, quick to come and slow to go.
	TestEqual(TEXT("Nothing as it begins"), FGustClock::Shape(0.f, 4.f), 0.f);
	TestEqual(TEXT("Full a third of the way in"), FGustClock::Shape(1.6f, 4.f), 1.f);
	TestEqual(TEXT("Nothing as it ends"), FGustClock::Shape(4.f, 4.f), 0.f);
	TestTrue(TEXT("Up quicker than down"), FGustClock::Shape(0.6f, 4.f) > FGustClock::Shape(3.4f, 4.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGustBroadcastTest, "Looter.World.Gust.Broadcast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGustBroadcastTest::RunTest(const FString& Parameters)
{
	// The subsystem tells every gust once as it begins, pushes while it blows and not after, starts one on demand (never on
	// top of one blowing), and is still while the air is off.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UGustSubsystem* Gusts = World->GetSubsystem<UGustSubsystem>();
	if (!TestNotNull(TEXT("The gusts run in test levels"), Gusts))
	{
		return false;
	}
	FAirSetting AirOn(1);
	Gusts->ResetClock(77);
	int32 Heard = 0;
	FGustEvent LastHeard;
	const FDelegateHandle Handle = Gusts->OnGust.AddLambda([&Heard, &LastHeard](const FGustEvent& Gust)
	{
		++Heard;
		LastHeard = Gust;
	});

	int32 Begun = 0;
	bool bPushOk = true;
	bool bHeardAsBegun = true;
	for (float Done = 0.f; Done < 600.f; Done += Frame)
	{
		const int32 Before = Gusts->GetClock().GetCount();
		const int32 HeardBefore = Heard;
		Gusts->Advance(Frame);
		const int32 New = Gusts->GetClock().GetCount() - Before;
		Begun += New;
		bHeardAsBegun &= Heard - HeardBefore == New;
		if (New > 0)
		{
			bHeardAsBegun &= LastHeard.Strength == Gusts->GetClock().GetCurrent().Strength;
		}
		bPushOk &= Gusts->IsBlowing() ? Gusts->GetStrengthNow() >= 0.f : Gusts->GetStrengthNow() == 0.f;
	}
	TestTrue(TEXT("Ten minutes bring five gusts or more"), Begun >= 5);
	TestEqual(TEXT("...each told once"), Heard, Begun);
	TestTrue(TEXT("...as it begins, with the gust that began"), bHeardAsBegun);
	TestTrue(TEXT("Its push is there only while one blows"), bPushOk);

	// One on demand: told at once, never on top of one blowing.
	while (Gusts->IsBlowing())
	{
		Gusts->Advance(Frame);
	}
	const int32 HeardBefore = Heard;
	TestTrue(TEXT("A gust on demand starts"), Gusts->Trigger(0.8f, 3.f));
	TestEqual(TEXT("...told at once"), Heard, HeardBefore + 1);
	TestTrue(TEXT("...as asked"), FMath::IsNearlyEqual(LastHeard.Strength, 0.8f) && FMath::IsNearlyEqual(LastHeard.Duration, 3.f));
	TestFalse(TEXT("No second one on top of it"), Gusts->Trigger(1.f, 2.f));
	float Strongest = 0.f;
	for (float Done = 0.f; Done < 3.5f; Done += Frame)
	{
		Gusts->Advance(Frame);
		Strongest = FMath::Max(Strongest, Gusts->GetStrengthNow());
	}
	TestTrue(TEXT("It rose near its strength"), Strongest > 0.7f && Strongest <= 0.8f + KINDA_SMALL_NUMBER);
	TestFalse(TEXT("...and was over after its time"), Gusts->IsBlowing());
	TestEqual(TEXT("...with nothing left pushing"), Gusts->GetStrengthNow(), 0.f);

	// The air off: the clock runs on, but nothing is told and nothing pushes.
	{
		FAirSetting AirOff(0);
		const int32 HeardOff = Heard;
		const int32 CountOff = Gusts->GetClock().GetCount();
		for (float Done = 0.f; Done < 300.f; Done += Frame)
		{
			Gusts->Advance(Frame);
			bPushOk &= Gusts->GetStrengthNow() == 0.f;
		}
		TestTrue(TEXT("With Looter.Air 0 gusts still come by the clock"), Gusts->GetClock().GetCount() > CountOff);
		TestEqual(TEXT("...but none is told"), Heard, HeardOff);
		TestTrue(TEXT("...and none pushes"), bPushOk);
	}
	Gusts->OnGust.Remove(Handle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGustSwingSettlesTest, "Looter.World.Gust.SwingSettles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGustSwingSettlesTest::RunTest(const FString& Parameters)
{
	// A hanging sign on a swing: a gust swings its bottom downwind (as squarely as it faces it), never past its hooks; it
	// swings back and settles once the gust has passed; with the player near it sways a little in the breeze, no more.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AActor* Bracket = World->SpawnActor<AActor>();
	USwingComponent* Swing = Bracket ? NewObject<USwingComponent>(Bracket, TEXT("Swing")) : nullptr;
	if (!TestNotNull(TEXT("A swing on a bracket"), Swing))
	{
		return false;
	}
	Bracket->SetRootComponent(Swing);
	Swing->RegisterComponent();
	// A sign facing along X, hung from a bar along Y.
	Swing->HingeAxis = FVector(0.0, 1.0, 0.0);
	Swing->PendulumLength = 60.f;
	TestTrue(TEXT("Its bottom swings across the bar (along X)"), FMath::Abs(Swing->GetSwingDirection().X) > 0.99);

	constexpr float Sixtieth = 1.f / 60.f;
	const FVector Down(0.0, 0.0, -Swing->PendulumLength);
	auto Bottom = [Swing, &Down]()
	{
		return FQuat(Swing->HingeAxis.GetSafeNormal(), FMath::DegreesToRadians(Swing->GetAngle())).RotateVector(Down);
	};

	FGustEvent Gust;
	Gust.Direction = FVector(1.0, 0.0, 0.0);
	Gust.Strength = 1.f;
	Gust.Duration = 3.f;
	Swing->SetNear(false);
	Swing->Kick(Gust);
	TestTrue(TEXT("A gust wakes it"), Swing->IsComponentTickEnabled());
	float Widest = 0.f;
	double Downwind = 0.0;
	for (float Done = 0.f; Done < 3.f; Done += Sixtieth)
	{
		Swing->Step(Sixtieth);
		Widest = FMath::Max(Widest, FMath::Abs(Swing->GetAngle()));
		if (Done < 1.f)
		{
			Downwind = FMath::Max(Downwind, FVector::DotProduct(Bottom(), Gust.Direction));
		}
	}
	TestTrue(TEXT("The gust swings its bottom downwind"), Downwind > 5.0);
	TestTrue(TEXT("...a good way (degrees)"), Widest > 8.f);
	TestTrue(TEXT("...never past its hooks"), Widest <= Swing->MaxDegrees + KINDA_SMALL_NUMBER);
	for (float Done = 0.f; Done < 40.f; Done += Sixtieth)
	{
		Swing->Step(Sixtieth);
	}
	TestTrue(TEXT("Long after, with nobody near, it hangs still"), FMath::Abs(Swing->GetAngle()) < 0.1f && Swing->IsSettled());

	// Edge-on to the wind it hardly moves.
	USwingComponent* EdgeOn = NewObject<USwingComponent>(Bracket, TEXT("EdgeOn"));
	EdgeOn->SetupAttachment(Swing);
	EdgeOn->RegisterComponent();
	EdgeOn->HingeAxis = FVector(1.0, 0.0, 0.0);
	EdgeOn->Kick(Gust);
	float EdgeWidest = 0.f;
	for (float Done = 0.f; Done < 3.f; Done += Sixtieth)
	{
		EdgeOn->Step(Sixtieth);
		EdgeWidest = FMath::Max(EdgeWidest, FMath::Abs(EdgeOn->GetAngle()));
	}
	TestTrue(TEXT("A sign edge-on to the gust hardly swings"), EdgeWidest < 1.f);

	// The player near: a slight sway in the breeze, never settling while they stay.
	Swing->SetNear(true);
	float Sway = 0.f;
	for (float Done = 0.f; Done < 20.f; Done += Sixtieth)
	{
		Swing->Step(Sixtieth);
		if (Done > 10.f)
		{
			Sway = FMath::Max(Sway, FMath::Abs(Swing->GetAngle()));
		}
	}
	TestTrue(TEXT("Near the player it sways a little"), Sway > 0.2f);
	TestTrue(TEXT("...no more than its idle sway (with a little overshoot)"), Sway <= Swing->IdleDegrees * 1.6f);
	TestTrue(TEXT("...and stays awake while they're near"), Swing->IsNear() && Swing->IsComponentTickEnabled());
	return true;
}

#endif
