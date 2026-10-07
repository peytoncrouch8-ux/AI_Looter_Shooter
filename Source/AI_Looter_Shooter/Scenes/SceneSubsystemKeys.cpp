// USceneSubsystem: the scene's keys while it holds the player (look, the scene's own, the skip keys) and the skip prompt.

#include "Scenes/SceneSubsystem.h"
#include "Scenes/SceneSkipPromptWidget.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"

namespace
{
	/** Over every gameplay component's keys and the HUD's menu keys, which all sit at 0. */
	constexpr int32 SceneInputPriority = 1000;

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
	// The scene's own keys (the claw-out's Jump), which nothing else hears while it plays.
	if (Current.IsValid() && Current->BindKeys)
	{
		Current->BindKeys(*Input);
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
	// As the character turns its view (ALooterCharacter::Look), at the player's look sensitivity, with nothing ever aimed
	// during a scene.
	APlayerController* Controller = HeldController.Get();
	const FVector2D Look = UControlSettingsSubsystem::ScaleLookInputFor(Controller, Value.Get<FVector2D>());
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
