#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/SlimeCreature.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "PhysicsEngine/BodySetup.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlimeHopTest, "Looter.Creatures.Slime.Hop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlimeHopTest::RunTest(const FString& Parameters)
{
	// A hop launched with HopVelocity lands as far away as asked (on flat ground) and rises as high as asked; and the
	// squash keeps the slime's volume, wide when flat and thin when stretched.
	const float Gravity = 980.f;
	for (const FVector2D Hop : { FVector2D(150.f, 60.f), FVector2D(260.f, 90.f), FVector2D(500.f, 120.f) })
	{
		const FVector Velocity = ASlimeCreature::HopVelocity(FVector(1.f, 1.f, 0.f), Hop.X, Hop.Y, Gravity);
		const float AirTime = 2.f * static_cast<float>(Velocity.Z) / Gravity;
		TestEqual(FString::Printf(TEXT("%.0f cm hop lands there"), Hop.X), static_cast<float>(Velocity.Size2D()) * AirTime, static_cast<float>(Hop.X), 0.5f);
		TestEqual(FString::Printf(TEXT("%.0f cm hop's height"), Hop.X), static_cast<float>(Velocity.Z * Velocity.Z) / (2.f * Gravity), static_cast<float>(Hop.Y), 0.5f);
		TestEqual(TEXT("Hops go where they're aimed"), static_cast<float>(Velocity.X), static_cast<float>(Velocity.Y), 0.01f);
	}
	for (const float Z : { 0.45f, 0.6f, 1.f, 1.3f })
	{
		const FVector Scale = ASlimeCreature::SquashScale(Z);
		TestEqual(FString::Printf(TEXT("Squash %.2f keeps the volume"), Z), static_cast<float>(Scale.X * Scale.Y * Scale.Z), 1.f, 0.001f);
		TestEqual(TEXT("Squash height"), static_cast<float>(Scale.Z), Z);
	}
	TestTrue(TEXT("Squashed is wider"), ASlimeCreature::SquashScale(0.6f).X > 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlimeShotsTest, "Looter.Creatures.Slime.Shots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlimeShotsTest::RunTest(const FString& Parameters)
{
	// The gel's hull takes every shot (the core sits inside it), and a shot is a crit when its line runs on through the
	// core: straight at it from any side, but not past it.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ASlimeCreature* Slime = WorldWrapper.GetTestWorld()->SpawnActor<ASlimeCreature>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Slime spawned"), Slime))
	{
		return false;
	}
	USkeletalMeshComponent* Mesh = Slime->GetMesh();
	TestTrue(TEXT("Hit zones are only for queries"), Mesh->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	TestTrue(TEXT("Hit zones block shots"), Mesh->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block);
	TestTrue(TEXT("Hit zones let pawns through"), Mesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
	TestEqual(TEXT("Hit zones: gel and core"), Mesh->Bodies.Num(), 2);
	for (const FBodyInstance* Zone : Mesh->Bodies)
	{
		const UBodySetup* Setup = Zone ? Zone->GetBodySetup() : nullptr;
		if (TestNotNull(TEXT("Hit zone setup"), Setup))
		{
			TestTrue(FString::Printf(TEXT("%s answers complex traces"), *Setup->BoneName.ToString()), Setup->CollisionTraceFlag == CTF_UseSimpleAsComplex);
		}
	}

	const FVector Core = Mesh->GetBoneLocation(TEXT("core"));
	const FCollisionQueryParams BulletQuery(SCENE_QUERY_STAT(SlimeShotsTest), /*bTraceComplex*/ true);
	struct FShot
	{
		const TCHAR* What;
		FVector From;
		FVector To;
		bool bHits;
		bool bCritical;
	};
	const FShot Shots[] = {
		{ TEXT("at the core from the front"), Core + FVector(400.0, 0.0, 0.0), Core, true, true },
		{ TEXT("at the core from above"), Core + FVector(0.0, 0.0, 300.0), Core, true, true },
		{ TEXT("at the core from a corner"), Core + FVector(-250.0, 250.0, 120.0), Core, true, true },
		{ TEXT("through the gel beside the core"), Core + FVector(400.0, 32.0, 0.0), Core + FVector(-400.0, 32.0, 0.0), true, false },
		{ TEXT("through the gel under the core"), Core + FVector(400.0, 0.0, -22.0), Core + FVector(-400.0, 0.0, -22.0), true, false },
		{ TEXT("over it"), Core + FVector(400.0, 0.0, 120.0), Core + FVector(-400.0, 0.0, 120.0), false, false } };
	for (const FShot& Shot : Shots)
	{
		FHitResult Hit;
		const bool bHit = Mesh->LineTraceComponent(Hit, Shot.From, Shot.To, BulletQuery);
		TestEqual(FString::Printf(TEXT("%s: hits"), Shot.What), bHit, Shot.bHits);
		if (bHit)
		{
			// Bullets keep the segment they traced on the hit; the test's segment stands in for it.
			Hit.TraceStart = Shot.From;
			Hit.TraceEnd = Shot.To;
			TestEqual(FString::Printf(TEXT("%s: critical"), Shot.What), Slime->IsCriticalSpot(Hit), Shot.bCritical);
		}
	}

	const UHealthComponent* Health = Slime->FindComponentByClass<UHealthComponent>();
	if (TestNotNull(TEXT("Slime has health"), Health))
	{
		TestEqual(TEXT("Slime max health"), Health->MaxHealth, 120.f);
	}
	TestTrue(TEXT("Slimes alert their group"), Slime->PackAlertRadius > 0.f);
	return true;
}

#endif
