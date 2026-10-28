# Gun pattern variety: contract (Main, 2026-10-27)

User ask: "On each AR part we just made, different texture/color/stains/designs that are simplistic but break up
monotony and add more variations. Including the elemental differences." User calls: rarer = fancier; subtle, worn-in
(patterns sit under the dirt like real field paint, stylized realism). Applies to every gun (all parts go through M_Gun).

## Data: M_Gun custom primitive data 29-35 (0 wear, 1-11 notches/soul, 12-19 maker mark, 20-28 element are taken)
- 29 PatternKind (float, integer): 0 none, 1 Grime streaks, 2 Worn edges+chips, 3 Stripes (1-3 bands), 4 Chevrons,
  5 Camo blotches, 6 Stencil (number/letter-ish blocky marks, script-made shapes, no fonts), 7 Clean bold (two-tone split
  with a pinstripe).
- 30 PatternScale (cm per repeat, 4-30).
- 31 PatternSeed (0-1: offsets/rotation inside the material's hash).
- 32-34 PatternTint RGB (linear).
- 35 PatternAmount (0-1; "subtle" means typically 0.25-0.6).
- The pattern lies in the part's own space (PreSkinnedPosition, as the tally and mark do), triplanar on the local normal,
  so Tripo atlas UVs never chop it. Parts are separate meshes: patterns needn't line up across parts.
- Painted (maker paint) surfaces take the full pattern; bare metal/rubber/glass/glow slots take only grime/wear
  (material decides by the slot's paint mask or by metalness: agent picks a robust rule and documents it). Glow slots
  (Gun*Glow, GunReticle) never take a pattern.
- Order: base texture -> maker paint -> PATTERN -> wear/scuffs -> maker mark -> tally -> element finish.

## Rarity rule (C++ picks, deterministic from FWeaponInstanceData Seed + rarity + maker, like WeaponParts::Wear; no save change)
- Common: 1 or 2 (amount 0.3-0.5).
- Uncommon: 1, 2 or 3.
- Rare: 3, 4, 5 or 6.
- Epic: 4, 5, 6 or 7 (cleaner: less grime).
- Legendary: 7 or a maker-specific bold design (amount 0.6+).
- Tints from a small per-maker palette (darker or lighter shade of the maker paint, an accent colour, black, bone).
- Named guns may set their own later (leave a hook, no data change now).
- Cvar Looter.Weapons.Pattern 0/1 (1 default), Looter.Weapons.Pattern.Force <kind> for checking.

## Elements: every part shows its element lightly, the element part strongly
- Today the element finish (20-28) is only on the element part. Extend: every other part gets the same finish kind at a
  low amount (about 0.25 of the element part's) as stains: Ember soot streaks and scorch at edges/vents, Soul-light thin
  gold veins/gilt edge wear, Grave-salt frost rime in crevices. Material: stain masks driven by the existing kind and the
  pattern seed. C++: ShowElementFinish with an amount scale for non-element parts.

## Ownership (disjoint files)
- MAT agent: Tools/Unreal/build_world_materials.py (M_Gun and its *_CODE blocks only) + rebuilding M_Gun in the open
  editor through its builder (Tools\console.ps1 "py <abs path>" as the file's docstring says). Renders its own check
  shots of a gun with Force kinds (single grabs, no Python capture batches).
- CODE agent: new Source/AI_Looter_Shooter/Weapons/WeaponModelPattern.cpp (+ declarations in WeaponModelComponent.h),
  WeaponModelElements.cpp (spread to all parts), WeaponParts.h/.cpp only if the pick lives there, a test file
  Source/AI_Looter_Shooter/Tests/WeaponPatternTests.cpp (Looter.Weapons.Pattern.*: rarity rule, determinism, every part
  gets data, glow slots excluded, element spread), CODEMAP.md lines. Doesn't build (Main builds).
- Main: builds, imports AR sets 2-4, runs tests, commits.
