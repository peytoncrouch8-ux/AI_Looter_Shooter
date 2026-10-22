# TRAITS + LOOK merge, staged (2026-10-20 11:45, ward bar 11:52; not copied in, not built)

The final version of every file that TIER-TRAITS (`Saved/Tiers/Traits`) and TIER-LOOK (`Saved/Tiers/Look`) add or change,
merged onto TIER-CORE's working copy in `Source/` as of 11:44-11:52: `Merged/Source/AI_Looter_Shooter/<same path>` (48
files), plus `Merged/Tools/Unreal/build_tier_material.py`. Each file's base, CORE's hash when it was read, and the result
are in `manifest.tsv`. `stage.sh` rebuilds the whole stage, or only the files you name (`bash Saved/Tiers/Merged/stage.sh
<path> ...`). It reads Source and never writes it. Bases: `Traits/patches/base/` (saved 11:44, equal to the hashes the TRAITS
README gives) and `Look/patches/base/` (11:00). Neither tree touches the other's files.

## Files

- **TRAITS, new (16):** `Creatures/CreatureTraitComponent{Pace,Ward,Rally,Burst,Phase}.cpp`, `CreatureRallied.h/.cpp`,
  `CreatureTraitRules.h/.cpp`, `CreatureTraitTuning.h`, `CreatureTraitSettings.h/.cpp`, `Dev/TraitDevCommands.cpp`,
  `Tests/CreatureTraitTestWorld.h`, `Tests/CreatureTraitTests.cpp`, `Tests/CreatureTraitPlayTests.cpp`.
- **TRAITS, replaces CORE's stand-ins (3):** `Creatures/CreatureTrait.h`, `CreatureTraitComponent.h/.cpp`.
- **TRAITS, merged (3; CORE's base hadn't changed: TRAITS' version as is):** `Combat/HealthComponent.h/.cpp` (`Shield`),
  `Creatures/CreatureBase.h` (`SettleBody`).
- **LOOK, new (9 + script):** `Creatures/CreatureTierLook.h/.cpp`, `CreatureTierLookComponent.h/.cpp`,
  `CreatureTierFXSubsystem.h/.cpp`, `CreatureTierFXSubsystemDraw.cpp`, `Tests/CreatureTierLookTests.cpp`,
  `Tests/TierTagTests.cpp`; `Tools/Unreal/build_tier_material.py`.
- **LOOK, merged (16; every base unchanged: LOOK's version as is):** `Creatures/CreatureBaseHurt.cpp`, `CreatureBaseRank.cpp`,
  `CreaturePackComponent.cpp`, `CreatureReactComponent.cpp`, `DeadMinerCreature.cpp`, `DeadMinerCreatureLamp.cpp`,
  `UnpaidCreature.cpp`, `Embers/Powers/EmberFX.cpp`, `UI/World/CreatureHealthBarWidget.h/.cpp`, `XpPopupActor.h/.cpp`,
  `XpPopupSubsystem.h/.cpp`, `XpPopupWidget.h/.cpp`.
- **Ward bar layer (1 new + 5 edited, below):** new `UI/World/CreatureHealthBarWidgetWard.cpp`; edits to
  `CreatureHealthBarWidget.h/.cpp`, `CreatureTierLookComponent.h/.cpp`, `Tests/TierTagTests.cpp`.
- No new file already exists in Source; no test class or test name clashes (checked across Source and the stage).

**Re-merge if CORE changes them again:** the 19 "merge" files (3-way: CORE now, the base, the tree's version) and the 3 stand-ins.
If CORE adds API to a stand-in, TRAITS' component must take it on. Today CORE's code uses only `SetTraits`, `RollTraits`,
`GetTraits` and `StaticEnum<ECreatureTrait>`, and all of them match. `stage.sh` re-merges automatically: rerun it right
before copying in. Then compare each `core_hash_read` with the file now; check every "3-way clean" result by eye; resolve
every "CONFLICTS=n" (markers are labelled Source/base/TRAITS or LOOK).

## LOOK against TRAITS' real API (checked; nothing to fix)

LOOK uses `CreatureTraitRules::Word` (FText), `CreatureTraits::All`, `UCreatureTraitComponent::GetTraits`, `IsSwelling`,
`GetGlowScale`, and CORE's `ACreatureBase::GetTraitComponent`: every name and signature matches. Each `LooterRanks::`,
`LooterUI::Color::Tier*`, `UCreatureRankSettings::`, `GlowColor` and `LooterSoundCue::Tier::` name LOOK uses exists in CORE's
working copy. How they play together:
- LOOK's shell is attached to the body mesh, so Grave-burst's mesh swell scales it too.
- Phasing's blink and Grave-burst's blast hide the actor, so the shell hides with it.
- LOOK holds its top-tier death show while `IsSwelling()` is true, and plays it at the blast.
- Neither tree puts a lasting look in the overlay slot.
- LOOK's tier `GetCoalColor` tints Grave-burst's Unpaid burst.

## The ward bar (Main's ask, 11:52): a layer on top of the stage

- **The look:** a slim bar (4 px; the health bar is 6) 2 px over the health bar's frame. It leans with the health bar: it
  takes the frame's own shear (`FrameBox`), so the two never drift apart. Back to front:
  - a dark track (`Color::Track` at 0.65, `MarkBackground`)
  - a paler-cyan chip
  - the fill in the kit's pale cyan (`Color::CyanText`)
  - a white flash
  - the rim
  It is collapsed unless a ward stands, so other tags look as before.
- **Behaviour:**
  - A hit snaps the fill down. The chip holds 0.25 s, then drains at 1.2 bar lengths a second.
  - Breaking: the whole bar flashes a white of its cyan, fades over 0.35 s while it swells to x1.15, then goes.
  - A ward whole again (a new life) shows full at once.
  - A creature that isn't Warded, or whose traits are taken away, has no bar and no flash.
- **Files:**
  - `UI/World/CreatureHealthBarWidget.h`: `SetWard`, `AdvanceWard`, getters, `WardBreakFlashAt`, `WardFillColor`/`WardChipColor` and the constants.
  - `UI/World/CreatureHealthBarWidget.cpp`: the bar built in `RebuildWidget`; `AdvanceWard` in the tick.
  - New `UI/World/CreatureHealthBarWidgetWard.cpp`: the ward logic. Keeps the widget's .cpp at 438 lines.
  - `Creatures/CreatureTierLookComponent.h/.cpp`: `DressTag` feeds `SetWard(HasTrait(Warded), GetWardFraction())` every
    frame for any creature.
  - `Tests/TierTagTests.cpp`: `Looter.UI.Tier.Tag` checks the bar alone and dressed from a Warded spider (drained by a
    hit with its health untouched; the bar equals `GetWardFraction()`; gone once it isn't Warded).
- **Layer:** `ward/base/<f>` is the staged file before the edit; `ward/mine/<f>` is after it. `stage.sh` re-applies it
  3-way after every re-stage, and the rows show as `ward layer` in `manifest.tsv`. A LOOK or CORE change to these files
  merges under it.

## Run order (when Main says; CORE's build green first)

1. `bash Saved/Tiers/Merged/stage.sh`, then read `manifest.tsv` and fix any CONFLICTS or CLASH.
2. Copy in: `cp -r Saved/Tiers/Merged/Source/AI_Looter_Shooter/. Source/AI_Looter_Shooter/` and
   `cp Saved/Tiers/Merged/Tools/Unreal/build_tier_material.py Tools/Unreal/`.
3. CODEMAP lines (below).
4. `Tools\launch.ps1 -Build`; fix own code. Compile risks are in both trees' READMEs.
5. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_tier_material.py"` (expect "TIERMAT done";
   `M_CreatureTier`).
6. `Tools\runtests.ps1`, all of it, minding `Looter.Creatures.Traits.*`, `Looter.Creatures.TierLook.*`, `Looter.UI.Tier.*`,
   `Looter.Tiers.*`, `Looter.Creatures.RankFlash`, `Looter.UI.XpPopup`, `Looter.Creatures.React`, `Looter.Embers.*`.
7. PIE, every trait: `Looter.SpawnCreature Spider Gravebound`, then `Looter.Traits.Give <Trait>` for each of the 7 (also
   Slime + Phasing, Unpaid + Phasing for the shroud, Spitter + Rallying); check every tell, sound and counterplay
   (`Looter.Traits.Show`).
8. PIE, every tier's look: `Looter.SpawnCreature Spider|Slime|Unpaid|DeadMiner <Regular..Ethereal>`. Check the eyes, core,
   coal and lamp, the rim, the tag frame and trait line, the notice sting, the death show (also after a Grave-burst), the
   loot pop and the XP colour. The ward bar on a Warded creature: leaning with the health bar, draining, flashing as it
   breaks, back full after a respawn; `Tools\hudshots.ps1` doesn't cover tags, so take a screenshot.
9. Perf: `Tools\perf.ps1 -Label "Ethereal horde" -Exec "Looter.Quality Medium"` with `Looter.Perf.Horde Spider 8 Ethereal`
   (traits on: wards, rally rings, blinks), then kill them for the death shows; target 8.3 ms. End on Medium.
10. Report to Main. Never commit or change git state.

## CODEMAP lines (Creatures section unless said; replace CORE's stand-in line if any)

TRAITS:
- `Creatures/CreatureTrait.h`: `ECreatureTrait` (Fleet, Mending, Warded, GraveBurst, Rallying, Vengeful, Phasing), `CreatureTraits::All`.
- `Creatures/CreatureTraitComponent`: `UCreatureTraitComponent` ("Traits" on every creature; the tier's traits, set as its rank is applied): each trait's play, tells and looks, a new life afresh, `RollTraits`, look getters (`GetWardFraction`, `GetGlowScale`, `IsSwelling`). `CreatureTraitComponentPace.cpp` Fleet, Vengeful; `...Ward.cpp` Warded (the health's `Shield`), Mending; `...Rally.cpp` Rallying; `...Burst.cpp` Grave-burst; `...Phase.cpp` Phasing.
- `Creatures/CreatureTraitRules`: `CreatureTraitRules` (kinds and fits, roll, ward and mend sums, burst falloff, blink spots), `ECreatureTraitKind`, `TraitCue::WardHit`.
- `Creatures/CreatureTraitTuning.h`: `F<Trait>TraitTuning`. `Creatures/CreatureTraitSettings`: `UCreatureTraitSettings` (Project Settings > Game > Creature Traits).
- `Creatures/CreatureRallied`: `UCreatureRallied`, a rallied creature (less damage, faster) while any rallier holds it.
- `Dev/TraitDevCommands.cpp`: `Looter.Traits.Give <Trait...|none>`, `Looter.Traits.Show`.
- `Tests/CreatureTraitTests.cpp`, `Tests/CreatureTraitPlayTests.cpp`, `Tests/CreatureTraitTestWorld.h`: `Looter.Creatures.Traits.*`.
- Edit `Combat/HealthComponent`: add "`Shield` (a ward in front of health)". Edit `Creatures/CreatureBase`: add "`SettleBody`".

LOOK:
- `Creatures/CreatureTierLook`: `CreatureTierLook`, `ECreatureTierBody`, `FCreatureTierShell`, `FCreatureTagTier`, `FCreatureTierDeathShow`: how the tiers look and sound, as plain rules (glow from Restless, traits line from Gravebound, the top tiers' frame, sting, death show, loot pop and coloured XP; the Ethereal's ghost-rim; colours from the rank settings).
- `Creatures/CreatureTierLookComponent`: `UCreatureTierLookComponent`, made on a creature's first tier above Regular (`Refresh` from `ApplyRank`): its tier shell (the body again on a leader-pose follower in `M_CreatureTier`: eyes, core, ghost-rim; skipped by the hit flash and ember tints, `IsTierShell`), its flare (rank sting, death, traits' `GetGlowScale`, `SetHeldFlare`) and fade, an Unpaid's coal scaled by its traits, a top tier's death show (after a Grave-burst's swell), `DressTag` (frame, trait line).
- `Creatures/CreatureTierFXSubsystem`: `UCreatureTierFXSubsystem`, the tiers' shared shell looks (one per kind and tier) and a top tier's death (`PlayDeath`: its cue, a soul-light burst in its colour, rings, a soul-light pillar from the loot beams' glow, motes, the loot's pop and cue; no lights; at most 6). `CreatureTierFXSubsystemDraw.cpp` frame by frame.
- `UI/World/CreatureHealthBarWidget` (add): tier frame and clamps on Hallowed/Ethereal, trait line, the top tiers' bigger notice pop and frame flare (`SetTier`, `SetTraitLine`); a Warded creature's ward bar over its health (`SetWard`; `CreatureHealthBarWidgetWard.cpp`: its fill, chip and break flash).
- `UI/World/XpPopupActor`, `XpPopupSubsystem`, `XpPopupWidget` (add): a Hallowed or Ethereal kill's "+N XP" in its tier's colour, a size up.
- Tests: `CreatureTierLookTests.cpp` (`Looter.Creatures.TierLook.*`), `TierTagTests.cpp` (`Looter.UI.Tier.*`).
- Tools: `Tools/Unreal/build_tier_material.py`: `M_CreatureTier`.
