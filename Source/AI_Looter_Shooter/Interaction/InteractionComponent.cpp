// UInteractionComponent: its life on the player, the Interact key's tap and hold, and using things.

#include "Interaction/InteractionComponent.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/Interactable.h"
#include "Interaction/InteractionSource.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

UInteractionComponent::UInteractionComponent()
{
	// Ticks only for the local player (SetupInput turns it on): every frame it looks for what the player means to use.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// The shared Interact action, the one the settings rebind; the character's Blueprint could point elsewhere.
	static ConstructorHelpers::FObjectFinder<UInputAction> InteractAsset(TEXT("/Game/Input/Actions/IA_Interact.IA_Interact"));
	InteractAction = InteractAsset.Object;
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UInteractionComponent::HandleControllerChanged);
		SetupInput(Pawn->GetController());
	}
}

void UInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TeardownInput();
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UInteractionComponent::HandleControllerChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// A release that never arrived (a menu took the input, alt-tab): the press is over, and it does nothing.
	if (bPressed && !IsInteractKeyDown())
	{
		ResetPress();
	}
	UpdateInteraction(DeltaTime);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void UInteractionComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	SetupInput(NewController);
}

void UInteractionComponent::SetupInput(AController* Controller)
{
	TeardownInput();

	// The key is mapped in the weapon controls, which the weapon manager adds with its own input; this binds the action
	// on an input component of its own, with no mapping context of its own to add or take away.
	UEnhancedInputComponent* Input = InputBinding.Setup(GetOwner(), Controller, nullptr, 0);
	if (!Input)
	{
		return;
	}
	if (InteractAction)
	{
		Input->BindAction(InteractAction, ETriggerEvent::Started, this, &UInteractionComponent::PressInteract);
		Input->BindAction(InteractAction, ETriggerEvent::Completed, this, &UInteractionComponent::ReleaseInteract);
		Input->BindAction(InteractAction, ETriggerEvent::Canceled, this, &UInteractionComponent::ReleaseInteract);
	}
	else
	{
		UE_LOG(LogLooter, Warning, TEXT("%s: no Interact action, so nothing can be used."), *GetPathName());
	}

	// Only the local player looks around for things to use.
	SetComponentTickEnabled(true);
}

void UInteractionComponent::TeardownInput()
{
	// Losing input (death, unpossess) means the release may never arrive.
	ResetPress();
	InputBinding.Teardown();
	SetComponentTickEnabled(false);
	SetFocus(nullptr);
}

bool UInteractionComponent::IsInteractKeyDown() const
{
	const APlayerController* PC = InputBinding.GetController();
	const UEnhancedPlayerInput* PlayerInput = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
	if (!PlayerInput || !InteractAction)
	{
		return true;
	}
	return PlayerInput->GetActionValue(InteractAction).Get<bool>();
}

// ---------------------------------------------------------------------------
// The key's tap and hold
// ---------------------------------------------------------------------------

float UInteractionComponent::GetHoldProgress() const
{
	return bPressed && PressHoldSeconds > 0.f ? FMath::Clamp(PressHeldSeconds / PressHoldSeconds, 0.f, 1.f) : 0.f;
}

void UInteractionComponent::PressInteract()
{
	ResetPress();
	AActor* Target = FocusedActor.Get();
	if (!Target || !FocusedOptions.CanUse())
	{
		return;
	}
	if (FocusedOptions.bHold)
	{
		// Nothing yet: the hold's time makes it a hold, and letting go before that a tap (when it takes one).
		bPressed = true;
		bPressTaps = FocusedOptions.bTap;
		PressedActor = Target;
		PressHeldSeconds = 0.f;
		PressHoldSeconds = FMath::Max(FocusedOptions.HoldSeconds, UE_KINDA_SMALL_NUMBER);
		return;
	}
	// Only a tap: nothing to wait for.
	UseFocused(/*bHeld*/ false);
}

void UInteractionComponent::ReleaseInteract()
{
	if (!bPressed)
	{
		return;
	}
	const AActor* Target = PressedActor.Get();
	const bool bTaps = bPressTaps;
	ResetPress();
	// Let go before the hold's time: a tap, when it takes one and the player still looks at it (a tap never uses
	// something else).
	if (bTaps && Target && Target == FocusedActor.Get())
	{
		UseFocused(/*bHeld*/ false);
	}
}

void UInteractionComponent::UpdateInteraction(float DeltaSeconds)
{
	RefreshFocus();
	if (!bPressed)
	{
		return;
	}
	// Looking away, or what the key went down on going away or no longer usable, ends the press: no tap, no hold.
	if (!PressedActor.IsValid() || PressedActor.Get() != FocusedActor.Get())
	{
		ResetPress();
		return;
	}
	PressHeldSeconds += DeltaSeconds;
	if (PressHeldSeconds >= PressHoldSeconds)
	{
		CompleteHold();
	}
}

void UInteractionComponent::CompleteHold()
{
	const AActor* Target = PressedActor.Get();
	ResetPress();
	if (Target && Target == FocusedActor.Get())
	{
		UseFocused(/*bHeld*/ true);
	}
}

void UInteractionComponent::ResetPress()
{
	bPressed = false;
	bPressTaps = false;
	PressedActor.Reset();
	PressHeldSeconds = 0.f;
	PressHoldSeconds = 0.f;
}

// ---------------------------------------------------------------------------
// Using things
// ---------------------------------------------------------------------------

bool UInteractionComponent::UseFocused(bool bHeld)
{
	AActor* Target = FocusedActor.Get();
	if (!Target)
	{
		return false;
	}
	IInteractionSource* Source = Cast<IInteractionSource>(FocusedSource.Get());
	IInteractable* Interactable = Source ? nullptr : Cast<IInteractable>(Target);
	if (!Source && !Interactable)
	{
		return false;
	}

	// Asked again as it's used: it may have changed since the look (a cooldown, a timer, something else using it).
	const FInteractionOptions Options = Source ? Source->GetOfferOptions(*Target) : Interactable->GetInteractionOptions(*this);
	if (!Options.bUsable || !(bHeld ? Options.bHold : Options.bTap))
	{
		return false;
	}
	const bool bDone = Source ? Source->UseOffer(*Target, bHeld) : Interactable->Interact(*this, bHeld);
	if (!bDone)
	{
		return false;
	}

	UE_LOG(LogLooter, Verbose, TEXT("Interaction: %s %s."), bHeld ? TEXT("held on") : TEXT("tapped"), *Target->GetName());
	// Missions hear of every use once: interact objectives (the posters, the hay bales) and hold ones (the bell).
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Interaction(Target, bHeld));
	}
	OnInteracted.Broadcast(Target, bHeld);

	// What was used may have changed (loot taken, a door that now closes, a lantern lit): look again at once, so the
	// HUD never shows the old words for a frame.
	RefreshFocus();
	return true;
}
