#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/EggSac.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "PhysicsEngine/AggregateGeom.h"
#include "PhysicsEngine/BodySetup.h"
#include "Tests/AutomationCommon.h"

// An egg sac's hit volume (Main 5, "The Keeper's Lantern"): shots that read as hits count. The 2026-10-23 play-check: a
// three-round burst at sac A's narrow foot, inside its fuzz of loose silk, counted none; a second, centred one brought it
// down. The volume is a capsule round the sac's body with a hand's room over it, seen by bullets only, gone as it falls.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEggSacHitVolumeTest, "Looter.Story.KeepersLantern.SacHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEggSacHitVolumeTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	AEggSac* Sac = World->SpawnActor<AEggSac>(FVector(0.0, 0.0, 600.0), FRotator::ZeroRotator);
	if (!TestTrue(TEXT("A sac and its hit volume"), Sac && Sac->HitVolume))
	{
		return false;
	}
	Sac->DropHeight = 600.f;
	Sac->DispatchBeginPlay();
	const UCapsuleComponent* Volume = Sac->HitVolume;

	// Bullets only (the Weapon channel, DefaultEngine.ini's ECC_GameTraceChannel2): not the Interact key, the camera or pawns.
	TestTrue(TEXT("Seen by bullets"), Volume->GetCollisionEnabled() == ECollisionEnabled::QueryOnly
		&& Volume->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block);
	TestTrue(TEXT("...by nothing else"), Volume->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore
		&& Volume->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore && Volume->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore
		&& Volume->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Ignore);

	// Round the body (its hulls, else its model; a 2 m drop without one), with room over it all round.
	const UStaticMesh* Model = Sac->Sac->GetStaticMesh();
	FBox Body(ForceInit);
	if (Model && Model->GetBodySetup())
	{
		Body = Model->GetBodySetup()->AggGeom.CalcAABB(FTransform::Identity);
	}
	if ((!Body.IsValid || Body.GetVolume() <= 0.0) && Model)
	{
		Body = Model->GetBoundingBox();
	}
	if (!Model)
	{
		AddInfo(TEXT("SM_EggSac_A isn't in this checkout: the volume is the 2 m stand-in drop."));
		Body = FBox(FVector(-60.0, -60.0, 0.0), FVector(60.0, 60.0, 200.0));
	}
	const FVector Half = Body.GetExtent();
	const double Radius = Volume->GetUnscaledCapsuleRadius();
	const double HalfHeight = Volume->GetUnscaledCapsuleHalfHeight();
	TestTrue(*FString::Printf(TEXT("Wider than the body by a hand (radius %.0f over %.0f)"), Radius, FMath::Max(Half.X, Half.Y)),
		Radius >= FMath::Max(Half.X, Half.Y) + 10.0);
	TestTrue(*FString::Printf(TEXT("...taller too, foot to top (half height %.0f over %.0f)"), HalfHeight, Half.Z), HalfHeight >= Half.Z + 10.0);
	TestTrue(TEXT("...round the body's middle"), Volume->GetRelativeLocation().Equals(Body.GetCenter(), 0.5));
	// The foot of the drop, where the burst went: within the volume (its bottom 2 cm over the body's lowest point).
	const FVector Foot = Sac->Sac->GetComponentTransform().TransformPosition(FVector(Body.GetCenter().X, Body.GetCenter().Y, Body.Min.Z + 2.0));
	const FVector FromMiddle = Volume->GetComponentTransform().InverseTransformPosition(Foot);
	const double Spine = FMath::Max(FMath::Abs(FromMiddle.Z) - (HalfHeight - Radius), 0.0);
	TestTrue(TEXT("...its foot inside it"), FMath::Sqrt(FMath::Square(FVector2D(FromMiddle.X, FromMiddle.Y).Size()) + FMath::Square(Spine)) <= Radius + 0.5);

	// Shot down: shots pass where it hung as it falls; hung again, it's a target again.
	TestTrue(TEXT("Shot down"), Sac->ShootDown(nullptr, /*bForce*/ true));
	TestTrue(TEXT("...falling, its volume is gone"), Sac->IsDown() && Volume->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	Sac->Hang();
	TestTrue(TEXT("Hung again: its volume is back"), Sac->GetState() == EEggSacState::Hanging && Volume->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	return true;
}

#endif
