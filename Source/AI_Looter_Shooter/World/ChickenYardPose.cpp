// AChickenYard's figures: a poseable mesh per bird, posed from the pose table (World/AnimalPoseTables.h) only while on
// screen: what it does blended from the table's poses, eased from what it did, the alert, the head's look, the walk's
// head-bob and the springs of the wattles and the tail's tip on top.

#include "World/ChickenYard.h"
#include "World/AnimalPoseTables.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"

namespace
{
	/** What a bird eases from when it changes what it does (s). */
	constexpr float ChangeSeconds = 0.18f;

	/** A peck's part spent going down (the rest coming up). */
	constexpr float PeckDownShare = 0.45f;
	/** The crow's rise into it and fall out of it (s). */
	constexpr float CrowRise = 0.35f;
	constexpr float CrowFall = 0.4f;
	/** Sitting down and getting up (s). */
	constexpr float SitDownSeconds = 0.8f;

	/** The run's wingbeats (a second). */
	constexpr float FlutterBeats = 8.f;

	/** The head-bob: the share of each step the head is held still, and how far it may lag (cm, a hen's). */
	constexpr float BobHold = 0.65f;
	constexpr float BobMost = 4.5f;

	/** How much of the strut is the alert's chest-out (the rooster). */
	constexpr float StrutChest = 0.4f;

	/** The springs: wattles and tail tip (Hz, share of critical damping, push per cm/s/s, widest swing in degrees). */
	constexpr float WattleHz = 3.f;
	constexpr float WattleDamping = 0.25f;
	constexpr float WattlePush = 1.2f;
	constexpr float TailHz = 2.f;
	constexpr float TailDamping = 0.3f;
	constexpr float TailPush = 0.6f;
	constexpr float SpringLimit = 30.f;
	/** The hardest acceleration a spring takes (cm/s/s): a teleport or a hitch isn't a jolt. */
	constexpr float MostAcceleration = 8000.f;
}

void AChickenYard::MakeFigures()
{
	Figures.Reset();
	Rigs.Reset();
	Rigs.SetNum(Birds.Num());
	for (int32 Index = 0; Index < Birds.Num(); ++Index)
	{
		const FChicken& Bird = Birds[Index];
		USkeletalMesh* Model = Bird.bRooster ? RoosterMesh.Get() : (Bird.bWhite ? HenWhiteMesh.Get() : HenBrownMesh.Get());
		if (!Model)
		{
			Figures.Add(nullptr);
			continue;
		}
		UPoseableMeshComponent* Figure = NewObject<UPoseableMeshComponent>(this, *FString::Printf(TEXT("Chicken%d"), Index), RF_Transient);
		// The asset first: the pose is set up as the component registers.
		Figure->SetSkinnedAssetAndUpdate(Model, false);
		Figure->SetMobility(EComponentMobility::Movable);
		Figure->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Figure->SetGenerateOverlapEvents(false);
		Figure->SetCanEverAffectNavigation(false);
		Figure->SetCastShadow(bCastShadows);
		Figure->bReceivesDecals = false;
		// A peck reaches past the standing bounds, a run's wings out wide.
		Figure->SetBoundsScale(1.5f);
		Figure->SetCachedMaxDrawDistance(CullDistance);
		Figure->SetupAttachment(Root);
		Figure->RegisterComponent();
		// Its pose comes from here: the component's own tick only looks after its level of detail.
		Figure->SetComponentTickInterval(0.25f);
		AddInstanceComponent(Figure);
		Figure->SetWorldLocationAndRotation(FVector(Bird.Position, Bird.Height), FRotator(0.f, Bird.Yaw, 0.f));
		Figure->SetWorldScale3D(FVector(Bird.Scale));
		Rigs[Index].Bind(Model, Bird.bRooster ? AnimalPoseTables::Rooster() : AnimalPoseTables::Hen());
		Figures.Add(Figure);
	}
}

void AChickenYard::PoseBirds(float DeltaSeconds)
{
	for (int32 Index = 0; Index < Birds.Num(); ++Index)
	{
		FChicken& Bird = Birds[Index];
		FAnimalPose Built;
		BuildPose(Bird, Built);
		// Changing what it does, it eases from where it was.
		if (Bird.PosedDoing != Bird.Doing)
		{
			Bird.PosedDoing = Bird.Doing;
			Bird.From = Bird.Pose;
			Bird.FromBlend = Bird.From.IsEmpty() ? 1.f : 0.f;
		}
		if (Bird.FromBlend < 1.f)
		{
			Bird.FromBlend = FMath::Min(1.f, Bird.FromBlend + DeltaSeconds / ChangeSeconds);
			AnimalPose::Blend(Bird.From, Built, AnimalPose::Ease(Bird.FromBlend), Built);
		}
		Bird.Pose = Built;

		// The springs on top (from the last update's swing), then onto the figure.
		const AnimalPoseTables::FChickenBones& Bones = AnimalPoseTables::ChickenBones();
		AnimalPose::Bend(Built, Bones.Wattles, static_cast<float>(Bird.Wattles.Angle.X), 0.f, static_cast<float>(Bird.Wattles.Angle.Y));
		AnimalPose::Bend(Built, Bones.TailTip, static_cast<float>(Bird.TailTip.Angle.X), static_cast<float>(Bird.TailTip.Angle.Y), 0.f);
		UPoseableMeshComponent* Figure = GetFigure(Index);
		if (!Figure || !Rigs.IsValidIndex(Index) || !Rigs[Index].IsBound())
		{
			continue;
		}
		Figure->SetWorldLocationAndRotation(FVector(Bird.Position, Bird.Height), FRotator(0.f, Bird.Yaw, 0.f));
		Rigs[Index].Apply(*Figure, Built);
		UpdateSprings(Index, DeltaSeconds);
	}
}

void AChickenYard::BuildPose(const FChicken& Bird, FAnimalPose& Out) const
{
	const bool bRooster = Bird.bRooster;
	auto Table = [bRooster](EChickenPose Pose, bool bMirrored = false) -> const FAnimalPose&
	{
		return AnimalPoseTables::Chicken(bRooster, Pose, bMirrored);
	};
	const AnimalPoseTables::FChickenBones& Bones = AnimalPoseTables::ChickenBones();
	const FAnimalPose& Rest = Table(EChickenPose::Rest);
	float Step = 0.f;
	float AlertShare = 1.f;
	switch (Bird.Doing)
	{
	case EChickenDoing::Peck:
	{
		const float T = FMath::Fmod(Bird.Clock, ChickenTimes::Peck) / ChickenTimes::Peck;
		const float Down = T < PeckDownShare ? AnimalPose::Ease(T / PeckDownShare) : 1.f - AnimalPose::Ease((T - PeckDownShare) / (1.f - PeckDownShare));
		AnimalPose::Blend(Rest, Table(EChickenPose::Peck), Bird.Clock < Bird.Count * ChickenTimes::Peck ? Down : 0.f, Out);
		AlertShare = 0.3f;
		break;
	}
	case EChickenDoing::Scratch:
	{
		// Two rakes of the foot, a step back on it, then pecks at the spot.
		const float Back = 2.f * ChickenTimes::Rake;
		if (Bird.Clock < Back)
		{
			const float T = FMath::Fmod(Bird.Clock, ChickenTimes::Rake) / ChickenTimes::Rake;
			AnimalPose::Blend(Rest, Table(EChickenPose::Scratch, Bird.bMirror), FMath::Sin(UE_PI * T), Out);
		}
		else if (Bird.Clock < Back + ChickenTimes::StepBack)
		{
			const float T = (Bird.Clock - Back) / ChickenTimes::StepBack;
			AnimalPose::Blend(Rest, Table(EChickenPose::Walk, !Bird.bMirror), FMath::Sin(UE_PI * T) * 0.6f, Out);
		}
		else
		{
			const float T = FMath::Fmod(Bird.Clock - Back - ChickenTimes::StepBack, ChickenTimes::Peck) / ChickenTimes::Peck;
			const float Down = T < PeckDownShare ? AnimalPose::Ease(T / PeckDownShare) : 1.f - AnimalPose::Ease((T - PeckDownShare) / (1.f - PeckDownShare));
			AnimalPose::Blend(Rest, Table(EChickenPose::Peck), Down, Out);
		}
		AlertShare = 0.3f;
		break;
	}
	case EChickenDoing::Walk:
	{
		Step = AnimalPose::StrideBlend(Table(EChickenPose::Walk), Table(EChickenPose::Walk, true), Bird.Stride, Out);
		if (bRooster)
		{
			// The strut: his chest out over the walking legs.
			AnimalPose::BlendBones(Out, Table(EChickenPose::Alert), StrutChest, Bones.Upper);
		}
		// The head-bob: held still in the world for most of each step while the body walks under it, then snapped
		// forward. Its lag goes from ahead to behind and back, carried by the head and the upper neck.
		const float StepSeconds = 0.5f * (bRooster ? ChickenTimes::RoosterStride : ChickenTimes::HenStride);
		const float Most = FMath::Min(BobMost * (bRooster ? 1.2f : 1.f), Bird.Speed * StepSeconds * BobHold * 0.5f) * Bird.Moving;
		const float Lag = Step < BobHold ? Most * (1.f - 2.f * Step / BobHold)
			: Most * (-1.f + 2.f * AnimalPose::Ease((Step - BobHold) / (1.f - BobHold)));
		AnimalPose::Nudge(Out, Bones.Neck2, FVector(Lag * 0.4f, 0.f, 0.f));
		AnimalPose::Nudge(Out, Bones.Head, FVector(Lag * 0.6f, 0.f, 0.f));
		AlertShare = 0.f;
		break;
	}
	case EChickenDoing::Sit:
		AnimalPose::Blend(Rest, Table(EChickenPose::Sit), AnimalPose::Ease(Bird.Clock / SitDownSeconds), Out);
		AlertShare = 0.f;
		break;
	case EChickenDoing::Crow:
	{
		const float Rise = AnimalPose::Ease(Bird.Clock / CrowRise);
		const float Fall = AnimalPose::Ease((ChickenTimes::Crow - Bird.Clock) / CrowFall);
		const float Share = FMath::Min(Rise, Fall);
		AnimalPose::Blend(Rest, Table(EChickenPose::Crow), Share, Out);
		// The beak trembling open through the call.
		AnimalPose::Bend(Out, Bones.Jaw, FMath::Sin(Bird.Clock * 40.f) * 4.f * Share, 0.f, 0.f);
		AlertShare = 0.f;
		break;
	}
	case EChickenDoing::Flee:
	{
		if (Bird.Delay > 0.f)
		{
			Out = Table(EChickenPose::Alert);
			break;
		}
		// Wings flapping between up and down at eight beats a second, the legs running a stride under them.
		const float Beat = 0.5f + 0.5f * FMath::Sin(UE_TWO_PI * FlutterBeats * Bird.Clock);
		AnimalPose::Blend(Table(EChickenPose::FlutterUp), Table(EChickenPose::FlutterDown), Beat, Out);
		FAnimalPose Legs;
		AnimalPose::StrideBlend(Table(EChickenPose::FlutterUp), Table(EChickenPose::FlutterUp, true), Bird.Stride, Legs);
		AnimalPose::BlendBones(Out, Legs, 1.f, Bones.Legs);
		AlertShare = 0.f;
		break;
	}
	case EChickenDoing::Stand:
	default:
		Out = Rest;
		break;
	}
	// Alert: stretched up tall, watching (only partly while pecking or scratching).
	if (Bird.Alert * AlertShare > 0.01f)
	{
		AnimalPose::Blend(Out, Table(EChickenPose::Alert), Bird.Alert * AlertShare, Out);
	}
	// The head's look, carried by the upper neck and the head.
	AnimalPose::Look(Out, Bones.Neck2, Bird.HeadYaw * 0.35f, Bird.HeadPitch * 0.3f);
	AnimalPose::Look(Out, Bones.Head, Bird.HeadYaw * 0.65f, Bird.HeadPitch * 0.7f);
}

void AChickenYard::UpdateSprings(int32 Index, float DeltaSeconds)
{
	FChicken& Bird = Birds[Index];
	const UPoseableMeshComponent* Figure = GetFigure(Index);
	if (!Figure || DeltaSeconds <= 0.f)
	{
		return;
	}
	const AnimalPoseTables::FChickenBones& Bones = AnimalPoseTables::ChickenBones();
	const FTransform& Place = Figure->GetComponentTransform();
	const FVector Head = Place.TransformPosition(Rigs[Index].JointOf(Bones.Head));
	const FVector Tail = Place.TransformPosition(Rigs[Index].JointOf(Bones.Tail));
	const FVector HeadVelocity = (Head - Bird.LastHead) / DeltaSeconds;
	const FVector TailVelocity = (Tail - Bird.LastTail) / DeltaSeconds;
	// Two updates in hand before a jolt can be measured (the first after coming into view has no last place).
	if (Bird.SpringFrames >= 2)
	{
		auto Push = [&](const FVector& Velocity, const FVector& LastVelocity, float Strength)
		{
			// The joint's acceleration in the bird's own frame: forward throws the hanging part back, sideways the other way.
			const FVector Acceleration = Place.InverseTransformVectorNoScale((Velocity - LastVelocity) / DeltaSeconds)
				.GetClampedToMaxSize(MostAcceleration);
			return FVector2D(-Acceleration.X - 0.5 * Acceleration.Z, -Acceleration.Y) * Strength;
		};
		Bird.Wattles.Update(DeltaSeconds, Push(HeadVelocity, Bird.LastHeadVelocity, WattlePush), WattleHz, WattleDamping, SpringLimit);
		Bird.TailTip.Update(DeltaSeconds, Push(TailVelocity, Bird.LastTailVelocity, TailPush), TailHz, TailDamping, SpringLimit);
	}
	Bird.SpringFrames = FMath::Min(Bird.SpringFrames + 1, 2);
	Bird.LastHeadVelocity = Bird.SpringFrames >= 2 ? HeadVelocity : FVector::ZeroVector;
	Bird.LastTailVelocity = Bird.SpringFrames >= 2 ? TailVelocity : FVector::ZeroVector;
	Bird.LastHead = Head;
	Bird.LastTail = Tail;
}
