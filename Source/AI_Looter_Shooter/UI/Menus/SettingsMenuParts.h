#pragma once

#include "CoreMinimal.h"
#include "Settings/GraphicsSettingsSubsystem.h"

/** What the settings menu's three .cpp files share: its buttons' actions and the key list's columns. */
namespace SettingsMenu
{
	/** Resume or Back (and the corner's X). */
	inline const FName ActionClose(TEXT("Close"));
	inline const FName ActionSaveQuit(TEXT("SaveQuit"));
	inline const FName ActionRebind(TEXT("Rebind"));
	inline const FName ActionResetOne(TEXT("ResetOne"));
	inline const FName ActionResetAll(TEXT("ResetAll"));
	inline const FName ActionQuality(TEXT("Quality"));
	inline const FName ActionMotionBlurOn(TEXT("MotionBlurOn"));
	inline const FName ActionMotionBlurOff(TEXT("MotionBlurOff"));
	inline const FName ActionMinimapOn(TEXT("MinimapOn"));
	inline const FName ActionMinimapOff(TEXT("MinimapOff"));
	inline const FName ActionFrameRateOn(TEXT("FrameRateOn"));
	inline const FName ActionFrameRateOff(TEXT("FrameRateOff"));
	inline const FName ActionHoldMode(TEXT("HoldMode"));
	inline const FName ActionToggleMode(TEXT("ToggleMode"));

	// Columns of the key list, shared by the key rows and the hold/toggle rows under them.
	inline constexpr float KeyColumnWidth = 180.f;
	inline constexpr float DefaultColumnWidth = 96.f;
	inline constexpr float QualityColumnWidth = 340.f;

	/** The quality presets, in the order of their buttons. */
	inline constexpr EGraphicsQuality Qualities[] = { EGraphicsQuality::Low, EGraphicsQuality::Medium, EGraphicsQuality::High, EGraphicsQuality::Epic };
}
