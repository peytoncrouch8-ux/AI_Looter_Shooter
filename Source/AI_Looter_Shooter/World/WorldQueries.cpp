#include "World/WorldQueries.h"
#include "World/GroundIgnoreSubsystem.h"
#include "Engine/World.h"

FCollisionQueryParams LooterWorld::StaticGeometryParams(const UWorld* World, FName TraceTag, const AActor* Ignored, bool bTraceComplex)
{
	FCollisionQueryParams Params(TraceTag, bTraceComplex, Ignored);
	if (World)
	{
		// Creatures build these ten times a second: played worlds keep the volumes found (UGroundIgnoreSubsystem), the editor's
		// own world, which has no such list, looks through its actors each time.
		if (UGroundIgnoreSubsystem* Cache = World->GetSubsystem<UGroundIgnoreSubsystem>())
		{
			Cache->AddIgnoredTo(Params);
		}
		else
		{
			UGroundIgnoreSubsystem::AddIgnoredByWalking(*World, Params);
		}
	}
	return Params;
}
