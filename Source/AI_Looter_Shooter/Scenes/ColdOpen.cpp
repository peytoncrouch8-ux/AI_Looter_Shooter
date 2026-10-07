// The cold open: its lines, its start and the gang's skiff seven days ago (dusk on Ransom's Point is ColdOpenPoint.cpp's).

#include "Scenes/ColdOpen.h"
#include "AI_Looter_Shooter.h"
#include "Player/PlayerViewComponent.h"
#include "Scenes/ColdOpenBeats.h"
#include "Scenes/ColdOpenCast.h"
#include "Scenes/ColdOpenCourse.h"
#include "Scenes/ColdOpenSet.h"
#include "Scenes/SceneCloudBank.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "World/LightingStates.h"
#include "World/LightingStateSubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#define LOCTEXT_NAMESPACE "LooterColdOpen"

namespace ColdOpenBeats
{
	FStoryLine LineFor(ELine Which)
	{
		const FText Deacon = LOCTEXT("Deacon", "The Deacon");
		switch (Which)
		{
		case ELine::Home:
			return FStoryLine::Make(Deacon, LOCTEXT("Home", "Home, kid."), 2.6f);
		case ELine::SevenDays:
			return FStoryLine::Make(FText::GetEmpty(), LOCTEXT("SevenDays", "Ransom's Rest. Seven days ago."), 4.f);
		case ELine::Bell:
			// No sound is made yet, so the bell is read: a line with no speaker, bracketed as captions mark a sound.
			return FStoryLine::Make(FText::GetEmpty(), LOCTEXT("Bell", "[The chapel bell clangs across the Rest.]"), 2.8f);
		case ELine::PutHerBack:
			return FStoryLine::Make(LOCTEXT("Ellis", "Ellis"), LOCTEXT("PutHerBack", "Put her back, Deacon. Not here. Not my home."), 4.2f);
		case ELine::El:
			return FStoryLine::Make(LOCTEXT("Abel", "Abel"), LOCTEXT("El", "El!"), 1.6f);
		case ELine::Sorry:
			return FStoryLine::Make(Deacon, LOCTEXT("Sorry", "I'm sorry, kid."), 2.8f);
		default:
			return FStoryLine();
		}
	}

	bool IsSkipping(const FColdOpenState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		return Scenes && Scenes->IsSkipping();
	}

	float SceneTime(const FColdOpenState& State)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		return Scenes ? Scenes->GetSceneTime() : 0.f;
	}

	void SetView(const FColdOpenState& State, const FVector& Location, const FRotator& Rotation)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		if (ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr)
		{
			Camera->SetActorLocationAndRotation(Location, Rotation);
		}
	}

	FRotator LookAt(const FVector& From, const FVector& To)
	{
		return (To - From).Rotation();
	}

	void Say(const FColdOpenState& State, ELine Which)
	{
		if (IsSkipping(State))
		{
			return;
		}
		if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(State.Scenes.Get()))
		{
			Captions->Play({ LineFor(Which) }, ECaptionPlay::Interrupt);
		}
	}

	void Sound(const FColdOpenState& State, USoundBase* Played, const FVector* At)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		if (!Played || !Scenes || IsSkipping(State))
		{
			return;
		}
		if (At)
		{
			UGameplayStatics::PlaySoundAtLocation(Scenes, Played, *At);
		}
		else
		{
			UGameplayStatics::PlaySound2D(Scenes, Played);
		}
	}

	void Fade(const FColdOpenState& State, float From, float To, float Seconds, bool bHold)
	{
		const USceneSubsystem* Scenes = State.Scenes.Get();
		const APlayerController* Controller = Scenes ? Scenes->GetHeldController() : nullptr;
		APlayerCameraManager* Camera = Controller ? Controller->PlayerCameraManager.Get() : nullptr;
		if (!Camera)
		{
			return;
		}
		if (IsSkipping(State))
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

	namespace
	{
		/** Looking a little down along the bow, as the packet skiff's ride began (degrees). */
		constexpr float DeckPitch = -3.f;

		/** A lighting state at once, behind whatever covers the screen (the white, the black); nothing when the level lacks it. */
		void SetLight(const USceneSubsystem* Scenes, FName Wanted)
		{
			ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(Scenes);
			if (Lighting && Lighting->GetStateNames().Contains(Wanted))
			{
				Lighting->SetState(Wanted, ELightingSwitch::Instant);
			}
		}

		/** The skiff where its course has it Seconds in, the view on its deck turned as the skiff turns. */
		void RideSkiff(FColdOpenState& State, float Seconds)
		{
			AColdOpenCast* Props = State.Props.Get();
			USceneSubsystem* Scenes = State.Scenes.Get();
			if (!Props || !Scenes || !State.Course.IsBuilt())
			{
				return;
			}
			const FTransform Pose = State.Course.PoseAt(Seconds);
			Props->PlaceSkiff(Pose);
			// The player keeps looking the way they were, measured from the bow.
			const double Yaw = Pose.Rotator().Yaw;
			if (State.bSkiffYawKnown)
			{
				Scenes->TurnHeldView(FRotator::NormalizeAxis(Yaw - State.SkiffYaw));
			}
			State.SkiffYaw = Yaw;
			State.bSkiffYawKnown = true;
			const APlayerController* Controller = Scenes->GetHeldController();
			const FRotator Look = Controller ? Controller->GetControlRotation() : FRotator(DeckPitch, Yaw, 0.0);
			SetView(State, Props->GetDeckEye(), Look);
		}

		void Start(FColdOpenState& State)
		{
			USceneSubsystem* Scenes = State.Scenes.Get();
			AColdOpenSet* Set = State.Set.Get();
			if (!Scenes || !Set)
			{
				return;
			}
			// Dusk, at once: the white the arrival held still covers the screen.
			SetLight(Scenes, ColdOpen::DuskState());

			if (AColdOpenCast* Props = AColdOpenCast::Spawn(*Set))
			{
				Scenes->AddSceneActor(Props);
				State.Props = Props;
				Props->ShowSkiff(true);
			}
			// The evening cloud the skiff glides out of: its front a little way along the course, reaching back over the start.
			UWorld* World = Scenes->GetWorld();
			if (World && State.Course.IsBuilt())
			{
				const float FrontAt = FMath::Min(Set->CloudFrontAhead, State.Course.GetLength());
				const FRotator Back(0.0, State.Course.HeadingAt(FrontAt).Rotation().Yaw + 180.0, 0.0);
				if (ASceneCloudBank* Bank = ASceneCloudBank::Spawn(*World, FTransform(Back, State.Course.PointAt(FrontAt)), Set->EveningCloudTint))
				{
					Scenes->AddSceneActor(Bank);
				}
			}

			// The player's own body stays where they arrived, unseen; the view is the scene's, on the skiff's deck, looking
			// along the bow as the packet skiff's last view did.
			Scenes->SetHeldPlayerHidden(true);
			if (APlayerController* Controller = Scenes->GetHeldController())
			{
				Controller->SetControlRotation(FRotator(DeckPitch, State.Course.PoseAt(0.f).Rotator().Yaw, 0.0));
			}
			if (ACameraActor* Camera = Scenes->ViewFromSceneCamera(0.f))
			{
				// Seen at the player's own field of view, so the match cut through the white keeps the frame.
				const UCameraComponent* Eyes = UPlayerViewComponent::FindFirstPersonCamera(Scenes->GetHeldPawn());
				if (Eyes && Camera->GetCameraComponent())
				{
					Camera->GetCameraComponent()->SetFieldOfView(Eyes->FieldOfView);
				}
			}
			RideSkiff(State, 0.f);

			// The white thins on the skiff, the title rising through it first.
			if (UTransitionScreenSubsystem* White = UTransitionScreenSubsystem::Get(Scenes))
			{
				White->Reveal(State.Title);
			}
			UE_LOG(LogLooter, Log, TEXT("Cold open: on the gang's skiff, %.0f m of course in %.0f s (cruising at %.1f m/s)."),
				State.Course.GetLength() / 100.f, State.Course.GetDuration(), State.Course.GetCruiseSpeed() / 100.f);
		}
	}
}

namespace ColdOpen
{
	FName SceneName()
	{
		return FName(TEXT("ColdOpen"));
	}

	FName DuskState()
	{
		return FName(TEXT("Dusk"));
	}

	float DurationFor(float SkiffSeconds)
	{
		return SkiffSeconds + PointSeconds;
	}

	TArray<FStoryLine> Lines()
	{
		using ColdOpenBeats::ELine;
		TArray<FStoryLine> Said;
		for (uint8 Index = 0; Index < static_cast<uint8>(ELine::Count); ++Index)
		{
			Said.Add(ColdOpenBeats::LineFor(static_cast<ELine>(Index)));
		}
		return Said;
	}

	FScenePlay Make(USceneSubsystem& Scenes, AColdOpenSet& Set, const FText& Title, TFunction<void()> Next)
	{
		using namespace ColdOpenBeats;
		const FStateRef State = MakeShared<FColdOpenState>();
		State->Scenes = &Scenes;
		State->Set = &Set;
		State->Title = Title;
		State->SkiffSeconds = Set.SkiffSeconds;
		State->Course.Build(Set.SkiffCourse, Set.SkiffSeconds);

		FScenePlay Scene;
		Scene.Name = SceneName();
		// On the skiff the player looks around; on the Point the view is Ellis's, moved by the scene.
		Scene.bLookOnly = true;
		// The grave wake-up follows with the player still held: it takes them over.
		Scene.bHoldAfterEnd = true;
		Scene.OnStart = [State]() { Start(*State); };

		const float Cut = State->SkiffSeconds;
		Scene.Timeline.AddMove(0.f, Cut, [State](float Alpha) { RideSkiff(*State, Alpha * State->SkiffSeconds); });
		Scene.Timeline.AddMoment(TEXT("Home"), Cut * HomeShare, [State]() { Say(*State, ELine::Home); });
		Scene.Timeline.AddMoment(TEXT("SevenDays"), Cut * SevenDaysShare, [State]() { Say(*State, ELine::SevenDays); });
		Scene.Timeline.AddMoment(TEXT("SkiffFade"), Cut - SkiffFadeBeforeCut, [State]() { Fade(*State, 0.f, 1.f, FadeSeconds, true); });
		AddPointBeats(Scene, State, Cut);

		Scene.AfterEnd = [State, Next = MoveTemp(Next)]()
		{
			// Back to the afternoon behind the black: the wake-up is seven days later.
			SetLight(State->Scenes.Get(), ALightingStates::DayState);
			if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(State->Scenes.Get()))
			{
				Captions->Clear();
			}
			if (Next)
			{
				Next();
			}
		};
		return Scene;
	}
}

#undef LOCTEXT_NAMESPACE
