#pragma once

#include "CoreMinimal.h"
#include "World/FaunaFlock.h"
#include "RoostCrows.generated.h"

/**
 * Hob's crows on one of his roosts (ARespawnMarker spawns them as play begins, on its perches): the ambient fauna's crows
 * (World/FaunaFlock: their model, their perched life and their flight), but Hob's, so they know Ellis. They let the player
 * walk right up to the roost (a tiny flee radius), still start at gunfire, and when the roost is found, or the crows have just
 * carried someone home, they burst off the perches, wheel once tight about the mast and settle again within a few seconds.
 * A found roost has a second, small flock of its own circling its beacon high over the basket (MakeCircling): with the
 * column, the form that tells a roost from a loot beam far off.
 * Updated by UFaunaSubsystem like every flock (nothing ticks on its own); passable; made in play only, never saved.
 */
UCLASS(NotPlaceable)
class AI_LOOTER_SHOOTER_API ARoostCrows : public AFaunaFlock
{
	GENERATED_BODY()

public:
	ARoostCrows();

	/** Its perches (world), from the roost's sockets: set before play begins (the roost spawns it deferred). */
	void SetRoostPerches(const TArray<FTransform>& InPerches);

	/**
	 * Makes it the crows circling a found roost's beacon instead (before play begins): Count of them round Beacon (world),
	 * wheeling a few metres to a dozen over it, never landing; hidden past the settings' CirclingCullDistance.
	 */
	void MakeCircling(const FVector& Beacon, int32 Count);

	/**
	 * Makes it the crows circling low over an unfound find instead (UFindSubsystem's tell; before play begins): Count of them
	 * on a tight, slow wheel Height over Over (world), rising and dipping a little, cawing now and then: low enough to read
	 * against the hills from 30-40 m, never a speck high in the sky. Hidden past the settings' CirclingCullDistance.
	 */
	void MakeFindCircling(const FVector& Over, int32 Count, float Height);

	/**
	 * Makes it one crow sitting at Perch (world) on an unfound find instead (before play begins): it lets the player come
	 * within a few metres, then goes up cawing and wheels off; back once all is calm, away over the hills if it isn't.
	 */
	void MakeFindPerch(const FTransform& Perch);

	/** The roost found: the crows lift off as its light catches, wheel once and settle. */
	void Lift(const FVector& From);

	/** Someone set down at the roost: the crows that brought them burst off it the same way. */
	void Scatter(const FVector& From);
};
