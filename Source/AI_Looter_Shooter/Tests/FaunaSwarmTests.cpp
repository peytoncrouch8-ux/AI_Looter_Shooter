#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FaunaTestWorld.h"
#include "World/FaunaCloth.h"
#include "World/FaunaSubsystem.h"
#include "World/FaunaSwarm.h"
#include "World/FaunaTumbleweeds.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

using namespace FaunaTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaSwarmCapsTest, "Looter.World.Fauna.Swarm.Caps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaSwarmCapsTest::RunTest(const FString& Parameters)
{
	// Butterflies come out only in the zones near the view, never more than the cap, and stay in their zones; fireflies
	// are out at dusk only.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AFaunaSwarm* Swarm = World->SpawnActor<AFaunaSwarm>();
	if (!TestNotNull(TEXT("A swarm"), Swarm))
	{
		return false;
	}
	UStaticMesh* Cube = StandIn();
	Swarm->Kind = EFaunaInsect::Butterfly;
	Swarm->BodyMesh = Swarm->WingMeshL = Swarm->WingMeshR = Cube;
	Swarm->MaxActive = 12;
	Swarm->ActiveRadius = 3000.f;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FFaunaZone& Zone = Swarm->Zones.AddDefaulted_GetRef();
		Zone.Center = FVector(Index * 1200.0, 0.0, 0.0);
		Zone.Radius = 400.f;
		Zone.MinHeight = 30.f;
		Zone.MaxHeight = 180.f;
		Zone.Count = 10;
	}
	FFaunaZone& Distant = Swarm->Zones.AddDefaulted_GetRef();
	Distant.Center = FVector(0.0, 50000.0, 0.0);
	Distant.Radius = 400.f;
	Distant.Count = 10;
	Swarm->DispatchBeginPlay();

	auto Seen = [](const FVector& From)
	{
		return [From](double)
		{
			FFaunaContext Context;
			Context.View.bValid = true;
			Context.View.Location = From;
			return Context;
		};
	};
	double Now = 0.0;
	Run(*Swarm, 5.f, Now, Seen(FVector(1200.0, -500.0, 170.0)));
	TestEqual(TEXT("Three near zones wanting 30: the cap's 12 out"), Swarm->GetActiveCount(), 12);
	bool bInZones = true;
	int32 InDistant = 0;
	for (const FFaunaInsect& Insect : Swarm->GetInsects())
	{
		if (Insect.Zone == INDEX_NONE)
		{
			continue;
		}
		InDistant += Insect.Zone == 3 ? 1 : 0;
		const FFaunaZone& Zone = Swarm->Zones[Insect.Zone];
		bInZones &= FVector::Dist2D(Insect.Position, Zone.Center) < Zone.Radius + 600.0;
		bInZones &= Insect.Position.Z >= Zone.Center.Z + Zone.MinHeight * 0.5 - 1.0 && Insect.Position.Z <= Zone.Center.Z + Zone.MaxHeight + 151.0;
	}
	TestEqual(TEXT("...none in the zone 500 m away"), InDistant, 0);
	TestTrue(TEXT("...and five seconds on, every one is still about its zone"), bInZones);

	Run(*Swarm, 1.f, Now, Seen(FVector(0.0, -40000.0, 170.0)));
	TestEqual(TEXT("The view 400 m away: none out"), Swarm->GetActiveCount(), 0);

	// Out only in their light.
	Swarm->ShownIn = FName(TEXT("Dusk"));
	TestFalse(TEXT("A dusk swarm isn't out by day"), Swarm->IsInItsLight(FName(TEXT("Day"))));
	TestTrue(TEXT("...is at dusk"), Swarm->IsInItsLight(FName(TEXT("Dusk"))));
	TestFalse(TEXT("...and never in a level without lighting states (always day)"), Swarm->IsInItsLight(NAME_None));
	Swarm->ShownIn = NAME_None;
	TestTrue(TEXT("A swarm for every light is out in all"), Swarm->IsInItsLight(FName(TEXT("Dusk"))) && Swarm->IsInItsLight(NAME_None));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaTumbleweedTest, "Looter.World.Fauna.Tumbleweed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaTumbleweedTest::RunTest(const FString& Parameters)
{
	// A tumbleweed rolls downwind along its lane, turning as it goes, touches nothing, and fades past the lane's end. (A
	// test level's traces find no ground, so it rolls level; the bounce is checked in Looter.World.Fauna.Rules.Bounce.)
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AFaunaTumbleweeds* Tumbleweeds = World->SpawnActor<AFaunaTumbleweeds>();
	if (!TestNotNull(TEXT("Tumbleweeds"), Tumbleweeds))
	{
		return false;
	}
	FFaunaLane& Lane = Tumbleweeds->Lanes.AddDefaulted_GetRef();
	Lane.Start = FVector(0.0, 0.0, 0.0);
	Lane.End = FVector(3000.0, 0.0, 0.0);
	Tumbleweeds->Mesh = StandIn();
	Tumbleweeds->SpawnSecondsMin = Tumbleweeds->SpawnSecondsMax = 1000.f;
	Tumbleweeds->DispatchBeginPlay();
	TInlineComponentArray<UStaticMeshComponent*> Bodies(Tumbleweeds);
	for (const UStaticMeshComponent* Body : Bodies)
	{
		TestTrue(TEXT("A tumbleweed has no collision"), Body->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	}
	TestTrue(TEXT("One sets off down its lane"), Tumbleweeds->Launch(0));
	const FVector From = Tumbleweeds->GetTumbles()[0].Position;
	double Now = 0.0;
	auto Watched = [](double) { FFaunaContext Context; Context.Threats.Add(Walker(FVector(1500.0, 800.0, 0.0))); return Context; };
	Run(*Tumbleweeds, 3.f, Now, Watched);
	const FFaunaTumble& Rolling = Tumbleweeds->GetTumbles()[0];
	TestTrue(TEXT("Three seconds on it's still rolling"), Rolling.bActive);
	TestTrue(TEXT("...downwind, several metres along"), Rolling.Position.X > 500.0);
	// Its total turn, not its orientation's change: a whole number of turns brings the orientation back where it began.
	const double Expected = FVector::Dist2D(Rolling.Position, From) / (Tumbleweeds->Radius * Rolling.Scale);
	TestTrue(TEXT("...and turning as it goes"), Rolling.Rolled > 1.f);
	TestEqual(TEXT("...as far as it has rolled (its distance over its radius)"), static_cast<double>(Rolling.Rolled), Expected,
		Expected * 0.05 + 0.05);
	Run(*Tumbleweeds, 40.f, Now, Watched);
	TestFalse(TEXT("Long past its lane's end it has gone"), Tumbleweeds->GetTumbles()[0].bActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFaunaToggleTest, "Looter.World.Fauna.Toggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFaunaToggleTest::RunTest(const FString& Parameters)
{
	// Looter.Fauna 0 hides everything (to measure by difference); 1 brings it back as the view finds it. The washing swings.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UFaunaSubsystem* Fauna = UFaunaSubsystem::Get(World);
	AFaunaCloth* Cloth = World->SpawnActor<AFaunaCloth>();
	if (!TestNotNull(TEXT("The fauna's subsystem runs in test levels"), Fauna) || !TestNotNull(TEXT("Washing"), Cloth))
	{
		return false;
	}
	Cloth->Meshes.Add(StandIn());
	FFaunaClothPiece& Sheet = Cloth->Pieces.AddDefaulted_GetRef();
	Sheet.Location = FVector(0.0, 0.0, 200.0);
	Sheet.Yaw = Cloth->WindYaw;
	Cloth->DispatchBeginPlay();
	TestTrue(TEXT("The washing registered"), Fauna->GetActors().Contains(Cloth));

	double Now = 0.0;
	Run(*Cloth, 6.f, Now, Quiet);
	float Widest = 0.f;
	for (int32 Step = 0; Step < 300; ++Step)
	{
		Run(*Cloth, Frame, Now, Quiet);
		Widest = FMath::Max(Widest, FMath::Abs(Cloth->GetSwings()[0].Angle));
	}
	TestTrue(TEXT("A sheet facing the wind swings out in its gusts"), Widest > 3.f);
	TestTrue(TEXT("...never far past its widest swing (a gust's overshoot)"), Widest < Cloth->SwingDegrees * 2.f);
	{
		FFaunaSetting Off(0);
		Fauna->Tick(Frame);
		TestFalse(TEXT("Looter.Fauna 0 hides it"), Cloth->IsFaunaShown());
		TestTrue(TEXT("...and it's hidden in game"), Cloth->IsHidden());
	}
	Fauna->Tick(Frame);
	TestTrue(TEXT("Looter.Fauna 1 shows it again (no view: everything near)"), Cloth->IsFaunaShown());
	return true;
}

#endif
