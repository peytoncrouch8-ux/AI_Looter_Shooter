#include "Settings/KeyBindingSubsystem.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

// UKeyBindingSubsystem's combat actions, built in code with their default keys: the melee strike, the grenade, the ember
// power, the soul flask, and Drop weapon's own default (moved off G for the grenade). Each is a static builder so the tests
// check the very mapping the settings menu rebinds. The rest of the subsystem is KeyBindingSubsystem.cpp.

FName UKeyBindingSubsystem::MeleeBindingId()
{
	return TEXT("Melee");
}

FKey UKeyBindingSubsystem::DefaultMeleeKey()
{
	return EKeys::V;
}

FKey UKeyBindingSubsystem::DefaultMeleeGamepadKey()
{
	return EKeys::Gamepad_RightThumbstick;
}

UInputAction* UKeyBindingSubsystem::AddMeleeAction(UObject* Outer, UInputMappingContext& Context)
{
	// Built in code like the other character actions: no input asset to keep in step.
	UInputAction* Action = NewObject<UInputAction>(Outer, TEXT("IA_Melee"));
	Context.MapKey(Action, DefaultMeleeKey());
	Context.MapKey(Action, DefaultMeleeGamepadKey());
	return Action;
}

FName UKeyBindingSubsystem::GrenadeBindingId()
{
	return TEXT("Grenade");
}

FKey UKeyBindingSubsystem::DefaultGrenadeKey()
{
	return EKeys::G;
}

FKey UKeyBindingSubsystem::DefaultGrenadeGamepadKey()
{
	return EKeys::Gamepad_RightShoulder;
}

UInputAction* UKeyBindingSubsystem::AddGrenadeAction(UObject* Outer, UInputMappingContext& Context)
{
	// Built in code like the melee action: no input asset to keep in step.
	UInputAction* Action = NewObject<UInputAction>(Outer, TEXT("IA_Grenade"));
	Context.MapKey(Action, DefaultGrenadeKey());
	Context.MapKey(Action, DefaultGrenadeGamepadKey());
	return Action;
}

FName UKeyBindingSubsystem::EmberBindingId()
{
	return TEXT("Ember");
}

FKey UKeyBindingSubsystem::DefaultEmberKey()
{
	return EKeys::Q;
}

FKey UKeyBindingSubsystem::DefaultEmberGamepadKey()
{
	return EKeys::Gamepad_LeftShoulder;
}

UInputAction* UKeyBindingSubsystem::AddEmberAction(UObject* Outer, UInputMappingContext& Context)
{
	// Built in code like the grenade action: no input asset to keep in step.
	UInputAction* Action = NewObject<UInputAction>(Outer, TEXT("IA_Ember"));
	Context.MapKey(Action, DefaultEmberKey());
	Context.MapKey(Action, DefaultEmberGamepadKey());
	return Action;
}

FKey UKeyBindingSubsystem::DefaultDropWeaponKey()
{
	// Out of the way of the fight's keys (G throws now; Q and F are kept for the ember powers), and never pressed by
	// accident while running and gunning.
	return EKeys::X;
}

FKey UKeyBindingSubsystem::DropWeaponAssetKey()
{
	return EKeys::G;
}

FName UKeyBindingSubsystem::FlaskBindingId()
{
	return TEXT("Flask");
}

FKey UKeyBindingSubsystem::DefaultFlaskKey()
{
	return EKeys::H;
}

FKey UKeyBindingSubsystem::DefaultFlaskGamepadKey()
{
	return EKeys::Gamepad_DPad_Left;
}

UInputAction* UKeyBindingSubsystem::AddFlaskAction(UObject* Outer, UInputMappingContext& Context)
{
	// Built in code like the grenade and ember actions: no input asset to keep in step.
	UInputAction* Action = NewObject<UInputAction>(Outer, TEXT("IA_Flask"));
	Context.MapKey(Action, DefaultFlaskKey());
	Context.MapKey(Action, DefaultFlaskGamepadKey());
	return Action;
}
