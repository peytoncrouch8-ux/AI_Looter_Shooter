#include "Audio/AmbienceSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/AmbientEmitterComponent.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Audio/LooterSoundSubsystem.h"
#include "Audio/MusicDirectorSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "World/ChapelBell.h"
#include "World/LightingStateSubsystem.h"
#include "World/Train.h"

namespace
{
	/** How often it checks the light itself, in case a state landed before it was listening (s). */
	constexpr float LightCheckSeconds = 1.f;

	/** The steam's place on the locomotive: about its boiler, over the frame (cm above its origin). */
	constexpr float SteamHeight = 180.f;

	const TCHAR* AreaName(ELooterAudioArea Area)
	{
		switch (Area)
		{
		case ELooterAudioArea::Skyreach:
			return TEXT("Skyreach");
		case ELooterAudioArea::RansomsRest:
			return TEXT("Ransom's Rest");
		default:
			return TEXT("no area of its own (Skyreach's)");
		}
	}
}

UAmbienceSubsystem* UAmbienceSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UAmbienceSubsystem>() : nullptr;
}

bool UAmbienceSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Only the game's own worlds: the tests' preview worlds and the editor's viewports have no air to hear.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAmbienceSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!ULooterSoundSubsystem::CanPlayIn(&InWorld))
	{
		return;
	}
	Area = AmbienceRules::AreaForMap(InWorld.GetMapName());
	if (ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(&InWorld))
	{
		Light = Lighting->GetState();
		LightingHandle = Lighting->OnChanged.AddUObject(this, &UAmbienceSubsystem::HandleLightingChanged);
	}
	StartBed(AmbienceRules::BedFor(Area, Light), AmbienceRules::BedFadeInSeconds);
	SweetenerReady.Init(0.0, AmbienceRules::SweetenersFor(Area, Light).Num());
	NextSweetener = AmbienceRules::FirstSweetenerDelay;
	HookWorldEmitters(InWorld);
	bStarted = true;
	UE_LOG(LogLooter, Log, TEXT("Ambience: %s in %s, the bed %s + %s."), AreaName(Area), Light.IsNone() ? TEXT("its only light") : *Light.ToString(),
		*Bed.Air.ToString(), *Bed.Life.ToString());
}

void UAmbienceSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (ULightingStateSubsystem* Lighting = World->GetSubsystem<ULightingStateSubsystem>())
		{
			Lighting->OnChanged.Remove(LightingHandle);
		}
	}
	FadeAway(AirLayer, 0.1f);
	FadeAway(LifeLayer, 0.1f);
	AirLayer = nullptr;
	LifeLayer = nullptr;
	bStarted = false;
	Super::Deinitialize();
}

TStatId UAmbienceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAmbienceSubsystem, STATGROUP_Tickables);
}

bool UAmbienceSubsystem::IsTickable() const
{
	return bStarted;
}

void UAmbienceSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Clock += DeltaTime;
	LightCheck -= DeltaTime;
	if (LightCheck <= 0.f)
	{
		LightCheck = LightCheckSeconds;
		const ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(this);
		if (Lighting && !Lighting->IsSwitching() && Lighting->GetState() != Light)
		{
			SetLight(Lighting->GetState(), AmbienceRules::BedCrossfadeSeconds(ELightingSwitch::Instant));
		}
	}
	if (Clock >= NextSweetener)
	{
		PlaySweetener();
	}
}

void UAmbienceSubsystem::HandleLightingChanged(const FLightingStateChange& Change)
{
	SetLight(Change.To, AmbienceRules::BedCrossfadeSeconds(Change.How));
}

void UAmbienceSubsystem::SetLight(FName NewLight, float CrossfadeSeconds)
{
	if (NewLight == Light)
	{
		return;
	}
	Light = NewLight;
	StartBed(AmbienceRules::BedFor(Area, Light), CrossfadeSeconds);
	// The evening's calls are other creatures': their cooldowns start fresh, and the first waits for the change.
	SweetenerReady.Init(0.0, AmbienceRules::SweetenersFor(Area, Light).Num());
	LastSweetener = INDEX_NONE;
	NextSweetener = FMath::Max(NextSweetener, Clock + CrossfadeSeconds + 2.0);
	UE_LOG(LogLooter, Log, TEXT("Ambience: %s, the bed now %s + %s."), *Light.ToString(), *Bed.Air.ToString(), *Bed.Life.ToString());
}

void UAmbienceSubsystem::StartBed(const FAmbienceBed& NewBed, float FadeSeconds)
{
	// A layer the new bed shares (the island's breeze at any light) plays on untouched.
	if (NewBed.Air != Bed.Air || !AirLayer)
	{
		FadeAway(AirLayer, FadeSeconds);
		AirLayer = StartLayer(NewBed.Air, FadeSeconds);
	}
	if (NewBed.Life != Bed.Life || !LifeLayer)
	{
		FadeAway(LifeLayer, FadeSeconds);
		LifeLayer = StartLayer(NewBed.Life, FadeSeconds);
	}
	Bed = NewBed;
}

UAudioComponent* UAmbienceSubsystem::StartLayer(FName Cue, float FadeSeconds)
{
	ULooterSoundSubsystem* Sounds = ULooterSoundSubsystem::Find(this);
	if (Cue.IsNone() || !Sounds)
	{
		return nullptr;
	}
	const ULooterSoundSubsystem::FPlay Play = Sounds->Prepare(Cue);
	if (!Play.IsValid())
	{
		return nullptr;
	}
	UAudioComponent* Layer = UGameplayStatics::CreateSound2D(this, Play.Sound, Play.Volume, 1.f, 0.f, Play.Concurrency,
		/*bPersistAcrossLevelTransition*/ false, /*bAutoDestroy*/ false);
	if (!Layer)
	{
		return nullptr;
	}
	// The world's air pauses with the game (only the menus' sounds and the music go on).
	Layer->bIsUISound = false;
	// From anywhere in the loop: coming back to a place never starts its air the same way.
	const float Length = Play.Sound->Duration;
	Layer->FadeIn(FMath::Max(FadeSeconds, 0.05f), 1.f, Length > 1.f ? FMath::FRandRange(0.f, Length - 0.5f) : 0.f);
	return Layer;
}

void UAmbienceSubsystem::FadeAway(UAudioComponent* Layer, float Seconds)
{
	if (IsValid(Layer))
	{
		Layer->bAutoDestroy = true;
		Layer->FadeOut(FMath::Max(Seconds, 0.05f), 0.f);
	}
}

void UAmbienceSubsystem::PlaySweetener()
{
	const TArray<FAmbienceSweetener>& Options = AmbienceRules::SweetenersFor(Area, Light);
	if (Options.IsEmpty())
	{
		NextSweetener = Clock + 10.0;
		return;
	}
	if (SweetenerReady.Num() != Options.Num())
	{
		SweetenerReady.Init(0.0, Options.Num());
	}
	// A boss's fight is no place for birdsong: the calls wait it out.
	const UMusicDirectorSubsystem* Music = UMusicDirectorSubsystem::Get(this);
	if (Music && Music->GetMood() == EMusicMood::Boss)
	{
		NextSweetener = Clock + 5.0;
		return;
	}
	const int32 Pick = AmbienceRules::PickSweetener(Options, LastSweetener, SweetenerReady, Clock, FMath::FRand());
	if (!Options.IsValidIndex(Pick))
	{
		NextSweetener = Clock + 2.0;
		return;
	}
	const FAmbienceSweetener& Sweetener = Options[Pick];
	FVector Listener;
	if (Sweetener.bFlat)
	{
		LooterSound::Play2D(this, Sweetener.Cue);
	}
	else if (UAmbientEmitterComponent::FindListener(this, Listener))
	{
		LooterSound::PlayAt(this, Sweetener.Cue, AmbienceRules::SpotAround(Listener, Sweetener, FMath::FRand(), FMath::FRand(), FMath::FRand()));
	}
	LastSweetener = Pick;
	SweetenerReady[Pick] = Clock + Sweetener.Cooldown;
	NextSweetener = Clock + AmbienceRules::GapAfter(Sweetener, FMath::FRand());
}

void UAmbienceSubsystem::PlaySweetenerNow()
{
	NextSweetener = Clock;
	PlaySweetener();
}

void UAmbienceSubsystem::HookWorldEmitters(UWorld& InWorld)
{
	int32 Bells = 0;
	for (TActorIterator<AChapelBell> It(&InWorld); It; ++It)
	{
		if (It->Bell && UAmbientEmitterComponent::AddTo(It->Bell, NAME_None, LooterSoundCue::ChapelBellHum))
		{
			++Bells;
		}
	}
	bool bTrain = false;
	if (ATrain* Train = ATrain::FindIn(&InWorld))
	{
		USceneComponent* Locomotive = Train->GetCar(ATrain::NumCars - 1);
		if (UAmbientEmitterComponent* Steam = UAmbientEmitterComponent::AddTo(Locomotive ? Locomotive : Train->GetRootComponent(),
			NAME_None, LooterSoundCue::TrainHiss))
		{
			Steam->SetRelativeLocation(FVector(0.f, 0.f, SteamHeight));
			// Cold and shut until the story warms it (Main 7): silent until then, and it follows the train in its shots.
			const TWeakObjectPtr<ATrain> WeakTrain(Train);
			Steam->PlaysWhile = [WeakTrain]() { return WeakTrain.IsValid() && WeakTrain->IsWarm(); };
			bTrain = true;
		}
	}
	if (Bells > 0 || bTrain)
	{
		UE_LOG(LogLooter, Log, TEXT("Ambience: voices for %d chapel bell(s)%s."), Bells, bTrain ? TEXT(" and the train's steam") : TEXT(""));
	}
}

FString UAmbienceSubsystem::Describe() const
{
	const TArray<FAmbienceSweetener>& Options = AmbienceRules::SweetenersFor(Area, Light);
	return FString::Printf(TEXT("Ambience: %s, light %s; bed %s (%s) + %s (%s); %d sweeteners, the next in %.1f s, the last %s."),
		AreaName(Area), Light.IsNone() ? TEXT("-") : *Light.ToString(),
		*Bed.Air.ToString(), AirLayer && AirLayer->IsPlaying() ? TEXT("playing") : TEXT("silent"),
		*Bed.Life.ToString(), LifeLayer && LifeLayer->IsPlaying() ? TEXT("playing") : TEXT("silent"),
		Options.Num(), FMath::Max(0.0, NextSweetener - Clock),
		Options.IsValidIndex(LastSweetener) ? *Options[LastSweetener].Cue.ToString() : TEXT("none yet"));
}
