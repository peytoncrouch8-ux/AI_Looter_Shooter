#pragma once

#include "CoreMinimal.h"
#include "Settings/AudioSettingsSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "UObject/WeakObjectPtrTemplates.h"

class UWidget;

/**
 * The settings menu's descriptions (the user, 2026-10-27: "as we hover over the buttons in the settings, it should display
 * what these settings do specifically"): what each row really does, in a sentence or two of plain words, written from the
 * code that applies it (the subsystems in Settings/, the HUD widgets that read them), and for graphics what it costs. Plain
 * text, no widgets, so the tests can check every row has its words. SettingsMenuTabs.cpp shows them in the panel beside the
 * page while a row is pointed at or has the keyboard or controller focus.
 */
namespace SettingsMenu
{
	/** The settings menu's pages, the tabs along its top, in order (LB / RB or Page Up / Down turn them). */
	enum class ESettingsPage : uint8
	{
		Graphics,
		Audio,
		Interface,
		Controls,
	};
	inline constexpr int32 NumPages = 4;

	/** Every row and button the menu describes, apart from the key list's (KeyHelp describes those by their binding). */
	enum class ESettingRow : uint8
	{
		Quality,
		MotionBlur,
		FieldOfView,
		MasterVolume,
		EffectsVolume,
		InterfaceVolume,
		MusicVolume,
		UITransparency,
		Minimap,
		MinimapSize,
		MinimapZoom,
		FrameRate,
		CrosshairColour,
		ControlHints,
		Subtitles,
		LookSensitivity,
		CameraShake,
		ControllerRumble,
		ResetAllKeys,
		Resume,
		SaveQuit,
		Back,
		Count,
	};

	/** What the panel shows: the row's name, what it does, and a quieter line under it (its range and default). */
	struct FSettingHelp
	{
		FString Title;
		FString Body;
		FString Detail;
	};

	/** A described widget (a row, or a button outside the rows) and its words, as the menu is built. */
	struct FHelpEntry
	{
		TWeakObjectPtr<UWidget> Target;
		FSettingHelp Help;
		/** The page it's on; INDEX_NONE for what every page shows (the tabs, Resume, the footer). */
		int32 Page = INDEX_NONE;
		/** The quality presets: the detail names the preset pointed at, or the one chosen. */
		bool bQuality = false;
		/** One of the key list's rows, which RebuildControls makes again. */
		bool bKeyList = false;
	};

	/** "Graphics", "Audio", "Interface", "Controls": the tab's words. */
	FString PageName(int32 Page);

	/** A page's own words, shown when it's turned to and nothing on it is pointed at yet. */
	FSettingHelp PageHelp(int32 Page);

	FSettingHelp RowHelp(ESettingRow Row);

	/** What a quality preset turns on and what it costs, measured on the reference card (Docs/Performance.md). */
	FString QualityPresetHelp(EGraphicsQuality Quality);

	/** The volume slider's row. */
	ESettingRow VolumeRow(EAudioVolume Which);

	/** A key binding's row (UKeyBindingSubsystem's Id and name), with its default key under it. */
	FSettingHelp KeyHelp(FName Id, const FString& DisplayName, const FString& DefaultKey);

	/** A key binding's Hold / Toggle switch (sprint, crouch, aim). */
	FSettingHelp KeyModeHelp(const FString& DisplayName);

	/** The binding has words of its own in KeyHelp (every binding in the list should: the tests check). */
	bool HasKeyWords(FName Id);
}
