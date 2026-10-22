# TIER-LOOK holding tree (2026-10-20)

How the enemy tiers look and sound (DESIGN.md "Readable and rewarding", CONTRACT.md section LOOK). Nothing here is built.
Every C++ file mirrors `Source/AI_Looter_Shooter/<same path>`; the material script mirrors `Tools/Unreal/`.

## Files

New (copy whole):
- `Creatures/CreatureTierLook.h/.cpp`: `CreatureTierLook`, the plain rules: tier tests (`HasBodyGlow` Restless+,
  `ShowsTraitLine` Gravebound+, `IsTopTier` Hallowed/Ethereal, `HasGhostRim` Ethereal), colours from the rank settings
  (`GlowColor`, `TagColor`), `BodyOf`/`ShellFor` (what each kind shows), `SpiderEyePatch`, `CoalGlow`, `LampColor`,
  `TagFor`, the cues (`NoticeCue`, `DeathCue`, `PopCue` = `LooterSoundCue::Tier::*`), `StingGivesWayTo`, `TraitWord`/
  `TraitLine` (TRAITS' `CreatureTraitRules::Word`), `TraitFlare` (TRAITS' `GetGlowScale`), `DeathShowFor`, `TintsKillXP`,
  flare/fade curves, the shell's custom primitive data (`FlareIndex` 0, `FadeIndex` 1, `ShellFlareGain` 2).
- `Creatures/CreatureTierLookComponent.h/.cpp`: `UCreatureTierLookComponent`, made by `Refresh` on a creature's first
  tier above Regular (never Regular, Legendary, boss): the tier shell, its flare/fade, the trait glow scale on the shell
  and on an Unpaid's coal, the deferred death show, `DressTag`, `IsTierShell`, `ShellTag` "TierShell", `SetHeldFlare`.
- `Creatures/CreatureTierFXSubsystem.h/.cpp` + `CreatureTierFXSubsystemDraw.cpp`: `UCreatureTierFXSubsystem` (tickable
  world subsystem): shared shell looks (one MID per kind+tier), top-tier death shows (burst, rings, pillars, motes, loot
  pop, sounds), pillar pool (MaxShows 6, 2 cylinders each), ticks only while a show plays.
- `Tools/Unreal/build_tier_material.py`: makes `/Game/Art/Materials/Masters/M_CreatureTier` (unlit translucent, no
  textures; masks: eyes by UV rect, core by vertex red + pre-skinned sphere, ghost-rim Fresnel with crawling shimmer;
  per creature Flare/Fade from the shell's custom primitive data 0/1; `ShroudFade` 1 only on Unpaid shells, since other
  models keep baked AO in vertex alpha).
- `Tests/CreatureTierLookTests.cpp`: `Looter.Creatures.TierLook.Rules`, `.Shell`, `.DeathShow`.
- `Tests/TierTagTests.cpp`: `Looter.UI.Tier.Tag`, `Looter.UI.Tier.XpPopup`.

Changed (whole new version here; `patches/<path>.diff` = my change only, against `patches/base/`, the Source file as it
stood when I copied it, CORE's in-progress edits included):
- `Creatures/CreatureBaseRank.cpp`: `ApplyRank` ends with `UCreatureTierLookComponent::Refresh(*this)` (after CORE's
  `ApplyTraits` and `OnRankChanged`).
- `Creatures/CreatureBaseHurt.cpp`: `UpdateHealthBar` calls `DressTag` before `SetRankStingAge`.
- `Creatures/CreaturePackComponent.cpp`: `TryRankSting` plays `Creature.Tier.Notice.<Tier>` over `Creature.RankSting`
  for Hallowed/Ethereal; a top tier's sting never gives way to a lower tier's (two top tiers together sting once).
- `Creatures/CreatureReactComponent.cpp`, `Embers/Powers/EmberFX.cpp`: the hit flash and `FEmberTint::Wear` skip tier
  shells (else the flash would draw twice).
- `Creatures/UnpaidCreature.cpp`: `GetCoalColor` uses `GlowColor`; `ApplyCoalColor`'s strength `CoalGlow` (Hallowed x1.25,
  Ethereal x1.6).
- `Creatures/DeadMinerCreatureLamp.cpp` (`BuildLampGlow`: flame and its light in `LampColor`), `DeadMinerCreature.cpp`
  (`OnRankChanged` rebuilds the lamp once play has begun: promotion, back to Regular on respawn).
- `UI/World/CreatureHealthBarWidget.h/.cpp`: `SetTier`, `SetTraitLine`, frame (`FramePad` 3, `FrameLine` 1.5,
  `ClampWidth` 4, `FrameFlareScale` 1.12), trait line (size 9, TextDim, spacing 90), top-tier pop (`TopTierFlashSeconds`
  0.9, `TopTierFlashPeakScale` 1.45, white-hot of own colour), `FrameFlareAt`. Regular pop numbers unchanged
  (`Looter.Creatures.RankFlash` still holds). CORE's word+name-in-tier-colour change kept.
- `UI/World/XpPopupWidget.h/.cpp`, `XpPopupActor.h/.cpp`, `XpPopupSubsystem.h/.cpp`: optional tier tint (default args,
  old calls unchanged); `TierSize` 22, `TierSeconds` 1.6, pop 1.6x, rise 58 px.

Overlaps: none of these files is in `Saved/Tiers/Traits/`. CORE had already edited `CreatureBaseRank.cpp` and
`CreatureHealthBarWidget.cpp` (included in my base); CORE may touch more of them before the merge: merge by diff.

## What it looks like

| Tier | Body | Tag | Notice | Death | Loot | XP |
|---|---|---|---|---|---|---|
| Regular | as today | as today | none | kind's own | as today | cyan |
| Restless | eyes/core glow 7 / 2.5, coal and lamp blue | word+name blue (CORE) | rank sting, eyes flare 0.8 | kind's own | - | cyan |
| Gravebound | glow 9 / 3.2, purple | + trait line | rank sting | kind's own | - | cyan |
| Hallowed | glow 11 / 4, gold, coal x1.25 | + gold frame and clamps | rank sting + Notice.Hallowed, word pops 1.45x 0.9 s white-hot, frame flares; eyes flare 1.6 | + soul-light burst x1.5 gold, ring 4.2 m, pillar 9 m r24 1.7 s, 24 motes, Death.Hallowed | pop at 0.15 s: flash, 18 glints, Pop.Hallowed | gold, 22 px |
| Ethereal | glow 13 / 4.8, red, coal x1.6, red ghost-rim shimmering up the body | + red frame | as Hallowed, Notice.Ethereal | burst x1.7 red, ring 5.4 m + second ring, pillar 13 m r30 2.1 s flickering, 36 motes, Death.Ethereal | pop at 0.18 s, 24 glints, Pop.Ethereal | red, 22 px |

By kind: spiders and the Spitter: eyes (UV patch, the sac stays venom green). Slime: core (inside parts within 15 cm of
(0,0,30) at rest). Unpaid, Bell-Ringer, dead miners: the coal in `GlowColor` (no shell below Ethereal); dead miner's lamp
flame and its budgeted light in the tier colour. Anything else: rim only at Ethereal. Traits: Vengeful/Grave-burst/
Mending's `GetGlowScale` brightens shell and coal; a Grave-burst's swell holds the glow bright and defers the death show
until the blast. Shell glow goes out over 0.85 s after death (flare 1.8 first). No lights added anywhere.

Why a shell, not the overlay slot: the hit flash and ember tints use the body's one overlay and stay on through steady fire
(0.4 s linger), which would hide the rim all fight; the spider's eyes share MI_SpiderBody (M_World: no emissive) with the
whole body, the slime's core shares FoliagePalette with the meadow. The shell = one leader-pose follower per tiered
creature, every slot in a shared MID, no collision/shadow/physics: one translucent draw.

## Cues used

`LooterSoundCue::Tier::NoticeHallowed/NoticeEthereal` (2D, over `Creature.RankSting`), `DeathHallowed/DeathEthereal` (3D
at the body), `LootPopHallowed/LootPopEthereal` (3D at the loot spot, 0.15/0.18 s after the death cue). SOUND: the notice
plays layered over the rank sting.

## Merge run order (after CORE, then TRAITS)

1. Copy the new files into Source (and `build_tier_material.py` into `Tools/Unreal/`).
2. Each changed file: `git merge-file -p Source/AI_Looter_Shooter/<f> Saved/Tiers/Look/patches/base/<f>
   Saved/Tiers/Look/Source/AI_Looter_Shooter/<f> > merged` (current, base, mine), check, put in place. If a file is
   unchanged since my base, just copy mine.
3. Build (`Tools\launch.ps1 -Build`).
4. `Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_tier_material.py"` (expect "TIERMAT done").
5. Tests: `Looter.Creatures.TierLook`, `Looter.UI.Tier`, plus `Looter.Creatures.RankFlash`, `Looter.UI.XpPopup`,
   `Looter.Creatures.React`, `Looter.Embers`, `Looter.Tiers`.
6. PIE: `Looter.SpawnCreature Spider Ethereal`, `Slime Hallowed`, `Unpaid Ethereal`, `DeadMiner Restless`; check the
   eyes glow (if the abdomen lights instead, the V flip is wrong: `SpiderEyePatch` -> V 0-0.125), the core, the rim, tag
   frame and trait line, notice, death show, XP colour.
7. Perf: `Tools\perf.ps1 -Label "Ethereal horde" -Exec "Looter.Quality Medium" ...` with `Looter.Perf.Horde Spider 8
   Ethereal`, and kill them for the shows.

## Compile risks

- Needs TRAITS' `CreatureTraitRules.h` (`Word`) and `UCreatureTraitComponent::IsSwelling()`, `GetGlowScale()`; CORE's
  `ACreatureBase::GetTraitComponent()`, `LooterRanks::TierIndex`, `GlowColor`, `LooterSoundCue::Tier::*`,
  `LooterUI::Color::TierHallowed()` (test only).
- `USkinnedMeshComponent::bUseBoundsFromLeaderPoseComponent`, `LeaderPoseComponent` (public in 5.8, checked),
  `SetLeaderPoseComponent(Body, true)`; `MakeUniqueObjectName(Creature, Class, TEXT(...))`.
- Range-for over `{ a, b }` initializer lists (bools, `UImage*`); `TConstArrayView` from `TArray` in tests.
- `UCreatureTierFXSubsystem` nests plain structs (as `ULevelUpBurstSubsystem` does); `TMap<FName, TObjectPtr<MID>>`.
- Material script: `MaterialExpressionPreSkinnedPosition` -> `VertexInterpolator` 'VS' (as M_Gun); falls back to a
  looser core mask with a warning if it can't wire.

## CODEMAP lines

- `Creatures/CreatureTierLook`: `CreatureTierLook`, `ECreatureTierBody`, `FCreatureTierShell`, `FCreatureTagTier`,
  `FCreatureTierDeathShow`: how the tiers look and sound, as plain rules (glow from Restless, traits line from Gravebound,
  the top tiers' frame, sting, death show, loot pop and coloured XP; the Ethereal's ghost-rim; colours from the rank settings).
- `Creatures/CreatureTierLookComponent`: `UCreatureTierLookComponent`, made on a creature's first tier above Regular
  (`Refresh` from `ApplyRank`): its tier shell (the body again on a leader-pose follower in `M_CreatureTier`: eyes, core,
  ghost-rim; skipped by the hit flash and ember tints, `IsTierShell`), its flare (rank sting, death, traits'
  `GetGlowScale`, `SetHeldFlare`) and fade, an Unpaid's coal scaled by its traits, a top tier's death show (after a
  Grave-burst's swell), `DressTag` (frame, trait line).
- `Creatures/CreatureTierFXSubsystem`: `UCreatureTierFXSubsystem`, the tiers' shared shell looks (one per kind and tier)
  and a top tier's death (`PlayDeath`: its cue, a soul-light burst in its colour, rings, a soul-light pillar from the
  loot beams' glow, motes, the loot's pop and cue; no lights; at most 6). `CreatureTierFXSubsystemDraw.cpp` frame by frame.
- `UI/World/CreatureHealthBarWidget` (add): tier frame and clamps on Hallowed/Ethereal, trait line, the top tiers'
  bigger notice pop and frame flare (`SetTier`, `SetTraitLine`).
- `UI/World/XpPopupActor`, ... (add): a Hallowed or Ethereal kill's "+N XP" in its tier's colour, a size up.
- Tests: `CreatureTierLookTests.cpp` (`Looter.Creatures.TierLook.*`), `TierTagTests.cpp` (`Looter.UI.Tier.*`).
- Tools: `Tools/Unreal/build_tier_material.py`: `M_CreatureTier`.

## Calls I made (for Main/the user to confirm or tune)

Notice cue layered over the rank sting (CORE's cue comment says "over"); trait line between name and bar; XP tint only
for the top tiers (the design's list); the dead miner's lamp doesn't take the traits' glow scale (its glow is private);
death show one frame after the death. All numbers are named constants in `CreatureTierLook.cpp`,
`CreatureTierFXSubsystemDraw.cpp` and the material's defaults.
