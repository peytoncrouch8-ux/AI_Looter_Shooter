// FerryLadderClimb: a climb on the Ferryman's rope ladder as a scene: the turn to the ladder, the rungs, the step off.

#include "Scenes/FerryLadderClimb.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Player/PlayerViewComponent.h"
#include "World/FerryLadder.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The climber's eyes over their feet when their camera can't say, and the bounds a crouch or a lean is stood up to (cm). */
	constexpr double ClimbFallbackEye = 150.0;
	constexpr double ClimbEyeLowest = 60.0;
	constexpr double ClimbEyeHighest = 200.0;
	/** Hanging on the ladder the eyes are this far out from its face, and on the top rungs this share of standing height up. */
	constexpr double OffFace = 40.0;
	constexpr double HangingShare = 0.55;
	/** Facing the rungs: looking a little down on the way down, a little up on the way up (degrees). */
	constexpr float DownPitch = -12.f;
	constexpr float UpPitch = 14.f;

	/** What the climb's moves and moments share; they outlive the call that made them. */
	struct FClimbState
	{
		TWeakObjectPtr<USceneSubsystem> Scenes;
		TWeakObjectPtr<AFerryLadder> Ladder;
		TWeakObjectPtr<AActor> Climber;
		bool bDown = true;
		FVector StartEye = FVector::ZeroVector;
		FVector OnFrom = FVector::ZeroVector;
		FVector OnTo = FVector::ZeroVector;
		FVector EndEye = FVector::ZeroVector;
		FVector EndFeet = FVector::ZeroVector;
		float EndYaw = 0.f;
		FQuat StartLook = FQuat::Identity;
		FQuat FaceLadder = FQuat::Identity;
		FQuat EndLook = FQuat::Identity;
		TWeakObjectPtr<UAudioComponent> Creak;
	};

	void StartClimb(FClimbState& Climb)
	{
		USceneSubsystem* Scenes = Climb.Scenes.Get();
		AFerryLadder* Ladder = Climb.Ladder.Get();
		AActor* Climber = Climb.Climber.Get();
		if (!Scenes || !Ladder || !Climber)
		{
			return;
		}
		const APlayerController* Controller = Scenes->GetHeldController();
		const FRotator Look = Controller ? Controller->GetControlRotation() : Climber->GetActorRotation();

		// Their eyes where they stand, off their own camera, so the view starts there and ends at their body's own eyes.
		const UCameraComponent* Eyes = UPlayerViewComponent::FindFirstPersonCamera(Climber);
		const FVector Feet = Climber->GetActorLocation() - FVector(0.0, 0.0, Climber->GetSimpleCollisionHalfHeight());
		const double EyeHeight = Eyes ? FMath::Clamp(Eyes->GetComponentLocation().Z - Feet.Z, ClimbEyeLowest, ClimbEyeHighest) : ClimbFallbackEye;
		Climb.StartEye = Eyes ? Eyes->GetComponentLocation() : Feet + FVector(0.0, 0.0, EyeHeight);

		const FVector Out = FRotator(0.0, Ladder->GetActorRotation().Yaw, 0.0).Vector();
		const FVector Up(0.0, 0.0, 1.0);
		const FVector TopOn = Ladder->GetTopRung() + Out * OffFace + Up * (EyeHeight * HangingShare);
		const FVector FootOn(Ladder->GetBottomRung().X + Out.X * OffFace, Ladder->GetBottomRung().Y + Out.Y * OffFace,
			Ladder->GetBottomFeet().Z + EyeHeight);
		Climb.OnFrom = Climb.bDown ? TopOn : FootOn;
		Climb.OnTo = Climb.bDown ? FootOn : TopOn;
		Climb.EndFeet = Climb.bDown ? Ladder->GetBottomFeet() : Ladder->GetTopFeet();
		Climb.EndYaw = Climb.bDown ? Ladder->GetBottomYaw() : Ladder->GetTopYaw();
		Climb.EndEye = Climb.EndFeet + Up * EyeHeight;
		Climb.StartLook = FQuat(Look);
		Climb.FaceLadder = FQuat(FRotator(Climb.bDown ? DownPitch : UpPitch, Ladder->GetTopYaw(), 0.f));
		Climb.EndLook = FQuat(FRotator(0.f, Climb.EndYaw, 0.f));

		// The body waits unseen at the far end, facing on; the view climbs, at the player's own field of view.
		Scenes->SetHeldPlayerHidden(true);
		Scenes->PlaceHeldPlayer(Climb.EndFeet, Climb.EndYaw);
		if (ACameraActor* Camera = Scenes->ViewFromSceneCamera(0.f))
		{
			if (UCameraComponent* Lens = Camera->GetCameraComponent())
			{
				Lens->SetConstraintAspectRatio(false);
				if (Eyes)
				{
					Lens->SetFieldOfView(Eyes->FieldOfView);
				}
			}
			Camera->SetActorLocationAndRotation(Climb.StartEye, Climb.StartLook);
		}
		// Hands on the rope: the grab, then the rope's creak while the climb lasts.
		LooterSound::PlayAt(Scenes, LooterSoundCue::Mantle, Ladder->GetTopRung());
		Climb.Creak = LooterSound::Start(Scenes, LooterSoundCue::ShipCreak, Ladder->GetRootComponent(), NAME_None, 0.6f, 1.15f);
		UE_LOG(LogLooter, Log, TEXT("Ferry ladder: climbing %s (%.1f s)."), Climb.bDown ? TEXT("down") : TEXT("up"), FerryLadderClimb::Duration());
	}

	void MoveClimb(FClimbState& Climb, float Seconds)
	{
		USceneSubsystem* Scenes = Climb.Scenes.Get();
		ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr;
		if (!Camera)
		{
			return;
		}
		const FVector Eye = FerryLadderClimb::EyeAt(Seconds, Climb.StartEye, Climb.OnFrom, Climb.OnTo, Climb.EndEye, AFerryLadder::RungCount);
		FQuat View = Climb.FaceLadder;
		if (Seconds < FerryLadderClimb::TurnSeconds)
		{
			View = FQuat::Slerp(Climb.StartLook, Climb.FaceLadder, FMath::SmoothStep(0.f, 1.f, Seconds / FerryLadderClimb::TurnSeconds));
		}
		else if (Seconds > FerryLadderClimb::TurnSeconds + FerryLadderClimb::ClimbSeconds)
		{
			const float Off = (Seconds - FerryLadderClimb::TurnSeconds - FerryLadderClimb::ClimbSeconds) / FerryLadderClimb::StepOffSeconds;
			View = FQuat::Slerp(Climb.FaceLadder, Climb.EndLook, FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Off, 0.f, 1.f)));
		}
		Camera->SetActorLocationAndRotation(Eye, View);
	}
}

FName FerryLadderClimb::SceneName()
{
	return FName(TEXT("FerryLadder"));
}

float FerryLadderClimb::Duration()
{
	return TurnSeconds + ClimbSeconds + StepOffSeconds;
}

float FerryLadderClimb::RungAlpha(float Alpha, int32 Rungs)
{
	const int32 Count = FMath::Max(Rungs, 1);
	const float Along = FMath::Clamp(Alpha, 0.f, 1.f) * Count;
	const float Rung = FMath::FloorToFloat(Along);
	return FMath::Clamp((Rung + FMath::SmoothStep(0.f, 1.f, Along - Rung)) / Count, 0.f, 1.f);
}

FVector FerryLadderClimb::EyeAt(float Seconds, const FVector& StartEye, const FVector& OnFrom, const FVector& OnTo, const FVector& EndEye,
	int32 Rungs)
{
	if (Seconds <= TurnSeconds)
	{
		return FMath::Lerp(StartEye, OnFrom, static_cast<double>(FMath::SmoothStep(0.f, 1.f, FMath::Max(Seconds, 0.f) / TurnSeconds)));
	}
	const float OnLadder = Seconds - TurnSeconds;
	if (OnLadder <= ClimbSeconds)
	{
		return FMath::Lerp(OnFrom, OnTo, static_cast<double>(RungAlpha(OnLadder / ClimbSeconds, Rungs)));
	}
	const float Off = FMath::Clamp((OnLadder - ClimbSeconds) / StepOffSeconds, 0.f, 1.f);
	return FMath::Lerp(OnTo, EndEye, static_cast<double>(FMath::SmoothStep(0.f, 1.f, Off)));
}

FScenePlay FerryLadderClimb::Make(USceneSubsystem& Scenes, AFerryLadder& Ladder, AActor& Climber, bool bDown)
{
	const TSharedRef<FClimbState> Climb = MakeShared<FClimbState>();
	Climb->Scenes = &Scenes;
	Climb->Ladder = &Ladder;
	Climb->Climber = &Climber;
	Climb->bDown = bDown;
	const float Total = Duration();

	FScenePlay Scene;
	Scene.Name = SceneName();
	// The climb has the view (the turn to the ladder and the step off are its own): only the skip keys work.
	Scene.bLookOnly = false;
	Scene.OnStart = [Climb]() { StartClimb(*Climb); };
	Scene.Timeline.AddMove(0.f, Total, [Climb, Total](float Alpha) { MoveClimb(*Climb, Alpha * Total); });
	// Feet on the planks (or the boards): the landing's thump.
	Scene.Timeline.AddMoment(FName(TEXT("StepOff")), TurnSeconds + ClimbSeconds, [Climb]()
	{
		const USceneSubsystem* Playing = Climb->Scenes.Get();
		if (Playing && !Playing->IsSkipping())
		{
			LooterSound::PlayAt(Playing, LooterSoundCue::Land, Climb->EndFeet, 0.7f);
		}
	});
	Scene.AfterEnd = [Climb]()
	{
		LooterSound::Stop(Climb->Creak.Get(), 0.2f);
		Climb->Creak.Reset();
	};
	return Scene;
}
