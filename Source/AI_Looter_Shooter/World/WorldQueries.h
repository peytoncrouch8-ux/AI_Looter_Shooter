#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"

class AActor;
class UWorld;

namespace LooterWorld
{
	/**
	 * Query params for traces that should see only real geometry among the world-static objects: every volume is
	 * skipped (a PCG volume, trigger or the like is an invisible box that still answers world-static queries, and
	 * the one around the meadow would otherwise be the first thing a downward trace hits), and so is Ignored.
	 */
	AI_LOOTER_SHOOTER_API FCollisionQueryParams StaticGeometryParams(const UWorld* World, FName TraceTag, const AActor* Ignored = nullptr,
		bool bTraceComplex = true);
}
