// ACliffSwallowFlock's birds: life at the nest (inside, peeking, clinging at the mouth), the loops out over the slope on
// smooth curves through waypoints, the dart home into the mouth, the burst out to the circle over the cliff, the wings'
// quick beats and swept glides, and the drawing: one instanced component per piece, every bird placed in one batch.

#include "World/CliffSwallowFlock.h"
#include "World/FaunaRules.h"
#include "World/WorldQueries.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"

namespace
{
	/** Leaving the mouth: the first waypoint's distance out and drop (cm). */
	constexpr float FirstOutMin = 300.f;
	constexpr float FirstOutMax = 600.f;
	/** Coming home: the last waypoint, lined up in front of the mouth (cm out). */
	constexpr float ApproachMin = 160.f;
	constexpr float ApproachMax = 240.f;
	/** The dart into the mouth: its speed's share and its shortest and longest time (s). */
	constexpr float DartShare = 0.6f;
	constexpr float DartMin = 0.25f;
	constexpr float DartMax = 0.8f;

	/** The widest a loop swings off straight out from its nest (degrees). */
	constexpr float LoopTurn = 75.f;
	/** A waypoint stays this far over the ground (cm); a line to it looks this far past it for the cliff. */
	constexpr float GroundRoom = 200.f;
	constexpr int32 PointTries = 4;

	/** A circling bird's slow rise and fall (cm). */
	constexpr float SwallowBob = 150.f;

	/** The steepest climb and dive (degrees). */
	constexpr float MostPitch = 40.f;

	/** The wings: a glide's slight lift, how far the hands sweep back at most (degrees), the hand lagging the arm (radians). */
	constexpr float GlideLift = 4.f;
	constexpr float SweepMost = 48.f;
	constexpr float HandLag = 0.9f;
	constexpr float HandShare = 0.45f;

	/** A peeking head sits this far back in the mouth and under its middle (cm, in the mouth's frame). */
	const FVector PeekOffset(-1.4, 0.0, -0.6);

	/** A shrunken piece (not in use for its bird). */
	constexpr float SwallowHiddenScale = 0.001f;

	void Batch(UInstancedStaticMeshComponent* Pieces, const TArray<FTransform>& Transforms)
	{
		if (Pieces && Transforms.Num() > 0 && Pieces->GetInstanceCount() == Transforms.Num())
		{
			Pieces->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/false, /*bTeleport=*/true);
		}
	}
}

void ACliffSwallowFlock::UpdateBird(FCliffSwallow& Bird, float DeltaSeconds)
{
	switch (Bird.State)
	{
	case ESwallowState::Inside:
	case ESwallowState::Clinging:
		// At home: the head turning in quick looks; out again when its stay is over.
		Bird.HeadTimer -= DeltaSeconds;
		if (Bird.HeadTimer <= 0.f)
		{
			Bird.HeadTimer = Bird.Random.FRandRange(0.3f, 1.4f);
			Bird.HeadGoal = Bird.Random.FRandRange(-60.f, 60.f);
			if (Bird.State == ESwallowState::Inside && Bird.Random.FRand() < 0.25f)
			{
				Bird.bPeeking = !Bird.bPeeking;
			}
		}
		Bird.HeadYaw = FMath::FInterpTo(Bird.HeadYaw, Bird.HeadGoal, DeltaSeconds, 18.f);
		Bird.Timer -= DeltaSeconds;
		if (Bird.Timer <= 0.f && !bAlarmed)
		{
			if (Bird.State == ESwallowState::Inside && Bird.Random.FRand() < 0.3f)
			{
				// Out to the mouth for a look first.
				Bird.State = ESwallowState::Clinging;
				Bird.Timer = Bird.Random.FRandRange(2.f, 6.f);
			}
			else if (Bird.State == ESwallowState::Clinging && Bird.Random.FRand() < 0.25f)
			{
				Bird.State = ESwallowState::Inside;
				Bird.Timer = Bird.Random.FRandRange(StaySecondsMin, FMath::Max(StaySecondsMin, StaySecondsMax));
			}
			else
			{
				StartLoop(Bird);
			}
		}
		break;
	case ESwallowState::Startled:
		Bird.Timer -= DeltaSeconds;
		if (Bird.Timer <= 0.f)
		{
			StartCircle(Bird);
		}
		break;
	case ESwallowState::Looping:
		UpdateWings(Bird, DeltaSeconds);
		if (FollowLeg(Bird, DeltaSeconds) && !NextLeg(Bird))
		{
			// In front of its nest: the dart in.
			const FTransform& Mouth = Mouths[Bird.Mouth];
			Bird.P0 = Bird.Position;
			Bird.V0 = Bird.Velocity;
			Bird.P1 = Mouth.GetLocation();
			Bird.V1 = (Bird.P1 - Bird.P0).GetSafeNormal() * (Bird.Speed * 0.25f);
			Bird.LegTime = 0.f;
			Bird.LegDuration = FaunaRules::FlightSeconds(static_cast<float>(FVector::Dist(Bird.P0, Bird.P1)), Bird.Speed * DartShare,
				DartMin, DartMax);
			Bird.State = ESwallowState::Arriving;
		}
		break;
	case ESwallowState::Arriving:
		UpdateWings(Bird, DeltaSeconds);
		if (FollowLeg(Bird, DeltaSeconds))
		{
			Settle(Bird);
		}
		break;
	case ESwallowState::Circling:
		UpdateWings(Bird, DeltaSeconds);
		if (Bird.bJoining)
		{
			Bird.bJoining = !FollowLeg(Bird, DeltaSeconds);
			break;
		}
		{
			const float Radius = FMath::Max(Bird.CircleRadius, 100.f);
			Bird.CircleAngle += CircleDirection * Bird.Speed / Radius * DeltaSeconds;
			Bird.BobPhase += DeltaSeconds * 1.1f;
			const float Angle = Bird.CircleAngle;
			const FVector Position = CircleCenter + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius,
				Bird.CircleHeight + SwallowBob * FMath::Sin(Bird.BobPhase));
			const FVector Velocity = FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0.0) * (CircleDirection * Bird.Speed)
				+ FVector(0.0, 0.0, SwallowBob * 1.1f * FMath::Cos(Bird.BobPhase));
			SteerBody(Bird, Velocity, DeltaSeconds);
			Bird.Position = Position;
		}
		if (!bAlarmed)
		{
			Bird.Timer -= DeltaSeconds;
			if (Bird.Timer <= 0.f)
			{
				StartHome(Bird);
			}
		}
		break;
	}
}

void ACliffSwallowFlock::Settle(FCliffSwallow& Bird)
{
	Bird.State = Bird.Random.FRand() < 0.3f ? ESwallowState::Clinging : ESwallowState::Inside;
	Bird.bPeeking = Bird.State == ESwallowState::Inside && Bird.Random.FRand() < 0.5f;
	Bird.Position = Mouths.IsValidIndex(Bird.Mouth) ? Mouths[Bird.Mouth].GetLocation() : Center;
	Bird.Velocity = FVector::ZeroVector;
	Bird.Route.Reset();
	Bird.bJoining = false;
	Bird.Timer = Bird.Random.FRandRange(StaySecondsMin, FMath::Max(StaySecondsMin, StaySecondsMax));
	Bird.HeadTimer = Bird.Random.FRandRange(0.2f, 1.f);
}

FVector ACliffSwallowFlock::LoopPoint(FCliffSwallow& Bird, const FVector& Previous, float Out, float Rise, float Turn) const
{
	const FTransform& Mouth = Mouths[Bird.Mouth];
	const FVector Home = Mouth.GetLocation();
	const UWorld* World = GetWorld();
	FVector Point = Home + OutFrom(Bird.Mouth).RotateAngleAxis(Turn, FVector::UpVector) * Out + FVector(0.0, 0.0, Rise);
	if (!World)
	{
		return Point;
	}
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SwallowLoop"), this, false);
	for (int32 Try = 0; Try < PointTries; ++Try)
	{
		// Clear of the cliff on the way there (a little past it, as the curve swings), and well over the ground.
		FHitResult Hit;
		const FVector Past = Point + (Point - Previous).GetSafeNormal() * 150.0;
		const bool bBlocked = World->LineTraceSingleByObjectType(Hit, Previous, Past, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		FHitResult Ground;
		if (World->LineTraceSingleByObjectType(Ground, Point + FVector(0.0, 0.0, 50.0), Point - FVector(0.0, 0.0, GroundRoom),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			Point.Z = Ground.ImpactPoint.Z + GroundRoom;
		}
		if (!bBlocked)
		{
			return Point;
		}
		// Blocked: nearer the nest's way out, and higher.
		Turn *= 0.5f;
		Out *= 0.7f;
		Rise = FMath::Abs(Rise) + 200.f;
		Point = Home + OutFrom(Bird.Mouth).RotateAngleAxis(Turn, FVector::UpVector) * Out + FVector(0.0, 0.0, Rise);
	}
	return Point;
}

void ACliffSwallowFlock::StartLoop(FCliffSwallow& Bird)
{
	const FTransform& Mouth = Mouths[Bird.Mouth];
	const FVector Home = Mouth.GetLocation();
	const FVector Front = Mouth.GetUnitAxis(EAxis::X);
	const FVector Out = OutFrom(Bird.Mouth);
	// Off the mouth along its spout, dropping away from the cliff.
	Bird.Position = Home;
	Bird.Velocity = (Front * 0.7 + Out * 0.5 - FVector::UpVector * 0.3).GetSafeNormal() * (Bird.Speed * 0.6f);
	Bird.Route.Reset();
	FVector Previous = Home;
	const FVector First = LoopPoint(Bird, Previous, Bird.Random.FRandRange(FirstOutMin, FirstOutMax), -Bird.Random.FRandRange(50.f, 150.f), 0.f);
	Bird.Route.Add(First);
	Previous = First;
	// Out over the slope and round, a swing to one side on the way out and back on the other.
	const int32 Far = Bird.Random.RandRange(1, 3);
	const float Side = Bird.Random.FRand() < 0.5f ? -1.f : 1.f;
	for (int32 Index = 0; Index < Far; ++Index)
	{
		const float Turn = Side * Bird.Random.FRandRange(10.f, LoopTurn) * (Index % 2 == 0 ? 1.f : -0.6f);
		const FVector Point = LoopPoint(Bird, Previous, Bird.Random.FRandRange(LoopOutMin, FMath::Max(LoopOutMin, LoopOutMax)),
			Bird.Random.FRandRange(-LoopBelow, LoopAbove), Turn);
		Bird.Route.Add(Point);
		Previous = Point;
	}
	// Back, lined up in front of the mouth for the dart in.
	Bird.Route.Add(Home + Out * Bird.Random.FRandRange(ApproachMin, ApproachMax) + FVector(0.0, 0.0, Bird.Random.FRandRange(-40.f, 10.f)));
	Bird.State = ESwallowState::Looping;
	Bird.bPeeking = false;
	Bird.FlapGoal = 1.f;
	Bird.FlapBout = Bird.Random.FRandRange(0.3f, 0.8f);
	NextLeg(Bird);
}

void ACliffSwallowFlock::StartCircle(FCliffSwallow& Bird)
{
	if (Bird.State == ESwallowState::Startled || Bird.State == ESwallowState::Inside || Bird.State == ESwallowState::Clinging)
	{
		// Bursting out of the mouth.
		const FTransform& Mouth = Mouths[Bird.Mouth];
		Bird.Position = Mouth.GetLocation();
		Bird.Velocity = (Mouth.GetUnitAxis(EAxis::X) + OutFrom(Bird.Mouth)).GetSafeNormal() * (Bird.Speed * 0.7f);
	}
	Bird.CircleAngle = static_cast<float>(FMath::Atan2(Bird.Position.Y - CircleCenter.Y, Bird.Position.X - CircleCenter.X))
		+ Bird.Random.FRandRange(-0.5f, 0.5f);
	Bird.CircleRadius = CircleRadius * Bird.Random.FRandRange(0.7f, 1.3f);
	Bird.CircleHeight = CircleHeight * Bird.Random.FRandRange(-0.3f, 0.3f);
	Bird.BobPhase = Bird.Random.FRandRange(0.f, UE_TWO_PI);
	const float Angle = Bird.CircleAngle;
	const FVector Entry = CircleCenter + FVector(FMath::Cos(Angle) * Bird.CircleRadius, FMath::Sin(Angle) * Bird.CircleRadius,
		Bird.CircleHeight + SwallowBob * FMath::Sin(Bird.BobPhase));
	Bird.P0 = Bird.Position;
	Bird.V0 = Bird.Velocity.SizeSquared() > 100.0 ? Bird.Velocity : FVector::UpVector * Bird.Speed;
	Bird.P1 = Entry;
	Bird.V1 = FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0.0) * (CircleDirection * Bird.Speed);
	Bird.LegTime = 0.f;
	Bird.LegDuration = FaunaRules::FlightSeconds(static_cast<float>(FVector::Dist(Bird.P0, Bird.P1)), Bird.Speed * 0.9f, 0.8f, 5.f);
	Bird.Route.Reset();
	Bird.State = ESwallowState::Circling;
	Bird.bJoining = true;
	Bird.bPeeking = false;
	Bird.FlapGoal = 1.f;
	Bird.FlapBout = Bird.LegDuration;
}

void ACliffSwallowFlock::StartHome(FCliffSwallow& Bird)
{
	// Down from the circle by a point out over the slope, then lined up in front of its nest.
	const FTransform& Mouth = Mouths[Bird.Mouth];
	const FVector Home = Mouth.GetLocation();
	const FVector Out = OutFrom(Bird.Mouth);
	Bird.Route.Reset();
	Bird.Route.Add(LoopPoint(Bird, Bird.Position, Bird.Random.FRandRange(LoopOutMin, FMath::Max(LoopOutMin, LoopOutMax)) * 0.6f,
		Bird.Random.FRandRange(0.f, LoopAbove * 0.5f), Bird.Random.FRandRange(-40.f, 40.f)));
	Bird.Route.Add(Home + Out * Bird.Random.FRandRange(ApproachMin, ApproachMax));
	Bird.State = ESwallowState::Looping;
	Bird.bJoining = false;
	NextLeg(Bird);
}

bool ACliffSwallowFlock::NextLeg(FCliffSwallow& Bird)
{
	if (Bird.Route.IsEmpty())
	{
		return false;
	}
	// Through the waypoints on a smooth curve: each leaves along the line from the last to the next (Catmull-Rom).
	const FVector Next = Bird.Route[0];
	Bird.P0 = Bird.Position;
	Bird.V0 = Bird.Velocity.SizeSquared() > 100.0 ? Bird.Velocity.GetSafeNormal() * Bird.Speed : (Next - Bird.Position).GetSafeNormal() * Bird.Speed;
	Bird.P1 = Next;
	if (Bird.Route.Num() > 1)
	{
		Bird.V1 = (Bird.Route[1] - Bird.Position).GetSafeNormal() * Bird.Speed;
	}
	else
	{
		// The last: slowing, pointed at the mouth.
		const FVector Home = Mouths[Bird.Mouth].GetLocation();
		Bird.V1 = (Home - Next).GetSafeNormal() * (Bird.Speed * 0.8f);
	}
	Bird.LegTime = 0.f;
	Bird.LegDuration = FaunaRules::FlightSeconds(static_cast<float>(FVector::Dist(Bird.P0, Bird.P1)), Bird.Speed, 0.3f, 8.f);
	Bird.Route.RemoveAt(0);
	return true;
}

bool ACliffSwallowFlock::FollowLeg(FCliffSwallow& Bird, float DeltaSeconds)
{
	Bird.LegTime += DeltaSeconds;
	const float Alpha = Bird.LegTime / FMath::Max(Bird.LegDuration, 0.01f);
	FVector Velocity;
	const FVector Position = FaunaRules::Hermite(Bird.P0, Bird.V0, Bird.P1, Bird.V1, Bird.LegDuration, Alpha, &Velocity);
	SteerBody(Bird, Velocity, DeltaSeconds);
	Bird.Position = Position;
	return Alpha >= 1.f;
}

void ACliffSwallowFlock::SteerBody(FCliffSwallow& Bird, const FVector& NewVelocity, float DeltaSeconds)
{
	Bird.Velocity = NewVelocity;
	const float Ground = static_cast<float>(NewVelocity.Size2D());
	if (Ground < 20.f)
	{
		return;
	}
	const float NewYaw = FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(NewVelocity.Y, NewVelocity.X)));
	const float Turned = FMath::FindDeltaAngleDegrees(Bird.Yaw, NewYaw);
	const float TurnRate = DeltaSeconds > 0.f ? FMath::DegreesToRadians(Turned) / DeltaSeconds : 0.f;
	Bird.Yaw = NewYaw;
	const float Climb = FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(NewVelocity.Z, static_cast<double>(Ground))));
	const float PitchGoal = FMath::Clamp(Climb * 0.8f, -MostPitch, MostPitch);
	// A swallow banks hard into its turns: a positive roll drops the right wing, turning right.
	const float RollGoal = FaunaRules::BankDegrees(static_cast<float>(NewVelocity.Size()), TurnRate, MaxBank);
	if (DeltaSeconds <= 0.f)
	{
		Bird.Pitch = PitchGoal;
		Bird.Roll = RollGoal;
		return;
	}
	Bird.Pitch = FMath::FInterpTo(Bird.Pitch, PitchGoal, DeltaSeconds, 8.f);
	Bird.Roll = FMath::FInterpTo(Bird.Roll, RollGoal, DeltaSeconds, 7.f);
}

void ACliffSwallowFlock::UpdateWings(FCliffSwallow& Bird, float DeltaSeconds)
{
	const float Speed = static_cast<float>(Bird.Velocity.Size());
	const bool bDiving = Speed > 1.f && Bird.Velocity.Z < -0.3f * Speed;
	Bird.FlapBout -= DeltaSeconds;
	if (Bird.FlapBout <= 0.f)
	{
		// Bouts of quick beats and short glides.
		const bool bGlide = Bird.Random.FRand() < GlideShare;
		Bird.FlapGoal = bGlide ? 0.f : 1.f;
		Bird.FlapBout = bGlide ? Bird.Random.FRandRange(0.3f, 0.9f) : Bird.Random.FRandRange(0.3f, 1.f);
	}
	const float Goal = bDiving ? 0.f : Bird.FlapGoal;
	Bird.FlapBlend = FMath::FInterpTo(Bird.FlapBlend, Goal, DeltaSeconds, 12.f);
	Bird.FlapPhase = FMath::Fmod(Bird.FlapPhase + UE_TWO_PI * Bird.FlapRate * (0.5f + 0.5f * Bird.FlapBlend) * DeltaSeconds, UE_TWO_PI);
	// The hands swept back to glide, all the way for a dive.
	const float SweepGoal = bDiving ? 1.f : (1.f - Bird.FlapBlend) * 0.55f + 0.08f;
	Bird.Sweep = FMath::FInterpTo(Bird.Sweep, SweepGoal, DeltaSeconds, 6.f);
}

void ACliffSwallowFlock::MakeComponents()
{
	auto Make = [this](const TCHAR* Name, UStaticMesh* Mesh) -> UInstancedStaticMeshComponent*
	{
		return Mesh ? MakeInstances(FName(Name), Mesh) : nullptr;
	};
	PerchedInstances = Make(TEXT("ClingingSwallows"), PerchedMesh);
	HeadInstances = Make(TEXT("SwallowHeads"), HeadMesh);
	FlyingInstances = Make(TEXT("FlyingSwallows"), FlyingMesh);
	InnerInstancesL = Make(TEXT("SwallowArmsL"), WingInnerMeshL);
	InnerInstancesR = Make(TEXT("SwallowArmsR"), WingInnerMeshR);
	OuterInstancesL = Make(TEXT("SwallowHandsL"), WingOuterMeshL);
	OuterInstancesR = Make(TEXT("SwallowHandsR"), WingOuterMeshR);
}

void ACliffSwallowFlock::ReadSockets()
{
	auto Read = [](const UStaticMesh* Mesh, const TCHAR* Name, FVector& Out)
	{
		if (const UStaticMeshSocket* Socket = Mesh ? Mesh->FindSocket(FName(Name)) : nullptr)
		{
			Out = Socket->RelativeLocation;
		}
	};
	Read(PerchedMesh, TEXT("Head"), HeadSocket);
	Read(FlyingMesh, TEXT("WingL"), ShoulderL);
	Read(FlyingMesh, TEXT("WingR"), ShoulderR);
	Read(WingInnerMeshL, TEXT("Wrist"), WristL);
	Read(WingInnerMeshR, TEXT("Wrist"), WristR);
}

void ACliffSwallowFlock::PushTransforms()
{
	const int32 Num = Birds.Num();
	if (Num == 0)
	{
		return;
	}
	for (TArray<FTransform>* Pose : { &PerchedPose, &HeadPose, &FlyingPose, &InnerPoseL, &InnerPoseR, &OuterPoseL, &OuterPoseR })
	{
		Pose->SetNum(Num, EAllowShrinking::No);
	}
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const FCliffSwallow& Bird = Birds[Index];
		const FTransform Hidden(FQuat::Identity, Bird.Position, FVector(SwallowHiddenScale));
		const FVector Scale(Bird.Scale);
		const bool bHome = Bird.State == ESwallowState::Inside || Bird.State == ESwallowState::Clinging
			|| Bird.State == ESwallowState::Startled;
		if (bHome)
		{
			FlyingPose[Index] = InnerPoseL[Index] = InnerPoseR[Index] = OuterPoseL[Index] = OuterPoseR[Index] = Hidden;
			const FTransform& Mouth = Mouths.IsValidIndex(Bird.Mouth) ? Mouths[Bird.Mouth] : FTransform::Identity;
			if (Bird.State == ESwallowState::Inside)
			{
				// Inside: nothing, or only its head peeking out of the mouth.
				PerchedPose[Index] = Hidden;
				HeadPose[Index] = Bird.bPeeking
					? FTransform(FRotator(0.f, Bird.HeadYaw * 0.5f, 0.f), PeekOffset * Bird.Scale, Scale) * Mouth : Hidden;
				continue;
			}
			// Clinging at the mouth, turned to face the hole; its head on its neck, looking about.
			const FTransform Body = FTransform(FRotator(0.f, 180.f, 0.f), FVector::ZeroVector, Scale) * Mouth;
			PerchedPose[Index] = Body;
			HeadPose[Index] = FTransform(FRotator(0.f, Bird.HeadYaw, 0.f), HeadSocket) * Body;
			continue;
		}
		PerchedPose[Index] = HeadPose[Index] = Hidden;
		const FTransform Body(FRotator(Bird.Pitch, Bird.Yaw, Bird.Roll).Quaternion(), Bird.Position, Scale);
		FlyingPose[Index] = Body;
		// The arm beats about the glide's lift; the hand lags it, so the wing bends through each beat, and sweeps back.
		const float Up = Bird.FlapBlend * WingbeatDegrees * (FMath::Sin(Bird.FlapPhase) + 0.15f) + GlideLift * (1.f - Bird.FlapBlend)
			+ Bird.Sweep * 10.f;
		const float Back = Bird.FlapBlend * 8.f * FMath::Max(0.f, FMath::Cos(Bird.FlapPhase)) + Bird.Sweep * 10.f;
		InnerPoseL[Index] = FTransform(FaunaRules::WingTurn(-1, Up, Back), ShoulderL) * Body;
		InnerPoseR[Index] = FTransform(FaunaRules::WingTurn(1, Up, Back), ShoulderR) * Body;
		const float Bend = Bird.FlapBlend * WingbeatDegrees * HandShare * FMath::Sin(Bird.FlapPhase - HandLag) - Bird.Sweep * 6.f;
		const float HandSweep = Bird.Sweep * SweepMost;
		OuterPoseL[Index] = FTransform(FaunaRules::WingTurn(-1, Bend, HandSweep), WristL) * InnerPoseL[Index];
		OuterPoseR[Index] = FTransform(FaunaRules::WingTurn(1, Bend, HandSweep), WristR) * InnerPoseR[Index];
	}
	Batch(PerchedInstances, PerchedPose);
	Batch(HeadInstances, HeadPose);
	Batch(FlyingInstances, FlyingPose);
	Batch(InnerInstancesL, InnerPoseL);
	Batch(InnerInstancesR, InnerPoseR);
	Batch(OuterInstancesL, OuterPoseL);
	Batch(OuterInstancesR, OuterPoseR);
}
