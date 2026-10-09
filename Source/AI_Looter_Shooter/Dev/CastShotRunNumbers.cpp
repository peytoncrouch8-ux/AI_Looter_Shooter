// FCastShotRun's numbers (CastShotRun.h): the body watched frame by frame while its state runs, and everything measured
// at the moment time stops, written to numbers.csv and the log, marked "!" past what a player would notice. The measures
// themselves are CastShotProbe's. What a state makes meaningless isn't counted: a dead body's feet sliding (its legs
// curling in), a hopping slime's foot over the ground. Developer builds only.

#include "Dev/CastShotRun.h"
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/UnpaidCreature.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	namespace Scene = CastShotScene;
	namespace Probe = CastShotProbe;

	USkinnedMeshComponent* SkinnedBodyOf(const TWeakObjectPtr<AActor>& Actor)
	{
		return Actor.IsValid() ? Cast<USkinnedMeshComponent>(Scene::BodyOf(*Actor.Get())) : nullptr;
	}
}

float FCastShotRun::MarkFor(const FString& Metric)
{
	// Past these a player notices: a foot a few centimetres into the ground, a hand through a coat, a foot skating.
	static const TMap<FString, float> Marks = {
		{ TEXT("feet_under_cm"), 3.f }, { TEXT("run_under_cm"), 4.f }, { TEXT("trailing_under_cm"), 3.f },
		{ TEXT("gel_under_cm"), 4.f }, { TEXT("gel_over_cm"), 6.f },
		{ TEXT("limbs_in_body_cm"), 2.f }, { TEXT("arms_in_torso_cm"), 1.f }, { TEXT("origin_gap_cm"), 6.f },
		{ TEXT("slide_mean_cm_s"), 12.f }, { TEXT("slide_worst_cm_s"), 60.f }, { TEXT("pop_cm_per_frame"), 25.f },
		{ TEXT("pack_overlap_cm"), 8.f }, { TEXT("in_level_cm"), 3.f } };
	const float* Mark = Marks.Find(Metric);
	return Mark ? *Mark : TNumericLimits<float>::Max();
}

void FCastShotRun::Note(TArray<FString>& Line, const FString& Metric, float Value, const FString& Detail)
{
	const FStep& Step = Steps[StepIndex];
	const bool bPast = Value > MarkFor(Metric);
	Rows.Add(FString::Printf(TEXT("%d,%s,%s,%s,%s,%.1f,%s,%s"), Pictures + 1, *Subjects[Step.Subject].Name, Scene::PlaceName(Step.Place),
		Scene::StateName(Step.State), *Metric, Value, *Detail.Replace(TEXT(","), TEXT(";")), bPast ? TEXT("!") : TEXT("")));
	FString Shown = FString::Printf(TEXT("%s%s %.1f"), bPast ? TEXT("!") : TEXT(""), *Metric, Value);
	if (!Detail.IsEmpty())
	{
		Shown += FString::Printf(TEXT(" (%s)"), *Detail);
	}
	Line.Add(MoveTemp(Shown));
	Marked += bPast ? 1 : 0;
}

void FCastShotRun::SampleBody(UWorld& World, float DeltaSeconds)
{
	USkinnedMeshComponent* Mesh = Actors.IsEmpty() ? nullptr : SkinnedBodyOf(Actors[0]);
	if (!Mesh)
	{
		return;
	}
	AActor* Subject = Actors[0].Get();
	const Scene::FBodyParts Parts = Scene::PartsOf(*Subject);
	Pop.Sample(*Mesh, DeltaSeconds);
	// A dead body's feet stand on nothing: a death curl drawing them in along the ground read as feet skating at 1000 cm/s.
	const ACreatureBase* Creature = Cast<ACreatureBase>(Subject);
	if (Creature && Creature->IsDead())
	{
		Slide.Lift();
	}
	else
	{
		Slide.Sample(World, *Mesh, Parts.Feet, Parts.PlantedHeight, Subject, DeltaSeconds);
	}
	// The deepest any foot or trailing part went under the ground while it ran.
	TArray<FName> Low = Parts.Feet;
	Low.Append(Parts.Trailing);
	if (!Low.IsEmpty())
	{
		const Probe::FGroundGap Gap = Probe::BonesAgainstGround(World, *Mesh, Low, Subject);
		if (Gap.Under > WorstUnder)
		{
			WorstUnder = Gap.Under;
			WorstUnderPart = Gap.UnderPart;
		}
	}
}

void FCastShotRun::SamplePack()
{
	for (int32 First = 0; First < Actors.Num(); ++First)
	{
		const USkinnedMeshComponent* A = SkinnedBodyOf(Actors[First]);
		for (int32 Second = First + 1; A && Second < Actors.Num(); ++Second)
		{
			const USkinnedMeshComponent* B = SkinnedBodyOf(Actors[Second]);
			if (!B)
			{
				continue;
			}
			// Both ways round: one's leg in the other's abdomen, or the other's in its own.
			for (const bool bSwap : { false, true })
			{
				const Probe::FIntrusion Into = bSwap ? Probe::BodyIntoBody(*B, *A) : Probe::BodyIntoBody(*A, *B);
				if (Into.Depth > WorstPack)
				{
					WorstPack = Into.Depth;
					WorstPackDetail = FString::Printf(TEXT("%d's %s in %d's %s"), (bSwap ? Second : First) + 1, *Into.Part,
						(bSwap ? First : Second) + 1, *Into.Into);
				}
			}
		}
	}
}

void FCastShotRun::MeasureGel(UWorld& World, const USkinnedMeshComponent& Mesh, TArray<FString>& Line)
{
	const int32 Index = Mesh.GetBoneIndex(TEXT("body"));
	const USkinnedAsset* Model = Mesh.GetSkinnedAsset();
	if (Index == INDEX_NONE || !Model)
	{
		return;
	}
	// The foot's edge at rest is a ring on the ground round the body bone (Slime.py); posed, it goes where the bone takes it.
	const FTransform Rest = FAnimationRuntime::GetComponentSpaceTransformRefPose(Model->GetRefSkeleton(), Index);
	const FTransform Posed = Mesh.GetBoneTransform(Index);
	TArray<FVector> Ring;
	TArray<FString> Names;
	constexpr int32 RingPoints = 12;
	for (int32 Point = 0; Point < RingPoints; ++Point)
	{
		const float RingAngle = 2.f * UE_PI * Point / RingPoints;
		const float Edge = ASlimeCreature::FootRadius * 0.92f;
		const FVector AtRest(Edge * FMath::Cos(RingAngle), Edge * FMath::Sin(RingAngle), 1.0);
		Ring.Add(Posed.TransformPosition(Rest.InverseTransformPosition(AtRest)));
		Names.Add(FString::Printf(TEXT("rim %d deg"), Point * 360 / RingPoints));
	}
	const Probe::FGroundGap Gap = Probe::PointsAgainstGround(World, Ring, Names, Mesh.GetOwner());
	Note(Line, TEXT("gel_under_cm"), Gap.Under, Gap.UnderPart);
	// Mid-hop the whole slime is off the ground, as it should be: its foot over the ground means nothing then.
	const ACharacter* Hopper = Cast<ACharacter>(Mesh.GetOwner());
	if (Hopper && Hopper->GetCharacterMovement() && Hopper->GetCharacterMovement()->IsFalling())
	{
		Note(Line, TEXT("gel_over_cm"), 0.f, TEXT("in the air (a hop): not measured"));
		return;
	}
	Note(Line, TEXT("gel_over_cm"), Gap.Over, Gap.OverPart);
}

void FCastShotRun::MeasureNow(UWorld& World)
{
	TArray<FString> Line;
	AActor* Subject = Actors.IsEmpty() ? nullptr : Actors[0].Get();
	UPrimitiveComponent* Body = Subject ? Scene::BodyOf(*Subject) : nullptr;
	USkinnedMeshComponent* Mesh = Cast<USkinnedMeshComponent>(Body);
	const Scene::FBodyParts Parts = Subject ? Scene::PartsOf(*Subject) : Scene::FBodyParts();
	const ACreatureBase* Creature = Cast<ACreatureBase>(Subject);
	const FStep& Step = Steps[StepIndex];

	if (Mesh && !Parts.Feet.IsEmpty())
	{
		const Probe::FGroundGap Feet = Probe::BonesAgainstGround(World, *Mesh, Parts.Feet, Subject);
		Note(Line, TEXT("feet_under_cm"), Feet.Under, Feet.UnderPart);
		// Over the ground only means something standing: walking, a foot is in the air mid-step.
		Note(Line, TEXT("feet_over_cm"), Feet.Over, Feet.OverPart);
	}
	if (Mesh && !Parts.Trailing.IsEmpty())
	{
		const Probe::FGroundGap Trailing = Probe::BonesAgainstGround(World, *Mesh, Parts.Trailing, Subject);
		Note(Line, TEXT("trailing_under_cm"), Trailing.Under, Trailing.UnderPart);
	}
	if (Mesh && Parts.bSlimeFoot)
	{
		MeasureGel(World, *Mesh, Line);
	}
	if (Mesh && !Parts.Limbs.IsEmpty())
	{
		const Probe::FIntrusion Limbs = Probe::LimbsIntoBody(*Mesh, Parts.Limbs, Parts.Body);
		Note(Line, TEXT("limbs_in_body_cm"), Limbs.Depth, Limbs.Part.IsEmpty() ? FString() : Limbs.Part + TEXT(" in ") + Limbs.Into);
	}
	if (const AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(Subject))
	{
		Note(Line, TEXT("arms_in_torso_cm"), Unpaid->GetArmIntrusion(), FString());
	}
	if (Creature && !Creature->IsDead() && Creature->GetCharacterMovement() && Creature->GetCharacterMovement()->IsFalling())
	{
		// A slime mid-hop (or anything mid-leap): its capsule is off the ground by design.
		Note(Line, TEXT("origin_gap_cm"), 0.f, TEXT("in the air (a hop or a leap): not measured"));
	}
	else if (Creature && !Creature->IsDead())
	{
		// Its capsule's foot (the model's ground) against the ground under its middle: a body sunk in or hovering.
		FVector Ground;
		const FVector Middle = Creature->GetActorLocation();
		const float HalfHeight = Creature->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		if (Probe::GroundUnder(World, Middle, 0.f, HalfHeight + 300.f, Creature, Ground))
		{
			Note(Line, TEXT("origin_gap_cm"), FMath::Abs(static_cast<float>(Middle.Z - HalfHeight - Ground.Z)), TEXT("capsule foot to the ground"));
		}
	}
	if (Subjects[Step.Subject].Found.IsValid() && Body)
	{
		const Probe::FLevelHit Level = Probe::AgainstLevel(World, *Body, Subject);
		FString Detail = FString::Printf(TEXT("%d of %d points inside"), Level.Inside, Level.Measured);
		if (!Level.Part.IsEmpty())
		{
			Detail += FString::Printf(TEXT("; deepest %s in %s"), *Level.Part, *Level.Into);
		}
		Note(Line, TEXT("in_level_cm"), Level.Depth, Detail);
	}
	if (Slide.GetPlantedSeconds() > 0.f)
	{
		const double Speed = Subject ? Subject->GetVelocity().Size2D() : 0.0;
		Note(Line, TEXT("slide_mean_cm_s"), Slide.MeanSlide(), FString::Printf(TEXT("body at %.0f cm/s"), Speed));
		Note(Line, TEXT("slide_worst_cm_s"), Slide.WorstSlide(), Slide.WorstFoot());
	}
	if (WorstUnder > 0.f)
	{
		Note(Line, TEXT("run_under_cm"), WorstUnder, WorstUnderPart);
	}
	if (Mesh)
	{
		Note(Line, TEXT("pop_cm_per_frame"), Pop.Worst, Pop.WorstBone);
	}
	if (Actors.Num() > 1)
	{
		SamplePack();
		Note(Line, TEXT("pack_overlap_cm"), WorstPack, WorstPackDetail);
	}
	UE_LOG(LogLooter, Display, TEXT("Looter.CastShots: %03d %s-%s %s: %s"), Pictures + 1, *Subjects[Step.Subject].Name,
		Scene::PlaceName(Step.Place), Scene::StateName(Step.State), Line.IsEmpty() ? TEXT("nothing to measure") : *FString::Join(Line, TEXT(", ")));
}

#endif
