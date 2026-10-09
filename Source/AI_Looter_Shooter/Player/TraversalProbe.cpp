#include "Player/TraversalProbe.h"
#include "Player/TraversalBodyQuery.h"
#include "CollisionShape.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// FTraversalProbe finding the move in front: the face, the top and how deep it runs, then a mantle onto it or a vault
// over it with room for the body. What can be climbed, and the free spot for a wedged player: TraversalProbeChecks.cpp.

namespace
{
	using namespace LooterTraversal;

	FVector Flat(const FVector& Vector)
	{
		return FVector(Vector.X, Vector.Y, 0.0).GetSafeNormal();
	}

	FVector At(const FVector& Point, double Z)
	{
		return FVector(Point.X, Point.Y, Z);
	}

	/** The obstacle's top seen from above: its height at the edge, how deep it runs along the way, and where the hands go. */
	struct FTopScan
	{
		ETraversalRefusal Refusal = ETraversalRefusal::None;
		double TopZ = 0.0;
		double Depth = 0.0;
		/** The top runs on past the scan, or into something taller: a ledge, not a fence. */
		bool bDeep = true;
		FHitResult TopHit;
	};

	/** Everything the moves are worked out from. */
	struct FContext
	{
		const FTraversalAsk& Ask;
		const FTraversalBodyQuery& Query;
		const UCharacterMovementComponent& Move;
		ACharacter* Character;
		FVector Center;
		double FeetZ;
		FVector Velocity;
		FTopScan Scan;
	};

	/**
	 * Balls dropped onto the obstacle just past its face and on along the way, until one falls past its far edge (a fence,
	 * a wall: its depth) or the top runs on or meets something taller (a ledge: deep). The first finds the top's edge.
	 */
	FTopScan ScanTop(const FTraversalBodyQuery& Query, APawn* Climber, const FVector& Center, const FVector& Heading, double NearAlong, double FeetZ,
		double MinTop)
	{
		static constexpr float Insets[] = { 3.f, 10.f, 20.f, 32.f, 45.f, 60.f, 75.f, 90.f, 105.f };
		const FCollisionShape Ball = FCollisionShape::MakeSphere(5.f);
		const double From = FeetZ + MaxMantleHeight + 25.0;
		const double To = FeetZ - MaxVaultDrop - 30.0;
		FTopScan Scan;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Insets); ++Index)
		{
			const FVector Point = Center + Heading * (NearAlong + Insets[Index]);
			FHitResult Hit;
			const bool bHit = Query.Sweep(Hit, At(Point, From), At(Point, To), Ball);
			if (Index == 0)
			{
				// Something there all the way up is too tall to reach over; nothing over the band means the probe met a post
				// or a slat, not a top.
				if (bHit && Hit.bStartPenetrating)
				{
					Scan.Refusal = ETraversalRefusal::TooHigh;
					return Scan;
				}
				if (!bHit || Hit.ImpactPoint.Z < MinTop - 5.0)
				{
					Scan.Refusal = ETraversalRefusal::NoTop;
					return Scan;
				}
				if (!FTraversalProbe::IsClimbable(Hit, Climber))
				{
					Scan.Refusal = ETraversalRefusal::NotClimbable;
					return Scan;
				}
				Scan.TopZ = Hit.ImpactPoint.Z;
				Scan.TopHit = Hit;
				continue;
			}
			const double Z = bHit && !Hit.bStartPenetrating ? Hit.ImpactPoint.Z : -UE_BIG_NUMBER;
			if (Insets[Index] <= Query.Radius && Z > Scan.TopZ)
			{
				// Still rising within a body's width of the face: a face leaning back (a rock, a terrain step), whose edge
				// is further in. That's the top the body has to come over.
				if (!FTraversalProbe::IsClimbable(Hit, Climber))
				{
					Scan.Refusal = ETraversalRefusal::NotClimbable;
					return Scan;
				}
				Scan.TopZ = Z;
				Scan.TopHit = Hit;
				continue;
			}
			if ((bHit && Hit.bStartPenetrating) || Z > Scan.TopZ + 25.0)
			{
				// Meets something taller further in: the top runs on to it (whether the body fits is checked on top).
				Scan.Depth = Insets[Index - 1];
				return Scan;
			}
			if (Z < Scan.TopZ - 25.0)
			{
				// Past the far edge: the floor beyond, or a drop.
				Scan.Depth = 0.5 * (Insets[Index - 1] + Insets[Index]);
				Scan.bDeep = false;
				return Scan;
			}
			if (!FTraversalProbe::IsClimbable(Hit, Climber))
			{
				// Something on top that isn't to be stood on (a creature on the wall).
				Scan.Refusal = ETraversalRefusal::NotClimbable;
				return Scan;
			}
		}
		Scan.Depth = Insets[UE_ARRAY_COUNT(Insets) - 1];
		return Scan;
	}

	/**
	 * Up onto the top, straight in from the face (Into): the body's middle a body's width in from the edge (or in the
	 * middle of a narrow top), on ground it can stand on, with room for it standing, and a clear way up over where the
	 * player is and across over the edge.
	 */
	bool TryMantle(const FContext& Context, const FVector& Into, double FaceDistance, double HeadingIn, FTraversalFind& Out)
	{
		const FTraversalBodyQuery& Query = Context.Query;
		const FTopScan& Scan = Context.Scan;
		const double Radius = Query.Radius;
		const double HalfHeight = Query.HalfHeight;
		const double Gap = FTraversalBodyQuery::FloorGap();
		const double Inset = Scan.bDeep ? Radius + 8.0 : FMath::Clamp(0.5 * Scan.Depth * HeadingIn, 0.5 * MinStandDepth, Radius + 8.0);
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Into);
		const double Side = FMath::Clamp((Context.Velocity | Right) * 0.1, -12.0, 12.0);
		const FVector EndXY = At(Context.Center + Into * (FaceDistance + Inset) + Right * Side, 0.0);

		FHitResult Floor;
		if (!Query.Sweep(Floor, At(EndXY, Scan.TopZ + HalfHeight + 25.0), At(EndXY, Scan.TopZ + HalfHeight - 25.0), Query.Body(0.5f)))
		{
			Out.Refusal = ETraversalRefusal::NoTop;
			return false;
		}
		if (Floor.bStartPenetrating)
		{
			Out.Refusal = ETraversalRefusal::NoRoom;
			return false;
		}
		if (!FTraversalProbe::IsClimbable(Floor, Context.Character))
		{
			Out.Refusal = ETraversalRefusal::NotClimbable;
			return false;
		}
		if (!Context.Move.IsWalkable(Floor))
		{
			Out.Refusal = ETraversalRefusal::NoTop;
			return false;
		}
		const double EndFeet = Floor.Location.Z - HalfHeight + Gap;
		const FVector End = At(EndXY, EndFeet + HalfHeight);

		const double Lift = FMath::Max(FMath::Max(End.Z, Scan.TopZ + Gap + HalfHeight) + 3.0, Context.Center.Z);
		if (Query.Blocked(Context.Center, At(Context.Center, Lift), Query.Body(1.f)))
		{
			Out.Refusal = ETraversalRefusal::NoRoom;
			return false;
		}
		if (Query.Blocked(At(Context.Center, Lift), At(End, Lift), Query.Body(1.f)))
		{
			Out.Refusal = ETraversalRefusal::Blocked;
			return false;
		}
		if (!Query.Inside(End))
		{
			Out.Refusal = ETraversalRefusal::OutsideArea;
			return false;
		}

		Out.Kind = ETraversalKind::Mantle;
		Out.Refusal = ETraversalRefusal::None;
		Out.Direction = Into;
		Out.NearFace = static_cast<float>(FaceDistance);
		Out.FarFace = 1.e6f;
		Out.End = End;
		Out.EndFeetZ = static_cast<float>(EndFeet);
		Out.ExitSpeed = Context.Ask.MantleExitSpeed;
		Out.Duration = MantleSeconds(static_cast<float>(EndFeet - Context.FeetZ));
		Out.FloorHit = Floor;
		return true;
	}

	/**
	 * Over the obstacle along the way the player goes (Heading), landing as far on as the run would carry the body in the
	 * hop (so it keeps its speed), else nearer, but always a body's width past the far face, on floor it can stand on not
	 * far under where it started; with a clear way up over the player and across with the legs tucked.
	 */
	bool TryVault(const FContext& Context, const FVector& Heading, double NearAlong, FTraversalFind& Out)
	{
		const FTraversalBodyQuery& Query = Context.Query;
		const FTopScan& Scan = Context.Scan;
		const double Radius = Query.Radius;
		const double HalfHeight = Query.HalfHeight;
		const double FarAlong = NearAlong + Scan.Depth;
		const double ApexFeet = Scan.TopZ - VaultTuck + 3.0;
		const float Duration = VaultSeconds(static_cast<float>(ApexFeet - Context.FeetZ));
		const double In = FMath::Max(Context.Velocity | Heading, 0.0);
		const double Natural = 0.5 * (In + Context.Ask.VaultExitSpeed) * Duration;
		const double Shortest = FarAlong + Radius + 10.0;
		const double Wanted = FMath::Clamp(0.9 * Natural, Shortest, Shortest + 250.0);
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Heading);
		const double Side = FMath::Clamp((Context.Velocity | Right) * 0.1, -12.0, 12.0);

		const double ApexCenter = ApexFeet + HalfHeight;
		if (Query.Blocked(Context.Center, At(Context.Center, FMath::Max(ApexCenter, Context.Center.Z)), Query.Body(1.f)))
		{
			Out.Refusal = ETraversalRefusal::NoRoom;
			return false;
		}
		// The body with its legs tucked up: its bottom just over the top, its head where the hop puts it.
		const double TuckBottom = Scan.TopZ + 5.0;
		const double TuckTop = ApexFeet + 2.0 * HalfHeight;
		const FCollisionShape Tucked = FCollisionShape::MakeCapsule(static_cast<float>(Radius) - 1.f,
			FMath::Max(static_cast<float>(0.5 * (TuckTop - TuckBottom)), static_cast<float>(Radius)));
		const double TuckMiddle = 0.5 * (TuckBottom + TuckTop);

		const double Candidates[] = { Wanted, 0.5 * (Wanted + Shortest), Shortest };
		Out.Refusal = ETraversalRefusal::NoLanding;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Candidates); ++Index)
		{
			const double Along = Candidates[Index];
			if (Index > 0 && Along > Candidates[Index - 1] - 5.0)
			{
				continue;
			}
			const FVector EndXY = At(Context.Center + Heading * Along + Right * Side, 0.0);
			if (Query.Blocked(At(Context.Center, TuckMiddle), At(EndXY, TuckMiddle), Tucked))
			{
				Out.Refusal = ETraversalRefusal::Blocked;
				continue;
			}
			FHitResult Floor;
			if (!Query.Sweep(Floor, At(EndXY, ApexCenter), At(EndXY, Context.FeetZ - MaxVaultDrop + HalfHeight), Query.Body(0.5f)))
			{
				Out.Refusal = ETraversalRefusal::NoLanding;
				continue;
			}
			if (Floor.bStartPenetrating)
			{
				Out.Refusal = ETraversalRefusal::NoRoom;
				continue;
			}
			if (!FTraversalProbe::IsClimbable(Floor, Context.Character))
			{
				Out.Refusal = ETraversalRefusal::NotClimbable;
				continue;
			}
			const double EndFeet = Floor.Location.Z - HalfHeight + FTraversalBodyQuery::FloorGap();
			// Not ground to stand on, or still the obstacle's top (or something as high): not past it.
			if (!Context.Move.IsWalkable(Floor) || EndFeet > Scan.TopZ - 10.0)
			{
				Out.Refusal = ETraversalRefusal::NoLanding;
				continue;
			}
			const FVector End = At(EndXY, EndFeet + HalfHeight);
			if (!Query.Inside(End))
			{
				Out.Refusal = ETraversalRefusal::OutsideArea;
				continue;
			}

			Out.Kind = ETraversalKind::Vault;
			Out.Refusal = ETraversalRefusal::None;
			Out.Direction = Heading;
			Out.NearFace = static_cast<float>(NearAlong);
			Out.FarFace = static_cast<float>(FarAlong);
			Out.End = End;
			Out.EndFeetZ = static_cast<float>(EndFeet);
			Out.ExitSpeed = Context.Ask.VaultExitSpeed;
			Out.Duration = Duration;
			Out.FloorHit = Floor;
			return true;
		}
		return false;
	}
}

FTraversalFind FTraversalProbe::Find(const FTraversalAsk& Ask)
{
	FTraversalFind Out;
	ACharacter* Character = Ask.Character;
	const UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
	const FVector Heading = Flat(Ask.Heading);
	if (!Move || !Character->GetCapsuleComponent() || !Character->GetWorld() || Heading.IsNearlyZero())
	{
		return Out;
	}
	const FTraversalBodyQuery Query(*Character);
	const FVector Center = Character->GetActorLocation();
	const double FeetZ = Center.Z - Query.HalfHeight;
	const FVector Velocity = Character->GetVelocity();
	const double Speed = Velocity.Size2D();
	const FVector Facing = Flat(Ask.Facing);

	// 1. The face: a probe as wide as the body swept ahead through the band a ledge's face can be in, from a step up (in
	// the air, a little over the feet) to past the reach. Its rounded bottom sits a radius under the band, so a face at
	// the band's foot is met square rather than on the probe's curve, but never lower than half a step over the feet: a
	// lip in the floor ahead (which the movement steps over) isn't the wall, and in the air just off the ground the probe
	// would start in it.
	const double MinTop = FeetZ + (Ask.bInAir ? MinAirHeight : Move->MaxStepHeight + 1.f);
	const double Reach = Ask.bInAir ? AirReach + MantleReachPerSpeed * Speed
		: (Ask.bRunning ? VaultReach + VaultReachPerSpeed * Speed : MantleReach + MantleReachPerSpeed * Speed);
	const float ProbeRadius = Query.Radius - 2.f;
	const double BandBottom = FMath::Max(MinTop - ProbeRadius, FeetZ + 0.5 * Move->MaxStepHeight);
	const double BandTop = FeetZ + MaxMantleHeight + 10.0;
	const FVector ProbeFrom = At(Center, 0.5 * (BandBottom + BandTop));
	const FCollisionShape Probe = FCollisionShape::MakeCapsule(ProbeRadius, FMath::Max(static_cast<float>(0.5 * (BandTop - BandBottom)), ProbeRadius));
	if (!Query.Sweep(Out.WallHit, ProbeFrom, ProbeFrom + Heading * Reach, Probe))
	{
		Out.Refusal = ETraversalRefusal::NoWall;
		return Out;
	}
	const FHitResult& Wall = Out.WallHit;
	if (Wall.bStartPenetrating)
	{
		Out.Refusal = ETraversalRefusal::NoRoom;
		return Out;
	}
	if (!IsClimbable(Wall, Character))
	{
		Out.Refusal = ETraversalRefusal::NotClimbable;
		return Out;
	}
	if (Move->IsWalkable(Wall))
	{
		Out.Refusal = ETraversalRefusal::Walkable;
		return Out;
	}
	FVector Into = Flat(-Wall.ImpactNormal);
	if (Into.IsNearlyZero())
	{
		Into = Flat(-Wall.Normal);
	}
	const double HeadingIn = Heading | Into;
	if (Into.IsNearlyZero() || HeadingIn < FMath::Cos(FMath::DegreesToRadians(HeadingDegrees))
		|| (Facing | Into) < FMath::Cos(FMath::DegreesToRadians(LookDegrees)))
	{
		Out.Refusal = ETraversalRefusal::NotFacing;
		return Out;
	}
	// The face's plane: how far ahead of the body's axis it stands, straight in, and along the way the player goes.
	const double FaceDistance = (Wall.ImpactPoint - Center) | Into;
	if (FaceDistance <= 0.0)
	{
		Out.Refusal = ETraversalRefusal::NoWall;
		return Out;
	}
	const double NearAlong = FaceDistance / HeadingIn;

	// 2. The top: its height, and how deep it runs.
	const FTopScan Scan = ScanTop(Query, Character, Center, Heading, NearAlong, FeetZ, MinTop);
	if (Scan.Refusal != ETraversalRefusal::None)
	{
		Out.Refusal = Scan.Refusal;
		return Out;
	}
	const double Height = Scan.TopZ - FeetZ;
	// Under a step, or (in the air) under what the jump still rising will clear by itself: left to the movement, so a
	// jump onto a crate stays a jump.
	if (Scan.TopZ < MinTop || (Ask.bInAir && Height < Ask.RiseLeft - 5.0))
	{
		Out.Refusal = ETraversalRefusal::TooLow;
		return Out;
	}
	// In the air the reach counts from the ground the jump left, so a jump never makes a wall climbable that a stand
	// can't reach (the user's 2 m wall).
	if (Height > MaxMantleHeight || (Ask.bInAir && Scan.TopZ - Ask.LastFloorZ > MaxMantleHeight))
	{
		Out.Refusal = ETraversalRefusal::TooHigh;
		return Out;
	}
	Out.TopZ = static_cast<float>(Scan.TopZ);
	Out.TopHit = Scan.TopHit;

	// 3. Which move: over a low thin thing at a run (or over what can't be stood on at all), else up onto the top; a low
	// wall that can't be climbed onto (no room on top) is still vaulted at a walk.
	const FContext Context{ Ask, Query, *Move, Character, Center, FeetZ, Velocity, Scan };
	const bool bThin = !Scan.bDeep && Scan.Depth < MinStandDepth;
	const bool bVaultable = !Scan.bDeep && Scan.Depth <= MaxVaultDepth && Height <= MaxVaultHeight;
	if (bVaultable && (Ask.bRunning || bThin) && TryVault(Context, Heading, NearAlong, Out))
	{
		return Out;
	}
	if (!bThin && TryMantle(Context, Into, FaceDistance, HeadingIn, Out))
	{
		return Out;
	}
	if (bVaultable && !Ask.bRunning && !bThin)
	{
		TryVault(Context, Heading, NearAlong, Out);
	}
	return Out;
}
