// BargeFall: the dead barge over the Rest and down into Gravewind Canyon, seen from the Lamplighter's deck (Main 6 step 7).

#include "Scenes/BargeFall.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Scenes/SceneSubsystem.h"
#include "World/BargeBoarding.h"
#include "World/BargeRules.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterRules.h"
#include "World/LamplighterShip.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The streak to the boards (s), the words, the view's hand-back, the end. */
	constexpr float StreakFrom = 0.3f;
	constexpr float StreakSeconds = 2.6f;
	constexpr float WordsAt = 1.5f;
	constexpr float HandBackAt = 11.8f;
	constexpr float LookBack = 0.6f;
	/** The lens: wide on her close, closing in as she goes far (degrees). */
	constexpr float NearFov = 75.f;
	constexpr float FarFov = 38.f;
	/** Puffs along the streak this often (s). */
	constexpr float StreakPuffEvery = 0.06f;
	const FName FallLines(TEXT("DA_Lines_Main6Fall"));

	struct FFallState
	{
		TWeakObjectPtr<USceneSubsystem> Scenes;
		TWeakObjectPtr<ABargeBoarding> Barge;
		FVector Eye = FVector::ZeroVector;
		FVector Start = FVector::ZeroVector;
		FVector Boards = FVector::ZeroVector;
		FVector LookAt = FVector::ZeroVector;
		float Clock = 0.f;
		float PuffWait = 0.f;
	};
	using FFallRef = TSharedRef<FFallState>;

	bool IsSkipping(const FFallState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		return !Scenes || Scenes->IsSkipping();
	}

	void SetView(FFallState& State, const FVector& Look, float Fov, float DeltaAlpha)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr;
		if (!Camera)
		{
			return;
		}
		// The eyes follow her a beat behind, as anyone watching a ship go down does.
		State.LookAt = State.LookAt.IsNearlyZero() ? Look : FMath::Lerp(State.LookAt, Look, static_cast<double>(FMath::Clamp(DeltaAlpha, 0.f, 1.f)));
		Camera->SetActorLocationAndRotation(State.Eye, (State.LookAt - State.Eye).Rotation());
		if (UCameraComponent* Lens = Camera->GetCameraComponent())
		{
			Lens->SetFieldOfView(Fov);
		}
	}
}

FName BargeFall::SceneName()
{
	static const FName Name(TEXT("BargeFall"));
	return Name;
}

float BargeFall::Duration()
{
	return BargeRules::FallSeconds + LookBack;
}

bool BargeFall::Play(ABargeBoarding& Barge, ALamplighterShip* Ship)
{
	USceneSubsystem* Scenes = USceneSubsystem::Get(&Barge);
	UWorld* World = Barge.GetWorld();
	if (!Scenes || !World || !USceneSubsystem::AreScenesOn(World) || Scenes->IsPlaying())
	{
		Barge.FinishFall();
		if (Scenes && !Scenes->HasPlayed(SceneName()))
		{
			Scenes->MarkPlayed(SceneName());
		}
		return false;
	}
	const FFallRef State = MakeShared<FFallState>();
	State->Scenes = Scenes;
	State->Barge = &Barge;
	State->Start = Barge.GetDeckMiddle();
	State->Boards = Barge.BoardsPoint.IsNearlyZero() ? State->Start + FVector(0.0, -20000.0, -2500.0) : Barge.BoardsPoint;
	TWeakObjectPtr<ALamplighterShip> WeakShip = Ship;

	FScenePlay Scene;
	Scene.Name = SceneName();
	Scene.bLookOnly = false;
	Scene.OnStart = [State, WeakShip]()
	{
		USceneSubsystem* Played = State->Scenes.Get();
		if (!Played)
		{
			return;
		}
		// From Sully's eyes on her deck (the ship holds still through it).
		const APawn* Player = Played->GetHeldPawn();
		if (Player)
		{
			State->Eye = Player->GetActorLocation() + FVector(0.0, 0.0, Player->BaseEyeHeight);
		}
		else if (const ALamplighterShip* Ship = WeakShip.Get())
		{
			State->Eye = Ship->DeckPoint(FVector(0.0, 0.0, 170.0));
		}
		else
		{
			State->Eye = State->Start + FVector(0.0, -3000.0, -500.0);
		}
		Played->ViewFromSceneCamera(0.6f);
		SetView(*State, State->Start, NearFov, 1.f);
	};
	Scene.OnTick = [State](float DeltaSeconds) { State->Clock += DeltaSeconds; };
	FSceneTimeline& Timeline = Scene.Timeline;

	// The fall itself: her pose moment by moment (BargeRules::FallPose), the view on her.
	Timeline.AddMove(0.f, BargeRules::FallSeconds, [State](float Alpha)
	{
		const float Seconds = Alpha * BargeRules::FallSeconds;
		ABargeBoarding* Barge = State->Barge.Get();
		if (!Barge)
		{
			return;
		}
		Barge->ApplyFallAt(Seconds);
		const float Far = FMath::SmoothStep(0.f, BargeRules::PlungeAt, Seconds);
		SetView(*State, Barge->GetDeckMiddle(), FMath::Lerp(NearFov, FarFov, Far), 0.12f);
	});
	// Ahead of her, the black streak coming down on the boards.
	Timeline.AddMove(StreakFrom, StreakSeconds, [State](float Alpha)
	{
		if (IsSkipping(*State))
		{
			return;
		}
		State->PuffWait -= 1.f / 60.f;
		if (State->PuffWait > 0.f)
		{
			return;
		}
		State->PuffWait = StreakPuffEvery;
		// Arcing high and dropping onto the point.
		const FVector At = FMath::Lerp(State->Start, State->Boards, static_cast<double>(Alpha)) + FVector(0.0, 0.0, 1800.0 * FMath::Sin(UE_PI * Alpha));
		LamplighterEffects::SmokePuff(State->Scenes.Get(), At, FVector(0.0, -300.0, -100.0));
	});
	Timeline.AddMoment(TEXT("Words"), WordsAt, [State]()
	{
		if (!IsSkipping(*State))
		{
			LandfallRules::Say(State->Scenes.Get(), FallLines);
		}
	});
	Timeline.AddMoment(TEXT("Impact"), BargeRules::ImpactAt, [State]()
	{
		if (!IsSkipping(*State))
		{
			// Heard across the valley as well as seen: a quieter boom close by the listener.
			LooterSound::Play2D(State->Scenes.Get(), LooterSoundCue::Launcher::ExplosionDistant, 0.6f, 0.7f);
		}
	});
	Timeline.AddMoment(TEXT("HandBack"), HandBackAt, [State]()
	{
		if (USceneSubsystem* Played = State->Scenes.Get())
		{
			Played->ViewFromPlayer(Played->IsSkipping() ? 0.f : 0.6f);
		}
	});
	Timeline.AddMoment(TEXT("End"), Duration());
	Scene.AfterEnd = [State]()
	{
		if (ABargeBoarding* Barge = State->Barge.Get())
		{
			Barge->FinishFall();
		}
	};
	const bool bPlays = Scenes->Play(MoveTemp(Scene));
	if (!bPlays)
	{
		Barge.FinishFall();
		if (!Scenes->HasPlayed(SceneName()))
		{
			Scenes->MarkPlayed(SceneName());
		}
	}
	else
	{
		UE_LOG(LogLooter, Log, TEXT("BargeFall: the scene plays (%.0f s)."), Duration());
	}
	return bPlays;
}
