# VFX-2 (2026-10-22): notes for Docs/Pipeline "VFX pass" (merge them there; the docs split was running)

Uncommitted. Before/after sheet: `Saved/Vfx2/compare.png`. Tests passed: Looter.Vfx (8), Looter.Weapons.FX (6),
Looter.Embers.Power (15), Looter.Creatures.React (5).

## Fire that reads in sun
- New master `M_FX_Fire` (`Tools/Unreal/build_vfx_materials.py fire`; args name masters to build, none = all four): unlit,
  translucent (alpha-blended), two-sided, ISM. Custom data (FireFloats 10): 0-2 fire colour, 3-5 smoke colour (light in it),
  6 opacity, 7 heat (1 white-hot .. 0 smoke), 8 seed, 9 erosion. One noise (two octaves; cheap on purpose: blasts cost overdraw).
  Heat ramp: soot -> deep red -> fire x1.9 -> white-yellow x4; rim burns dark first. Scalars Glow, SoftDepth (40).
- Why: additive fire (M_FX_Shape fireballs) washes to a pale disc in daylight; alpha-blended emissive keeps its hue.
- `FWeaponFX::SpawnFlame(FVfxFlame)` (`WeaponFXFire.cpp`, type in `VfxTypes.h`): a puff burning for `Burn` of its life, then
  soot; `Delay` (spawned with negative age: not moved or drawn until 0). Drawn on the "Fire" ISM, sorted far to near
  (`CollectFlames`), TranslucentSortPriority 1 (over the dust). Without the material: heat-tinted puffs on the smoke.
  Fades: camera inside (Near), and size/distance 0.8 -> 1.6 (`FlameCoverFrom/To`): a puff over half the view is overdraw.
- `SpawnExplosion` moved to `WeaponFXFire.cpp`. Fire blast: small white crack (9), core flame (0.65 -> 1.0 x reach, 0.75 s,
  burns 40%), `Fireballs` fire puffs rolling out/up (burn 24-34% of 1.5-2.1 s, then soot rising), soot column
  (SmokeColumn x 3/5, delay 0.15-0.55 s, 2.4-3.2 s, slow rise), faint ground ring (3.5, was 14), faint air shock (2.5),
  dust skirt (SmokeRing x 2/3, ground colour 0.27/0.22/0.165 x light), embers, debris, the ground's impact at 0.7 (pellet's
  half). Soot 0.075 x DustLight. Fire sized by min(reach, 380) (`FlameReachMost`) and smaller when the eye is near
  (`BlastCloseShare` 2.5 reaches -> down to 0.4). Soul blasts (Last Rites) keep the old additive look.
- Slag Bomb: additive flash 2.4 -> 1.1 (washed the fire white); no SpawnDirt on top; pool haze 9 -> 5/s and smaller
  (55-85 -> 115-160 cm), pool smoke 4 -> 2.5/s.

## The white ghost
- Cause: the hit flash (M_FX_HitFlash, additive overlay, Glow 1.7) started by the killing shot was held at full by the kill's
  hit-stop (CustomTimeDilation 0.02 for 75-100 ms) and faded over 90 ms after: the whole body white for ~10 frames, longer in
  slomo. Fix: `UCreatureReactComponent::HandleStateChanged(Dead)` -> `FCreatureHitFlash::Stop()` + overlay off. Kills show the
  kind's own death only.
- Slag Bomb's horde looked like ghosts too: every burn tick flashed white, then its orange tint (0.35 + 0.9 flare, additive)
  held. Now `UCreatureReactComponent::FQuietHits` (scope) around burn ticks: no white flash and no `EmberVfx::BodyHit` spill;
  tint 0 + 0.22 flare (a warm pulse). `Looter.Vfx.BurnTint 0` drops the tint (measured: costs ~0.03 ms GPU).
- Test added to Looter.Creatures.React.HitFlash (quiet tick, kill clears the overlay).

## Dirt and stone hits that read at 10-30 m
- `VfxRules::ImpactViewScale(distance)`: puff and chunk sizes x1 within 10 m -> x1.8 at 30 m+ (pixel size kept; counts are
  still DetailAt's). `FWeaponFX::ImpactShade`: dust colours x the light at the hit (FDustLight / 2: 1 in sun, bluish ~0.35 in
  shade), one sun trace per 0.25 s / 4 m.
- Dense geysers (`Geyser(..., bDense)`): billows of M_FX_Fire with no heat instead of soft puffs (the soft round puffs read as
  pale blobs on a sunlit road). Dirt: 5 soil billows (0.17/0.105/0.055, 30 -> 130 cm, thrown 520 cm/s), 9 clods 3.5-6.5 cm,
  12 soil crumbs (round, drawn as spray), 3 big clods 7-11 cm, bigger dust. Grass: 3 billows. Stone: pale jet billows
  (0.7/0.67/0.62) with a dark grit core (dense), chips 3.5-6.5 cm, pale flakes, 3 big chips 7-10 cm. All puffs ~1.6x bigger.

## Cost (RR town, Medium 1080p, `Looter.Perf.Horde Spider 8 Basic 0 -1400 90`, + `Looter.Hero Gauge,Looter.Embers.Repeat SlagBomb 4`)
- Before (current HEAD + nothing): horde 7.6 ms (p95 8.0-8.1); Slag 8.0-8.3 (p95 8.9-9.2). GPU translucency horde 0.19,
  Slag avg 0.72 / p95 1.58 / max 2.31; GPU p95 7.58.
- After: Slag translucency avg 0.43 / p95 0.65 / max 1.28; GPU p95 6.98 (horde 6.66-6.76). Rockets every 2 s (`Looter.Vfx.Blast
  2`): translucency avg 0.34 / p95 0.58, GPU p95 7.03. Clean frame runs mid-pass: Slag 7.8-8.0 (p95 8.5-8.7). The final frame
  times are void: an art agent's Blender terrain job (`RansomsRest.py --macro`, started 06:44 while ArtPause was off between
  builds) held a core, game thread +2 ms in every run (horde alone 8.3-8.6, p95 10.5+). Re-measure the frame with nothing
  else running; the remaining Slag p95 is game thread (first burst: hurts, numbers, kills) and render-thread visibility wait.
- Also cut for every fight: FX instances kept in blocks of 32 (`FWeaponFX::DrawInstances`, `WeaponFXInstances.cpp`): a
  particle born or gone no longer rebuilds the ISM proxy every frame (render thread 8.0 -> 7.7 in the Slag run); FX ISMs off
  distance fields, dynamic indirect, ray tracing. Kill crowd thinning (`KillCrowdDetail`: kills within 0.6 s / 8 m thin as a
  stream: 1, 1, 0.71, 0.58, 0.5...). Smoke puffs with no opacity are not drawn.
- Knobs: `Looter.Vfx.Fire 0` (heat-tinted puffs), `Looter.Vfx.BurnTint 0`, `Looter.Vfx.Blast.Scale`, `Looter.Vfx.Smoke`.

## Commands (`Weapons/VfxCommands.cpp`, not shipping)
- `Looter.Vfx.Blast [seconds | off]`: a rocket's blast where the view's aim meets the world; repeats for perf captures.
- `Looter.Vfx.Impact [scale] [surface]`: a round's hit at the aim in that surface's look, or the named one's (`1 Stone`).

## Lessons
- Never run a material script while PIE runs: the asset can't be saved ("Editor is in a play mode") and an "Overwrite Existing
  Object" modal blocks MCP until answered (Cancel at 348,200 of that 403x230 dialog).
- Grabbing timed effects: PIE's world time runs slow when the editor isn't in front; use `slomo 0.05-0.1`, then
  `console.ps1 ... -Wait 0.05` and grabs back to back. A blast's smoke column needs a view with open sky (the town sign hid it).
- `BugItGo` leaves ghost mode; `Walk` drops you to the ground (aim changes). z 170 near the town sign put the view under the
  street once; use 260.
- The Slag Bomb lands on the creature in sights: in a horde that is at the feet, inside its own fire (hence the eye scaling).
- Rocks near town for stone hits: SM_CliffFace_* round (-3500, 2700); the stone house at (-2680, 3100) reads as Wood
  (no Surface tag), so `Looter.Vfx.Impact 1 Stone` there.
