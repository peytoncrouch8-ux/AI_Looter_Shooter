// UNewGameIntroSubsystem: the new game's intro, its planet part in the menu world, and what it leaves for the level's.

#include "Story/NewGameIntroSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "UI/Story/IntroWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** A frame longer than this is played as this long: a hitch mustn't throw the camera or skip a caption. */
	constexpr float LongestStep = 0.1f;

	/** The music's tail goes this fast when the part is skipped or a preview ends. */
	constexpr float MusicStopSeconds = 0.4f;
}

UNewGameIntroSubsystem* UNewGameIntroSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UNewGameIntroSubsystem>() : nullptr;
}

void UNewGameIntroSubsystem::Deinitialize()
{
	Stop();
	Super::Deinitialize();
}

bool UNewGameIntroSubsystem::PlayPlanet(UWorld& World, int32 Session, TFunction<void()> InThen)
{
	if (IsPlayingPlanet() || !NewGameIntro::IsOn() || !USceneSubsystem::AreScenesOn(&World) || !NewGameIntro::IsPlanetWorld(&World))
	{
		return false;
	}
	APlayerController* PC = World.GetFirstPlayerController();
	APlayerCameraManager* View = PC ? PC->PlayerCameraManager.Get() : nullptr;
	if (!View || !PC->IsLocalController())
	{
		return false;
	}

	PlayWorld = &World;
	Controller = PC;
	PlaySession = Session;
	Then = MoveTemp(InThen);
	SkipWhiteLeft = -1.f;
	Words = NewGameIntro::Lines();
	Planet = NewGameIntro::FindPlanet(&World);
	// The camera starts exactly where the menu's is, so the menu fading is the only change at first.
	Base.Location = View->GetCameraLocation();
	Base.Rotation = View->GetCameraRotation();
	Base.Fov = View->GetFOVAngle();

	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Camera = World.SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform(Base.Rotation, Base.Location), Params);
	if (Camera)
	{
		Camera->GetCameraComponent()->SetFieldOfView(Base.Fov);
		Camera->GetCameraComponent()->SetConstraintAspectRatio(false);
		PreviousViewTarget = PC->GetViewTarget();
		PC->SetViewTarget(Camera.Get());
	}

	// The menu's widgets step aside (faded, then collapsed); the intro's screen takes the keys over them.
	Menus.Reset();
	MenuOpacity.Reset();
	MenuVisibility.Reset();
	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(&World, Found, UUserWidget::StaticClass(), /*TopLevelOnly*/ true);
	for (UUserWidget* Menu : Found)
	{
		Menus.Add(Menu);
		MenuOpacity.Add(Menu->GetRenderOpacity());
		MenuVisibility.Add(Menu->GetVisibility());
	}
	Widget = CreateWidget<UIntroWidget>(PC);
	if (Widget)
	{
		Widget->AddToViewport(UIntroWidget::MenuZOrder);
		Widget->SetLetterbox(true);
		Widget->SetTakesKeys(true);
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(Widget->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		Widget->SetKeyboardFocus();
	}
	bShowedCursor = PC->bShowMouseCursor;
	PC->bShowMouseCursor = false;

	BuildTimeline();
	Music = LooterSound::Start(&World, LooterSoundCue::IntroPlanet);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UNewGameIntroSubsystem::HandleTick));
	UE_LOG(LogLooter, Log, TEXT("Intro: Teropa plays (%.0f s; %s)%s."), NewGameIntro::PlanetSeconds,
		Planet.bFound ? *FString::Printf(TEXT("the planet %.0f m across"), Planet.Radius * 0.02f) : TEXT("no planet found: the menu's view turns"),
		Then ? *FString::Printf(TEXT(", then session %d's level"), Session + 1) : TEXT(", a preview"));
	return true;
}

void UNewGameIntroSubsystem::BuildTimeline()
{
	Timeline = FSceneTimeline();
	const TWeakObjectPtr<UNewGameIntroSubsystem> WeakThis = this;
	for (const FIntroCaption& Caption : NewGameIntro::PlanetCaptions())
	{
		Timeline.AddMoment(TEXT("Caption"), Caption.At, [WeakThis, Caption]()
		{
			UNewGameIntroSubsystem* Intro = WeakThis.Get();
			if (!Intro || !Intro->Widget)
			{
				return;
			}
			if (Caption.Card != EIntroCard::None)
			{
				Intro->Widget->ShowCard(NewGameIntro::CardTitle(Caption.Card), NewGameIntro::CardSub(Caption.Card), Caption.Seconds);
				LooterSound::Play2D(Intro->PlayWorld.Get(), LooterSoundCue::IntroCard);
				UE_LOG(LogLooter, Log, TEXT("Intro: [%s]"), *NewGameIntro::CardTitle(Caption.Card).ToString());
			}
			else if (Intro->Words.IsValidIndex(Caption.Line))
			{
				Intro->Widget->ShowLine(Intro->Words[Caption.Line].Text, Caption.Seconds);
				UE_LOG(LogLooter, Log, TEXT("Intro: \"%s\""), *Intro->Words[Caption.Line].Text.ToString());
			}
		});
	}
	Timeline.AddMoment(TEXT("White"), NewGameIntro::PlanetWhiteAt, [WeakThis]()
	{
		UNewGameIntroSubsystem* Intro = WeakThis.Get();
		if (UTransitionScreenSubsystem* White = Intro ? UTransitionScreenSubsystem::Get(Intro->PlayWorld.Get()) : nullptr)
		{
			White->FadeToWhite(NewGameIntro::WhiteSeconds);
		}
		LooterSound::Play2D(Intro ? Intro->PlayWorld.Get() : nullptr, LooterSoundCue::IntroWhiteout);
	});
	Timeline.AddMoment(TEXT("End"), NewGameIntro::PlanetSeconds);
}

bool UNewGameIntroSubsystem::HandleTick(float DeltaSeconds)
{
	TGuardValue<bool> InTick(bInTick, true);
	UWorld* World = PlayWorld.Get();
	APlayerController* PC = Controller.Get();
	if (!World || !PC)
	{
		// The menu went away under it (a quit): nothing to finish in.
		UE_LOG(LogLooter, Warning, TEXT("Intro: the menu's world went away; Teropa stops."));
		Stop();
		return false;
	}
	const float Step = FMath::Min(DeltaSeconds, LongestStep);

	if (SkipWhiteLeft >= 0.f)
	{
		SkipWhiteLeft -= Step;
		if (SkipWhiteLeft <= 0.f)
		{
			Finish(true);
		}
		return TickHandle.IsValid();
	}
	if (Widget && Widget->ConsumeSkip())
	{
		SkipPlanet();
		return TickHandle.IsValid();
	}

	Timeline.Advance(Step);
	const float Time = Timeline.GetTime();
	if (Camera)
	{
		const FIntroView Shot = NewGameIntro::PlanetView(Base, Planet, Time);
		Camera->SetActorLocationAndRotation(Shot.Location, Shot.Rotation);
		Camera->GetCameraComponent()->SetFieldOfView(Shot.Fov);
		// The menu's controller may set its own view again: the intro keeps the view while it plays.
		if (PC->GetViewTarget() != Camera.Get())
		{
			PC->SetViewTarget(Camera.Get());
		}
	}
	FadeMenus(1.f - FMath::Clamp(Time / NewGameIntro::MenuFadeSeconds, 0.f, 1.f));
	if (Timeline.IsFinished())
	{
		Finish(false);
	}
	return TickHandle.IsValid();
}

void UNewGameIntroSubsystem::SkipPlanet()
{
	if (!IsPlayingPlanet() || SkipWhiteLeft >= 0.f)
	{
		return;
	}
	UE_LOG(LogLooter, Log, TEXT("Intro: skipped at %.1f s (the Rest's part goes with it)."), Timeline.GetTime());
	if (Widget)
	{
		Widget->ClearWords();
	}
	LooterSound::Stop(Music.Get(), MusicStopSeconds);
	FadeMenus(0.f);
	UTransitionScreenSubsystem* White = UTransitionScreenSubsystem::Get(PlayWorld.Get());
	if (White)
	{
		White->FadeToWhite(NewGameIntro::SkipWhiteSeconds);
	}
	// A frame past the white's own fade, so Then opens the level under a full white.
	SkipWhiteLeft = White ? NewGameIntro::SkipWhiteSeconds + LongestStep : 0.f;
}

FNewGameIntroHandoff UNewGameIntroSubsystem::TakeHandoff(int32 Session)
{
	FNewGameIntroHandoff Taken;
	if (Session != INDEX_NONE && Session == HandoffSession)
	{
		Taken = Handoff;
	}
	Handoff = FNewGameIntroHandoff();
	HandoffSession = INDEX_NONE;
	return Taken;
}

void UNewGameIntroSubsystem::FadeMenus(float Alpha)
{
	for (int32 Index = 0; Index < Menus.Num(); ++Index)
	{
		UUserWidget* Menu = Menus[Index].Get();
		if (!Menu || Menu == Widget.Get())
		{
			continue;
		}
		Menu->SetRenderOpacity(MenuOpacity[Index] * Alpha);
		if (Alpha <= 0.f && Menu->GetVisibility() != ESlateVisibility::Collapsed)
		{
			Menu->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UNewGameIntroSubsystem::RestoreMenus()
{
	for (int32 Index = 0; Index < Menus.Num(); ++Index)
	{
		if (UUserWidget* Menu = Menus[Index].Get())
		{
			Menu->SetRenderOpacity(MenuOpacity[Index]);
			Menu->SetVisibility(MenuVisibility[Index]);
		}
	}
	Menus.Reset();
}

void UNewGameIntroSubsystem::Finish(bool bSkipped)
{
	TFunction<void()> Next = MoveTemp(Then);
	Then = nullptr;
	const bool bPreview = !Next;
	if (PlaySession != INDEX_NONE)
	{
		Handoff.bPlanetShown = true;
		Handoff.bSkippedAll = bSkipped;
		HandoffSession = PlaySession;
	}
	UTransitionScreenSubsystem* White = UTransitionScreenSubsystem::Get(PlayWorld.Get());
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	if (bPreview)
	{
		// The menu back as it was, through the white thinning.
		RestoreMenus();
		if (APlayerController* PC = Controller.Get())
		{
			PC->SetViewTarget(PreviousViewTarget.Get() ? PreviousViewTarget.Get() : PC->GetPawnOrSpectator());
			PC->bShowMouseCursor = bShowedCursor;
			PC->SetInputMode(FInputModeUIOnly());
		}
		if (Camera)
		{
			Camera->Destroy();
			Camera = nullptr;
		}
		LooterSound::Stop(Music.Get(), MusicStopSeconds);
		if (White)
		{
			White->Reveal();
		}
	}
	else if (White)
	{
		// Held through the level's load; the Rest's part (or the cold open) takes it there.
		White->HoldWhite();
	}
	UE_LOG(LogLooter, Log, TEXT("Intro: Teropa %s."), bSkipped ? TEXT("skipped") : TEXT("played"));
	Stop();
	if (Next)
	{
		Next();
	}
}

void UNewGameIntroSubsystem::Stop()
{
	if (TickHandle.IsValid())
	{
		if (!bInTick)
		{
			FTSTicker::RemoveTicker(TickHandle);
		}
		TickHandle.Reset();
	}
	SkipWhiteLeft = -1.f;
	PlaySession = INDEX_NONE;
	Menus.Reset();
	Words.Reset();
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	// The camera stays until its world goes (the level's opening): the white covers it.
	Camera = nullptr;
	Controller.Reset();
	PlayWorld.Reset();
}
