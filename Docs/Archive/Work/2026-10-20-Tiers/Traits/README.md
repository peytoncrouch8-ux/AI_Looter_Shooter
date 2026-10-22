# TIER-TRAITS: the creature traits (holding tree, 2026-10-20; not built)

Contract: `Saved/Tiers/CONTRACT.md` (section TRAITS), design `Saved/Tiers/DESIGN.md` "Traits". Every file below sits under
`Saved/Tiers/Traits/Source/AI_Looter_Shooter/<same path as Source>`. Nothing under `Source/` was touched; nothing built.

## Files

New (copy whole):
- `Creatures/CreatureTrait.h`: `ECreatureTrait` (contract order) + `CreatureTraits::NumTraits`, `All[]`. **Replaces CORE's stand-in.**
- `Creatures/CreatureTraitComponent.h` + `.cpp` (life, events, shared measures): **replace CORE's stand-ins**; API exactly the
  contract's/stand-in's (`SetTraits(TConstArrayView)`, `HasTrait`, `GetTraits`, `DECLARE_MULTICAST_DELEGATE(FOnCreatureTraitsChanged)
  OnTraitsChanged`, `static RollTraits(Count, bEthereal, const ACreatureBase&, Seed)`), plus `Find`, `FindOrAdd`, `Advance`
  (tests), look getters, test getters.
- `Creatures/CreatureTraitComponentPace.cpp` (Fleet, Vengeful), `...Ward.cpp` (Warded, Mending), `...Rally.cpp`, `...Burst.cpp`
  (Grave-burst), `...Phase.cpp` (Phasing).
- `Creatures/CreatureRallied.h/.cpp`: `UCreatureRallied`, the buff a rallier puts on others (like `UBellRingerSteady`).
- `Creatures/CreatureTraitRules.h/.cpp`: `ECreatureTraitKind`, `CreatureTraitRules` (KindOf, Fits, Roll, Word, Name, Parse,
  AbsorbWard, MendAmount, BurstShare, IsHardHit, BlinkCandidates), `TraitCue::WardHit`.
- `Creatures/CreatureTraitTuning.h`: one USTRUCT per trait (`F<Trait>TraitTuning`). `Creatures/CreatureTraitSettings.h/.cpp`:
  `UCreatureTraitSettings` (Project Settings > Game > Creature Traits; DefaultGame.ini needs nothing, defaults are in code).
- `Dev/TraitDevCommands.cpp`: `Looter.Traits.Give <Trait...|none>` (nearest living non-boss creature within 50 m gets exactly
  these, until its rank is applied again), `Looter.Traits.Show`.
- `Tests/CreatureTraitTestWorld.h`, `Tests/CreatureTraitTests.cpp` (Roll, Ward, Mending, Fleet, Vengeful),
  `Tests/CreatureTraitPlayTests.cpp` (Rally, GraveBurst, Phasing, NewLife): `Looter.Creatures.Traits.<Name>`, all leaves.

Changed existing (the holding copies = CORE's working copy as of 2026-10-20 ~11:00 plus my delta; git hash-object of the
bases: CreatureBase.h `0ecb7e85`, HealthComponent.h `bc4fccad`, HealthComponent.cpp `c4a939a7`). If CORE's file differs from
that base at merge, re-apply only the delta:
- `Combat/HealthComponent.h`: after the three dynamic delegates,
  `DECLARE_DELEGATE_RetVal_ThreeParams(float, FHealthShield, float /*Damage*/, bool /*bCritical*/, const FVector& /*HitLocation*/);`
  and a public member `FHealthShield Shield;` (after `AfterDefense`, with its comment).
- `Combat/HealthComponent.cpp` `HandleAnyDamage`, inside `if (!bInvulnerable)`: `const float ToHealth = Shield.IsBound() ?
  FMath::Clamp(Shield.Execute(Damage, bCritical, HitLocation), 0.f, Damage) : Damage;` then subtract `ToHealth` (not
  `Damage`). Runs after CORE's defense; the number shown and `OnDamaged` keep the whole hit.
- `Creatures/CreatureBase.h` public, after `PlayEntranceIfSeen`: `void SettleBody() { OnPoseThawed(); }` (with comment).

## Each trait (numbers: `UCreatureTraitSettings`; colours sRGB, each trait its own)

All looks are code-drawn (FWeaponFX streaks/glows, `EmberFX::FEmberSparks`), never the overlay slot (hit flash and LOOK's
Ethereal ghost-rim live there); drawn only with a player within `LookRange` 90 m, standing looks only when the body isn't
pose-frozen. Widths >= 3 cm (Pipeline lesson: thinner vanish past 5 m).
- **Fleet** (#DCEAF4): on: walk and chase x1.25 (multiply/divide). Each frame: state turns Attack -> wind-up x0.85 from the
  first frame (saved; restored when Attack ends unless the kind set its own) and a 0.16 s glint (glow + cross) at its front
  (the quicker strike's tell). Running above 55% of chase: three wind streaks (flanks, back) trailing, length speed x0.16 s.
- **Mending** (#A6EA8C): any hit (ward hits too) zeroes the unhit clock. Unhit 3 s and hurt: heals 3% of max/s; on start a
  ring of 12 motes and the `Tier::TraitMend` loop (Start/Stop, quiet); while on, a mote every 0.09 s and a soft glow.
  A hit: motes flung away, loop cut (0.08 s). Whole again: loop eased off (0.4 s).
- **Warded** (#8CE6FF): ward = 30% of max health (kept as a share, so a level change scales it), on the health's `Shield`
  (after defense and scales). A hit: ward takes it, a crit counts x2 (on top of x1.5); a ripple ring (0.22 s) where it hit,
  cage flare, `TraitCue::WardHit` ping (>=0.07 s apart, pitch up as it wears). Breaking hit spills the rest to health; break:
  `Tier::TraitWardBreak`, 28 falling shards, 0.18 s flash, two rings out (0.35 s), next frame a 0.8 s `Daze` (the reward).
  Standing look: three turning rings (a squat oval cage), glow 5 x (0.4 + 0.6 x ward), under 35% flickering and dropping pieces.
  No regeneration (design has none).
- **Grave-burst** (#FF8A3D): on death (never a boss): swell 1 s: `Tier::TraitSwell` (the 1 s recipe), mesh grows to x1.3
  with a shiver, heart glow, 3 m ground ring pulsing 2 -> 10 Hz, motes drawn in. At 1 s: flash, soul-light burst (3 m) and its
  own kind's burst, dust ring, `Tier::TraitBurst`; players within 3 m (to the capsule's nearest side, line of sight, players
  only) take 1.5 x AttackDamage (full within 1.2 m, x0.6 at 3 m), `UCreatureAttackDamageType`, knocked out 650 / up 260 x share;
  views within 15 m jolt (kick + 0.35 s shake); the corpse is hidden (respawn unhides). A rank set on the corpse doesn't stop it.
- **Rallying** (#FFE09A): every 0.25 s finds living non-boss creatures (any kind, not itself) within 8 m flat / 3 m rise:
  `UCreatureRallied::Join` (damage-taken source "Trait.Rally" x0.85, walk/chase x1.1; several ralliers count once), leavers
  let go. In a fight (or while holding any): its 8 m ground ring (relaid when it moves 60 cm, at most every 0.3 s), tethers
  chest -> each held one with a flowing bead and a small foot ring. Fight start and every 6 s: `Tier::TraitRally` + a pulse
  ring out over 0.7 s (tethers brighten). Death: all let go, 7 sparks off each (tethers snap).
- **Vengeful** (#FF4A36): any hit (and each frame) checks health < 30%: enrage for good (healing doesn't calm it): damage
  and walk/chase x1.25 (damage re-applied if its stats are applied again), `Tier::TraitEnrage`, 22 embers out, 0.45 s red flare,
  then a beating heart glow and embers rising every 0.07 s; `GetGlowScale` x2 (x4 in the flare) for LOOK. Death or a new life
  calms it.
- **Phasing** (#FF3B3B, Ethereal's own): hits remembered 1 s; at >= 8% of max within 1 s (shotgun, grenade, crits, ~1 s of
  rifle), cooldown 4 s since the last blink's start, never mid-attack, knocked/dazed, in an Unpaid phase-step, passive or
  hidden: blink. Spot: 18 candidates aside of the hitter (90, then 65 and 115 degrees off its line; 6, 5, 4 m; one side then
  the other, sides alternate per blink): walkable ground within 1.2 m of its height, not on an `Obstacle`, room for the capsule,
  no wall between, the hitter's eye can see it, >= 2.5 m from the hitter, no safe zone, inside the playable area and its
  hunting ground; none: retry in 0.5 s. Blink: `Tier::TraitPhase` + soul-light burst + 14 sparks at the origin, hidden,
  teleported facing the hitter, streak origin -> destination and a 4-streak after-image (0.3 s); 0.25 s of gathering light at
  the destination (the tell); back: unhidden, `SettleBody()` (feet/shroud/springs reset), shimmer and the cue again (x0.6, pitch
  x1.12). Hard-hit share is 8% (design said "hit hard": chosen so focused fire blinks it about every 4 s, not every hit).

Life: `HandleDeath` ends rally/rage/mend/blink/wind-up (ward goes quietly); a new life (health reset after a death) restarts
every trait; `SetTraits` with new ones undoes all first; the same ones again is a no-op (CORE's `Looter.Tiers.Traits` needs it).

## Which kinds (CreatureTraitRules::Fits; ECreatureTraitKind by class, most specific first)

| Trait | Spider | Slime | Spitter | Unpaid | Dead miner | Bell-Ringer | Other |
|---|---|---|---|---|---|---|---|
| Fleet | yes | yes | no: keeps away | yes | yes | no: keeps behind | yes |
| Grave-burst | yes | yes | no: dies at range | yes | yes | no: dies at range | yes |
| Vengeful | yes | yes | yes | yes | yes | no: 6-damage support | yes |
| Mending, Warded, Rallying, Phasing | yes | yes | yes | yes | yes | yes | yes |

Phasing fits every kind (the Ethereal's mark; a slime phases too), so "Phasing always one of an Ethereal's" always holds.
Pools: Spitter 4 (+Phasing), Bell-Ringer 3 (+Phasing): an Ethereal always gets 3.

## Cues

The contract's 7 trait cues through `LooterSoundCue::Tier::Trait*` (CORE's block; SOUND made them, d1db327a: Mend is a loop).
**Asked for, not in the contract:** `Creature.Trait.WardHit` (a soft glassy ping per ward hit), `TraitCue::WardHit` in
CreatureTraitRules.h: silent (one "missing cue" warning) until SOUND makes it; then move it into the Tier block.

## For LOOK and CORE

- LOOK: the tag can draw the ward as a pale cyan bar over health (`GetWardFraction`), the trait line from
  `CreatureTraitRules::Word` (refresh on `OnTraitsChanged`), and multiply the tier glow by `GetGlowScale()`.
- CORE tests that hurt a tier creature by exact amounts with traits on will see Warded absorb and Vengeful raise
  `AttackDamage`/`ChaseSpeed` (Fleet changes `ChaseSpeed`/`WalkSpeed` from the start): clear traits first (`SetTraits({})`),
  as `Looter.Tiers.Defense` already does.

## Merge run order (when Main says)

1. CORE's build green. 2. Copy the new files whole (overwrite the 3 stand-ins). 3. Re-apply the 3 deltas onto CORE's
current HealthComponent.h/.cpp and CreatureBase.h (check the base hashes). 4. `Tools\launch.ps1 -Build`, fix. 5.
`Tools\runtests.ps1` (`Looter.Creatures.Traits.*`, `Looter.Tiers.*`, then all). 6. CODEMAP lines below. 7. PIE: a spider,
`Looter.Traits.Give Warded Mending` etc., each trait seen and heard; a horde perf check with Ethereals (Phasing + 2).

## Compile risks (written without a compiler)

- `BindUObject` on the private `AbsorbIntoWard` (signature must match `FHealthShield` exactly: `float(float, bool, const FVector&)`).
- `FindOrAdd` calls the protected `BeginPlay` on a new component (same class: legal); test worlds don't begin components
  registered late.
- Nested plain structs (`FWardRipple`, `FRecentHit`) and `TInlineAllocator` arrays in a UCLASS (UBulletSubsystem does the same).
- `TSet<ECreatureTrait>(const TArray&)` via the explicit TArrayView constructor (tests); `TArray<ECreatureTrait>(initializer_list)`.
- `UE_LOG` with `cond ? TEXT("none") : *FString::Join(...)`; `UE_ARRAY_COUNT` int compares (precedent in the code).
- `GetSkinnedAsset()`, `IsHidden()`, `GetPawnViewLocation()`, `UE_HALF_PI` checked in the 5.8 engine source.
- Uses CORE's uncommitted names: `LooterSoundCue::Tier::Trait*`, `ACreatureBase::GetTraitComponent` (tests don't need it).

## CODEMAP lines (Creatures section; replace CORE's stand-in line if any)

- `Creatures/CreatureTrait.h`: `ECreatureTrait` (Fleet, Mending, Warded, GraveBurst, Rallying, Vengeful, Phasing), `CreatureTraits::All`.
- `Creatures/CreatureTraitComponent`: `UCreatureTraitComponent` ("Traits" on every creature; the tier's traits, set as its rank is applied): each trait's play, tells and looks, a new life afresh, `RollTraits`, look getters (`GetWardFraction`, `GetGlowScale`). `CreatureTraitComponentPace.cpp` Fleet, Vengeful; `...Ward.cpp` Warded (the health's `Shield`), Mending; `...Rally.cpp` Rallying; `...Burst.cpp` Grave-burst; `...Phase.cpp` Phasing.
- `Creatures/CreatureTraitRules`: `CreatureTraitRules` (kinds and fits, roll, ward and mend sums, burst falloff, blink spots), `ECreatureTraitKind`, `TraitCue::WardHit`.
- `Creatures/CreatureTraitTuning.h`: `F<Trait>TraitTuning`. `Creatures/CreatureTraitSettings`: `UCreatureTraitSettings` (Project Settings > Game > Creature Traits).
- `Creatures/CreatureRallied`: `UCreatureRallied`, a rallied creature (less damage, faster) while any rallier holds it.
- `Dev/TraitDevCommands.cpp`: `Looter.Traits.Give <Trait...|none>`, `Looter.Traits.Show`.
- `Tests/CreatureTraitTests.cpp`, `Tests/CreatureTraitPlayTests.cpp`, `Tests/CreatureTraitTestWorld.h`: `Looter.Creatures.Traits.*`.
- Edit the `Combat/HealthComponent` line: add "`Shield` (a ward in front of health)". Edit `Creatures/CreatureBase`: add "`SettleBody`".
