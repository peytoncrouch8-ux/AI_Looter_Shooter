# UI round 3 build (2026-10-29)

The user's pick of the round 3 concepts (Docs/Work/2026-10-29-Morning/ui3/c1..c6.png, scripts make_a.js c1-c3, make_b.js c4-c6):
"Mix 1 and 3 settings menu, lets use 3's main menu but change the logo in the top left and use 5's level up ui"
(first said 4's level-up, corrected to 5 at once). Colours: Slate & Salt only, from LooterUI::Color (SaltLine = lines
#d4eef4, SoulGold/Accent = gold selected, SlateLabel labels, SlateText text, Danger = Save & Quit red, SlateGlassTop/Foot
glass). Chakra Petch. Mockup CSS px = Slate font sizes as they are. Every painted background calls LooterUI::MarkBackground.
Vector art through PaintedIconBrush / IconBrush (no new textures). Hollow keys: ULooterHollowKey (UI/Cards/LooterHollowKey).

## Rules for every lane
- Agents never build, commit, open/close the editor, run tests, or edit CODEMAP.md / Docs. Main builds and verifies.
- Only your own files (below). Read anything. Keep every public name other code or tests use (grep Tests/ before changing
  a signature); if a test's fact changes with the look, update that test and say so in your report.
- Behaviour (keys, focus, navigation, sounds already wired, actions) stays as it is; this is a look + motion job.
- Files under ~500 lines; split a .cpp by topic if needed (new files: list them in the report for CODEMAP).
- Comments say why in plain words. Tabs. No unity build: include what you use.
- Report: files changed/added, what you could not do, any test you changed, any new sound cue you want (Main adds sounds).

## Shared art: UI/Style/LooterReticleArt.h/.cpp (lane M owns it; lane P uses it, never edits it)
namespace LooterReticle, painted pictures for PaintedIconBrush (view-box px, y down), salt line on a dark edge like every kit line:
- `LooterUI::FPaintedIcon Halo(float Radius, float TickStep, int32 LongEvery, bool bCross, float GoldFrom, float GoldTo)`:
  ring + ticks outside it (long every LongEvery), optional inner cross hairs, a gold arc GoldFrom..GoldTo degrees (equal = none).
- `LooterUI::FPaintedIcon Ruler(float Length, float Step, int32 LongEvery, bool bDots)`: a horizontal salt ruler (ticks
  down from the line, long ones every LongEvery; bDots = small dots between, as under c3's GAME PAUSED).
- `LooterUI::FPaintedIcon Bracket(float Height, bool bRight)`: c3's reticle bracket half "(" with a short horizontal tick
  through its middle; bRight mirrors it.
- `LooterUI::FPaintedIcon SmallReticle(float Radius)`: circle, four ticks, centre dot (c3's Resume mark / main menu pick mark).
- `LooterUI::FPaintedIcon ArcScale(float Radius, float FromDeg, float ToDeg, float TickStep)`: a part arc with ticks (c3's
  main menu range arc along the list, the pause panel's lower-left corner arc).
Lane P codes against these names before they exist; Main fixes small mismatches at build.

## Lane L: level-up popup = concept 5 (c5.png bottom right). Files: UI/HUD/HudLevelUpBannerWidget.h/.cpp (+ split if needed)
- No glass. "L E V E L   U P" wide-spaced (c5: ~30 px, letter spacing ~0.5 em) in SlateText with the floating outline;
  a short gold bar (SoulGold/Accent, ~80x4) under it; the big new level number (~150-170 px bold, SlateText, outline) in
  the middle; a salt hairline each side of the number ending in a small gold arrowhead pointing at it (c5); the reward
  line "MAX HEALTH +8%" (existing text, ~26 px spaced, SlateText) under it.
- Motion (c5's note): the two hairlines wipe out from the middle; the number counts old -> new (Level-1 -> Level) with one
  soft flash (a GlowBrush behind the number, tinted gold, up then down); the gold bar draws last (scale X 0->1); holds
  ~2 s, then fades. Keep: Show(NewLevel) restarts, ULevelUpBurstSubsystem still plays, the UI.LevelUp cue (keep where it
  plays; time it to the count if it is played here), the Width/Height box (change values if needed; check who reads them:
  PlayerHUDWidgetLayout, LootFanfarePopLabel), the loot fanfare pop keeps clear of it. Costs nothing hidden.

## Lane M: main menu = concept 3 (c3.png top right). Files: UI/Menus/MainMenuWidget.h/.cpp, MainMenuArt.h/.cpp,
## MainMenuMotion.cpp, UI/Style/LooterReticleArt.h/.cpp (new). Not MainMenuSessions/MainMenuNav/hero select.
- Title lockup: a halo ring (Halo(): ticks, cross gaps) at the far left holding the LOGO MARK, then REVENANT (GameTitle,
  big spaced, SlateText), a ruler under it (Ruler(), salt), TEROPA small spaced under the ruler.
- The logo mark itself is being changed (the user: "change the logo in the top left"; Main is getting the user's pick).
  Put the mark in one function `MainMenuArt::LogoMark()` returning FPaintedIcon (for now the current TitleMark picture,
  in salt/gold instead of orange) so the pick drops in with one change. The old orange ember glow behind it goes.
- The list: words right-aligned to a column (c3), a vertical range arc (ArcScale) to the right of the words with the row
  numbers 01-04 on it; the lit word is gold (SoulGold) with SmallReticle() snapped beside it on the arc, which then turns
  slowly (8 s a turn); MULTIPLAYER dim with a SOON tag (hollow salt outline). The focus look moves the reticle with a
  quick ease (~0.09 s) and plays the existing hover cue. ENTER SELECT hint as a hollow key bottom-left (restyle).
- Arrival (c3): the halo spins up 90 deg (0.25 s) and its ticks light clockwise, then the words fade in. Keep
  BeginReveal/HoldReveal, UI.Menu.Reveal, the page fades to the session picker / settings, ColumnWidth (test) and the
  dark shade on the left. Keep Looter.UI.MainMenu.Look facts true (bare buttons, UI.Menu.Select, title text).

## Lane P: pause / settings menu = mix of concept 1 (Ledger & Brass) and 3 (Halo Reticle). Files: UI/Menus/SettingsMenu*.cpp/.h
## (Widget, Tabs, Rows, Pages, Audio, Input, Preview, Parts, Nav; Help only if a word changes). Not LooterUIStyle, not MainMenu*.
Main's mix (c1.png + c3.png left halves):
- Panel: c1's plate (slate glass, salt outline with an inner second line = the double trim, small rivets in the four
  corners, chamfered corners like c3), with c3's halo dial (Halo(), gold arcs, the pause bars inside; opened from the
  main menu: a plain cross in it) breaking out over the top-left corner.
- Header: GAME PAUSED (c3 size, spaced, SlateText); c3's dotted Ruler under it; c1's stamped plate right of the title:
  "NO. 01 · RANSOM'S REST" style = the area's name in a thin gold-outlined box (from the main menu: title "SETTINGS",
  no plate).
- RESUME: c3's reticle: Bracket() left and right, SmallReticle() at the left inside, word centred gold, a soft gold
  wash between the brackets. From the main menu this row is BACK (whatever it is today).
- Tabs: c3's tab strip (PGUP / PGDN hollow keys at the ends, words spaced, the lit tab gold with a gold underline) over a
  Ruler(); a small gold triangle under the lit tab slides to it (0.09 s).
- Rows: c1's ledger: every row on a ruled salt hairline (low alpha), a ruled margin line with the row number 01, 02, 03
  in the margin (gold on the hovered row); c1's gold ribbon (swallowtail) sticking out left of the panel at the hovered
  row, sliding to it in 0.09 s. Choice buttons as c3 (hollow salt outlines, the chosen one gold outline + gold word);
  sliders as c3 (ruler track with end labels, ring knob).
- Description column: c3's (right side, a short vertical ruler line at its left, gold spaced title, text).
- SAVE & QUIT: c1's rubber stamp (Danger red double outline, red spaced word, tilted about -3 deg, light red wash),
  bottom right; a serial small spaced label bottom-left: "SER. RV-000<session>-<level, 2 digits>".
- Keys under the panel: hollow keys ENTER SELECT, PGUP / PGDN TABS, ESC RESUME (restyle what exists).
- Motion: c1's open (the plate drops 16 px and settles, ~0.18 s) + c3's halo spin up 90 deg (0.25 s); ribbon/triangle
  slides as above; the small reticle turns slowly (8 s a turn) on the lit row. Sounds stay as wired.

## Lane D: logo choices (mockup only). Files: Docs/Work/2026-10-29-UI3Build/logos/*
Four to six new marks for the top-left of concept 3's main menu, each drawn inside c3's halo ring at the real size, shown
on one 1920x1080 sheet, our own art, nothing like an existing game's logo.

## Main: builds, Looter.UI tests, grabs, compares against the concepts, commits.
