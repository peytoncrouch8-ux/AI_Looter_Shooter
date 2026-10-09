#include "Audio/AmbientEmitterComponent.h"
#include "Audio/AmbienceRules.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundBank.h"
#include "Audio/LooterSoundRules.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundWave.h"

namespace
{
	/** A place's reach when its cue says nothing (cm): about a creature's attenuation. */
	constexpr float DefaultReach = 3500.f;
}

bool UAmbientEmitterComponent::FindListener(const UObject* WorldContext, FVector& OutLocation)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (!Controller || !Controller->IsLocalController())
	{
		return false;
	}
	// The camera's place, for the first-person and third-person views alike.
	FVector Front;
	FVector Right;
	Controller->GetAudioListenerPosition(OutLocation, Front, Right);
	return true;
}

UAmbientEmitterComponent::UAmbientEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	// Nothing here needs a frame's precision: whether the listener is in earshot, and where along a creek it stands.
	PrimaryComponentTick.TickInterval = 0.25f;
	SetMobility(EComponentMobility::Movable);
}

UAmbientEmitterComponent* UAmbientEmitterComponent::AddTo(USceneComponent* Parent, FName Socket, FName InLoopCue, float InVolumeScale)
{
	AActor* Owner = Parent ? Parent->GetOwner() : nullptr;
	if (!Owner)
	{
		return nullptr;
	}
	UAmbientEmitterComponent* Emitter = NewObject<UAmbientEmitterComponent>(Owner, NAME_None, RF_Transient);
	Emitter->LoopCue = InLoopCue;
	Emitter->VolumeScale = InVolumeScale;
	Emitter->SetupAttachment(Parent, Socket);
	// It begins play with its owner (at once, if its owner already has).
	Emitter->RegisterComponent();
	return Emitter;
}

void UAmbientEmitterComponent::BeginPlay()
{
	Super::BeginPlay();
	// The first one-shot comes at its own time, so emitters placed together don't speak together.
	NextOneShot = FMath::FRandRange(0.3f * static_cast<float>(OneShotGap.X), static_cast<float>(FMath::Max(OneShotGap.X, OneShotGap.Y)));
}

void UAmbientEmitterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Loop)
	{
		Loop->Stop();
	}
	bLoopOn = false;
	Super::EndPlay(EndPlayReason);
}

void UAmbientEmitterComponent::SetModulation(float InVolume, float InPitch)
{
	ModVolume = FMath::Clamp(FMath::IsFinite(InVolume) ? InVolume : 1.f, 0.f, 4.f);
	ModPitch = FMath::Clamp(FMath::IsFinite(InPitch) ? InPitch : 1.f, 0.25f, 4.f);
}

FVector UAmbientEmitterComponent::SourceFor(const FVector& ListenerLocation) const
{
	return Path.IsEmpty() ? GetComponentLocation() : AmbienceRules::NearestOnPath(Path, bClosedPath, ListenerLocation);
}

float UAmbientEmitterComponent::ReachOf(FName Cue) const
{
	if (AudibleRadius > 0.f)
	{
		return AudibleRadius;
	}
	if (CachedReach > 0.f)
	{
		return CachedReach;
	}
	const ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(this);
	const ULooterSoundBank* Bank = Sounds ? Sounds->GetBank() : nullptr;
	const FLooterSoundCueEntry* Entry = Bank ? Bank->FindCue(Cue) : nullptr;
	const USoundAttenuation* Attenuation = Entry ? Entry->Attenuation.Get() : nullptr;
	CachedReach = Attenuation ? Attenuation->Attenuation.GetMaxDimension() : DefaultReach;
	return CachedReach;
}

void UAmbientEmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!ULooterSoundSubsystem::Find(this))
	{
		return;
	}
	FVector Listener;
	if (!FindListener(this, Listener))
	{
		return;
	}
	const FVector Source = SourceFor(Listener);
	const float Distance = FVector::Dist(Source, Listener);
	const bool bAllowed = !PlaysWhile || PlaysWhile();

	if (!LoopCue.IsNone())
	{
		// Stopped from outside (its cue's concurrency gave its voice to a nearer one): it starts again when it can.
		if (bLoopOn && (!Loop || !Loop->IsPlaying()))
		{
			bLoopOn = false;
		}
		const float Reach = ReachOf(LoopCue);
		const bool bWanted = bAllowed && Distance < Reach * (bLoopOn ? Hysteresis : 1.f);
		if (bWanted && !bLoopOn)
		{
			StartLoop(Source);
		}
		else if (!bWanted && bLoopOn)
		{
			StopLoop();
		}
		if (bLoopOn && Loop)
		{
			if (!Path.IsEmpty())
			{
				Loop->SetWorldLocation(Source);
			}
			const float Volume = LoopGain * VolumeScale * ModVolume;
			if (!FMath::IsNearlyEqual(Volume, AppliedVolume, 0.005f))
			{
				Loop->SetVolumeMultiplier(Volume);
				AppliedVolume = Volume;
			}
			if (!FMath::IsNearlyEqual(ModPitch, AppliedPitch, 0.002f))
			{
				Loop->SetPitchMultiplier(ModPitch);
				AppliedPitch = ModPitch;
			}
		}
	}

	if (!OneShotCues.IsEmpty() && bAllowed)
	{
		NextOneShot -= DeltaTime;
		if (NextOneShot <= 0.f)
		{
			// Only heard in earshot, but the clock runs anyway, so walking up to a place never finds it mid-silence.
			if (LoopCue.IsNone() ? Distance < ReachOf(OneShotCues[0]) : Distance < ReachOf(LoopCue))
			{
				PlayOneShot(Source);
			}
			const float Low = static_cast<float>(FMath::Min(OneShotGap.X, OneShotGap.Y));
			const float High = static_cast<float>(FMath::Max(OneShotGap.X, OneShotGap.Y));
			NextOneShot = FMath::FRandRange(FMath::Max(Low, 0.5f), FMath::Max(High, 0.5f));
		}
	}
}

void UAmbientEmitterComponent::StartLoop(const FVector& At)
{
	ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(this);
	const ULooterSoundSubsystem::FPlay Play = Sounds ? Sounds->Prepare(LoopCue) : ULooterSoundSubsystem::FPlay();
	if (!Play.IsValid())
	{
		// No sound for it yet (it warned once): it looks again next time.
		return;
	}
	if (!Loop)
	{
		AActor* Owner = GetOwner();
		if (!Owner)
		{
			return;
		}
		Loop = NewObject<UAudioComponent>(Owner, NAME_None, RF_Transient);
		Loop->bAutoActivate = false;
		Loop->bAutoDestroy = false;
		Loop->SetupAttachment(this);
		Loop->RegisterComponent();
	}
	Loop->SetSound(Play.Sound);
	Loop->AttenuationSettings = Play.Entry->Attenuation;
	Loop->ConcurrencySet.Reset();
	if (Play.Concurrency)
	{
		Loop->ConcurrencySet.Add(Play.Concurrency);
	}
	LoopGain = Play.Volume;
	AppliedVolume = LoopGain * VolumeScale * ModVolume;
	AppliedPitch = ModPitch;
	Loop->SetVolumeMultiplier(AppliedVolume);
	Loop->SetPitchMultiplier(AppliedPitch);
	if (!Path.IsEmpty())
	{
		Loop->SetWorldLocation(At);
	}
	// From a random point of the loop each time, so two creeks never run in step and coming back never starts the same.
	const float Length = Play.Sound->Duration;
	Loop->FadeIn(FadeSeconds, 1.f, Length > 1.f ? FMath::FRandRange(0.f, Length - 0.5f) : 0.f);
	bLoopOn = true;
}

void UAmbientEmitterComponent::StopLoop()
{
	if (Loop)
	{
		Loop->FadeOut(FadeSeconds, 0.f);
	}
	bLoopOn = false;
}

void UAmbientEmitterComponent::PlayOneShot(const FVector& Around)
{
	const int32 Pick = LooterSoundRules::PickVariation(OneShotCues.Num(), LastOneShot, FMath::FRand());
	if (!OneShotCues.IsValidIndex(Pick))
	{
		return;
	}
	LastOneShot = Pick;
	const float Angle = FMath::FRand() * UE_TWO_PI;
	const float Radius = Scatter * FMath::Sqrt(FMath::FRand());
	const FVector At = Around + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
	LooterSound::PlayAt(this, OneShotCues[Pick], At, VolumeScale);
}
