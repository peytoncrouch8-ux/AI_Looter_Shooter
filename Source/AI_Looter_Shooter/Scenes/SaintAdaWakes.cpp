// SaintAdaWakes: the scene as Abel's lantern is set on Saint Ada's Reliquary (Main 7's payoff): his light into her, her blaze,
// the wave of light over the valley, the dead taken west, Abel himself by her, Delia at the door. The world's side (the end
// state, who's in it, the Unpaid going quiet) is SaintAdaWakesWorld.cpp.

#include "Scenes/SaintAdaWakes.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/AbelOnBoard.h"
#include "Story/CaptionSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/CarriedLantern.h"
#include "World/CarriedLanternRules.h"
#include "World/ChapelBell.h"
#include "World/ChapelReliquary.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterRules.h"
#include "World/LightingStateSubsystem.h"
#include "World/ReliquaryLantern.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "LooterSaintAda"

namespace SaintAdaWakes
{
	namespace
	{
		// When things happen (s).
		constexpr float FinchBackAt = 0.4f;
		constexpr float FinchBackSeconds = 1.6f;
		constexpr float PourAt = 1.4f;
		constexpr float CatchAt = 4.4f;
		constexpr float BlazeUpSeconds = 1.6f;
		constexpr float BellAt = 5.2f;
		constexpr float OutsideAt = 7.f;
		constexpr float WaveAt = 7.6f;
		constexpr float WaveSeconds = 6.f;
		/** Just after the river rises: the shrouds lift off the boards as their lights go, the coal burns off the Rest. */
		constexpr float AwakeAt = 13.2f;
		constexpr float PointAt = 12.f;
		constexpr float RiverAt = 12.6f;
		constexpr float InsideAt = 18.f;
		constexpr float BlazeDownSeconds = 2.f;
		constexpr float LineAt[] = { 19.f, 22.4f, 25.2f, 29.f };
		constexpr float HandBackAt = 33.4f;
		constexpr float EndAt = 34.6f;

		/** The views (cm): close on the lid from the nave; high over the knoll looking west; Abel; Delia. */
		constexpr float CloseOut = 330.f;
		constexpr float CloseUp = 120.f;
		const FVector WideFrom(0.0, 1800.0, 2600.0);
		const FVector WideLook(0.0, -26000.0, -900.0);
		constexpr float WideFov = 70.f;
		constexpr float PointFov = 26.f;
		/** The finale's FOV (the camera's own, given back as it was). */
		constexpr float CloseFov = 60.f;
		/** Finch steps back this far from the Reliquary. */
		constexpr float FinchBack = 130.f;
		/** The motes pouring from the lantern into her, and the light's colour. */
		constexpr float MoteEvery = 0.06f;
		const FLinearColor KeeperGold(1.f, 0.85f, 0.55f);
		const FLinearColor WakeWhite(1.f, 0.94f, 0.78f);
		/** The wave: a ring of light growing from the knoll out over the valley, in the air over it (cm). */
		constexpr float WaveUp = 1400.f;
		constexpr float WaveFrom = 400.f;
		constexpr float WaveTo = 65000.f;
		constexpr float WaveWidth = 1600.f;
		/** The Unpaid sink this far into the ground over this long. */
		constexpr float SinkDepth = 240.f;
		constexpr float SinkSeconds = 3.f;
		constexpr float QuietReach = 30000.f;

		struct FState
		{
			TWeakObjectPtr<USceneSubsystem> Scenes;
			TWeakObjectPtr<AReliquaryLantern> Rest;
			TWeakObjectPtr<AChapelReliquary> Saint;
			TWeakObjectPtr<AAbelOnBoard> Abel;
			TWeakObjectPtr<AActor> Delia;
			TWeakObjectPtr<AActor> Finch;
			FVector FinchFrom = FVector::ZeroVector;
			FVector FinchTo = FVector::ZeroVector;
			FVector Ember = FVector::ZeroVector;
			FVector Point = FVector::ZeroVector;
			bool bHasPoint = false;
			FTransform Close;
			FTransform Wide;
			FTransform OnPoint;
			FTransform OnAbel;
			FTransform OnDelia;
			float OwnFov = 90.f;
			float MoteWait = 0.f;
			TArray<FStoryLine> Said;
			TArray<TWeakObjectPtr<AActor>> Sinking;
			TArray<FVector> SinkFrom;
			bool bSinkStarted = false;
		};
		using FStateRef = TSharedRef<FState>;

		bool IsSkipping(const FState& State)
		{
			const USceneSubsystem* Scenes = State.Scenes.Get();
			return !Scenes || Scenes->IsSkipping();
		}

		FTransform Looking(const FVector& From, const FVector& At)
		{
			return FTransform((At - From).Rotation(), From);
		}

		ACameraActor* Camera(const FState& State)
		{
			const USceneSubsystem* Scenes = State.Scenes.Get();
			return Scenes ? Scenes->GetSceneCamera() : nullptr;
		}

		void SetView(const FState& State, const FTransform& View, float Fov)
		{
			if (ACameraActor* Lens = Camera(State))
			{
				Lens->SetActorLocationAndRotation(View.GetLocation(), View.GetRotation());
				if (UCameraComponent* Component = Lens->GetCameraComponent())
				{
					Component->SetFieldOfView(Fov);
				}
			}
		}

		void Between(const FState& State, const FTransform& From, const FTransform& To, float Alpha, float FromFov, float ToFov)
		{
			const float Eased = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
			SetView(State, FTransform(FQuat::Slerp(From.GetRotation(), To.GetRotation(), Eased), FMath::Lerp(From.GetLocation(), To.GetLocation(), Eased)),
				FMath::Lerp(FromFov, ToFov, Eased));
		}

		FWeaponFX* Effects(const FState& State)
		{
			const AReliquaryLantern* Rest = State.Rest.Get();
			return Rest && !IsSkipping(State) ? LamplighterEffects::Find(Rest) : nullptr;
		}

		void Say(const FState& State, int32 Line)
		{
			if (IsSkipping(State) || !State.Said.IsValidIndex(Line))
			{
				return;
			}
			if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(State.Rest.Get()))
			{
				Captions->Play({ State.Said[Line] }, ECaptionPlay::Interrupt);
			}
		}

		FStateRef MakeState(USceneSubsystem& Scenes, AReliquaryLantern& Rest)
		{
			FStateRef State = MakeShared<FState>();
			State->Scenes = &Scenes;
			State->Rest = &Rest;
			State->Saint = Rest.GetReliquary();
			State->Abel = FindAbelSoul(Rest);
			State->Delia = FindDelia(Rest);
			State->Finch = FindFinch(Rest);
			State->Said = Lines();
			State->bHasPoint = FindPoint(Rest.GetWorld(), State->Point);
			const AChapelReliquary* Saint = State->Saint.Get();
			State->Ember = Saint ? Saint->GetEmberStart() : Rest.GetActorLocation();
			// Close on the lid, from the nave (her front, +X) a little to one side, Abel's light and her ember in frame.
			const FVector Front = Saint ? Saint->GetActorForwardVector().GetSafeNormal2D() : Rest.GetActorForwardVector().GetSafeNormal2D();
			const FVector Side = FVector::CrossProduct(FVector::UpVector, Front);
			State->Close = Looking(State->Ember + Front * CloseOut + Side * 90.f + FVector(0.0, 0.0, CloseUp), State->Ember + FVector(0.0, 0.0, 20.0));
			// High over the knoll, east of it, looking west over the valley toward the sunset.
			const FVector Knoll = Rest.GetActorLocation();
			State->Wide = Looking(Knoll + WideFrom, Knoll + WideLook);
			State->OnPoint = State->bHasPoint ? Looking(Knoll + WideFrom, State->Point + FVector(0.0, 0.0, 1500.0)) : State->Wide;
			if (AActor* Finch = State->Finch.Get())
			{
				State->FinchFrom = Finch->GetActorLocation();
				const FVector Away = (State->FinchFrom - State->Ember).GetSafeNormal2D();
				State->FinchTo = State->FinchFrom + (Away.IsNearlyZero() ? Front : Away) * FinchBack;
			}
			// Abel by her, seen over her lid from the nave's side; Delia in the door, seen past him.
			const AAbelOnBoard* Abel = State->Abel.Get();
			const FVector AbelHead = Abel ? Abel->GetHeadLocation() : State->Ember + FVector(0.0, 0.0, 120.0);
			State->OnAbel = Looking(AbelHead + Front * 260.f - Side * 140.f + FVector(0.0, 0.0, -10.0), AbelHead - FVector(0.0, 0.0, 15.0));
			const AActor* Delia = State->Delia.Get();
			const FVector DeliaHead = Delia ? Delia->GetActorLocation() + FVector(0.0, 0.0, 55.0) : State->Ember + Front * 1500.f;
			State->OnDelia = Looking(AbelHead + (DeliaHead - AbelHead).GetSafeNormal() * 120.f + Side * 50.f, DeliaHead);
			if (const ACameraActor* Lens = Scenes.GetSceneCamera())
			{
				if (const UCameraComponent* Component = Lens->GetCameraComponent())
				{
					State->OwnFov = Component->FieldOfView;
				}
			}
			return State;
		}

		/** Abel's light pours out of the lantern down into her: gold motes spiralling off the globe and sinking into the lid. */
		void PourMote(FState& State, float DeltaSeconds)
		{
			State.MoteWait -= DeltaSeconds;
			FWeaponFX* FX = Effects(State);
			const AReliquaryLantern* Rest = State.Rest.Get();
			if (State.MoteWait > 0.f || !FX || !Rest)
			{
				return;
			}
			State.MoteWait = MoteEvery;
			const float Angle = FMath::FRandRange(0.f, UE_TWO_PI);
			const FVector Out(FMath::Cos(Angle) * 14.0, FMath::Sin(Angle) * 14.0, 6.0);
			const FVector From = Rest->GetGlobe() + Out;
			FX->SpawnFireball(From, (State.Ember - From) * 1.6f + FVector(0.0, 0.0, -30.0), 7.f, 2.f, KeeperGold, 6.f, 0.6f);
		}
	}

	FName SceneName()
	{
		return FName(TEXT("SaintAdaWakes"));
	}

	float Duration()
	{
		return EndAt;
	}

	bool Play(AReliquaryLantern& Rest)
	{
		USceneSubsystem* Scenes = USceneSubsystem::Get(&Rest);
		if (!Scenes || !USceneSubsystem::AreScenesOn(Rest.GetWorld()))
		{
			return false;
		}
		const FStateRef State = MakeState(*Scenes, Rest);
		FScenePlay Scene;
		Scene.Name = SceneName();
		// The scene's eyes are Sully's; the mouse doesn't move them.
		Scene.bLookOnly = false;
		Scene.OnStart = [State]()
		{
			if (USceneSubsystem* Played = State->Scenes.Get())
			{
				Played->SetHeldPlayerHidden(true);
				Played->ViewFromSceneCamera(0.7f);
			}
			SetView(*State, State->Close, CloseFov);
			UE_LOG(LogLooter, Log, TEXT("Saint Ada Wakes: Abel's lantern on her lid."));
		};
		Scene.OnTick = [State](float DeltaSeconds)
		{
			USceneSubsystem* Played = State->Scenes.Get();
			const float Time = Played ? Played->GetSceneTime() : 0.f;
			if (Time >= PourAt && Time < CatchAt + 0.6f)
			{
				PourMote(*State, DeltaSeconds);
			}
		};
		FSceneTimeline& Timeline = Scene.Timeline;

		// Inside: Finch steps back from her; the lantern flares and his light pours into her recess.
		Timeline.AddMove(0.f, OutsideAt, [State](float) { SetView(*State, State->Close, CloseFov); });
		Timeline.AddMove(FinchBackAt, FinchBackSeconds, [State](float Alpha)
		{
			if (AActor* Finch = State->Finch.Get())
			{
				Finch->SetActorLocation(FMath::Lerp(State->FinchFrom, State->FinchTo, FMath::SmoothStep(0.f, 1.f, Alpha)));
			}
		});
		Timeline.AddMoment(TEXT("Pour"), PourAt, [State]()
		{
			AReliquaryLantern* Rest = State->Rest.Get();
			if (!Rest || IsSkipping(*State))
			{
				return;
			}
			LooterSound::PlayAt(Rest, LooterSoundCue::Element::SoulLightApply, Rest->GetGlobe(), 1.f, 0.7f);
			LooterSound::PlayAt(Rest, LooterSoundCue::ChapelBellHum, Rest->GetGlobe(), 0.8f);
			if (UEnemyProjectileSubsystem* Glow = Rest->GetWorld()->GetSubsystem<UEnemyProjectileSubsystem>())
			{
				Glow->ShowCharge(Rest, Rest->GetActorTransform().InverseTransformPosition(Rest->GetGlobe()), CatchAt - PourAt, 22.f, KeeperGold);
			}
		});
		Timeline.AddMove(PourAt, CatchAt - PourAt, [State](float Alpha)
		{
			// The lantern flares as it gives itself up, then dims as his light leaves it for her.
			if (AReliquaryLantern* Rest = State->Rest.Get())
			{
				Rest->ShowLantern(true, 0.6f + 1.2f * FMath::Sin(UE_PI * Alpha));
			}
		});

		// The coal under ash catches, and Saint Ada blazes white-gold; her bell sounds once in the tower by itself.
		Timeline.AddMoment(TEXT("Catch"), CatchAt, [State]()
		{
			if (AChapelReliquary* Saint = State->Saint.Get())
			{
				Saint->SetLitNow(true);
			}
			if (FWeaponFX* FX = Effects(*State))
			{
				FX->SpawnStarburst(State->Ember + FVector(0.0, 0.0, 15.0), 160.f, WakeWhite, 14.f, 0.8f, 10);
				FX->SpawnShockRing(State->Ember, FVector::UpVector, 20.f, 900.f, 60.f, WakeWhite, 8.f, 1.2f);
			}
			if (!IsSkipping(*State))
			{
				LooterSound::Play2D(State->Rest.Get(), LooterSoundCue::StingBossVictory, 0.9f);
				LamplighterEffects::Shake(State->Rest.Get(), 0.25f, 0.8f);
			}
		});
		Timeline.AddMove(CatchAt, BlazeUpSeconds, [State](float Alpha)
		{
			if (AChapelReliquary* Saint = State->Saint.Get())
			{
				Saint->SetBlaze(FMath::SmoothStep(0.f, 1.f, Alpha));
			}
		});
		Timeline.AddMoment(TEXT("Bell"), BellAt, [State]()
		{
			if (const AReliquaryLantern* Rest = State->Rest.Get(); Rest && !IsSkipping(*State))
			{
				RingHerBell(Rest->GetWorld());
			}
		});

		// Outside, over the knoll looking west: the wave of light, the smog burning off, the dead going quiet.
		Timeline.AddMove(OutsideAt, PointAt - OutsideAt, [State](float Alpha)
		{
			// A slow push out over the valley as the wave goes.
			FTransform Pushed = State->Wide;
			Pushed.AddToTranslation(State->Wide.GetRotation().GetForwardVector() * (900.0 * Alpha));
			SetView(*State, Pushed, WideFov);
		});
		Timeline.AddMoment(TEXT("Wave"), WaveAt, [State]()
		{
			AReliquaryLantern* Rest = State->Rest.Get();
			if (!Rest)
			{
				return;
			}
			if (ULightingStateSubsystem* Light = ULightingStateSubsystem::Get(Rest))
			{
				Light->SetState(TEXT("Dusk"), IsSkipping(*State) ? ELightingSwitch::Instant : ELightingSwitch::Blend,
					IsSkipping(*State) ? 0.f : WaveSeconds);
			}
			if (FWeaponFX* FX = Effects(*State))
			{
				const FVector Knoll = Rest->GetActorLocation();
				FX->SpawnShockRing(Knoll + FVector(0.0, 0.0, WaveUp), FVector::UpVector, WaveFrom, WaveTo, WaveWidth, WakeWhite, 9.f, WaveSeconds);
				FX->SpawnShockRing(Knoll + FVector(0.0, 0.0, 200.0), FVector::UpVector, WaveFrom, WaveTo * 0.6f, WaveWidth * 0.5f, KeeperGold, 6.f,
					WaveSeconds * 0.8f);
			}
			if (!IsSkipping(*State))
			{
				LooterSound::Play2D(Rest, LooterSoundCue::IntroWhiteout, 0.8f);
			}
			// The Unpaid across the Rest go quiet now; the scene lowers them into their graves.
			State->Sinking.Reset();
			QuietUnpaid(Rest->GetWorld(), Rest->GetActorLocation(), QuietReach, &State->Sinking);
			State->SinkFrom.Reset();
			for (const TWeakObjectPtr<AActor>& Each : State->Sinking)
			{
				State->SinkFrom.Add(Each.IsValid() ? Each->GetActorLocation() : FVector::ZeroVector);
			}
			State->bSinkStarted = true;
		});
		Timeline.AddMove(WaveAt + 0.6f, SinkSeconds, [State](float Alpha)
		{
			for (int32 Index = 0; Index < State->Sinking.Num(); ++Index)
			{
				AActor* Unpaid = State->Sinking[Index].Get();
				if (Unpaid && State->SinkFrom.IsValidIndex(Index))
				{
					Unpaid->SetActorLocation(State->SinkFrom[Index] - FVector(0.0, 0.0, SinkDepth * FMath::SmoothStep(0.f, 1.f, Alpha)));
				}
			}
		});
		Timeline.AddMoment(TEXT("Awake"), AwakeAt, [State]()
		{
			// For good from here: the coal burnt off the Rest, the shrouds lifted, Abel's soul by her (out of sight yet).
			if (AReliquaryLantern* Rest = State->Rest.Get())
			{
				// Not instant: the light's own blend (the wave's) goes on; the scene's sinking dead finish sinking.
				ApplyEndState(*Rest, /*bInstant*/ false);
				if (AChapelReliquary* Saint = State->Saint.Get())
				{
					// The end state cools her blaze; through the scene she's still blazing.
					Saint->SetBlaze(1.f);
				}
			}
		});
		Timeline.AddMoment(TEXT("Sunk"), WaveAt + 0.6f + SinkSeconds, [State]()
		{
			for (const TWeakObjectPtr<AActor>& Each : State->Sinking)
			{
				if (AActor* Unpaid = Each.Get())
				{
					Unpaid->Destroy();
				}
			}
			State->Sinking.Reset();
		});

		// Toward Gravewind Point: the Gravewind takes the six dead and the coal's souls west into the sunset.
		Timeline.AddMove(PointAt, 1.4f, [State](float Alpha) { Between(*State, State->Wide, State->OnPoint, Alpha, WideFov, PointFov); });
		Timeline.AddMove(PointAt + 1.4f, InsideAt - PointAt - 1.4f, [State](float Alpha)
		{
			// Following the river a little west as it goes.
			FTransform Drift = State->OnPoint;
			Drift.SetRotation(Drift.GetRotation() * FQuat(FVector::UpVector, FMath::DegreesToRadians(-6.f * Alpha)));
			SetView(*State, Drift, PointFov);
		});
		Timeline.AddMoment(TEXT("River"), RiverAt, [State]()
		{
			AReliquaryLantern* Rest = State->Rest.Get();
			if (!Rest)
			{
				return;
			}
			if (UCarriedLanternSubsystem* Carried = UCarriedLanternSubsystem::Get(Rest))
			{
				Carried->ReleaseFollowers(DeadOnTheBoards(Rest->GetWorld()));
			}
			if (!IsSkipping(*State) && State->bHasPoint)
			{
				LooterSound::PlayAt(Rest, LooterSoundCue::AbelWindRise, State->Point, 1.f);
			}
		});

		// Inside again: Abel himself by her, Delia in the door, their words; her blaze settles to her steady light.
		Timeline.AddMoment(TEXT("Inside"), InsideAt, [State]()
		{
			if (AAbelOnBoard* Abel = State->Abel.Get())
			{
				Abel->ShowNow();
			}
		});
		Timeline.AddMove(InsideAt, BlazeDownSeconds, [State](float Alpha)
		{
			if (AChapelReliquary* Saint = State->Saint.Get())
			{
				Saint->SetBlaze(1.f - FMath::SmoothStep(0.f, 1.f, Alpha));
			}
		});
		Timeline.AddMove(InsideAt, LineAt[1] - InsideAt, [State](float) { SetView(*State, State->OnAbel, CloseFov); });
		Timeline.AddMoment(TEXT("Knees"), LineAt[0], [State]() { Say(*State, 0); });
		Timeline.AddMove(LineAt[1], 0.8f, [State](float Alpha) { Between(*State, State->OnAbel, State->OnDelia, Alpha, CloseFov, CloseFov); });
		Timeline.AddMoment(TEXT("Door"), LineAt[1], [State]() { Say(*State, 1); });
		Timeline.AddMove(LineAt[2], 0.8f, [State](float Alpha) { Between(*State, State->OnDelia, State->OnAbel, Alpha, CloseFov, CloseFov); });
		Timeline.AddMoment(TEXT("Keep"), LineAt[2], [State]() { Say(*State, 2); });
		Timeline.AddMoment(TEXT("Warden"), LineAt[3], [State]() { Say(*State, 3); });
		Timeline.AddMove(LineAt[2] + 0.8f, HandBackAt - LineAt[2] - 0.8f, [State](float) { SetView(*State, State->OnAbel, CloseFov); });

		// Sully's own eyes back, facing Delia in the door, for her turn-in.
		Timeline.AddMoment(TEXT("HandBack"), HandBackAt, [State]()
		{
			USceneSubsystem* Played = State->Scenes.Get();
			if (!Played)
			{
				return;
			}
			if (ACameraActor* Lens = Played->GetSceneCamera())
			{
				if (UCameraComponent* Component = Lens->GetCameraComponent())
				{
					Component->SetFieldOfView(State->OwnFov);
				}
			}
			const APawn* Player = Played->GetHeldPawn();
			const AActor* Delia = State->Delia.Get();
			if (Player && Delia)
			{
				const float Half = Player->GetRootComponent() ? static_cast<float>(Player->GetRootComponent()->Bounds.BoxExtent.Z) : 90.f;
				const FVector Feet = Player->GetActorLocation() - FVector(0.0, 0.0, Half);
				Played->PlaceHeldPlayer(Feet, static_cast<float>((Delia->GetActorLocation() - Feet).Rotation().Yaw));
			}
			Played->ViewFromPlayer(Played->IsSkipping() ? 0.f : 1.f);
		});
		Timeline.AddMoment(TEXT("End"), EndAt);
		// Played or skipped, the world as the scene leaves it.
		Scene.AfterEnd = [State]()
		{
			if (AReliquaryLantern* Rest = State->Rest.Get())
			{
				ApplyEndState(*Rest, /*bInstant*/ false);
			}
			if (AChapelReliquary* Saint = State->Saint.Get())
			{
				Saint->SetBlaze(0.f);
			}
			if (AActor* Finch = State->Finch.Get())
			{
				Finch->SetActorLocation(State->FinchTo);
			}
			for (const TWeakObjectPtr<AActor>& Each : State->Sinking)
			{
				if (AActor* Unpaid = Each.Get())
				{
					Unpaid->Destroy();
				}
			}
			State->Sinking.Reset();
		};
		const bool bPlays = Scenes->Play(MoveTemp(Scene));
		if (bPlays)
		{
			UE_LOG(LogLooter, Log, TEXT("Saint Ada Wakes: the scene plays (%.0f s)."), EndAt);
		}
		return bPlays;
	}
}

#undef LOCTEXT_NAMESPACE
