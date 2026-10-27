#include "World/SwimWaterSubsystem.h"
#include "World/SwimWater.h"
#include "Engine/World.h"

USwimWaterSubsystem* USwimWaterSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<USwimWaterSubsystem>() : nullptr;
}

bool USwimWaterSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// The game's worlds and the tests' preview worlds (Looter.Swim.Play spawns its water there).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void USwimWaterSubsystem::Register(ASwimWater* Water)
{
	if (Water)
	{
		Waters.AddUnique(Water);
	}
}

void USwimWaterSubsystem::Unregister(ASwimWater* Water)
{
	Waters.RemoveAll([Water](const TWeakObjectPtr<ASwimWater>& Known) { return !Known.IsValid() || Known.Get() == Water; });
}

ASwimWater* USwimWaterSubsystem::FindAt(const FVector& Point, float Above, float Below) const
{
	const FVector2D Flat(Point.X, Point.Y);
	for (const TWeakObjectPtr<ASwimWater>& Known : Waters)
	{
		ASwimWater* Water = Known.Get();
		if (!Water)
		{
			continue;
		}
		const float Rise = static_cast<float>(Point.Z) - Water->GetSurfaceZ();
		if (Rise <= Above && Rise >= -Below && Water->ContainsXY(Flat))
		{
			return Water;
		}
	}
	return nullptr;
}

bool USwimWaterSubsystem::IsDeepAt(const UObject* WorldContext, const FVector& Point)
{
	// Asked five times a second by every creature hunting or prowling: straight to the level's few waters.
	const USwimWaterSubsystem* Self = Get(WorldContext);
	if (!Self || Self->Waters.IsEmpty())
	{
		return false;
	}
	const FVector2D Flat(Point.X, Point.Y);
	for (const TWeakObjectPtr<ASwimWater>& Known : Self->Waters)
	{
		const ASwimWater* Water = Known.Get();
		if (!Water)
		{
			continue;
		}
		const float Rise = static_cast<float>(Point.Z) - Water->GetSurfaceZ();
		if (Rise <= DeepReachAbove && Rise >= -DeepReachBelow && Water->IsDeepXY(Flat))
		{
			return true;
		}
	}
	return false;
}
