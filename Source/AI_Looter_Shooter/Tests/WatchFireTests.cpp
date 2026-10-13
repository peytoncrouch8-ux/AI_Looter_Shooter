#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "World/FarSignalLight.h"
#include "World/WatchFire.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

namespace
{
	const FName LastWatchId(TEXT("LastWatch"));
	const FName PostArea(TEXT("TestRim"));
	/** The Last Watch's five steps (from 0): clear the yard, the oil, light the fire, keep the post, read the answer. */
	constexpr int32 LastWatchSteps = 5;
	constexpr int32 LightStep = 2;
	constexpr int32 AnswerStep = 4;

	/** A test level playing The Last Watch (five steps that never finish by themselves) against Campaign. */
	struct FPostLevel
	{
		FTestWorldWrapper Level;
		UWorld* World = nullptr;
		UMissionRunner* Runner = nullptr;
		AActor* Player = nullptr;

		bool Build(FCampaignRecord& Campaign)
		{
			if (!Level.CreateTestWorld(EWorldType::EditorPreview))
			{
				return false;
			}
			World = Level.GetTestWorld();
			Runner = World->GetSubsystem<UMissionRunner>();
			Player = MissionTestWorld::SpawnMarker(World, FVector(-500.0, 0.0, 100.0));
			if (!Runner || !Player)
			{
				return false;
			}
			UPackage* Scratch = CreatePackage(nullptr);
			UMissionDefinition* LastWatch = MissionTestWorld::NewMission(Scratch, TEXT("LastWatch"), EMissionKind::Side, EMissionStart::Manual, PostArea);
			for (int32 Step = 0; Step < LastWatchSteps; ++Step)
			{
				MissionTestWorld::AddObjective<UMissionEventObjective>(LastWatch, Step)->Event = TEXT("Test.NeverHappens");
			}
			Runner->BeginForTesting({ LastWatch }, Campaign, Player, PostArea);
			return true;
		}

		template <typename T>
		T* Spawn(const FVector& Where)
		{
			T* Made = World->SpawnActor<T>(Where, FRotator::ZeroRotator);
			if (Made)
			{
				Made->DispatchBeginPlay();
			}
			return Made;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWatchFireLitTest, "Looter.WatchFire.Lit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWatchFireLitTest::RunTest(const FString& Parameters)
{
	// The fire's look as plain math: an unsteady flicker about 1, a flare that swells and is gone, flames catching.
	float Lowest = 2.f;
	float Highest = 0.f;
	for (float Seconds = 0.f; Seconds < 10.f; Seconds += 0.01f)
	{
		Lowest = FMath::Min(Lowest, AWatchFire::Flicker(Seconds));
		Highest = FMath::Max(Highest, AWatchFire::Flicker(Seconds));
	}
	TestTrue(TEXT("The flicker stays within a fifth of steady"), Lowest > 0.8f && Highest < 1.2f && Highest - Lowest > 0.1f);
	TestTrue(TEXT("No flare before it's lit, a full one a quarter second in, none by 2 s"),
		AWatchFire::Flare(-1.f) == 0.f && FMath::IsNearlyEqual(AWatchFire::Flare(0.25f), 1.f, 0.01f) && AWatchFire::Flare(2.f) == 0.f);
	TestTrue(TEXT("The flames catch from nothing over 0.6 s"), AWatchFire::Catch(0.f) == 0.f && AWatchFire::Catch(1.f) == 1.f);

	FCampaignRecord Campaign;
	FPostLevel Post;
	if (!TestTrue(TEXT("Test level made"), Post.Build(Campaign)))
	{
		return false;
	}
	AWatchFire* Fire = Post.Spawn<AWatchFire>(FVector(0.0, 0.0, 1200.0));
	if (!TestNotNull(TEXT("The watch fire"), Fire))
	{
		return false;
	}
	TestFalse(TEXT("Dark before The Last Watch"), Fire->IsLit() || Fire->CanLight() || Fire->IsDryNow());
	TestFalse(TEXT("...and it can't be lit"), Fire->Light(Post.Player));

	Post.Runner->StartMission(LastWatchId, 1, /*bForce*/ true);
	TestTrue(TEXT("The Last Watch before the oil: the brazier's found dry"), Fire->IsDryNow() && !Fire->CanLight());
	TestFalse(TEXT("...and won't light"), Fire->Light(Post.Player));

	Post.Runner->SetStep(LastWatchId, LightStep);
	TestTrue(TEXT("With the oil, it can be lit"), Fire->CanLight() && !Fire->IsDryNow());
	TestTrue(TEXT("Held: it catches"), Fire->Light(Post.Player));
	TestTrue(TEXT("...lit, and kept by the story"), Fire->IsLit() && Campaign.HasFlag(Fire->LitFlag));
	TestTrue(TEXT("...used for good: nothing more to do at it"), Fire->IsUsedUp() && !Fire->CanLight() && !Fire->IsDryNow());
	Fire->Advance(0.5f);
	TestTrue(TEXT("It flares as it catches"), Fire->GetLightCandela() > Fire->LightCandela * 1.5f && Fire->GetFlameScale() > 1.f);
	Fire->Advance(2.f);
	const float Settled = Fire->GetLightCandela();
	TestTrue(TEXT("...then burns steadily, flickering"), Settled > Fire->LightCandela * 0.6f && Settled < Fire->LightCandela * 1.2f);

	// A later visit (the session loaded, the mission moved on or not): it burns from the start, no flare.
	FPostLevel Later;
	if (TestTrue(TEXT("A later level"), Later.Build(Campaign)))
	{
		AWatchFire* Again = Later.Spawn<AWatchFire>(FVector(0.0, 0.0, 1200.0));
		TestTrue(TEXT("Kept lit by its flag"), Again && Again->IsLit() && Again->GetLightCandela() > 0.f);
	}

	// An old save that lost the flag, its mission finished: it burns all the same.
	FCampaignRecord Finished;
	Finished.Complete(LastWatchId);
	FPostLevel After;
	if (TestTrue(TEXT("A level after The Last Watch"), After.Build(Finished)))
	{
		AWatchFire* Lit = After.Spawn<AWatchFire>(FVector(0.0, 0.0, 1200.0));
		TestTrue(TEXT("Lit after its mission without the flag"), Lit && Lit->IsLit());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFarSignalMorseTest, "Looter.FarSignalLight.Morse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFarSignalMorseTest::RunTest(const FString& Parameters)
{
	const int32 Pause = AFarSignalLight::RepeatGapUnits;
	// E is a dot, T a dash: one on, three off between letters, three on, then the pause before it starts again.
	const TArray<bool> ET = AFarSignalLight::KeyUnits(TEXT("ET"));
	if (TestEqual(TEXT("E then T: 1 + 3 + 3 units and the pause"), ET.Num(), 7 + Pause))
	{
		TestTrue(TEXT("...a dot, a letter's gap, a dash"), ET[0] && !ET[1] && !ET[2] && !ET[3] && ET[4] && ET[5] && ET[6] && !ET[7]);
	}
	// A: dot, a gap, dash; a word's gap is seven; anything but letters and digits is a word's gap.
	const TArray<bool> A = AFarSignalLight::KeyUnits(TEXT("A"));
	TestTrue(TEXT("A is a dot, a gap, a dash"), A.Num() == 5 + Pause && A[0] && !A[1] && A[2] && A[3] && A[4]);
	TestEqual(TEXT("Words apart by seven"), AFarSignalLight::KeyUnits(TEXT("E E")).Num(), 1 + 7 + 1 + Pause);
	TestEqual(TEXT("...and a dash between them is a word's gap"), AFarSignalLight::KeyUnits(TEXT("e-e")).Num(), 1 + 7 + 1 + Pause);
	TestEqual(TEXT("Nothing to key, nothing at all"), AFarSignalLight::KeyUnits(TEXT("--")).Num(), 0);

	// Keyed: a dash holds full in its middle, eases on at its start, dark between, and it all comes round again.
	const TArray<bool> T = AFarSignalLight::KeyUnits(TEXT("T"));
	const float Unit = 0.2f;
	TestEqual(TEXT("Dark as a wink starts"), AFarSignalLight::KeyAt(T, 0.f, Unit), 0.f);
	TestEqual(TEXT("Full in the dash's middle"), AFarSignalLight::KeyAt(T, 1.5f * Unit, Unit), 1.f);
	TestEqual(TEXT("Dark in the pause"), AFarSignalLight::KeyAt(T, 5.5f * Unit, Unit), 0.f);
	TestEqual(TEXT("...and round again"), AFarSignalLight::KeyAt(T, (T.Num() + 1.5f) * Unit, Unit), 1.f);

	// The post's answer winks often enough to read as a signal.
	const TArray<bool> Answer = AFarSignalLight::KeyUnits(TEXT("LARKSPUR WE SEE YOU"));
	const int32 On = Answer.FilterByPredicate([](bool bOn) { return bOn; }).Num();
	TestTrue(TEXT("The answer's light is on about a third to a half of the time"), Answer.Num() > 0 && On > Answer.Num() / 5 && On < Answer.Num() * 3 / 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFarSignalStoryTest, "Looter.FarSignalLight.Story",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFarSignalStoryTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FPostLevel Post;
	if (!TestTrue(TEXT("Test level made"), Post.Build(Campaign)))
	{
		return false;
	}
	AFarSignalLight* Light = Post.Spawn<AFarSignalLight>(FVector(-300000.0, 0.0, 5000.0));
	if (!TestNotNull(TEXT("The far light"), Light))
	{
		return false;
	}
	TestFalse(TEXT("Dark before The Last Watch"), Light->IsShown());
	Post.Runner->StartMission(LastWatchId, LightStep + 1, /*bForce*/ true);
	Light->RefreshStory();
	TestFalse(TEXT("Dark while the post is kept"), Light->IsShown());
	Post.Runner->SetStep(LastWatchId, AnswerStep);
	Light->RefreshStory();
	TestTrue(TEXT("The post down the Rim answers"), Light->IsShown());
	float Brightest = 0.f;
	float Darkest = 1.f;
	for (int32 Frame = 0; Frame < 300; ++Frame)
	{
		Light->Advance(1.f / 30.f);
		Brightest = FMath::Max(Brightest, Light->GetKey());
		Darkest = FMath::Min(Darkest, Light->GetKey());
	}
	TestTrue(TEXT("...winking on and off"), Brightest > 0.95f && Darkest < 0.05f);
	Post.Runner->CompleteMission(LastWatchId);
	Light->RefreshStory();
	TestTrue(TEXT("...and goes on answering after The Last Watch"), Light->IsShown());

	// Shown by a flag too.
	FCampaignRecord Flagged;
	Flagged.SetFlag(TEXT("Test.FarLight"));
	FPostLevel Other;
	if (TestTrue(TEXT("A second level"), Other.Build(Flagged)))
	{
		AFarSignalLight* ByFlag = Other.World->SpawnActor<AFarSignalLight>(FVector(-300000.0, 0.0, 5000.0), FRotator::ZeroRotator);
		if (TestNotNull(TEXT("A far light"), ByFlag))
		{
			ByFlag->ShownWhen.Reset();
			ByFlag->ShownFlag = TEXT("Test.FarLight");
			ByFlag->DispatchBeginPlay();
			TestTrue(TEXT("Its flag shows it"), ByFlag->IsShown());
		}
	}
	return true;
}

#endif
