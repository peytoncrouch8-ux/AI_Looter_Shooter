// USceneSubsystem: the player while a scene plays (their keys, the HUD, being carried, the scene's camera), the skip keys
// and their prompt, and giving the player back.

#include "Scenes/SceneSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Scenes/SceneSkipPromptWidget.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"

namespace
{
	/** Over every gameplay component's keys and the HUD's menu keys, which all sit at 0. */
	constexpr int32 SceneInputPriority = 1000;

	/** What autosaves wait for while a scene holds the player (USessionSubsystem::HoldSaves). */
	const FName SceneSaveHold(TEXT("Scene"));

	// The shared input assets the character and its interaction component load too.
	const TCHAR* LookActionPath = TEXT("/Game/Input/Actions/IA_Look.IA_Look");
	const TCHAR* MouseLookActionPath = TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook");
	const TCHAR* InteractActionPath = TEXT("/Game/Input/Actions/IA_Interact.IA_Interact");

	UKeyBindingSubsystem* GetKeyBindings(const APlayerController* Controller)
	{
		const ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
		return LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
	}

	/** The player's Interact key for people, "E" by default. */
	FString InteractKeyName(const APlayerController* Controller)
	{
		const UKeyBindingSubsystem* Bindings = GetKeyBindings(Controller);
		const FKey Key = Bindings ? Bindings->GetKey(TEXT("Interact")) : FKey();
		return Key.IsValid() ? Key.GetDisplayName(/*bLongDisplayName*/ false).ToString().ToUpper() : FString(TEXT("E"));
	}

	/** The action is held down now. True when that can't be told (no player input): a hold is never cut short by not knowing. */
	bool IsActionDown(const APlayerController* Controller, const UInputAction* Action)
	{
		const UEnhancedPlayerInput* PlayerInput = Controller ? Cast<UEnhancedPlayerInput>(Controller->PlayerInput) : nullptr;
		return !PlayerInput || !Action || PlayerInput->GetActionValue(Action).Get<bool>();
	}
}

// ---------------------------------------------------------------------------
// Holding the player
// ---------------------------------------------------------------------------

void USceneSubsystem::HoldPlayer(bool bLookOnly)
{
	UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	if (!Controller || !Controller->IsLocalController())
	{
		return;
	}
	APawn* Pawn = Controller->GetPawn();
	bHoldingPlayer = true;
	bCarried = false;
	bSceneCameraView = false;
	HeldController = Controller;
	HeldPawn = Pawn;
	HeldFromView = Controller->GetControlRotation();
	HeldFrom = Pawn ? Pawn->GetActorTransform() : FTransform::Identity;

	if (Pawn)
	{
		// Nothing goes on firing (the button's release won't reach the gun now), and nothing hurts them while they can't move.
		if (UWeaponManagerComponent* Weapons = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			Weapons->StopFire();
		}
		bHeldCanBeDamaged = Pawn->CanBeDamaged();
		Pawn->SetCanBeDamaged(false);
		// Through the player's own eyes: a third-person camera would sit in the skiff's hull, and the cold open picks up from
		// the first-person view the ride ends in.
		if (UPlayerViewComponent* View = Pawn->FindComponentByClass<UPlayerViewComponent>())
		{
			HeldViewMode = static_cast<uint8>(View->GetViewMode());
			View->SetViewMode(EPlayerViewMode::FirstPerson);
		}
	}
	// The menu that started the scene (the station board) may have left the mouse free.
	Controller->SetInputMode(FInputModeGameOnly());
	Controller->SetShowMouseCursor(false);
	BindSceneInput(*Controller, bLookOnly);

	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->HoldSaves(SceneSaveHold);
		bSavesHeld = true;
	}
	if (!SkipPrompt)
	{
		SkipPrompt = CreateWidget<USceneSkipPromptWidget>(Controller, USceneSkipPromptWidget::StaticClass());
	}
	if (SkipPrompt && !SkipPrompt->IsInViewport())
	{
		SkipPrompt->AddToViewport(USceneSkipPromptWidget::ViewportZOrder);
	}
}

void USceneSubsystem::ReleasePlayer(const FTransform* PutBack, const FRotator* View)
{
	PendingReturn = nullptr;
	HeldAfterEnd = -1.f;
	if (SkipPrompt)
	{
		SkipPrompt->RemoveFromParent();
	}
	if (!bHoldingPlayer)
	{
		return;
	}
	APlayerController* Controller = HeldController.Get();
	APawn* Pawn = HeldPawn.Get();
	UnbindSceneInput();
	if (Pawn)
	{
		if (bCarried)
		{
			Pawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
		if (PutBack)
		{
			Pawn->TeleportTo(PutBack->GetLocation(), PutBack->Rotator(), /*bIsATest*/ false, /*bNoCheck*/ true);
		}
		ACharacter* Character = Cast<ACharacter>(Pawn);
		UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
		if (Movement && bCarried)
		{
			// On their own feet again: walking finds the ground under them, or they fall to it.
			Movement->SetMovementMode(Movement->DefaultLandMovementMode);
		}
		Pawn->SetCanBeDamaged(bHeldCanBeDamaged);
		if (UPlayerViewComponent* PlayerView = Pawn->FindComponentByClass<UPlayerViewComponent>())
		{
			PlayerView->SetViewMode(static_cast<EPlayerViewMode>(HeldViewMode));
		}
	}
	if (Controller)
	{
		if (View)
		{
			Controller->SetControlRotation(*View);
		}
		if (bSceneCameraView && Pawn)
		{
			Controller->SetViewTarget(Pawn);
		}
	}
	if (bSavesHeld)
	{
		if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
		{
			Sessions->ReleaseSaves(SceneSaveHold);
		}
		bSavesHeld = false;
	}
	bHoldingPlayer = false;
	bCarried = false;
	bSceneCameraView = false;
	HeldController.Reset();
	HeldPawn.Reset();
}

void USceneSubsystem::ReturnFromScene(const FText& Title)
{
	if (Current.IsValid())
	{
		UE_LOG(LogLooter, Warning, TEXT("Scene: %s is still playing; it gives the player back when it ends."), *Current->Name.ToString());
		return;
	}
	const TFunction<void()> PutThingsBack = MoveTemp(PendingReturn);
	PendingReturn = nullptr;
	HeldAfterEnd = -1.f;
	if (PutThingsBack)
	{
		PutThingsBack();
	}
	const bool bWasHolding = bHoldingPlayer;
	const FTransform From = HeldFrom;
	const FRotator FromView = HeldFromView;
	ReleasePlayer(&From, &FromView);
	UTransitionScreenSubsystem* Screen = UTransitionScreenSubsystem::Get(this);
	if (Screen && (Screen->IsShowing() || !Title.IsEmpty()))
	{
		Screen->Reveal(Title);
	}
	if (bWasHolding)
	{
		UE_LOG(LogLooter, Log, TEXT("Scene: no trip after it, so the player is back where it took them from."));
	}
}

bool USceneSubsystem::HidesGameplayHUD(const UObject* WorldContextObject)
{
	const USceneSubsystem* Scenes = Get(WorldContextObject);
	return Scenes && Scenes->bHoldingPlayer;
}

APawn* USceneSubsystem::GetHeldPawn() const
{
	return HeldPawn.Get();
}

void USceneSubsystem::CarryPlayer(USceneComponent* Carrier, const FVector& Feet, float Yaw)
{
	APawn* Pawn = HeldPawn.Get();
	if (!Carrier || !Pawn)
	{
		return;
	}
	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			// Carried, not walking: walking would try to stand them on the moving deck by itself, and fight it.
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
	}
	// Standing on the spot (a pawn's location is its middle), then fixed to the carrier wherever it goes.
	const FVector Middle = Feet + FVector(0.0, 0.0, Pawn->GetSimpleCollisionHalfHeight() + 2.0);
	Pawn->SetActorLocationAndRotation(Middle, FRotator(0.0, Yaw, 0.0), /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
	Pawn->AttachToComponent(Carrier, FAttachmentTransformRules::KeepWorldTransform);
	if (APlayerController* Controller = HeldController.Get())
	{
		// Looking the way it faces, a little down at the deck's bow and the sky ahead.
		Controller->SetControlRotation(FRotator(-3.0, Yaw, 0.0));
	}
	bCarried = true;
}

void USceneSubsystem::TurnHeldView(float DeltaYaw)
{
	APlayerController* Controller = HeldController.Get();
	if (Controller && !FMath::IsNearlyZero(DeltaYaw))
	{
		FRotator View = Controller->GetControlRotation();
		View.Yaw = FRotator::ClampAxis(View.Yaw + DeltaYaw);
		Controller->SetControlRotation(View);
	}
}

ACameraActor* USceneSubsystem::ViewFromSceneCamera(float BlendSeconds)
{
	UWorld* World = GetWorld();
	if (!SceneCamera && World)
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		SceneCamera = World->SpawnActor<ACameraActor>(Params);
	}
	APlayerController* Controller = HeldController.Get();
	if (Controller && SceneCamera)
	{
		Controller->SetViewTargetWithBlend(SceneCamera.Get(), BlendSeconds);
		bSceneCameraView = true;
	}
	return SceneCamera;
}

// ---------------------------------------------------------------------------
// The scene's keys
// ---------------------------------------------------------------------------

void USceneSubsystem::BindSceneInput(APlayerController& Controller, bool bLookOnly)
{
	UnbindSceneInput();
	UEnhancedInputComponent* Input = NewObject<UEnhancedInputComponent>(&Controller);
	Input->RegisterComponent();
	// Over every other key in the game and blocking them all: no moving, firing, using, menus or view changes while a
	// scene plays. They get it all back the moment this goes.
	Input->Priority = SceneInputPriority;
	Input->bBlockInput = true;
	if (bLookOnly)
	{
		for (const TCHAR* Path : { LookActionPath, MouseLookActionPath })
		{
			if (const UInputAction* Look = LoadObject<UInputAction>(nullptr, Path))
			{
				Input->BindAction(Look, ETriggerEvent::Triggered, this, &USceneSubsystem::HandleLook);
			}
		}
	}
	const UInputAction* Interact = LoadObject<UInputAction>(nullptr, InteractActionPath);
	InteractAction = Interact;
	if (Interact)
	{
		Input->BindAction(Interact, ETriggerEvent::Started, this, &USceneSubsystem::HandleSkipKeyDown);
		Input->BindAction(Interact, ETriggerEvent::Completed, this, &USceneSubsystem::HandleSkipKeyUp);
		Input->BindAction(Interact, ETriggerEvent::Canceled, this, &USceneSubsystem::HandleSkipKeyUp);
	}
	const UKeyBindingSubsystem* Bindings = GetKeyBindings(&Controller);
	if (Bindings && Bindings->GetPauseAction())
	{
		Input->BindAction(Bindings->GetPauseAction(), ETriggerEvent::Started, this, &USceneSubsystem::HandleEscape);
	}
	Controller.PushInputComponent(Input);
	SceneInput = Input;
}

void USceneSubsystem::UnbindSceneInput()
{
	if (UEnhancedInputComponent* Input = SceneInput.Get())
	{
		if (APlayerController* Controller = HeldController.Get())
		{
			Controller->PopInputComponent(Input);
		}
		Input->DestroyComponent();
	}
	SceneInput = nullptr;
	bSkipKeyDown = false;
	SkipKeyHeld = 0.f;
}

void USceneSubsystem::HandleLook(const FInputActionValue& Value)
{
	// As the character turns its view (ALooterCharacter::Look), with nothing ever aimed during a scene.
	APlayerController* Controller = HeldController.Get();
	const FVector2D Look = Value.Get<FVector2D>();
	if (Controller)
	{
		Controller->AddYawInput(Look.X);
		Controller->AddPitchInput(Look.Y);
	}
}

bool USceneSubsystem::CanSkipByKey() const
{
	return Current.IsValid() && Current->Timeline.GetTime() >= SkipKeysAfter;
}

void USceneSubsystem::HandleSkipKeyDown()
{
	if (!CanSkipByKey())
	{
		return;
	}
	bSkipKeyDown = true;
	SkipKeyHeld = 0.f;
	PromptLeft = SkipPromptSeconds;
	bPromptForEscape = false;
}

void USceneSubsystem::HandleSkipKeyUp()
{
	bSkipKeyDown = false;
	SkipKeyHeld = 0.f;
}

void USceneSubsystem::HandleEscape()
{
	if (!CanSkipByKey())
	{
		return;
	}
	if (EscapeWait > 0.f)
	{
		SkipScene();
		return;
	}
	// The first Escape asks, and a second while the prompt shows skips: Escape opens the pause menu everywhere else, so
	// one press out of habit never throws a scene away.
	EscapeWait = SkipPromptSeconds;
	PromptLeft = SkipPromptSeconds;
	bPromptForEscape = true;
}

void USceneSubsystem::UpdateSkipKeys(float DeltaSeconds)
{
	// A release that never came (the window lost focus): the key is up, and the hold starts over.
	if (bSkipKeyDown && !IsActionDown(HeldController.Get(), InteractAction.Get()))
	{
		HandleSkipKeyUp();
	}
	SkipKeyHeld = bSkipKeyDown ? SkipKeyHeld + DeltaSeconds : 0.f;
	EscapeWait = FMath::Max(EscapeWait - DeltaSeconds, 0.f);
	PromptLeft = bSkipKeyDown ? SkipPromptSeconds : FMath::Max(PromptLeft - DeltaSeconds, 0.f);

	if (SkipPrompt)
	{
		const bool bShown = Current.IsValid() && PromptLeft > 0.f;
		if (bPromptForEscape && !bSkipKeyDown)
		{
			SkipPrompt->Update(bShown, TEXT("PRESS"), TEXT("[ESC]"), TEXT("AGAIN TO SKIP"), 0.f, DeltaSeconds);
		}
		else
		{
			const FString Key = bShown ? FString::Printf(TEXT("[%s]"), *InteractKeyName(HeldController.Get())) : FString();
			const float Hold = bSkipKeyDown ? FMath::Clamp(SkipKeyHeld / SkipHoldSeconds, 0.f, 1.f) : 0.f;
			SkipPrompt->Update(bShown, TEXT("HOLD"), Key, TEXT("TO SKIP"), Hold, DeltaSeconds);
		}
	}

	if (bSkipKeyDown && SkipKeyHeld >= SkipHoldSeconds && Current.IsValid())
	{
		HandleSkipKeyUp();
		SkipScene();
	}
}
