#pragma once

#include "CoreMinimal.h"
#include "Settings/GraphicsSettingsSubsystem.h"

/** What the settings menu's .cpp files share: its buttons' actions, the key list's columns and value labels. */
namespace SettingsMenu
{
	/** Back (and the corner's X). */
	inline const FName ActionClose(TEXT("Close"));
	/** A tab along the top; Index is its page (ESettingsPage, SettingsMenuHelp.h). */
	inline const FName ActionTab(TEXT("Tab"));
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
	inline constexpr float DefaultColumnWidth = 112.f;
	inline constexpr float QualityColumnWidth = 340.f;

	/** The quality presets, in the order of their buttons. */
	inline constexpr EGraphicsQuality Qualities[] = { EGraphicsQuality::Low, EGraphicsQuality::Medium, EGraphicsQuality::High, EGraphicsQuality::Epic };

	/** The field of view slider moves a whole degree at a time. */
	inline constexpr float FieldOfViewStep = 1.f;

	/**
	 * The plate's size: wide enough for a page's rows beside the description column (round 3's words are set as the
	 * mockup's px, a third bigger in game), and the same on every page; the keys sit under it.
	 */
	inline constexpr float PanelWidth = 1080.f;
	inline constexpr float PanelHeight = 900.f;
	/** The room between the plate's edge and its contents. */
	inline constexpr float PlatePadX = 44.f;
	inline constexpr float PlatePadTop = 32.f;
	inline constexpr float PlatePadBottom = 26.f;
	/** The description column beside the page, its short ruler included. */
	inline constexpr float HelpColumnWidth = 240.f;
	/** A tab's width along the top (they share the bar between the keys; this is the least). */
	inline constexpr float TabWidth = 150.f;

	// The ledger (c1): rows on ruled lines, their numbers in a margin ruled off by a gold double line.

	/** The margin's width, where the row numbers sit, and the gap between its rule and a row's words. */
	inline constexpr float LedgerMargin = 56.f;
	inline constexpr float LedgerGap = 16.f;
	/** A settings row's height, and a key list row's. */
	inline constexpr float LedgerRowHeight = 56.f;
	inline constexpr float KeyRowHeight = 46.f;

	// The halo (c3) on the plate's top-left corner, and the title beside it (plate space).

	inline constexpr float HaloRadius = 100.f;
	inline constexpr float HaloCentreX = 80.f;
	inline constexpr float HaloCentreY = 92.f;
	/** The title's (and Resume's) room left for the halo, from the column's left. */
	inline constexpr float TitleIndent = 186.f;
	inline constexpr float ResumeIndent = 214.f;
	inline constexpr float ResumeHeight = 66.f;
	/** A slider's ruler track. */
	inline constexpr float SliderWidth = 190.f;
	inline constexpr float SliderHeight = 46.f;

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
