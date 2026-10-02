#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Creatures/CreatureUpdateRate.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlimeSlowUpdateTest, "Looter.Creatures.Slime.SlowUpdates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSlimeSlowUpdateTest::RunTest(const FString& Parameters)
{
	// A distant slime updates only five times a second. Its springs sub-step, so a landing's splat and wobble play out
	// the same in a few long updates as in many short ones (60 fps here), and never blow up or jitter.
	using FSpring = ASlimeCreature::FSquashSpring;
	struct FSlimeSprings
	{
		FSpring Squash;
		FVector CoreOffset = FVector::ZeroVector;
		FVector CoreVelocity = FVector::ZeroVector;

		void Land()
		{
			// A landing: a quick squash down, then the spring back up; the core kicked up and back.
			Squash.Ramp(0.6f, 0.06f);
			CoreVelocity = FVector(-60.0, 0.0, 110.0);
		}
		void Step(float DeltaSeconds) { ASlimeCreature::StepSprings(Squash, CoreOffset, CoreVelocity, DeltaSeconds); }
	};

	const float SlowDelta = FCreatureUpdateRate().VeryFarInterval;
	constexpr int32 FramesPerUpdate = 12;
	FSlimeSprings Slow;
	FSlimeSprings Fast;
	Slow.Land();
	Fast.Land();
	float Lowest = 1.f;
	float Highest = 1.f;
	float FarthestCore = 0.f;
	for (int32 Update = 0; Update < 10; ++Update)
	{
		Slow.Step(SlowDelta);
		for (int32 Frame = 0; Frame < FramesPerUpdate; ++Frame)
		{
			Fast.Step(SlowDelta / FramesPerUpdate);
		}
		if (!TestTrue(TEXT("The springs stay finite"), FMath::IsFinite(Slow.Squash.Value) && !Slow.CoreOffset.ContainsNaN()))
		{
			return false;
		}
		TestNearlyEqual(FString::Printf(TEXT("Update %d squashes like twelve frames"), Update), Slow.Squash.Value, Fast.Squash.Value, 0.005f);
		TestTrue(FString::Printf(TEXT("Update %d: the core lags like twelve frames"), Update), FVector::Dist(Slow.CoreOffset, Fast.CoreOffset) < 0.5);
		Lowest = FMath::Min(Lowest, Slow.Squash.Value);
		Highest = FMath::Max(Highest, Slow.Squash.Value);
		FarthestCore = FMath::Max(FarthestCore, static_cast<float>(Slow.CoreOffset.Size()));
	}
	TestTrue(TEXT("Never squashed past the splat"), Lowest >= 0.59f);
	TestTrue(TEXT("Overshoots no more than a landing should (about 1.1)"), Highest <= 1.2f);
	TestTrue(TEXT("The core stays inside the gel"), FarthestCore < 15.f);
	TestNearlyEqual(TEXT("Settled at rest after two seconds"), Slow.Squash.Value, 1.f, 0.01f);
	TestTrue(TEXT("The core settled too"), Slow.CoreOffset.Size() < 0.5);

	// A long hitch moves the springs on at most FCreatureUpdateRate::MaxInterval, and leaves them sane.
	FSlimeSprings Hitch;
	Hitch.Land();
	Hitch.Step(5.f);
	TestTrue(TEXT("A hitch leaves the squash sane"), FMath::IsFinite(Hitch.Squash.Value) && Hitch.Squash.Value > 0.5f && Hitch.Squash.Value < 1.3f);
	TestTrue(TEXT("A hitch leaves the core sane"), !Hitch.CoreOffset.ContainsNaN() && Hitch.CoreOffset.Size() < 15.0);
	return true;
}

#endif
