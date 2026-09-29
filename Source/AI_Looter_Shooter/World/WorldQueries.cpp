#include "World/WorldQueries.h"
#include "EngineUtils.h"
#include "GameFramework/Volume.h"

FCollisionQueryParams LooterWorld::StaticGeometryParams(const UWorld* World, FName TraceTag, const AActor* Ignored, bool bTraceComplex)
{
	FCollisionQueryParams Params(TraceTag, bTraceComplex, Ignored);
	if (World)
	{
		for (TActorIterator<AVolume> It(World); It; ++It)
		{
			Params.AddIgnoredActor(*It);
		}
	}
	return Params;
}
