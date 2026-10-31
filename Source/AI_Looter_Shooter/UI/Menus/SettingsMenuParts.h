#pragma once

#include "CoreMinimal.h"
#include "Settings/GraphicsSettingsSubsystem.h"

/** What the settings menu's .cpp files share: its buttons' actions, the key list's columns and value labels. */
namespace SettingsMenu
{
	/** Back: the ESC BACK key hint, clicked. */
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

	// --- The full page (E2 "Big Dial", the user's pick 2026-10-30: mockups/E2b_settings.png at 1920 x 1080, e.js MODE
	// settings). Mockup px are Slate units as they are. The round 3 plate's sizes above stay for the pause menu's pieces
	// until it stops using them. In a namespace of their own, so their common names (TitleSize, HintGap) never meet a file
	// that uses SettingsMenu for its actions. ---
	namespace Layout
	{
		/** The room kept from the screen's left and right edges. */
		inline constexpr float PageMargin = 96.f;
		/** The dial in the top-left corner, cut by it (e.js: R 104 at 30, 150, its needle pointing right at the title). */
		inline constexpr float DialRadius = 104.f;
		inline constexpr float DialCentreX = 30.f;
		inline constexpr float DialCentreY = 150.f;
		/** SETTINGS over its ruler and the breadcrumb (e.js lockup at 220, 92: 58 px spaced .24em; the crumb 17 px spaced .5em). */
		inline constexpr float TitleLeft = 220.f;
		/** The title block's top: a text block's box starts above its capitals, so this sits higher than the mockup's 92. */
		inline constexpr float TitleTop = 80.f;
		inline constexpr int32 TitleSize = 58;
		inline constexpr int32 TitleSpacing = 240;
		inline constexpr int32 CrumbSize = 17;
		inline constexpr int32 CrumbSpacing = 500;
		/** The tabs' strip: its top (the ruler's line 14 px down it), its height, and where the PGUP / PGDN caps sit in it. */
		inline constexpr float TabStripTop = 262.f;
		inline constexpr float TabStripHeight = 140.f;
		inline constexpr float TabKeyTop = 52.f;
		/** A tab's icon and word (e.js: a 24 px icon, 26 px bold spaced .12em, 12 apart, 20 px into the pennant). */
		inline constexpr float TabIconSize = 24.f;
		inline constexpr int32 TabWordSize = 26;
		inline constexpr int32 TabWordSpacing = 120;
		inline constexpr float TabLabelTop = 20.f;
		/** The rows' list: from 432 down to the key hints' band, the left two thirds of the page (the help card's room kept right). */
		inline constexpr float RowsTop = 432.f;
		inline constexpr float RowsBottomMargin = 116.f;
		/** A row's number and name (e.js: 26 px, 32 px bold), and a key list row's. */
		inline constexpr int32 RowNumberSize = 26;
		inline constexpr int32 RowNameSize = 32;
		inline constexpr int32 KeyNumberSize = 22;
		inline constexpr int32 KeyNameSize = 26;
		/** A key list heading ("MOVEMENT"): gold, spaced. */
		inline constexpr int32 SectionSize = 18;
		inline constexpr int32 SectionSpacing = 300;
		/** A switch's word (e.js: 24 px bold spaced .08em), a key's and Default's in the key list, and their columns. */
		inline constexpr int32 SwitchFontSize = 24;
		inline constexpr int32 SwitchSpacing = 80;
		inline constexpr int32 KeyFontSize = 20;
		inline constexpr int32 DefaultFontSize = 20;
		inline constexpr float KeyColumnWide = 300.f;
		/** The key list's hint over it (how to change a key). */
		inline constexpr int32 KeyHintSize = 20;
		inline constexpr float DefaultColumnWide = 150.f;
		/** A slider's value (e.js: 34 px bold, right-aligned, 22 px from the ruler). */
		inline constexpr int32 ValueSize = 34;
		inline constexpr float ValueWidth = 104.f;
		inline constexpr float ValueGap = 40.f;
		/** The help card (e.js: 40 px right of the rows, to the page's right margin, 8 px above the rows, three rows tall). */
		inline constexpr float HelpCardWidth = 528.f;
		inline constexpr float HelpCardHeight = 404.f;
		inline constexpr float HelpCardTop = 424.f;
		inline constexpr float HelpCardGap = 40.f;
		/** The card's words: 34 px in from its left, 30 from its right, 46 down from its top. */
		inline constexpr float HelpPadLeft = 34.f;
		inline constexpr float HelpPadRight = 30.f;
		inline constexpr float HelpPadTop = 46.f;
		/** The gem's middle below the card's top (e.js: a 48 px gem at -18) and the icon in it. */
		inline constexpr float HelpGemCentreY = 6.f;
		inline constexpr float HelpGemIconSize = 22.f;
		/** The key hints' middle, up from the screen's foot (e.js: 40 px caps from 986), their gaps, and the word's size. */
		inline constexpr float HintsFromBottom = 74.f;
		inline constexpr float HintGap = 34.f;
		inline constexpr float HintKeyGap = 12.f;
		inline constexpr int32 HintWordSize = 26;
		inline constexpr int32 HintWordSpacing = 80;
		/** The status line (what a change did), bottom-left across from the hints. */
		inline constexpr int32 StatusSize = 18;
		inline constexpr float StatusWidth = 900.f;
	}

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
