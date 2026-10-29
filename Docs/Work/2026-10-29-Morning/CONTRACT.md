# 2026-10-29 morning job (the user's answers, 01:40)

The user: per-kind crosshairs YES (the mockup Docs/Work/2026-10-28-Core/crosshair/crosshairs.png); the Sonniss GDC 2026
bundle YES (7.5 GB, 5 zips, downloads.sonniss.com, to Saved/SoundPacks/Sonniss/2026; licence page saved beside it);
crosshair colour = "Pick in Settings" (white with a dark edge by default, a colour option under Settings > Interface);
after these: the open issues first, then UI round 2 (Slate & Salt on the other screens).

## Lanes (disjoint files; agents never build, commit, open the editor, or edit CODEMAP/Docs; Main does)
- X crosshair (Opus): new `UI/HUD/HudCrosshairWidget.h/.cpp` (UMG wrapper + SLeafWidget painting one shape per
  `EWeaponKind`, dark outline under a coloured line as the mockup's make.js), `UI/HUD/HudCrosshairRules.h/.cpp` (shapes as
  plain data, per-kind kick scale and time, colour list), `PlayerHUDWidgetCrosshair.cpp`, the crosshair block of
  `PlayerHUDWidgetLayout.cpp`, crosshair members of `PlayerHUDWidget.h`, `Settings/GraphicsSettingsSubsystem*` (a
  crosshair colour setting, saved like the others), the Interface page rows in `UI/Menus/SettingsMenuPages.cpp` +
  `SettingsMenuHelp.*`, new `Tests/HudCrosshairTests.cpp` (`Looter.UI.Crosshair.*`).
- I issues (Opus): LMG long-burst climb; Ranger Post crows (low, visible tell); tarn lockbox visibility; hits on the
  capsule get no zone. Files: weapons recoil/kick files, `World/FindSubsystemTells.cpp`, `World/FindGlints.*`,
  `Creatures/CreatureHitZones.*`, `Components/HealthComponent*`, build_area_finds*.py; NOT the HUD or settings files.
- S sounds (Main, after the download): gun excerpts from the 2026 bundle into `Art/Sounds/packs/Sonniss/` via make_pack.py,
  recipes, render, import.

## Sound pack outcome (02:00)
- GDC 2026 downloaded (6.9 GB zips, 14 GB unpacked, 347 files, ~3 per library): NO gunshots. Track lists (sonniss.com
  Google Sheets): guns are in 2016/2017 (have), 2018 (Outdoor Gun Acoustics, Battle Rifles & Pistols, Guns & Explosions,
  29.3 GB), 2021-2023 (Black Powder Guns, Assault Gun IKV 103, 38.7 GB), 2020 (Hand Guns, 48.2 GB).
- The user: "Neither" (no more downloads). Use the 2026 clicks/cloth only: aim-in/out cloth, sniper bolt latch thunk,
  SMG/LMG spring/mechanism rattle (lane S, Opus agent, Art/Sounds only; Main imports).
