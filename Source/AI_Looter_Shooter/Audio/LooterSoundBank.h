#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LooterSoundBank.generated.h"

class USoundAttenuation;
class USoundClass;
class USoundWave;

/**
 * One cue in the sound bank: its sounds (variations; each play picks one, never the last one again) and how they play.
 * Written by Tools/Unreal/build_sound_bank.py from Art/Sounds/cues.json; edit the JSON and run the script again rather
 * than editing the bank.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FLooterSoundCueEntry
{
	GENERATED_BODY()

	/** The cue's name, as LooterSoundCues.h gives it ("Weapon.Rifle.Fire"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	FName Cue;

	/** Its variations (/Game/Audio/<First cue part>/S_<Cue_With_Underscores>_<NN>). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	TArray<TObjectPtr<USoundWave>> Sounds;

	/** Its loudness against the others, in decibels (0: as rendered). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	float VolumeDb = 0.f;

	/** Each play's pitch moves up to this share either way (0.04: within 4%), so repeats never sound machine-made. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound", meta = (ClampMin = "0", ClampMax = "0.5"))
	float PitchJitter = 0.f;

	/** Its sound class (/Game/Audio/Mix/SC_Effects and the like): which volume slider it answers to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundClass> SoundClass;

	/** How it fades with distance (ATT_Gun, ATT_Creature, ATT_Near); none for a sound with no place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	TObjectPtr<USoundAttenuation> Attenuation;

	/** Plays until stopped (LooterSound::Start and Stop): the low-health heartbeat, the slide's scrape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	bool bLoop = false;

	/** Placed in the world (3D, with its attenuation); off: heard the same from anywhere (2D). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	bool bSpatial = true;

	/** Goes on over a paused game (the menus' sounds, the Interface class); everything else pauses with the game. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	bool bPlaysWhilePaused = false;

	/** At most this many play at once; one more stops the quietest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound", meta = (ClampMin = "1"))
	int32 MaxConcurrent = 4;
};

/**
 * Every sound cue the game plays (/Game/Audio/DA_SoundBank), loaded once per game by ULooterSoundSubsystem. A cue the
 * bank doesn't have plays nothing (and warns once), so code can play a cue before its sound exists.
 */
UCLASS(BlueprintType)
class AI_LOOTER_SHOOTER_API ULooterSoundBank : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Where the game finds it. */
	static const TCHAR* AssetPath;

	/** In cue order (the script sorts them). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sounds")
	TArray<FLooterSoundCueEntry> Cues;

	/** The cue's entry, or null. A plain search: the sound subsystem keeps its own index for playing. */
	const FLooterSoundCueEntry* FindCue(FName Cue) const;

	/** The bank at AssetPath, or null (quietly, with no engine warning) before build_sound_bank.py has made it. */
	static ULooterSoundBank* Load();
};
