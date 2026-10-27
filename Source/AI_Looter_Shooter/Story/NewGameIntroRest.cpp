// The new game's intro on Ransom's Rest: Westward when the planet didn't play, then over the Rim, the chapel, home and the
// grave, as a scene (USceneSubsystem) that ends on the white and leaves the player held for the cold open.

#include "Story/NewGameIntro.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Audio/LooterSound.h"
#include "Embers/HeroDefinition.h"
#include "Scenes/GraveWake.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "Story/NewGameIntroSubsystem.h"
#include "UI/Story/IntroWidget.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The music's tail goes this fast on a skip; Westward's music hands over to the Rest's this fast at the cut. */
	constexpr float MusicStopSeconds = 0.8f;
	constexpr float MusicHandOverSeconds = 1.2f;

	/** What the scene's moves and moments share; it outlives the call that made them. */
	struct FRestState
	{
		TWeakObjectPtr<USceneSubsystem> Scenes;
		TWeakObjectPtr<UIntroWidget> Widget;
		TWeakObjectPtr<UAudioComponent> Music;
		FIntroRestSetup Setup;
		TArray<FIntroMove> Moves;
		/** Each move's lift over the ground (ClearanceLift), measured as the scene starts. */
		TArray<float> Lifts;
	};
	using FStateRef = TSharedRef<FRestState>;

	bool IsSkipping(const FRestState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		return Scenes && Scenes->IsSkipping();
	}

	APlayerCameraManager* CameraManager(const FRestState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		const APlayerController* Controller = Scenes ? Scenes->GetHeldController() : nullptr;
		return Controller ? Controller->PlayerCameraManager.Get() : nullptr;
	}

	void ApplyMove(const FRestState& State, int32 Index, float Alpha)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr;
		if (!Camera || !State.Moves.IsValidIndex(Index))
		{
			return;
		}
		FIntroView View = NewGameIntro::ViewAlong(State.Moves[Index], Alpha);
		View.Location.Z += State.Lifts.IsValidIndex(Index) ? State.Lifts[Index] : 0.f;
		Camera->SetActorLocationAndRotation(View.Location, View.Rotation);
		if (UCameraComponent* Lens = Camera->GetCameraComponent())
		{
			Lens->SetFieldOfView(View.Fov);
		}
	}

	/** A cut's black dip: out before it, in after it. Nothing on a skip (the end clears it). */
	void Dip(const FRestState& State, bool bOut)
	{
		APlayerCameraManager* Camera = CameraManager(State);
		if (!Camera || IsSkipping(State))
		{
			return;
		}
		if (bOut)
		{
			Camera->StartCameraFade(0.f, 1.f, NewGameIntro::CutDipSeconds, FLinearColor::Black, /*bShouldFadeAudio*/ false, /*bHoldWhenFinished*/ true);
		}
		else
		{
			Camera->StartCameraFade(1.f, 0.f, NewGameIntro::CutDipSeconds, FLinearColor::Black, false, false);
		}
	}

	void Show(const FRestState& State, const FIntroCaption& Caption)
	{
		UIntroWidget* Widget = State.Widget.Get();
		const USceneSubsystem* Scenes = State.Scenes.Get();
		if (IsSkipping(State))
		{
			return;
		}
		if (Caption.Card == EIntroCard::Hero)
		{
			if (State.Setup.HeroTitle.IsEmpty())
			{
				return;
			}
			if (Widget)
			{
				Widget->ShowCard(State.Setup.HeroTitle, State.Setup.HeroRole, Caption.Seconds, /*bAccentSub*/ true);
			}
			LooterSound::Play2D(Scenes, LooterSoundCue::IntroCard);
			UE_LOG(LogLooter, Log, TEXT("Intro: [%s, %s]"), *State.Setup.HeroTitle.ToString(), *State.Setup.HeroRole.ToString());
		}
		else if (Caption.Card != EIntroCard::None)
		{
			if (Widget)
			{
				Widget->ShowCard(NewGameIntro::CardTitle(Caption.Card), NewGameIntro::CardSub(Caption.Card), Caption.Seconds);
			}
			LooterSound::Play2D(Scenes, LooterSoundCue::IntroCard);
			UE_LOG(LogLooter, Log, TEXT("Intro: [%s]"), *NewGameIntro::CardTitle(Caption.Card).ToString());
		}
		else if (State.Setup.Lines.IsValidIndex(Caption.Line))
		{
			const FText& Words = State.Setup.Lines[Caption.Line].Text;
			if (Widget)
			{
				Widget->ShowLine(Words, Caption.Seconds);
			}
			UE_LOG(LogLooter, Log, TEXT("Intro: \"%s\""), *Words.ToString());
		}
	}

	void StartMusic(FRestState& State, FName Cue)
	{
		LooterSound::Stop(State.Music.Get(), MusicHandOverSeconds);
		State.Music = LooterSound::Start(State.Scenes.Get(), Cue);
	}

	void OnStart(const FStateRef& State)
	{
		USceneSubsystem* Scenes = State->Scenes.Get();
		if (!Scenes)
		{
			return;
		}
		UWorld* World = Scenes->GetWorld();
		for (const FIntroMove& Move : State->Moves)
		{
			State->Lifts.Add(NewGameIntro::ClearanceLift(World, Move));
		}
		// The player's body stays where they arrived, unseen (the grave shot would find them standing at it).
		Scenes->SetHeldPlayerHidden(true);
		if (ACameraActor* Camera = Scenes->ViewFromSceneCamera(0.f))
		{
			if (UCameraComponent* Lens = Camera->GetCameraComponent())
			{
				Lens->SetConstraintAspectRatio(false);
			}
		}
		if (APlayerController* Controller = Scenes->GetHeldController(); Controller && Controller->IsLocalController())
		{
			if (UIntroWidget* Widget = CreateWidget<UIntroWidget>(Controller))
			{
				Widget->AddToViewport(UIntroWidget::ViewportZOrder);
				// Under the white: the bars are already in when it thins.
				Widget->SetLetterbox(true, /*bInstant*/ true);
				// The scene skips on Interact and Escape; the widget adds the menu part's other hold keys (Space, Enter,
				// the pad's A), so the same keys skip in both parts.
				Widget->SetPollsKeys(Controller);
				State->Widget = Widget;
			}
		}
		StartMusic(*State, State->Setup.bWestward ? LooterSoundCue::IntroPlanet : LooterSoundCue::IntroRest);
		// The white the trip held thins on the first shot (plain: the title is the cold open's, at the end).
		if (UTransitionScreenSubsystem* White = UTransitionScreenSubsystem::Get(Scenes); White && White->IsHeld())
		{
			White->Reveal();
		}
		UE_LOG(LogLooter, Log, TEXT("Intro: Ransom's Rest plays (%.1f s%s)."), NewGameIntro::RestSceneSeconds(State->Setup.bWestward),
			State->Setup.bWestward ? TEXT(", Westward first: the planet didn't play") : TEXT(""));
	}
}

FScenePlay NewGameIntro::MakeRest(USceneSubsystem& Scenes, const FIntroRestSetup& Setup)
{
	const FStateRef State = MakeShared<FRestState>();
	State->Scenes = &Scenes;
	State->Setup = Setup;
	State->Moves = RestMoves(Setup.bWestward);
	if (Setup.bHasGrave && State->Moves.Num() > 0)
	{
		FIntroMove& Last = State->Moves.Last();
		Last = GraveMove(Setup.GraveLook, Setup.GraveFoot, Last.Start, Last.Duration);
	}

	FScenePlay Scene;
	Scene.Name = SceneName();
	Scene.bLookOnly = false;
	// The cold open takes the held player over under the white.
	Scene.bHoldAfterEnd = true;
	Scene.OnStart = [State]() { OnStart(State); };
	Scene.OnTick = [State](float)
	{
		UIntroWidget* Widget = State->Widget.Get();
		USceneSubsystem* Scenes = State->Scenes.Get();
		if (Widget && Scenes && Widget->ConsumeSkip())
		{
			Scenes->SkipScene();
		}
	};

	for (int32 Index = 0; Index < State->Moves.Num(); ++Index)
	{
		const FIntroMove& Move = State->Moves[Index];
		Scene.Timeline.AddMove(Move.Start, Move.Duration, [State, Index](float Alpha) { ApplyMove(*State, Index, Alpha); });
		if (Index > 0)
		{
			Scene.Timeline.AddMoment(TEXT("CutOut"), Move.Start - CutDipSeconds, [State]() { Dip(*State, true); });
			Scene.Timeline.AddMoment(TEXT("CutIn"), Move.Start, [State]() { Dip(*State, false); });
		}
	}

	const float RestAt = Setup.bWestward ? PlanetSeconds : 0.f;
	if (Setup.bWestward)
	{
		for (const FIntroCaption& Caption : PlanetCaptions())
		{
			Scene.Timeline.AddMoment(TEXT("Caption"), Caption.At, [State, Caption]() { Show(*State, Caption); });
		}
		Scene.Timeline.AddMoment(TEXT("Rest"), RestAt, [State]()
		{
			if (!IsSkipping(*State))
			{
				StartMusic(*State, LooterSoundCue::IntroRest);
			}
		});
	}
	for (const FIntroCaption& Caption : RestCaptions())
	{
		FIntroCaption At = Caption;
		At.At += RestAt;
		Scene.Timeline.AddMoment(TEXT("Caption"), At.At, [State, At]() { Show(*State, At); });
	}

	// The white rises at the end; the cold open's title rises through it.
	Scene.Timeline.AddMoment(TEXT("White"), RestAt + RestWhiteAt, [State]()
	{
		const USceneSubsystem* Scenes = State->Scenes.Get();
		UTransitionScreenSubsystem* White = UTransitionScreenSubsystem::Get(Scenes);
		if (IsSkipping(*State))
		{
			if (White)
			{
				White->HoldWhite();
			}
			LooterSound::Stop(State->Music.Get(), MusicStopSeconds);
			return;
		}
		if (White)
		{
			White->FadeToWhite(WhiteSeconds);
		}
		LooterSound::Play2D(Scenes, LooterSoundCue::IntroWhiteout);
	});
	Scene.Timeline.AddMoment(TEXT("End"), RestAt + RestSeconds, [State]()
	{
		if (UIntroWidget* Widget = State->Widget.Get())
		{
			Widget->ClearWords();
		}
		if (APlayerCameraManager* Camera = CameraManager(*State))
		{
			Camera->StopCameraFade();
		}
	});

	// Played or skipped: its screen goes (under the white). The music's last chord rings on under the title.
	Scene.AfterEnd = [State]()
	{
		if (UIntroWidget* Widget = State->Widget.Get())
		{
			Widget->RemoveFromParent();
		}
		State->Widget.Reset();
	};
	return Scene;
}

bool NewGameIntro::PlayRest(UWorld& World, bool bWestward, TFunction<void()> AfterEnd)
{
	USceneSubsystem* Scenes = USceneSubsystem::Get(&World);
	if (!Scenes || Scenes->IsPlaying() || !USceneSubsystem::AreScenesOn(&World))
	{
		return false;
	}
	FIntroRestSetup Setup;
	Setup.bWestward = bWestward;
	Setup.Lines = Lines();
	const UHeroDefinition* Hero = USessionSubsystem::FindPlayedHero(&World);
	if (!Hero)
	{
		Hero = UHeroDefinition::Load(UHeroDefinition::DefaultId());
	}
	if (Hero)
	{
		Setup.HeroTitle = FText::FromString(Hero->DisplayName.ToString().ToUpper());
		Setup.HeroRole = FText::FromString(Hero->Role.ToString().ToUpper());
	}
	if (AActor* Grave = GraveWake::FindGrave(&World))
	{
		const FGraveWakeSpots Spots = GraveWake::SpotsFor(*Grave);
		Setup.bHasGrave = true;
		Setup.GraveLook = Spots.LookAt;
		Setup.GraveFoot = Spots.Stand;
	}
	FScenePlay Scene = MakeRest(*Scenes, Setup);
	Scene.AfterEnd = [Inner = MoveTemp(Scene.AfterEnd), AfterEnd = MoveTemp(AfterEnd)]()
	{
		if (Inner)
		{
			Inner();
		}
		if (AfterEnd)
		{
			AfterEnd();
		}
	};
	return Scenes->Play(MoveTemp(Scene));
}

bool NewGameIntro::PlayRestIfDue(UWorld& World, TFunction<void()> Then)
{
	FCampaignRecord* Campaign = UAreaTravelSubsystem::FindCampaign(&World);
	if (!Campaign || !IsDue(*Campaign))
	{
		return false;
	}
	const USessionSubsystem* Sessions = USessionSubsystem::Get(&World);
	UNewGameIntroSubsystem* Intro = UNewGameIntroSubsystem::Get(&World);
	const FNewGameIntroHandoff Handoff = Intro ? Intro->TakeHandoff(Sessions ? Sessions->GetActiveSession() : INDEX_NONE) : FNewGameIntroHandoff();
	if (!IsOn() || Handoff.bSkippedAll || !USceneSubsystem::AreScenesOn(&World))
	{
		// Passed: off, skipped whole in the menu, or no scenes here (a tour): seen, so it never waits for a later arrival.
		Campaign->SetFlag(SeenFlag());
		UE_LOG(LogLooter, Log, TEXT("Intro: passed (%s)."), !IsOn() ? TEXT("Looter.Intro 0") : Handoff.bSkippedAll ? TEXT("skipped in the menu")
			: TEXT("scenes are off"));
		return false;
	}
	const TWeakObjectPtr<UWorld> WeakWorld = &World;
	return PlayRest(World, !Handoff.bPlanetShown, [WeakWorld, Then = MoveTemp(Then)]()
	{
		// Kept with the session's record: it goes into the save the cold open's end makes.
		if (FCampaignRecord* Record = UAreaTravelSubsystem::FindCampaign(WeakWorld.Get()))
		{
			Record->SetFlag(SeenFlag());
		}
		if (Then)
		{
			Then();
		}
		// The intro ends on the game's title card: the cold open reveals the white with REVENANT as it starts. Should
		// nothing have taken the white (no cold open here), the title rises through it now all the same.
		if (UTransitionScreenSubsystem* White = UTransitionScreenSubsystem::Get(WeakWorld.Get()); White && White->IsHeld())
		{
			UE_LOG(LogLooter, Log, TEXT("Intro: nothing took the white; the title rises through it."));
			White->Reveal(UTransitionScreenSubsystem::GameTitle());
		}
	});
}
