#include "World/FallRecoveryTracker.h"
#include "World/PlayableBoundary.h"

TOptional<EFallRecoveryReason> FFallRecoveryTracker::Update(float DeltaTime, const FVector& Location, const FRotator& ViewRotation,
	bool bOnWalkableGround, const FPlayableBoundary* Boundary, const FRules& Rules)
{
	const FVector2D Flat(Location.X, Location.Y);
	const bool bArea = Boundary && Boundary->IsValid();
	const bool bInside = !bArea || Boundary->Contains(Flat);

	// Which edge the player left the area over: the one between where they last were inside and where they are now,
	// taken on the frame they go out (a frame's step is short, so it's the edge they really crossed).
	if (bInside)
	{
		LastInside = Flat;
		bHasLastInside = true;
		bOutside = false;
		ExitEdge = INDEX_NONE;
	}
	else if (!bOutside)
	{
		bOutside = true;
		ExitEdge = INDEX_NONE;
		if (bHasLastInside)
		{
			ExitEdge = Boundary->FindCrossedEdge(LastInside, Flat);
			if (ExitEdge == INDEX_NONE)
			{
				ExitEdge = Boundary->FindNearestEdge(Flat);
			}
		}
	}

	// Take the safe spot while standing on walkable ground, and only inside the area, a few times a second, so it's
	// recent but not the exact frame the player stepped off an edge.
	if (bOnWalkableGround && bInside)
	{
		SampleTimer -= DeltaTime;
		if (SampleTimer <= 0.f || !bHasSafeSpot)
		{
			SafeLocation = Location;
			SafeRotation = ViewRotation;
			bHasSafeSpot = true;
			SampleTimer = Rules.SampleInterval;
		}
		return {};
	}
	if (!bHasSafeSpot)
	{
		return {};
	}

	const double Drop = SafeLocation.Z - Location.Z;
	// Off an open edge a short drop is enough: past the Rim or the deck there's nothing to land on, and the sooner the
	// player is back, the less the fall costs them.
	if (bOutside && Drop > Rules.OpenEdgeDrop && LeftOverOpenEdge(*Boundary, Flat))
	{
		return EFallRecoveryReason::OffOpenEdge;
	}
	// Anywhere else only a long fall: off a floating island, or as the backstop on the ground.
	if (Drop > Rules.LongDrop)
	{
		return EFallRecoveryReason::LongFall;
	}
	return {};
}

void FFallRecoveryTracker::MarkRecovered()
{
	// Back on the safe spot, which is inside.
	LastInside = FVector2D(SafeLocation.X, SafeLocation.Y);
	bHasLastInside = true;
	bOutside = false;
	ExitEdge = INDEX_NONE;
}

bool FFallRecoveryTracker::LeftOverOpenEdge(const FPlayableBoundary& Boundary, const FVector2D& Where) const
{
	return Boundary.IsOpen(ExitEdge) || Boundary.IsOpen(Boundary.FindNearestEdge(Where));
}
