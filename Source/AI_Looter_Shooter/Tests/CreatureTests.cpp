#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/CombatRules.h"
#include "Combat/HealthComponent.h"
#include "Combat/TargetDummy.h"
#include "Creatures/SpiderCreature.h"
#include "Components/CapsuleComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UObjectHash.h"

namespace
{
	TArray<UPrimitiveComponent*> FindTaggedShapes(const UObject* Owner, FName Tag)
	{
		TArray<UObject*> Subobjects;
		GetObjectsWithOuter(Owner, Subobjects, EGetObjectsFlags::None);
		TArray<UPrimitiveComponent*> Shapes;
		for (UObject* Subobject : Subobjects)
		{
			UPrimitiveComponent* Shape = Cast<UPrimitiveComponent>(Subobject);
			if (Shape && Shape->ComponentHasTag(Tag))
			{
				Shapes.Add(Shape);
			}
		}
		return Shapes;
	}

	/** True if a world-space point lies inside a capsule hit shape (with a hair of slack for round-off). */
	bool IsInsideCapsule(const UPrimitiveComponent* Shape, const FVector& Point)
	{
		const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Shape);
		if (!Capsule)
		{
			return false;
		}
		const FVector Local = Capsule->GetComponentTransform().InverseTransformPosition(Point);
		const double Segment = Capsule->GetUnscaledCapsuleHalfHeight_WithoutHemisphere();
		const FVector OnAxis(0.0, 0.0, FMath::Clamp(Local.Z, -Segment, Segment));
		return FVector::Dist(Local, OnAxis) <= Capsule->GetUnscaledCapsuleRadius() + 0.01;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCriticalHitRuleTest, "Looter.Combat.CriticalHitRule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCriticalHitRuleTest::RunTest(const FString& Parameters)
{
	// Game rule: a hit on a critical spot deals exactly 1.5x what the same hit would deal anywhere else.
	TestEqual(TEXT("Critical multiplier"), LooterCombat::CriticalHitMultiplier, 1.5f);
	TestEqual(TEXT("Critical hit damage (middle of the range)"), LooterCombat::HitDamage(20.f, true, 0.5f), 30.f);
	TestEqual(TEXT("Normal hit damage (middle of the range)"), LooterCombat::HitDamage(20.f, false, 0.5f), 20.f);
	for (const float Roll : { 0.f, 0.25f, 0.8f, 1.f })
	{
		TestEqual(FString::Printf(TEXT("Crit is 1.5x the same roll (%.2f)"), Roll), LooterCombat::HitDamage(20.f, true, Roll),
			LooterCombat::HitDamage(20.f, false, Roll) * 1.5f);
	}

	// Target dummies: the head bone is the critical spot.
	const ATargetDummy* Dummy = GetDefault<ATargetDummy>();
	FHitResult HeadHit;
	HeadHit.BoneName = TEXT("head");
	FHitResult BodyHit;
	BodyHit.BoneName = TEXT("spine_03");
	TestTrue(TEXT("Dummy head is critical"), Dummy->IsCriticalSpot(HeadHit));
	TestFalse(TEXT("Dummy body is not critical"), Dummy->IsCriticalSpot(BodyHit));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageRangeTest, "Looter.Combat.DamageRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDamageRangeTest::RunTest(const FString& Parameters)
{
	// Game rule: every hit rolls within +/-10% of the weapon's damage (the one number its card shows), which is the average.
	TestEqual(TEXT("Variance"), LooterCombat::DamageVariance, 0.1f);
	TestEqual(TEXT("Lowest roll"), LooterCombat::HitDamage(20.f, false, 0.f), 18.f);
	TestEqual(TEXT("Highest roll"), LooterCombat::HitDamage(20.f, false, 1.f), 22.f);
	TestEqual(TEXT("Hit minimum"), LooterCombat::MinHitDamage(20.f), 18.f);
	TestEqual(TEXT("Hit maximum"), LooterCombat::MaxHitDamage(20.f), 22.f);
	TestEqual(TEXT("Rolls outside [0,1] clamp"), LooterCombat::HitDamage(20.f, false, 7.f), 22.f);

	// Random rolls: always in range, actually spread out, and centered on the weapon's damage.
	constexpr int32 Samples = 4000;
	float Lowest = TNumericLimits<float>::Max();
	float Highest = TNumericLimits<float>::Lowest();
	double Sum = 0.0;
	for (int32 Index = 0; Index < Samples; ++Index)
	{
		const float Damage = LooterCombat::RollHitDamage(20.f, false);
		Lowest = FMath::Min(Lowest, Damage);
		Highest = FMath::Max(Highest, Damage);
		Sum += Damage;
	}
	TestTrue(TEXT("Never below the range"), Lowest >= 18.f - UE_KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Never above the range"), Highest <= 22.f + UE_KINDA_SMALL_NUMBER);
	TestTrue(TEXT("Hits are not all the same"), Highest - Lowest > 3.5f);
	TestTrue(TEXT("Average is the weapon's damage"),FMath::IsNearlyEqual(static_cast<float>(Sum / Samples), 20.f, 0.15f));

	for (int32 Index = 0; Index < 200; ++Index)
	{
		const float Crit = LooterCombat::RollHitDamage(20.f, true);
		if (Crit < 27.f - UE_KINDA_SMALL_NUMBER || Crit > 33.f + UE_KINDA_SMALL_NUMBER)
		{
			AddError(FString::Printf(TEXT("Critical roll %.2f outside 27-33"), Crit));
			break;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderCriticalSpotTest, "Looter.Creatures.Spider.CriticalSpot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderCriticalSpotTest::RunTest(const FString& Parameters)
{
	const ASpiderCreature* Spider = GetDefault<ASpiderCreature>();

	// Design: the head is the critical spot; legs, thorax, abdomen ("butt") and fangs are not.
	struct FCase
	{
		const TCHAR* Tag;
		bool bCritical;
	};
	const FCase Cases[] = {
		{ TEXT("Head"), true },
		{ TEXT("Body"), false },
		{ TEXT("Abdomen"), false },
		{ TEXT("Leg"), false },
		{ TEXT("Fang"), false } };

	for (const FCase& Case : Cases)
	{
		const TArray<UPrimitiveComponent*> Shapes = FindTaggedShapes(Spider, Case.Tag);
		TestTrue(FString::Printf(TEXT("%s has hit shapes"), Case.Tag), Shapes.Num() > 0);
		for (UPrimitiveComponent* Shape : Shapes)
		{
			FHitResult Hit;
			Hit.Component = Shape;
			TestEqual(FString::Printf(TEXT("%s (%s) critical"), Case.Tag, *Shape->GetName()), Spider->IsCriticalSpot(Hit), Case.bCritical);
		}
	}

	// Two segments per leg, eight legs; one shape per fang.
	TestEqual(TEXT("Leg hit shapes"), FindTaggedShapes(Spider, TEXT("Leg")).Num(), 16);
	TestEqual(TEXT("Fang hit shapes"), FindTaggedShapes(Spider, TEXT("Fang")).Num(), 2);

	// A hit with no component (or an untagged one) is never critical.
	TestFalse(TEXT("Untagged is not critical"), Spider->IsCriticalSpot(FHitResult()));

	TArray<UObject*> Subobjects;
	GetObjectsWithOuter(Spider, Subobjects, EGetObjectsFlags::None);
	const UHealthComponent* Health = nullptr;
	for (UObject* Subobject : Subobjects)
	{
		Health = Health ? Health : Cast<UHealthComponent>(Subobject);
	}
	if (TestNotNull(TEXT("Spider has health"), Health))
	{
		TestEqual(TEXT("Spider max health"), Health->MaxHealth, 150.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderFangHitShapesTest, "Looter.Creatures.Spider.FangHitShapes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderFangHitShapesTest::RunTest(const FString& Parameters)
{
	// Shots at the chelicerae and fangs used to pass straight through them. Every vertex of both fang meshes has to sit
	// inside a Fang hit shape, whether the fangs are closed or spread for a bite.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	// Spawning runs the construction script, which builds the meshes.
	ASpiderCreature* Spider = WorldWrapper.GetTestWorld()->SpawnActor<ASpiderCreature>();
	if (!TestNotNull(TEXT("Spider spawned"), Spider))
	{
		return false;
	}

	const TArray<UPrimitiveComponent*> FangShapes = FindTaggedShapes(Spider, TEXT("Fang"));
	TArray<UDynamicMeshComponent*> Fangs;
	Spider->GetComponents(Fangs);
	Fangs.RemoveAll([](const UDynamicMeshComponent* Part) { return !Part->GetName().StartsWith(TEXT("Fang")); });
	TestEqual(TEXT("Fang meshes"), Fangs.Num(), 2);

	// Closed, spread for a bite (the attack opens them to 30 degrees), and wider still.
	for (const float Spread : { 0.f, 30.f, 45.f })
	{
		for (UDynamicMeshComponent* Fang : Fangs)
		{
			const float Outward = Fang->GetRelativeLocation().Y < 0.f ? -1.f : 1.f;
			Fang->SetRelativeRotation(FRotator(Spread * 0.4f, Outward * Spread, 0.f));
			const FTransform FangToWorld = Fang->GetComponentTransform();
			int32 Vertices = 0;
			int32 Outside = 0;
			Fang->ProcessMesh([&](const UE::Geometry::FDynamicMesh3& Geometry)
			{
				for (const int32 Vertex : Geometry.VertexIndicesItr())
				{
					const FVector Point = FangToWorld.TransformPosition(Geometry.GetVertex(Vertex));
					++Vertices;
					Outside += FangShapes.ContainsByPredicate([&Point](const UPrimitiveComponent* Shape) { return IsInsideCapsule(Shape, Point); }) ? 0 : 1;
				}
			});
			TestTrue(FString::Printf(TEXT("%s mesh is built"), *Fang->GetName()), Vertices > 0);
			TestEqual(FString::Printf(TEXT("%s vertices outside the fang hit shapes (spread %.0f)"), *Fang->GetName(), Spread), Outside, 0);
		}
	}
	return true;
}

#endif
