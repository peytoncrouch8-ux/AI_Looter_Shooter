# Ellis round 6: the shared contract (orchestrator, 2026-10-15)

The user, after round 5's in-game pictures: "Keep polishing the model, the pictures you're showing me aren't looking good.
Cape clippings, hair looks like mud, can see through the head/hat, hat doesn't seat on his head properly, don't stop until
you get it right." Then: "Run multiple agents if you need to, we've spent a lot of time with little to no progress."

In-game evidence (Unreal, PIE, Medium-like lighting): `Saved/Screenshots/Tools/aud_heads.png` (head from 5 sides x 3
heights), `aud_bodies.png` (whole body 5 sides), and each `aud_head_<height>_<side>.png` / `aud_body_<side>.png`.
Note: in those, "above" rows are actually seen from below and "below" rows from above.

## Three agents, one model (Art/Models/Characters/Ellis.py), disjoint parts

| Agent | Owns (may edit) | Must not touch |
|---|---|---|
| HAT | `build_hat`, the scalp/head top under the hat (in `build_skin` or a new `ellis_head.py`), hat bone placement (`hat_bone`, `add_hat_bone`), `hat_contact`; hat parts' entries in `make_lows`/LOW tables; new module `Tools/Blender/ellis_hat.py` if wanted | hair, cape, collar geometry, coat |
| HAIR | `build_hair`, `lock_frames`, `lock_piece`, hair's `make_lows` handling (`HAIR_LOW`, `hair_under`); hair materials/looks; new module `Tools/Blender/ellis_hair.py`; new texture set `Art/Textures/EllisHair/` if cards; a new master builder function in `Tools/Unreal/build_world_materials.py` (M_CharacterHair) if needed (the orchestrator runs it in Unreal) | hat, scalp, cape, collar |
| CAPE | `build_cape`, `build_collar`, the cape/collar weights and swing-bone placement for `cape_*` chains, the drape for the cape/collar (cache entries) | hat, hair, head, duster body below the cape (except where the cape meets it) |

Rules for all three:
- Edit Ellis.py only inside your functions (Edit tool, small exact replacements; re-read before each edit: the others are
  editing other parts of the same file at the same time). Prefer putting new code in your own module and calling it.
- Blender: at most 2 Blender processes run at once on this PC (16 GB, the Unreal editor open): before starting one, check
  `Get-Process blender` and wait while 2 are running. Use quick runs (`--shape`, `--rig-check`, `--audit <tag>`
  with clay, `--no-sim` where the drape cache holds); NO full export and NO `--rebake` (the orchestrator runs the one
  export when all three are done). Run Blender through `Tools\artrun.ps1` (it waits while `Saved\ArtPause.flag` exists).
- No Unreal, no git, no C++. Triangle budget for the whole model stays at about 35k (now 34.9k): HAIR may take up to
  +2.5k, HAT up to +0.8k, CAPE up to +0.8k; pay from parts that don't show if you need more.
- The boundaries between your parts: HAT and HAIR meet at the hat band: the hair comes out from UNDER the band (the band's
  inner edge sits on the scalp/hair, no gap). HAIR must not go through the hat's crown; HAT keeps the crown's inside
  covered. CAPE and HAT meet at the collar/brim: the brim must clear the collar when the head turns 35 degrees and pitches
  20 (the existing `HEAD_TESTS`); if it can't, tell the orchestrator, don't move the hat.
- Renders: put yours in `Saved/Concepts/EllisRound6/<agent>/` (before and after, same angles). Look at every render
  yourself before reporting. Your bar is the user's: "the model has to be perfect", a big studio's third-person hero.
- Report briefly: what was wrong (the cause), what you changed, numbers, render paths, anything left.
