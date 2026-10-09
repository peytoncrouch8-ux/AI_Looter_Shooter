// AFaunaSwarm: its pool of insects, the zones near the view bringing them out, the flies' buzz and the drawing. Each
// kind's motion is in FaunaSwarmMotion.cpp.

#include "World/FaunaSwarm.h"
#include "World/FaunaRules.h"
#include "Audio/LooterSound.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	/** A shrunken instance (an insect not out, a firefly between flashes). */
	constexpr float HiddenScale = 0.001f;

	/** The longest step an insect's motion takes in one go (s): longer updates are split. */
	constexpr float MaxStep = 1.f / 20.f;

	/** A butterfly's wings: up this far on average, swinging this much either way (degrees); closed over its back at rest. */
	constexpr float ButterflyWingMean = 25.f;
	constexpr float ButterflyWingSwing = 60.f;
	constexpr float ButterflyWingClosed = 82.f;

	void Batch(UInstancedStaticMeshComponent* Pieces, const TArray<FTransform>& Transforms)
	{
		if (Pieces && Transforms.Num() > 0 && Pieces->GetInstanceCount() == Transforms.Num())
		{
			// No render state rebuild: the instances' changes go to the renderer as they are.
			Pieces->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/false, /*bTeleport=*/true);
		}
	}
}

AFaunaSwarm::AFaunaSwarm()
{
	CullDistance = 4500.f;
}

void AFaunaSwarm::BeginPlay()
{
	Super::BeginPlay();
	SetupSwarm();
}

void AFaunaSwarm::SetupSwarm()
{
	if (bReady)
	{
		return;
	}
	bReady = true;
	Random.Initialize(Seed);
	// Tiny things cast no shadow worth its cost (MakeInstances leaves them shadowless).
	BodyInstances = BodyMesh ? MakeInstances(TEXT("Bodies"), BodyMesh) : nullptr;
	WingInstancesL = WingMeshL ? MakeInstances(TEXT("WingsL"), WingMeshL) : nullptr;
	WingInstancesR = WingMeshR ? MakeInstances(TEXT("WingsR"), WingMeshR) : nullptr;
	if (Kind == EFaunaInsect::Fly && !SoundCue.IsNone())
	{
		SoundAnchor = NewObject<USceneComponent>(this, TEXT("SoundAnchor"), RF_Transient);
		SoundAnchor->SetMobility(EComponentMobility::Movable);
		SoundAnchor->SetupAttachment(Root);
		SoundAnchor->RegisterComponent();
	}
	Insects.SetNum(MaxActive);
	const FTransform Hidden(FQuat::Identity, GetActorLocation(), FVector(HiddenScale));
	for (UInstancedStaticMeshComponent* Pieces : { BodyInstances.Get(), WingInstancesL.Get(), WingInstancesR.Get() })
	{
		if (Pieces)
		{
			TArray<FTransform> Start;
			Start.Init(Hidden, MaxActive);
			Pieces->AddInstances(Start, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/true, /*bUpdateNavigation=*/false);
		}
	}
	// Until a view comes near, its middle is the zones' middle.
	if (Zones.Num() > 0)
	{
		FVector Sum = FVector::ZeroVector;
		for (const FFaunaZone& Zone : Zones)
		{
			Sum += Zone.Center;
		}
		ActiveCenter = Sum / Zones.Num();
		for (const FFaunaZone& Zone : Zones)
		{
			ActiveReach = FMath::Max(ActiveReach, static_cast<float>(FVector::Dist(ActiveCenter, Zone.Center)) + Zone.Radius);
		}
	}
}

FVector AFaunaSwarm::GetFaunaCenter() const
{
	return ActiveCenter;
}

float AFaunaSwarm::GetFaunaRadius() const
{
	return ActiveReach;
}

void AFaunaSwarm::OnFaunaShownChanged(bool bShown)
{
	if (!bShown)
	{
		LooterSound::Stop(Loop.Get());
		Loop.Reset();
	}
}

FVector AFaunaSwarm::PointIn(const FFaunaZone& Zone)
{
	// The swarm's own stream (never FMath's), so a run plays out the same; the square root spreads them evenly over the disc.
	const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
	const float Distance = FMath::Sqrt(Random.FRand()) * Zone.Radius;
	const float Height = Random.FRandRange(Zone.MinHeight, FMath::Max(Zone.MinHeight, Zone.MaxHeight));
	return Zone.Center + FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, Height);
}

void AFaunaSwarm::Spawn(FFaunaInsect& Insect, int32 Zone)
{
	Insect = FFaunaInsect();
	Insect.Zone = Zone;
	Insect.Position = PointIn(Zones[Zone]);
	Insect.Goal = PointIn(Zones[Zone]);
	Insect.Scale = InsectScale * Random.FRandRange(0.85f, 1.15f);
	Insect.Yaw = Random.FRandRange(-180.f, 180.f);
	Insect.FlapPhase = Random.FRandRange(0.f, UE_TWO_PI);
	Insect.FlapRate = Random.FRandRange(7.5f, 10.5f);
	Insect.Phase = FVector(Random.FRandRange(0.f, 10.f), Random.FRandRange(0.f, 10.f), Random.FRandRange(0.f, 10.f));
	Insect.Timer = Random.FRandRange(0.f, 2.f);
	if (Kind == EFaunaInsect::Dragonfly)
	{
		Insect.DartTime = Insect.DartDuration = 0.f;
		Insect.DartFrom = Insect.Position;
		Insect.Goal = Insect.Position;
	}
}

void AFaunaSwarm::UpdateZones(const FFaunaContext& Context)
{
	// Which zones are near the view now (all of them without a view: a test, a level with no camera yet).
	TArray<int32> Wanted;
	Wanted.Init(0, Zones.Num());
	int32 Nearest = INDEX_NONE;
	double NearestDistance = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Zones.Num(); ++Index)
	{
		const FFaunaZone& Zone = Zones[Index];
		const double Distance = Context.View.bValid ? FVector::Dist(Zone.Center, Context.View.Location) - Zone.Radius : 0.0;
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = Index;
		}
		Wanted[Index] = Distance < ActiveRadius ? Zone.Count : 0;
	}
	// Away with those whose zone is no longer near; count the rest by zone.
	TArray<int32> Out;
	Out.Init(0, Zones.Num());
	for (FFaunaInsect& Insect : Insects)
	{
		if (Insect.Zone == INDEX_NONE)
		{
			continue;
		}
		if (!Zones.IsValidIndex(Insect.Zone) || Wanted[Insect.Zone] <= Out[Insect.Zone])
		{
			Insect.Zone = INDEX_NONE;
			continue;
		}
		++Out[Insect.Zone];
	}
	ActiveCount = 0;
	for (const FFaunaInsect& Insect : Insects)
	{
		ActiveCount += Insect.Zone != INDEX_NONE ? 1 : 0;
	}
	// Out in the near zones that lack some, up to the cap.
	for (int32 Index = 0; Index < Zones.Num() && ActiveCount < MaxActive; ++Index)
	{
		int32 Room = FaunaRules::SwarmRoom(MaxActive, ActiveCount, Wanted[Index] - Out[Index]);
		for (FFaunaInsect& Insect : Insects)
		{
			if (Room <= 0)
			{
				break;
			}
			if (Insect.Zone == INDEX_NONE)
			{
				Spawn(Insect, Index);
				--Room;
				++ActiveCount;
			}
		}
	}
	// Its middle and reach: what's out, else the nearest zone, so the view's distance to it means something.
	FBox Box(ForceInit);
	for (const FFaunaInsect& Insect : Insects)
	{
		if (Insect.Zone != INDEX_NONE)
		{
			const FFaunaZone& Zone = Zones[Insect.Zone];
			Box += Zone.Center + FVector(Zone.Radius, Zone.Radius, Zone.MaxHeight);
			Box += Zone.Center - FVector(Zone.Radius, Zone.Radius, -Zone.MinHeight);
		}
	}
	if (Box.IsValid)
	{
		ActiveCenter = Box.GetCenter();
		ActiveReach = static_cast<float>(Box.GetExtent().Size());
	}
	else if (Nearest != INDEX_NONE)
	{
		ActiveCenter = Zones[Nearest].Center;
		ActiveReach = Zones[Nearest].Radius;
	}
}

int32 AFaunaSwarm::ThreatNear(const FVector& Where, const FFaunaContext& Context) const
{
	for (int32 Index = 0; Index < Context.Threats.Num(); ++Index)
	{
		const FFaunaThreat& Threat = Context.Threats[Index];
		// Measured to the middle of a player's body, where a passing insect would meet it.
		if (Threat.bPlayer && FVector::DistSquared(Where, Threat.Location) < FMath::Square(FleeRadius))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void AFaunaSwarm::UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context)
{
	if (!bReady)
	{
		SetupSwarm();
	}
	UpdateZones(Context);
	DartSoundCooldown = FMath::Max(0.f, DartSoundCooldown - Tick.DeltaSeconds);
	// Long updates (off screen, far) are split so the motion's steering stays steady.
	float Left = Tick.DeltaSeconds;
	while (Left > 0.f)
	{
		const float Step = FMath::Min(Left, MaxStep);
		Left -= Step;
		for (FFaunaInsect& Insect : Insects)
		{
			if (Insect.Zone == INDEX_NONE)
			{
				continue;
			}
			Insect.Age += Step;
			switch (Kind)
			{
			case EFaunaInsect::Butterfly:
				MoveButterfly(Insect, Step, Context);
				break;
			case EFaunaInsect::Dragonfly:
				MoveDragonfly(Insect, Step, Context);
				break;
			case EFaunaInsect::Firefly:
				MoveFirefly(Insect, Step);
				break;
			case EFaunaInsect::Fly:
				MoveFly(Insect, Step);
				break;
			}
		}
	}
	UpdateLoop(Context);
	if (Tick.bOnScreen && IsFaunaShown())
	{
		PushTransforms();
	}
}

void AFaunaSwarm::UpdateLoop(const FFaunaContext& Context)
{
	if (!SoundAnchor || SoundCue.IsNone())
	{
		return;
	}
	// The nearest zone with flies out, and the nearest player to it.
	double Best = TNumericLimits<double>::Max();
	FVector Where = FVector::ZeroVector;
	for (const FFaunaInsect& Insect : Insects)
	{
		if (Insect.Zone == INDEX_NONE)
		{
			continue;
		}
		for (const FFaunaThreat& Player : Context.Threats)
		{
			const double Distance = Player.bPlayer ? FVector::Dist(Player.Location, Zones[Insect.Zone].Center) : Best;
			if (Distance < Best)
			{
				Best = Distance;
				Where = Zones[Insect.Zone].Center + FVector(0.0, 0.0, Zones[Insect.Zone].MaxHeight * 0.5f);
			}
		}
	}
	const bool bNear = Best < SoundRange;
	const bool bFar = Best > SoundRange * 1.5;
	if (bNear && !Loop.IsValid() && IsFaunaShown())
	{
		SoundAnchor->SetWorldLocation(Where);
		Loop = LooterSound::Start(this, SoundCue, SoundAnchor);
	}
	else if (bFar && Loop.IsValid())
	{
		LooterSound::Stop(Loop.Get(), 0.6f);
		Loop.Reset();
	}
	else if (Loop.IsValid())
	{
		SoundAnchor->SetWorldLocation(Where);
	}
}

void AFaunaSwarm::PushTransforms()
{
	const int32 Num = Insects.Num();
	BodyPose.SetNum(Num, EAllowShrinking::No);
	WingPoseL.SetNum(Num, EAllowShrinking::No);
	WingPoseR.SetNum(Num, EAllowShrinking::No);
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const FFaunaInsect& Insect = Insects[Index];
		const FTransform Hidden(FQuat::Identity, Insect.Position, FVector(HiddenScale));
		if (Insect.Zone == INDEX_NONE)
		{
			BodyPose[Index] = WingPoseL[Index] = WingPoseR[Index] = Hidden;
			continue;
		}
		float Size = Insect.Scale;
		if (Kind == EFaunaInsect::Firefly)
		{
			// Its light is all of it: between flashes it's gone.
			Size = Insect.Glow > 0.02f ? Insect.Scale * (0.35f + 0.65f * Insect.Glow) : HiddenScale;
		}
		const FTransform Body(FRotator(Insect.Pitch, Insect.Yaw, Insect.Roll).Quaternion(), Insect.Position, FVector(Size));
		BodyPose[Index] = Body;
		if (Kind == EFaunaInsect::Butterfly)
		{
			const float Up = Insect.Rest > 0.f ? ButterflyWingClosed - 25.f * (0.5f + 0.5f * FMath::Sin(Insect.FlapPhase))
				: ButterflyWingMean + ButterflyWingSwing * FMath::Sin(Insect.FlapPhase);
			WingPoseL[Index] = FTransform(FaunaRules::WingTurn(-1, Up, 0.f)) * Body;
			WingPoseR[Index] = FTransform(FaunaRules::WingTurn(1, Up, 0.f)) * Body;
		}
		else
		{
			WingPoseL[Index] = WingPoseR[Index] = Hidden;
		}
	}
	Batch(BodyInstances, BodyPose);
	Batch(WingInstancesL, WingPoseL);
	Batch(WingInstancesR, WingPoseR);
}
