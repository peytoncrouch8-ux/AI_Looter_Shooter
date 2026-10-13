#pragma once

#include "CoreMinimal.h"
#include "Scenes/SceneSubsystem.h"

class AActor;
class AOreLift;
struct FOreLiftRunShape;

/**
 * A ride on the ore lift (Docs/Areas/RansomsRest_Expansion.md, section 6 "The ore lift"), as a scene like the skiff ride:
 * carried, look only, skippable (Looter.Scene.Skip, holding Interact, Escape twice). The rider steps into the cage and
 * turns to face out of it over a second (the boarding), the winch takes the strain, the cage climbs (or drops) about 21 s
 * along its cable, swinging on its grip and clanking over the tower, and is set down on the far landing.
 *
 * The rider's own body waits unseen on the far landing from the start (AOreLift::ParkRider): what rides is the view, the
 * scene's camera at the rider's eye in the cage, turned by the player's look and tipped with the cage's swing by a share
 * (a rider braced on the rail). At the end the cage rests where the body stands, the camera is at the body's own eye,
 * and the view goes back to it without a seam. So whatever happens mid-ride (a save, a quit, a death) finds the player on
 * the far landing, never hanging over the ridge face, and a skip lands exactly where the ride would have ended.
 *
 * The first ride is a set piece (Hiram's "Hold the rail.", the quarry dropping away, the valley opening south behind the
 * cage at 6-12 s, the clank over the tower, the Bench opening ahead over the crest): the boarding turns the rider to face
 * out of the cage over the valley.
 */
namespace OreLiftRide
{
	/** The scene ("OreLift": the missions hear Scene.OreLift), and its moments: the cage leaving its landing, passing the
	 *  tower, set down on the far landing. */
	AI_LOOTER_SHOOTER_API FName SceneName();
	AI_LOOTER_SHOOTER_API FName DepartMoment();
	AI_LOOTER_SHOOTER_API FName TowerMoment();
	AI_LOOTER_SHOOTER_API FName ArriveMoment();

	/** Stepping into the cage and turning to face out of it, before the winch takes the strain (s). */
	inline constexpr float BoardSeconds = 1.f;

	/** Where the boarding turns the rider's look: a little down, out over the landing (degrees of pitch). */
	inline constexpr float BoardPitch = -6.f;

	/** The share of the cage's swing the view takes (a rider braced on the rail sways less than the cage). */
	inline constexpr float ViewSwayShare = 0.5f;

	/** How much a jolt lifts and drops the view (cm per degree of its roll). */
	inline constexpr float JoltLift = 0.6f;

	/** A rider's eyes over their feet when their own first-person camera can't say (cm; the player's are about 140). */
	inline constexpr float FallbackEyeHeight = 140.f;

	/** The whole ride with its boarding (s): about 24 for the lift's ride. */
	AI_LOOTER_SHOOTER_API float DurationFor(const FOreLiftRunShape& Run);

	/**
	 * The ride as a scene for USceneSubsystem::Play: Lift's run under way (AOreLift::Ride began it), Rider carried from its
	 * landing to the other. bFirstRide adds the first ride's lines as it starts. AfterEnd lets the rider go
	 * (AOreLift::FinishRide).
	 */
	AI_LOOTER_SHOOTER_API FScenePlay Make(USceneSubsystem& Scenes, AOreLift& Lift, AActor& Rider, bool bFirstRide);
}
