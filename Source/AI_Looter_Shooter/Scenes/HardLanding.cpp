// HardLanding: the Lamplighter's fall onto the Mooring Ledge with her bag torn: her path as math of time, and the scene.

#include "Scenes/HardLanding.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Player/ViewKick.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterShip.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The skid's judder: fine and quick, dying as she stops (cm, Hz). */
	constexpr float JudderCm = 3.f;
	constexpr float JudderHz = 17.f;

	/** What the scene's moves and moments share; they outlive the call that made them. */
	struct FLandingState
	{
		TWeakObjectPtr<USceneSubsystem> Scenes;
		TWeakObjectPtr<ALamplighterShip> Ship;
		/** Her heading as the player's view last turned with it (degrees). */
		float LastYaw = 0.f;
	};

	bool IsSkipping(const FLandingState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		return Scenes && Scenes->IsSkipping();
	}

	/** A value that dies away to nothing over the last moments, so she lies exactly at Moored by the end. */
	float Fade(float Seconds)
	{
		return 1.f - FMath::SmoothStep(HardLanding::TotalSeconds - 0.35f, HardLanding::TotalSeconds, Seconds);
	}
}

FName HardLanding::SceneName()
{
	static const FName Name(TEXT("HardLanding"));
	return Name;
}

float HardLanding::DropAt(float Seconds)
{
	const float Share = FMath::Clamp(Seconds / TouchAt, 0.f, 1.f);
	// Falling: slow while the bag still holds some lift, then faster and faster into the hit.
	return Share * Share;
}

float HardLanding::GlideAt(float Seconds)
{
	const float Share = FMath::Clamp(Seconds / (TouchAt + SkidSeconds), 0.f, 1.f);
	// Smootherstep: out of the stop with no jolt, slowing to nothing as the skid ends.
	return Share * Share * Share * (Share * (Share * 6.f - 15.f) + 10.f);
}

FTransform HardLanding::PoseAt(const FTransform& From, const FTransform& Moored, float Seconds, float TornSide)
{
	if (Seconds >= TotalSeconds)
	{
		return Moored;
	}
	const float S = FMath::Max(Seconds, 0.f);
	const float Glide = GlideAt(S);
	const float Drop = DropAt(S);
	const float Since = S - TouchAt;
	const float Fading = Fade(S);

	FVector Location = FMath::Lerp(From.GetLocation(), Moored.GetLocation(), static_cast<double>(Glide));
	Location.Z = FMath::Lerp(From.GetLocation().Z, Moored.GetLocation().Z, static_cast<double>(Drop));

	float NoseDown = 0.f;
	float List = 0.f;
	if (Since < 0.f)
	{
		// Nose down as she goes over, the flare at the last moment; listing more and more to her torn side.
		NoseDown = MaxNoseDown * FMath::Sin(UE_PI * FMath::Clamp(S / (TouchAt * 0.85f), 0.f, 1.f))
			- Flare * FMath::SmoothStep(TouchAt - 1.2f, TouchAt - 0.15f, S);
		List = MaxList * FMath::SmoothStep(0.f, 3.5f, S);
	}
	else
	{
		// The hit: squashed into the ledge and bouncing back, the list slammed out of her, rocking; the skid's judder.
		const float Decay = FMath::Exp(-4.f * Since);
		Location.Z -= Squash * Decay * FMath::Sin(UE_TWO_PI * 1.3f * Since + 0.15f) * Fading;
		if (Since < SkidSeconds)
		{
			const float Judder = JudderCm * (1.f - Since / SkidSeconds);
			Location.Z += Judder * FMath::Sin(UE_TWO_PI * JudderHz * Since);
			Location += Moored.GetRotation().GetRightVector() * (Judder * FMath::Sin(UE_TWO_PI * JudderHz * 0.7f * Since + 1.f));
		}
		NoseDown = (-Flare * (1.f - FMath::SmoothStep(0.f, 0.3f, Since)) + 2.f * Decay * FMath::Sin(UE_TWO_PI * 1.1f * Since)) * Fading;
		List = (MaxList * (1.f - FMath::SmoothStep(0.f, 0.35f, Since)) - 3.f * FMath::Exp(-3.5f * Since) * FMath::Sin(UE_TWO_PI * 0.9f * Since))
			* Fading;
	}
	const FQuat Heading = FQuat::Slerp(From.GetRotation(), Moored.GetRotation(), Glide);
	// In her own frame: a turn about X lifts her +Y side, so listing toward TornSide is the other way; about Y dips her nose.
	const FQuat Rock = FQuat(FVector::XAxisVector, FMath::DegreesToRadians(-TornSide * List))
		* FQuat(FVector::YAxisVector, FMath::DegreesToRadians(NoseDown));
	return FTransform(Heading * Rock, Location, Moored.GetScale3D());
}

FScenePlay HardLanding::Make(USceneSubsystem& Scenes, ALamplighterShip& Ship)
{
	const TSharedRef<FLandingState> State = MakeShared<FLandingState>();
	State->Scenes = &Scenes;
	State->Ship = &Ship;
	State->LastYaw = Ship.GetActorRotation().Yaw;

	FScenePlay Scene;
	Scene.Name = SceneName();
	// Through their own eyes, free to look: they ride it down holding on.
	Scene.bLookOnly = true;
	Scene.OnStart = [State]()
	{
		USceneSubsystem* Played = State->Scenes.Get();
		ALamplighterShip* Boat = State->Ship.Get();
		APawn* Pawn = Played ? Played->GetHeldPawn() : nullptr;
		if (!Played || !Boat || !Pawn)
		{
			return;
		}
		// Held where they stand, fixed to her deck, facing the way they looked.
		const APlayerController* Controller = Played->GetHeldController();
		const float Yaw = Controller ? Controller->GetControlRotation().Yaw : Pawn->GetActorRotation().Yaw;
		const FVector Feet = Pawn->GetActorLocation() - FVector(0.0, 0.0, Pawn->GetSimpleCollisionHalfHeight());
		Played->CarryPlayer(Boat->GetDeck(), Feet, Yaw);
		State->LastYaw = Boat->GetActorRotation().Yaw;
	};

	Scene.Timeline.AddMove(0.f, SceneSeconds, [State](float Alpha)
	{
		ALamplighterShip* Boat = State->Ship.Get();
		if (!Boat)
		{
			return;
		}
		Boat->ApplyLanding(BreakSeconds + Alpha * SceneSeconds);
		// The view turns with her as she swings onto the ledge's line (never rocked: the shakes do that).
		const float Yaw = Boat->GetActorRotation().Yaw;
		if (USceneSubsystem* Played = State->Scenes.Get())
		{
			Played->TurnHeldView(FMath::FindDeltaAngleDegrees(State->LastYaw, Yaw));
		}
		State->LastYaw = Yaw;
	});

	Scene.Timeline.AddMoment(TEXT("Drop"), 0.f, [State]()
	{
		ALamplighterShip* Boat = State->Ship.Get();
		if (!Boat || IsSkipping(*State))
		{
			return;
		}
		// The stomach-drop as what's left of the lift goes.
		LooterSound::PlayAt(Boat, LooterSoundCue::ShipLurch, Boat->GetActorLocation(), 0.8f, 0.85f);
		LamplighterEffects::Shake(Boat, 0.5f, 1.2f);
		FViewKick Jolt;
		Jolt.Pitch = -2.f;
		Jolt.FieldOfView = 0.03f;
		Jolt.Frequency = 4.f;
		Jolt.Damping = 0.45f;
		LamplighterEffects::Kick(Boat, Jolt);
	});
	Scene.Timeline.AddMoment(TEXT("Brace"), TouchAt - BreakSeconds - 1.2f, [State]()
	{
		ALamplighterShip* Boat = State->Ship.Get();
		if (!Boat || IsSkipping(*State))
		{
			return;
		}
		// The ledge coming up fast: the crew grab hold, the timbers scream.
		Boat->BraceCrew(3.f);
		LamplighterEffects::Shake(Boat, 0.6f, 1.2f);
	});
	Scene.Timeline.AddMoment(TEXT("Touch"), TouchAt - BreakSeconds, [State]()
	{
		if (ALamplighterShip* Boat = State->Ship.Get())
		{
			Boat->Touchdown(IsSkipping(*State));
		}
	});
	Scene.Timeline.AddMoment(TEXT("Settled"), SceneSeconds, [State]()
	{
		if (ALamplighterShip* Boat = State->Ship.Get())
		{
			Boat->FinishLanding();
		}
	});
	return Scene;
}
