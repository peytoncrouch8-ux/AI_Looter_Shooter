#pragma once

#include "CoreMinimal.h"
#include "Scenes/SceneSubsystem.h"

class AActor;
class ALandfallSpyglass;

/**
 * Landfall's reveal through the spyglass on Ransom's Point (Main 1 step 6): what Vane has done to the Rest. The view goes
 * into the glass (a dip, a long lens): Vane's dredge barge hanging over the Sink, held a moment; down its hose into the
 * Sink's floor where it drinks the dead; up the soul smog pouring off it. Vane gloats over the valley's loudspeakers
 * (DA_Lines_LandfallVaneValley, through UVaneBroadcast) and the scene waits for him to finish; then out of the glass, and
 * Finch on the tube: the Lantern has Abel's trail (DA_Lines_LandfallTrail), the trail glowing down the bluff. A later look
 * (bFull false) is the quiet one: the barge and its hose, no words. Skippable: a skip cuts Vane off.
 */
namespace SmogReveal
{
	/** "SmogReveal": the missions hear Scene.SmogReveal (nothing waits for it: the step is the spyglass's use). */
	AI_LOOTER_SHOOTER_API FName SceneName();

	/** The shots (seconds): on the barge; down the hose; up the smog; where it waits for Vane; out of the glass. */
	inline constexpr float BargeSeconds = 3.5f;
	inline constexpr float HoseSeconds = 5.5f;
	inline constexpr float SmogSeconds = 4.f;
	inline constexpr float WaitAt = BargeSeconds + HoseSeconds + SmogSeconds;
	inline constexpr float OutSeconds = 0.8f;
	/** Vane begins this far in; the scene waits for him at most this long in all. */
	inline constexpr float BroadcastAt = 1.f;
	inline constexpr float LongestWait = 45.f;
	/** The glass's field of view: a long lens closing in, then easing out (degrees). */
	inline constexpr float WideFov = 18.f;
	inline constexpr float CloseFov = 10.f;
	/** How high over the barge its smog is looked up to, and how far below it the hose's foot is when no ground is found (cm). */
	inline constexpr float SmogAbove = 3500.f;
	inline constexpr float HoseFallback = 4500.f;

	/** The scene's length when nothing waits: the full reveal, or the quiet look. */
	AI_LOOTER_SHOOTER_API float DurationFor(bool bFull);

	/** Where the barge's hose meets the Sink's floor: the ground under the barge (traced), or HoseFallback below it. */
	AI_LOOTER_SHOOTER_API FVector HoseFootUnder(const UWorld* World, const AActor& Barge);

	/** The scene for USceneSubsystem::Play. Barge may be null (WORLD's barge not placed yet): it looks where the glass points. */
	AI_LOOTER_SHOOTER_API FScenePlay Make(USceneSubsystem& Scenes, ALandfallSpyglass& Spyglass, AActor* Barge, bool bFull);
}
