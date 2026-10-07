// The grave wake-up: clawing out of Ellis's grave (Main 1, step 2), its rules and its scene.

#include "Scenes/GraveWake.h"
#include "AI_Looter_Shooter.h"
#include "Combat/BulletSubsystem.h"
#include "Scenes/GraveClawPromptWidget.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/LightingStates.h"
#include "World/LightingStateSubsystem.h"
#include "World/WorldQueries.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/HitResult.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"

namespace
{
	/** The model the grave is (Art/Models/Props/Graves.py), and its sockets: where the body rises, the headboard's face. */
	const TCHAR* GraveModelName = TEXT("SM_Grave_Ellis");
	const FName RespawnSocket(TEXT("Respawn"));
	const FName InteractSocket(TEXT("Interact"));

	/** The same in the model's own measurements, for a grave whose model has no sockets (cm, its frame). */
	const FVector FallbackRise(10.0, 0.0, 12.0);
	const FVector FallbackBoard(-190.0, -10.0, 55.0);

	/** The jump the claws are, as the character's own (the settings rebind the key, not the action). */
	const TCHAR* JumpActionPath = TEXT("/Game/Input/Actions/IA_Jump.IA_Jump");

	/** Lying in the coffin: the eyes this far toward the headboard's end from the hole's middle and this high (cm). */
	constexpr float CoffinHeadward = 70.f;
	constexpr float CoffinEyeHeight = 18.f;

	/** Looking up past the grave's rim, a little toward the foot; sat up once out, looking out over the foot (degrees). */
	constexpr float LyingPitch = 78.f;
	constexpr float SatUpPitch = 16.f;

	/** How far the claws raise the view out of the coffin (cm): sitting up in it. */
	constexpr float ClawRise = 62.f;

	/** How hard a claw jolts the view at most (degrees). */
	constexpr float JoltDegrees = 4.f;

	/** Standing: Ellis's eyes over their feet (cm), looking a little down at the headboard (degrees). */
	constexpr float StandingEye = 165.f;
	constexpr float StandingPitch = -12.f;

	/** The climb out lifts the view this much more at its middle, over the heap (cm). */
	constexpr float ClimbLift = 25.f;

	/** The view goes back to the player's eyes over this long, at the end of the climb (seconds). */
	constexpr float HandBackSeconds = 0.7f;

	/** A beat in the dark before the prompt shows (seconds). */
	constexpr float PromptAfter = 0.8f;

	UStaticMeshComponent* FindModel(const AActor& Grave)
	{
		TInlineComponentArray<UStaticMeshComponent*> Meshes(&Grave);
		for (UStaticMeshComponent* Mesh : Meshes)
		{
			const UStaticMesh* Model = Mesh ? Mesh->GetStaticMesh() : nullptr;
			if (Model && Model->GetName() == GraveModelName)
			{
				return Mesh;
			}
		}
		return nullptr;
	}

	/** The ground's height under Where, from a little above it (the terrain, never a volume or the grave's own boxes). */
	float GroundUnder(const UWorld* World, const FVector& Where, float Otherwise, const AActor* Grave)
	{
		if (!World)
		{
			return Otherwise;
		}
		FHitResult Hit;
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("GraveWakeGround"), Grave);
		const bool bHit = World->LineTraceSingleByObjectType(Hit, Where + FVector(0.0, 0.0, 200.0), Where - FVector(0.0, 0.0, 300.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		return bHit ? Hit.ImpactPoint.Z : Otherwise;
	}
}

// ---------------------------------------------------------------------------
// The claws' rules
// ---------------------------------------------------------------------------

bool FGraveClawOut::Press(float Now)
{
	if (IsOut() || Now - LastPress < PressSpacing)
	{
		return false;
	}
	++Presses;
	LastPress = Now;
	return true;
}

bool FGraveClawOut::IsSettled(float Now) const
{
	return IsOut() && Now - LastPress >= RiseSeconds;
}

float FGraveClawOut::RiseAt(float Now) const
{
	if (Presses <= 0)
	{
		return 0.f;
	}
	// From where the claws before the last one left it, easing on to where the last one takes it.
	const float From = static_cast<float>(Presses - 1) / PressesNeeded;
	const float To = static_cast<float>(FMath::Min(Presses, PressesNeeded)) / PressesNeeded;
	const float Alpha = FMath::SmoothStep(0.f, 1.f, (Now - LastPress) / RiseSeconds);
	return Alpha >= 1.f ? To : FMath::Lerp(From, To, Alpha);
}

float FGraveClawOut::DarknessAt(float Now) const
{
	// Light gets in faster than Ellis rises: about half of it with the first claw, nearly all with the second, and none
	// left once through.
	const float Rise = FMath::Clamp(RiseAt(Now), 0.f, 1.f);
	return Rise >= 1.f ? 0.f : BuriedDarkness * FMath::Pow(1.f - Rise, 1.5f);
}

float FGraveClawOut::JoltAt(float Now) const
{
	return Presses > 0 ? FMath::Exp(-9.f * FMath::Max(Now - LastPress, 0.f)) : 0.f;
}

// ---------------------------------------------------------------------------
// The grave
// ---------------------------------------------------------------------------

namespace GraveWake
{
	FName SceneName()
	{
		return FName(TEXT("GraveWake"));
	}

	FName GraveTag()
	{
		return FName(TEXT("Grave_Ellis"));
	}

	AActor* FindGrave(const UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Candidate = *It;
			if (Candidate && !Candidate->IsActorBeingDestroyed() && (Candidate->ActorHasTag(GraveTag()) || FindModel(*Candidate)))
			{
				return Candidate;
			}
		}
		return nullptr;
	}

	FGraveWakeSpots SpotsFor(AActor& Grave)
	{
		FGraveWakeSpots Spots;
		Spots.Grave = &Grave;
		const UStaticMeshComponent* Model = FindModel(Grave);
		const FTransform Frame = Model ? Model->GetComponentTransform() : Grave.GetActorTransform();

		// Where the body rises, facing out of the grave's foot (its +X), level.
		const FTransform Rise = Model && Model->DoesSocketExist(RespawnSocket) ? Model->GetSocketTransform(RespawnSocket)
			: FTransform(Frame.GetRotation(), Frame.TransformPosition(FallbackRise));
		FVector Foot = Frame.GetUnitAxis(EAxis::X);
		Foot.Z = 0.0;
		Foot = Foot.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		Spots.Up = FVector::UpVector;
		Spots.Hole = Rise.GetLocation();

		// Lying in the coffin, the head toward the headboard: looking up (the view's top toward the head) and a little
		// toward the foot.
		const FVector Eye = Spots.Hole - Foot * CoffinHeadward + Spots.Up * CoffinEyeHeight;
		Spots.Coffin = FTransform(FRotator(LyingPitch, Foot.Rotation().Yaw, 0.0), Eye);

		// Out at the foot, past the heap, on the ground there; turned back to the headboard.
		Spots.Stand = Spots.Hole + Foot * StandOut;
		Spots.Stand.Z = GroundUnder(Grave.GetWorld(), Spots.Stand, Spots.Hole.Z, &Grave);
		Spots.LookAt = Model && Model->DoesSocketExist(InteractSocket) ? Model->GetSocketLocation(InteractSocket)
			: Frame.TransformPosition(FallbackBoard);
		FVector ToBoard = Spots.LookAt - Spots.Stand;
		ToBoard.Z = 0.0;
		Spots.StandYaw = ToBoard.IsNearlyZero() ? (-Foot).Rotation().Yaw : ToBoard.Rotation().Yaw;
		return Spots;
	}

	// -----------------------------------------------------------------------
	// The scene
	// -----------------------------------------------------------------------

	struct FState
	{
		TWeakObjectPtr<USceneSubsystem> Scenes;
		FGraveWakeSpots Spots;
		FGraveClawOut Claw;

		/** Seconds since it began, through the wait (the scene's own clock stops while it waits). */
		float Clock = 0.f;

		/** The view while clawing, and where the climb out starts from (where the last claw left it). */
		FTransform View = FTransform::Identity;
		FTransform OutView = FTransform::Identity;
		bool bClimbing = false;

		TWeakObjectPtr<UGraveClawPromptWidget> Prompt;
		FString KeyName = TEXT("SPACE BAR");
		FRandomStream Shake{ 0x6a7e };
	};

	namespace
	{
		/** Puts the scene's camera at View (the player's view is on it from the start until it's handed back). */
		void SetView(FState& State, const FTransform& View)
		{
			const USceneSubsystem* Scenes = State.Scenes.Get();
			if (ACameraActor* Camera = Scenes ? Scenes->GetSceneCamera() : nullptr)
			{
				Camera->SetActorLocationAndRotation(View.GetLocation(), View.GetRotation());
			}
		}

		APlayerCameraManager* CameraManager(const FState& State)
		{
			const USceneSubsystem* Scenes = State.Scenes.Get();
			const APlayerController* Controller = Scenes ? Scenes->GetHeldController() : nullptr;
			return Controller ? Controller->PlayerCameraManager.Get() : nullptr;
		}

		/** How dark the view is: black over it, as the camera's own fade (0 clears it). */
		void SetDarkness(const FState& State, float Darkness)
		{
			if (APlayerCameraManager* Camera = CameraManager(State))
			{
				if (Darkness > 0.f)
				{
					Camera->SetManualCameraFade(Darkness, FLinearColor::Black, /*bInFadeAudio*/ false);
				}
				else
				{
					Camera->StopCameraFade();
				}
			}
		}

		FString JumpKeyName(const APlayerController* Controller)
		{
			const ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
			const UKeyBindingSubsystem* Bindings = LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
			const FKey Key = Bindings ? Bindings->GetKey(TEXT("Jump")) : FKey();
			return Key.IsValid() ? Key.GetDisplayName(/*bLongDisplayName*/ false).ToString().ToUpper() : FString(TEXT("SPACE BAR"));
		}

		void Start(FState& State)
		{
			USceneSubsystem* Scenes = State.Scenes.Get();
			if (!Scenes)
			{
				return;
			}
			// Seven days later, a golden afternoon: the light goes back to Day while the screen is still dark.
			ULightingStateSubsystem* Lighting = ULightingStateSubsystem::Get(Scenes);
			if (Lighting && Lighting->GetStateNames().Contains(ALightingStates::DayState))
			{
				Lighting->SetState(ALightingStates::DayState, ELightingSwitch::Instant);
			}
			// The player stands already where they'll be once out, unseen; the view is the scene's, in the coffin.
			Scenes->PlaceHeldPlayer(State.Spots.Stand, State.Spots.StandYaw);
			Scenes->SetHeldPlayerHidden(true);
			Scenes->ViewFromSceneCamera(0.f);
			State.View = State.Spots.Coffin;
			SetView(State, State.View);
			SetDarkness(State, FGraveClawOut::BuriedDarkness);

			APlayerController* Controller = Scenes->GetHeldController();
			if (Controller && Controller->IsLocalController())
			{
				State.KeyName = JumpKeyName(Controller);
				if (UGraveClawPromptWidget* Prompt = CreateWidget<UGraveClawPromptWidget>(Controller, UGraveClawPromptWidget::StaticClass()))
				{
					Prompt->AddToViewport(UGraveClawPromptWidget::ViewportZOrder);
					State.Prompt = Prompt;
				}
			}
			UE_LOG(LogLooter, Log, TEXT("Grave wake-up: in the coffin at %s, waiting for %d claws."), *State.Spots.Hole.ToCompactString(),
				FGraveClawOut::PressesNeeded);
		}

		/** Every frame: the clock, the prompt, and while still clawing the view and the dark. */
		void TickClaws(FState& State, float DeltaSeconds)
		{
			State.Clock += DeltaSeconds;
			if (UGraveClawPromptWidget* Prompt = State.Prompt.Get())
			{
				const bool bShown = !State.Claw.IsOut() && State.Clock >= PromptAfter;
				Prompt->Update(bShown, State.KeyName, State.Claw.GetPresses(), DeltaSeconds);
			}
			if (State.bClimbing)
			{
				// The climb out has the view now.
				return;
			}
			// Risen as far as the claws have taken it, sitting up out of the coffin, jolted by the last claw.
			const float Rise = State.Claw.RiseAt(State.Clock);
			const float Jolt = State.Claw.JoltAt(State.Clock) * JoltDegrees;
			const FTransform& Coffin = State.Spots.Coffin;
			FRotator Rotation = Coffin.Rotator();
			Rotation.Pitch = FMath::Lerp(LyingPitch, SatUpPitch, Rise);
			Rotation += FRotator(State.Shake.FRandRange(-Jolt, Jolt), State.Shake.FRandRange(-Jolt, Jolt) * 0.6f, State.Shake.FRandRange(-Jolt, Jolt));
			State.View = FTransform(Rotation, Coffin.GetLocation() + State.Spots.Up * (Rise * ClawRise));
			SetView(State, State.View);
			SetDarkness(State, State.Claw.DarknessAt(State.Clock));
		}

		/** Out of the grave: from where the claws left the view to standing at its foot, turned back to the headboard. */
		void Climb(FState& State, float Alpha)
		{
			const float Eased = FMath::SmoothStep(0.f, 1.f, Alpha);
			const FVector Eye = State.Spots.Stand + State.Spots.Up * StandingEye;
			// Skipped before the claws were in, it starts from the coffin.
			const FTransform& From = State.bClimbing ? State.OutView : State.Spots.Coffin;
			const FVector Location = FMath::Lerp(From.GetLocation(), Eye, Eased) + State.Spots.Up * (ClimbLift * FMath::Sin(UE_PI * Eased));
			const FQuat Standing = FRotator(StandingPitch, State.Spots.StandYaw, 0.0).Quaternion();
			const FQuat Rotation = FQuat::Slerp(From.GetRotation(), Standing, Eased);
			State.View = FTransform(Rotation, Location);
			SetView(State, State.View);
		}

		/** It has ended, played or skipped: the prompt goes, and nothing is left over the view. */
		void Finish(FState& State)
		{
			if (UGraveClawPromptWidget* Prompt = State.Prompt.Get())
			{
				Prompt->RemoveFromParent();
			}
			State.Prompt.Reset();
			SetDarkness(State, 0.f);
			UE_LOG(LogLooter, Log, TEXT("Grave wake-up: out, after %d claws."), State.Claw.GetPresses());
		}
	}

	bool Press(FState& State)
	{
		if (!State.Claw.Press(State.Clock))
		{
			return false;
		}
		const int32 Claws = State.Claw.GetPresses();
		UE_LOG(LogLooter, Log, TEXT("Grave wake-up: claw %d of %d."), Claws, FGraveClawOut::PressesNeeded);
		// Grave dirt thrown up through the hole, more with each claw: the last breaks out into the afternoon.
		USceneSubsystem* Scenes = State.Scenes.Get();
		UWorld* World = Scenes ? Scenes->GetWorld() : nullptr;
		if (UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr)
		{
			FWeaponFX& Effects = Bullets->GetEffects();
			Effects.Initialize(World);
			Effects.SpawnDirt(State.Spots.Hole + State.Spots.Up * 12.f, State.Spots.Up, static_cast<float>(Claws) / FGraveClawOut::PressesNeeded);
		}
		return true;
	}

	int32 GetPresses(const FState& State)
	{
		return State.Claw.GetPresses();
	}

	FScenePlay Make(USceneSubsystem& Scenes, const FGraveWakeSpots& Spots, TSharedPtr<FState>& OutState)
	{
		const TSharedRef<FState> State = MakeShared<FState>();
		State->Scenes = &Scenes;
		State->Spots = Spots;
		State->View = Spots.Coffin;
		OutState = State;

		FScenePlay Scene;
		Scene.Name = SceneName();
		// The view is the scene's: the claws move it, and the mouse doesn't.
		Scene.bLookOnly = false;
		Scene.OnStart = [State]() { Start(*State); };
		Scene.OnTick = [State](float DeltaSeconds) { TickClaws(*State, DeltaSeconds); };
		Scene.BindKeys = [WeakState = TWeakPtr<FState>(State)](UEnhancedInputComponent& Input)
		{
			// Jump claws, through the player's own binding of it; nothing else hears it meanwhile.
			if (const UInputAction* Jump = LoadObject<UInputAction>(nullptr, JumpActionPath))
			{
				Input.BindActionValueLambda(Jump, ETriggerEvent::Started, [WeakState](const FInputActionValue&)
				{
					if (const TSharedPtr<FState> Pinned = WeakState.Pin())
					{
						Press(*Pinned);
					}
				});
			}
		};

		// The scene waits in the dark for the claws; once the last has settled, the climb out starts from where it left
		// the view.
		Scene.Timeline.AddWait(ClawWaitAt, [State]()
		{
			if (!State->Claw.IsSettled(State->Clock))
			{
				return false;
			}
			State->OutView = State->View;
			State->bClimbing = true;
			return true;
		});
		// Just past the wait, so nothing of the climb happens while the claws have the view: through the dirt, into the light.
		Scene.Timeline.AddMove(ClawWaitAt + 0.01f, ClimbSeconds, [State](float Alpha) { Climb(*State, Alpha); });
		Scene.Timeline.AddMoment(TEXT("Out"), ClawWaitAt + 0.01f, [State]() { SetDarkness(*State, 0.f); });
		Scene.Timeline.AddMoment(TEXT("Standing"), ClawWaitAt + ClimbSeconds - HandBackSeconds, [State]()
		{
			// The view eases back into the player's own eyes as they come to their feet.
			if (USceneSubsystem* Played = State->Scenes.Get())
			{
				Played->ViewFromPlayer(Played->IsSkipping() ? 0.f : HandBackSeconds);
			}
		});
		Scene.Timeline.AddMoment(TEXT("Up"), ClawWaitAt + ClimbSeconds + 0.1f);
		Scene.AfterEnd = [State]() { Finish(*State); };
		return Scene;
	}
}
