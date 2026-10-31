# Pause menu E2 "Big Dial" + full-page Settings (2026-10-31, Main's build contract)

## The user's asks (their words, kept)
- "Instead of a popup like ui, the ESC menu should encompass the entire screen and the options should be on the left hand
  side of the screen. Esc Menu should not have whats in the inventory tabs."
- Reference: a Borderlands-3-style pause screen, "almost something like this, but better, in our style and only with what
  we have in game, we might add more later."
- "When clicking on an option in the esc menu, it should open up a page instead of a popup on the right hand side."
- Picks over three rounds:
  - a mix of C (Tag Rack) and D (Spur Rowel), "the left dial should look like our main menu screen";
  - "The sun symbol needs to go and the dial to fit the removal of the symbol";
  - then E2 "Big Dial".

## The target pictures (match them; README.md in this folder has the round notes and every data getter)
- Hub: `mockups/E2b_hub.png`.
- Settings page: `mockups/E2b_settings.png`.
- The page code is `mockups/e.js` and `common.js`/`common.css`. Its shapes, sizes and colours are the spec: read the SVG
  paths and numbers there. CSS px = Slate units as they are (CLAUDE.md UI rules).

## The hub (full screen, over the paused game)
- **Backdrop:** the game dimmed and darkened toward the left edge (the mockup's vignette). No plate; the gameplay HUD hidden
  as today. Every painted background calls `LooterUI::MarkBackground`.
- **Title, top-left:** "GAME PAUSED", with the ruler and the area name under it (RANSOM'S REST, from today's code).
- **The dial,** big, cut by the screen's left edge, from `PauseMenuArt::Dial`:
  - the main menu's salt ring, ticks every 10° and long cross ticks;
  - a fine inner ring with inward ticks;
  - a slim gold needle on a hollow salt pivot with a gold dot, and a short salt tail;
  - a short gold arc inside the ring by the needle;
  - NO rowel or sun.
  - Its range arc (`LooterReticle::ArcScale`) carries ticks 01-04. Each tick feeds its option's tag by a peg line through the
    number, and the lit one gets the small gold reticle (`LooterReticle::SmallReticle`).
  - Choosing an option turns the needle to its tick with a short eased swing (about 0.18 s, a slight overshoot). That is
    the "one click per option".
- **The options:** C's tag buttons stacked straight, in this order:
  1. BACK TO GAME
  2. SETTINGS
  3. SAVE & QUIT (save, back to the main menu, as today)
  4. QUIT GAME (save, then quit to the desktop; red `LooterUI::Color::Worse`, as Save & Quit is today)
- **No** Inventory, Missions, Map, Bestiary or Embers rows: those stay on Tab and their own keys.
- **The lit tag:** gold (SoulGold edges, a gold wash), slides out a little to the right. Each tag has its icon in a diamond
  gem on its left.
- **The callout** beside the lit tag, C's tag-card callout pointing back at it: the option's name, a rule with a diamond, a
  line saying what it does, a gold italic flavor line. Use the mockup's words:
  - BACK TO GAME: "Close the menu and pick up where you stopped." / "The world waits while you're here."
  - SETTINGS: "Graphics, audio, interface and controls." / "Changes apply as you make them." Main must verify this flavor
    line against the settings code before the commit.
  - SAVE & QUIT: "Save and return to the main menu." / "Your session is saved first."
  - QUIT GAME: "Save and close the game." / "Your session is saved first."
- **The hero card, top-right:** the gun card's tag shape with the level gem on top.
  - Ellis's HUD portrait medallion (the portrait code the HUD uses), "HERO · LEVEL n" and the name.
  - Rows: Health cur / max; Experience cur / next; Soul flask (pips + n / 3); Grenades (pips + n / 3).
  - The XP bar at its foot.
- **The tracked-mission card** under it: TRACKED MISSION, the step pips with n / m, the mission's name, its current step
  and its optional step (as the HUD's tracker shows them). With no tracked mission, the card isn't shown.
- **The session card, bottom-left:** SESSION n OF 3, the serial, "Played 3 h 05 min" (`GetPlayedSecondsNow` +
  `FormatPlayTime`) and "Saved Today 14:32" (`GetSummary().Saved` + `FormatSavedTime`). Outside a session (`Looter.PlayLevel`)
  it reads "NO SESSION · not saved" or is hidden: HUB decides and says which.
- **Key hints, bottom-right:** ENTER SELECT, ESC RESUME, following the device last used as today (`MenuNav` prompts).
- **Driven as today:** keyboard, controller, mouse (hover marks, click picks). Up/down wrap, A/Enter picks, B/Esc/Start
  resume.
- **Motion:** the open motion fades the backdrop in, the dial swings up from below, the tags slide in one after another,
  and the cards fade in. Keep today's sounds for move, pick and back.

## The Settings page (full screen, replaces the hub while open; Esc returns to the hub with SETTINGS lit)
- **Header, top-left:**
  - the dial (`PauseMenuArt::Dial`, smaller, cut by the top-left corner, needle pointing right at the title);
  - "SETTINGS" big over a ruler;
  - the breadcrumb "PAUSED / SETTINGS" from the pause menu, "MAIN MENU / SETTINGS" from the main menu.
- **The page is ONE widget for both** (`USettingsMenuWidget`, both `ESettingsMenuMode`s): the main menu's Settings gets
  the same full-page look, with the main menu's backdrop behind it and no extra dimming needed.
- **Tabs:** a long ruler across the page with gun-card pennant tabs hanging off it: GRAPHICS, AUDIO, INTERFACE, CONTROLS.
  - Each tab has an icon; the lit one is gold, on a gold tick.
  - PGUP / PGDN keycaps sit at the ends; LB/RB on a controller, as today.
  - Mouse clicks a tab.
- **Rows:** big, banded rows across about two thirds of the page.
  - Number, name, then the control at the right: the switch buttons in cut-corner boxes (the lit one gold) and the slider
    with its ruler, gold diamond knob, end labels and value.
  - The pointed-at row is lit gold with a gold edge on its left.
  - Every page's rows (Graphics, Audio, Interface, Controls including the key list) take this look. A page longer than the
    screen scrolls as today.
- **The help card** on the right (the gun card's shape, its icon gem on top): "POINTED AT", the row's name, a rule, the help
  text (`SettingsMenuHelp`), and the gold italic cost line where there is one.
- **Kept from today:**
  - the field-of-view and minimap previews (the page ghosts while the FOV slider is held);
  - Esc letting go of a held preview first;
  - all navigation;
  - the settings saving as now.
- **Key hints, bottom-right:** ENTER SELECT, PGUP / PGDN TABS, ESC BACK.

## Shared API (HUB writes it; SETTINGS compiles against it as written here)
`UI/Menus/PauseMenuArt.h` (new):

```cpp
#pragma once
#include "CoreMinimal.h"
#include "UI/Style/LooterUIStyle.h"

/** The pause menu's and the settings page's pictures (E2 "Big Dial"), in LooterReticle's conventions: view-box px, y down,
 *  angles in degrees with 0 at three o'clock growing clockwise, Pad round every line, drawn with a dark edge under. */
namespace PauseMenuArt
{
	/** The dial without the rowel: salt ring of Radius (ticks every 10 deg, long at the four cross points), a fine inner ring
	 *  with inward ticks, a slim gold needle aimed at NeedleDeg on a hollow salt pivot with a gold dot, a short salt tail,
	 *  and a short gold arc inside the ring centred on NeedleDeg. Square view box 2 x DialExtent(Radius), centre in the middle. */
	LooterUI::FPaintedIcon Dial(float Radius, float NeedleDeg);
	float DialExtent(float Radius);

	/** How a tag or pennant reads. */
	enum class ELook : uint8 { Plain, Lit, Danger };

	/** C's option tag laid on its side, Width x Height: the gem notch at the left, cut shoulders, the point at the right. Fill
	 *  glass, edge salt (Plain), gold with a gold wash (Lit) or red (Danger). View box Width + 2 Pad by Height + 2 Pad. */
	LooterUI::FPaintedIcon Tag(float Width, float Height, ELook Look);

	/** A card in the gun card's shape, Width x Height: cut corners, a notch on top holding a diamond gem (bTopGem), and, with
	 *  bPointLeft, a point on its left edge at PointY (the callout pointing back at its tag). Glass fill, salt edge. */
	LooterUI::FPaintedIcon Card(float Width, float Height, bool bTopGem, bool bPointLeft, float PointY);

	/** A settings tab hanging from the ruler: the gun card's pennant, Width x Height, its diamond hook on top. */
	LooterUI::FPaintedIcon Pennant(float Width, float Height, ELook Look);

	/** A diamond gem of Size holding an icon (the tags' and cards' gems); bLit draws it gold. */
	LooterUI::FPaintedIcon Gem(float Size, bool bLit);
}
```

Paint them with `LooterUI::PaintedIconBrush`, giving every distinct picture its own brush name: LooterReticle explains why.
- **Icons** (resume, gear, save-exit, power; the tab icons; health, XP, flask, grenade) are new vector pictures in the
  kit's way (`IconBrush`/`PaintedIconBrush`, CLAUDE.md: never new texture assets). The flask and grenade reuse their
  Inked icons.
- **Ownership:** HUB owns these pictures. Put them in `UI/Menus/PauseMenuIcons.{h,cpp}`: `PauseMenuIcons::Get(FName Id)`
  returns a `LooterUI::FPaintedIcon`, with ids "Resume", "Settings", "SaveQuit", "Quit", "Graphics", "Audio", "Interface",
  "Controls", "Health", "Experience". SETTINGS uses the four tab ids.

## Who owns what (two agents never own one file)
- **HUB (Opus):**
  - new: `UI/Menus/PauseMenuArt.*`, `UI/Menus/PauseMenuIcons.*`, and any split pieces (`PauseMenuCards.cpp`, ...);
  - reworked: `UI/Menus/PauseMenuWidget.*`, `PauseMenuPieces.cpp`, `PauseMenuNav.cpp`, `PauseMenuMotion.cpp`;
  - `UI/HUD/LooterHUDPause.cpp` and `LooterHUD.h` if needed (the routing: the inventory rows go; QUIT GAME saves then quits
    to the desktop; Settings opens the page and Esc comes back to SETTINGS);
  - `Tests/PauseMenuTests.cpp`.
  - Anything the session or progression code must newly expose: write the exact edit in the README for Main.
- **SETTINGS (Opus):** `UI/Menus/SettingsMenu*.{h,cpp}` (the frame, tabs, rows, help column, nav prompts, motion, pages) and
  the settings tests (`Tests/Settings*Tests.cpp`, `Tests/MenuNavTests.cpp` as needed).
  - The main menu's call sites, if they need a change: write the edit in the README.
- **MAIN:** builds, `Tools\uishots.ps1` and PIE checks, hooks in other files, CODEMAP, docs, git.

## Rules for agents
- Holding tree `Saved/PauseCode/<AGENT>/Source/AI_Looter_Shooter/...`: copies of the files you change, plus your new
  files, with the originals' `git hash-object` in `HASHES.txt`.
- Never build, open Unreal or touch git.
- README in your tree: files, CODEMAP lines, hooks for Main, tests, risks, and how to photograph it (`Tools\uishots.ps1`
  states).
- Read CLAUDE.md (UI rules), Docs/Pipeline/Game.md ("The gun card", "UI round 3: menus and level-up"), and the existing
  widgets you change, before writing. Keep everything they do that isn't replaced: sounds, navigation, previews,
  transparency.
- One class per file, under ~500 lines a file (split by topic). Every .cpp includes what it uses. Comments say why in
  plain words. Colours only from `LooterUI::Color`.
