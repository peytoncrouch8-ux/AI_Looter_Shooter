// SmogReveal: the look through the spyglass at Vane's dredge barge, its hose and its smog, with Vane on the loudspeakers.

#include "Scenes/SmogReveal.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Story/VaneBroadcast.h"
#include "World/LamplighterRules.h"
#include "World/LandfallSpyglass.h"
#include "World/WorldQueries.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The dip into and out of the glass (seconds). */
	constexpr float DipInSeconds = 0.5f;
	constexpr float DipBackSeconds = 0.6f;
	/** The hand on the glass: a slow, slight sway (degrees, Hz). */
	constexpr float SwayPitch = 0.15f;
	constexpr float SwayYaw = 0.2f;
	/** Where the glass looks with no barge in the level: this far along its tube. */
	constexpr float NoBargeAhead = 20000.f;

	/** What the scene's moves and moments share; they outlive the call that made them. */
	struct FRevealState
	{
		TWeakObjectPtr<USceneSubsystem> Scenes;
		FVector Eye = FVector::ZeroVector;
		FVector Barge = FVector::ZeroVector;
		FVector HoseFoot = FVector::ZeroVector;
		FVector SmogTop = FVector::ZeroVector;
		bool bFull = true;
		float Clock = 0.f;
		TWeakObjectPtr<UAudioComponent> Ambience;
	};

	bool IsSkipping(const FRevealState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		return Scenes && Scenes->IsSkipping();
	}

	float Ease(float Alpha)
	{
		const float A = FMath::Clamp(Alpha, 0.f, 1.f);
		return A * A * A * (A * (A * 6.f - 15.f) + 10.f);
	}

	/** The glass looking at Look with a lens of Fov, swaying a little in the hand. */
	void SetView(const FRevealState& State, const FVector& Look, float Fov)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr;
		if (!Camera)
		{
			return;
		}
		const float T = State.Clock;
		const FRotator Sway(SwayPitch * FMath::Sin(UE_TWO_PI * 0.21f * T), SwayYaw * FMath::Sin(UE_TWO_PI * 0.14f * T + 1.f), 0.0);
		Camera->SetActorLocationAndRotation(State.Eye, (Look - State.Eye).Rotation() + Sway);
		if (UCameraComponent* Lens = Camera->GetCameraComponent())
		{
			Lens->SetFieldOfView(Fov);
		}
	}

	APlayerCameraManager* CameraManagerOf(const FRevealState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		const APlayerController* Controller = Scenes ? Scenes->GetHeldController() : nullptr;
		return Controller ? Controller->PlayerCameraManager.Get() : nullptr;
	}
}

FName SmogReveal::SceneName()
{
	static const FName Name(TEXT("SmogReveal"));
	return Name;
}

float SmogReveal::DurationFor(bool bFull)
{
	return (bFull ? WaitAt : BargeSeconds + HoseSeconds) + OutSeconds;
}

FVector SmogReveal::HoseFootUnder(const UWorld* World, const AActor& Barge)
{
	const FVector From = Barge.GetActorLocation();
	FHitResult Hit;
	if (World && World->LineTraceSingleByObjectType(Hit, From - FVector(0.0, 0.0, 300.0), From - FVector(0.0, 0.0, 20000.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), LooterWorld::StaticGeometryParams(World, TEXT("SmogRevealHose"), &Barge)))
	{
		return Hit.ImpactPoint;
	}
	return From - FVector(0.0, 0.0, HoseFallback);
}

FScenePlay SmogReveal::Make(USceneSubsystem& Scenes, ALandfallSpyglass& Spyglass, AActor* Barge, bool bFull)
{
	const TSharedRef<FRevealState> State = MakeShared<FRevealState>();
	State->Scenes = &Scenes;
	State->Eye = Spyglass.GetLens();
	State->bFull = bFull;
	if (Barge)
	{
		State->Barge = Barge->GetActorLocation();
		State->HoseFoot = HoseFootUnder(Spyglass.GetWorld(), *Barge);
	}
	else
	{
		const FVector Ahead = (State->Eye - Spyglass.Mount->GetComponentLocation()).GetSafeNormal(UE_SMALL_NUMBER, Spyglass.GetActorForwardVector());
		State->Barge = State->Eye + Ahead * NoBargeAhead;
		State->HoseFoot = State->Barge - FVector(0.0, 0.0, HoseFallback);
	}
	State->SmogTop = State->Barge + FVector(0.0, 0.0, SmogAbove);

	FScenePlay Scene;
	Scene.Name = SceneName();
	// The glass has the view: only the skip keys work.
	Scene.bLookOnly = false;
	Scene.OnStart = [State]()
	{
		USceneSubsystem* Played = State->Scenes.Get();
		if (!Played)
		{
			return;
		}
		Played->ViewFromSceneCamera(0.f);
		SetView(*State, State->Barge, WideFov);
		if (APlayerCameraManager* Camera = CameraManagerOf(*State))
		{
			Camera->StartCameraFade(1.f, 0.f, DipInSeconds, FLinearColor::Black);
		}
		LooterSound::Play2D(Played, LooterSoundCue::SpyglassOpen);
		if (State->bFull)
		{
			// The smog's breath close through the glass (softer: the valley's own ambience may be playing under it).
			State->Ambience = LooterSound::Start(Played, LooterSoundCue::SmogAmbience, nullptr, NAME_None, 0.6f);
		}
	};
	Scene.OnTick = [State](float DeltaSeconds) { State->Clock += DeltaSeconds; };

	// On the barge, the lens closing in.
	Scene.Timeline.AddMove(0.f, BargeSeconds, [State](float Alpha)
	{
		SetView(*State, State->Barge, FMath::Lerp(WideFov, CloseFov, Ease(Alpha)));
	});
	// Down its hose into the Sink.
	Scene.Timeline.AddMove(BargeSeconds, HoseSeconds, [State](float Alpha)
	{
		SetView(*State, FMath::Lerp(State->Barge, State->HoseFoot, static_cast<double>(Ease(Alpha))), FMath::Lerp(CloseFov, 13.f, Ease(Alpha)));
	});
	const float End = DurationFor(bFull);
	if (bFull)
	{
		// Up the smog pouring off it, while Vane talks; then the scene waits for him to finish.
		Scene.Timeline.AddMove(BargeSeconds + HoseSeconds, SmogSeconds, [State](float Alpha)
		{
			SetView(*State, FMath::Lerp(State->HoseFoot, State->SmogTop, static_cast<double>(Ease(Alpha))), FMath::Lerp(13.f, 16.f, Ease(Alpha)));
		});
		Scene.Timeline.AddMoment(TEXT("Broadcast"), BroadcastAt, [State]()
		{
			if (!IsSkipping(*State))
			{
				UVaneBroadcast::Say(State->Scenes.Get(), LandfallRules::LoadLines(LandfallRules::VaneValleyLines()));
			}
		});
		Scene.Timeline.AddWait(WaitAt, [State]()
		{
			return !UVaneBroadcast::IsSpeaking(State->Scenes.Get()) || State->Clock >= LongestWait;
		});
	}
	Scene.Timeline.AddMoment(TEXT("Out"), End - OutSeconds, [State]()
	{
		if (IsSkipping(*State))
		{
			return;
		}
		if (APlayerCameraManager* Camera = CameraManagerOf(*State))
		{
			Camera->StartCameraFade(0.f, 1.f, OutSeconds * 0.8f, FLinearColor::Black, /*bShouldFadeAudio*/ false, /*bHoldWhenFinished*/ true);
		}
	});
	Scene.Timeline.AddMoment(TEXT("Done"), End, [State]()
	{
		if (IsSkipping(*State))
		{
			UVaneBroadcast::Stop(State->Scenes.Get());
		}
		LooterSound::Stop(State->Ambience.Get(), 1.f);
		State->Ambience.Reset();
	});

	TWeakObjectPtr<ALandfallSpyglass> WeakGlass = &Spyglass;
	Scene.AfterEnd = [State, WeakGlass]()
	{
		// Out of the glass to their own eyes; and with the full reveal, Finch on the tube: the Lantern has Abel's trail.
		const ALandfallSpyglass* Glass = WeakGlass.Get();
		const UWorld* World = Glass ? Glass->GetWorld() : nullptr;
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		if (Controller && Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, DipBackSeconds, FLinearColor::Black, false, false);
		}
		if (State->bFull && Glass)
		{
			LooterSound::Play2D(Glass, LooterSoundCue::LanternLean);
			LandfallRules::Say(Glass, LandfallRules::TrailLines());
		}
	};
	return Scene;
}
