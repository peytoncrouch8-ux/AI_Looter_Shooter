# NIAGARA (round 5): the cinder jar's fire rebuilt in Niagara, by script

Nothing built, run or committed (Edit/Write only). Main builds, enables the plugin (after asking the user), runs the script.

## What's there
- Niagara: engine plugin, on by default (not listed in the .uproject = enabled); the game module already links `Niagara`
  (Build.cs). Nothing to change for the runtime.
- **The Python route** (researched on disk, UE 5.8): NiagaraEditor exposes no authoring functions to Python (only
  baker/upgrade helpers; `NiagaraSystemFactoryNew` creates an empty system but its emitter list isn't a property). The one
  scripting API that adds emitters, modules, inputs and renderers is the **Cascade To Niagara Converter** plugin's
  (`unreal.FXConverterUtilitiesLibrary`: `create_system_conversion_context(system)` -> `add_empty_emitter`,
  `find_or_add_module_script`, `set_parameter`, `set_parameter_directly`, `add_renderer`, `set_sim_target`, `finalize`;
  inputs: float/int/bool/vector/vec2/enum/linked attribute/dynamic input). Engine plugin, `Editor` module only (ships
  nothing), Beta, **off by default**. The script uses only its C++ library; every module is the engine's `/Niagara/...`.
  Not possible through it: user parameters (so no per-spawn strength/radius parameters; the code scales the component and
  deactivates it instead), and no in-editor preview check (eyes in PIE).

## Built (files)
- `Tools/Unreal/build_niagara_fx.py` (new): materials + `NS_CinderBed` + `NS_CinderBurst` in `/Game/Art/Effects/Niagara`
  (already cooked: DefaultGame.ini cooks `/Game/Art/Effects`). Reuses build_world_materials.py's graph helpers and
  build_vfx_materials.py's `FIRE_CODE`/`NOISE` by AST (neither edited, neither run). Every look number in its `BED`/`BURST`
  tables. Refuses non-matching assets at its paths; rebuilds a system by delete + create (not during PIE).
  - `M_NFX_Fire` (M_FX_Fire's shader on Niagara sprites: particle colour = fire, alpha = opacity, heat burns down over
    `Burn` of life, age erodes, MaterialRandom = seed; SmokeColor, FadeIn/FadeOut, Glow, SoftDepth), `MI_NFX_Tongue`
    (Burn 0.85), `MI_NFX_Smoke` (HeatScale 0), `M_NFX_Ember` (additive dot/streak, flicker, cools with age),
    `M_NFX_Heat` (additive pool lying on the ground, coals as a world-space cell grid pulsing, CoalScale 22 cm),
    `MI_NFX_HeatFlash`.
  - `NS_CinderBed` (looping, CPU, world space): Heat 3/s flat glows 5.2-6 m; Flames 14/s (the code's numbers: 22-38 cm
    x2.6, 0.8-1.2 s, burn 55%); Tongues 9/s velocity-aligned licks 18-30 x 50-85 cm rising 150-240; Embers 26/s streaks
    with curl noise; Smoke 5/s 70-110 cm x3, 2.2-3.2 s. ~85 live sprites.
  - `NS_CinderBurst` (one shot): HeatFlash 1 flat glow; Sparks 70 embers flung 280-1100 cm/s falling back; Flare 8 fire
    puffs; Plume 6 soot puffs at 0.12 s. ~85 sprites for ~1.7 s, the plume to ~3.4 s.
- `Weapons/NiagaraFX.h/.cpp` (new): `ENiagaraFX {CinderBed, CinderBurst}`, `NiagaraFX::IsOn/ScaleKnob/PathOf/Find/UseFor/
  SpawnAt` (spawned at a location with its Z up the ground normal, scaled, not attached, auto-destroying, not pooled,
  no pre-cull). Cvars `Looter.Fx.Niagara` [1] (0 = code look everywhere; read as each effect starts),
  `Looter.Fx.Niagara.Scale` [1, 0.5..2]. A system not built = the code look (so today's build plays as before).
- `Combat/GraveSaltCinderBed.h/.cpp`: on its first look it starts NS_CinderBed (scale Radius/260); while Niagara draws
  it, the code's flames and ember puffs are skipped; the edge ring, the point light flicker and bodies catching stay the
  code's in both looks. Dies down: deactivated at its last second (Strength < 1) and in EndPlay; its last flames/smoke
  finish after the bed is gone. A bed keeps the look it started with. `IsDrawnByNiagara()` for tests.
- `Combat/GraveSaltBurstEffects.cpp` (`SpawnEffects`): for the cinder look (Element Ember, not small) with NS_CinderBurst
  built, spawns it (scale reach/340) and draws the blast without the code's Ember burst; the shared salt-jar explosion
  under it is unchanged. `DrawBlast` itself untouched (tests call it).
- `GraveSaltGrenadeKinds.cpp`: no change needed.
- `Tests/NiagaraFXTests.cpp` (new, `Looter.Fx.Niagara`): off = code look and nothing spawned; on = Niagara exactly when
  built (info line when not); a bed keeps its look through a mid-burn flip and burns out on time.

## How Main runs it
1. Ask the user (CLAUDE.md rule 3), then add to the .uproject Plugins:
   `{"Name": "CascadeToNiagaraConverter", "Enabled": true, "TargetAllowList": ["Editor"]}`.
2. `Tools\launch.ps1 -Build` (new files: NiagaraFX.*, NiagaraFXTests.cpp).
3. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_niagara_fx.py check"` (plugin + every engine
   module/enum present; builds nothing), then the same without `check` (or `materials`, `bed`, `burst`). Look for
   `NIAGARAFX done` and errors in the output log (a wrong input name shows as a conversion-context log line, not an
   exception). Open NS_CinderBed once in the editor if it shows nothing in PIE: a compile error lists there.
4. Tests: `Tools\runtests.ps1 -Filter Looter.Fx.Niagara`, `-Filter Looter.Grenade.Kinds.Cinder`, `-Filter Looter.Grenade.Blast`.
5. Compare in PIE: throw a cinder jar with `Looter.Fx.Niagara 1`, then `0` (no give command exists:
   `Looter.Grenade.Kinds.FromLevel 1`, `.Share 1`, `.PitchWeight 0`, `.ScatterWeight 0` make found tins cinder).
6. Perf: `Tools\perf.ps1` with a cinder bed in a fight, Niagara 1 vs 0 (ArtPause on).

## Unverified (no editor here) and the likely fixes
- Input names were taken from the converter's own scripts and the module assets' strings (InitializeParticle "Lifetime",
  "Sprite Size Mode"/"Sprite Size", "Sprite Rotation Mode"/"Sprite Rotation Angle"; ShapeLocation "Shape Primitive",
  "Cylinder Radius/Height/Height Midpoint", "Sphere Radius", "Apply Owner Scale"; AddVelocity [1,2] "Velocity",
  "Rotation Coordinate Space"; ScaleSpriteSize [1,1] "Scale Sprite Size Mode"/"Uniform Scale Factor"; CurlNoiseForce
  "Noise Strength"/"Noise Frequency" (frequency 50 is a guess at its units: drop `curl` in BED.embers if embers swirl
  wrong). Velocity-aligned sprites: SpriteSize.y is assumed the length along velocity (swap the tuples if streaks lie flat).
- Flat glows use `Particles.SpriteFacing = Engine.Owner.SystemZAxis` with CustomFacingVector (world-space sim).
- Heat glows are flat quads (like the code's ring): on bumpy ground parts hide under it or float a few cm.

## Cost (estimate; Main measures)
- Code look per bed: ~14 fire quads + ~8 ember pieces on FWeaponFX's ISMs, ring, light: ~0.02-0.04 ms game thread,
  ~0.05-0.15 ms GPU (flame overdraw).
- Niagara bed: ~85 CPU-sim sprites in 5 emitters: ~0.04-0.08 ms game/worker thread (Niagara's per-system fixed cost
  dominates), ~0.02 ms render thread; GPU ~0.15-0.35 ms near it (smoke growing to ~3 m and five overlapping 5.6 m
  additive ground glows are the extra overdraw). Burst: +0.1-0.2 ms GPU for ~2 s. Knobs: smoke `grow`/`rate`, heat
  `rate`, `Looter.Fx.Niagara 0`.
- First spawn may hitch once (new shaders/PSOs; the editor compiles the materials and system on the script's run).

## For Main's docs (not edited by me)
- CODEMAP: `Weapons/NiagaraFX.h/.cpp` Niagara effects: paths, Looter.Fx.Niagara switch, SpawnAt; `Tests/NiagaraFXTests.cpp`
  Looter.Fx.Niagara; `Tools/Unreal/build_niagara_fx.py` builds NS_CinderBed/NS_CinderBurst and their M_NFX_* materials.
- Gunplay.md Grenades/VFX pass: the cinder look's Niagara option and cvars as above; lesson: Python authors Niagara only
  through the Cascade To Niagara Converter's contexts (editor plugin); no user parameters, so code scales/deactivates.
