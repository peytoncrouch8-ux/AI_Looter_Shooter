#pragma once

#include "CoreMinimal.h"
#include "Settings/GraphicsSettingsSubsystem.h"

/** What the settings menu's .cpp files share: its buttons' actions, the key list's columns and value labels. */
namespace SettingsMenu
{
	/** Resume or Back (and the corner's X). */
	inline const FName ActionClose(TEXT("Close"));
	/** A tab along the top; Index is its page (ESettingsPage, SettingsMenuHelp.h). */
	inline const FName ActionTab(TEXT("Tab"));
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
	/** A crosshair colour; Index is its ECrosshairColour (AllCrosshairColours' order). */
	inline const FName ActionCrosshairColour(TEXT("CrosshairColour"));
	inline const FName ActionHintsOn(TEXT("HintsOn"));
	inline const FName ActionHintsOff(TEXT("HintsOff"));
	inline const FName ActionSubtitlesOn(TEXT("SubtitlesOn"));
	inline const FName ActionSubtitlesOff(TEXT("SubtitlesOff"));
	inline const FName ActionHoldMode(TEXT("HoldMode"));
	inline const FName ActionToggleMode(TEXT("ToggleMode"));

	// Columns of the key list, shared by the key rows and the hold/toggle rows under them.
	inline constexpr float KeyColumnWidth = 180.f;
	inline constexpr float DefaultColumnWidth = 96.f;
	inline constexpr float QualityColumnWidth = 340.f;

	/** The quality presets, in the order of their buttons. */
	inline constexpr EGraphicsQuality Qualities[] = { EGraphicsQuality::Low, EGraphicsQuality::Medium, EGraphicsQuality::High, EGraphicsQuality::Epic };

	/** The field of view slider moves a whole degree at a time. */
	inline constexpr float FieldOfViewStep = 1.f;

	/** The panel's size: wide enough for a page's rows beside the description column, and the same on every page. */
	inline constexpr float PanelWidth = 940.f;
	inline constexpr float PanelHeight = 840.f;
	/** The description column beside the page. */
	inline constexpr float HelpColumnWidth = 272.f;
	/** A tab's width along the top. */
	inline constexpr float TabWidth = 132.f;

	/** "90°": an angle as the field of view slider shows it. */
	inline FString DegreesText(float Degrees)
	{
		return FString::Printf(TEXT("%d°"), FMath::RoundToInt32(Degrees));
	}

	/** "1.25x": a look sensitivity as its slider shows it (two places, for its 0.05 steps). */
	inline FString LookSensitivityText(float Sensitivity)
	{
		return FString::Printf(TEXT("%.2fx"), Sensitivity);
	}

	/** "85%": a 0-1 share as a slider shows it (the camera shake). */
	inline FString PercentText(float Share)
	{
		return FString::Printf(TEXT("%d%%"), FMath::RoundToInt32(Share * 100.f));
	}
}
