#include "Settings/AudioSettingsSubsystem.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* AudioSaveSlot = TEXT("AudioSettings");
}

void UAudioSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SaveData = LoadAudio(AudioSaveSlot, this);
	// The sound system keeps them and sets them on every world of the game as it starts; the first is still to come.
	Apply();
}

ULooterAudioSave* UAudioSettingsSubsystem::LoadAudio(const FString& Slot, UObject* Outer)
{
	// Asked first: reading a slot that isn't there logs a warning, and a fresh install has none.
	if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		if (ULooterAudioSave* Saved = Cast<ULooterAudioSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0)))
		{
			return Saved;
		}
	}
	return NewObject<ULooterAudioSave>(Outer ? Outer : GetTransientPackage());
}

float UAudioSettingsSubsystem::ClampVolume(float Share)
{
	if (!FMath::IsFinite(Share))
	{
		return DefaultVolume;
	}
	// The slider's steps, so the menu's label and the saved value always agree.
	return FMath::Clamp(FMath::GridSnap(Share, VolumeStep), 0.f, 1.f);
}

FLooterVolumes UAudioSettingsSubsystem::VolumesOf(const ULooterAudioSave* Audio)
{
	FLooterVolumes Volumes;
	if (Audio)
	{
		Volumes.Master = ClampVolume(Audio->Master);
		Volumes.Effects = ClampVolume(Audio->Effects);
		Volumes.Interface = ClampVolume(Audio->Interface);
		Volumes.Music = ClampVolume(Audio->Music);
	}
	return Volumes;
}

void UAudioSettingsSubsystem::SetVolumeOf(ULooterAudioSave& Audio, EAudioVolume Which, float Share)
{
	const float Clamped = ClampVolume(Share);
	switch (Which)
	{
	case EAudioVolume::Master:    Audio.Master = Clamped; break;
	case EAudioVolume::Effects:   Audio.Effects = Clamped; break;
	case EAudioVolume::Interface: Audio.Interface = Clamped; break;
	case EAudioVolume::Music:     Audio.Music = Clamped; break;
	}
}

FString UAudioSettingsSubsystem::VolumeName(EAudioVolume Which)
{
	switch (Which)
	{
	case EAudioVolume::Master:    return TEXT("Master");
	case EAudioVolume::Effects:   return TEXT("Effects");
	case EAudioVolume::Interface: return TEXT("Interface");
	case EAudioVolume::Music:     return TEXT("Music");
	}
	return TEXT("");
}

FLooterVolumes UAudioSettingsSubsystem::GetVolumes() const
{
	return VolumesOf(SaveData);
}

float UAudioSettingsSubsystem::GetVolume(EAudioVolume Which) const
{
	const FLooterVolumes Volumes = GetVolumes();
	switch (Which)
	{
	case EAudioVolume::Master:    return Volumes.Master;
	case EAudioVolume::Effects:   return Volumes.Effects;
	case EAudioVolume::Interface: return Volumes.Interface;
	case EAudioVolume::Music:     return Volumes.Music;
	}
	return DefaultVolume;
}

void UAudioSettingsSubsystem::SetVolume(EAudioVolume Which, float Share, bool bSave)
{
	if (!SaveData)
	{
		return;
	}
	SetVolumeOf(*SaveData, Which, Share);
	Apply();
	if (bSave)
	{
		SaveSettings();
	}
}

void UAudioSettingsSubsystem::SaveSettings() const
{
	if (SaveData)
	{
		UGameplayStatics::SaveGameToSlot(SaveData, AudioSaveSlot, 0);
	}
}

void UAudioSettingsSubsystem::Apply() const
{
	const ULocalPlayer* Player = GetLocalPlayer();
	const UGameInstance* Game = Player ? Player->GetGameInstance() : nullptr;
	if (ULooterSoundSubsystem* Sounds = Game ? Game->GetSubsystem<ULooterSoundSubsystem>() : nullptr)
	{
		Sounds->SetVolumes(GetVolumes());
	}
}
