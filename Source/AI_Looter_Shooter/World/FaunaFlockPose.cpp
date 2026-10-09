// AFaunaFlock's perched life (the head's quick turns, shuffles, preening, walking or hopping and pecking on the ground)
// and its drawing: one instanced component per piece, every bird's pieces placed in one batch per piece.

#include "World/FaunaFlock.h"
#include "World/FaunaRules.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

namespace
{
	/** How quickly a head snaps to where it looks next, and a body tilts (rates for FInterpTo). */
	constexpr float HeadSnap = 22.f;
	constexpr float BodyTilt = 14.f;

	/** The widest a head turns and tilts while looking about (degrees). */
	constexpr float HeadTurn = 80.f;
	constexpr float HeadUp = 18.f;
	constexpr float HeadDown = -20.f;

	/** A peck: down this far (degrees, about the feet) for its first part, up again for the rest (s). */
	constexpr float PeckDown = -42.f;
	constexpr float PeckSeconds = 0.3f;
	constexpr float PeckDownSeconds = 0.12f;

	/** Turning on its perch (degrees a second). */
	constexpr float ShuffleTurnRate = 720.f;

	/** A sparrow's hops while it moves on the ground: how high (cm) and how many a metre. */
	constexpr float HopHeight = 4.f;
	constexpr float HopsPerMetre = 7.f;

	/** A shrunken piece (not in use for its bird). */
	constexpr float HiddenScale = 0.001f;

	/** On the upstroke the wings sweep back this much (degrees), and the outer wing lags the inner by this (radians). */
	constexpr float UpstrokeSweep = 14.f;
	constexpr float OuterLag = 0.9f;
	constexpr float OuterShare = 0.45f;

	void Batch(UInstancedStaticMeshComponent* Pieces, const TArray<FTransform>& Transforms)
	{
		if (Pieces && Transforms.Num() > 0 && Pieces->GetInstanceCount() == Transforms.Num())
		{
			// No render state rebuild: the instances' changes go to the renderer as they are.
			Pieces->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/false, /*bTeleport=*/true);
		}
	}
}

void AFaunaFlock::UpdatePerched(FFaunaBird& Bird, float DeltaSeconds)
{
	const FFaunaPerch* Perch = Perches.IsValidIndex(Bird.Perch) ? &Perches[Bird.Perch] : nullptr;
	const bool bGround = Perch && Perch->Kind == EFaunaPerchKind::Ground;

	// The head: still a while, then a quick turn (birds look about in steps, not sweeps).
	Bird.HeadTimer -= DeltaSeconds;
	if (Bird.HeadTimer <= 0.f && Bird.State == EFaunaBirdState::Perched && Bird.PecksLeft == 0)
	{
		Bird.HeadTimer = Bird.Random.FRandRange(HeadSecondsMin, FMath::Max(HeadSecondsMin, HeadSecondsMax));
		Bird.HeadYawGoal = Bird.Random.FRandRange(-HeadTurn, HeadTurn);
		Bird.HeadPitchGoal = Bird.Random.FRandRange(HeadDown, HeadUp);
	}

	// A call: the head thrown up and the body bobbing once.
	if (Bird.CallTime > 0.f)
	{
		Bird.CallTime -= DeltaSeconds;
		Bird.HeadPitchGoal = 28.f;
		Bird.BodyPitchGoal = 9.f * FMath::Sin(FMath::Max(Bird.CallTime, 0.f) * 14.f);
		if (Bird.CallTime <= 0.f)
		{
			Bird.BodyPitchGoal = 0.f;
		}
	}
	else if (Bird.PecksLeft > 0)
	{
		// Pecking: down hard and up, a few times, looking at the ground.
		Bird.PeckTimer -= DeltaSeconds;
		if (Bird.PeckTimer <= 0.f)
		{
			--Bird.PecksLeft;
			Bird.PeckTimer = PeckSeconds;
		}
		Bird.BodyPitchGoal = Bird.PecksLeft > 0 && Bird.PeckTimer > PeckSeconds - PeckDownSeconds ? PeckDown : -10.f;
		Bird.HeadPitchGoal = -25.f;
		Bird.HeadYawGoal = 0.f;
		if (Bird.PecksLeft == 0)
		{
			Bird.BodyPitchGoal = 0.f;
		}
	}
	else if (Bird.WalkDuration > 0.f && Perch)
	{
		// Walking (a crow's bobbing stride) or hopping (a sparrow's) to its next spot.
		Bird.WalkTime += DeltaSeconds;
		const float Alpha = FMath::Clamp(Bird.WalkTime / Bird.WalkDuration, 0.f, 1.f);
		const FVector2D Offset = FMath::Lerp(Bird.WalkFrom, Bird.WalkTo, FMath::SmoothStep(0.f, 1.f, Alpha));
		const float Metres = static_cast<float>(FVector2D::Distance(Bird.WalkFrom, Bird.WalkTo)) / 100.f;
		const float Steps = FMath::Max(1.f, FMath::RoundToFloat(Metres * HopsPerMetre));
		const float Lift = bHopsOnGround ? FMath::Abs(FMath::Sin(UE_PI * Alpha * Steps)) * HopHeight * Bird.Scale : 0.f;
		Bird.Position = FaunaRules::GroundNear(*Perch, Offset) + FVector(0.0, 0.0, Lift);
		Bird.BodyPitchGoal = bHopsOnGround ? 0.f : -6.f + 3.f * FMath::Sin(UE_TWO_PI * Alpha * Steps);
		if (Alpha >= 1.f)
		{
			Bird.WalkFrom = Bird.WalkTo;
			Bird.WalkDuration = 0.f;
			Bird.BodyPitchGoal = 0.f;
		}
	}
	else if (Bird.State == EFaunaBirdState::Perched)
	{
		Bird.IdleTimer -= DeltaSeconds;
		if (Bird.IdleTimer <= 0.f)
		{
			Bird.IdleTimer = Bird.Random.FRandRange(2.f, 6.f);
			Bird.BodyPitchGoal = 0.f;
			const float Roll = Bird.Random.FRand();
			if (bGround)
			{
				if (Roll < 0.55f || Perch->Wander < 10.f)
				{
					Bird.PecksLeft = Bird.Random.RandRange(2, 4);
					Bird.PeckTimer = PeckSeconds;
				}
				else
				{
					const float Angle = Bird.Random.FRandRange(0.f, UE_TWO_PI);
					const float Distance = Perch->Wander * Bird.Random.FRandRange(0.3f, 1.f);
					Bird.WalkTo = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance;
					Bird.WalkTime = 0.f;
					Bird.WalkDuration = FMath::Max(0.3f, static_cast<float>(FVector2D::Distance(Bird.WalkFrom, Bird.WalkTo)) / GroundSpeed);
					const FVector2D Way = Bird.WalkTo - Bird.WalkFrom;
					Bird.WalkYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Way.Y, Way.X)));
				}
			}
			else if (Roll < 0.25f)
			{
				// Turning about on its perch: round on a rail or a ridge, any way on a post or a limb.
				const bool bAcross = Perch && (Perch->Kind == EFaunaPerchKind::Rail || Perch->Kind == EFaunaPerchKind::Roof);
				Bird.WalkYaw = Bird.Yaw + (bAcross ? 180.f : Bird.Random.FRandRange(40.f, 120.f) * (Bird.Random.FRand() < 0.5f ? -1.f : 1.f));
			}
			else if (Roll < 0.45f)
			{
				// Preening: the head turned back into a wing, the body leaning a little, for a moment.
				Bird.BodyPitchGoal = -14.f;
				Bird.HeadYawGoal = Bird.Random.FRand() < 0.5f ? -130.f : 130.f;
				Bird.HeadPitchGoal = -30.f;
				Bird.HeadTimer = 0.9f;
				Bird.IdleTimer = 0.9f;
			}
		}
	}
	Bird.Yaw = FaunaRules::StepAngle(Bird.Yaw, Bird.WalkYaw, ShuffleTurnRate * DeltaSeconds);
	Bird.HeadYaw = FMath::FInterpTo(Bird.HeadYaw, Bird.HeadYawGoal, DeltaSeconds, HeadSnap);
	Bird.HeadPitch = FMath::FInterpTo(Bird.HeadPitch, Bird.HeadPitchGoal, DeltaSeconds, HeadSnap);
	Bird.BodyPitch = FMath::FInterpTo(Bird.BodyPitch, Bird.BodyPitchGoal, DeltaSeconds, BodyTilt);
}

void AFaunaFlock::MakeComponents()
{
	auto Make = [this](const TCHAR* Name, UStaticMesh* Mesh) -> UInstancedStaticMeshComponent*
	{
		return Mesh ? MakeInstances(FName(Name), Mesh) : nullptr;
	};
	if (Mode == EFaunaFlockMode::Perching)
	{
		PerchedInstances = Make(TEXT("PerchedBirds"), PerchedMesh);
		HeadInstances = Make(TEXT("Heads"), HeadMesh);
	}
	FlyingInstances = Make(TEXT("FlyingBirds"), FlyingMesh);
	WingInstancesL = Make(TEXT("WingsL"), WingMeshL);
	WingInstancesR = Make(TEXT("WingsR"), WingMeshR);
	OuterInstancesL = Make(TEXT("OuterWingsL"), OuterWingMeshL);
	OuterInstancesR = Make(TEXT("OuterWingsR"), OuterWingMeshR);
}

void AFaunaFlock::ReadSockets()
{
	// The pieces fit together at their models' sockets; without one the defaults (a crow's proportions) stand in.
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
	Read(WingMeshL, TEXT("Wrist"), WristL);
	Read(WingMeshR, TEXT("Wrist"), WristR);
	if (PerchedMesh)
	{
		BodyLift = FMath::Max(0.f, static_cast<float>(PerchedMesh->GetBoundingBox().GetCenter().Z) * 0.9f);
	}
}

void AFaunaFlock::PushTransforms()
{
	const int32 Num = Birds.Num();
	if (Num == 0)
	{
		return;
	}
	for (TArray<FTransform>* Pose : { &PerchedPose, &HeadPose, &FlyingPose, &WingPoseL, &WingPoseR, &OuterPoseL, &OuterPoseR })
	{
		Pose->SetNum(Num, EAllowShrinking::No);
	}
	const bool bOuter = OuterInstancesL || OuterInstancesR;
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const FFaunaBird& Bird = Birds[Index];
		const FTransform Hidden(FQuat::Identity, Bird.Position, FVector(HiddenScale));
		const bool bPerchedLook = Bird.State == EFaunaBirdState::Perched || Bird.State == EFaunaBirdState::Startled;
		if (Bird.State == EFaunaBirdState::Away)
		{
			PerchedPose[Index] = HeadPose[Index] = FlyingPose[Index] = Hidden;
			WingPoseL[Index] = WingPoseR[Index] = OuterPoseL[Index] = OuterPoseR[Index] = Hidden;
			continue;
		}
		const FVector Scale(Bird.Scale);
		if (bPerchedLook)
		{
			// The body tilts about its feet (the pivot); the head turns on its neck socket.
			const FTransform Body(FRotator(Bird.BodyPitch, Bird.Yaw, 0.f).Quaternion(), Bird.Position, Scale);
			PerchedPose[Index] = Body;
			HeadPose[Index] = FTransform(FRotator(Bird.HeadPitch, Bird.HeadYaw, 0.f).Quaternion(), HeadSocket) * Body;
			FlyingPose[Index] = WingPoseL[Index] = WingPoseR[Index] = OuterPoseL[Index] = OuterPoseR[Index] = Hidden;
			continue;
		}
		const FTransform Body(FRotator(Bird.Pitch, Bird.Yaw, Bird.Roll).Quaternion(), Bird.Position, Scale);
		FlyingPose[Index] = Body;
		PerchedPose[Index] = HeadPose[Index] = Hidden;
		// Beating: about the glide's angle, higher on the upstroke than low on the down; the flare throws them up.
		const float Beat = Bird.FlapBlend * WingbeatDegrees * (FMath::Sin(Bird.FlapPhase) + 0.25f);
		const float Up = GlideDegrees * (1.f - Bird.FlapBlend) + Beat + Bird.Flare;
		const float Back = Bird.FlapBlend * UpstrokeSweep * FMath::Max(0.f, FMath::Cos(Bird.FlapPhase)) + Bird.Flare * 0.3f;
		WingPoseL[Index] = FTransform(FaunaRules::WingTurn(-1, Up, Back), ShoulderL) * Body;
		WingPoseR[Index] = FTransform(FaunaRules::WingTurn(1, Up, Back), ShoulderR) * Body;
		if (bOuter)
		{
			// The hand lags the arm, so the wing bends through each beat instead of flapping like a board.
			const float Bend = Bird.FlapBlend * WingbeatDegrees * OuterShare * FMath::Sin(Bird.FlapPhase - OuterLag) - Bird.Flare * 0.25f;
			OuterPoseL[Index] = FTransform(FaunaRules::WingTurn(-1, Bend, 0.f), WristL) * WingPoseL[Index];
			OuterPoseR[Index] = FTransform(FaunaRules::WingTurn(1, Bend, 0.f), WristR) * WingPoseR[Index];
		}
	}
	Batch(PerchedInstances, PerchedPose);
	Batch(HeadInstances, HeadPose);
	Batch(FlyingInstances, FlyingPose);
	Batch(WingInstancesL, WingPoseL);
	Batch(WingInstancesR, WingPoseR);
	Batch(OuterInstancesL, OuterPoseL);
	Batch(OuterInstancesR, OuterPoseR);
}
