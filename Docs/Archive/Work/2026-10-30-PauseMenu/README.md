# Pause menu (ESC): four full-screen concepts, 2026-10-30

The user's ask: replace today's pop-up plate with a full-screen menu, the options in a column on the left, a callout for
the lit option, and info panels. Their reference is a Borderlands-3-style pause screen; we keep its structure in our own
look. Nothing is built yet: this is only the design pick.
- Options: the four real ones only: BACK TO GAME, SETTINGS (tabs Graphics/Audio/Interface/Controls), SAVE & QUIT (to the
  main menu, the session saves), QUIT GAME (saves, to the desktop; the danger red `LooterUI::Color::Worse` #FF7A6B, the
  stamp red of today's Save & Quit).
- The inventory's tabs (Inventory, Missions, Map, Bestiary, Embers) stay on Tab only.
- Single player: no news, party or friends panels.

Pictures are in `mockups/`. Each concept has `<X>_hub.png` and `<X>_settings.png` (the column stays; the Graphics tab's
rows on the right). `sheet.png` is the four hubs, 2x2.
Sources: `<X>_hub.html` / `<X>_settings.html` (a one-line shell: MODE + `<x>.js`), `common.js` (data, shapes, icons),
`common.css` (fonts, palette, the dimmed world). Render with `node render.js [files]` (Node at
%LOCALAPPDATA%\Programs\node, Playwright from the repo's node_modules); no arguments renders everything plus the sheet.
The backdrop is `backdrop.png`, zoomed 1.22x, blurred and dimmed so the HUD's edge pieces drop off screen.

## The concepts

- **A · Salt Rail** (closest to the reference). Slim cut-corner glass plates on a rail, salt corner brackets and hairline
  edges framing the screen, today's pause dial by GAME PAUSED. The lit plate is gold (4 px line, gold wash, rivets, a gold
  chevron), with a tag-shaped callout pointing at it. Panels: hero and mission top-right, the session card bottom-left.
  Settings opens as a glass panel joined to the lit plate by a gold lead.
- **B · Open Range** (no plates). The HUD's floating outlined type on a dark sweep from the left edge, PAUSED in hollow salt
  type, a ruler down the left edge with a gold pointer. The lit word is bigger and gold on a slanted gold bar; the callout
  is a floating sentence on a hairline with the level-up's gold arrowhead. Hero and mission float top-right like the HUD
  tracker; the session is one floating line bottom-left. Settings is floating type too: slanted gold bars mark the picks.
- **C · Tag Rack** (the gun card). Each option is the gun card's tag laid on its side (notch with a gem holding the
  option's icon, cut shoulders, a point), hung from a rail by pegs; the lit tag slides out gold. The callout is a small tag
  card. The hero is a tag card read like the gun card: level gem in the notch, banded stat rows, the ornament. Mission and
  session are tags too. Settings opens as a big tag card whose shoulders point back at the Settings tag.
- **D · Spur Rowel** (own idea; the Spur Rowel logo mark made the menu). A big rowel at the left edge with today's pause
  dial as its hub. Its slim spikes are 30 degrees apart, and the four options sit one spike each on a range arc. Moving the
  selection turns the wheel one click; the gold spike points at the lit word in today's reticle brackets. The callout,
  info and settings sit on "Ledger & Brass" plates (double salt trim, rivets, ruled ledger rows, the gold flag on the
  hovered row).

The hubs show BACK TO GAME lit (A, C) or SETTINGS lit (B, D). Key hints: ENTER SELECT, ESC RESUME (hub); ENTER SELECT,
PGUP / PGDN TABS, ESC BACK (settings). Colours are only Slate & Salt, `LooterUI::Color`: slate glass, salt lines, soul gold
= lit, flavour gold, text/labels/icons, heal green only for the soul flask, and the red for Quit.

## What the info panels show, and where it comes from (all exists today)

- **Area name** "RANSOM'S REST": `USessionSubsystem::AreaName(Map)` (`UAreaDefinition::DisplayName`).
- **Tracked mission** "SEVEN DAYS", step 3 / 5, "Read the headboard beside yours.", plus its optional step:
  `UMissionSubsystem::GetTrackedMission()`, the same data as the HUD tracker (`UI/HUD/HudMissionTracker*`).
- **Hero** Ellis Ransom: the session's hero (`ULooterSessionSave::GetHeroId`, `SessionSubsystemHero.cpp`). Portrait is
  `Art/Icons/HudPortrait.svg`, the HUD's.
- **Level and XP**: `UPlayerProgressionSubsystem::GetLevel()`, `GetXP()` / `GetXPToNextLevel()`, shown "1,208 / 2,647 XP"
  as the HUD's player frame does (`HudPlayerFrameWidgetXP.cpp`).
- **Health**: `UHealthComponent::GetHealth()` / `GetMaxHealth()`.
- **Soul flask**: `UPlayerFlaskComponent::GetCharges()`, 3 max (`FFlaskRules`).
- **Grenades**: `UPlayerThrowComponent::GetGrenades()` / `GetMaxGrenades()` (3).
- **Session**: "Session 1 of 3" from `USessionSubsystem::GetActiveSession()`, `MaxSessions`. Time played from
  `GetPlayedSecondsNow(World)` via `FormatPlayTime` ("3 h 05 min"). Last saved from `GetSummary(Index).Saved` via
  `FormatSavedTime` ("Today 14:32"); the session saves every minute. The serial "SER. RV-000<session>-<level>" is today's
  pause menu's.
- **Settings rows and help**: `SettingsMenuPages.cpp` (Quality LOW-EPIC, Motion blur ON/OFF, Field of view 70-110) and the
  help text from `SettingsMenuHelp.cpp` (Quality and its Medium preset line).

Mockup numbers (level 2, 84 / 100, 1,208 XP, 3 flask charges, 2 grenades, 3 h 05 min) are examples.

## Round 2: E, the mix of C and D (the user's pick, 2026-10-30)

The user: "a mix of C and D, the spur rowel is a good idea, but the left dial should look like our main menu screen". Then:
"when clicking on an option in the esc menu, it should open up a page instead of a popup on the right hand side".
- The dial is the main menu's (`mockups/main_menu.png`), drawn the way the game draws it:
  - the ring and its ticks are `LooterReticle::Halo`: ticks every 10 degrees outside the ring, the four at the cross long,
    gap/length/extra `max(4, .05R)` / `max(6, .07R)` / `max(7, .09R)`, ring 3 px from R 80;
  - inside it is `MainMenuArt::LogoMark` scaled R/34: the ten-point Spur Rowel in salt, the hub's ring cut out, the gold
    hub with its axle hole;
  - the range arc is `ArcScale` (ticks every 10.7 px pointing in, every fifth long);
  - the lit row has the gold run on the arc and the `SmallReticle`, as on the main menu;
  - every line has the kit's dark edge.
- The rowel's points are 36 degrees apart. It turns one click per option so a point, painted gold, aims at the lit row,
  with the halo's gold arc inside the ring beside it and a gold lead out to the arc.
- Options: C's sideways tags. Each hangs off its row's dash on the arc, with the number (01-04) past the arc as on the main
  menu. The lit tag slides out gold, with C's callout tag. C's hero card, tracked-mission tag and session tag are kept.
- **E1 · Tags on the Arc** (`E1_hub.png`): the dial whole at the left (R 150). The options sit on the arc at -54, -18,
  18 and 54 degrees, which are exactly the rowel's points, so the tags follow the curve.
- **E2 · Big Dial, Straight Tags** (`E2_hub.png`): the dial big (R 232) and cut by the screen's left edge, with a faint
  inner scale. The tags are stacked straight; the arc's dash and number feed each tag by a peg line.
- **Settings is a full page** (`E1_settings.png`, `E2_settings.png`). The column and the cards go away.
  - Header: E1 is the main menu's lockup (the small dial, SETTINGS over the ruler, "PAUSED / SETTINGS" in TEROPA's place).
    E2 keeps a big dial cut by the corner.
  - Tabs: E1 has C's tags in a row, each gem holding the tab's icon, the lit one gold and dropped. E2's tabs hang as small
    gun-card pennants off a ruler across the page, the lit one on a gold tick.
  - Below: the three Graphics rows large and banded (LOW-EPIC chips, ON/OFF, a ruler slider 70-110), and a help card on the
    page's right in the gun card's shape.
  - Key hints: ENTER SELECT, PGUP / PGDN TABS, ESC BACK (to the hub).
- `sheet2.png`: the two hubs side by side. Sources: `e.js` (VARIANT E1/E2, MODE hub/settings) and the `E*_*.html` shells.

## Round 3: E without the sun (E1b, E2b)

The user: "The sun symbol needs to go and the dial to fit the removal of the symbol." Both variations are redone (VARIANT
`E1b` / `E2b` in `e.js`, `needleCentre`); the old E files are kept. Everything except the dial's centre is unchanged.
- What stays: the main menu's salt ring with its ticks and long cross ticks.
- The open centre now has:
  - a fine inner ring at 0.64 R with ticks pointing in (every 5 degrees, longer at the cross);
  - the halo's gold arc beside the aim;
  - a slim needle on a hollow salt pivot with a gold dot: gold to just short of the ring, a short salt tail.
- One click per option: the needle aims at the lit option's tick, and the gold lead carries on to the arc.
- On the Settings page header the small dial's needle points at the title.
- Pictures: `E1b_hub.png`, `E1b_settings.png`, `E2b_hub.png`, `E2b_settings.png`; `sheet3.png` has the two hubs.
