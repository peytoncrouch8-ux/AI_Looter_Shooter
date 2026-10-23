# FP-GRIP: first-person hands grip the guns like a trained soldier (2026-10-22)

The user, verbatim: "Look at the way the hands grip the gun in first person pov (especially on the AR). The fingers and
hand need to grip the gun better and like an actual military officer."
The user's style pick (via Main): "Modern tactical", "Recommend but not so much hand showing": high firing grip, support
hand forward, thumb over the handguard (C-clamp on rifles), but hands/arms compact on screen (Borderlands amount; the
user earlier: our hands showed too much arm). Keep the FP-HANDS framing (forearm on screen: rifle 20%, LMG 18, sniper 12,
shotgun/launcher 17, SMG 14 of the frame height; `Looter.FPHands.Play.OnScreen`) at or under today's.

## What's wrong now (grabs: Saved/Screenshots/Tools/muzzle_<Kind>_hip|aim.png 10-22 23:00, fp_<Kind>_*.png 10-22 17:00)

- **Rifle (Bullpup), hip:** support hand palm up under the handguard; its finger stubs poke up above the far top edge
  behind the front sight (not wrapped), the thumb stands up off the near side like a peg. The trigger hand is out of
  frame. ADS: support fingers a flat block on the handguard's right; reads OK-ish.
- **Shotgun:** support hand cups the pump from the lower left; fingers go up the near side, not round it.
- **Revolver (one-handed):** the trigger hand's fingers lie straight and flat across the grip's front, poking past its
  left side (karate chop), thumb up the back. On 10-20 (two-handed, before FP-HANDS) they were curled round the grip.
  Suspect: the curls aren't reaching the bones on this hold (check first in phase 2: `Looter.Hands.Grip.Debug 1`,
  `Looter.Hands.Contact 0` vs 1), or the +6 roll / forearm framing turns the hand off the grip.
- **SMG:** support hand a flat palm-up tray under the helix, fingers straight; trigger hand fingers flat under the gun.
- **Sniper:** support thumb a peg up the fore-end's near side; fingers hidden.
- **LMG:** support hand sunk inside the handguard (only the forearm shows).
- **Launcher:** support hand round the front grip reads fine; trigger hand barely in frame.

## Why (code)

- Curls are fixed per grip (`FHandFingers::Of(EHandGrip)`, FirstPersonHandsFingers.cpp): one table for every support
  hand, nothing knows how thick each handle is, so fingers float or sink. The approved Blender sheet
  (Tools/Blender/fp_arms_sheet.py) closed each finger till it touched the gun mesh; the game port dropped that.
- One curl axis per joint, no spread: the thumb can only fold toward the palm (the "peg"/"nub"), the trigger finger
  can't lie along the frame.
- The palm's distance to the handle is a fixed 3.6 cm (`GripOffPalm`) from the socket-based hold: on thick handles the
  palm sinks in (LMG), on thin ones it floats.
- Trigger finger always on the trigger (`FGunHands::TriggerCurl`), never indexed.

## The fix (code prepared in Saved/FPGripCode/, HASHES.txt = working copies it was made from; nothing committed)

1. `FHandleShape` (Types.h): a handle as a rounded box (Center, Axis, Across, HalfAcross, HalfThrough, Corner,
   HalfLength), signed `Distance`. `FHandContact` on every `FHandPlacement` (handle, Share, Seat, SeatGap,
   FingerRadius, TriggerPoint, TriggerShare); `Moved`/`Blend` carry it (a side without a handle fades it out).
2. `FHandFingers::Spread[5]`: first-joint swing about the palm's facing (finger index-ward; thumb toward the fingers'
   way). Rig: `JointLocal` applies it; per-finger spread axes, finger bases, tips cached at bind.
3. Rig (new FirstPersonHandsContact.cpp): `Seated` slides the palm along its facing onto the handle (gap 0.15, at most
   2.5 cm, fading out to 5 cm so a hand leaving for a reload isn't pulled back); after the arm solve, `ContactFingers`
   closes middle/ring/pinky (and the index on the support hand) till they touch (all joints together at 1/1.15/0.8,
   then each, 3 degree march + 6 halvings, start nearly straight, lift if inside), the thumb's two outer joints the same
   from the table's base swing; the trigger finger is searched onto `TriggerPoint` (grid knuckle -10..80, middle 0..100,
   spread -6/0/6, then fine), staying out of the grip. Cached per hand while the handle stays put in the hand's space
   (recomputed only when the hand moves on the gun: reload, bolt, pump). A finger touching nothing keeps the table curls.
4. Per-kind data (new FirstPersonHandsGripShapes.cpp, `AddContact`): grip and fore shapes, trigger face, thumbs, read
   off the default parts (numbers and sources in the comments). Rifle's support hand turned into a first-guess C-clamp
   in `MakeRifle` (palm on the near side, knuckles ahead tipped 15 down, thumb over the top swing 30, forearm out lower
   left, hold 3 cm forward); old values in the comment to restore.
5. Trigger discipline (new FirstPersonHandsComponentGrip.cpp): `FHandsGun::TriggerReady` 0-1, on the trigger while
   `WantsToFire()`, aim alpha > 0.25, or 0.6 s after a pull (on in 0.06 s, off in 0.2 s); else indexed straight along
   the frame (`IndexedCurl` 4/6/4, spread 6). No Weapons/ file touched (GF-RELOAD owns WeaponBase.h).
6. Tests (new Tests/FirstPersonHandsGripTests.cpp): `Looter.Player.Hands.Grip.Shape` (distance math),
   `Looter.Player.Hands.Grip.Contact` (every kind on SK_FP_Arms: palm seated within 0.3 of the gap, middle/ring/pinky
   touch within 0.4 and never sink past 0.35, pad within 1.5 cm of the trigger, indexed off it by 1 cm more).

## Controls (all live in PIE; `Looter.Hands.Kind 1..7` picks the gun, 0 all)

- Shares: `Looter.Hands.Contact` (0 = as before), `.Seat`, `.SeatGap`, `.FingerRadius` (0.8 camera cm),
  `.TriggerDiscipline`, `.TriggerHoldSeconds`.
- Shapes: `Looter.Hands.GripShape{Ahead,Right,Up,HalfAcross,HalfThrough,Corner}`, `Looter.Hands.ForeShape{...}`,
  `Looter.Hands.TriggerAt{Ahead,Up}`.
- Fingers: `Looter.Hands.{Trigger,Support}Thumb{Swing,Spread}`, `.IndexedSpread`; old `Looter.Hands.SupportFingerCurl`
  still nudges the table curls (the fallback).
- Hand turn/hold (existing): `Looter.Hands.{Trigger,Support}{PalmRoll,PalmPitch,PalmYaw,ForearmYaw,ForearmPitch,
  HoldAhead,HoldRight,HoldUp}`; framing `Looter.FPHands.<Kind>.*`, `.SupportForearmAngle`, `.ForearmBack`.
- Debug: `Looter.Hands.Grip.Debug 1` (handle boxes green/cyan, trigger spot red, finger joints; world-space draw, so
  it lines up only while the view model's FOV is the camera's: `Looter.FPHands.ViewFov 0`).
- Bake: `Looter.Hands.Dump` (holds) + `Looter.Hands.ContactDump` (shapes, trigger, thumbs).

## Phase 2 method (editor lane from Main)

1. Copy the held files in (check HASHES.txt against `git hash-object` first; merge if a file moved), add CODEMAP lines
   (below), `Tools\launch.ps1 -Build`.
2. `Tools\pie.ps1` on Ransom's Rest, `Looter.FPHands.PreviewAspect 1.7778`, `Looter.GiveWeapon` each kind,
   `play.py equip <n>`. Per kind: one `grab.ps1` window grab at hip, one ADS (input.ps1 RMB hold), one with
   `Looter.Hands.Grip.Debug 1`; `Looter.Hands.Contact 0/1` for before/after. Never Python capture batches.
3. Tune by cvars only (console.ps1), one grab per change, order: Rifle (C-clamp: thumb over the top, fingers under
   the belly, glove compact low-left; forearm on screen <= 20%), then Shotgun (pump wrapped, thumb along it), Revolver
   (fix the flat hand first; high grip, web under the tang), SMG (helix cupped, thumb over), Sniper (fore-end support
   under, thumb along), LMG (C-clamp on the handguard, palm out of the gun), Launcher (front grip wrapped, thumb over).
   High firing grip everywhere: `Looter.Hands.TriggerHoldUp` until the web sits under the receiver/tang (<= 0.5 cm gap).
4. Check reload (`R`), sprint and a throw on each: no snap when contact fades (Blend), no finger in the gun.
5. `Looter.Hands.Dump` + `ContactDump` -> bake into MakeX / AddContact; cvars back to 0; rebuild; tests once:
   `Tools\runtests.ps1 -Filter Looter.Player.Hands` (Grip.*, Rig, Arms, HeroArms, Reloads, Throw, OnGun) and
   `-Filter Looter.FPHands`; perf check if the solve shows in `stat game` (expected well under 0.1 ms: cached).
6. Before/after sheet per kind (hip, ADS) for the user; update Docs/Pipeline/Game.md ("First-person hands" and a new
   "Grip contact" bullet), CODEMAP, Handoff.

## CODEMAP lines to add (Player: first-person hands entry)

- `Player/FirstPersonHandsContact.cpp`: `FHandleShape`/`FHandContact` math, `FHandPlacement` make/blend/move; the rig's
  `Seated`, `ContactFingers` (fingers closed till they touch the handle, trigger finger searched onto the trigger; cached).
- `Player/FirstPersonHandsGripShapes.cpp`: per-kind handles, trigger faces, thumbs (`AddContact`), touch tuning
  (`Looter.Hands.Contact/Seat/...`, `GripShape*`, `ForeShape*`, `ContactDump`), `ContactOnGun`, trigger discipline cvars.
- `Player/FirstPersonHandsComponentGrip.cpp`: trigger readiness (`NextTriggerReady`), `Looter.Hands.Grip.Debug` drawing.
- Tests: `Tests/FirstPersonHandsGripTests.cpp` (`Looter.Player.Hands.Grip.Shape`, `.Contact`).

## Open points

- Trigger discipline default on (finger along the frame at rest, on the trigger to fire/aim): Main to confirm with the
  user if unsure; `Looter.Hands.TriggerDiscipline 0` turns it off.
- Rifle seat vs. the FP-HANDS `TriggerRight 2.5` (kept a finger from showing through the thumbhole): the seat pulls the
  trigger palm back onto the post; check the thumbhole in the grab, else lower `Seat` for the trigger hand.
- Third-person grip (LooterStancePoseGrip.cpp) untouched: first person only, as asked.
- No gun model / socket changes needed; part keys untouched.

## Phase 2 done (2026-10-22/23, not committed)

Built, tuned in PIE, baked (numbers and lessons in Docs/Pipeline/Game.md "Grip contact"). Added on the way:
`GripOffPalmSkin` 2.0 (seat by the palm's skin, not the bones), seat in three passes, thumb outer joints bend 80 toward
the fingers (`ThumbFlexToFingers`), thumb metacarpal not counted as touching, trigger search may press against the grip,
`Looter.Hands.Grip.Report`, `GetSeatedHoldPoint` (OnGun test checks the seated spot), `IndexedKnuckle` cvar. Tests
once: Looter.Player.Hands 9/9, Looter.Anim.Grip 4/4, Looter.FPHands 6/6. Grabs: Saved/Screenshots/Tools/fp_grip_<Kind>_hip|ads.png.
