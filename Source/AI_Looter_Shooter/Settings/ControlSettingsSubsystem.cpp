#include "Settings/ControlSettingsSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* ControlsSaveSlot = TEXT("ControlSettings");
}

void UControlSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Read once here: everything that turns or shakes the view asks for its value as it needs it, so nothing to apply.
	SaveData = LoadControls(ControlsSaveSlot, this);
}

ULooterControlsSave* UControlSettingsSubsystem::LoadControls(const FString& Slot, UObject* Outer)
{
	// Asked first: reading a slot that isn't there logs a warning, and a fresh install has none.
	if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		if (ULooterControlsSave* Saved = Cast<ULooterControlsSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0)))
		{
			return Saved;
		}
	}
	// A fresh install (or a damaged save): the controls as the game was made.
	return NewObject<ULooterControlsSave>(Outer ? Outer : GetTransientPackage());
}

float UControlSettingsSubsystem::ClampLookSensitivity(float Sensitivity)
{
	if (!FMath::IsFinite(Sensitivity))
	{
		return DefaultLookSensitivity;
	}
	// The slider's steps, so the menu's label and the saved value always agree.
	return FMath::Clamp(FMath::GridSnap(Sensitivity, LookSensitivityStep), MinLookSensitivity, MaxLookSensitivity);
}

float UControlSettingsSubsystem::LookSensitivityOf(const ULooterControlsSave* Controls)
{
	return Controls ? ClampLookSensitivity(Controls->LookSensitivity) : DefaultLookSensitivity;
}

FVector2D UControlSettingsSubsystem::ScaleLookInput(const FVector2D& Input, float Sensitivity)
{
	return Input * ClampLookSensitivity(Sensitivity);
}

FVector2D UControlSettingsSubsystem::ScaleLookInputFor(const AController* Controller, const FVector2D& Input)
{
	// Only a local player has the setting; anything else turns as the game was made.
	const APlayerController* Player = Cast<APlayerController>(Controller);
	const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	const UControlSettingsSubsystem* Controls = LocalPlayer ? LocalPlayer->GetSubsystem<UControlSettingsSubsystem>() : nullptr;
	return Controls ? ScaleLookInput(Input, Controls->GetLookSensitivity()) : Input;
}

float UControlSettingsSubsystem::GetLookSensitivity() const
{
	return LookSensitivityOf(SaveData);
}

void UControlSettingsSubsystem::SetLookSensitivity(float Sensitivity, bool bSave)
{
	if (!SaveData)
	{
		return;
	}
	SaveData->LookSensitivity = ClampLookSensitivity(Sensitivity);
	if (bSave)
	{
		SaveSettings();
	}
}

float UControlSettingsSubsystem::ClampCameraShake(float Shake)
{
	if (!FMath::IsFinite(Shake))
	{
		return DefaultCameraShake;
	}
	return FMath::Clamp(FMath::GridSnap(Shake, CameraShakeStep), MinCameraShake, MaxCameraShake);
}

float UControlSettingsSubsystem::CameraShakeOf(const ULooterControlsSave* Controls)
{
	return Controls ? ClampCameraShake(Controls->CameraShake) : DefaultCameraShake;
}

float UControlSettingsSubsystem::GetCameraShake() const
{
	return CameraShakeOf(SaveData);
}

void UControlSettingsSubsystem::SetCameraShake(float Shake, bool bSave)
{
	if (!SaveData)
	{
		return;
	}
	SaveData->CameraShake = ClampCameraShake(Shake);
	if (bSave)
	{
		SaveSettings();
	}
}

void UControlSettingsSubsystem::SaveSettings() const
{
	if (SaveData)
	{
		UGameplayStatics::SaveGameToSlot(SaveData, ControlsSaveSlot, 0);
	}
}
