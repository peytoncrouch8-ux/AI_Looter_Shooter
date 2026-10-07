// USceneSubsystem: the player while a scene plays (the HUD, being carried or stood somewhere, hidden, the scene's camera)
// and giving the player back. Their keys meanwhile, and the skip prompt, are SceneSubsystemKeys.cpp's.

#include "Scenes/SceneSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Player/PlayerViewComponent.h"
#include "Scenes/SceneSkipPromptWidget.h"
#include "Scenes/TransitionScreenSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** What autosaves wait for while a scene holds the player (USessionSubsystem::HoldSaves). */
	const FName SceneSaveHold(TEXT("Scene"));
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
	SetHeldPlayerHidden(false);
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

APlayerController* USceneSubsystem::GetHeldController() const
{
	return HeldController.Get();
}

void USceneSubsystem::PlaceHeldPlayer(const FVector& Feet, float Yaw)
{
	APawn* Pawn = HeldPawn.Get();
	if (!Pawn)
	{
		return;
	}
	if (bCarried)
	{
		Pawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		ACharacter* Character = Cast<ACharacter>(Pawn);
		UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
		if (Movement)
		{
			Movement->SetMovementMode(Movement->DefaultLandMovementMode);
		}
		bCarried = false;
	}
	// Standing on the spot: a pawn's location is its middle.
	const FRotator Facing(0.0, Yaw, 0.0);
	const FVector Middle = Feet + FVector(0.0, 0.0, Pawn->GetSimpleCollisionHalfHeight() + 2.0);
	Pawn->TeleportTo(Middle, Facing, /*bIsATest*/ false, /*bNoCheck*/ true);
	if (APlayerController* Controller = HeldController.Get())
	{
		Controller->SetControlRotation(Facing);
	}
}

void USceneSubsystem::SetHeldPlayerHidden(bool bHidden)
{
	APawn* Pawn = HeldPawn.Get();
	if (bHidden == bHeldHidden || (bHidden && !Pawn))
	{
		return;
	}
	if (bHidden)
	{
		// The pawn and what it carries (its guns are actors of their own), but only what shows now: what something
		// else hid stays its business.
		TArray<AActor*> Carried;
		Pawn->GetAttachedActors(Carried, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);
		Carried.Insert(Pawn, 0);
		HiddenActors.Reset();
		for (AActor* Each : Carried)
		{
			if (Each && !Each->IsHidden())
			{
				Each->SetActorHiddenInGame(true);
				HiddenActors.Add(Each);
			}
		}
	}
	else
	{
		for (const TWeakObjectPtr<AActor>& Each : HiddenActors)
		{
			if (AActor* Shown = Each.Get())
			{
				Shown->SetActorHiddenInGame(false);
			}
		}
		HiddenActors.Reset();
	}
	bHeldHidden = bHidden;
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

ACameraActor* USceneSubsystem::GetSceneCamera() const
{
	return SceneCamera.Get();
}

void USceneSubsystem::ViewFromPlayer(float BlendSeconds)
{
	APlayerController* Controller = HeldController.Get();
	APawn* Pawn = HeldPawn.Get();
	if (Controller && Pawn && bSceneCameraView)
	{
		// From here the release has nothing to cut back: the view is on its way to the player's eyes.
		Controller->SetViewTargetWithBlend(Pawn, BlendSeconds, VTBlend_EaseInOut, 2.f);
		bSceneCameraView = false;
	}
}
