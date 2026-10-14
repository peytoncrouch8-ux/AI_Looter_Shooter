#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/RoostRules.h"
#include "World/RoostVeil.h"
#include "RoostCarrySubsystem.generated.h"

class APlayerController;
class UHudCrowVeilWidget;

/**
 * Hob's crows carrying Ellis (the user's call, 2026-10-13): a fast travel between roosts and a death's drag back to the
 * roost last found (or, before any, the level's start: ARespawnMarker::ChooseWakeSpot), as the player sees and hears it. It plays the crows over the screen (FRoostVeil, drawn by UHudCrowVeilWidget
 * over the local player's view, made the first time a carry plays), their sounds, and Hob's word as they set Ellis down.
 *  - Fast travel (UGraveTravelSubsystem): BeginTravel as the screen goes, the flock pouring up across it under the rush of
 *    wings (RoostMark::CarryOut); Reveal once the player stands at the roost, the crows bursting away.
 *  - A death on a map with roosts (UPlayerVitalsSubsystem): BeginDeath, crows gathering at the screen's edges round the
 *    dying, then dragging the screen to black (RoostMark::DeathDrag); Reveal at the wake-up.
 *  - SayOnArrival: Hob after every death (his lines in a shuffled round; the first death's own once in a story), and after
 *    the first ride; only where the story is played (Hob is never on Skyreach, which has no roosts anyway).
 * The camera's own fade still runs under the crows, so the screen goes black with or without the overlay (a test level has
 * none). It ticks only while the crows are on the screen.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API URoostCarrySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static URoostCarrySubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** A fast travel leaving: the crows fill Viewer's screen over CoverSeconds (the travel's fade), with their rush of wings. */
	void BeginTravel(APlayerController* Viewer, float CoverSeconds);

	/**
	 * A death on a map with roosts: crows gather round the dying, then drag the screen black by CoverSeconds. A carry already
	 * covering (killed in a fast travel's black) stays covered.
	 */
	void BeginDeath(APlayerController* Viewer, float CoverSeconds);

	/** The player is at the roost: the crows burst off the screen and the world comes back. */
	void Reveal();

	/** Moves the crows on by DeltaSeconds (the tick; a test level never ticks, so the tests call it). */
	void Advance(float DeltaSeconds);

	/** The crows over the screen now (the tests read it). */
	const FRoostVeil& GetVeil() const { return Veil; }

	/**
	 * Hob's word as his crows set Ellis down (captions, after whatever is being said): after a death every time (the first
	 * death's own line once in a story, then his lines in a shuffled round, never the last one again), and the first ride
	 * between roosts. Nothing where no story is played. Returns the line said, or null.
	 */
	const FStoryLine* SayOnArrival(ERoostArrival Arrival);

	/** How many carries have played in this world (the tests). */
	int32 GetCarries() const { return Carries; }

private:
	void Begin(APlayerController* InViewer, ERoostVeilKind Kind, float CoverSeconds);
	/** Hands the veil to the overlay, made the first time it's wanted (a test level has nobody to show it to). */
	void ShowVeil();
	UHudCrowVeilWidget* FindOrMakeOverlay();

	FRoostVeil Veil;
	TWeakObjectPtr<APlayerController> Viewer;

	UPROPERTY(Transient)
	TObjectPtr<UHudCrowVeilWidget> Overlay;

	/** Hob's round of death lines, and the one said last. */
	TArray<int32> LineOrder;
	int32 LastLine = INDEX_NONE;
	FRandomStream LineRandom;
	/** The line said last (SayOnArrival's answer). */
	FStoryLine LastSaid;
	int32 Carries = 0;
};
