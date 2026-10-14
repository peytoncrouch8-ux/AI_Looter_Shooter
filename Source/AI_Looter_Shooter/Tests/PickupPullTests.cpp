#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/SilverPickup.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/AmmoPickup.h"
#include "Loot/PickupPull.h"
#include "Loot/PickupPullSubsystem.h"
#include "Tests/LocomotionTestWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"

// Loot that comes to the player (concept F, "pickups that come to you"; Looter.Loot.Pull.*): the flight's path (an arc
// back onto its line, gaining speed, arriving), the reach and the clear line (never through a wall), silver's pull
// waiting behind a wall and dropping when one comes between, and the ammo's fly-in after it has counted: only in reach
// and in line, pooled and capped, gone once it lands, the ammo counted all the same.

namespace
{
	constexpr float PullTestFrame = 1.f / 60.f;

	/** A wall across the X axis halfway to x = 300, tall enough to stand between a pickup on the ground and the player. */
	AStaticMeshActor* PullSpawnWall(UWorld* World, double AtX)
	{
		return LocomotionTestWorld::SpawnBlock(World, FVector(AtX, 0.0, 100.0), FVector(20.0, 300.0, 300.0));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPickupPullFlightTest, "Looter.Loot.Pull.Flight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupPullFlightTest::RunTest(const FString& Parameters)
{
	// A flight from 3 m out, the silver's speeds: it rises over its line, comes back down onto it, and arrives in time.
	FPickupFlight Flight;
	TestFalse(TEXT("Not under way before it starts"), Flight.IsStarted());
	const FVector Target(300.0, 0.0, 0.0);
	Flight.Start(FVector::ZeroVector, Target, ASilverPickup::PullStartSpeed);
	TestTrue(TEXT("Under way"), Flight.IsStarted());
	float Time = 0.f;
	float LastProgress = 0.f;
	float HighestLift = 0.f;
	bool bArrived = false;
	bool bAlwaysOn = true;
	float LastSpeed = Flight.GetSpeed();
	bool bGaining = true;
	while (!bArrived && Time < 2.f)
	{
		bArrived = Flight.Step(PullTestFrame, Target, ASilverPickup::PullAcceleration, ASilverPickup::PullMaxSpeed);
		Time += PullTestFrame;
		bAlwaysOn &= Flight.GetProgress() >= LastProgress;
		LastProgress = Flight.GetProgress();
		bGaining &= Flight.GetSpeed() >= LastSpeed;
		LastSpeed = Flight.GetSpeed();
		HighestLift = FMath::Max(HighestLift, static_cast<float>(Flight.GetLocation().Z - Flight.GetBase().Z));
	}
	TestTrue(FString::Printf(TEXT("It arrives in under a second (%.2f s)"), Time), bArrived && Time < 1.f);
	TestTrue(TEXT("...at the target"), Flight.GetBase().Equals(Target, 0.01));
	TestTrue(TEXT("Its progress only goes forward"), bAlwaysOn);
	TestTrue(TEXT("It gains speed"), bGaining);
	TestTrue(FString::Printf(TEXT("It's tossed in an arc (%.1f cm over its line at most)"), HighestLift),
		HighestLift >= FPickupFlight::MinLift * 0.9f && HighestLift <= FPickupFlight::MaxLift + 0.01f);
	TestTrue(TEXT("...and comes down onto its line at the end"), FMath::IsNearlyEqual(static_cast<float>(Flight.GetLocation().Z), 0.f, 0.01f));

	// A target that moves on (the player running): it still gets there.
	FPickupFlight Chase;
	Chase.Start(FVector::ZeroVector, FVector(200.0, 0.0, 0.0), ASilverPickup::PullStartSpeed);
	FVector Runner(200.0, 0.0, 0.0);
	bool bCaught = false;
	for (int32 FrameIndex = 0; FrameIndex < 120 && !bCaught; ++FrameIndex)
	{
		Runner.Y += 400.0 * PullTestFrame;
		bCaught = Chase.Step(PullTestFrame, Runner, ASilverPickup::PullAcceleration, ASilverPickup::PullMaxSpeed);
	}
	TestTrue(TEXT("It catches a player running across"), bCaught);

	// The glint: nothing at the start, a quick flare, gone by its time.
	TestEqual(TEXT("No glint before it starts"), PickupPull::GlintAt(-0.1f), 0.f);
	TestTrue(TEXT("A flare a moment in"), PickupPull::GlintAt(0.05f) > 0.9f);
	TestTrue(TEXT("...fading"), PickupPull::GlintAt(0.2f) > 0.f && PickupPull::GlintAt(0.2f) < PickupPull::GlintAt(0.05f));
	TestEqual(TEXT("...and gone"), PickupPull::GlintAt(PickupPull::GlintSeconds), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPickupPullLineTest, "Looter.Loot.Pull.Line",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupPullLineTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	APawn* Player = World->SpawnActor<APawn>();
	AStaticMeshActor* Wall = PullSpawnWall(World, 150.0);
	if (!TestTrue(TEXT("A player and a wall"), Player && Wall))
	{
		return false;
	}
	const FVector Behind(300.0, 0.0, 0.0);
	const FVector Open(0.0, 300.0, 0.0);
	TestFalse(TEXT("No line through a wall"), PickupPull::HasClearLine(*World, Behind, *Player, nullptr));
	TestTrue(TEXT("A clear line in the open"), PickupPull::HasClearLine(*World, Open, *Player, nullptr));
	TestFalse(TEXT("In line but out of reach: no pull"), PickupPull::CanPull(*World, Open, *Player, 260.f, nullptr));
	TestTrue(TEXT("In line and in reach: a pull"), PickupPull::CanPull(*World, Open, *Player, 400.f, nullptr));
	TestFalse(TEXT("In reach behind a wall: no pull"), PickupPull::CanPull(*World, Behind, *Player, 400.f, nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPickupPullSilverTest, "Looter.Loot.Pull.Silver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupPullSilverTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UWalletSubsystem* Wallet = UWalletSubsystem::Get(World);
	APawn* Player = World->SpawnActor<APawn>();
	ASilverPickup* Hidden = ASilverPickup::SpawnSilver(World, 20, FVector(300.0, 0.0, 0.0));
	ASilverPickup* Seen = ASilverPickup::SpawnSilver(World, 7, FVector(0.0, 300.0, 0.0));
	if (!TestTrue(TEXT("A wallet, a player, two piles and a wall"), Wallet && Player && Hidden && Seen && PullSpawnWall(World, 150.0)))
	{
		return false;
	}
	// Both in reach (MagnetRadius); one behind the wall.
	for (int32 FrameIndex = 0; FrameIndex < 60; ++FrameIndex)
	{
		Hidden->Advance(PullTestFrame, Player);
	}
	TestFalse(TEXT("A pile behind a wall waits"), Hidden->IsPulled());
	bool bCollected = false;
	for (int32 FrameIndex = 0; FrameIndex < 120 && !bCollected; ++FrameIndex)
	{
		bCollected = Seen->Advance(PullTestFrame, Player);
	}
	TestTrue(TEXT("One in the open comes and is collected"), bCollected);
	TestEqual(TEXT("...its silver alone in the wallet"), Wallet->GetSilver(), 7);

	// A wall coming between them on the way (the player ducking round a corner): it drops there, never through it.
	ASilverPickup* Cut = ASilverPickup::SpawnSilver(World, 5, FVector(0.0, -350.0, 0.0));
	if (!TestNotNull(TEXT("A third pile"), Cut))
	{
		return false;
	}
	Cut->Advance(ASilverPickup::CollectDelay, nullptr);
	Cut->Advance(PullTestFrame, Player);
	TestTrue(TEXT("It sets off"), Cut->IsPulled());
	LocomotionTestWorld::SpawnBlock(World, FVector(0.0, -120.0, 100.0), FVector(300.0, 20.0, 300.0));
	for (int32 FrameIndex = 0; FrameIndex < 12 && Cut->IsPulled(); ++FrameIndex)
	{
		Cut->Advance(PullTestFrame, Player);
	}
	TestFalse(TEXT("...and drops when a wall comes between"), Cut->IsPulled());
	TestEqual(TEXT("...not collected through it"), Wallet->GetSilver(), 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPickupPullAmmoTest, "Looter.Loot.Pull.Ammo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPickupPullAmmoTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UPickupPullSubsystem* Pull = UPickupPullSubsystem::Get(World);
	APawn* Player = World->SpawnActor<APawn>();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestTrue(TEXT("The pull, a player, a mesh and a wall"), Pull && Player && Cube && PullSpawnWall(World, 150.0)))
	{
		return false;
	}
	const FLinearColor Glint = FLinearColor::White;
	const FTransform Near(FVector(0.0, 200.0, 0.0));
	TestFalse(TEXT("No player: no flight"), Pull->FlyIn(Cube, Near, nullptr, nullptr, Glint));
	TestFalse(TEXT("Out of reach: no flight"), Pull->FlyIn(Cube, FTransform(FVector(0.0, 400.0, 0.0)), Player, nullptr, Glint));
	TestFalse(TEXT("Behind a wall: no flight"), Pull->FlyIn(Cube, FTransform(FVector(240.0, 0.0, 0.0)), Player, nullptr, Glint));
	TestTrue(TEXT("In reach and in line: it flies"), Pull->FlyIn(Cube, Near, Player, nullptr, Glint));
	TestEqual(TEXT("...one under way"), Pull->NumFlying(), 1);
	TestTrue(TEXT("...and the pull ticks while it does"), Pull->IsTickable());
	for (int32 FrameIndex = 0; FrameIndex < 60 && Pull->NumFlying() > 0; ++FrameIndex)
	{
		Pull->Advance(PullTestFrame);
	}
	TestEqual(TEXT("It lands in the player and is done"), Pull->NumFlying(), 0);
	TestFalse(TEXT("...and nothing ticks"), Pull->IsTickable());
	TestEqual(TEXT("...its mesh kept for the next"), Pull->NumPooled(), 1);

	// Capped: a field of drops run through at once flies only so many; the rest just go.
	int32 Flown = 0;
	for (int32 Index = 0; Index < UPickupPullSubsystem::MaxFlights + 4; ++Index)
	{
		Flown += Pull->FlyIn(Cube, Near, Player, nullptr, Glint) ? 1 : 0;
	}
	TestEqual(TEXT("At most MaxFlights at once"), Flown, UPickupPullSubsystem::MaxFlights);
	TestEqual(TEXT("...on that many meshes"), Pull->NumPooled(), UPickupPullSubsystem::MaxFlights);
	for (int32 FrameIndex = 0; FrameIndex < 60 && Pull->NumFlying() > 0; ++FrameIndex)
	{
		Pull->Advance(PullTestFrame);
	}

	// An ammo pickup run past: it counts at once as before (and is gone), and its bundle flies in after.
	UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>(Player, TEXT("Weapons"));
	AAmmoPickup* Ammo = AAmmoPickup::SpawnAmmo(World, EAmmoType::Pistol, 12, FVector(0.0, 150.0, 0.0));
	if (!TestNotNull(TEXT("A pickup of 12"), Ammo))
	{
		return false;
	}
	const bool bHasModel = Ammo->GetModel() && Ammo->GetModel()->GetStaticMesh();
	TWeakObjectPtr<AAmmoPickup> AmmoGone(Ammo);
	TestEqual(TEXT("All twelve counted at once"), Ammo->CollectInto(*Inventory), 12);
	TestFalse(TEXT("...the pickup gone at once, as before"), AmmoGone.IsValid());
	if (bHasModel)
	{
		TestEqual(TEXT("...and its bundle flies in"), Pull->NumFlying(), 1);
	}
	else
	{
		AddWarning(TEXT("SM_AmmoPistol isn't imported: the fly-in is left unchecked"));
	}
	return true;
}

#endif
