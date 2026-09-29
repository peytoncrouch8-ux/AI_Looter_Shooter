#include "Settings/PawnInputBinding.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"

namespace
{
	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem(const APlayerController* PC)
	{
		const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
		return LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	}
}

UEnhancedInputComponent* FPawnInputBinding::Setup(AActor* Owner, AController* InController, UInputMappingContext* InContext, int32 Priority)
{
	Teardown();

	APlayerController* PC = Cast<APlayerController>(InController);
	if (!Owner || !PC || !PC->IsLocalController())
	{
		return nullptr;
	}

	UEnhancedInputComponent* Input = NewObject<UEnhancedInputComponent>(Owner);
	Input->RegisterComponent();
	PC->PushInputComponent(Input);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem(PC); Subsystem && InContext)
	{
		Subsystem->AddMappingContext(InContext, Priority);
	}

	Controller = PC;
	Component = Input;
	Context = InContext;
	return Input;
}

void FPawnInputBinding::Teardown()
{
	APlayerController* PC = Controller.Get();
	UEnhancedInputComponent* Input = Component.Get();
	if (PC && Input)
	{
		PC->PopInputComponent(Input);
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem(PC); Subsystem && Context.IsValid())
	{
		Subsystem->RemoveMappingContext(Context.Get());
	}
	if (Input)
	{
		Input->DestroyComponent();
	}
	Controller.Reset();
	Component.Reset();
	Context.Reset();
}

UKeyBindingSubsystem* FPawnInputBinding::GetBindings(const AController* InController)
{
	const APlayerController* PC = Cast<APlayerController>(InController);
	const ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
}
