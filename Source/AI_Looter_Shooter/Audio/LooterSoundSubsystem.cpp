#include "Audio/LooterSoundSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSoundBank.h"
#include "AudioDeviceHandle.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundWave.h"
#include "UObject/UObjectGlobals.h"

const TCHAR* ULooterSoundSubsystem::MixPath = TEXT("/Game/Audio/Mix/SMX_Volumes.SMX_Volumes");
const TCHAR* ULooterSoundSubsystem::MasterClassPath = TEXT("/Game/Audio/Mix/SC_Master.SC_Master");
const TCHAR* ULooterSoundSubsystem::EffectsClassPath = TEXT("/Game/Audio/Mix/SC_Effects.SC_Effects");
const TCHAR* ULooterSoundSubsystem::InterfaceClassPath = TEXT("/Game/Audio/Mix/SC_Interface.SC_Interface");
const TCHAR* ULooterSoundSubsystem::AmbienceClassPath = TEXT("/Game/Audio/Mix/SC_Ambience.SC_Ambience");
const TCHAR* ULooterSoundSubsystem::MusicClassPath = TEXT("/Game/Audio/Mix/SC_Music.SC_Music");

namespace
{
	/** An asset by its object path, quietly null until the sound bank script has made it. */
	template <typename T>
	T* LoadIfMade(const TCHAR* Path)
	{
		const FString Package = FPackageName::ObjectPathToPackageName(FString(Path));
		return FPackageName::DoesPackageExist(Package) ? LoadObject<T>(nullptr, Path) : nullptr;
	}

	/** A cue heard flat: the same from anywhere, going on over a paused game only if its cue says so (the menus'). */
	void PlayFlat(const UObject* WorldContext, const ULooterSoundSubsystem::FPlay& Play)
	{
		UGameplayStatics::PlaySound2D(WorldContext, Play.Sound, Play.Volume, Play.Pitch, 0.f, Play.Concurrency, nullptr,
			Play.Entry->bPlaysWhilePaused);
	}
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool ULooterSoundSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// A dedicated server hears nothing.
	return Super::ShouldCreateSubsystem(Outer) && !IsRunningDedicatedServer();
}

void ULooterSoundSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SetBank(ULooterSoundBank::Load());
	if (!Bank)
	{
		UE_LOG(LogLooter, Warning, TEXT("No sound bank at %s yet: the game is silent until Tools/Unreal/build_sound_bank.py makes it."),
			ULooterSoundBank::AssetPath);
	}
	LoadMix();
	ActorsInitializedHandle = FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &ULooterSoundSubsystem::HandleActorsInitialized);
}

void ULooterSoundSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldInitializedActors.Remove(ActorsInitializedHandle);
	Super::Deinitialize();
}

bool ULooterSoundSubsystem::CanPlayIn(const UWorld* World)
{
	if (!World || !GEngine || !GEngine->UseSound())
	{
		return false;
	}
	// Only a game's own worlds: preview worlds (the tests', the editor's) would play through the editor's own device.
	const bool bGameWorld = World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE;
	return bGameWorld && World->bAllowAudioPlayback && World->GetNetMode() != NM_DedicatedServer && World->GetAudioDevice().IsValid();
}

ULooterSoundSubsystem* ULooterSoundSubsystem::Find(const UObject* WorldContext)
{
	const UWorld* World = GEngine && WorldContext ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!CanPlayIn(World))
	{
		return nullptr;
	}
	const UGameInstance* Game = World->GetGameInstance();
	return Game ? Game->GetSubsystem<ULooterSoundSubsystem>() : nullptr;
}

// ---------------------------------------------------------------------------
// The bank
// ---------------------------------------------------------------------------

void ULooterSoundSubsystem::SetBank(ULooterSoundBank* InBank)
{
	Bank = InBank;
	Concurrencies.Reset();
	LastVariation.Reset();
	CueGains.Reset();
	CueIndex.Reset();
	WarnedCues.Reset();
	if (!Bank)
	{
		return;
	}
	for (int32 Index = 0; Index < Bank->Cues.Num(); ++Index)
	{
		const FLooterSoundCueEntry& Entry = Bank->Cues[Index];
		if (CueIndex.Contains(Entry.Cue))
		{
			UE_LOG(LogLooter, Warning, TEXT("The sound bank has cue %s twice: the first plays."), *Entry.Cue.ToString());
		}
		else
		{
			CueIndex.Add(Entry.Cue, Index);
		}
		// One concurrency per cue groups its plays: its limit, the quietest stolen for a new one (a far gunshot gives way
		// to a near one), and a pellet volley's hits heard as one.
		USoundConcurrency* Concurrency = NewObject<USoundConcurrency>(this);
		Concurrency->Concurrency.SetMaxCount(FMath::Max(Entry.MaxConcurrent, 1));
		Concurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopQuietest;
		Concurrency->Concurrency.RetriggerTime = LooterSoundRules::RetriggerSeconds;
		Concurrency->Concurrency.bLimitToOwner = false;
		Concurrencies.Add(Concurrency);
		LastVariation.Add(INDEX_NONE);
		CueGains.Add(LooterSoundRules::DbToGain(Entry.VolumeDb));
	}
}

bool ULooterSoundSubsystem::HasSounds(FName Cue) const
{
	const int32* Index = CueIndex.Find(Cue);
	if (!Index || !Bank || !Bank->Cues.IsValidIndex(*Index))
	{
		return false;
	}
	return Bank->Cues[*Index].Sounds.ContainsByPredicate([](const TObjectPtr<USoundWave>& Sound) { return Sound != nullptr; });
}

const USoundConcurrency* ULooterSoundSubsystem::GetConcurrency(FName Cue) const
{
	const int32* Index = CueIndex.Find(Cue);
	return Index && Concurrencies.IsValidIndex(*Index) ? Concurrencies[*Index].Get() : nullptr;
}

ULooterSoundSubsystem::FPlay ULooterSoundSubsystem::Prepare(FName Cue, float VolumeScale, float PitchScale)
{
	FPlay Play;
	const int32* Index = CueIndex.Find(Cue);
	const FLooterSoundCueEntry* Entry = Index && Bank && Bank->Cues.IsValidIndex(*Index) ? &Bank->Cues[*Index] : nullptr;
	// Only the sounds that loaded count (a wave deleted since the bank was built leaves a gap).
	TArray<USoundWave*, TInlineAllocator<8>> Sounds;
	if (Entry)
	{
		for (const TObjectPtr<USoundWave>& Sound : Entry->Sounds)
		{
			if (Sound)
			{
				Sounds.Add(Sound.Get());
			}
		}
	}
	if (Sounds.IsEmpty())
	{
		// With no bank at all the game said so once as it began; one line per cue would only repeat it.
		if (Bank)
		{
			WarnOnce(Cue, TEXT("has no sounds in the sound bank yet: it plays nothing (Art/Sounds/cues.json, then Tools/Unreal/build_sound_bank.py)"));
		}
		return Play;
	}
	if (!(VolumeScale > 0.f))
	{
		return Play;
	}

	const int32 Pick = LooterSoundRules::PickVariation(Sounds.Num(), LastVariation[*Index], FMath::FRand());
	LastVariation[*Index] = Pick;
	Play.Entry = Entry;
	Play.Sound = Sounds[Pick];
	Play.Concurrency = Concurrencies[*Index];
	Play.Volume = VolumeScale * CueGains[*Index];
	Play.Pitch = LooterSoundRules::JitteredPitch(PitchScale, Entry->PitchJitter, FMath::FRand());
	return Play;
}

void ULooterSoundSubsystem::WarnOnce(FName Cue, const TCHAR* Why)
{
	bool bAlreadyWarned = false;
	WarnedCues.Add(Cue, &bAlreadyWarned);
	if (!bAlreadyWarned)
	{
		UE_LOG(LogLooter, Warning, TEXT("Sound cue %s %s."), *Cue.ToString(), Why);
	}
}

bool ULooterSoundSubsystem::RefuseLoop(const FPlay& Play)
{
	if (!Play.Entry->bLoop)
	{
		return false;
	}
	WarnOnce(Play.Entry->Cue, TEXT("is a loop, which a one-shot would never stop: start it with LooterSound::Start"));
	return true;
}

// ---------------------------------------------------------------------------
// Playing
// ---------------------------------------------------------------------------

void ULooterSoundSubsystem::PlayAt(const UObject* WorldContext, FName Cue, const FVector& Location, float VolumeScale, float PitchScale)
{
	const FPlay Play = Prepare(Cue, VolumeScale, PitchScale);
	if (!Play.IsValid() || RefuseLoop(Play))
	{
		return;
	}
	if (!Play.Entry->bSpatial)
	{
		PlayFlat(WorldContext, Play);
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(WorldContext, Play.Sound, Location, FRotator::ZeroRotator, Play.Volume, Play.Pitch, 0.f,
		Play.Entry->Attenuation, Play.Concurrency);
}

void ULooterSoundSubsystem::PlayAttached(FName Cue, USceneComponent& AttachTo, FName Socket, float VolumeScale, float PitchScale)
{
	const FPlay Play = Prepare(Cue, VolumeScale, PitchScale);
	if (!Play.IsValid() || RefuseLoop(Play))
	{
		return;
	}
	if (!Play.Entry->bSpatial)
	{
		PlayFlat(&AttachTo, Play);
		return;
	}
	// It finishes where it was if what it follows goes (a body sinking away mid-cry).
	UGameplayStatics::SpawnSoundAttached(Play.Sound, &AttachTo, Socket, FVector::ZeroVector, FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset, /*bStopWhenAttachedToDestroyed*/ false, Play.Volume, Play.Pitch, 0.f,
		Play.Entry->Attenuation, Play.Concurrency, /*bAutoDestroy*/ true);
}

void ULooterSoundSubsystem::Play2D(const UObject* WorldContext, FName Cue, float VolumeScale, float PitchScale)
{
	const FPlay Play = Prepare(Cue, VolumeScale, PitchScale);
	if (Play.IsValid() && !RefuseLoop(Play))
	{
		PlayFlat(WorldContext, Play);
	}
}

UAudioComponent* ULooterSoundSubsystem::Start(const UObject* WorldContext, FName Cue, USceneComponent* AttachTo, FName Socket,
	float VolumeScale, float PitchScale)
{
	const FPlay Play = Prepare(Cue, VolumeScale, PitchScale);
	if (!Play.IsValid())
	{
		return nullptr;
	}
	if (AttachTo && Play.Entry->bSpatial)
	{
		// A loop on a body stops with it.
		return UGameplayStatics::SpawnSoundAttached(Play.Sound, AttachTo, Socket, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset, /*bStopWhenAttachedToDestroyed*/ true, Play.Volume, Play.Pitch, 0.f,
			Play.Entry->Attenuation, Play.Concurrency, /*bAutoDestroy*/ true);
	}
	// Made first and started after: the engine makes every 2D sound go on over a paused game, and only the menus' should.
	UAudioComponent* Sound = UGameplayStatics::CreateSound2D(WorldContext, Play.Sound, Play.Volume, Play.Pitch, 0.f, Play.Concurrency,
		/*bPersistAcrossLevelTransition*/ false, /*bAutoDestroy*/ true);
	if (Sound)
	{
		Sound->bIsUISound = Play.Entry->bPlaysWhilePaused;
		Sound->Play();
	}
	return Sound;
}

// ---------------------------------------------------------------------------
// Volumes
// ---------------------------------------------------------------------------

void ULooterSoundSubsystem::LoadMix()
{
	Mix = LoadIfMade<USoundMix>(MixPath);
	MasterClass = LoadIfMade<USoundClass>(MasterClassPath);
	EffectsClass = LoadIfMade<USoundClass>(EffectsClassPath);
	InterfaceClass = LoadIfMade<USoundClass>(InterfaceClassPath);
	AmbienceClass = LoadIfMade<USoundClass>(AmbienceClassPath);
	MusicClass = LoadIfMade<USoundClass>(MusicClassPath);
	if (!Mix && Bank)
	{
		UE_LOG(LogLooter, Warning, TEXT("No sound mix at %s: the volume sliders do nothing until Tools/Unreal/build_sound_bank.py makes it."), MixPath);
	}
}

void ULooterSoundSubsystem::SetVolumes(const FLooterVolumes& InVolumes)
{
	Volumes = InVolumes;
	const UGameInstance* Game = GetGameInstance();
	ApplyVolumesTo(Game ? Game->GetWorld() : nullptr);
}

void ULooterSoundSubsystem::HandleActorsInitialized(const FActorsInitializedParams& Params)
{
	// Every world of this game as it starts (a level, the menu, a play-in-editor session's own audio device).
	if (Params.World && Params.World->GetGameInstance() == GetGameInstance())
	{
		ApplyVolumesTo(Params.World);
	}
}

void ULooterSoundSubsystem::ApplyVolumesTo(UWorld* World)
{
	if (!Mix || !CanPlayIn(World))
	{
		return;
	}
	const FLooterClassGains Gains = LooterSoundRules::ClassGains(Volumes);
	// At once (no fade): the slider's effect is heard while it's dragged. Master hands its gain down to the classes under it.
	auto SetGain = [this, World](USoundClass* Class, float Gain, bool bChildren)
	{
		if (Class)
		{
			UGameplayStatics::SetSoundMixClassOverride(World, Mix, Class, Gain, 1.f, 0.f, bChildren);
		}
	};
	SetGain(MasterClass, Gains.Master, true);
	SetGain(EffectsClass, Gains.Effects, false);
	SetGain(InterfaceClass, Gains.Interface, false);
	SetGain(AmbienceClass, Gains.Ambience, false);
	SetGain(MusicClass, Gains.Music, false);

	const FAudioDeviceHandle Device = World->GetAudioDevice();
	bool bOnDevice = false;
	MixedDevices.Add(Device.GetDeviceID(), &bOnDevice);
	if (!bOnDevice)
	{
		UGameplayStatics::PushSoundMixModifier(World, Mix);
	}
}
