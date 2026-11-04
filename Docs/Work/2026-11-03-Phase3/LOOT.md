# LOOT (Phase 3 lane, change 5): rarity that plays differently; every drop seen

Not built, not run (rules: no build/editor/PIE/commit). Code written against the files read; Main reruns the parts script,
builds once and runs the tests below.

## Files
Edited (all in the lane's list):
- `Art/Models/Weapons/*.parts.csv` (all 7): Weight column only, 56 rows. Effect rows 0.5/0.6 -> 3 (22 rows), element rows
  0.35 -> 1.5 (34 rows). No key, slot, rarity or stat touched. Powderkeg `Incendiary` (Rare, Ember, weight 1) left at 1 (the
  rule named the 0.35 rows).
- `Weapons/WeaponParts.h/.cpp`: the tier slot in `Pick` (below); `TierSlotFrom` (Rare), `TierSlotRarity`, `TierSlot`; cvar
  `Looter.Loot.Parts.TierSlot`. `Effect` doc notes the new weights.
- `Weapons/LegendaryRules.h/.cpp`: `SignatureOf` gives a rolled (not named) Legendary a signature by seed; new
  `RolledSignature(Seed, bHeldFire)`; cvar `Looter.Legendary.Rolled`. Card line, tracer tint, silk, Scrip, lantern all
  follow (everything already asks `SignatureOf`: GunCardRules, LegendarySubsystem, LegendaryEffectComponent).
- `Weapons/LegendaryEffectComponent.h`: two comments only. `LegendarySubsystem.*`: no change needed.
- `Loot/LootDropMoments.h/.cpp`: Common beam and soft landing sting; `FDropMoment::StingerVolume/StingerPitch`; flat-from rule;
  4 cvars.
- `Loot/LootBeamCrowd.h/.cpp`: `LootBeamCrowd::ShowsBeam(Beam)`; `Refresh` counts only guns whose beam shows.
- `Loot/LootFanfareSubsystem.cpp`: `IsDropCueFlat` reads the moment (`bStingerFlat`); `Land` plays the moment's stinger
  with its volume/pitch, 2D when flat.
- Tests: `Tests/DropMomentTests.cpp`, `Tests/RoundThreeFixTests.cpp`, `Tests/LegendaryWeaponTests.cpp` (472 lines).

New (CODEMAP one-liner):
- `Tests/WeaponPartsTierTests.cpp`: Looter.Weapons.Parts.TierSlot (the tier slot on made-up guns: own tier, Legendary
  falls back to Epic, fit hand-on, saved guns, switch off) and .EffectOdds (effect/element odds over 2000 seeds on the 7
  guns). New file because `WeaponPartsTests.cpp` is 522 lines.

## The rules (before -> after)
- Tier slot (fresh Rare+ roll only): before, every slot drew among all parts with MinRarity <= rarity. After, one slot,
  chosen by the seed (own stream, salt 0x71E25107; each slot that offers one equally likely), draws by its usual roll among
  its parts of the tier and up. Tier = the gun's rarity, else the best below it any slot offers, down to Rare (no Bullpup
  part is Legendary: a Legendary Bullpup's slot rolls an Epic). A slot whose tier parts don't fit (Rustler 60-round helix
  needs barrel >= 13) hands the rule to the next slot. Saved guns: unchanged (saved path never forced; a lost part re-picks
  by the plain draw). Determinism per seed kept; the slots after the tier slot draw exactly as before (same rolls).
- Weights: effect parts 0.5 (Rare) / 0.6 (Epic) -> 3; element parts 0.35 -> 1.5.
- Rolled Legendary: before none (only DA_Named_Gravesilk/CompanyScrip/Lamplighter). After one of Gravesilk / Company
  Scrip / Lamplighter by seed (`HashCombine(Seed, 0x5160A7)` stream), each equally likely; Company Scrip only when the
  definition is `FireMode == FullAuto` (rifle, SMG, LMG): on a pump, six-gun, bolt or launcher its burst never builds and a
  20% refund of 1-6 rounds is 0-1, so those roll Gravesilk or Lamplighter 50/50. Named guns keep theirs (Heirloom keeps
  none). Save format untouched (derived from Named, Rarity, Seed, Definition->FireMode).
- Common drop: beam 0 -> 250 cm, glow 1.6 (Uncommon 2), radius 5 (Uncommon 6), in the Common rarity colour; landing sting
  none -> `Loot.Drop.Uncommon` x0.5 volume, x0.8 pitch, 3D from where it fell (no new cue; the toss's `Loot.Land` knock
  still plays). No flare, spin or ring.
- Stingers flat (2D, for the local player): Epic+ -> Uncommon+ (Uncommon -22.5 dB, Rare -21 dB now heard at any range;
  were 3D on the Creature attenuation). Common's stays 3D.
- Beam crowd: guns counted for "a gun lies within 300 cm" only if `ShowsBeam` (visible flag, not hidden in game). Before:
  every pickup gun, so ammo/silver near a beamless Common went dark.

## Odds (Monte Carlo of `Pick`, 200,000 rolls each, no maker; before = old weights, no tier slot -> after)
| Gun | Rarity | Effect | Element | Effect or element |
|---|---|---|---|---|
| Bullpup (rifle) | Rare | 6.6 -> 33.4 | 22.6 -> 46.3 | 27.8 -> 63.8 |
| | Epic | 28.3 -> 82.6 | 16.8 -> 32.3 | 41.1 -> 89.8 |
| Ranchhand (shotgun) | Rare | 0 -> 0 | 23.5 -> 47.3 | 23.5 -> 47.3 |
| | Epic | 18.6 -> 69.4 | 17.3 -> 34.2 | 33.5 -> 82.7 |
| Drover (revolver) | Rare | 6.6 -> 36.0 | 25.9 -> 47.8 | 30.8 -> 65.9 |
| | Epic | 29.2 -> 81.6 | 19.4 -> 30.9 | 45.1 -> 91.1 |
| Rustler (SMG) | Rare | 7.7 -> 37.9 | 34.4 -> 57.7 | 39.5 -> 73.2 |
| | Epic | 20.9 -> 70.4 | 31.6 -> 61.5 | 46.0 -> 88.0 |
| Longhorn (sniper) | Rare | 6.6 -> 34.8 | 20.0 -> 43.2 | 25.3 -> 62.6 |
| | Epic | 22.9 -> 75.2 | 16.9 -> 33.8 | 36.7 -> 86.4 |
| Thresher (LMG) | Rare | 0 -> 0 | 22.6 -> 46.3 | 22.6 -> 46.3 |
| | Epic | 13.8 -> 54.8 | 18.5 -> 35.6 | 30.9 -> 76.5 |
| Powderkeg (launcher) | Rare | 0 -> 0 | 32.4 -> 56.8 | 32.4 -> 56.8 |
| | Epic | 0 -> 0 | 28.7 -> 44.6 | 28.7 -> 44.6 |
Uncommon (no tier slot): element 26-43% -> 60-76%. Legendary: always a signature; effect as Epic (rifle 82.7) except where
the tier slot lands on Legendary parts, which carry no effect (Rustler 56.1, Thresher 27.6). A part of its tier: Rare
80-93% / Epic 63-76% before -> 100% after. Makers lean further (Saint Maud elements x3, Windfall effects x2).
The sim reproduces the audit's "before" (Rare Bullpup 6.7%, Epic 28%). Script: node, scratch only (not in the repo).
- vs the contract's targets (Rare ~40% effect, Epic ~70%): Rare 33-38% where a gun HAS a Rare effect part; Epic 55-83%.
- Data gap (a user/Main call, not this lane: MinRarity column): Ranchhand, Thresher and Powderkeg have no Rare effect part
  (0% at Rare), and Powderkeg has no effect part at all. A Rare effect row on each (e.g. Ranchhand `BlastingTube6`, Thresher
  `BoreThrough` MinRarity Rare) would bring them to ~35%.

## Cvars (live)
- `Looter.Loot.Parts.TierSlot` [1] (0 = the old picks; new rolls only, saved guns keep their parts).
- `Looter.Legendary.Rolled` [1] (0 = rolled Legendaries have none; nothing saved, so it switches every one at once).
- `Looter.Loot.Drop.Beam.Common` [250 cm] (0 = no Common beam, as before; read when a gun becomes loot).
- `Looter.Loot.Drop.Common.Volume` [0.5] (0 = only the knock), `Looter.Loot.Drop.Common.Pitch` [0.8].
- `Looter.Loot.Drop.Stinger.FlatFrom` [1 = Uncommon] (3 = Epic and up, the old rule).
- Common glow 1.6 / radius 5 sit in `FDropTuning` (LootDropMoments.cpp) beside the other tiers' fixed glow/radius.
- The CSV weights are data: edit the sheets and rerun the script.

## Editor script Main must rerun (before the tests)
`Tools\console.ps1 "py C:\Dev\AI_Looter_Shooter\Tools\Unreal\setup_gun_parts.py"` (editor open, after the build). It
rewrites the 7 definitions' Parts/Paints/base range-recoil-handling-zoom from the sheets (same as every parts change);
log lines `GUNPARTS DA_...`. Without it the DAs keep 0.5/0.6/0.35 and `Looter.Weapons.Parts.EffectOdds` fails.

## Tests (own)
- `Tools\runtests.ps1 -Filter Looter.Weapons.Parts` (new .TierSlot, .EffectOdds; old .Picks .Saved .Ranges .Caps .Assembly
  .Wear .Stats should still pass).
- `-Filter Looter.Loot.DropMoments` (.Rules re-fit: Common 250 cm, soft sting, flat from Uncommon; new .Common: a Common
  rifle in a test level shows a 2.5 m beam, lands with no flare/spin/ring, cvar 0 = dark; .NamePop, .Legendary unchanged).
- `-Filter Looter.Loot.BeamCrowd` (+ `ShowsBeam`: visible, hidden in game, switched off, null).
- `-Filter Looter.Weapons.Legendary` (new .Rolled: 3000 seeds rifle a third each, pump half Gravesilk/half Lamplighter and
  never Scrip, same every time, card line shown, Epic none, named kept, cvar 0 none; old four unchanged).

## Other files' tests this changes (not in the lane: Main's call)
- `Tests/FeedbackTests.cpp:383` `TestFalse("A Rare is heard from where it fell", IsDropCueFlat(Rare))` now fails by design:
  make it `TestTrue` ("a Rare is heard flat, 2026-11-03"). Line 372 (`DropCue(Common)` none) still holds: `DropCue` is
  unchanged (wards use it); the gun's landing now plays `FDropMoment::Stinger`.
- `Tests/WeaponMakersTests.cpp:395-398` (Looter.Weapons.Makers.Bias, Windfall): wants Windfall's Epic rifles with an effect
  > 1.3x plain; plain is now 82.6%, Windfall 93.4% (x1.13: the share saturates). Compare odds as the Saint Maud check
  does: odds 4.7 -> 14.2 (x3.0); e.g. `WindfallOdds * 2 > PlainOdds * 3`.
- Random-seed Rare rolls in other tests now carry an effect far more often: a Rare rifle has Triplet (Burst trigger)
  ~1 in 3 (was ~1 in 15). Watch `WeaponTriggerTests` (RollGun Rare rifle held-trigger checks), `WeaponFXTests` (Brass:
  "one case a shot"), `GunIdeasPlayTests` (Rare rifle seed 1234 with a random maker). Fix by rolling Common or setting
  parts, if they flake.
- Header comments not in the lane, now stale: `Loot/LootFanfareSubsystem.h` (`DropCue`: "none for a Common";
  `IsDropCueFlat`: "an Epic's and a Legendary's"; class doc "only the landing's knock for a Common").
- `Shields/ShieldPickup.cpp` reads `LootDropMoments::For`: Common wards now show the 250 cm beam too (as "a ward reads like
  a gun of its rarity"); its landing sting is still `DropCue` + `PlayAt` (3D) for every rarity.
- Every Common gun lying as loot shows the beam: kill drops, chests, gun racks (`AWeaponRack`), lost-loot ground guns.
- Pipeline (Main merges): Gunplay.md "Loot" Parts line (element weight 0.35, effect 0.6/0.5 -> 1.5 / 3, the tier slot),
  "Drop moments" (beams + Common 250; stinger Common soft 3D, Uncommon+ flat), and the Legendary section (rolled
  Legendaries' signature by seed).

## PIE check for the user
`Looter.GiveWeapon Rifle Rare 5` three times: at least one card has an effect or element row (Burst, Cinder...).
`Looter.GiveWeapon Rifle Legendary 5`: its card has a signature line (silk roots, Scrip pay, or a lantern on scoped kills).
`Looter.Feedback.Drop Common`: a short 2.5 m beam over it and a soft glint as it lands; `Looter.Feedback.Drop Rare`: the
chime plays at the same loudness whichever way you face.
