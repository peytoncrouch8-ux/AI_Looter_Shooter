#pragma once

#include "CoreMinimal.h"

class AActor;
class AController;
class APlayerController;
class UEnhancedInputComponent;
class UInputMappingContext;
class UKeyBindingSubsystem;

/**
 * A gameplay component's own input: an Enhanced Input component pushed onto the local player's controller plus the
 * mapping context that feeds it. Setup and Teardown always come in pairs, so possession changes (Build Mode, respawn)
 * never leave stale bindings behind, and the character Blueprint needs no input wiring.
 */
class AI_LOOTER_SHOOTER_API FPawnInputBinding
{
public:
	/**
	 * Tears down any previous binding, then pushes a fresh input component owned by Owner onto Controller and adds
	 * Context. Returns the component to bind actions on, or null if Controller isn't a local player.
	 */
	UEnhancedInputComponent* Setup(AActor* Owner, AController* Controller, UInputMappingContext* Context, int32 Priority);
	void Teardown();

	bool IsBound() const { return Controller.IsValid(); }
	APlayerController* GetController() const { return Controller.Get(); }

	/** The key binding subsystem of a controller's local player (rebindable keys, hold/toggle modes). */
	static UKeyBindingSubsystem* GetBindings(const AController* Controller);

private:
	TWeakObjectPtr<APlayerController> Controller;
	TWeakObjectPtr<UEnhancedInputComponent> Component;
	TWeakObjectPtr<UInputMappingContext> Context;
};
