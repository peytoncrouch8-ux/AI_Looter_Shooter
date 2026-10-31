#pragma once

#include "CoreMinimal.h"
#include "UI/Style/LooterUIStyle.h"

/**
 * The pause menu's and the settings page's small icons (E2 "Big Dial"), drawn in code from the mockup's 24 px line icons
 * (Docs/Archive/Work/2026-10-30-PauseMenu/mockups/common.js ICON, e.js): white on a 24 x 24 view box, so a brush's tint gives them
 * their colour (LooterUI::PaintedIconBrush(Name, Icon, PixelsPerUnit, Size, Tint)). Never texture assets (CLAUDE.md).
 * The hero card's soul flask and grenade rows use their Inked icons instead (LooterUI::FindInkedIcon "SoulFlask",
 * "GraveSaltGrenade").
 */
namespace PauseMenuIcons
{
	/**
	 * The icon of Id: the options "Resume" (a play mark), "Settings" (a gear), "SaveQuit" (a door with an arrow out), "Quit"
	 * (a power mark); the settings tabs "Graphics" (a monitor), "Audio" (a speaker), "Interface" (stacked layers), "Controls"
	 * (a keyboard); the hero card's rows "Health" (a diamond with a cross), "Experience" (a star); and "Mission" (a star in a
	 * ring, the mission card's seal). An unknown id gives an empty picture.
	 */
	LooterUI::FPaintedIcon Get(FName Id);

	/** The ids Get draws, in the order above (for tests and for brush names). */
	TConstArrayView<FName> Ids();
}
