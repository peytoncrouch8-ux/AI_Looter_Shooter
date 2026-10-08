#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundRules.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LooterSoundSubsystem.generated.h"

class UAudioComponent;
class ULooterSoundBank;
class USceneComponent;
class USoundClass;
class USoundConcurrency;
class USoundMix;
class USoundWave;
class UWorld;
struct FActorsInitializedParams;
struct FLooterSoundCueEntry;

/**
 * Plays the game's sound cues; LooterSound.h is the way in. It loads the sound bank (/Game/Audio/DA_SoundBank) once per
 * game and keeps, for each cue, the variation it played last (so the next is another), the cue's gain and its concurrency:
 * at most its MaxConcurrent at once, one more stopping the quietest, and the same cue never starting twice within
 * LooterSoundRules::RetriggerSeconds. It does nothing between plays: no tick, no timers.
 *
 * The volume sliders (UAudioSettingsSubsystem) reach the sound classes through one sound mix (/Game/Audio/Mix/SMX_Volumes)
 * whose class volumes are set as they change, and again as every world of this game initializes (play-in-editor gives
 * each session its own audio device).
 *
 * Only a game's own worlds play sounds (the game and play-in-editor, CanPlayIn): the tests' preview worlds, editor
 * viewports and servers stay silent, and a cue the bank doesn't have plays nothing and warns once.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULooterSoundSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** The mix and the classes under it, made by Tools/Unreal/build_sound_bank.py. */
	static const TCHAR* MixPath;
	static const TCHAR* MasterClassPath;
	static const TCHAR* EffectsClassPath;
	static const TCHAR* InterfaceClassPath;
	static const TCHAR* AmbienceClassPath;
	static const TCHAR* MusicClassPath;

	/** Whether World plays sounds: a game or play-in-editor world that allows audio, with an audio device, and no server. */
	static bool CanPlayIn(const UWorld* World);

	/** The subsystem of WorldContext's game when its world can play sounds, else null. */
	static ULooterSoundSubsystem* Find(const UObject* WorldContext);

	// --- Playing (LooterSound.h's functions; the bank decides 2D or 3D, so a 2D cue played at a place is heard flat) ---

	void PlayAt(const UObject* WorldContext, FName Cue, const FVector& Location, float VolumeScale, float PitchScale);
	void PlayAttached(FName Cue, USceneComponent& AttachTo, FName Socket, float VolumeScale, float PitchScale);
	void Play2D(const UObject* WorldContext, FName Cue, float VolumeScale, float PitchScale);
	UAudioComponent* Start(const UObject* WorldContext, FName Cue, USceneComponent* AttachTo, FName Socket, float VolumeScale,
		float PitchScale);

	/** What one play of a cue takes: the sound picked, its cue's settings and concurrency, and its volume and pitch. */
	struct FPlay
	{
		const FLooterSoundCueEntry* Entry = nullptr;
		USoundWave* Sound = nullptr;
		USoundConcurrency* Concurrency = nullptr;
		float Volume = 1.f;
		float Pitch = 1.f;

		bool IsValid() const { return Entry && Sound; }
	};

	/**
	 * Picks Cue's next sound (never the one it played last, when it has another) with its gain and jittered pitch on top
	 * of the scales. Invalid when the cue has no sounds: it warns once per cue (unless there's no bank at all, which
	 * warned as the game began).
	 */
	FPlay Prepare(FName Cue, float VolumeScale = 1.f, float PitchScale = 1.f);

	// --- The bank ---

	/** Plays from InBank from now on (the game loads its own as it starts; tests give theirs). */
	void SetBank(ULooterSoundBank* InBank);
	ULooterSoundBank* GetBank() const { return Bank; }

	/** Whether Cue has at least one sound to play. */
	bool HasSounds(FName Cue) const;

	/** The concurrency the cue plays under (its limit, the quietest stolen), or null when the bank has no such cue. */
	const USoundConcurrency* GetConcurrency(FName Cue) const;

	// --- Volumes ---

	/** The sliders' volumes, applied to the sound classes at once and to every world of this game as it starts. */
	void SetVolumes(const FLooterVolumes& InVolumes);
	const FLooterVolumes& GetVolumes() const { return Volumes; }

private:
	void HandleActorsInitialized(const FActorsInitializedParams& Params);
	void ApplyVolumesTo(UWorld* World);
	void LoadMix();
	void WarnOnce(FName Cue, const TCHAR* Why);
	/** A one-shot function was handed a loop, which would never stop: it plays nothing and says so once. */
	bool RefuseLoop(const FPlay& Play);

	UPROPERTY(Transient)
	TObjectPtr<ULooterSoundBank> Bank;

	/** Per bank cue, in the bank's order: its concurrency, the variation it played last, its gain. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundConcurrency>> Concurrencies;
	TArray<int32> LastVariation;
	TArray<float> CueGains;
	TMap<FName, int32> CueIndex;

	/** Cues already warned about. */
	TSet<FName> WarnedCues;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> Mix;
	UPROPERTY(Transient)
	TObjectPtr<USoundClass> MasterClass;
	UPROPERTY(Transient)
	TObjectPtr<USoundClass> EffectsClass;
	UPROPERTY(Transient)
	TObjectPtr<USoundClass> InterfaceClass;
	UPROPERTY(Transient)
	TObjectPtr<USoundClass> AmbienceClass;
	UPROPERTY(Transient)
	TObjectPtr<USoundClass> MusicClass;

	FLooterVolumes Volumes;
	/** The audio devices the mix is on (it goes on once per device; the volumes then change it in place). */
	TSet<uint32> MixedDevices;
	FDelegateHandle ActorsInitializedHandle;
};
