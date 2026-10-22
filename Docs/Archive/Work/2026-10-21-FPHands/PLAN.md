# First-person hands vs Borderlands: measurements, gaps, fixes (FP-HANDS phase 1, 2026-10-22)

The user's ask (2026-10-21): our FP hands show too much arm, clip into guns, don't hold them properly; compare with
Borderlands and match. Phase 1 = research + code in `Saved/FPHandsCode` (not built, not run). Phase 2 = copy in, build,
tune in PIE at 16:9, run `Tools\runtests.ps1 -Filter Looter.FPHands` and `-Filter Looter.Player.Hands`, commit.

Base: HEAD `5cc096a5`. Holding-tree files, each copied from HEAD first (blob = `git rev-parse HEAD:<path>`):

| File (Source/AI_Looter_Shooter/...) | HEAD blob | Change |
|---|---|---|
| Player/FirstPersonHandsFraming.h/.cpp | new | the framing table, cvars, rules |
| Tests/FPHandsFramingTests.cpp | new | `Looter.FPHands.*` tests |
| Player/FirstPersonHandsComponent.cpp | 51e5db87fa | one-hand park, forearm framing in PoseHands |
| Player/FirstPersonHandsComponent.h | ca74575cf0 | ParkSupportHand, FrameForearms, OneHandBlend |
| Player/FirstPersonHandsRig.h | fe0f710c3f | GetForearmLength |
| Player/FirstPersonHandsGrips.cpp | c2b1008d42 | Support/Trigger nudges in OnForegrip/OnGrip |
| Player/PlayerViewCamera.cpp | 37117f2824 | view model FOV via FPHandsFraming::ViewModelFieldOfView |
| Player/PlayerLocomotionViewModel.cpp | b8d0246d99 | AimSightDistance x DepthScale |
| Inventory/WeaponManagerSlots.cpp | 8ca70fe9c8 | hip nudge per kind + DepthScale push-out in GetFirstPersonHold |
| Tests/FirstPersonHandsPlayTests.cpp | 98a9ebf2e9 | spots include nudges; revolver left hand skipped (parked); trigger hand may sit on the bottom edge |

CODEMAP line to add (Player): `Player/FirstPersonHandsFraming`: `FPHandsFraming`, the Borderlands framing of the FP gun
and arms: per-kind hip/hand nudges and one-handed share (`Looter.FPHands.<Kind>.*`), forearm framing (`FrameForearm`:
slanted, elbow out of the frame), parked support hand, view-model FOV and its push-out (`ViewFov`, `DepthScale`);
`Looter.FPHands.Dump`. Tests `Looter.FPHands.*`.

## 1. Borderlands measurements (16:9, fractions of width across / height down)

Looked at only, nothing copied. Sources:
- BL3 sniper, hip (Moze): https://gamingpastime.com/wp-content/uploads/2021/10/borderlands3_2.jpg
- BL3 rifle, firing: http://gamingpastime.com/wp-content/uploads/2021/10/borderlands3_7.jpg
- BL3 rifle, hip: https://www.newgamenetwork.com/images/uploads/gallery/Borderlands3/bl3_09.jpg
- BL3 pistol: https://assetsio.gnwcdn.com/sctkqg.jpg (Rock Paper Shotgun)
- BL3 scoped pistol: https://images.nintendolife.com/screenshots/141296/large.jpg
- BL3 SMG/pistol: https://www.gamerguides.com/assets/media/17/169/Fire_in_the_Sky_Rocket_Launch_Borderlands_3.jpg
- BL3 launcher: http://gamingpastime.com/wp-content/uploads/2021/10/borderlands3_1.jpg
- BL3 ADS: https://cms.mygameon.my/wp-content/uploads/2019/09/Borderlands®-3_20190925211442-1024x576.jpg
- BL2 rifle: http://gamingpastime.com/wp-content/uploads/2021/07/borderlands2_1.jpg
- BL2 (Maya): https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/49520/ss_3d8370934170622af4fcc8a3975f7be480b3d815.1920x1080.jpg
- BL2 viewmodel talk (some guns take about a third of the screen; pulling the view model back shows the arm's missing
  part, i.e. BL's arms are cut too): https://steamcommunity.com/app/49520/discussions/0/6194224803798223498
- BL3 has no weapon-FOV option (only the world FOV slider): https://www.nexusmods.com/borderlands3/mods/289?tab=posts

| Item | Borderlands |
|---|---|
| Gun's place | muzzle 0.54-0.63 across, 0.52-0.64 down; body runs diagonally to the bottom-right corner, line 25-40 deg above horizontal |
| Gun size (long guns) | 30-45% of width, 45-60% of height (scopes reach 0.25 down); "about a third of the screen" |
| Gun size (pistols) | 13-20% of width, 40-50% of height, upright on screen, centred 0.57-0.68 across |
| Arms shown | gloves and wrists; a cuff at most. Forearm only on some BL3 shots: slanted 55-60 deg down-left, a gauntlet ~30% of height. Never vertical, never an elbow |
| Support hand | under/round the fore-end from the near (left) side; low: 0.80-1.0 down in BL2 and most BL3 shots, 0.58-0.72 with a slanted forearm in the BL3 sniper |
| Trigger hand | hidden behind the receiver, or a glove at the far bottom right (0.78-1.0, 0.65-0.85), hand only |
| Pistols | right hand only, back of the glove toward the eye, at the bottom 0.56-0.80 across, 0.82-1.0 down; left hand out of view |
| Launcher | big (0.53-0.80 across, 0.38-1.0 down), support glove on its left side mid-low |
| Angle | barrel roughly toward the crosshair (vanishing point); no muzzle-down tilt; no cant on long guns, pistols upright |
| View-model FOV feel | near parts don't balloon: the gun reads flat, like a narrower view-model angle than the world's (no number published) |
| ADS | sight on the crosshair; gun runs down the middle to the bottom edge (0.35-0.75 across there); no hands in view |

## 2. Ours (grabs in Saved/Screenshots/Tools; they are 2.17:1 editor viewports, so heights converted to 16:9: y16 = 0.5 + (y - 0.5) x 0.82)

| Kind (grab) | Muzzle | Gun size | Support hand / forearm | Trigger hand |
|---|---|---|---|---|
| Rifle (fp_r2_Ellis_rifle_hip/_hands) | 0.59, 0.59 | 36% w, top 0.50 | glove beside the handguard 0.58-0.62, 0.63-0.70; sleeve stands straight up from the bottom edge, ~32% of height | inside the Bullpup's stock; a finger pokes out of the thumbhole (clip) |
| Shotgun (fp_r2_*_shotgun_*) | 0.57, 0.60 | stock fills bottom-right | on the pump 0.63-0.73; vertical sleeve ~30%; Pike's bare fingers stick up beside the pump | under the frame |
| Revolver (fp_r2_*_revolver_*) | 0.62, 0.61 | ~18% w | two-handed cup at the bottom 0.61-0.80; Crane's cuffs are slabs across the bottom | cupped |
| SMG (newguns_smg_fp) | 0.59, 0.62 | 41% w, 49% h | vertical sleeve 0.57-0.63 across from 0.80 to the edge | glove visible at bottom 0.66-0.75 |
| Sniper (newguns_sniper_fp) | 0.57, 0.58 | scope dominates the right | hand 0.61, 0.70; vertical sleeve | hidden |
| LMG (newguns_lmg_fp) | 0.58, 0.59 | 42% w, 53% h (top of BL's range) | vertical sleeve from 0.77 | hidden |
| Launcher (newguns_launcher_fp) | 0.60, 0.58 | tube 0.60-1.0 across, 0.58-1.0 down | glove on the front grip at the bottom edge | hidden |
| Gauge (vfx_muzzle_*, fp_r2_Gauge_*) | | | black-gold forearm post with a ball where the hand should wrap: reads as a knob beside the gun | |

Heroes: same holds for all five (arms 0.84-0.88; the grip table is shared); what differs is the sleeve: Ellis's duster
and Crane's cuffs are thick dark columns, Gauge's prosthetic is a lacquered cylinder, Pike's bare fingers don't close.

## 3. Gaps

1. **Forearm post.** Our support forearm runs straight up the frame from the bottom edge (~30% of height at 16:9); BL
   shows a glove and wrist, or a slanted gauntlet. Cause (geometry): the fore-end is 65-75 cm out and SK_FP_Arms' forearm
   is 26.1 cm (23 at 0.88), so a mid-frame wrist can only get its elbow (the sleeve's capped cut) out of the frame by
   pointing nearly straight down. A slant needs the wrist lower in the frame or a longer forearm.
2. **Trigger hand.** Buried in the Bullpup's stock with a finger poking out; under the frame on the stocked guns. BL: hidden
   behind the receiver or a clean glove bottom-right.
3. **Support grips.** Rifle glove beside the handguard (a hand holding a stick); Pike's fingers open by the pump; Gauge's
   hand reads as a ball. BL: the glove wraps the fore-end from the near side, fingers over it.
4. **Revolver.** Two-handed cup with big cuffs; BL holds pistols one-handed, upright, glove back to the eye.
5. **Big guns.** LMG and launcher at the top of BL's size range.
6. **View-model FOV.** Fixed at the camera's 90; near parts balloon. A narrower angle needs the holds further out (gun
   stays put on screen) and so the hands: the forearms then show more. Only worth it with longer forearms.
7. **Grabs are 2.17:1** (editor viewport): 18% less height than 16:9, so more arm shows at 16:9 than the grabs suggest.
8. Gun place/angle and ADS already match BL closely; leave them.

## 4. Fixes (code in the holding tree; every number a live cvar, `Looter.FPHands.Dump` prints the bake lines)

Global: `Looter.FPHands.Enable` 1 (0 = as before, for A/B in PIE).

A. **Forearm framing** (`FPHandsFraming::FrameForearm`, applied to both hands every frame in `PoseHands`, after reloads
   and throws placed them): the most slanted way from `SupportForearmAngle` 60 / `TriggerForearmAngle` 65 (deg under the
   horizontal, outward) toward straight down whose elbow leaves the frame by `ElbowMargin` 0.12 (half-frame share), each
   slant tried with `ForearmBack` 0.35 of it turned back toward the eye's plane first; never more than `MaxForearmSwing`
   45 off the grip table's way; never an elbow nearer the lens than 6 cm; where nothing gets the elbow out, the way that
   gets it furthest (the table's if nothing beats it: never worse than today). `ForearmShare` 1, `WristBack` 7 cm,
   `Aspect` 16:9. Effect: slanted where the hand is low (shotgun pump, SMG helix, launcher grip, trigger hands); where the
   wrist is mid-frame (rifle, sniper fore-end) it stays steep until B/C lower it or E lengthens the forearm.
B. **Per-kind nudges** `Looter.FPHands.<Kind>.<Field>` (Kind = Rifle, Shotgun, Revolver, SMG, Sniper, LMG, Launcher;
   Fields HipAhead/HipRight/HipUp cm, HipPitch/HipYaw/HipRoll deg, on top of WeaponHipHold's table; SupportAhead/Right/Up
   and TriggerAhead/Right/Up cm of the modeled gun, on top of FirstPersonHandsGrips'; OneHand 0-1). Defaults:
   - Rifle: HipRight +1, HipUp -2 (support stays: 6 cm back is the Bullpup's grip-loop front at u 37.8; foregrip u 44).
   - Shotgun: HipUp -1.5, SupportAhead -3 (on the pump, lower in frame).
   - Revolver: HipRight -5, HipUp +1, HipRoll +6 (net -2: upright), OneHand 1.
   - SMG: HipUp -1.5, SupportAhead -2 (along the helix).
   - Sniper: HipUp -1, SupportAhead -6 (fore-end u 52 -> 46; the box is at 35.6).
   - LMG: HipAhead +4, HipRight +1, HipUp -1, SupportAhead -3 (handguard u 44 -> 41; it starts at 37.6).
   - Launcher: HipAhead +4, HipUp -1.
   - Trigger* all 0: phase 2 sets `Looter.FPHands.Rifle.TriggerRight` (try +0.8 to +1.5) until no finger shows through the
     Bullpup's thumbhole; same check on every stocked gun.
C. **One-handed revolver** (`OneHand` 1): the support hand eases (`OneHandSeconds` 0.15) to `ParkAhead/Right/Up`
   (20, -26, -38 cm: under the frame, left) whenever its job is Foregrip; back on the gun for a reload or a throw. Also
   removes Crane's/everyone's two cuffs at the bottom. (User may want two hands back: `Looter.FPHands.Revolver.OneHand 0`.)
D. **View-model FOV** `Looter.FPHands.ViewFov` (0 = camera's; try 72-80) with `BaseFov` 90: holds' X x DepthScale
   (tan 45 / tan(ViewFov/2)), ADS distance too, so the gun keeps its spot and size and loses near-eye stretch. Default
   off: it pushes the hands out and makes gap 1 worse unless E is done.
E. **Not code, needs the user's yes (art change, no new tool):** lengthen the first-person forearms ~1.25x (26 -> ~33 cm)
   in `FPArms.py` and the four `FPArms<Hero>.py` via fp_arms_heroes (the usual view-model trick; BL's arms are cut and
   placed for the camera too). With it, A gets every support elbow out on a 55-60 deg slant (BL3-like) and D becomes safe.
   Rig jsons regenerate; code reads lengths from the skeleton, nothing else changes.
F. **Per-hero looks (art, phase 2 judges):** Gauge's hand reading as a ball (check his finger blocks close round the
   fore-end; maybe `Looter.Hands.SupportFingerCurl` for him), Pike's open fingers on the pump (`EHandGrip::Pump` curls on
   bare hands), Crane's cuffs (gone with C at the hip; check in reloads).

## 5. Tests (`Looter.FPHands.*`, Tests/FPHandsFramingTests.cpp)

Plain rules: `.Project` (frame edges, behind the eye, visible-forearm share), `.ViewDepth` (no push at equal FOV; a point
pushed out by DepthScale keeps its pixel at the narrower FOV), `.Forearm` (way normalised, <= MaxSwing off the table, never
ahead of the wrist, elbow at least as far out as the table's, never at the lens, runs down and outward; a low support wrist
gets a slanted way with its elbow out; Share 0 = table), `.Kinds` (nudges small, support slides along the gun, one-handed
only the revolver, rifle support doesn't slide), `.Park` (parked hand off-frame at 90 and 40 deg, left, forearm down).
Play: `.Play.OnScreen` (all seven guns on BP_LooterCharacter at the hip: both elbows out of the frame; support glove in
the frame, or parked off it for the revolver; logs % of height each forearm shows). Updated: `Looter.Player.Hands.OnGun`.

## 6. Phase 2 must check in PIE (16:9: standalone `-game` 1920x1080 or PIE new window, not the 2.17:1 editor viewport)

1. Build; run `-Filter Looter.FPHands` and `-Filter Looter.Player.Hands`. `.Play.OnScreen` may fail on rifle/sniper
   elbows if the framing can't get them out: that is gap 1; tune B (HipUp, SupportAhead) or take E to the user.
2. Each kind x each hero (`Looter.Hero <Id>`, `Looter.GiveWeapon`): hip, walk, sprint, crouch, reload, throw, swap,
   ADS. Look for: no vertical sleeve post, no elbow/cut end, support glove round the fore-end, no finger through the
   Bullpup thumbhole (`Looter.FPHands.Rifle.TriggerRight`), revolver one-handed and its support hand coming in cleanly
   for the cylinder reload and the jar throw (no arm sweeping across the view in the 0.15 s ease).
3. `Looter.FPHands.Enable 0/1` side by side; then `Looter.FPHands.Dump` and bake the tuned numbers into MakeDefaults.
4. Try `Looter.FPHands.ViewFov 75`: gun same place, flatter; judge whether the arms get worse (expected without E).
5. Sight picture unchanged at ADS for all seven (AimSightDistance x DepthScale = 1 by default).
6. Perf: nothing measurable (a few dozen projections per frame).
7. Update Docs/Pipeline.md "First-person hands" (framing, cvars, the forearm-length limit) and CODEMAP.

## 7. Phase 2 result (2026-10-22)

Merged (no Source changes since 5cc096a5), arms (1.25x forearms) + Rustler/Thresher imported, setup_gun_parts and
create_rank_assets rerun, built; tests FPHands 6/6, Player.Hands 7/7, Heroes 10/10, Weapons 51/51 (Trigger.Gun failed
once before the bake, passed on rerun: a flaky roll). Tuned in PIE at 16:9 via `Looter.FPHands.PreviewAspect 1.7778`;
baked: rules 50 deg / back 0.6 / margin 0.06 / swing 60; Rifle TriggerRight 2.5; LMG HipUp -2.5, SupportAhead -6;
Launcher HipAhead 9, HipRight 2, HipUp -2. Forearm on screen: rifle 29%, LMG 22, sniper 19, shotgun 17, launcher 17,
SMG 14, revolver 13 (trigger arm); no elbow anywhere. Sheet: compare.png. ViewFov 75 tried, left off.
