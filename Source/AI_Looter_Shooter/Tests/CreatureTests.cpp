#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/CombatRules.h"
#include "Combat/HealthComponent.h"
#include "Combat/TargetDummy.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureUpdateRate.h"
#include "Creatures/SpiderCreature.h"
#include "UI/World/CreatureHealthBarWidget.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UObjectHash.h"
#if WITH_EDITOR
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#endif

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

	// Design: the head is the critical spot; legs, thorax, abdomen ("butt"), fangs and feelers are not.
	const TPair<const TCHAR*, bool> Cases[] = {
		{ TEXT("head"), true }, { TEXT("body"), false }, { TEXT("abdomen"), false }, { TEXT("fang_l"), false },
		{ TEXT("fang_r"), false }, { TEXT("palp_l"), false }, { TEXT("femur_0_l"), false }, { TEXT("tibia_3_r"), false } };
	for (const TPair<const TCHAR*, bool>& Case : Cases)
	{
		FHitResult Hit;
		Hit.Component = Spider->GetMesh();
		Hit.BoneName = Case.Key;
		TestEqual(FString::Printf(TEXT("%s critical"), Case.Key), Spider->IsCriticalSpot(Hit), Case.Value);
	}

	// Only the spider's own mesh counts: a hit without a component or bone is never critical.
	FHitResult NoBone;
	NoBone.Component = Spider->GetMesh();
	TestFalse(TEXT("No bone is not critical"), Spider->IsCriticalSpot(NoBone));
	FHitResult NoComponent;
	NoComponent.BoneName = TEXT("head");
	TestFalse(TEXT("No component is not critical"), Spider->IsCriticalSpot(NoComponent));

	TArray<UObject*> Subobjects;
	GetObjectsWithOuter(Spider, Subobjects, EGetObjectsFlags::None);
	const UHealthComponent* Health = nullptr;
	for (UObject* Subobject : Subobjects)
	{
		Health = Health ? Health : Cast<UHealthComponent>(Subobject);
	}
	if (TestNotNull(TEXT("Spider has health"), Health))
	{
		TestEqual(TEXT("Spider max health"), Health->MaxHealth, 300.f);
	}
	return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderHitZonesTest, "Looter.Creatures.Spider.HitZones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderHitZonesTest::RunTest(const FString& Parameters)
{
	// Shots at a part without a hit zone pass straight through it (they once did at the fangs). Every vertex of the
	// spider must lie inside the hit zone of the bone it moves with; then every part is covered in every pose.
	const USkeletalMesh* Model = GetDefault<ASpiderCreature>()->GetMesh()->GetSkeletalMeshAsset();
	if (!TestNotNull(TEXT("Spider model"), Model))
	{
		return false;
	}
	UPhysicsAsset* Physics = Model->GetPhysicsAsset();
	if (!TestNotNull(TEXT("Spider hit zones (physics asset)"), Physics))
	{
		return false;
	}
	TestEqual(TEXT("Hit zones (one per drawn part)"), Physics->SkeletalBodySetups.Num(), 23);

	// Hulls are measured with their physics shapes.
	for (USkeletalBodySetup* Body : Physics->SkeletalBodySetups)
	{
		if (Body->AggGeom.ConvexElems.ContainsByPredicate([](const FKConvexElem& Hull) { return !Hull.GetChaosConvexMesh().IsValid(); }))
		{
			Body->CreatePhysicsMeshes();
		}
	}

	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	int32 Vertices = 0;
	TMap<FName, int32> Outside;
	float Farthest = 0.f;
	for (const FSkelMeshSection& Section : Model->GetImportedModel()->LODModels[0].Sections)
	{
		for (const FSoftSkinVertex& Vertex : Section.SoftVertices)
		{
			++Vertices;
			uint16 SectionBone = 0;
			const FName Bone = Vertex.GetRigidWeightBone(SectionBone) ? Skeleton.GetBoneName(Section.BoneMap[SectionBone]) : NAME_None;
			const int32 Body = Bone.IsNone() ? INDEX_NONE : Physics->FindBodyIndex(Bone);
			const float Distance = Body == INDEX_NONE ? UE_BIG_NUMBER : Physics->SkeletalBodySetups[Body]->GetShortestDistanceToPoint(
				FVector(Vertex.Position), FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Skeleton.FindBoneIndex(Bone)), true);
			if (Distance > 0.5f)
			{
				++Outside.FindOrAdd(Bone);
				Farthest = FMath::Max(Farthest, Distance);
			}
		}
	}
	TestTrue(TEXT("The model has vertices"), Vertices > 1000);
	for (const TPair<FName, int32>& Part : Outside)
	{
		AddError(FString::Printf(TEXT("%d vertices on %s lie outside its hit zone (farthest %.1f cm)"), Part.Value, *Part.Key.ToString(), Farthest));
	}
	return true;
}
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderShotsTest, "Looter.Creatures.Spider.Shots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderShotsTest::RunTest(const FString& Parameters)
{
	// Shots report the part they hit: a spider facing +X, standing at the origin in its resting pose.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ASpiderCreature* Spider = WorldWrapper.GetTestWorld()->SpawnActor<ASpiderCreature>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Spider spawned"), Spider))
	{
		return false;
	}
	USkeletalMeshComponent* Mesh = Spider->GetMesh();

	// Bullets trace the weapon channel: the hit zones block it, and nothing ever bumps into them. A skeletal mesh's
	// bodies collide as the mesh says (FBodyInstance::BuildBodyFilterData), unless a body's own collision is off. (A
	// preview world's scene doesn't pick up new bodies for world traces, so the shots below trace the mesh itself.)
	TestTrue(TEXT("Hit zones are only for queries"), Mesh->GetCollisionEnabled() == ECollisionEnabled::QueryOnly);
	TestTrue(TEXT("Hit zones block shots"), Mesh->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Block);
	TestTrue(TEXT("Hit zones let pawns through"), Mesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
	TestTrue(TEXT("Hit zones let the camera through"), Mesh->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore);
	TestEqual(TEXT("Hit zones"), Mesh->Bodies.Num(), 23);
	for (const FBodyInstance* Zone : Mesh->Bodies)
	{
		const UBodySetup* Setup = Zone ? Zone->GetBodySetup() : nullptr;
		if (TestNotNull(TEXT("Hit zone setup"), Setup))
		{
			TestTrue(FString::Printf(TEXT("%s collides"), *Setup->BoneName.ToString()), Setup->CollisionReponse != EBodyCollisionResponse::BodyCollision_Disabled);
			// Bullets trace complex collision (UBulletSubsystem), and the zones are simple shapes: they must answer both.
			TestTrue(FString::Printf(TEXT("%s answers complex traces"), *Setup->BoneName.ToString()), Setup->CollisionTraceFlag == CTF_UseSimpleAsComplex);
		}
	}

	const FVector Body = Mesh->GetBoneLocation(TEXT("body"));
	const FVector FemurMiddle = (Mesh->GetBoneLocation(TEXT("femur_0_r")) + Mesh->GetBoneLocation(TEXT("tibia_0_r"))) * 0.5;
	// Traced the way bullets are: complex collision.
	const FCollisionQueryParams BulletQuery(SCENE_QUERY_STAT(SpiderShotsTest), /*bTraceComplex*/ true);
	struct FShot
	{
		const TCHAR* What;
		FVector From;
		FVector To;
		const TCHAR* Bone;  // nullptr: a miss
		bool bCritical;
	};
	// From the thorax's center: the head is ahead of it, the abdomen behind, the fangs below the head.
	const FShot Shots[] = {
		{ TEXT("head-on"), Body + FVector(400.0, 0.0, 10.0), Body + FVector(0.0, 0.0, 10.0), TEXT("head"), true },
		{ TEXT("thorax from above"), Body + FVector(0.0, 0.0, 200.0), Body - FVector(0.0, 0.0, 100.0), TEXT("body"), false },
		{ TEXT("abdomen from behind"), Body + FVector(-400.0, 0.0, 15.0), Body + FVector(-50.0, 0.0, 15.0), TEXT("abdomen"), false },
		{ TEXT("left fang"), Body + FVector(400.0, -6.0, -8.0), Body + FVector(0.0, -6.0, -8.0), TEXT("fang_l"), false },
		{ TEXT("front right femur from above"), FemurMiddle + FVector(0.0, 0.0, 150.0), FemurMiddle - FVector(0.0, 0.0, 150.0), TEXT("femur_0_r"), false },
		{ TEXT("well over it"), Body + FVector(400.0, 0.0, 150.0), Body + FVector(-400.0, 0.0, 150.0), nullptr, false } };
	for (const FShot& Shot : Shots)
	{
		FHitResult Hit;
		const bool bHit = Mesh->LineTraceComponent(Hit, Shot.From, Shot.To, BulletQuery);
		if (!Shot.Bone)
		{
			TestFalse(FString::Printf(TEXT("%s misses"), Shot.What), bHit);
			continue;
		}
		if (TestTrue(FString::Printf(TEXT("%s hits"), Shot.What), bHit))
		{
			TestEqual(FString::Printf(TEXT("%s: bone"), Shot.What), Hit.BoneName, FName(Shot.Bone));
			TestEqual(FString::Printf(TEXT("%s: critical"), Shot.What), Spider->IsCriticalSpot(Hit), Shot.bCritical);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureHealthBarDividersTest, "Looter.Creatures.HealthBarDividers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureHealthBarDividersTest::RunTest(const FString& Parameters)
{
	// Every creature's bar is the same length and cut into the same quarters, whatever its health: a 100 000-health
	// boss's bar is as clean as a spider's, and the lines say how much of its health is left, not how many hundreds.
	using UBar = UCreatureHealthBarWidget;
	const TArray<float> Lines = UBar::DividerPositions();
	if (TestEqual(TEXT("Three lines: quarters"), Lines.Num(), UBar::Parts - 1))
	{
		for (int32 Index = 0; Index < Lines.Num(); ++Index)
		{
			TestNearlyEqual(TEXT("Line on its quarter"), Lines[Index], (Index + 1) / static_cast<float>(UBar::Parts), 0.001f);
			TestTrue(TEXT("Inside the bar, never on its ends"), Lines[Index] > 0.f && Lines[Index] < 1.f);
		}
	}
	TestEqual(TEXT("Quarters"), UBar::Parts, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureUpdateRateTest, "Looter.Creatures.UpdateRate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureUpdateRateTest::RunTest(const FString& Parameters)
{
	// Distant creatures update less often: every frame up close, then twenty, ten and five times a second farther out.
	// Creatures busy with a player, and ones on screen nearby, always update every frame.
	const FCreatureUpdateRate Rate;
	TestEqual(TEXT("Near: every frame"), Rate.IntervalFor(1000.f, false, false), 0.f);
	TestEqual(TEXT("Just inside near"), Rate.IntervalFor(Rate.NearDistance - 1.f, false, false), 0.f);
	TestEqual(TEXT("Mid band"), Rate.IntervalFor(Rate.NearDistance + 1.f, false, false), Rate.MidInterval);
	TestEqual(TEXT("Far band"), Rate.IntervalFor(Rate.MidDistance + 1.f, false, false), Rate.FarInterval);
	TestEqual(TEXT("Very far"), Rate.IntervalFor(Rate.FarDistance + 1.f, false, false), Rate.VeryFarInterval);
	TestEqual(TEXT("Beyond every band: the slowest"), Rate.IntervalFor(1.0e7f, false, false), Rate.VeryFarInterval);

	// The user's bands: within 25 m every frame, about 1/20 s to 60 m, 1/10 s to 120 m, 1/5 s beyond.
	TestNearlyEqual(TEXT("Near band ends at 25 m"), Rate.NearDistance, 2500.f, 1.f);
	TestNearlyEqual(TEXT("Mid band ends at 60 m"), Rate.MidDistance, 6000.f, 1.f);
	TestNearlyEqual(TEXT("Far band ends at 120 m"), Rate.FarDistance, 12000.f, 1.f);
	TestNearlyEqual(TEXT("Mid: 20 a second"), Rate.MidInterval, 1.f / 20.f, 0.001f);
	TestNearlyEqual(TEXT("Far: 10 a second"), Rate.FarInterval, 1.f / 10.f, 0.001f);
	TestNearlyEqual(TEXT("Very far: 5 a second"), Rate.VeryFarInterval, 1.f / 5.f, 0.001f);

	// Busy with a player (hunting, attacking, alerted or just hurt): every frame, however far.
	for (const float Distance : { 3000.f, 9000.f, 50000.f })
	{
		TestEqual(FString::Printf(TEXT("Engaged at %.0f m: every frame"), Distance / 100.f), Rate.IntervalFor(Distance, true, false), 0.f);
		TestFalse(FString::Printf(TEXT("Engaged at %.0f m: posed"), Distance / 100.f), Rate.ShouldFreezePose(Distance, true, false));
	}
	TestTrue(TEXT("Chasing needs every frame"), ACreatureBase::NeedsFullRate(ECreatureState::Chase, false, false));
	TestTrue(TEXT("Attacking needs every frame"), ACreatureBase::NeedsFullRate(ECreatureState::Attack, false, false));
	TestTrue(TEXT("Alerted (a target) needs every frame"), ACreatureBase::NeedsFullRate(ECreatureState::Return, true, false));
	TestTrue(TEXT("Just hurt needs every frame"), ACreatureBase::NeedsFullRate(ECreatureState::Idle, false, true));
	TestTrue(TEXT("Just killed needs every frame (its death plays)"), ACreatureBase::NeedsFullRate(ECreatureState::Dead, false, true));
	for (const ECreatureState Calm : { ECreatureState::Idle, ECreatureState::Wander, ECreatureState::Return, ECreatureState::Dead })
	{
		TestFalse(FString::Printf(TEXT("%s alone can slow down"), *UEnum::GetValueAsString(Calm)), ACreatureBase::NeedsFullRate(Calm, false, false));
	}

	// On screen nearby: every frame (choppy motion would show); farther out on screen, the bands again.
	TestEqual(TEXT("On screen at 30 m: every frame"), Rate.IntervalFor(3000.f, false, true), 0.f);
	TestEqual(TEXT("Off screen at 30 m: mid band"), Rate.IntervalFor(3000.f, false, false), Rate.MidInterval);
	TestEqual(TEXT("On screen at 90 m: far band"), Rate.IntervalFor(9000.f, false, true), Rate.FarInterval);

	// The pose stops only off screen, far enough away, and never while busy.
	TestFalse(TEXT("Near: posed"), Rate.ShouldFreezePose(1000.f, false, false));
	TestTrue(TEXT("Off screen and far: frozen"), Rate.ShouldFreezePose(9000.f, false, false));
	TestFalse(TEXT("On screen and far: posed"), Rate.ShouldFreezePose(9000.f, false, true));

	// Farther never updates more often, and no band is slower than the bodies' animation takes in one step.
	float Previous = 0.f;
	for (float Distance = 0.f; Distance <= 20000.f; Distance += 250.f)
	{
		const float Interval = Rate.IntervalFor(Distance, false, false);
		if (Interval < Previous)
		{
			AddError(FString::Printf(TEXT("%.0f m updates more often (%.2f s) than nearer (%.2f s)"), Distance / 100.f, Interval, Previous));
		}
		TestTrue(TEXT("Within what the animation takes"), Interval <= FCreatureUpdateRate::MaxInterval);
		Previous = Interval;
	}
	FCreatureUpdateRate Mistyped;
	Mistyped.VeryFarInterval = 3.f;
	TestEqual(TEXT("A mistyped interval is clamped"), Mistyped.IntervalFor(50000.f, false, false), FCreatureUpdateRate::MaxInterval);

	// Switched off: every frame, always posed.
	FCreatureUpdateRate Off;
	Off.bEnabled = false;
	TestEqual(TEXT("Off: every frame"), Off.IntervalFor(50000.f, false, false), 0.f);
	TestFalse(TEXT("Off: always posed"), Off.ShouldFreezePose(50000.f, false, false));

	// Every creature starts from these defaults.
	TestEqual(TEXT("Spiders use the default bands"), GetDefault<ASpiderCreature>()->UpdateRate.FarDistance, Rate.FarDistance);
	TestTrue(TEXT("Spiders' update rates are on"), GetDefault<ASpiderCreature>()->UpdateRate.bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCreatureUpdateRateViewTest, "Looter.Creatures.UpdateRateView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCreatureUpdateRateViewTest::RunTest(const FString& Parameters)
{
	// On screen means in a player's view, walls and all: drawn-on-screen time stops behind cover, and a body frozen behind
	// a rock would pop as it stepped out. A camera at the origin looks along +X, 90 degrees across (corners at about 49).
	const FCreatureUpdateRate Rate;
	const FVector Eye = FVector::ZeroVector;
	const FVector Ahead = FVector::ForwardVector;
	auto Around = [](float Degrees, float Distance)
	{
		const float Radians = FMath::DegreesToRadians(Degrees);
		return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.f) * Distance;
	};
	auto Seen = [&Eye, &Ahead](float FieldOfView, const FVector& Point, float Radius, float Margin)
	{
		return FCreatureUpdateRate::IsInView(Eye, Ahead, FieldOfView, Point, Radius, Margin);
	};
	TestTrue(TEXT("Straight ahead"), Seen(90.f, Around(0.f, 5000.f), 100.f, 0.f));
	TestTrue(TEXT("Near the side of the screen"), Seen(90.f, Around(40.f, 5000.f), 0.f, 0.f));
	TestFalse(TEXT("Past the screen's corners"), Seen(90.f, Around(55.f, 5000.f), 0.f, 0.f));
	TestTrue(TEXT("Past the corners, within the margin (a turning view reaches it next)"), Seen(90.f, Around(55.f, 5000.f), 0.f, Rate.ViewMargin));
	TestTrue(TEXT("A big body reaching into view"), Seen(90.f, Around(55.f, 2000.f), 600.f, 0.f));
	TestFalse(TEXT("Off to the side"), Seen(90.f, Around(90.f, 5000.f), 150.f, Rate.ViewMargin));
	TestFalse(TEXT("Behind"), Seen(90.f, Around(180.f, 5000.f), 150.f, Rate.ViewMargin));
	TestTrue(TEXT("A camera inside the body sees it"), Seen(90.f, Around(180.f, 50.f), 100.f, 0.f));

	// Through a scope a creature looks nearer by the zoom, and updates as if it were that near. A sight narrows the view
	// in tangents (UPlayerViewComponent): through 2x, it shows half as wide.
	auto SightView = [&Rate](float Sight)
	{
		return FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(Rate.ReferenceFieldOfView) * 0.5f) / Sight));
	};
	TestNearlyEqual(TEXT("No zoom at the bands' own view"), Rate.ZoomFor(Rate.ReferenceFieldOfView), 1.f, 0.001f);
	TestNearlyEqual(TEXT("A wide view never slows the bands"), Rate.ZoomFor(110.f), 1.f, 0.001f);
	for (const float Sight : { 1.25f, 2.f, 4.f, 8.f })
	{
		TestNearlyEqual(FString::Printf(TEXT("A %.2fx sight zooms %.2fx"), Sight, Sight), Rate.ZoomFor(SightView(Sight)), Sight, 0.01f);
	}
	const float FourTimes = SightView(4.f);
	TestEqual(TEXT("100 m through a 4x scope looks 25 m away: every frame"), Rate.IntervalFor(10000.f / Rate.ZoomFor(FourTimes), false, true), 0.f);
	TestEqual(TEXT("100 m unzoomed: the far band"), Rate.IntervalFor(10000.f / Rate.ZoomFor(Rate.ReferenceFieldOfView), false, true), Rate.FarInterval);
	TestTrue(TEXT("In the scope's view"), Seen(FourTimes, Around(0.f, 10000.f), 100.f, 0.f));
	TestFalse(TEXT("Beside the scope's view: not magnified"), Seen(FourTimes, Around(30.f, 10000.f), 100.f, 0.f));
	return true;
}

#endif
