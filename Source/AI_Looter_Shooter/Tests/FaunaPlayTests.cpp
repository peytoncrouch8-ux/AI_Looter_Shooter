#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FaunaTestWorld.h"
#include "World/FaunaFlock.h"
#include "World/FaunaRules.h"
#include "World/FaunaSubsystem.h"
#include "World/MinimapSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

using namespace FaunaTestWorld;

namespace
{
	/** A fence rail of Count perches 50 cm apart along +Y at 1.2 m, facing across it (+X). */
	TArray<FFaunaPerch> Rail(int32 Count)
	{
		TArray<FFaunaPerch> Perches;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FFaunaPerch& Perch = Perches.AddDefaulted_GetRef();
			Perch.Location = FVector(0.0, Index * 50.0, 120.0);
			Perch.Yaw = 0.f;
			Perch.Kind = EFaunaPerchKind::Rail;
		}
		return Perches;
	}

	/** A crow flock on a rail, begun as play begins it, with every piece a stand-in. */
	AFaunaFlock* SpawnFlock(UWorld* World, int32 Perches, int32 Birds, int32 Seed)
	{
		AFaunaFlock* Flock = World->SpawnActor<AFaunaFlock>();
		if (!Flock)
		{
			return nullptr;
		}
		UStaticMesh* Cube = StandIn();
		Flock->Perches = Rail(Perches);
		Flock->BirdCount = Birds;
		Flock->Seed = Seed;
		Flock->PerchedMesh = Flock->HeadMesh = Flock->FlyingMesh = Cube;
		Flock->WingMeshL = Flock->WingMeshR = Flock->OuterWingMeshL = Flock->OuterWingMeshR = Cube;
		// No idle hops between perches: the tests count who sits where.
		Flock->HopSecondsMin = Flock->HopSecondsMax = 10000.f;
		Flock->DispatchBeginPlay();
		return Flock;
	}

	/** Whether every bird sits on a perch of its own. */
	bool EachOnItsOwnPerch(const AFaunaFlock& Flock)
	{
		TSet<int32> Taken;
		for (const FFaunaBird& Bird : Flock.GetBirds())
		{
			if (Bird.State != EFaunaBirdState::Perched || !Flock.Perches.IsValidIndex(Bird.Perch) || Taken.Contains(Bird.Perch)
				|| !Bird.Position.Equals(Flock.Perches[Bird.Perch].Location, 1.0))
			{
				return false;
			}
			Taken.Add(Bird.Perch);
		}
		return true;
	}

	/** A gunshot's noise at Where, at Time (world seconds). */
	FFaunaNoise Shot(const FVector& Where, double Time)
	{
		FFaunaNoise Noise;
		Noise.Kind = EFaunaNoise::Gunshot;
		Noise.Location = Where;
		Noise.Radius = UFaunaSubsystem::GunshotRadius;
		Noise.Time = Time;
		return Noise;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaFlockScatterTest, "Looter.World.Fauna.Flock.Scatter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaFlockScatterTest::RunTest(const FString& Parameters)
{
	// Crows on a fence: they sit while nobody comes near, burst off when a player walks up, wheel overhead, and come back
	// to the fence once the player has gone, each on a perch of its own.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AFaunaFlock* Flock = SpawnFlock(World, 8, 6, 31);
	if (!TestNotNull(TEXT("A flock"), Flock))
	{
		return false;
	}
	TestEqual(TEXT("Six birds"), Flock->GetBirds().Num(), 6);
	double Now = 0.0;
	Run(*Flock, 3.f, Now, Quiet);
	TestEqual(TEXT("Left alone, all six stay perched"), Flock->CountPerched(), 6);
	TestTrue(TEXT("...each on a perch of its own"), EachOnItsOwnPerch(*Flock));

	// A player walks up to 15 m: not yet. To 8 m: they go.
	const FVector Far(1500.0, 150.0, 100.0);
	Run(*Flock, 1.f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(Far)); return Context; });
	TestFalse(TEXT("A walker 15 m off doesn't startle them"), Flock->IsFlushed());
	const FVector Near(800.0, 150.0, 100.0);
	Run(*Flock, 0.1f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(Near)); return Context; });
	TestTrue(TEXT("A walker 8 m off does"), Flock->IsFlushed());
	Run(*Flock, 3.f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(Near)); return Context; });
	TestEqual(TEXT("Three seconds on, every bird is in the air"), Flock->CountPerched(), 0);
	int32 High = 0;
	for (const FFaunaBird& Bird : Flock->GetBirds())
	{
		High += Bird.Position.Z > 120.0 + 500.0 ? 1 : 0;
	}
	TestEqual(TEXT("...all over 5 m above the fence"), High, 6);

	// The player goes; the flock comes back within a calm spell and the flight in.
	Run(*Flock, 45.f, Now, Quiet);
	TestFalse(TEXT("Long after the player left, the flock has settled"), Flock->IsFlushed());
	TestEqual(TEXT("...all six perched"), Flock->CountPerched(), 6);
	TestTrue(TEXT("...each on a perch of its own"), EachOnItsOwnPerch(*Flock));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaFlockGunfireTest, "Looter.World.Fauna.Flock.Gunfire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaFlockGunfireTest::RunTest(const FString& Parameters)
{
	// A gunshot within the crows' gunfire radius (50 m) sends them up; farther off, or a noise they heard already, doesn't.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AFaunaFlock* Flock = SpawnFlock(World, 6, 5, 7);
	if (!TestNotNull(TEXT("A flock"), Flock))
	{
		return false;
	}
	double Now = 0.0;
	Run(*Flock, 1.f, Now, Quiet);
	// Fired 60 m off, at the next frame.
	const double FarTime = Now + 0.5 * Frame;
	Run(*Flock, 0.5f, Now, [&](double) { FFaunaContext Context; Context.Noises.Add(Shot(FVector(6000.0, 0.0, 0.0), FarTime)); return Context; });
	TestFalse(TEXT("A shot 60 m off doesn't startle them"), Flock->IsFlushed());
	// One fired long ago (an old noise still in the list) means nothing now.
	Run(*Flock, 0.5f, Now, [&](double) { FFaunaContext Context; Context.Noises.Add(Shot(FVector(1000.0, 0.0, 0.0), 0.1)); return Context; });
	TestFalse(TEXT("...nor a near one heard before"), Flock->IsFlushed());
	// Fired 40 m off, now.
	const double NearTime = Now + 0.5 * Frame;
	Run(*Flock, 0.2f, Now, [&](double) { FFaunaContext Context; Context.Noises.Add(Shot(FVector(4000.0, 0.0, 0.0), NearTime)); return Context; });
	TestTrue(TEXT("A shot 40 m off does"), Flock->IsFlushed());
	Run(*Flock, 2.f, Now, Quiet);
	TestEqual(TEXT("...and every bird goes"), Flock->CountPerched(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaFlockLeaveTest, "Looter.World.Fauna.Flock.Leave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaFlockLeaveTest::RunTest(const FString& Parameters)
{
	// A player who stays among the perches: the flock wheels a while, then goes off over the hills, and is back once the
	// player has gone.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AFaunaFlock* Flock = SpawnFlock(World, 6, 4, 12);
	if (!TestNotNull(TEXT("A flock"), Flock))
	{
		return false;
	}
	double Now = 0.0;
	const FVector AmongThem(0.0, 100.0, 100.0);
	Run(*Flock, Flock->LeaveAfterSeconds + 20.f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(AmongThem)); return Context; });
	int32 Away = 0;
	for (const FFaunaBird& Bird : Flock->GetBirds())
	{
		Away += Bird.State == EFaunaBirdState::Away || Bird.State == EFaunaBirdState::Leaving ? 1 : 0;
	}
	TestEqual(TEXT("With the player staying among the perches, the flock has gone off"), Away, 4);
	TestEqual(TEXT("...and none sits on a perch"), Flock->CountPerched(), 0);
	Run(*Flock, 60.f, Now, Quiet);
	TestEqual(TEXT("Once the player has gone, they're all back"), Flock->CountPerched(), 4);
	TestTrue(TEXT("...each on a perch of its own"), EachOnItsOwnPerch(*Flock));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaFlockPassableTest, "Looter.World.Fauna.Flock.Passable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaFlockPassableTest::RunTest(const FString& Parameters)
{
	// Nothing of the fauna is in anyone's way: no collision, no navigation, never an Obstacle; pieces not in use are shrunk
	// away; and two flocks with the same seed play out the same.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AFaunaFlock* Flock = SpawnFlock(World, 6, 6, 99);
	AFaunaFlock* Twin = SpawnFlock(World, 6, 6, 99);
	if (!TestNotNull(TEXT("A flock"), Flock) || !TestNotNull(TEXT("...and its twin"), Twin))
	{
		return false;
	}
	TestFalse(TEXT("The flock isn't an obstacle"), Flock->Tags.Contains(MinimapTags::Obstacle));
	TInlineComponentArray<UInstancedStaticMeshComponent*> Pieces(Flock);
	TestEqual(TEXT("Seven pieces for crows (perched, head, flying, two wings and two outer wings)"), Pieces.Num(), 7);
	for (const UInstancedStaticMeshComponent* Piece : Pieces)
	{
		TestTrue(FString::Printf(TEXT("%s has no collision"), *Piece->GetName()), Piece->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestFalse(FString::Printf(TEXT("%s doesn't affect navigation"), *Piece->GetName()), Piece->CanEverAffectNavigation());
		TestFalse(FString::Printf(TEXT("%s casts no shadow"), *Piece->GetName()), Piece->CastShadow != 0);
		TestEqual(FString::Printf(TEXT("%s has one instance per bird"), *Piece->GetName()), Piece->GetInstanceCount(), 6);
	}

	// Perched: the flying pieces are shrunk away.
	double Now = 0.0;
	Run(*Flock, 1.f, Now, Quiet);
	for (UInstancedStaticMeshComponent* Piece : Pieces)
	{
		if (Piece->GetName().Contains(TEXT("FlyingBirds")))
		{
			FTransform Instance;
			Piece->GetInstanceTransform(0, Instance, true);
			TestTrue(TEXT("A perched bird's flying body is shrunk away"), Instance.GetScale3D().GetMax() < 0.01);
		}
	}

	// The same seed, the same updates: the same flock.
	double TwinNow = 1.0;
	Run(*Twin, 1.f, TwinNow, Quiet);
	const FVector Startle(700.0, 100.0, 100.0);
	Run(*Flock, 6.f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(Startle)); return Context; });
	Run(*Twin, 6.f, TwinNow, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(Startle)); return Context; });
	bool bSame = Flock->GetBirds().Num() == Twin->GetBirds().Num();
	for (int32 Index = 0; bSame && Index < Flock->GetBirds().Num(); ++Index)
	{
		bSame &= Flock->GetBirds()[Index].Position.Equals(Twin->GetBirds()[Index].Position, 0.01)
			&& FMath::IsNearlyEqual(Flock->GetBirds()[Index].Yaw, Twin->GetBirds()[Index].Yaw, 0.01f);
	}
	TestTrue(TEXT("Two flocks with one seed fly the same"), bSame);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaAerialTest, "Looter.World.Fauna.Flock.Aerial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaAerialTest::RunTest(const FString& Parameters)
{
	// Hawks over the valley: they keep to their sky (round its middle, between its heights) and never land or flee.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AFaunaFlock* Hawks = World->SpawnActor<AFaunaFlock>();
	if (!TestNotNull(TEXT("Hawks"), Hawks))
	{
		return false;
	}
	UStaticMesh* Cube = StandIn();
	Hawks->Mode = EFaunaFlockMode::Aerial;
	Hawks->BirdCount = 3;
	Hawks->AerialArea.Center = FVector(1000.0, -2000.0, 0.0);
	Hawks->AerialArea.Radius = 3500.f;
	Hawks->AerialArea.MinHeight = 4500.f;
	Hawks->AerialArea.MaxHeight = 7500.f;
	Hawks->FlightSpeed = 1000.f;
	Hawks->FlyingMesh = Hawks->WingMeshL = Hawks->WingMeshR = Cube;
	Hawks->DispatchBeginPlay();
	double Now = 0.0;
	bool bInSky = true;
	for (int32 Second = 0; Second < 120; ++Second)
	{
		Run(*Hawks, 1.f, Now, [&](double) { FFaunaContext Context; Context.Threats.Add(Walker(Hawks->AerialArea.Center)); return Context; });
		for (const FFaunaBird& Bird : Hawks->GetBirds())
		{
			const double Height = Bird.Position.Z - Hawks->AerialArea.Center.Z;
			// Each circles at up to 1.2 times the sky's radius, its thermal drifting up to half that again.
			bInSky &= Height >= 4500.0 - 1.0 && Height <= 7500.0 + 1.0
				&& FVector::Dist2D(Bird.Position, Hawks->AerialArea.Center) < 3500.0 * 1.2 * 1.5 + 1.0;
			bInSky &= Bird.State == EFaunaBirdState::Soaring;
		}
	}
	TestTrue(TEXT("For two minutes every hawk soars within its sky"), bInSky);
	TestTrue(TEXT("An aerial flock is always busy (it updates while seen)"), Hawks->IsBusy());
	return true;
}

#endif
