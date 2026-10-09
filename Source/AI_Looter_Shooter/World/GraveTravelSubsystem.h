#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/GraveTravelRules.h"
#include "GraveTravelSubsystem.generated.h"

class APawn;
class ARespawnMarker;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGraveTravelled, APawn* /*Player*/, const ARespawnMarker& /*Grave*/);

/**
 * Fast travel between the open respawn graves of the level being played (the inventory's map page asks for it): the
 * screen fades to black under a rush of grave-wind (UI.TravelWhoosh), the player is put at the grave as a death's
 * wake-up puts them (on its spot, facing its arrow), and the screen fades back. Never in a fight (a creature hunting the
 * player), a scene or a boss's fight, nor while a trip is under way (GraveTravelRules). The autosave waits through the
 * fade; the travel saves nothing of its own: where the player stands goes into the session as it always does. Graves
 * open with the story (ARespawnMarker), so the session already keeps which ones the player may travel to.
 *
 * The fade runs on the subsystem's own clock (Advance, from its tick; it pauses with the game), not a timer: a timer set
 * before the timer manager has ticked that frame only starts on its next tick, which a test level never gives it.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGraveTravelSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UGraveTravelSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGraveTravelSubsystem, STATGROUP_Tickables); }

	/** Moves a travel under way on by DeltaSeconds (the tick does; a test level never ticks, so tests call it): the move comes once the black has held. */
	void Advance(float DeltaSeconds);

	/** What the rules look at for Player now: alive, in a scene, a boss's fight, who hunts them, a trip under way. */
	FGraveTravelSenses Sense(const APawn* Player) const;

	/** Whether Player may travel at all now (None), or why not. */
	EGraveTravelBlock CheckNow(const APawn* Player) const;

	/** Whether Player may travel to Grave now (None), or why not. */
	EGraveTravelBlock CheckGrave(const APawn* Player, const ARespawnMarker& Grave) const;

	/** The grave is open in the story being played (the mission runner's campaign record; without one, open from the start). */
	bool IsOpen(const ARespawnMarker& Grave) const;

	/**
	 * Starts the travel to Grave: the fade out and its whoosh, then the move, then the fade in. False (with the reason in
	 * OutBlock) when the rules say no.
	 */
	bool TravelTo(APawn* Player, const ARespawnMarker& Grave, EGraveTravelBlock* OutBlock = nullptr);

	/** A travel is fading out or holding the black. */
	bool IsTravelling() const { return bTravelling; }

	/** Puts Player at Grave's wake spot at once (the travel's move; tests): standing on it, facing its arrow, stopped. */
	static void MoveToGrave(APawn& Player, const ARespawnMarker& Grave);

	/** A travel has put the player at a grave (the screen is fading back in). */
	FOnGraveTravelled OnTravelled;

private:
	/** The black is held: the player is moved, the screen comes back, the saves go on. */
	void Arrive();

	/** Gives the player back their keys and the screen, and lets the autosave run again. */
	void EndTravel(APawn* Player);

	TWeakObjectPtr<APawn> Traveller;
	TWeakObjectPtr<const ARespawnMarker> Destination;
	/** Seconds since the travel began fading out. */
	float TravelClock = 0.f;
	bool bTravelling = false;
};
