# Round 4 contract (Main, 2026-10-27): melee polish, boss cutscenes, swimming, grenade types

The user picked all four (2026-10-27): melee polish (per-tool hit sounds and impact effects, steam on Gauge's piston, the
tools in third person), boss cutscenes (short in-engine intros for the Gravemother, Cyrus Teague and Abel), swimming and
underwater (swim in deep water: underwater tint, muffled sound, bubbles, a swim stroke), grenade types (a few kinds found as
loot). The Unreal Engine logo: the user downloads it into `Art/UI/Logos/UnrealEngineLogo.png` (README there); Main imports.

Read first: `CLAUDE.md`, `CONTRACT.md` "Rules for every agent", `CONTRACT3.md` (round 3), the pipeline topic files for your
area, CODEMAP lines for your files. Holding trees: `Saved/<Job>4Code/`. Read sources from Source/ (rounds 1-3 merged).
Sounds script-made (Art/Sounds recipes, a new recipe file + GROUPS entry each). Original names, art and text only. Every
number a cvar; plain rules tested; the 8.3 ms budget.

## HANDS (Opus, same agent as round 3): melee polish
Owns its round 3 files (Player/PlayerMelee*, PlayerMeleeTool*, FirstPersonHands* melee/cast, Heroes/HeroDefinition melee
fields), `Player/Animation/LooterStancePose*` (the third-person tool), new melee FX code (`Weapons/WeaponFXMelee*` new file
only; read WeaponFX for the API), melee sound recipes. Per-tool hit sounds (the spade's ringing clang, the fan's slicing
snick, the gaff's hook bite, the piston's hiss-thump, the pick's chink) and swing whooshes; per-tool impact effects on
creatures and on the world (sparks off the spade and pick on stone, steam burst from the piston, card-edge glints from the
fan); the tool in the third-person hand during a swing (and on the belt or back at rest if cheap); DEMO's round 3 melee finds
(Main forwards them).

## SCENES (Opus): boss cutscenes
Owns: new `Scenes/BossIntro*`, new lines (write them in `Saved/SCENES4Code/lines.py` for Main to paste into
create_story_lines.py), the one-line hooks in `Creatures/GravemotherCreature*`, `Creatures/ForemanCreature*`,
`Bosses/AbelKeeper*` (only the call that starts the intro when the fight starts), sound recipes for stings. Read the scene
system first (Scenes/: cold open, Sit with Pa, SceneSubsystem, captions; Story/NewGameIntro for the newest camera-shot
style). Short (8-15 s), skippable, once per session per boss (a rematch from a vigil stone skips it): a camera move that
shows the arena and the boss's entrance move (the Gravemother bursting from the Sink's floor, Cyrus stepping out of the
mine's dark with his lamp, Abel at the lantern), a name card (name and a short epithet), one or two caption lines from the
story; the fight starts as it ends. Fair: the player can't be hit during it.

## SWIM (Opus): swimming and underwater
Owns: `Player/PlayerLocomotion*` (a swim mode), new `World/SwimWater*` (water bodies the player can swim in), new
`Tools/Unreal/build_area_swimwater.py` (places them from the layout's water features: ponds, the tarn, deep creek pools; read
Art/Levels/area_model.py build_water and layout.json features), the first-person hands swim stroke (FirstPersonHands swim pose
in a new file `Player/FirstPersonHandsSwim.cpp`), new `Player/UnderwaterView*` (post-process tint and fog, a caustic shimmer,
bubbles, an audio low-pass via a sound mix: script-made assets only), sound recipes (splash in/out, underwater ambience,
stroke). Deep enough water (more than chest deep) swims; shallow stays wading. Guns: no firing underwater (holstered or
lowered), a breath meter only if cheap and fair (no drowning damage unless it's fun; say what you chose). Creatures don't
follow into deep water (say how). Keep Main's M_Water two-sided underside. Hooks into files others own this round
(FirstPersonHandsComponent.cpp is HANDS's; the weapon manager; creatures): exact lines in your report, Main applies them.

## GRENADES (Opus): grenade types
Owns: `Combat/GraveSalt*`, `Player/PlayerThrow*`, new `Combat/GrenadeKinds*`, `Loot/GrenadePickup*`, the grenade row of
the vendor (`UI/Vendor/VendorWidgetRows.cpp` grenade row only), the HUD grenade counter (find it; the grenade part only),
`Art/Models/Throwables/` (jar variants), sound recipes. Three original grenade kinds beside today's grave-salt jar (fitting
the frontier-tech grave-salt world; e.g. a sticky tar jar, a cluster jar that splits, a cinder jar that burns an area),
each with its own look (jar variant/tint), explosion and sound; found as loot (a jar kind drops like grenades do; picking up
another kind swaps your carried kind and keeps the count, or say a better simple rule), the HUD shows the carried kind, the
vendor sells the base jar. Early-game strength follows today's ramp (10% to level 3). Tests.
