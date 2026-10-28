# Polish job, 2026-10-28 (the user's picks: gallery fix, gun looks, melee hands, small cleanups)

Main (Opus 5.5) orchestrates and owns the editor lane. Agents never run `Tools\launch.ps1 -Build`, never commit, never
close the editor: SendMessage Main when code is ready, or before any editor/PIE use, and wait for the go-ahead.
The user is at work: no playtest from them. Reports to Main: short (files changed, what to build, what to check).

## Who owns which files (disjoint; touch nothing else without asking Main)

- **Main:** Unpaid gallery (`Creatures/CreatureBase*`, `CreatureBaseHunting.cpp`, `CreatureBasePursuit.cpp`,
  `HuntingGround.*`, `FightDirectorPlaces.cpp`, `CreatureTraitComponentPhase.cpp`, `CreaturePackComponent.cpp`, every
  `Unpaid*` file, `UnpaidRules.*`, their tests); rocket drops (`Loot/LootTable.cpp`, `Loot/LootOdds.cpp`, loot tests);
  the split of `UnpaidCreature.h/.cpp` and `UnpaidCreatureArms.cpp` (over ~500 lines). CODEMAP, Handoff, pipeline notes
  (agents send Main their lines; Main writes them).
- **Looks agent:** gun looks. `Weapons/WeaponModelPattern.cpp` (+ .h), M_Gun's build script under `Tools/Unreal/`,
  `Art/Models/Weapons/Bullpup.py` reticle/reflex only (`RETICLE`, the `Reflex` sight), any reticle material script.
  a) Saint Maud's clean finish reads flat (Defender Pump Shotgun, a pale grey block): darker base, stronger wear.
  b) Camo on red/cream reads as cow hide: blotches in two close tones of the gun's own base, never red on cream.
  c) The AR reflex dot is a tiny orange dot in a tall frame: about 2x, a thin ring round it; keep the element tint rule
     (GunReticle takes the element's colour).
  Subtle and worn-in stays the rule (the user: rarer = fancier). M_Gun is 800 instructions: don't grow it much.
- **Hands agent:** first-person hands. `Player/FirstPersonHandsActions.cpp`, `FirstPersonHandsCast.cpp`,
  `FirstPersonHandsGripShapes.cpp`, `FirstPersonHandsGrips.*`, `Weapons/Melee*` tool hold data, their tests.
  a) Crane's Ferry Gaff (Tripo): on the thrust the glove sits beside/below the shaft's butt instead of closing round it
     (`Saved/Screenshots/Tools/pt_mel_Crane_d.png`; bRoundHandle already set).
  b) Odessa's Razor Fan: the swing's first frames show her glove as a large dark blob near the lens (`pt_mel_Odessa_a.png`).
  c) The shotgun's trigger finger can't reach its trigger (below the frame at the hip; Docs/Archive/Handoff_history.md).
  Never touch `FirstPersonHandsFraming.cpp` (Main changed the sniper there today) or the cup files.

## Rules that bind everyone

CLAUDE.md (C++ first, one class per file, ~500 lines, comments say why, LogLooter, tabs). Tests: only the new or changed
work's, `Tools\runtests.ps1 -Filter`, once, with Main's go-ahead. No Python capture batches (they crash the editor); a
single `Tools\grab.ps1` grab is fine. `Tools\input.ps1`: never Ctrl+key or Delete.
