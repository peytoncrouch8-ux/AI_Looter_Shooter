// A boss intro as a scene (USceneSubsystem): the boss and the creatures round it held still, the camera's shots, the
// letterbox and the name card, the captions, the music; the boss let go for its own entrance, the view eased back to the
// player's eyes facing it, its bar swept in again. The scripts are BossIntro.cpp's, each boss's own moments
// BossIntroBosses.cpp's.

#include "Scenes/BossIntro.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossComponent.h"
#include "Scenes/BossIntroFreeze.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "UI/HUD/HudBossBarWidget.h"
#include "UI/Story/IntroWidget.h"
#include "World/WorldQueries.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The music's bed fades under the reveal's hit this fast, and goes this fast on a skip. */
	constexpr float MusicOutSeconds = 1.0f;
	constexpr float MusicSkipSeconds = 0.4f;

	/** With nobody to frame from (a test level): the player's eyes this far (cm) in front of the boss, this high. */
	constexpr float NoPlayerDistance = 1500.f;
	constexpr float NoPlayerEye = 170.f;

	/** What the scene's moves and moments share; it outlives the call that made them. */
	struct FIntroState
	{
		EBossIntro Kind = EBossIntro::Gravemother;
		FBossIntroScript Script;
		FBossIntroActs Acts;
		FBossIntroFrame Frame;
		TArray<FStoryLine> Lines;
		FBossIntroFreeze Freeze;

		TWeakObjectPtr<USceneSubsystem> Scenes;
		TWeakObjectPtr<AActor> Boss;
		TWeakObjectPtr<UBossComponent> Fight;
		TWeakObjectPtr<UIntroWidget> Widget;
		TWeakObjectPtr<UAudioComponent> Music;

		/** Where the boss stood once its entrance began (the tracking shots follow it from there; before, nothing moves). */
		FVector TrackFrom = FVector::ZeroVector;
		bool bLive = false;
		bool bSkipped = false;
		bool bOwnLineSaid = false;
		/** The scene camera's own settings (it's shared by every scene), given back at the end. */
		bool bCameraSaved = false;
		float SavedFov = 90.f;
		bool bSavedExposureOn = false;
		float SavedExposure = 0.f;
		float SavedBlendWeight = 1.f;
		bool bUsesExposure = false;
	};
	using FStateRef = TSharedRef<FIntroState>;

	bool IsSkipping(const FIntroState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		return !Scenes || Scenes->IsSkipping();
	}

	/** Its feet: a character's capsule bottom, else where it stands. */
	FVector FeetOf(const AActor& Actor)
	{
		const ACharacter* Character = Cast<ACharacter>(&Actor);
		const float Half = Character && Character->GetCapsuleComponent() ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 0.f;
		return Actor.GetActorLocation() - FVector(0.0, 0.0, Half);
	}

	APlayerCameraManager* CameraManager(const FIntroState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		const APlayerController* Controller = Scenes ? Scenes->GetHeldController() : nullptr;
		return Controller ? Controller->PlayerCameraManager.Get() : nullptr;
	}

	/** The frame from where the boss and the player stand as it starts (the held player's eyes; none: in front of the boss). */
	FBossIntroFrame MakeFrame(const FIntroState& State, const AActor& Boss)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		const APawn* Player = Scenes ? Scenes->GetHeldPawn() : nullptr;
		const FVector Feet = FeetOf(Boss);
		const FVector Eye = Player ? Player->GetPawnViewLocation()
			: Feet + Boss.GetActorForwardVector() * NoPlayerDistance + FVector(0.0, 0.0, NoPlayerEye);
		return FBossIntroFrame::Make(Feet, Eye, State.Acts.bHasBurst ? State.Acts.BurstFeet : Feet, Boss.GetActorForwardVector());
	}

	void ApplyShot(FIntroState& State, int32 Index, float Alpha)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr;
		if (!Camera || !State.Script.Shots.IsValidIndex(Index))
		{
			return;
		}
		const AActor* Boss = State.Boss.Get();
		const FVector Shift = State.bLive && Boss ? Boss->GetActorLocation() - State.TrackFrom : FVector::ZeroVector;
		const FBossIntroShot& Shot = State.Script.Shots[Index];
		FBossIntroView View = BossIntro::ViewAlong(Shot, State.Frame, Alpha, Shift);

		// The eye kept in front of the den's roof, the stope's walls, a rise of the deck: whatever stands between it and its mark.
		const FVector Mark = FMath::Lerp(State.Frame.Resolve(Shot.FromLook), State.Frame.Resolve(Shot.ToLook), static_cast<double>(BossIntro::Ease(Alpha)))
			+ (Shot.bTrackBoss ? Shift : FVector::ZeroVector);
		if (UWorld* World = Camera->GetWorld())
		{
			FHitResult Hit;
			const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("BossIntroBoom"));
			// A mark inside the rock itself (a low stope roof over a tall boss) says nothing about the way to the eye: left as is.
			if (World->LineTraceSingleByObjectType(Hit, Mark, View.Location, FCollisionObjectQueryParams(ECC_WorldStatic), Params)
				&& !Hit.bStartPenetrating)
			{
				View.Location = BossIntro::BoomClip(Mark, View.Location, Hit.Time);
				View.Rotation = (Mark - View.Location).Rotation();
				View.Rotation.Roll = 0.f;
			}
		}
		Camera->SetActorLocationAndRotation(View.Location, View.Rotation);
		if (UCameraComponent* Lens = Camera->GetCameraComponent())
		{
			Lens->SetFieldOfView(View.Fov);
			if (State.bUsesExposure)
			{
				Lens->PostProcessSettings.bOverride_AutoExposureBias = true;
				Lens->PostProcessSettings.AutoExposureBias = View.Exposure;
				Lens->PostProcessBlendWeight = 1.f;
			}
		}
	}

	/** A cut's black dip: out before it, in after. Nothing on a skip (the end clears it). */
	void Dip(const FIntroState& State, bool bOut)
	{
		APlayerCameraManager* Camera = CameraManager(State);
		if (!Camera || IsSkipping(State))
		{
			return;
		}
		if (bOut)
		{
			Camera->StartCameraFade(0.f, 1.f, BossIntro::CutDipSeconds, FLinearColor::Black, /*bShouldFadeAudio*/ false, /*bHoldWhenFinished*/ true);
		}
		else
		{
			Camera->StartCameraFade(1.f, 0.f, BossIntro::CutDipSeconds, FLinearColor::Black, false, false);
		}
	}

	void Say(FIntroState& State, int32 Line)
	{
		if (IsSkipping(State))
		{
			return;
		}
		UCaptionSubsystem* Captions = UCaptionSubsystem::Get(State.Scenes.Get());
		if (!Captions)
		{
			return;
		}
		if (Line == BossIntro::OwnLine)
		{
			if (!State.Acts.OwnLine.Text.IsEmpty())
			{
				Captions->Play({ State.Acts.OwnLine }, ECaptionPlay::Interrupt);
				State.bOwnLineSaid = true;
			}
			return;
		}
		if (State.Lines.IsValidIndex(Line))
		{
			Captions->Play({ State.Lines[Line] }, ECaptionPlay::Interrupt);
		}
	}

	void SaveCamera(FIntroState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr;
		UCameraComponent* Lens = Camera ? Camera->GetCameraComponent() : nullptr;
		if (!Lens)
		{
			return;
		}
		State.bCameraSaved = true;
		State.SavedFov = Lens->FieldOfView;
		State.bSavedExposureOn = Lens->PostProcessSettings.bOverride_AutoExposureBias;
		State.SavedExposure = Lens->PostProcessSettings.AutoExposureBias;
		State.SavedBlendWeight = Lens->PostProcessBlendWeight;
	}

	void RestoreCamera(FIntroState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr;
		UCameraComponent* Lens = Camera ? Camera->GetCameraComponent() : nullptr;
		if (!Lens || !State.bCameraSaved)
		{
			return;
		}
		// The next scene finds the camera as it was (the Sit with Pa never sets a lens or an exposure of its own).
		Lens->SetFieldOfView(State.SavedFov);
		Lens->PostProcessSettings.bOverride_AutoExposureBias = State.bSavedExposureOn;
		Lens->PostProcessSettings.AutoExposureBias = State.SavedExposure;
		Lens->PostProcessBlendWeight = State.SavedBlendWeight;
		State.bCameraSaved = false;
	}

	void OnStart(const FStateRef& State)
	{
		USceneSubsystem* Scenes = State->Scenes.Get();
		AActor* Boss = State->Boss.Get();
		if (!Scenes || !Boss)
		{
			return;
		}
		// Held at once, inside the fight's start: its first move never gets a frame.
		State->Freeze.Hold(*Boss, BossIntro::FreezeDilation);
		State->Frame = MakeFrame(*State, *Boss);
		State->TrackFrom = Boss->GetActorLocation();
		if (UBossComponent* Fight = State->Fight.Get())
		{
			// Its bar sweeps in again as the view comes back (HandBack): not under the letterbox.
			if (UHudBossBarWidget* Bar = Fight->GetBar())
			{
				Bar->SetShown(false);
			}
		}
		// The player's body stays where they stand, unseen (the first shot looks past their shoulder).
		Scenes->SetHeldPlayerHidden(true);
		Scenes->ViewFromSceneCamera(BossIntro::EnterBlendSeconds);
		SaveCamera(*State);
		ApplyShot(*State, 0, 0.f);
		if (APlayerController* Controller = Scenes->GetHeldController(); Controller && Controller->IsLocalController())
		{
			if (UIntroWidget* Widget = CreateWidget<UIntroWidget>(Controller))
			{
				Widget->AddToViewport(UIntroWidget::ViewportZOrder);
				Widget->SetLetterbox(true);
				State->Widget = Widget;
			}
		}
		if (State->Script.Music)
		{
			State->Music = LooterSound::Start(Scenes, FName(State->Script.Music));
		}
		UE_LOG(LogLooter, Log, TEXT("Boss intro: %s plays (%.1f s)."), BossIntro::KindName(State->Kind), State->Script.Seconds);
	}

	void GoLive(FIntroState& State)
	{
		AActor* Boss = State.Boss.Get();
		const bool bSkipping = IsSkipping(State);
		// The boss alone runs again: its entrance is its own (the climb, the slam, the lantern), played on camera.
		State.Freeze.Release(Boss);
		if (State.Acts.Live)
		{
			State.Acts.Live(bSkipping);
		}
		State.bLive = true;
		State.TrackFrom = Boss ? Boss->GetActorLocation() : FVector::ZeroVector;
		LooterSound::Stop(State.Music.Get(), bSkipping ? MusicSkipSeconds : MusicOutSeconds);
		if (!bSkipping)
		{
			LooterSound::Play2D(State.Scenes.Get(), FName(BossIntroCue::Reveal));
		}
	}

	void ShowCard(const FIntroState& State)
	{
		if (IsSkipping(State))
		{
			return;
		}
		if (UIntroWidget* Widget = State.Widget.Get())
		{
			Widget->ShowCard(BossIntro::CardTitle(State.Kind), BossIntro::CardSub(State.Kind), State.Script.CardSeconds, /*bAccentSub*/ true);
		}
		UE_LOG(LogLooter, Log, TEXT("Boss intro: [%s, %s]"), *BossIntro::CardTitle(State.Kind).ToString(), *BossIntro::CardSub(State.Kind).ToString());
	}

	void HandBack(FIntroState& State)
	{
		USceneSubsystem* Scenes = State.Scenes.Get();
		const bool bSkipping = IsSkipping(State);
		State.bSkipped |= bSkipping;
		if (UIntroWidget* Widget = State.Widget.Get())
		{
			Widget->SetLetterbox(false, bSkipping);
			if (bSkipping)
			{
				Widget->ClearWords();
			}
		}
		if (APlayerCameraManager* Camera = CameraManager(State))
		{
			Camera->StopCameraFade();
		}
		// They come back facing it: the fight is on.
		const AActor* Boss = State.Boss.Get();
		APlayerController* Controller = Scenes ? Scenes->GetHeldController() : nullptr;
		const APawn* Player = Scenes ? Scenes->GetHeldPawn() : nullptr;
		if (Controller && Player && Boss)
		{
			const FVector Aim = Boss->GetActorLocation() - Player->GetPawnViewLocation();
			FRotator Facing = Aim.Rotation();
			Facing.Pitch = FMath::Clamp(Facing.Pitch, -30.f, 30.f);
			Facing.Roll = 0.f;
			Controller->SetControlRotation(Facing);
		}
		if (Scenes)
		{
			Scenes->ViewFromPlayer(bSkipping ? 0.f : BossIntro::HandBackSeconds);
		}
		if (UBossComponent* Fight = State.Fight.Get(); Fight && Fight->IsFighting() && Fight->IsBarShown())
		{
			if (UHudBossBarWidget* Bar = Fight->GetBar())
			{
				Bar->SetShown(true);
				Bar->PlayIntro();
			}
		}
	}

	void AfterEnd(FIntroState& State)
	{
		// Everything round it wakes; the camera is put back for the next scene; the screen goes.
		State.Freeze.ReleaseAll();
		RestoreCamera(State);
		if (UIntroWidget* Widget = State.Widget.Get())
		{
			Widget->RemoveFromParent();
		}
		State.Widget.Reset();
		if (State.bSkipped)
		{
			LooterSound::Stop(State.Music.Get(), MusicSkipSeconds);
		}
		// Its own line is the fight's: said still, after a skip passed it.
		if (!State.bOwnLineSaid && !State.Acts.OwnLine.Text.IsEmpty() && State.Script.Captions.ContainsByPredicate(
			[](const FBossIntroCaption& Caption) { return Caption.Line == BossIntro::OwnLine; }))
		{
			if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(State.Scenes.Get()))
			{
				Captions->Play({ State.Acts.OwnLine }, ECaptionPlay::Queue);
			}
			State.bOwnLineSaid = true;
		}
		if (State.Acts.Ended)
		{
			State.Acts.Ended(State.bSkipped);
		}
		UE_LOG(LogLooter, Log, TEXT("Boss intro: %s %s; the fight is on."), BossIntro::KindName(State.Kind), State.bSkipped ? TEXT("skipped") : TEXT("played"));
	}
}

bool BossIntro::Play(EBossIntro Kind, AActor& Boss, UBossComponent* Fight, FBossIntroActs&& Acts)
{
	UWorld* World = Boss.GetWorld();
	USceneSubsystem* Scenes = USceneSubsystem::Get(&Boss);
	if (!World || !Scenes || Scenes->IsPlaying() || !USceneSubsystem::AreScenesOn(World))
	{
		return false;
	}
	const FStateRef State = MakeShared<FIntroState>();
	State->Kind = Kind;
	State->Script = Script(Kind);
	State->Acts = MoveTemp(Acts);
	State->Lines = Lines(Kind);
	State->Scenes = Scenes;
	State->Boss = &Boss;
	State->Fight = Fight;
	State->bUsesExposure = State->Script.Shots.ContainsByPredicate([](const FBossIntroShot& Shot)
		{ return !FMath::IsNearlyZero(Shot.FromExposure) || !FMath::IsNearlyZero(Shot.ToExposure); });
	const FBossIntroScript& Plan = State->Script;

	FScenePlay Scene;
	Scene.Name = SceneName(Kind);
	// The scene's camera has the view; only the skip keys work.
	Scene.bLookOnly = false;
	Scene.OnStart = [State]() { OnStart(State); };

	// As its first frame plays (after the fight's first phase has begun): whatever that phase raised is held too, and the
	// words queued at the fight's start make way for the scene's.
	Scene.Timeline.AddMoment(TEXT("Freeze"), 0.f, [State]()
	{
		AActor* Held = State->Boss.Get();
		if (UWorld* Here = Held ? Held->GetWorld() : nullptr)
		{
			State->Freeze.HoldCreaturesNear(*Here, Held->GetActorLocation(), FreezeRadius, FreezeDilation);
		}
		if (State->Script.bClearCaptions)
		{
			if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(State->Scenes.Get()))
			{
				Captions->Clear();
			}
		}
	});

	for (int32 Index = 0; Index < Plan.Shots.Num(); ++Index)
	{
		const FBossIntroShot& Shot = Plan.Shots[Index];
		Scene.Timeline.AddMove(Shot.Start, Shot.Duration, [State, Index](float Alpha) { ApplyShot(*State, Index, Alpha); });
		if (Index > 0)
		{
			Scene.Timeline.AddMoment(TEXT("CutOut"), Shot.Start - CutDipSeconds, [State]() { Dip(*State, true); });
			Scene.Timeline.AddMoment(TEXT("CutIn"), Shot.Start, [State]() { Dip(*State, false); });
		}
	}

	if (Plan.DiveAt >= 0.f)
	{
		Scene.Timeline.AddMoment(TEXT("Dive"), Plan.DiveAt, [State]()
		{
			if (State->Acts.Dive)
			{
				State->Acts.Dive(IsSkipping(*State));
			}
		});
	}
	for (int32 Puff = 0; Puff < Plan.TrailPuffs; ++Puff)
	{
		// Evenly from her feet toward the burst, the last a step short of it (the burst is its own).
		const float Share = static_cast<float>(Puff + 1) / static_cast<float>(Plan.TrailPuffs + 1);
		const float At = Plan.TrailPuffs > 1 ? FMath::Lerp(Plan.TrailFrom, Plan.TrailTo, static_cast<float>(Puff) / (Plan.TrailPuffs - 1)) : Plan.TrailFrom;
		Scene.Timeline.AddMoment(TEXT("Trail"), At, [State, Puff, Share]()
		{
			if (State->Acts.Trail)
			{
				State->Acts.Trail(Puff, FMath::Lerp(State->Frame.Boss, State->Frame.Burst, static_cast<double>(Share)), IsSkipping(*State));
			}
		});
	}
	Scene.Timeline.AddMoment(TEXT("Live"), Plan.LiveAt, [State]() { GoLive(*State); });
	Scene.Timeline.AddMoment(TEXT("Card"), Plan.CardAt, [State]() { ShowCard(*State); });
	for (const FBossIntroCaption& Caption : Plan.Captions)
	{
		const int32 Line = Caption.Line;
		Scene.Timeline.AddMoment(TEXT("Line"), Caption.At, [State, Line]() { Say(*State, Line); });
	}
	Scene.Timeline.AddMoment(TEXT("HandBack"), Plan.HandBackAt, [State]() { HandBack(*State); });
	Scene.Timeline.AddMoment(TEXT("End"), Plan.Seconds, [State]()
	{
		State->bSkipped |= IsSkipping(*State);
	});
	// Played or skipped (and if the level goes mid-scene, the freeze lets go as the state does).
	Scene.AfterEnd = [State]() { AfterEnd(*State); };
	return Scenes->Play(MoveTemp(Scene));
}
