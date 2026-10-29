#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// The spiders' hit feel (HITS, 2026-10-28; SpiderCreatureHurt.cpp, SpiderCreatureLegs.cpp, SpiderCreatureDeath.cpp), measured
// on the pose its code works out: a shot leg buckles and no other, a stagger crouches the body, a head crit's kill drops it at
// once ("lights out") where a body kill throws its throe first, and a badly hurt spider limps on its most-shot leg. Shots
// arrive as the game's do (point damage with the bone, then the damage): a test level has no game mode, so they're handed
// to the listeners as AActor::TakeDamage hands them. The world's clock is moved on with each frame: the stagger and the
// burst rules read it.

#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/SpiderCreature.h"
#include "Tests/BossTestWorld.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

namespace
{
	constexpr float Frame = 1.f / 60.f;

	/** A spider in a test world, started as play starts it, dropping no loot. */
	ASpiderCreature* SpawnSpider(UWorld* World, const FVector& Where)
	{
		ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(Where, FRotator::ZeroRotator);
		if (Spider && !Spider->HasActorBegunPlay())
		{
			Spider->DispatchBeginPlay();
		}
		BossTestWorld::NoLoot(Spider);
		return Spider;
	}

	/** The pose the spider's code worked out (component space), by bone. */
	TMap<FName, FTransform> PoseOf(const ASpiderCreature& Spider)
	{
		TMap<FName, FTransform> Pose;
		for (const FCreatureBonePose& Bone : Spider.GetBonePose())
		{
			Pose.Add(Bone.Bone, Bone.Transform);
		}
		return Pose;
	}

	/** Frames on for every spider, the world's clock with them. */
	void Run(UWorld* World, TConstArrayView<ASpiderCreature*> Spiders, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds - 0.5f * Frame; Elapsed += Frame)
		{
			World->TimeSeconds += Frame;
			for (ASpiderCreature* Spider : Spiders)
			{
				Spider->Tick(Frame);
			}
		}
	}

	/** A shot into Bone for a share of its health (a crit's damage type with bCritical), from AtWorld (its posed bone's place if zero). */
	void Shoot(ASpiderCreature& Spider, float Share, FName Bone, bool bCritical, FVector AtWorld = FVector::ZeroVector)
	{
		const UHealthComponent* Health = Spider.FindComponentByClass<UHealthComponent>();
		const float Damage = Share * (Health ? Health->GetMaxHealth() : 230.f);
		if (AtWorld.IsZero())
		{
			const FTransform* Posed = PoseOf(Spider).Find(Bone);
			AtWorld = Posed ? Spider.GetMesh()->GetComponentTransform().TransformPosition(Posed->GetLocation()) : Spider.GetActorLocation();
		}
		const UDamageType* Type = bCritical ? GetDefault<UWeaponCritDamageType>() : GetDefault<UDamageType>();
		const FVector Way = (Spider.GetActorLocation() - AtWorld).GetSafeNormal();
		FEditorScriptExecutionGuard RunActorEvents;
		Spider.OnTakePointDamage.Broadcast(&Spider, Damage, nullptr, AtWorld, Spider.GetMesh(), Bone, Way, Type, nullptr);
		Spider.OnTakeAnyDamage.Broadcast(&Spider, Damage, Type, nullptr, nullptr);
	}

	/** A leg's knee against its hip (cm, up): how far its femur's far end stands over the hip it hangs from. */
	double KneeOverHip(const TMap<FName, FTransform>& Pose, const FString& Leg)
	{
		const FTransform* Hip = Pose.Find(*(TEXT("femur_") + Leg));
		const FTransform* Knee = Pose.Find(*(TEXT("tibia_") + Leg));
		return Hip && Knee ? Knee->GetLocation().Z - Hip->GetLocation().Z : 0.0;
	}

	/** The thorax's height over the ground (the mesh's origin is the ground under it). */
	double BodyHeight(const ASpiderCreature& Spider)
	{
		const TMap<FName, FTransform> Pose = PoseOf(Spider);
		const FTransform* Body = Pose.Find(TEXT("body"));
		return Body ? Body->GetLocation().Z : 0.0;
	}

	bool HasRig(FAutomationTestBase& Test, const ASpiderCreature* Spider)
	{
		if (!Test.TestNotNull(TEXT("Spider spawned"), Spider))
		{
			return false;
		}
		if (Spider->GetBonePose().IsEmpty())
		{
			Test.AddWarning(TEXT("SK_Spider isn't imported: its hit feel isn't checked."));
			return false;
		}
		return true;
	}

	/** The legs a test watches besides the shot one: the front pair taps and grooms on its own dice, so it's left out. */
	const TCHAR* const WatchedLegs[] = { TEXT("1_l"), TEXT("1_r"), TEXT("2_l"), TEXT("2_r"), TEXT("3_l"), TEXT("3_r") };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderLegBuckleTest, "Looter.Creatures.Hits.Spider.LegBuckle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderLegBuckleTest::RunTest(const FString& Parameters)
{
	// A shot leg buckles (its knee drops 12 cm a flinch, in 0.04 s and back over 0.14) and no other leg does. Two spiders take
	// the same shot from the same place, one into its left middle femur, one into its body: the body's jolt, tilt and twist
	// are the same for both, so what differs is the leg's buckle and the roll onto it.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Shot = SpawnSpider(World, FVector::ZeroVector);
	ASpiderCreature* Control = SpawnSpider(World, FVector(3000.0, 0.0, 0.0));
	if (!HasRig(*this, Shot) || !HasRig(*this, Control))
	{
		return true;
	}
	TArray<ASpiderCreature*> Both = { Shot, Control };
	Run(World, Both, 0.3f);
	const FString HitLeg = TEXT("1_l");
	const FName Femur = *(TEXT("femur_") + HitLeg);
	const FTransform* Posed = PoseOf(*Shot).Find(Femur);
	if (!TestNotNull(TEXT("Its left middle femur is posed"), Posed))
	{
		return false;
	}
	// A rifle round's worth (6.5% of its health: a flinch of about 1, no stagger), into the femur's middle.
	const FVector Local = Posed->GetLocation() + (PoseOf(*Shot)[*(TEXT("tibia_") + HitLeg)].GetLocation() - Posed->GetLocation()) * 0.5;
	auto Before = [](const ASpiderCreature& Spider)
	{
		TMap<FString, double> Knees;
		const TMap<FName, FTransform> Pose = PoseOf(Spider);
		for (const TCHAR* Leg : WatchedLegs)
		{
			Knees.Add(Leg, KneeOverHip(Pose, Leg));
		}
		return Knees;
	};
	const TMap<FString, double> ShotRest = Before(*Shot);
	const TMap<FString, double> ControlRest = Before(*Control);
	Shoot(*Shot, 0.065f, Femur, false, Shot->GetMesh()->GetComponentTransform().TransformPosition(Local));
	Shoot(*Control, 0.065f, TEXT("body"), false, Control->GetMesh()->GetComponentTransform().TransformPosition(Local));

	// How much further each knee has dropped against its hip on the shot spider than on the other.
	auto Drops = [&]()
	{
		TMap<FString, double> Drop;
		const TMap<FName, FTransform> ShotPose = PoseOf(*Shot);
		const TMap<FName, FTransform> ControlPose = PoseOf(*Control);
		for (const TCHAR* Leg : WatchedLegs)
		{
			Drop.Add(Leg, (ShotRest[Leg] - KneeOverHip(ShotPose, Leg)) - (ControlRest[Leg] - KneeOverHip(ControlPose, Leg)));
		}
		return Drop;
	};
	double Deepest = 0.0;
	double OthersDeepest = 0.0;
	FString OtherLeg;
	for (float Elapsed = 0.f; Elapsed < 0.1f - 0.5f * Frame; Elapsed += Frame)
	{
		Run(World, Both, Frame);
		for (const TPair<FString, double>& Leg : Drops())
		{
			if (Leg.Key == HitLeg)
			{
				Deepest = FMath::Max(Deepest, Leg.Value);
			}
			else if (Leg.Value > OthersDeepest)
			{
				OthersDeepest = Leg.Value;
				OtherLeg = Leg.Key;
			}
		}
	}
	TestTrue(FString::Printf(TEXT("The shot leg's knee drops within 0.1 s (%.1f cm)"), Deepest), Deepest >= 8.0);
	TestTrue(FString::Printf(TEXT("...and no other leg's (%.1f cm at most, %s)"), OthersDeepest, *OtherLeg), OthersDeepest < 3.0);
	Run(World, Both, 0.4f);
	const double Left = Drops()[HitLeg];
	TestTrue(FString::Printf(TEXT("By 0.5 s it's back (%.1f cm off)"), Left), FMath::Abs(Left) < 1.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderStaggerPoseTest, "Looter.Creatures.Hits.Spider.StaggerPose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderStaggerPoseTest::RunTest(const FString& Parameters)
{
	// A stagger (a crit staggers a Basic one 0.4 s) crouches it: body 10 cm down at the stagger's hold, nose up, feet splayed.
	// The crit lands from its side at its body's height, so its jolt pushes it sideways, not down.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Spider = SpawnSpider(World, FVector::ZeroVector);
	if (!HasRig(*this, Spider))
	{
		return true;
	}
	TArray<ASpiderCreature*> One = { Spider };
	Run(World, One, 0.3f);
	const double Standing = BodyHeight(*Spider);
	const FTransform* Body = PoseOf(*Spider).Find(TEXT("body"));
	if (!TestNotNull(TEXT("Its body is posed"), Body))
	{
		return false;
	}
	const FVector Side = Spider->GetMesh()->GetComponentTransform().TransformPosition(Body->GetLocation() + FVector(0.0, 40.0, 0.0));
	Shoot(*Spider, 0.02f, TEXT("body"), true, Side);
	double Lowest = 0.0;
	for (float Elapsed = 0.f; Elapsed < 0.3f - 0.5f * Frame; Elapsed += Frame)
	{
		Run(World, One, Frame);
		Lowest = FMath::Max(Lowest, Standing - BodyHeight(*Spider));
	}
	TestTrue(FString::Printf(TEXT("Staggered, its body crouches (%.1f cm lower)"), Lowest), Lowest >= 7.0);
	Run(World, One, 0.6f);
	TestTrue(FString::Printf(TEXT("...and stands again after (%.1f cm off)"), Standing - BodyHeight(*Spider)),
		FMath::Abs(Standing - BodyHeight(*Spider)) < 2.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderLightsOutTest, "Looter.Creatures.Hits.Spider.LightsOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderLightsOutTest::RunTest(const FString& Parameters)
{
	// A head crit's kill puts its lights out: no throe, its body down on the ground inside 0.3 s, where a body kill rears first
	// and lands on its cry's knock (0.42 s). Both end with every leg curled under the body (the user's 2026-10-14 rule).
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Head = SpawnSpider(World, FVector::ZeroVector);
	ASpiderCreature* Body = SpawnSpider(World, FVector(3000.0, 0.0, 0.0));
	if (!HasRig(*this, Head) || !HasRig(*this, Body))
	{
		return true;
	}
	TArray<ASpiderCreature*> Both = { Head, Body };
	// Worn down to 4% first, then killed by a small hit (4.5%): a kill, not a Big one or an overkill.
	for (ASpiderCreature* Spider : Both)
	{
		const UHealthComponent* Health = Spider->FindComponentByClass<UHealthComponent>();
		BossTestWorld::Hurt(Spider, Health ? Health->GetMaxHealth() * 0.96f : 220.f);
	}
	Run(World, Both, 1.5f);
	Shoot(*Head, 0.045f, TEXT("head"), true);
	Shoot(*Body, 0.045f, TEXT("body"), false);
	if (!TestTrue(TEXT("Both die"), Head->IsDead() && Body->IsDead()))
	{
		return false;
	}
	TestTrue(TEXT("The head crit's kill is lights out"), Head->GetKillStyle() == ASpiderCreature::EKillStyle::LightsOut);
	TestTrue(TEXT("The body shot's is today's death"), Body->GetKillStyle() == ASpiderCreature::EKillStyle::Body);
	const double OnGround = ASpiderCreature::DeadRide * Head->GetSizeScale() + 2.0;
	float HeadDown = -1.f;
	float BodyDown = -1.f;
	for (float Dead = Frame; Dead < 1.4f; Dead += Frame)
	{
		Run(World, Both, Frame);
		if (HeadDown < 0.f && BodyHeight(*Head) <= OnGround)
		{
			HeadDown = Dead;
		}
		if (BodyDown < 0.f && BodyHeight(*Body) <= OnGround)
		{
			BodyDown = Dead;
		}
	}
	TestTrue(FString::Printf(TEXT("Lights out: its body is on the ground at once (%.2f s)"), HeadDown), HeadDown >= 0.f && HeadDown < 0.3f);
	TestTrue(FString::Printf(TEXT("A body kill's throe first (%.2f s)"), BodyDown), BodyDown >= 0.4f);
	for (const ASpiderCreature* Spider : Both)
	{
		const TMap<FName, FTransform> Pose = PoseOf(*Spider);
		for (int32 Pair = 0; Pair < 4; ++Pair)
		{
			for (const TCHAR* Side : { TEXT("l"), TEXT("r") })
			{
				const FTransform* Hip = Pose.Find(*FString::Printf(TEXT("femur_%d_%s"), Pair, Side));
				const FTransform* Knee = Pose.Find(*FString::Printf(TEXT("tibia_%d_%s"), Pair, Side));
				if (Hip && Knee)
				{
					TestTrue(FString::Printf(TEXT("%s, leg %d_%s: curled, its knee low (%.0f cm up, its hip %.0f)"), Spider == Head ? TEXT("Lights out")
						: TEXT("Body kill"), Pair, Side, Knee->GetLocation().Z, Hip->GetLocation().Z),
						Knee->GetLocation().Z < 25.0 * Spider->GetSizeScale() && Knee->GetLocation().Z < Hip->GetLocation().Z - 15.0 * Spider->GetSizeScale());
				}
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpiderLimpTest, "Looter.Creatures.Hits.Spider.Limp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSpiderLimpTest::RunTest(const FString& Parameters)
{
	// Under half its health it limps (looks only): on its most-shot leg, else its front left; that leg steps half as high.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	ASpiderCreature* Spider = SpawnSpider(World, FVector::ZeroVector);
	ASpiderCreature* Unshot = SpawnSpider(World, FVector(3000.0, 0.0, 0.0));
	if (!HasRig(*this, Spider) || !HasRig(*this, Unshot))
	{
		return true;
	}
	TArray<ASpiderCreature*> One = { Spider };
	Run(World, One, 0.3f);
	TestEqual(TEXT("Unhurt, it doesn't limp"), Spider->GetLimpLeg(), static_cast<int32>(INDEX_NONE));
	// Four shots into its right third leg (Legs' order: front to back, left then right: index 5), 52% of its health.
	for (int32 Shots = 0; Shots < 4; ++Shots)
	{
		Shoot(*Spider, 0.13f, TEXT("tibia_2_r"), false);
		Run(World, One, 0.3f);
	}
	Run(World, One, 0.6f);
	TestEqual(TEXT("Under half its health, wound stage 1"), Spider->GetWoundStage(), 1);
	TestEqual(TEXT("It limps on the leg it was shot in"), Spider->GetLimpLeg(), 5);
	const UHealthComponent* Health = Unshot->FindComponentByClass<UHealthComponent>();
	BossTestWorld::Hurt(Unshot, Health ? Health->GetMaxHealth() * 0.6f : 140.f);
	TArray<ASpiderCreature*> Other = { Unshot };
	Run(World, Other, 0.1f);
	TestEqual(TEXT("Hurt with no leg shot, it limps on its front left"), Unshot->GetLimpLeg(), 0);

	// Walked along (moved on under its feet, so its feet step after it), the limping leg lifts lower than its twin.
	const double Ground = Spider->GetActorLocation().Z - 62.0 * Spider->GetSizeScale();
	double LimpLift = 0.0;
	double TwinLift = 0.0;
	for (float Walked = 0.f; Walked < 2.f; Walked += Frame)
	{
		Spider->SetActorLocation(Spider->GetActorLocation() + FVector(300.0 * Frame, 0.0, 0.0));
		Run(World, One, Frame);
		LimpLift = FMath::Max(LimpLift, Spider->GetPosedFoot(2, 1.f).Z - Ground);
		TwinLift = FMath::Max(TwinLift, Spider->GetPosedFoot(2, -1.f).Z - Ground);
	}
	TestTrue(FString::Printf(TEXT("Its twin steps up (%.1f cm)"), TwinLift), TwinLift > 8.0);
	TestTrue(FString::Printf(TEXT("...and the limping leg lower (%.1f cm)"), LimpLift), LimpLift < TwinLift * 0.75);
	return true;
}

#endif
