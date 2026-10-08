#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundCues.h"

class AActor;
class UAudioComponent;
class USceneComponent;

/**
 * The game's sounds by cue (LooterSoundCues.h names them). Each cue's sounds and variations, its volume and pitch jitter,
 * its sound class (the volume sliders) and its attenuation live in the sound bank (/Game/Audio/DA_SoundBank, filled by
 * Tools/Unreal/build_sound_bank.py from Art/Sounds/cues.json). A cue with no sounds yet plays nothing and warns once, so
 * code calls any cue before its sound exists. Nothing plays without a world that can play audio (tests, servers).
 */
namespace LooterSound
{
	/** One shot in the world at a place (3D, with the cue's attenuation). */
	AI_LOOTER_SHOOTER_API void PlayAt(const UObject* WorldContext, FName Cue, const FVector& Location, float VolumeScale = 1.f,
		float PitchScale = 1.f);

	/** One shot that follows a component as it plays (a gun in hand, a creature's body). */
	AI_LOOTER_SHOOTER_API void PlayAttached(FName Cue, USceneComponent* AttachTo, FName Socket = NAME_None, float VolumeScale = 1.f,
		float PitchScale = 1.f);

	/** One shot with no place (the interface, the player's own feedback). */
	AI_LOOTER_SHOOTER_API void Play2D(const UObject* WorldContext, FName Cue, float VolumeScale = 1.f, float PitchScale = 1.f);

	/**
	 * A sound the caller keeps and stops (a loop: the slide's scrape, the low-health heartbeat): attached to a component,
	 * or 2D when AttachTo is null. Null when the cue has no sound; hold it weakly (TWeakObjectPtr), as it goes once stopped.
	 */
	AI_LOOTER_SHOOTER_API UAudioComponent* Start(const UObject* WorldContext, FName Cue, USceneComponent* AttachTo = nullptr,
		FName Socket = NAME_None, float VolumeScale = 1.f, float PitchScale = 1.f);

	/** Fades out and lets go of a sound Start returned (null is fine). */
	AI_LOOTER_SHOOTER_API void Stop(UAudioComponent* Sound, float FadeSeconds = 0.1f);
}
