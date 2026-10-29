#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The creatures' motion through the moments Looter.CastShots caught going wrong (2026-10-09): a spider's death (its legs
// snapped to a new bend and stood up in a cage through its abdomen; since 2026-10-14 a throe, a drop and a curl under the
// body), its bite's raised front legs (they leapt off the ground), and an Unpaid banking into a turn (its head and hat
// jumped 30-50 cm in a frame when a turn began).

#include "Creatures/CreatureBodyHulls.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "Tests/BossTestWorld.h"
#include "AnimationRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Tests/AutomationCommon.h"

namespace
{
	constexpr float Frame = 1.f / 60.f;

	/** A creature in a test world, started as play starts it (a test world never begins play itself), dropping no loot. */
	template <typename T>
	T* SpawnStarted(UWorld* World, const FVector& Where)
	{
		T* Creature = World->SpawnActor<T>(Where, FRotator::ZeroRotator);
		if (Creature && !Creature->HasActorBegunPlay())
		{
			Creature->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Creature);
		return Creature;
	}

	/** The pose the creature's code worked out (component space), by bone. */
	TMap<FName, FTransform> PoseOf(const ACreatureBase& Creature)
	{
		TMap<FName, FTransform> Pose;
		for (const FCreatureBonePose& Bone : Creature.GetBonePose())
		{
			Pose.Add(Bone.Bone, Bone.Transform);
		}
		return Pose;
	}

	/**
	 * Watches posed bones frame by frame: the biggest step any takes in a frame, and the biggest change from one frame's
	 * step to the next (a pop: a bone that starts or stops at once).
	 */
	struct FMotionWatch
	{
		TMap<FName, FVector> Last;
		TMap<FName, FVector> LastStep;
		float BiggestStep = 0.f;
		float BiggestJolt = 0.f;
		FName JoltBone;

		void Sample(const TMap<FName, FTransform>& Pose, TConstArrayView<FName> Bones)
		{
			for (const FName Bone : Bones)
			{
				const FTransform* Now = Pose.Find(Bone);
				if (!Now)
				{
					continue;
				}
				if (const FVector* Was = Last.Find(Bone))
				{
					const FVector Step = Now->GetLocation() - *Was;
					BiggestStep = FMath::Max(BiggestStep, static_cast<float>(Step.Size()));
					const FVector* WasStep = LastStep.Find(Bone);
					const float Jolt = static_cast<float>(FVector::Dist(Step, WasStep ? *WasStep : FVector::ZeroVector));
					if (Jolt > BiggestJolt)
					{
						BiggestJolt = Jolt;
						JoltBone = Bone;
					}
					LastStep.Add(Bone, Step);
				}
				Last.Add(Bone, Now->GetLocation());
			}
		}
	};

	/** Every posed bone's name. */
	TArray<FName> PosedBones(const ACreatureBase& Creature)
	{
		TArray<FName> Names;
		for (const FCreatureBonePose& Bone : Creature.GetBonePose())
		{
			Names.Add(Bone.Bone);
		}
		return Names;
	}

	/**
	 * A femur's hull within this of its hip (cm, size 1) is its coxa, the ball at the hip joint, buried in the body: a back
	 * femur laid down turns its inner end into the abdomen's front by about 5 cm where nothing shows, so only the femur
	 * beyond it is measured.
	 */
	constexpr float SpiderCoxaReach = 10.f;
	/** The carapace's top over the thorax bone (Spider.py: its middle 2 cm up, 22 cm high), cm at size 1. */
	constexpr float SpiderThoraxTop = 24.f;

	/**
	 * How deep the spider's femurs' hit hulls reach into its abdomen's past where they reach at rest, in its code's pose (cm),
	 * the coxa left out (SpiderCoxaReach), and so is any of a femur's hull still inside the thorax's: it doesn't show. Since
	 * the 2026-10-22 remodel (Spider.py's FEMUR_SEGS) the coxa starts 8 cm behind the hip, its rim there 11-12 cm from the
	 * hip, past SpiderCoxaReach: a back femur laid down swung that buried rim 4-5 cm into the abdomen's front inside the
	 * carapace, where nothing shows, and the dead pose's numbers could only trade it against the femur's visible flank.
	 */
	float FemursIntoAbdomen(const ASpiderCreature& Spider, FString& OutWhere)
	{
		const USkeletalMesh* Model = Spider.GetMesh()->GetSkeletalMeshAsset();
		const UPhysicsAsset* Hulls = Spider.GetMesh()->GetPhysicsAsset();
		if (!Model || !Hulls)
		{
			return 0.f;
		}
		const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
		TArray<FName> FemurNames;
		for (int32 Pair = 0; Pair < 4; ++Pair)
		{
			FemurNames.Add(*FString::Printf(TEXT("femur_%d_l"), Pair));
			FemurNames.Add(*FString::Printf(TEXT("femur_%d_r"), Pair));
		}
		FCreatureBodyHulls Femurs;
		FCreatureBodyHulls Abdomen;
		FCreatureBodyHulls Thorax;
		Femurs.AddFromPhysicsAsset(Hulls, &Skeleton, FemurNames);
		Abdomen.AddFromPhysicsAsset(Hulls, &Skeleton, { FName(TEXT("abdomen")) });
		Thorax.AddFromPhysicsAsset(Hulls, &Skeleton, { FName(TEXT("body")) });
		const TMap<FName, FTransform> Pose = PoseOf(Spider);
		// A point (component space) inside the thorax's hull, posed: buried in the carapace, out of sight.
		auto Hidden = [&Thorax, &Pose](const FVector& Point)
		{
			for (const FCreatureBodyHulls::FShape& Shape : Thorax.Shapes)
			{
				const FTransform* Posed = Pose.Find(Shape.Bone);
				if (Posed && FCreatureBodyHulls::Depth(Shape, Posed->InverseTransformPosition(Point)) > 0.f)
				{
					return true;
				}
			}
			return false;
		};
		float Deepest = 0.f;
		for (const FCreatureBodyHulls::FShape& Femur : Femurs.Shapes)
		{
			const FTransform* FemurPosed = Pose.Find(Femur.Bone);
			const FTransform FemurRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Femur.BoneIndex);
			for (const FCreatureBodyHulls::FShape& Body : Abdomen.Shapes)
			{
				const FTransform* BodyPosed = Pose.Find(Body.Bone);
				if (!FemurPosed || !BodyPosed)
				{
					continue;
				}
				const FTransform BodyRest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Body.BoneIndex);
				for (const FVector& Point : Femur.Points)
				{
					// Points are in the femur bone's space, whose origin is the hip.
					if (Point.SizeSquared() < FMath::Square(SpiderCoxaReach))
					{
						continue;
					}
					const FVector PointPosed = FemurPosed->TransformPosition(Point);
					const float Now = FCreatureBodyHulls::Depth(Body, BodyPosed->InverseTransformPosition(PointPosed));
					if (Now <= 0.f || Hidden(PointPosed))
					{
						continue;
					}
					const float AtRest = FCreatureBodyHulls::Depth(Body, BodyRest.InverseTransformPosition(FemurRest.TransformPosition(Point)));
					const float Past = Now - FMath::Max(AtRest, 0.f);
					if (Past > Deepest)
					{
						Deepest = Past;
						OutWhere = FString::Printf(TEXT("%s, %.0f cm from its hip"), *Femur.Bone.ToString(), Point.Size());
					}
				}
			}
		}
		return Deepest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderDeathCurlTest, "Looter.Creatures.Anim.SpiderDeathCurl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderDeathCurlTest::RunTest(const FString& Parameters)
{
	// Dead, it throws a short throe (it rears, its legs kicking), drops hard onto the ground on its death cry's knock and
	// bounces once, then folds every leg under its body: knees low and drawn in, tips under the thorax and abdomen, nothing
	// above the body (2026-10-14: the curl before stood its knees up and out round it like a cage). All of it without a jump
	// or a snap, and with its femurs out of its abdomen (an older curl stood them straight up through it, 25-48 cm).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	ASpiderCreature* Spider = SpawnStarted<ASpiderCreature>(WorldWrapper.GetTestWorld(), FVector::ZeroVector);
	if (!TestNotNull(TEXT("Spider spawned"), Spider))
	{
		return false;
	}
	if (Spider->GetBonePose().IsEmpty() || !Spider->GetMesh()->GetPhysicsAsset())
	{
		AddWarning(TEXT("SK_Spider (with its hit hulls) isn't imported: its death curl isn't checked."));
		return true;
	}
	for (float Elapsed = 0.f; Elapsed < 0.5f; Elapsed += Frame)
	{
		Spider->Tick(Frame);
	}
	// The knees (each tibia's root) as the body carries them, and against the body (the body's own drop starts at once).
	TArray<FName> Knees;
	for (const FName Bone : PosedBones(*Spider))
	{
		if (Bone.ToString().StartsWith(TEXT("tibia_")))
		{
			Knees.Add(Bone);
		}
	}
	auto KneesOnBody = [Spider, &Knees]()
	{
		TMap<FName, FTransform> Pose = PoseOf(*Spider);
		const FVector Body = Pose.Contains(TEXT("body")) ? Pose[TEXT("body")].GetLocation() : FVector::ZeroVector;
		for (const FName Knee : Knees)
		{
			if (FTransform* Each = Pose.Find(Knee))
			{
				Each->SetLocation(Each->GetLocation() - Body);
			}
		}
		return Pose;
	};
	// The thorax's height over the ground (the pose is in the mesh's space, whose origin is the ground under the thorax).
	auto BodyHeight = [Spider]()
	{
		const TMap<FName, FTransform> Pose = PoseOf(*Spider);
		const FTransform* Body = Pose.Find(TEXT("body"));
		return Body ? Body->GetLocation().Z : 0.0;
	};
	const float Size = Spider->GetSizeScale();
	const float Slower = ASpiderCreature::DeathTimeScale(Size);
	const float Lands = ASpiderCreature::DropSeconds * Slower;
	const double Standing = BodyHeight();
	FMotionWatch Watch;
	FMotionWatch OnBody;
	Watch.Sample(PoseOf(*Spider), Knees);
	OnBody.Sample(KneesOnBody(), Knees);
	BossTestWorld::Kill(Spider);
	TestTrue(TEXT("It dies"), Spider->IsDead());
	double Reared = Standing;
	double Landed = -1.0;
	double Bounced = 0.0;
	float Dead = 0.f;
	while (Dead < (ASpiderCreature::CurledSeconds + 0.3f) * Slower)
	{
		Spider->Tick(Frame);
		Dead += Frame;
		Watch.Sample(PoseOf(*Spider), Knees);
		OnBody.Sample(KneesOnBody(), Knees);
		const double Height = BodyHeight();
		if (Dead < Lands - 0.1f * Slower)
		{
			Reared = FMath::Max(Reared, Height);
		}
		else if (Dead >= Lands && Landed < 0.0)
		{
			Landed = Height;
		}
		if (Dead > Lands && Dead < Lands + 0.3f * Slower)
		{
			Bounced = FMath::Max(Bounced, Height);
		}
	}
	// The throe and the drop: it rears about 14 cm, lands on its dead ride as its cry knocks (the ground takes it over a few
	// frames: a dip of a couple of cm), bounces about 6 cm, and lies still on its folded legs by the curl's end.
	const double DeadRide = ASpiderCreature::DeadRide * Size;
	TestTrue(FString::Printf(TEXT("It rears in its throe (%.1f cm up)"), Reared - Standing), Reared - Standing > 8.0 * Size);
	TestTrue(FString::Printf(TEXT("...drops onto the ground on its cry's knock (%.1f cm off its dead ride)"), Landed - DeadRide),
		Landed >= 0.0 && FMath::Abs(Landed - DeadRide) < 4.0 * Size);
	TestTrue(FString::Printf(TEXT("...bounces once (%.1f cm)"), Bounced - DeadRide), Bounced - DeadRide > 3.0 * Size);
	TestTrue(FString::Printf(TEXT("...and lies on its folded legs (%.1f cm off its dead ride)"), BodyHeight() - DeadRide),
		FMath::Abs(BodyHeight() - DeadRide) < 1.5 * Size);

	// Fast, never a jump: the rear and the drop carry a front knee about 10 cm a frame (12 at most), the curl its knees 5-7.
	// Against the body no snap to a new bend (an old curl's knees swung 15-20 cm over in its first frame): the kicks, the
	// rear's turn and the ground taking the drop all ease in and out over a few frames (about 3.5 cm at most).
	TestTrue(FString::Printf(TEXT("The legs move without a jump (%.1f cm in a frame at most)"), Watch.BiggestStep), Watch.BiggestStep < 12.f);
	TestTrue(FString::Printf(TEXT("...or a snap (against the body a knee's step changes by %.1f cm at most, the %s)"), OnBody.BiggestJolt,
		*OnBody.JoltBone.ToString()), OnBody.BiggestJolt < 4.f);

	FString Where;
	const float Deepest = FemursIntoAbdomen(*Spider, Where);
	TestTrue(FString::Printf(TEXT("Curled up, its femurs keep out of its abdomen (%.1f cm past rest at worst: %s)"), Deepest, *Where), Deepest <= 3.f);

	// Curled under: every knee low on the ground and drawn in toward the body's line, every tip within the body's footprint
	// (under the thorax and abdomen), and no part of any leg above the body.
	const TMap<FName, FTransform> Pose = PoseOf(*Spider);
	const FTransform* Body = Pose.Find(TEXT("body"));
	const USkeletalMesh* Model = Spider->GetMesh()->GetSkeletalMeshAsset();
	if (!TestNotNull(TEXT("Its body is posed"), Body) || !TestNotNull(TEXT("Its model"), Model))
	{
		return false;
	}
	const FReferenceSkeleton& Skeleton = Model->GetRefSkeleton();
	const FTransform ToWorld = Spider->GetMesh()->GetComponentTransform();
	FVector FootprintFront;
	FVector FootprintBack;
	float FootprintRadius = 0.f;
	Spider->GetFootprintInWorld(FootprintFront, FootprintBack, FootprintRadius);
	FootprintFront.Z = 0.0;
	FootprintBack.Z = 0.0;
	const double ThoraxTop = Body->GetLocation().Z + SpiderThoraxTop * Size;
	for (int32 Pair = 0; Pair < 4; ++Pair)
	{
		for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
		{
			const FString Leg = FString::Printf(TEXT("%d_%s"), Pair, Side);
			const FTransform* Hip = Pose.Find(*(TEXT("femur_") + Leg));
			const FTransform* Knee = Pose.Find(*(TEXT("tibia_") + Leg));
			const int32 TibiaIndex = Skeleton.FindBoneIndex(*(TEXT("tibia_") + Leg));
			const int32 FootIndex = Skeleton.FindBoneIndex(*(TEXT("foot_") + Leg));
			if (!TestTrue(FString::Printf(TEXT("Leg %s is posed"), *Leg), Hip && Knee && TibiaIndex != INDEX_NONE && FootIndex != INDEX_NONE))
			{
				continue;
			}
			// The tip rides on the tibia as it does at rest.
			const FTransform FootInTibia = FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, FootIndex)
				.GetRelativeTransform(FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, TibiaIndex));
			const FVector Tip = (FootInTibia * *Knee).GetLocation();
			const FVector KneeAt = Knee->GetLocation();
			const double Inward = FMath::Abs(KneeAt.Y - Body->GetLocation().Y);
			TestTrue(FString::Printf(TEXT("Leg %s: its knee lies low (%.0f cm up, its hip %.0f) and drawn in (%.0f cm off the body's line)"), *Leg,
				KneeAt.Z, Hip->GetLocation().Z, Inward),
				KneeAt.Z < 25.0 * Size && KneeAt.Z < Hip->GetLocation().Z - 15.0 * Size && Inward < 80.0 * Size);
			FVector TipOnGround = ToWorld.TransformPosition(Tip);
			TipOnGround.Z = 0.0;
			const double FromLine = FMath::PointDistToSegment(TipOnGround, FootprintFront, FootprintBack);
			TestTrue(FString::Printf(TEXT("Leg %s: its tip is under the body (%.0f cm from its line, within %.0f)"), *Leg, FromLine, FootprintRadius),
				FromLine <= FootprintRadius);
			TestTrue(FString::Printf(TEXT("Leg %s: no part of it above the body (its highest %.0f cm, the thorax's top %.0f)"), *Leg,
				FMath::Max3(Hip->GetLocation().Z, KneeAt.Z, Tip.Z), ThoraxTop),
				FMath::Max3(Hip->GetLocation().Z, KneeAt.Z, Tip.Z) < ThoraxTop);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderBiteRaiseTest, "Looter.Creatures.Anim.SpiderBiteRaise",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderBiteRaiseTest::RunTest(const FString& Parameters)
{
	// A bite's front legs lift off from rest and rise high: they used to leap off the ground at full speed in the first frame.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Spider = SpawnStarted<ASpiderCreature>(World, FVector::ZeroVector);
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(170.0, 0.0, 0.0));
	if (!TestNotNull(TEXT("Spider spawned"), Spider) || !TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	if (Spider->GetBonePose().IsEmpty())
	{
		AddWarning(TEXT("SK_Spider isn't imported: its bite isn't checked."));
		return true;
	}
	for (float Elapsed = 0.f; Elapsed < 0.5f; Elapsed += Frame)
	{
		Spider->Tick(Frame);
	}
	const TArray<FName> Knees = { TEXT("tibia_0_l"), TEXT("tibia_0_r") };
	const TMap<FName, FTransform> Before = PoseOf(*Spider);
	FMotionWatch Watch;
	Watch.Sample(Before, Knees);
	Spider->DevPutInState(ECreatureState::Attack, Player);
	TestTrue(TEXT("It bites"), Spider->GetCreatureState() == ECreatureState::Attack);
	for (float Elapsed = 0.f; Elapsed < Spider->AttackWindup * 0.95f; Elapsed += Frame)
	{
		Spider->Tick(Frame);
		Watch.Sample(PoseOf(*Spider), Knees);
	}
	// The old chase moved a front foot a quarter of the way up (35 cm) in the first frame, its knee about 20.
	TestTrue(FString::Printf(TEXT("Its front legs ease up, they don't leap (a knee's step changes by %.1f cm at most, the %s)"), Watch.BiggestJolt,
		*Watch.JoltBone.ToString()), Watch.BiggestJolt < 10.f);
	const TMap<FName, FTransform> Raised = PoseOf(*Spider);
	if (Before.Contains(Knees[0]) && Raised.Contains(Knees[0]))
	{
		const double Rise = Raised[Knees[0]].GetLocation().Z - Before[Knees[0]].GetLocation().Z;
		TestTrue(FString::Printf(TEXT("...and rise high by the bite (the knee %.0f cm up)"), Rise), Rise > 25.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidBankTest, "Looter.Creatures.Anim.UnpaidBank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidBankTest::RunTest(const FString& Parameters)
{
	// Turned 60 degrees in one frame (squaring up to its prey), it banks into the turn over a few frames: its head (and the
	// hat on it) used to jump with the whole bank at once.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	AUnpaidCreature* Unpaid = SpawnStarted<AUnpaidCreature>(WorldWrapper.GetTestWorld(), FVector::ZeroVector);
	if (!TestNotNull(TEXT("Unpaid spawned"), Unpaid))
	{
		return false;
	}
	const FName Head = Unpaid->Rig.Head;
	if (!Unpaid->GetMesh()->GetSkeletalMeshAsset() || !PoseOf(*Unpaid).Contains(Head))
	{
		AddWarning(TEXT("SK_Unpaid isn't imported: its bank isn't checked."));
		return true;
	}
	for (float Elapsed = 0.f; Elapsed < 1.f; Elapsed += Frame)
	{
		Unpaid->Tick(Frame);
	}
	FMotionWatch Watch;
	Watch.Sample(PoseOf(*Unpaid), { Head });
	Unpaid->SetActorRotation(FRotator(0.f, 60.f, 0.f));
	for (int32 Index = 0; Index < 30; ++Index)
	{
		Unpaid->Tick(Frame);
		Watch.Sample(PoseOf(*Unpaid), { Head });
	}
	TestTrue(FString::Printf(TEXT("Its head rides the bank in, no jump (%.1f cm in a frame at most)"), Watch.BiggestStep), Watch.BiggestStep < 6.f);
	return true;
}

#endif
