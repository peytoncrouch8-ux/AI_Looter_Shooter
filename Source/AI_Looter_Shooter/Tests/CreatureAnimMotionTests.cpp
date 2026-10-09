#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The creatures' motion through the moments Looter.CastShots caught going wrong (2026-10-09): a spider's death curl
// (its legs snapped to a new bend and stood up in a cage through its abdomen), its bite's raised front legs (they leapt off
// the ground), and an Unpaid banking into a turn (its head and hat jumped 30-50 cm in a frame when a turn began).

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

	/** How deep the spider's femurs' hit hulls reach into its abdomen's past where they reach at rest, in its code's pose (cm). */
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
		Femurs.AddFromPhysicsAsset(Hulls, &Skeleton, FemurNames);
		Abdomen.AddFromPhysicsAsset(Hulls, &Skeleton, { FName(TEXT("abdomen")) });
		const TMap<FName, FTransform> Pose = PoseOf(Spider);
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
					const float Now = FCreatureBodyHulls::Depth(Body, BodyPosed->InverseTransformPosition(FemurPosed->TransformPosition(Point)));
					const float AtRest = FCreatureBodyHulls::Depth(Body, BodyRest.InverseTransformPosition(FemurRest.TransformPosition(Point)));
					const float Past = Now - FMath::Max(AtRest, 0.f);
					if (Past > Deepest)
					{
						Deepest = Past;
						OutWhere = Femur.Bone.ToString();
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
	// Dead, the legs fold in from where they stood without a snap, end with their knees up and out to the sides, and keep
	// their femurs out of the abdomen (the old curl stood them straight up through it, 25-48 cm).
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
	FMotionWatch Watch;
	FMotionWatch OnBody;
	Watch.Sample(PoseOf(*Spider), Knees);
	OnBody.Sample(KneesOnBody(), Knees);
	BossTestWorld::Kill(Spider);
	TestTrue(TEXT("It dies"), Spider->IsDead());
	for (float Elapsed = 0.f; Elapsed < ASpiderCreature::CurlSeconds + 0.6f; Elapsed += Frame)
	{
		Spider->Tick(Frame);
		Watch.Sample(PoseOf(*Spider), Knees);
		OnBody.Sample(KneesOnBody(), Knees);
	}
	// The curl moves a knee about a metre in half a second, eased, as the body sinks onto it: a few cm a frame, never a
	// jump, and against the body no snap to a new bend (the old curl's knees swung 15-20 cm over in its first frame).
	TestTrue(FString::Printf(TEXT("The legs fold without a jump (%.1f cm in a frame at most)"), Watch.BiggestStep), Watch.BiggestStep < 12.f);
	TestTrue(FString::Printf(TEXT("...or a snap (against the body a knee's step changes by %.1f cm at most, the %s)"), OnBody.BiggestJolt,
		*OnBody.JoltBone.ToString()), OnBody.BiggestJolt < 4.f);

	FString Where;
	const float Deepest = FemursIntoAbdomen(*Spider, Where);
	TestTrue(FString::Printf(TEXT("Curled up, its femurs keep out of its abdomen (%.1f cm past rest at worst: %s)"), Deepest, *Where), Deepest <= 3.f);

	// The knees (each tibia's root) lie out past the hips (each femur's root) and above them.
	const TMap<FName, FTransform> Pose = PoseOf(*Spider);
	for (const TCHAR* Leg : { TEXT("0_l"), TEXT("1_r"), TEXT("2_l"), TEXT("3_r") })
	{
		const FTransform* Hip = Pose.Find(*FString::Printf(TEXT("femur_%s"), Leg));
		const FTransform* Knee = Pose.Find(*FString::Printf(TEXT("tibia_%s"), Leg));
		if (!Hip || !Knee)
		{
			continue;
		}
		const double Out = Knee->GetLocation().Size2D() - Hip->GetLocation().Size2D();
		const double Up = Knee->GetLocation().Z - Hip->GetLocation().Z;
		TestTrue(FString::Printf(TEXT("Leg %s: the knee is out to the side (%.0f cm past the hip) and up (%.0f cm)"), Leg, Out, Up), Out > 30.0 && Up > 20.0);
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
