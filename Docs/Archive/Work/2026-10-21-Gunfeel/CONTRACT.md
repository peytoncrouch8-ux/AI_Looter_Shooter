# Gunfeel contract (Phase 12.1, Docs/Plan.md; written by Main 2026-10-21)

Goal (the user, after playing Ransom's Rest): enemy hit reactions, gun sound and kick, and guns that feel different are
the weak parts of gunplay. Hit markers and damage numbers are fine: don't touch them. Quality bar: big-studio looter
shooters (Borderlands). Read CLAUDE.md first; the Pipeline sections you touch; this file whole.

## Rules for every agent
- Every number that affects feel is a `Looter.Gunfeel.*` cvar (TAutoConsoleVariable<float>/<int32>, ECVF_Default),
  read through an accessor in your own tuning file, so the user tunes it live in PIE. Defaults below are starting
  values; Main bakes the user's values in later. Never cache a cvar's value across frames where a live change matters.
- Plain rules (numbers in, numbers out) live in structs/free functions the tests drive without a world, as
  `FHitReaction` does. Tests: `Tests/Gunfeel<Agent>Tests.cpp`, names `Looter.Gunfeel.<Agent>.<What>`. A test reads the
  rule with explicit numbers or the cvar defaults, never a value the user may have set (reset any cvar a test sets).
- Edit only the files you own (below). Need a change elsewhere: write it in your report, Main makes it.
- **Do not build, launch or close the editor, run tests, or commit.** Write code that compiles (every .cpp includes
  what it uses; no unity build; anonymous-namespace names are file-private). Main batches one build for all agents,
  fixes small compile errors itself and sends you anything bigger.
- Bosses (`ECreatureRank::Boss` or any creature with a `UBossComponent`) never have a move interrupted by anything new:
  no new stagger/knock/slow on them; a smaller flinch only (`Flinch.BossScale`).
- Performance: RR spawn, 8 spiders, Medium: baseline 7.9 ms (p95 9.0) today; target under 8.3. No new lights on
  Medium (`AEmberPropActor::LightsAllowed` pattern), no per-hit traces beyond one, effects through the pooled
  `FWeaponFX`, no Niagara.
- Style: tabs, UE naming, comments say why in plain words, files under ~500 lines (split by topic), log to LogLooter.
- Update `CODEMAP.md` lines for files you add or change the role of (one line each), and append your how-to and
  numbers to the Pipeline section named for you below (Main merges wording).
- Report in under 300 words: files changed, every cvar and its default, test names, what you couldn't check, risks.

## Shared pieces (Main wrote them; don't change their signatures)
- `Weapons/GunshotSound.h/.cpp`: `GunshotSound::PlayShot(const AWeaponBase&)` (called from `AWeaponBase::FireShot`;
  today plays the kind's one Fire cue), `PlayLowAmmo(Weapon, RoundsLeft, Magazine)` (stub), `PlayCritRing(WorldContext)`
  (2D cue `Weapon.Revolver.CritRing`), `CueFor(Kind, EGunshotLayer)` -> "Weapon.<Kind>.<Layer>" (Mech, Low, TailOpen,
  TailTown, TailInside, Distant, LowAmmo). A cue with no sounds plays nothing, so code may call them before they exist.
- A bullet's damage reaches health with `DamageCauser` = the `AWeaponBase` that fired it (`UBulletSubsystem`
  `ApplyPointDamage(..., Weapon, ...)`): read the gun's kind with `Cast<AWeaponBase>(DamageCauser)` ->
  `GetInstance().Definition->Kind`. Shotgun pellets each arrive as their own damage event within `FHitReaction`'s
  80 ms burst window.

## GUNFEEL-REACT (Opus): every hit reacts; the guns' jobs on the target
Owns: `Combat/HitReaction.h/.cpp`, `Creatures/CreatureHitReactionComponent.h/.cpp`, `Creatures/CreatureKnockComponent.*`,
`Creatures/CreatureBase.h` (only: a protected "last hit" struct and its accessor), `Creatures/CreatureBaseHurt.cpp`,
each kind's hurt/pose/react code: `SpiderCreature*.cpp/.h`, `SpitterSpider*`, `SlimeCreature*`, `UnpaidCreature*`,
`DeadMinerCreature*`, `BellRingerCreature*` (flinch only); `Weapons/WeaponFXCreatures.cpp`, `Weapons/WeaponFXWorld.cpp`
(death bursts), `Weapons/WeaponFX.h` (only additions for these), `Weapons/CreatureBlood.*`; new
`Combat/GunfeelReactTuning.h/.cpp` (its cvars), new `Tests/GunfeelReactTests.cpp`, and the existing tests of the rules
it changes (`Looter.Combat.HitReaction*` and any creature reaction tests: list the groups in your report).
Not: bosses' own files (Foreman, Gravemother, Abel/Keeper, BossComponent).

1. **Flinch on every hit, sized by damage.** Before `OnHurt`, `ACreatureBase::HandleDamaged` stores the hit:
   `FCreatureHit { float Share (damage / max health); EWeaponKind Kind (None if not a gun); bool bCritical; FVector
   Direction (shot's way); }` read by `GetLastHit()`. Each kind's `OnHurt` scales today's flinch (spider body kick and
   tilt, abdomen; slime squash/lean/core; Unpaid flinch, arms, shroud; miner via Unpaid) by
   `FlinchStrength = Scale * clamp(0.6 + DamageGain * Share, 0.6, Max)`, crit x CritScale. The strongest hit in a burst
   stands (pellets don't add up), as today. Plus a brief slow on any non-boss hit (its move speed x Slow for
   SlowSeconds, not stacking past one), so a charging enemy visibly catches the bullet. Make the flinch read at 20 m.
2. **Staggers more often**: the rank table scales from Basic's seconds; heavy share, cooldown, shove from cvars. A crit
   or heavy body hit also gets a short hit-stop (`HitStop.Heavy`), cooldown as the crit's.
3. **Kills by gun**: shotgun kill: corpse knock `Kill.ShotgunKnock` with a small lift arc (`Kill.ShotgunLift` cm), never
   through walls or off drops (today's sweep and ground rules); revolver crit kill: death burst x `Kill.RevolverBurst`
   at the crit spot plus an extra spurt; rifle kill: knock `Kill.RifleKnock` (crumples where it stands). A crit kill
   keeps today's crit knock where larger.
4. **Hit spurts**: creature hit FX (blood/gel/ash by `ECreatureBlood`) x `Spurt.Scale`, plus an exit spray behind the
   target along the shot (`Spurt.Exit` share of the entry's particles; 0 turns it off). Shotgun pellets in one burst
   share one spurt budget (no 8x particle spam).
5. **Step 3, the guns' jobs on the target:**
   - Rifle, pressure: `Rifle.PressureHits` rifle hits within `Rifle.PressureWindow` s on one creature = a stagger (its
     own cooldown = the stagger cooldown).
   - Shotgun, power: a blast whose burst share >= `Shotgun.KnockShare` within `Shotgun.KnockRange` cm knocks a creature
     of size <= `Shotgun.KnockSize` back through `UCreatureKnockComponent::Knock` (launch `Shotgun.KnockSpeed`), else
     staggers it; never a boss or Legendary.
   - Revolver, precision: crit hit-stop `Revolver.CritHitStop`; on a revolver crit call `GunshotSound::PlayCritRing`
     for the shooter (local player only).

Cvars (name: default; today's value in brackets):
`Looter.Gunfeel.Flinch.Scale` 1.0 · `.Flinch.DamageGain` 3.0 · `.Flinch.Max` 2.5 · `.Flinch.CritScale` 1.4 ·
`.Flinch.BossScale` 0.4 · `.Flinch.Slow` 0.55 · `.Flinch.SlowSeconds` 0.12 ·
`.Stagger.HeavyShare` 0.12 [0.2] · `.Stagger.Cooldown` 0.7 [1.1] · `.Stagger.Seconds` 0.4 [0.32] · `.Stagger.Shove`
420 [360] · `.HitStop.Crit` 0.05 [0.045] · `.HitStop.Heavy` 0.035 [none] · `.HitStop.Kill` 0.075 [0.065] ·
`.HitStop.CritKill` 0.1 [0.08] · `.Kill.ShotgunKnock` 650 [260] · `.Kill.ShotgunLift` 35 [0] · `.Kill.RifleKnock` 180 ·
`.Kill.RevolverBurst` 1.6 · `.Spurt.Scale` 1.5 · `.Spurt.Exit` 0.6 · `.Rifle.PressureHits` 5 · `.Rifle.PressureWindow`
1.0 · `.Shotgun.KnockShare` 0.35 · `.Shotgun.KnockRange` 600 · `.Shotgun.KnockSize` 1.2 · `.Shotgun.KnockSpeed` 520 ·
`.Revolver.CritHitStop` 0.09.
Pipeline section: "Gunfeel: hit reactions".

## GUNFEEL-KICK (Sonnet): gun kick, rumble, handling, the guns' jobs in the hand
Owns: `Weapons/WeaponRecoil.h/.cpp`, `Player/ViewKick.h/.cpp`, `Player/PlayerViewKicks.cpp`, `Player/PlayerViewAim.cpp`,
`Player/PlayerViewCamera.cpp` (held gun motion only), `Player/CameraShakeModifier.*`, `Weapons/WeaponBaseFiring.cpp`
(keep the `GunshotSound::PlayShot` call), `Weapons/WeaponBase.h/.cpp` (muzzle flash, spread), `Weapons/WeaponDefinition.h`,
`Weapons/WeaponHipHold.*`, `Inventory/WeaponManagerSwap.cpp`, `Player/WeaponSwapMotion.*`, the aim-move speed's file
(find it), `Settings/ControlSettingsSubsystem.*` and its save, `UI/Menus/SettingsMenuPreview.cpp/.h` (the rumble row
beside Camera shake), `Combat/BulletSubsystem.cpp` (only the shot's damage line and its falloff), new
`Weapons/GunfeelKickTuning.h/.cpp`, new `Tests/GunfeelKickTests.cpp`, and existing tests of what it changes
(ViewKick, recoil, settings: list the groups).

1. **Kick per kind** (step 2): view kicks x `Kick.<Kind>`; gun model recoil (kick back, flip, twist, roll) x
   `Recoil.<Kind>`; slower settle for the shotgun; first-person gun motion bigger (kick back and flip visible); the
   built-in muzzle flash x `Muzzle.Scale1P` in first person (third person unchanged).
2. **Controller rumble** (new): `APlayerController::PlayDynamicForceFeedback` per shot (intensity, seconds per kind),
   on a hit taken (by its share of health), x the new setting "Controller rumble" 0-100% (default 100%, saved like
   Camera shake, its row beside it) x `Rumble.Scale`. Only for the local player using a gamepad (nothing on mouse).
3. **Low ammo**: after each shot call `GunshotSound::PlayLowAmmo`; fill in its body in GunshotSound.cpp's stub only
   (the share and pitch rules: last `LowAmmo.Share` of the magazine, pitch from 1.0 to `LowAmmo.PitchTop` at the last
   round; magazines under 4 never). GUNFEEL-SOUND owns the rest of GunshotSound.cpp later.
4. **Step 3, the guns' jobs in the hand:**
   - Rifle: aim kick grows `Recoil.RifleClimb` per shot in a burst (up to `Recoil.RifleClimbMax` x), spread blooms on
     the hip `Spread.RifleBloom` per shot (max x `Spread.BloomMax`, recovers `Spread.BloomRecovery` per s); aimed shots
     bloom a quarter as much.
   - Shotgun: damage falloff sharper past its range: full to `Shotgun.FalloffStart` x range, down to
     `Shotgun.FalloffFloor` at `Shotgun.FalloffEnd` x range (say what today's falloff is in your report).
   - Revolver: crit multiplier `Crit.Revolver` 1.75 (the user's call 2026-10-21) through `FBulletShot::CritMultiplier`;
     a cursed iron's own multiplier wins when larger; stat cards that print the crit show 1.75x.
   - Handling per kind: time to aim x `Aim.Time.<Kind>`, move speed while aiming x `Aim.Move.<Kind>`, swap time x
     `Swap.<Kind>`.

Cvars (defaults; find today's values and report them): `Looter.Gunfeel.Kick.Rifle` 1.6 · `.Kick.Shotgun` 1.5 ·
`.Kick.Revolver` 1.4 · `.Recoil.Rifle` 1.4 · `.Recoil.Shotgun` 1.5 · `.Recoil.Revolver` 1.4 · `.Recoil.ShotgunSettle`
0.8 (snappiness x) · `.Muzzle.Scale1P` 1.4 · `.Rumble.Scale` 1.0 · `.Rumble.Rifle` 0.35 / `.Rumble.RifleSeconds` 0.06 ·
`.Rumble.Shotgun` 0.9 / 0.18 · `.Rumble.Revolver` 0.7 / 0.12 · `.Rumble.Hurt` 0.6 · `.LowAmmo.Share` 0.25 ·
`.LowAmmo.PitchTop` 1.35 · `.Recoil.RifleClimb` 0.06 · `.Recoil.RifleClimbMax` 1.8 · `.Spread.RifleBloom` 0.15 ·
`.Spread.BloomMax` 2.0 · `.Spread.BloomRecovery` 3.0 · `.Shotgun.FalloffStart` 0.6 · `.Shotgun.FalloffEnd` 1.6 ·
`.Shotgun.FalloffFloor` 0.25 · `.Crit.Revolver` 1.75 · `.Aim.Time.Rifle` 1.0 · `.Aim.Time.Shotgun` 1.1 ·
`.Aim.Time.Revolver` 0.8 · `.Aim.Move.Rifle` 1.0 · `.Aim.Move.Shotgun` 0.9 · `.Aim.Move.Revolver` 1.1 · `.Swap.Rifle` 1.0
· `.Swap.Shotgun` 1.1 · `.Swap.Revolver` 0.8.
Pipeline section: "Gunfeel: kick and handling".

## GUNFEEL-SOUND (Opus, starts after the pack is downloaded)
Owns: `Weapons/GunshotSound.cpp` (except `PlayLowAmmo`'s rule lines), `Art/Sounds/recipes/guns*.py`, `Art/Sounds/cues.json`
(gun entries), `Audio/LooterSoundCues.h` (gun and impact cues), `Art/Sounds/packs/` (processing scripts and the
licence; the raw pack stays outside git), new `Tests/GunfeelSoundTests.cpp`.

Source (extracted 2026-10-21 by Main): `Saved/SoundPacks/Sonniss/2016|2017/<Vendor - Library>/*.wav` (59 files, list
in `Saved/Gunfeel/SONNISS_FILES.txt`; licences `Licensing.pdf` in each; current terms
https://sonniss.com/gdc-bundle-license/: games OK, edits OK, no attribution, no resale/redistribution as sounds, no AI
training). The files you actually use are copied (trimmed if long) into `Art/Sounds/packs/Sonniss/` (Git LFS, the
private repo = the team, which the licence allows) so a render works from a fresh clone; `Art/Sounds/packs/README.md`
lists each file's source and the licence; the rest stays in Saved.
What's there: bullpup/rifle shots near and far (Enfield L85 on-gun and 1 m, Tavor TAR-21 1 m and 20 m, FAMAS G2 1 m
and 55 m, HK416, G36C 1 m and 50 m behind, AKM, FN FAL), revolvers (S&W M29 .44 at 1.5-2 m, M66 .357 at 1 m and 50 m,
Gamemaster revolver shot), shotguns (TS Sound 12 gauge fire, slug, quick pump, shell load; SPAS-12/Saiga/USAS slugs near
and far; Audiobeast Remington 870 pump in a warehouse = an indoor tail), Audiobeast warehouse rifle/pistol (indoor
tails), Pole Position gun foley (G36C mag in/out, M29 cylinder open/close, Saiga cocking, TMP cocking/dry fire),
Gamemaster bullet impacts (body thump, concrete, metal), fly-by, punch body impacts, Stuart Duffield shell/bullet drops,
Pole Position Warfare whizzes/ricochets/cracks and a field tail.

Do:
1. A sample loader for recipes (`Art/Sounds/lib/sample.py`: read WAV, mono/stereo, resample to the pipeline's rate,
   find and slice single shots out of multi-shot takes by onset, trim, fade, normalize by loudness) and a `guns` recipe
   rework: each gun's layers built from recordings, then shaped in script (EQ, transient shaping, saturation, layering
   with today's synthesized body where it helps) so the three guns sound like ours, not stock: Bullpup rifle (tight,
   punchy, mid crack), Ranchhand pump shotgun (huge low boom, wide), Drover revolver (big .44 bark with a ring).
   Layers per kind (cue "Weapon.<Kind>.<Layer>", see `GunshotSound::CueFor`): Fire (the whole shot, kept for anyone
   who calls it), Mech, Low, TailOpen, TailTown, TailInside, Distant, LowAmmo; plus `Weapon.Revolver.CritRing` (a short
   bright ring for the shooter, 2D, felt as a reward).
2. `GunshotSound::PlayShot`: the shooter's own gun (locally controlled pawn) plays Fire's core (punch) + Mech + Low +
   the tail by surroundings; others play Fire + tail, and past `Sound.DistantFrom` cm from the listener the Distant
   layer instead. Surroundings by a cheap probe: up to 6 short traces (up, 4 sides, one slanted) from the shooter,
   cached per gun for 0.25 s, world static only (`LooterWorld::StaticGeometryParams`): a roof = Inside, walls on 2+
   sides within 8 m = Town, else Open. Cvars (`Looter.Gunfeel.Sound.*`): `Tail` 1.0 (volume), `Mech` 1.0, `Low` 1.0,
   `DistantFrom` 3000, `ProbeRange` 800. Keep the definition's own FireSound override and the low-ammo rule lines.
3. Impacts: bullet hits on creatures (by `ECreatureBlood`) and on surfaces (`ImpactSurface` cues) get a recorded layer
   where it improves them (body thump/wet for flesh and gel, concrete for stone, metal heavy for sheet metal); the
   ricochet and fly-by cues too. Keep the cue names that exist; only their sounds change, plus new layer cues if needed.
4. Render with `Tools\sounds.ps1 Weapon.*,Impact.*,<fly-by cue>` and read the check (peaks, clicks, loudness) before you
   report; mix levels against the existing cues so guns are the loudest thing in a fight but not clipping.
Do not run `build_sound_bank.py` or anything in the editor: Main imports the bank, builds and checks in PIE.
Cvars, tests (`Looter.Gunfeel.Sound.*`: the surroundings rule as plain numbers, the layer choice), Pipeline section
"Gunfeel: gun sounds" (also update "Sounds": recorded gun/impact sources are now allowed, the licence, where the files
live), `Art/Sounds/README.md` (the same rule change).
The user's rule (2026-10-21): recorded royalty-free gun and impact sounds allowed, processed through the script
pipeline; everything else stays script-made. Licence: no resale as sounds, no AI training.
Pipeline section: "Gunfeel: gun sounds".
