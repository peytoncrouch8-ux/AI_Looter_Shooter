// TrainShots: the train pulling out and backing in as scenes: its course, the black, the camera on the platform.

#include "Scenes/TrainShots.h"
#include "AI_Looter_Shooter.h"
#include "World/Train.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** What a shot's moves and moments share; they outlive the call that made them. */
	struct FShotState
	{
		TWeakObjectPtr<USceneSubsystem> Scenes;
		TWeakObjectPtr<ATrain> Train;
		/** Where the arriving player's feet go, and the way they face. */
		FTransform Landing = FTransform::Identity;
	};

	bool IsSkipping(const FShotState& Shot)
	{
		const USceneSubsystem* Scenes = Shot.Scenes.Get();
		return Scenes && Scenes->IsSkipping();
	}

	/** The held player's camera, whose fade is the black (none in a test level). */
	APlayerCameraManager* CameraOf(const FShotState& Shot)
	{
		const USceneSubsystem* Scenes = Shot.Scenes.Get();
		const APlayerController* Controller = Scenes ? Scenes->GetHeldController() : nullptr;
		return Controller ? Controller->PlayerCameraManager.Get() : nullptr;
	}

	/** The screen from From to To black over Seconds (held there when bHold); at once while skipping, or with no time. */
	void Fade(const FShotState& Shot, float From, float To, float Seconds, bool bHold)
	{
		APlayerCameraManager* Camera = CameraOf(Shot);
		if (!Camera)
		{
			return;
		}
		if (IsSkipping(Shot) || Seconds <= 0.f)
		{
			if (To > 0.f)
			{
				Camera->SetManualCameraFade(To, FLinearColor::Black, /*bInFadeAudio*/ false);
			}
			else
			{
				Camera->StopCameraFade();
			}
			return;
		}
		Camera->StartCameraFade(From, To, Seconds, FLinearColor::Black, /*bShouldFadeAudio*/ false, bHold);
	}

	/** The view on the train's fixed camera on the platform. */
	void ViewFromPlatform(const FShotState& Shot)
	{
		USceneSubsystem* Scenes = Shot.Scenes.Get();
		const ATrain* Train = Shot.Train.Get();
		ACameraActor* Camera = Scenes && Train ? Scenes->ViewFromSceneCamera(0.f) : nullptr;
		if (!Camera)
		{
			return;
		}
		const FTransform At = Train->GetShotCameraTransform();
		Camera->SetActorLocationAndRotation(At.GetLocation(), At.Rotator());
		if (UCameraComponent* Lens = Camera->GetCameraComponent())
		{
			Lens->SetFieldOfView(Train->ShotFieldOfView);
		}
	}

	void MoveTrain(const FShotState& Shot, float Distance)
	{
		if (ATrain* Train = Shot.Train.Get())
		{
			Train->SetTravel(Distance);
		}
	}
}

FName TrainShots::DepartureName()
{
	return FName(TEXT("TrainDeparture"));
}

FName TrainShots::ArrivalName()
{
	return FName(TEXT("TrainArrival"));
}

FName TrainShots::PullOutMoment()
{
	return FName(TEXT("PullOut"));
}

FName TrainShots::BlackMoment()
{
	return FName(TEXT("Black"));
}

FName TrainShots::StopMoment()
{
	return FName(TEXT("Stop"));
}

FName TrainShots::HandBackMoment()
{
	return FName(TEXT("HandBack"));
}

float TrainShots::DepartDistanceAt(float Seconds)
{
	// From a stand, at an even pull: out along the track as the square of the time since the brakes came off.
	const float Rolling = FMath::Max(Seconds - PullOutTime, 0.f);
	return 0.5f * Acceleration * Rolling * Rolling;
}

float TrainShots::ArriveDistanceAt(float Seconds)
{
	// The pull-out run backwards: braking evenly to a stand at the platform.
	const float Left = FMath::Clamp(RollInSeconds - Seconds, 0.f, RollInSeconds);
	return 0.5f * Acceleration * Left * Left;
}

float TrainShots::DepartBlackAt(float Seconds)
{
	return FMath::Clamp((Seconds - FadeOutStart) / (DepartSeconds - FadeOutStart), 0.f, 1.f);
}

float TrainShots::ArriveBlackAt(float Seconds)
{
	return 1.f - FMath::Clamp(Seconds / FadeInSeconds, 0.f, 1.f);
}

FScenePlay TrainShots::MakeDeparture(USceneSubsystem& Scenes, ATrain& Train, TFunction<void()> AtBlack, bool bTripFollows)
{
	const TSharedRef<FShotState> Shot = MakeShared<FShotState>();
	Shot->Scenes = &Scenes;
	Shot->Train = &Train;

	FScenePlay Scene;
	Scene.Name = DepartureName();
	// The platform's camera has the view: no looking about.
	Scene.bLookOnly = false;
	Scene.bHoldAfterEnd = bTripFollows;
	Scene.OnStart = [Shot]()
	{
		// Aboard Tilly's car: the player is out of sight, and the cut to the platform comes out of a moment's black.
		if (USceneSubsystem* Played = Shot->Scenes.Get())
		{
			Played->SetHeldPlayerHidden(true);
		}
		ViewFromPlatform(*Shot);
		Fade(*Shot, 1.f, 0.f, CutInSeconds, false);
		MoveTrain(*Shot, 0.f);
	};
	Scene.Timeline.AddMove(0.f, DepartSeconds, [Shot](float Alpha) { MoveTrain(*Shot, DepartDistanceAt(Alpha * DepartSeconds)); });
	Scene.Timeline.AddMoment(PullOutMoment(), PullOutTime);
	Scene.Timeline.AddMoment(TEXT("FadeOut"), FadeOutStart, [Shot]() { Fade(*Shot, 0.f, 1.f, DepartSeconds - FadeOutStart, true); });
	// Black, and it stays black for what follows (the trip's level load, or the train put back).
	Scene.Timeline.AddMoment(BlackMoment(), DepartSeconds, [Shot]() { Fade(*Shot, 1.f, 1.f, 0.f, true); });

	Scene.AfterEnd = [Shot, AtBlack = MoveTemp(AtBlack), bTripFollows]()
	{
		if (AtBlack)
		{
			AtBlack();
		}
		// Nobody was going (the console): the player has been given back already; the train comes back and the black lifts.
		USceneSubsystem* Played = Shot->Scenes.Get();
		ATrain* Moved = Shot->Train.Get();
		if (!bTripFollows && Played && Moved)
		{
			PutBack(*Played, *Moved);
		}
	};
	// A trip that never came: the same, as the scene gives the player back.
	Scene.OnReturn = [Shot]()
	{
		USceneSubsystem* Played = Shot->Scenes.Get();
		ATrain* Moved = Shot->Train.Get();
		if (Played && Moved)
		{
			PutBack(*Played, *Moved);
		}
	};
	return Scene;
}

FScenePlay TrainShots::MakeArrival(USceneSubsystem& Scenes, ATrain& Train, const FTransform& Landing)
{
	const TSharedRef<FShotState> Shot = MakeShared<FShotState>();
	Shot->Scenes = &Scenes;
	Shot->Train = &Train;
	Shot->Landing = Landing;

	FScenePlay Scene;
	Scene.Name = ArrivalName();
	Scene.bLookOnly = false;
	Scene.OnStart = [Shot]()
	{
		// Out of the black onto the platform. The player is where the trip lands them already, unseen: they're in the car.
		Fade(*Shot, 1.f, 1.f, 0.f, true);
		if (USceneSubsystem* Played = Shot->Scenes.Get())
		{
			Played->PlaceHeldPlayer(Shot->Landing.GetLocation(), static_cast<float>(Shot->Landing.Rotator().Yaw));
			Played->SetHeldPlayerHidden(true);
		}
		ViewFromPlatform(*Shot);
		MoveTrain(*Shot, ArriveDistanceAt(0.f));
		Fade(*Shot, 1.f, 0.f, FadeInSeconds, false);
	};
	Scene.Timeline.AddMove(0.f, RollInSeconds, [Shot](float Alpha) { MoveTrain(*Shot, ArriveDistanceAt(Alpha * RollInSeconds)); });
	Scene.Timeline.AddMoment(StopMoment(), RollInSeconds, [Shot]() { MoveTrain(*Shot, 0.f); });
	Scene.Timeline.AddMoment(HandBackMoment(), HandBackAt, [Shot]()
	{
		// Stepped down from Tilly's car: the view eases back into the player's own eyes at its door.
		USceneSubsystem* Played = Shot->Scenes.Get();
		if (!Played)
		{
			return;
		}
		Played->SetHeldPlayerHidden(false);
		Played->ViewFromPlayer(Played->IsSkipping() ? 0.f : HandBackSeconds);
		if (Played->IsSkipping())
		{
			// Skipped before the black had lifted.
			Fade(*Shot, 0.f, 0.f, 0.f, false);
		}
	});
	Scene.Timeline.AddMoment(TEXT("Arrived"), ArriveSeconds);
	return Scene;
}

void TrainShots::PutBack(USceneSubsystem& Scenes, ATrain& Train)
{
	Train.SetTravel(0.f);
	const UWorld* World = Scenes.GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (Controller && Controller->PlayerCameraManager)
	{
		Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, FadeInSeconds, FLinearColor::Black, /*bShouldFadeAudio*/ false,
			/*bHoldWhenFinished*/ false);
	}
	UE_LOG(LogLooter, Log, TEXT("Train: back at the platform, no trip."));
}
