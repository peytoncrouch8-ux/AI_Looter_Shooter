# ROSTER plan (2026-10-21 night; the user's creature roster ask)

Roster itself: `Docs/Story.md` "Creature roster" (edited in place; Areas table marked †, five region lines added).
Code in the holding tree `Saved/RosterCode/` (mirrors the repo; no existing file edited, so nothing to diff against).

## Ransom's Rest placeholders (only where the story gives a reason)

| Id (spawner Placeholder_<Id>) | Kind (tag) | Count | Where | When | Story reason |
|---|---|---|---|---|---|
| CellarRats | Coffin Rat (rat) | 4 | Bright & Daughter's cellar: copies `Encounter_TillyCellar` (ground, roof band, level 3, spots, condition) | Knocking Below, the cellar step to its clear step | coffins waiting out their seven days, no keeper: rats at the dead |
| BaggageRats | Coffin Rat (rat) | 4 | depot baggage room: ring of 1.4 m round `Place_BaggageRoom`, `Encounter_DepotWaiting`'s ground and condition | The Old Fare, until its clear step | Fenn's coffin and the hearse freight bound for Gravesend |
| KeepersRoadHounds | Bone Hound (coyote) | 4 + 1 Restless | keeper's road west of the old lantern house, layout (2000, -10000), radius 11 m (moved 20 m in from the Rim after PIE: hounds fell off the lip) | after Main 4, before Main 6 | Story.md's own Bone Hounds: they raid boards no keeper sits up with; Abel keeps his after Main 6 |
| TramwayKnockers | Knocker (goblin) | 5 + 1 Restless | Larkspur Bench, the ore tramway's tipped cars, (17300, 3900), radius 12 m | after Dead Lift | mine folklore: knocked the night before the fall; clear of SpoilAmbush, Dinner Pail, Crest patrol (30+ m) |
| BootHillDiggers | Gravepicker (bandit) 4, Gravepicker Lookout (bandit rifleman) 2 | 6 | Boot Hill zone, (3200, 0) | after Main 3, before Main 4 (the roaming Unpaid take Boot Hill from Main 4) | living grave robbers after grave gold in unkept graves; sell to a polite buyer (Sexton seed) |

Not placed in the Rest (no honest reason): bats, golems, giants, scorpions, vampires, dragons, insects, wolves proper.
~31 extra kills, mostly optional (cellar/depot rats ride mission fights). XP: rats 3, knockers 5, hounds 10, diggers 12
at level 1 (kill rules on top): roughly +4% of the Rest's kill XP; re-check `Looter.Progression.Pacing` if it reads spawners.

## Files (holding tree -> repo, same paths)

- `Source/AI_Looter_Shooter/Creatures/PlaceholderCreatureDefinition.h/.cpp` (UPlaceholderCreatureDefinition, EPlaceholderRole, EPlaceholderVoice)
- `Source/AI_Looter_Shooter/Creatures/PlaceholderRules.h/.cpp` (pure rules: tag name, stretch, capsule, head, pose, aim, keep)
- `Source/AI_Looter_Shooter/Creatures/PlaceholderCreature.h/.cpp` (APlaceholderCreature: body, look, sounds, death, BakeDefinition)
- `Source/AI_Looter_Shooter/Creatures/PlaceholderCreatureMoves.cpp` (attacks by role, ranged keep-away, pellets)
- `Source/AI_Looter_Shooter/Dev/PlaceholderDevCommands.cpp` (`Looter.SpawnPlaceholder <Name> [rank] [count] [chase]`)
- `Source/AI_Looter_Shooter/Tests/PlaceholderCreatureTests.cpp` (`Looter.Creatures.Placeholder.Rules/.Body/.Shots/.Pages`)
- `Tools/Unreal/create_placeholder_creatures.py` (DA_Placeholder_*, BP_Placeholder_*, DA_Bestiary_*; default = PLACED kinds, `all` = every kind)
- `Tools/Unreal/build_area_placeholders.py` (PLACEMENTS above)

## Merge and run order

1. Copy `Saved/RosterCode/*` into the repo. Add one line to `Tools/Unreal/build_area.py` gameplay(), right after the
   `build_area_secrets` line:
   `importlib.reload(importlib.import_module('build_area_placeholders')).place(self)`
2. Build (`Tools\launch.ps1 -Build`).
3. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_placeholder_creatures.py"` (PLACEHOLDER lines,
   "PLACEHOLDER done: 5 kinds"). `all` only when the user wants every kind's page (it lists unmet silhouettes).
4. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_placeholders.py RansomsRest"` (after the valley
   props exist; or a full `build_area.py RansomsRest gameplay`). Check the log: 5 of 5 placed, no level warnings.
5. Tests: `Tools\runtests.ps1 -Filter Looter.Creatures.Placeholder` and `-Filter Looter.Bestiary.Entries`.
6. PIE: `Looter.SpawnPlaceholder GravepickerLookout Regular 2 chase`, `... BoneHound Restless 4 chase` (look, tells, tag,
   shots, death crumble, loot/XP). Then commit; add CODEMAP lines and the Pipeline section below.

## CODEMAP lines (add under Creatures / Dev / Tests / Tools)

- `Creatures/PlaceholderCreatureDefinition`: `UPlaceholderCreatureDefinition` (DA_Placeholder_<Name>: names, role, clay size/colour, numbers, shots), `EPlaceholderRole`, `EPlaceholderVoice`, `GetTagName` ("Name (type)").
- `Creatures/PlaceholderRules`: `PlaceholderRules` (tag name, `BodyStretch`, `CapsuleFor`, `HeadSpot`, `SquashScale`, `PoseAt`, `LeadAim`, `ChooseKeep`), `FPlaceholderPose`, `EPlaceholderKeep`.
- `Creatures/PlaceholderCreature`: `APlaceholderCreature`, a clay stand-in for an unmodelled kind (SK_Slime's dome stretched, M_StylizedSurface tinted, a head ball crit, two lumps), fights on the game's systems; `BakeDefinition` writes a kind into its BP's defaults. `PlaceholderCreatureMoves.cpp` attacks by role, ranged keep-away and pellets.
- `Dev/PlaceholderDevCommands.cpp`: `Looter.SpawnPlaceholder <Name> [rank] [count] [chase]`.
- `Tests/PlaceholderCreatureTests.cpp`: `Looter.Creatures.Placeholder.Rules`, `.Body`, `.Shots`, `.Pages`.
- `Tools/Unreal/create_placeholder_creatures.py`: the kinds (KINDS, ROLES presets, PLACED) -> DA_Placeholder_*, BP_Placeholder_*, DA_Bestiary_*.
- `Tools/Unreal/build_area_placeholders.py`: Ransom's Rest's placeholder encounters (PLACEMENTS), gameplay pass after the secrets.

## Pipeline section (paste as "## Placeholder creatures (ROSTER, written 2026-10-21, unbuilt)")

- Why: the user's roster ask (Story.md "Creature roster"): a named clay blob wherever the story wants a kind not modelled.
- A kind = `DA_Placeholder_<Name>` + data-only `BP_Placeholder_<Name>` (its own class: page, kill counts, groups) + `DA_Bestiary_<Name>`, all from `create_placeholder_creatures.py`. The BP's defaults hold the numbers (`BakeDefinition`); rerun the script after editing a definition (`.Pages` catches drift).
- Body: SK_Slime with M_StylizedSurface on every slot (MID, `Color` the kind's clay, `Variation` 0.25), mesh scale = size / dome (156 x 156 x 60 cm), capsule from the narrow side; head and lumps on the `body` bone, scale undoing the stretch. Pose on the `body` bone only (breath, bob, tell, strike, hover/dive, slump, sink, rise-in).
- Tag and page: "Name (plain type)" (the user, 2026-10-22). Hits: slime's wet hit; cries borrowed by Voice (Beast spider, Dead Unpaid, Clay slime); death = Gel death burst in clay colour + dirt.
- Pays like real enemies (the user, 2026-10-22): XPReward per kind through the kill rules, the default/rank loot tables, ranks/levels/traits untouched.
- Ranged: band BackOffWithin..CloseInTo (AttackRange = CloseInTo / 0.75), shoots within ShotRange with a clear world-static line, `ShowCharge` tell for the wind-up, pellets via `UEnemyProjectileSubsystem` (damage AttackDamage x share x fight scale), bites within 2.3 m.
- Replacing one: model the kind, make a real class, point its page's ActorClass at it, delete its DA/BP (spawner groups name the BP: update `build_area_placeholders.py`).

## Risks

- Unbuilt, untested: Python BP creation (`BlueprintFactory`, `bake_definition` on the CDO, then save) may need a compile or `modify` tweak; the head's bone-space placement assumes SK_Slime's `body` bone rest turn maps axes (tests check the head spot).
- Blood on bullet hits falls back to ichor and the melee cue to flesh (CreatureBlood.cpp / DeathBurstOf not edited, to stay out of other lanes); a one-line `ECreatureBlood::Gel` case for `APlaceholderCreature` would read better.
- Stretched convex hulls are fine, but a squashed head ball's sphere collision uses its smallest axis (small).
- Perf: 3 draws each (+ tier shell); the boot hill camp is 6.
- The Gravepickers make the Rest's first living human enemies: a tone call for the user (grave robbers, not townsfolk).
