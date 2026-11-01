// "Free Abel": the scene after the last call at the keeper's post (Main 7's step 3; was "Sit with Pa").

#include "Scenes/FreeAbel.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/AbelCorruption.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/AbelPoses.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/KeeperLanternPost.h"
#include "World/LamplighterRules.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "LooterAbel"

namespace FreeAbel
{
	namespace
	{
		/** The lines' order in Lines(). */
		enum ELine : int32
		{
			MyLantern,
			CarryMe,
		};

		// When things happen (s).
		constexpr float BoilFrom = 0.4f;
		constexpr float BoilSeconds = 3.2f;
		constexpr float BoilBursts[] = { 0.5f, 1.2f, 2.0f, 2.8f };
		constexpr float WholeAt = 3.5f;
		constexpr float RiseAt = 3.9f;
		constexpr float LineAt[] = { 4.6f, 8.4f };
		constexpr float ToPostAt = 11.8f;
		constexpr float ToPostSeconds = 2.2f;
		constexpr float IntoAt = 14.f;
		constexpr float IntoSeconds = 1.3f;
		constexpr float SunriseAt = 14.6f;
		constexpr float TakeDownAt = 16.4f;
		constexpr float HandBackAt = 16.6f;
		constexpr float EndAt = 17.6f;

		/** He stands this far in front of the keeper's post to step into its lantern (cm). */
		constexpr float PostStandOff = 110.f;
		/** The views: Sully this near him at most (cm), his head at the kneel, the lantern from this far. */
		constexpr float NearestTalk = 600.f;
		constexpr float TalkDistance = 320.f;
		constexpr float EyeHeight = 160.f;
		constexpr float LanternView = 340.f;
		/** The sunrise: the lantern's glow as he steps in (times its lit light), and the gold it flashes. */
		constexpr float SunriseGlow = 4.f;
		const FLinearColor SunriseGold(1.f, 0.82f, 0.48f);
		const FLinearColor TornSmog(0.05f, 0.035f, 0.07f);

		struct FState
		{
			TWeakObjectPtr<USceneSubsystem> Scenes;
			TWeakObjectPtr<AAbelKeeper> Abel;
			TWeakObjectPtr<AKeeperLanternPost> Post;
			FVector Kneel = FVector::ZeroVector;
			float KneelYaw = 0.f;
			FVector AtPost = FVector::ZeroVector;
			float AtPostYaw = 0.f;
			FTransform TalkView;
			FTransform LanternView;
			FVector PlayerFeet = FVector::ZeroVector;
			bool bHasPlayer = false;
			TArray<FStoryLine> Said;
		};
		using FStateRef = TSharedRef<FState>;

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

		FWeaponFX* EffectsFor(const FState& State)
		{
			const AAbelKeeper* Abel = State.Abel.Get();
			UWorld* World = Abel ? Abel->GetWorld() : nullptr;
			UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
			if (!Bullets || IsSkipping(State))
			{
				return nullptr;
			}
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			return &Effects;
		}

		void Say(const FState& State, int32 Line)
		{
			AAbelKeeper* Abel = State.Abel.Get();
			if (Abel)
			{
				Abel->SetSpeaking(!IsSkipping(State));
			}
			if (IsSkipping(State))
			{
				return;
			}
			if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(State.Scenes.Get()); Captions && State.Said.IsValidIndex(Line))
			{
				Captions->Play({ State.Said[Line] }, ECaptionPlay::Interrupt);
			}
		}

		void Quiet(const FState& State)
		{
			if (AAbelKeeper* Abel = State.Abel.Get())
			{
				Abel->SetSpeaking(false);
			}
		}

		/** Works out the scene's spots from where Abel knelt, the keeper's post and the player. */
		FStateRef MakeState(USceneSubsystem& Scenes, AAbelKeeper& Abel)
		{
			FStateRef State = MakeShared<FState>();
			State->Scenes = &Scenes;
			State->Abel = &Abel;
			State->Post = Abel.GetKeepersPost();
			State->Kneel = Abel.GetActorLocation();
			State->KneelYaw = static_cast<float>(Abel.GetActorRotation().Yaw);
			// The writer's words when made, the code's otherwise (the same count, or the code's).
			const TArray<FStoryLine> Written = LandfallRules::LoadLines(TEXT("DA_Lines_FreeAbel"));
			State->Said = Written.Num() == Lines().Num() ? Written : Lines();
			const float Scale = Abel.GetSizeScale();
			const float HalfHeight = Abel.GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			const FVector Feet = State->Kneel - FVector(0.0, 0.0, HalfHeight);
			const int32 HeadBone = AbelPoses::FindBone(TEXT("head"));
			const FAbelPoseHeads Knelt = AbelPoses::SolveHeads(EAbelPose::Kneel);
			const FVector Head = Feet + FVector(0.0, 0.0, (Knelt.Heads.IsValidIndex(HeadBone) ? Knelt.Heads[HeadBone].Z : 90.0) * Scale);
			FVector Eye = State->Kneel + FRotator(0.f, State->KneelYaw, 0.f).Vector() * TalkDistance;
			Eye.Z = Feet.Z + EyeHeight;
			if (const APawn* Player = Scenes.GetHeldPawn())
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
			if (const AKeeperLanternPost* Post = State->Post.Get())
			{
				const FVector PostAt = Post->GetActorLocation();
				const FVector ToHim = (State->Kneel - PostAt).GetSafeNormal2D();
				State->AtPost = PostAt + ToHim * PostStandOff;
				State->AtPost.Z = State->Kneel.Z;
				State->AtPostYaw = static_cast<float>((-ToHim).Rotation().Yaw);
				const FVector Globe = Post->GetKeepersLanternGlobe();
				const FVector Side = FVector::CrossProduct(FVector::UpVector, ToHim);
				State->LanternView = Looking(Globe + ToHim * LanternView + Side * 110.f + FVector(0.0, 0.0, 10.0), Globe - FVector(0.0, 0.0, 20.0));
			}
			else
			{
				State->AtPost = State->Kneel;
				State->AtPostYaw = State->KneelYaw;
				State->LanternView = State->TalkView;
			}
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
			Abel->SetSceneFade(0.f);
			Scenes->SetHeldPlayerHidden(true);
			Scenes->ViewFromSceneCamera(0.8f);
			SetView(State, State.TalkView);
			UE_LOG(LogLooter, Log, TEXT("Free Abel: the smog boils off him at %s."), *State.Kneel.ToCompactString());
		}
	}

	FName SceneName()
	{
		return FName(TEXT("FreeAbel"));
	}

	TArray<FStoryLine> Lines()
	{
		// The design's words (MAINS_2_7.md section 11); DA_Lines_FreeAbel holds the same.
		const FText Abel = LOCTEXT("Abel", "Abel");
		return {
			FStoryLine::Make(Abel, LOCTEXT("MyLantern", "That's my lantern. You've been looking after it."), 3.4f),
			FStoryLine::Make(Abel, LOCTEXT("CarryMe", "I can't get home on my own. Will you carry me?"), 3.4f),
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
		// Sully's eyes are the scene's; the mouse doesn't move them.
		Scene.bLookOnly = false;
		Scene.OnStart = [State]()
		{
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

		// The last of the smog boils off him into the fog: his black coat, a third of it still on him, burns away to gold-white.
		Timeline.AddMove(0.f, ToPostAt, [State](float) { SetView(*State, State->TalkView); });
		Timeline.AddMove(BoilFrom, BoilSeconds, [State](float Alpha)
		{
			AAbelKeeper* Abel = State->Abel.Get();
			UAbelCorruptionComponent* Corruption = Abel ? Abel->GetCorruption() : nullptr;
			if (Corruption)
			{
				Corruption->SetCorruptionShown((1.f / 3.f) * (1.f - FMath::SmoothStep(0.f, 1.f, Alpha)));
			}
		});
		for (int32 Burst = 0; Burst < UE_ARRAY_COUNT(BoilBursts); ++Burst)
		{
			Timeline.AddMoment(*FString::Printf(TEXT("Boil%d"), Burst), BoilBursts[Burst], [State, Burst]()
			{
				AAbelKeeper* Abel = State->Abel.Get();
				if (!Abel)
				{
					return;
				}
				if (FWeaponFX* Effects = EffectsFor(*State))
				{
					Effects->SpawnSoulTatters(Abel->GetCoalLocation(), FVector::UpVector, TornSmog, 80.f, 12);
				}
				if (Burst == 0 && !IsSkipping(*State))
				{
					// The furnace's last breath leaving him.
					LooterSound::PlayAt(Abel, LooterSoundCue::AbelDeath, Abel->GetActorLocation(), 0.8f);
				}
			});
		}
		Timeline.AddMoment(TEXT("Whole"), WholeAt, [State]()
		{
			AAbelKeeper* Abel = State->Abel.Get();
			if (!Abel)
			{
				return;
			}
			// What's left of him: gold-white, scorched but whole.
			if (FWeaponFX* Effects = EffectsFor(*State))
			{
				Effects->SpawnStarburst(Abel->GetCoalLocation(), 150.f, SunriseGold, 5.f, 0.7f, 10);
			}
			if (!IsSkipping(*State))
			{
				LooterSound::PlayAt(Abel, LooterSoundCue::Element::SoulLightApply, Abel->GetCoalLocation());
			}
		});
		Timeline.AddMoment(TEXT("Rise"), RiseAt, [State]()
		{
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				Abel->SetPose(EAbelPose::Idle, 1.f);
			}
		});
		Timeline.AddMoment(TEXT("MyLantern"), LineAt[MyLantern], [State]() { Say(*State, MyLantern); });
		Timeline.AddMoment(TEXT("CarryMe"), LineAt[CarryMe], [State]() { Say(*State, CarryMe); });

		// To the post, the view going with him to its lantern; he steps into it and it burns like a sunrise.
		Timeline.AddMoment(TEXT("ToPost"), ToPostAt, [State]() { Quiet(*State); });
		Timeline.AddMove(ToPostAt, ToPostSeconds, [State](float Alpha)
		{
			Between(*State, State->TalkView, State->LanternView, Alpha);
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				const float Eased = FMath::SmoothStep(0.f, 1.f, Alpha);
				Abel->SetScenePlace(AbelRules::GlideAt(State->Kneel, State->AtPost, Alpha, 25.f), TurnedBy(State->KneelYaw, State->AtPostYaw, Eased));
			}
		});
		Timeline.AddMove(ToPostAt + ToPostSeconds, HandBackAt - ToPostAt - ToPostSeconds, [State](float) { SetView(*State, State->LanternView); });
		Timeline.AddMove(IntoAt, IntoSeconds, [State](float Alpha)
		{
			AAbelKeeper* Abel = State->Abel.Get();
			AKeeperLanternPost* Post = State->Post.Get();
			if (Abel)
			{
				Abel->SetSceneFade(Alpha);
				if (Post)
				{
					// Drawn into the lantern as he fades: a little toward its globe.
					const FVector Toward = FMath::Lerp(State->AtPost, Post->GetKeepersLanternGlobe(), 0.35f * Alpha);
					Abel->SetScenePlace(Toward, State->AtPostYaw);
				}
			}
			if (Post)
			{
				Post->SetCallGlow(FMath::Lerp(0.7f, SunriseGlow, FMath::SmoothStep(0.f, 1.f, Alpha)));
			}
		});
		Timeline.AddMoment(TEXT("Sunrise"), SunriseAt, [State]()
		{
			AKeeperLanternPost* Post = State->Post.Get();
			if (!Post)
			{
				return;
			}
			Post->LightKeepersLantern();
			if (FWeaponFX* Effects = EffectsFor(*State))
			{
				Effects->SpawnStarburst(Post->GetKeepersLanternGlobe(), 220.f, SunriseGold, 7.f, 0.9f, 12);
				Effects->SpawnShockRing(Post->GetKeepersLanternGlobe(), FVector::UpVector, 30.f, 700.f, 26.f, SunriseGold, 3.f, 0.9f, /*bFacing*/ true);
			}
			if (!IsSkipping(*State))
			{
				LooterSound::PlayAt(Post, LooterSoundCue::Element::SoulLightKill, Post->GetKeepersLanternGlobe());
				LooterSound::PlayAt(Post, LooterSoundCue::ChapelBellHum, Post->GetKeepersLanternGlobe(), 0.7f);
				BossCameraShake::Kick(Post, Post->GetKeepersLanternGlobe(), 0.25f, 0.6f);
			}
		});
		Timeline.AddMove(SunriseAt, TakeDownAt - SunriseAt, [State](float Alpha)
		{
			if (AKeeperLanternPost* Post = State->Post.Get())
			{
				// The sunrise settling to a lantern's steady burn, still bright.
				Post->SetCallGlow(FMath::Lerp(SunriseGlow, 1.6f, Alpha));
			}
		});
		// Sully reaches up and takes it down, lit: carried home from here.
		Timeline.AddMoment(TEXT("TakeDown"), TakeDownAt, [State]()
		{
			if (AKeeperLanternPost* Post = State->Post.Get())
			{
				Post->TakeDownLit();
			}
		});
		Timeline.AddMoment(TEXT("HandBack"), HandBackAt, [State]()
		{
			USceneSubsystem* Played = State->Scenes.Get();
			if (!Played)
			{
				return;
			}
			if (State->bHasPlayer)
			{
				const AKeeperLanternPost* Post = State->Post.Get();
				const FVector Toward = (Post ? Post->GetActorLocation() : State->Kneel) - State->PlayerFeet;
				Played->PlaceHeldPlayer(State->PlayerFeet, static_cast<float>(Toward.Rotation().Yaw));
			}
			Played->ViewFromPlayer(Played->IsSkipping() ? 0.f : 1.f);
		});
		Timeline.AddMoment(TEXT("End"), EndAt);
		// Played or skipped, he's in his lantern and it's carried at the end.
		Scene.AfterEnd = [State]()
		{
			Quiet(*State);
			if (AAbelKeeper* Abel = State->Abel.Get())
			{
				Abel->CompleteFreeing(true);
			}
		};
		const bool bPlays = Scenes->Play(MoveTemp(Scene));
		if (bPlays)
		{
			UE_LOG(LogLooter, Log, TEXT("Free Abel: the scene plays (%.0f s)."), EndAt);
		}
		return bPlays;
	}
}

#undef LOCTEXT_NAMESPACE
