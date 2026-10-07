#pragma once

#include "CoreMinimal.h"
#include "Scenes/SceneSubsystem.h"

class ATrain;

/**
 * The train's two short shots (Docs/Areas/RansomsRest.md, step 9's choice: "a short shot, then a fade"; Stations and the
 * train), scenes with nothing in Sequencer, seen from the train's fixed camera on the platform (ATrain::ShotCamera):
 *  - the departure: the player has boarded Tilly's hearse car (hidden), the brakes come off and the train pulls out along
 *    its track toward the gap, gathering speed for about 4 s, and the screen fades to black. Then the trip goes (or, from
 *    the console, the train is back at the platform and the black lifts).
 *  - the arrival, the same in reverse: out of the black, the train backs in along the platform, braking, and stops where it
 *    stands parked; the view eases back to the player, standing at the hearse car's door, and they have control.
 * The train's course is plain math of time, so the tests can check it, and a skip lands where the shot would end. The
 * black is the player's camera fade (a level change starts the next level unfaded; the arrival puts it back at once).
 */
namespace TrainShots
{
	/** The scenes ("TrainDeparture", "TrainArrival": the missions hear Scene.<Name>) and their moments. */
	AI_LOOTER_SHOOTER_API FName DepartureName();
	AI_LOOTER_SHOOTER_API FName ArrivalName();
	AI_LOOTER_SHOOTER_API FName PullOutMoment();
	AI_LOOTER_SHOOTER_API FName BlackMoment();
	AI_LOOTER_SHOOTER_API FName StopMoment();
	AI_LOOTER_SHOOTER_API FName HandBackMoment();

	// --- The departure ---

	/** Seconds in: the brakes come off; the screen starts going black; it's black (the shot's end, and the trip). */
	inline constexpr float PullOutTime = 0.6f;
	inline constexpr float FadeOutStart = 3.4f;
	inline constexpr float DepartSeconds = 4.2f;

	/** How hard it pulls away (cm/s/s): about 10 m out, at a running pace, by the black. */
	inline constexpr float Acceleration = 160.f;

	/** The cut to the platform comes out of black this quickly as the shot begins. */
	inline constexpr float CutInSeconds = 0.35f;

	// --- The arrival ---

	/** Seconds in: out of the black; rolling in, braking, to its stop; the view eases back to the player; their control. */
	inline constexpr float FadeInSeconds = 0.8f;
	inline constexpr float RollInSeconds = 4.5f;
	inline constexpr float HandBackAt = RollInSeconds + 0.5f;
	inline constexpr float HandBackSeconds = 0.8f;
	inline constexpr float ArriveSeconds = HandBackAt + HandBackSeconds;

	/** How far out along its track the departing train is Seconds into the shot (cm): still until the brakes come off. */
	AI_LOOTER_SHOOTER_API float DepartDistanceAt(float Seconds);

	/** How far out along its track the arriving train still is Seconds into the shot (cm): 0 once it has stopped. */
	AI_LOOTER_SHOOTER_API float ArriveDistanceAt(float Seconds);

	/** How black the screen is Seconds into each shot: 0 to 1. */
	AI_LOOTER_SHOOTER_API float DepartBlackAt(float Seconds);
	AI_LOOTER_SHOOTER_API float ArriveBlackAt(float Seconds);

	/**
	 * The departure as a scene for USceneSubsystem::Play. AtBlack runs once it has ended, the screen black (played or
	 * skipped). bTripFollows: the player stays held for the trip (a scene that holds them for one), and if none comes they
	 * get the train back at the platform and the black lifted; else (the console) that happens as it ends.
	 */
	AI_LOOTER_SHOOTER_API FScenePlay MakeDeparture(USceneSubsystem& Scenes, ATrain& Train, TFunction<void()> AtBlack, bool bTripFollows);

	/**
	 * The arrival as a scene for USceneSubsystem::Play: the player is stood at Landing (where their feet go, and the way they
	 * face) unseen while the train backs in, and handed back there once it has stopped.
	 */
	AI_LOOTER_SHOOTER_API FScenePlay MakeArrival(USceneSubsystem& Scenes, ATrain& Train, const FTransform& Landing);

	/** The train back where it stands parked and the black lifted over FadeInSeconds: a departure no trip followed. */
	AI_LOOTER_SHOOTER_API void PutBack(USceneSubsystem& Scenes, ATrain& Train);
}
