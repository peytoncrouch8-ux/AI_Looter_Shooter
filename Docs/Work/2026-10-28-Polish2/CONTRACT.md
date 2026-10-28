# 2026-10-28 afternoon polish: contract (Main writes; agents follow)

The user (at work, no playtest): "Unpaid balance check, Gun maker sounds, Packaged-game fixes, Small cleanups".
Rules for every agent: CLAUDE.md. Agents never build, never commit, never close/open the editor; edit only their files
(Bash heredoc/sed or short scripts; the engine's python is at
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe" -I). Plain `python` is missing.
Main builds, runs the tests, commits and pushes. Report back: files changed, what to build/test, cvars, open doubts.

## Lanes (disjoint files)
1. UNPAID (editor + PIE owner): `Creatures/UnpaidFightTuning.h/.cpp` defaults only (cvars `Looter.Unpaid.*`), notes for
   `Docs/Pipeline/Enemies.md` (sent to Main, Main writes). Live-tunes cvars in PIE; no source edits but tuning defaults.
2. SOUNDS: `Weapons/GunshotSound.h/.cpp`, `Weapons/GunfeelSoundTuning.h`, `Tests/GunfeelSoundTests.cpp`, new
   `Weapons/MakerSoundFlavor.h/.cpp` if needed, `Art/Sounds/**` recipes if a new layer is needed (then Main renders/imports).
   No editor.
3. MARK (third person never shows a gun's maker mark): investigate; may edit `Tools/Unreal/build_world_materials.py`
   (MARK_CODE only), `Weapons/WeaponModelPattern.*`, third-person weapon attach/framing code it finds (name the files to
   Main before editing anything outside those). No editor.
4. CLEANUPS: `Tests/FirstPersonHandsGripTests.cpp`, the stale comment in `Player/FirstPersonHandsCup.cpp` (~line 77),
   `Missions/ObjectiveHighlight*` and a new small world marker for flat/decal mission items if needed
   (`UI/World/ObjectiveMarker*` new). No editor.
Main: PSO errors in the packaged game (Config/*.ini), builds, tests, docs.

## Shared names
- Maker ids as in `Affixes/WeaponMakers.h` (Roundhouse, Saint Maud, Windfall, Meridian; read the file for the exact FNames).
- Maker sound flavour (Main's suggestion the user picked): Roundhouse heavy, Saint Maud ringing, Windfall rattly, Meridian clean.
