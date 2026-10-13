// OreLiftRide: a ride on the ore lift as a scene: the boarding, the view riding the cage, the rider let go at the end.

#include "Scenes/OreLiftRide.h"
#include "AI_Looter_Shooter.h"
#include "Player/PlayerViewComponent.h"
#include "Story/CaptionSubsystem.h"
#include "World/OreLift.h"
#include "World/OreLiftMotion.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** A view leaning or crouched as the ride began is stood up straight in the cage: the eyes' reach from the feet (cm). */
	constexpr double EyeAheadMost = 40.0;
	constexpr double EyeLowest = 60.0;
	constexpr double EyeHighest = 200.0;

	/** What the ride's moves and moments share; they outlive the call that made them. */
	struct FRideState
	{
		TWeakObjectPtr<USceneSubsystem> Scenes;
		TWeakObjectPtr<AOreLift> Lift;
		TWeakObjectPtr<AActor> Rider;
		EOreLiftEnd From = EOreLiftEnd::Bottom;
		EOreLiftEnd To = EOreLiftEnd::Top;
		bool bFirstRide = false;
		/** The rider's eyes over their feet, in their own yaw's frame, and where they were as the ride began. */
		FVector EyeOffset = FVector(0.0, 0.0, OreLiftRide::FallbackEyeHeight);
		FVector StartEye = FVector::ZeroVector;
		/** The look as the ride began, and the one the boarding turns it to (out of the cage). */
		FQuat StartLook = FQuat::Identity;
		FQuat BoardLook = FQuat::Identity;
		bool bBoarded = false;
	};

	APlayerController* ControllerOf(const FRideState& Ride)
	{
		const USceneSubsystem* Scenes = Ride.Scenes.Get();
		return Scenes ? Scenes->GetHeldController() : nullptr;
	}

	/** As it starts: the rider's eyes measured, their body laid on the far landing unseen, the view on the scene's camera. */
	void StartRide(FRideState& Ride)
	{
		USceneSubsystem* Scenes = Ride.Scenes.Get();
		AOreLift* Lift = Ride.Lift.Get();
		AActor* Rider = Ride.Rider.Get();
		if (!Scenes || !Lift || !Rider)
		{
			return;
		}
		const APlayerController* Controller = ControllerOf(Ride);
		const FRotator Look = Controller ? Controller->GetControlRotation() : Rider->GetActorRotation();

		// Their eyes where they stand, off their own camera, so the view starts there and ends at their body's own eyes.
		const UCameraComponent* Eyes = UPlayerViewComponent::FindFirstPersonCamera(Rider);
		const FVector Feet = Rider->GetActorLocation() - FVector(0.0, 0.0, Rider->GetSimpleCollisionHalfHeight());
		const FRotator Facing(0.0, Look.Yaw, 0.0);
		if (Eyes)
		{
			FVector Offset = Facing.UnrotateVector(Eyes->GetComponentLocation() - Feet);
			Offset.X = FMath::Clamp(Offset.X, -EyeAheadMost, EyeAheadMost);
			Offset.Y = 0.0;
			Offset.Z = FMath::Clamp(Offset.Z, EyeLowest, EyeHighest);
			Ride.EyeOffset = Offset;
			Ride.StartEye = Eyes->GetComponentLocation();
		}
		else
		{
			Ride.StartEye = Feet + Facing.RotateVector(Ride.EyeOffset);
		}
		Ride.StartLook = FQuat(Look);
		Ride.BoardLook = FQuat(FRotator(OreLiftRide::BoardPitch, Lift->GetCageRest(Ride.From).Rotator().Yaw, 0.0));

		// The body waits unseen on the far landing; the view rides the cage, at the player's own field of view.
		Scenes->SetHeldPlayerHidden(true);
		Lift->ParkRider(Ride.To);
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
		}
		if (Ride.bFirstRide && !Lift->FirstRideLines.IsEmpty())
		{
			if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(Scenes))
			{
				Captions->Play(Lift->FirstRideLines, ECaptionPlay::Interrupt);
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Ore lift ride: %.0f m of cable in %.1f s%s."), Lift->GetRun().Length / 100.f,
			OreLiftRide::DurationFor(Lift->GetRun()), Ride.bFirstRide ? TEXT(", the first ride") : TEXT(""));
	}

	/** Seconds into the ride: the cage where its run has it, the view at the rider's eye in it, swayed and jolted with it. */
	void MoveRide(FRideState& Ride, float Seconds)
	{
		USceneSubsystem* Scenes = Ride.Scenes.Get();
		AOreLift* Lift = Ride.Lift.Get();
		if (!Scenes || !Lift)
		{
			return;
		}
		Lift->SetRunTime(FMath::Max(Seconds - OreLiftRide::BoardSeconds, 0.f), Scenes->IsSkipping());

		// Stepping in, the rider turns to face out of the cage: a second, then the look is theirs again.
		APlayerController* Controller = ControllerOf(Ride);
		const float Boarding = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Seconds / OreLiftRide::BoardSeconds, 0.f, 1.f));
		if (Controller && !Ride.bBoarded)
		{
			const FRotator Turned = FQuat::Slerp(Ride.StartLook, Ride.BoardLook, Boarding).Rotator();
			Controller->SetControlRotation(FRotator(Turned.Pitch, Turned.Yaw, 0.0));
			Ride.bBoarded = Seconds >= OreLiftRide::BoardSeconds;
		}
		const FRotator Look = Controller ? Controller->GetControlRotation() : Ride.BoardLook.Rotator();
		ACameraActor* Camera = Scenes->GetSceneCamera();
		if (!Camera)
		{
			return;
		}
		FVector Eye = Lift->GetRiderEye(Ride.EyeOffset, Look.Yaw);
		if (Boarding < 1.f)
		{
			Eye = FMath::Lerp(Ride.StartEye, Eye, static_cast<double>(Boarding));
		}
		// Tipped with the cage's swing, by a braced rider's share, and jolted through the floor at each clank.
		const FOreLiftSway& Sway = Lift->GetSway();
		const FVector Heading = Lift->GetRunHeading();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Heading);
		const FQuat Tilt = FQuat(Right, FMath::DegreesToRadians(Sway.Along * OreLiftRide::ViewSwayShare))
			* FQuat(Heading, -FMath::DegreesToRadians(Sway.Side * OreLiftRide::ViewSwayShare));
		FQuat View = Tilt * FQuat(Look);
		View = FQuat(View.GetForwardVector(), FMath::DegreesToRadians(Sway.Jolt)) * View;
		Eye.Z += Sway.Jolt * OreLiftRide::JoltLift;
		Camera->SetActorLocationAndRotation(Eye, View);
	}
}

FName OreLiftRide::SceneName()
{
	return FName(TEXT("OreLift"));
}

FName OreLiftRide::DepartMoment()
{
	return FName(TEXT("Depart"));
}

FName OreLiftRide::TowerMoment()
{
	return FName(TEXT("Tower"));
}

FName OreLiftRide::ArriveMoment()
{
	return FName(TEXT("Arrive"));
}

float OreLiftRide::DurationFor(const FOreLiftRunShape& Run)
{
	return BoardSeconds + Run.GetDuration();
}

FScenePlay OreLiftRide::Make(USceneSubsystem& Scenes, AOreLift& Lift, AActor& Rider, bool bFirstRide)
{
	const TSharedRef<FRideState> Ride = MakeShared<FRideState>();
	Ride->Scenes = &Scenes;
	Ride->Lift = &Lift;
	Ride->Rider = &Rider;
	Ride->From = Lift.GetRunFrom();
	Ride->To = Ride->From == EOreLiftEnd::Bottom ? EOreLiftEnd::Top : EOreLiftEnd::Bottom;
	Ride->bFirstRide = bFirstRide;
	const FOreLiftRunShape& Run = Lift.GetRun();
	const float Total = DurationFor(Run);

	FScenePlay Scene;
	Scene.Name = SceneName();
	Scene.bLookOnly = true;
	Scene.OnStart = [Ride]() { StartRide(*Ride); };
	Scene.Timeline.AddMove(0.f, Total, [Ride, Total](float Alpha) { MoveRide(*Ride, Alpha * Total); });

	// Its moments (the cage's own sounds are the lift's, as each passes): off the landing, over the tower, set down.
	Scene.Timeline.AddMoment(DepartMoment(), BoardSeconds);
	const TArray<float> Clanks = OreLiftMotion::ClankTimes(Run);
	if (!Clanks.IsEmpty())
	{
		Scene.Timeline.AddMoment(TowerMoment(), BoardSeconds + Clanks[Clanks.Num() / 2]);
	}
	Scene.Timeline.AddMoment(ArriveMoment(), BoardSeconds + Run.GetArrival());

	// Played or skipped, the rider is let go on the far landing (after the scene has given the player back).
	Scene.AfterEnd = [Ride]()
	{
		if (AOreLift* Lift = Ride->Lift.Get())
		{
			Lift->FinishRide();
		}
	};
	return Scene;
}
