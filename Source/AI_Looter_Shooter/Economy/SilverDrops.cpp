#include "Economy/SilverDrops.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Economy/EconomyRules.h"
#include "Economy/SilverPickup.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"

bool SilverDrops::DropsAt(const UObject* WorldContextObject)
{
	return UAreaRulesSubsystem::DropsGunsAt(WorldContextObject);
}

ASilverPickup* SilverDrops::Spill(UWorld* World, int32 Amount, const FVector& From, const FVector& Velocity)
{
	ASilverPickup* Pickup = ASilverPickup::SpawnSilver(World, Amount, From);
	if (Pickup)
	{
		Pickup->Toss(Velocity);
	}
	return Pickup;
}

ASilverPickup* SilverDrops::DropForKill(const AActor& Body, ECreatureRank Rank, bool bUnpaid, int32 Level, FRandomStream& Random)
{
	// Rolled even where nothing drops, so the stream's later numbers don't depend on where the kill was.
	const int32 Amount = LooterEconomy::Roll(LooterEconomy::KillOdds(Rank, bUnpaid), Level, Random);
	const FVector Outward = FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f).Vector();
	const FVector Velocity = Outward * Random.FRandRange(120.f, 260.f) + FVector(0.f, 0.f, Random.FRandRange(320.f, 460.f));
	if (Amount <= 0 || !DropsAt(&Body))
	{
		return nullptr;
	}
	return Spill(Body.GetWorld(), Amount, Body.GetActorLocation() + FVector(0.f, 0.f, 50.f), Velocity);
}

int32 SilverDrops::RollForContainer(const UObject* WorldContextObject, EChestKind Kind, int32 Level, FRandomStream& Random)
{
	const int32 Amount = LooterEconomy::Roll(LooterEconomy::ContainerOdds(Kind), Level, Random);
	return DropsAt(WorldContextObject) ? Amount : 0;
}

ASilverPickup* SilverDrops::DropForBreakable(UWorld* World, int32 Level, const FVector& From, FRandomStream& Random)
{
	const int32 Amount = LooterEconomy::Roll(LooterEconomy::BreakableOdds(), Level, Random);
	const FVector Outward = FRotator(0.f, Random.FRandRange(0.f, 360.f), 0.f).Vector();
	const FVector Velocity = Outward * Random.FRandRange(80.f, 180.f) + FVector(0.f, 0.f, Random.FRandRange(260.f, 380.f));
	if (Amount <= 0 || !World || !DropsAt(World))
	{
		return nullptr;
	}
	return Spill(World, Amount, From, Velocity);
}
