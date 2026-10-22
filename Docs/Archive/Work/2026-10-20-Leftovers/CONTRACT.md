# Leftovers round (2026-10-20): contract

The user picked "Fix the leftovers" after the third-person gun pass (32913628). Three agents, disjoint files, Main
(the orchestrator) builds, tests, commits and writes the docs. Read CLAUDE.md first; the user's rules there bind you.

## Who owns what

| Agent | Job | Files (only these) | Editor |
|---|---|---|---|
| SKIN | the red/white blotches on the heroes' bare skin | `Tools/Unreal/build_world_materials.py` (M_CharacterSkin only), `/Game/Art/Materials/**` skin assets (M_CharacterSkin, SP_Skin or a new SP_HeroSkin, MI_*Skin incl. first-person arms' skin MIs), `GAME_MATERIALS` skin lines in `Art/Models/Characters/*.py`, `Tools/Blender/odessa_looks.py` skin line, `Tools/Blender/fp_arms_heroes.py` skin params, scratch in `Saved/Leftovers/Skin/` | owns it NOW until it reports |
| HANDS | the third-person left hand grips the foregrip (Ellis and every hero) | `Player/Animation/LooterStancePoseWeapon.cpp`, `LooterStancePoseDetail.h`, `LooterStancePose.cpp`, a new `LooterStancePoseGrip.cpp` if wanted, `LooterStanceInput.h`, `LooterCharacterAnimInstance.cpp/.h`, `Tests/StanceGripTests.cpp` (new) | none until Main says the build lane is free; then owns it |
| CAMERA | the third-person camera fits each hero's size | `Embers/HeroDefinition.h`, `Player/Animation/BodyFigureComponent.h/.cpp`, `Player/PlayerViewCamera.cpp`, `Player/PlayerViewComponent.h/.cpp`, `Tools/Unreal/create_ember_assets.py`, `Tests/HeroCameraTests.cpp` (new) | none until Main says |

Never edit another agent's files. CODEMAP.md, Docs/*.md: don't edit; put the lines you'd add (CODEMAP line, Pipeline
note, bug line) at the end of your report and Main writes them. No commits. No `git checkout`/`reset`/`stash`.

## Shared names

- `UHeroDefinition::ThirdPersonCamera` (FVector, cm on the full-size body, zero for Ellis): X = extra boom length
  (+ farther back), Y = extra to the right, Z = extra up; added to `UPlayerViewComponent::ShoulderOffset` / `ArmLength`
  in the over-the-shoulder view only (the front view, first person untouched); scaled like the rest by the actor scale.
  `create_ember_assets.py` `THIRD_PERSON_CAMERA` dict beside `THIRD_PERSON_HOLD`, written by `set_editor_property(
  'third_person_camera', ...)`. `UBodyFigureComponent::GetCameraOffset()` beside `GetHoldOffset()` (the worn hero's,
  zero when none/Ellis), read by the view each frame (a Looter.Hero swap mid-play changes it, eased, not a jump).
- HANDS: the left hand's grip is a pose of the hand and finger bones (`hand_l`, `thumb_0[1-3]_l`, `index_0[1-3]_l`,
  `middle_..`, `ring_..`, `pinky_..`; the mannequin's names) applied after `ApplyLeftHandOnForegrip`'s arm solve, by the
  same blend weight (lets go in a reload and a one-handed carry). Per hold kind: a rifle's handguard/pump (palm under
  and to the left, fingers wrapping up its far side, thumb along the near side), a six-gun (`HoldReach > 0`: the support
  hand cups the shooting hand's fingers). The heroes' fingers follow the mannequin's through their retargeters.

## Build lane

1. SKIN works in the editor now (material edits, no C++ build). It must not close the editor or build.
2. HANDS and CAMERA write code now without building or touching the editor. Compile-clean by reading: every `.cpp`
   includes what it uses (no unity build), UE naming, tabs, comments say why, files under ~500 lines.
3. When SKIN reports, Main closes the editor, builds once with HANDS + CAMERA, fixes compile errors, runs tests, then
   hands the editor to HANDS for the visual pass (`Tools\heroguns.ps1 -Setup -Tag grip`, one sheet per hero), then to
   CAMERA for its tuning (`Looter.ThirdPerson.ShoulderRight` / `.ShoulderUp` / `.ArmLength` cvars live in PIE, then
   the data; they override the base, so keep the hero offset added on top of an override too).
4. After a long Python capture run, restart the editor before other work (it crashed twice ~30 s after one, 2026-10-20).

## Rules that bite

- Never send Ctrl+key or Delete through `Tools\input.ps1`; keep its foreground guard. Stop PIE before closing the editor.
- A modal editor dialog blocks every MCP call (`Tools\editorwindows.ps1`).
- Performance: 8.3 ms at 1080p Medium on the reference PC; Nanite and Lumen are off on Medium: the skin must look right
  there (`Looter.Quality Medium`), and on High/Epic.
- Two failures at the same thing: stop and report, don't push on.
