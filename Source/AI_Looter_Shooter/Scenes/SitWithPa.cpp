// "Sit with Pa": the scene after Abel's fight (Main 6's last step).

#include "Scenes/SitWithPa.h"
#include "AI_Looter_Shooter.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelPoses.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "World/KeeperLanternPost.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "LooterAbel"

namespace SitWithPa
{
	namespace
	{
		/** The lines' order in Lines(). */
		enum ELine : int32
		{
			CameHome,
			Pa,
			Ned,
			Deacon,
			TallFella,
			BringHer,
			Leans,
			Reunions,
		};

		// When things happen (s).
		constexpr float LineAt[] = { 0.6f, 4.2f, 6.2f, 10.2f, 14.4f, 18.6f, 28.2f, 35.4f };
		constexpr float RiseAt = 22.6f;
		constexpr float ToPostAt = 23.f;
		constexpr float ToPostSeconds = 3.f;
		constexpr float RaiseAt = 26.2f;
		constexpr float CatchAt = 26.8f;
		constexpr float LitAt = 27.6f;
		constexpr float ToBoardAt = 30.4f;
		constexpr float ToBoardSeconds = 3.f;
		constexpr float SitAt = 33.4f;
		constexpr float SeatedAt = 35.f;
		constexpr float HandBackAt = 38.6f;
		constexpr float EndAt = 39.8f;

		/** He stands this far in front of the keeper's post to light its lantern (cm). */
		constexpr float PostStandOff = 150.f;
		/** The views: Ellis this far from Pa kneeling at most (cm), the lantern from this far, the board from behind his shoulder. */
		constexpr float NearestTalk = 600.f;
		constexpr float TalkDistance = 320.f;
		constexpr float EyeHeight = 160.f;
		constexpr float LanternView = 360.f;
		const FLinearColor CatchColor(1.f, 0.78f, 0.45f);

		struct FState
		{
			TWeakObjectPtr<USceneSubsystem> Scenes;
			TWeakObjectPtr<AAbelKeeper> Abel;
			TWeakObjectPtr<AKeeperLanternPost> Post;
			FVector Kneel = FVector::ZeroVector;
			float KneelYaw = 0.f;
			FVector AtPost = FVector::ZeroVector;
			float AtPostYaw = 0.f;
			FVector Board = FVector::ZeroVector;
			float BoardYaw = 0.f;
			FTransform TalkView;
			FTransform LanternView;
			FTransform BoardView;
			/** Where the player stood, and the way to face once they're given back (toward Pa's board). */
			FVector PlayerFeet = FVector::ZeroVector;
			bool bHasPlayer = false;
		};
		using FStateRef = TSharedRef<FState>;

		/** A yaw turned from From toward To the short way round, Alpha of the way. */
		float TurnedBy(float From, float To, float Alpha)
		{
			return From + FMath::FindDeltaAngleDegrees(From, To) * Alpha;
		}

		FTransform Looking(const FVector& From, const FVector& At)
		{
			return FTransform((At - From).Rotation(), From);
		}

		void SetView(const FState& State, const FTransform& View)
		{
			const USceneSubsystem* Scenes = State.Scenes.Get();
			if (ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr)
			{
				Camera->SetActorLocationAndRotation(View.GetLocation(), View.GetRotation());
			}
		}

		void Between(const FState& State, const FTransform& From, const FTransform& To, float Alpha)
		{
			const float Eased = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
			SetView(State, FTransform(FQuat::Slerp(From.GetRotation(), To.GetRotation(), Eased), FMath::Lerp(From.GetLocation(), To.GetLocation(), Eased)));
		}

		bool IsSkipping(const FState& State)
		{
			const USceneSubsystem* Scenes = State.Scenes.Get();
			return !Scenes || Scenes->IsSkipping();
		}

		void Say(const FState& State, int32 Line, bool bAbelSpeaks)
		{
			AAbelKeeper* Abel = State.Abel.Get();
			if (Abel)
			{
				Abel->SetSpeaking(bAbelSpeaks && !IsSkipping(State));
			}
			if (IsSkipping(State))
			{
				return;
			}
			const TArray<FStoryLine> Said = Lines();
			if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(State.Scenes.Get()); Captions && Said.IsValidIndex(Line))
			{
				Captions->Play({ Said[Line] }, ECaptionPlay::Interrupt);
			}
		}

		void Quiet(const FState& State)
		{
			if (AAbelKeeper* Abel = State.Abel.Get())
			{
				Abel->SetSpeaking(false);
			}
		}

		/** Works out the scene's spots from where Abel knelt, the keeper's post, his board and the player. */
		FStateRef MakeState(USceneSubsystem& Scenes, AAbelKeeper& Abel)
		{
			FStateRef State = MakeShared<FState>();
			State->Scenes = &Scenes;
			State->Abel = &Abel;
			State->Post = Abel.GetKeepersPost();
			State->Kneel = Abel.GetActorLocation();
			State->KneelYaw = static_cast<float>(Abel.GetActorRotation().Yaw);
			const float Scale = Abel.GetSizeScale();
			const float HalfHeight = Abel.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const FVector Feet = State->Kneel - FVector(0.0, 0.0, HalfHeight);
			const FVector Home = Abel.GetHome().GetLocation();
			const FVector Sunset = Abel.GetSunsetDirection();

			// Talking: Ellis's eyes where they stand (no farther than a few steps), on Pa's head as he kneels.
			const int32 HeadBone = AbelPoses::FindBone(TEXT("head"));
			const FAbelPoseHeads Knelt = AbelPoses::SolveHeads(EAbelPose::Kneel);
			const FVector Head = Feet + FVector(0.0, 0.0, (Knelt.Heads.IsValidIndex(HeadBone) ? Knelt.Heads[HeadBone].Z : 90.0) * Scale);
			const APawn* Player = Scenes.GetHeldPawn();
			FVector Eye = State->Kneel + FRotator(0.f, State->KneelYaw, 0.f).Vector() * TalkDistance;
			Eye.Z = Feet.Z + EyeHeight;
			if (Player)
			{
				State->bHasPlayer = true;
				const float PlayerHalf = Player->GetRootComponent() ? static_cast<float>(Player->GetRootComponent()->Bounds.BoxExtent.Z) : 90.f;
				State->PlayerFeet = Player->GetActorLocation() - FVector(0.0, 0.0, PlayerHalf);
				const FVector Theirs = Player->GetActorLocation() + FVector(0.0, 0.0, Player->BaseEyeHeight);
				if (FVector::Dist2D(Theirs, State->Kneel) <= NearestTalk)
				{
					Eye = Theirs;
				}
			}
			State->TalkView = Looking(Eye, Head);

			// The keeper's post: he stands before it, facing it; the view from the deck's middle side onto its lantern.
			if (const AKeeperLanternPost* Post = State->Post.Get())
			{
				const FVector PostAt = Post->GetActorLocation();
				const FVector ToDeck = (Home - PostAt).GetSafeNormal2D();
				State->AtPost = PostAt + ToDeck * PostStandOff;
				State->AtPost.Z = Home.Z;
				State->AtPostYaw = static_cast<float>((-ToDeck).Rotation().Yaw);
				const FVector Globe = Post->GetKeepersLanternGlobe();
				const FVector Side = FVector::CrossProduct(FVector::UpVector, ToDeck);
				State->LanternView = Looking(Globe + ToDeck * LanternView + Side * 120.f + FVector(0.0, 0.0, 20.0), Globe - FVector(0.0, 0.0, 30.0));
			}
			else
			{
				State->AtPost = State->Kneel;
				State->AtPostYaw = State->KneelYaw;
				State->LanternView = State->TalkView;
			}

			// His board: over his shoulder into the sunset.
			if (!Abel.GetBoardPlace(State->Board, State->BoardYaw))
			{
				State->Board = State->AtPost;
				State->BoardYaw = static_cast<float>(Sunset.Rotation().Yaw);
			}
			const FVector Facing = FRotator(0.f, State->BoardYaw, 0.f).Vector();
			const FVector Right = FVector::CrossProduct(FVector::UpVector, Facing);
			const FVector Seat = State->Board - FVector(0.0, 0.0, HalfHeight);
			State->BoardView = Looking(Seat - Facing * 420.f + Right * 210.f + FVector(0.0, 0.0, 190.0), Seat + Facing * 900.f + FVector(0.0, 0.0, 70.0));
			return State;
		}

		void Start(FState& State)
		{
			USceneSubsystem* Scenes = State.Scenes.Get();
			AAbelKeeper* Abel = State.Abel.Get();
			if (!Scenes || !Abel)
			{
				return;
			}
			Abel->BeginScene();
			Abel->SetPose(EAbelPose::Kneel, 0.4f);
			Scenes->SetHeldPlayerHidden(true);
			Scenes->ViewFromSceneCamera(0.8f);
			SetView(State, State.TalkView);
			UE_LOG(LogLooter, Log, TEXT("Sit with Pa: Abel kneels at %s."), *State.Kneel.ToCompactString());
		}
	}

	FName SceneName()
	{
		return FName(TEXT("SitWithPa"));
	}

	TArray<FStoryLine> Lines()
	{
		const FText Abel = LOCTEXT("Abel", "Abel");
		const FText Ellis = LOCTEXT("Ellis", "Ellis");
		const FText Hob = LOCTEXT("Hob", "Hob");
		return {
			// The doc's words.
			FStoryLine::Make(Abel, LOCTEXT("CameHome", "...El? You came home."), 3.4f),
			// Drafts: Ellis's answer, and "he tells it straight" in his words.
			FStoryLine::Make(Ellis, LOCTEXT("Pa", "Pa."), 1.8f),
			FStoryLine::Make(Abel, LOCTEXT("Ned", "Ned Purcell shot me. I never got the lantern up."), 3.8f),
			FStoryLine::Make(Abel, LOCTEXT("Deacon", "The Deacon wept over you, El. Wept, and kept her all the same."), 4.f),
			// Docs/Story.md's own phrase for it.
			FStoryLine::Make(Abel, LOCTEXT("TallFella", "And there was a tall fella on the rail who never lifted a finger."), 4.f),
			// The doc's words.
			FStoryLine::Make(Abel, LOCTEXT("BringHer", "Bring Saint Ada home, El. I'll wait."), 3.8f),
			// A draft, as the lantern leans north-east.
			FStoryLine::Make(Abel, LOCTEXT("Leans", "There. She leans for the nearest light still burning. Follow her."), 3.8f),
			// The doc's words.
			FStoryLine::Make(Hob, LOCTEXT("Reunions", "Well. I've seen worse reunions."), 3.f),
		};
	}

	float Duration()
	{
		return EndAt;
	}

	bool Play(AAbelKeeper& Abel)
	{
		USceneSubsystem* Scenes = USceneSubsystem::Get(&Abel);
		if (!Scenes)
		{
			return false;
		}
		const FStateRef State = MakeState(*Scenes, Abel);
		FScenePlay Scene;
		Scene.Name = SceneName();
		// Ellis's eyes are the scene's; the mouse doesn't move them.
		Scene.bLookOnly = false;
		Scene.OnStart = [State]()
		{
			// The held player is known only now: the talking view from their eyes.
			if (USceneSubsystem* Played = State->Scenes.Get(); Played && !State->bHasPlayer)
			{
				if (AAbelKeeper* Kneeling = State->Abel.Get())
				{
					const FStateRef Again = MakeState(*Played, *Kneeling);
					*State = *Again;
				}
			}
			Start(*State);
		};
		FSceneTimeline& Timeline = Scene.Timeline;

		// He talks, knelt; Ellis answers once.
		Timeline.AddMove(0.f, RiseAt, [State](float) { SetView(*State, State->TalkView); });
		Timeline.AddMoment(TEXT("CameHome"), LineAt[CameHome], [State]() { Say(*State, CameHome, true); });
		Timeline.AddMoment(TEXT("Pa"), LineAt[Pa], [State]() { Say(*State, Pa, false); });
		Timeline.AddMoment(TEXT("Ned"), LineAt[Ned], [State]() { Say(*State, Ned, true); });
		Timeline.AddMoment(TEXT("Deacon"), LineAt[Deacon], [State]() { Say(*State, Deacon, true); });
		Timeline.AddMoment(TEXT("TallFella"), LineAt[TallFella], [State]() { Say(*State, TallFella, true); });
		Timeline.AddMoment(TEXT("BringHer"), LineAt[BringHer], [State]() { Say(*State, BringHer, true); });

		// He rises and goes to the keeper's post; the view goes with him to its lantern.
		Timeline.AddMoment(TEXT("Rise"), RiseAt, [State]()
		{
			Quiet(*State);
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				Abel->SetPose(EAbelPose::Idle, 1.f);
			}
		});
		Timeline.AddMove(RiseAt, ToPostAt + ToPostSeconds - RiseAt, [State](float Alpha) { Between(*State, State->TalkView, State->LanternView, Alpha); });
		Timeline.AddMove(ToPostAt, ToPostSeconds, [State](float Alpha)
		{
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				const float Eased = FMath::SmoothStep(0.f, 1.f, Alpha);
				Abel->SetScenePlace(AbelRules::GlideAt(State->Kneel, State->AtPost, Alpha, 30.f),
					TurnedBy(State->KneelYaw, State->AtPostYaw, Eased));
			}
		});
		Timeline.AddMove(ToPostAt + ToPostSeconds, ToBoardAt - ToPostAt - ToPostSeconds, [State](float) { SetView(*State, State->LanternView); });

		// He raises his ghost light, and the Keeper's Lantern catches from it.
		Timeline.AddMoment(TEXT("Raise"), RaiseAt, [State]()
		{
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				Abel->SetPose(EAbelPose::Flare, 0.6f);
			}
		});
		Timeline.AddMove(RaiseAt, LitAt - RaiseAt, [State](float Alpha)
		{
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				Abel->SetLanternFlare(Alpha);
			}
		});
		Timeline.AddMoment(TEXT("Catch"), CatchAt, [State]()
		{
			AKeeperLanternPost* Post = State->Post.Get();
			UWorld* World = Post ? Post->GetWorld() : nullptr;
			UEnemyProjectileSubsystem* Shots = World ? World->GetSubsystem<UEnemyProjectileSubsystem>() : nullptr;
			if (Shots && !IsSkipping(*State))
			{
				Shots->ShowCharge(Post, Post->GetActorTransform().InverseTransformPosition(Post->GetKeepersLanternGlobe()), LitAt - CatchAt, 14.f, CatchColor);
			}
		});
		Timeline.AddMoment(TEXT("Lit"), LitAt, [State]()
		{
			if (AKeeperLanternPost* Post = State->Post.Get())
			{
				Post->LightKeepersLantern();
			}
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				Abel->SetLanternFlare(0.f);
			}
		});
		Timeline.AddMoment(TEXT("Leans"), LineAt[Leans], [State]() { Say(*State, Leans, true); });

		// He goes to his board, and sits on it facing the sunset; the view over his shoulder.
		Timeline.AddMoment(TEXT("ToBoard"), ToBoardAt, [State]()
		{
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				Abel->SetPose(EAbelPose::Idle, 0.6f);
			}
		});
		Timeline.AddMove(ToBoardAt, ToBoardSeconds, [State](float Alpha)
		{
			Between(*State, State->LanternView, State->BoardView, Alpha);
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				const float Eased = FMath::SmoothStep(0.f, 1.f, Alpha);
				Abel->SetScenePlace(AbelRules::GlideAt(State->AtPost, State->Board, Alpha, 30.f),
					TurnedBy(State->AtPostYaw, State->BoardYaw, Eased));
			}
		});
		Timeline.AddMoment(TEXT("Sit"), SitAt, [State]()
		{
			Quiet(*State);
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				Abel->SetPose(EAbelPose::Sit, 1.4f);
			}
		});
		Timeline.AddMove(ToBoardAt + ToBoardSeconds, HandBackAt - ToBoardAt - ToBoardSeconds, [State](float) { SetView(*State, State->BoardView); });
		Timeline.AddMoment(TEXT("Seated"), SeatedAt, [State]()
		{
			// Sat in the sit at his board: his friend's place, for good.
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				Abel->SitOnBoard();
			}
		});
		Timeline.AddMoment(TEXT("Reunions"), LineAt[Reunions], [State]() { Say(*State, Reunions, false); });

		// The view back in Ellis's own eyes, where they stood, facing Pa's board.
		Timeline.AddMoment(TEXT("HandBack"), HandBackAt, [State]()
		{
			USceneSubsystem* Played = State->Scenes.Get();
			if (!Played)
			{
				return;
			}
			if (State->bHasPlayer)
			{
				const FVector Toward = State->Board - State->PlayerFeet;
				Played->PlaceHeldPlayer(State->PlayerFeet, static_cast<float>(Toward.Rotation().Yaw));
			}
			Played->ViewFromPlayer(Played->IsSkipping() ? 0.f : 1.f);
		});
		Timeline.AddMoment(TEXT("End"), EndAt);
		// Played or skipped, he's on his board at the end.
		Scene.AfterEnd = [State]()
		{
			Quiet(*State);
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				Abel->SitOnBoard();
			}
		};
		const bool bPlays = Scenes->Play(MoveTemp(Scene));
		if (bPlays)
		{
			UE_LOG(LogLooter, Log, TEXT("Sit with Pa: the scene plays (%.0f s)."), EndAt);
		}
		return bPlays;
	}
}

#undef LOCTEXT_NAMESPACE
