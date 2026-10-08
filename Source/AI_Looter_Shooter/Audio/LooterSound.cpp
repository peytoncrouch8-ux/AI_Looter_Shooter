#include "Audio/LooterSound.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"

void LooterSound::PlayAt(const UObject* WorldContext, FName Cue, const FVector& Location, float VolumeScale, float PitchScale)
{
	if (ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(WorldContext))
	{
		Sounds->PlayAt(WorldContext, Cue, Location, VolumeScale, PitchScale);
	}
}

void LooterSound::PlayAttached(FName Cue, USceneComponent* AttachTo, FName Socket, float VolumeScale, float PitchScale)
{
	if (ULooterSoundSubsystem* Sounds = AttachTo ? ULooterSoundSubsystem::Find(AttachTo) : nullptr)
	{
		Sounds->PlayAttached(Cue, *AttachTo, Socket, VolumeScale, PitchScale);
	}
}

void LooterSound::Play2D(const UObject* WorldContext, FName Cue, float VolumeScale, float PitchScale)
{
	if (ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(WorldContext))
	{
		Sounds->Play2D(WorldContext, Cue, VolumeScale, PitchScale);
	}
}

UAudioComponent* LooterSound::Start(const UObject* WorldContext, FName Cue, USceneComponent* AttachTo, FName Socket, float VolumeScale,
	float PitchScale)
{
	// What it follows knows the world as well as the caller does.
	const UObject* Context = WorldContext ? WorldContext : AttachTo;
	ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(Context);
	return Sounds ? Sounds->Start(Context, Cue, AttachTo, Socket, VolumeScale, PitchScale) : nullptr;
}

void LooterSound::Stop(UAudioComponent* Sound, float FadeSeconds)
{
	if (!IsValid(Sound))
	{
		return;
	}
	// It lets itself go once quiet (every sound Start makes destroys itself when it stops).
	if (FadeSeconds > 0.f)
	{
		Sound->FadeOut(FadeSeconds, 0.f);
	}
	else
	{
		Sound->Stop();
	}
}
