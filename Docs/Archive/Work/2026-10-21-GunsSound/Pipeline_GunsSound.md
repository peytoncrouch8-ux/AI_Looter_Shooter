## Loot: new guns' sounds (GUNS-SOUND, written 2026-10-21 night, unbuilt and untested in play)

Code (holding tree `Saved/GunsSound/Source/...`, Main merges): `GunshotSound.cpp` KindWord/CoreCue for SMG, Sniper, LMG,
Launcher; `GunshotSound::PlayExplosion(WorldContext, Location)` (near `Weapon.Launcher.Explosion`, past
`Looter.Gunfeel.Sound.ExplosionDistantFrom` [6000 cm] `Weapon.Launcher.Explosion.Distant`; rule `IsExplosionDistant`);
`WeaponSounds::Fire` for the four kinds (Equip/Holster/DryFire stay the long guns' shared ones); every new cue named in
`LooterSoundCues.h` (`Gunshot::<Kind><Layer>`, `SMG::`, `Sniper::`, `LMG::`, `Launcher::`, `Element::`, `DropLegendaryLand`).
Sounds: `recipes/guns_smg.py`, `guns_sniper.py`, `guns_lmg.py`, `guns_launcher.py` (each adds its kind to
`guns_shots.KINDS`/`MIX`, registers through `guns_shots._register`, imported at the end of guns_shots.py), `elements.py`
(imported at the end of loot.py until `elements` joins `recipes/__init__.py` GROUPS), `loot.py` `Loot.Drop.Legendary.Land`.
Excerpts: 34 new files in `packs/Sonniss/` (~27 MB), table in `packs/README.md`.
- Per kind: Fire, Core, Mech, Low, TailOpen, TailTown, TailInside, Distant, LowAmmo (Core/Fire 6 takes, Launcher 4; layers 4;
  Distant 6, Launcher 4). Extras: SMG HelixOut/HelixIn; Sniper BoltOpen/BoltClose/BoltCycle (~0.6 s); LMG SpinUp (0.35 s,
  clack at 0.345), BoxOut/BoxIn/BeltLay; Launcher RocketOut/RocketIn, Flight (1.2 s loop), Explosion (5), Explosion.Distant
  (4). Elements: Element.<Ember|SoulLight|GraveSalt>.<Apply|Tick|Kill> (4/5/3 takes). Rare/Epic/Legendary stingers kept.
- Levels vs the rifle's Fire (measured L200 at cues.json volumes): SMG -1.6, Sniper +2.0, LMG -0.9 (lows-heavy: plays big),
  Launcher +0.6; own-gun layer sums within 0.2-0.4 dB of Fire; own-gun peaks: SMG -0.4, Sniper 0.0 (inside tail, worst),
  LMG -1.4, Launcher -2.2 dBFS. The mix's shift stays -0.8 dB (World.ShutterSlam still sets it): no old cue's volume moved.
- Lessons: a short, gated core (SMG) measures quiet over 200 ms and asks for a high volume: densify it (master clip 7.5 dB)
  rather than raise the set; a lows-heavy Fire (LMG, sniper) measures quiet for its peak, keep its level ~1 dB lower; a
  gun-mounted mic hears the bolt return ~50 ms on (MP7): gate the front at 30 ms; a reversed approaching whoosh makes the
  rocket's departure but must go on after the drive or the master holds it into a drone.
- Didn't check: how any of it sounds (can't listen), the tails' concurrency at 900 rpm (Core 8, tails 6), the spin-up's
  timing against GUNS-CODE's ramp.
