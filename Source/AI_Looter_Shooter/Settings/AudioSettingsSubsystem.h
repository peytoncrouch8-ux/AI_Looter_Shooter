#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundRules.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "AudioSettingsSubsystem.generated.h"

/** One of the volume sliders in Settings > Audio. */
enum class EAudioVolume : uint8
{
	Master,
	Effects,
	Interface,
	Music,
};

UCLASS()
class AI_LOOTER_SHOOTER_API ULooterAudioSave : public USaveGame
{
	GENERATED_BODY()

public:
	/** Each slider's share, 0 (silent) to 1 (the sounds as they were made). */
	UPROPERTY()
	float Master = 1.f;

	UPROPERTY()
	float Effects = 1.f;

	UPROPERTY()
	float Interface = 1.f;

	UPROPERTY()
	float Music = 1.f;
};

/**
 * The player's sound options from the settings menu's Audio section, saved to the "AudioSettings" slot: the Master,
 * Effects, Interface and Music volumes. They reach the game's sound classes through ULooterSoundSubsystem, at once as a
 * slider moves (heard while it's dragged) and again as every level starts; the world's ambience follows Effects.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UAudioSettingsSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Every slider moves in 5% steps and starts full. */
	static constexpr float VolumeStep = 0.05f;
	static constexpr float DefaultVolume = 1.f;

	/** Rounded to the slider's steps and kept within 0-1; anything that isn't a number is the default. */
	static float ClampVolume(float Share);

	/** Slot's saved volumes, or a fresh set at the defaults if it holds none. */
	static ULooterAudioSave* LoadAudio(const FString& Slot, UObject* Outer);

	/** A save's volumes, clamped (a hand-edited save can't go past full); the defaults without one. */
	static FLooterVolumes VolumesOf(const ULooterAudioSave* Audio);

	/** Sets one slider's share in a save, clamped. */
	static void SetVolumeOf(ULooterAudioSave& Audio, EAudioVolume Which, float Share);

	/** "Master", "Effects", "Interface", "Music": the slider's label. */
	static FString VolumeName(EAudioVolume Which);

	FLooterVolumes GetVolumes() const;
	float GetVolume(EAudioVolume Which) const;

	/** Heard at once. bSave writes it to disk; a slider being dragged passes false and saves on release. */
	void SetVolume(EAudioVolume Which, float Share, bool bSave = true);

	void SaveSettings() const;

	/** Hands the volumes to the sound system (ULooterSoundSubsystem), which sets them on the sound classes. */
	void Apply() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULooterAudioSave> SaveData;
};
