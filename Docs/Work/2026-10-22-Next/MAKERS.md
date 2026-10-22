# Gun makers (MANUFACTURERS job, 2026-10-22)

The user picked gun manufacturers (2026-10-22) so every drop's name and feel promises something, Borderlands-like but
original and in the Revenant world. Concept sheet: `Saved/ArtPreviews/Weapons/Concepts/Makers.png` (generator:
the job's scratchpad `makers_sheet.js`, HTML beside the PNG). **The user approved the four exactly as shown
(2026-10-22).** Status: phase 1 code in `Saved/MakersCode` (holding tree, `HASHES.txt`; outside-file patches and the
phase 2 order in `Saved/MakersCode/ForMain/README.md`). Mark atlas made (`Art/Textures/GunMakerMarks`). Nothing built
or imported yet.

Story.md's "Who makes the guns" line now names these four (it replaced "Coachman and Breacher shotguns, Regulator
rifles, Homestead farm guns, Kestrel carbines"). "Coachman" stays a word for the
Deacon's shotgun model (Last Rites); "Homestead" stays a Drover frame part.

## The four makers

| Key (saved, never rename) | Name / full name, where | Fiction | Quirk (name: numbers) | Look |
|---|---|---|---|---|
| `Roundhouse` | ROUNDHOUSE / Roundhouse Rail Works, Halfway Junction | The railroad's own shops in the Junction roundhouse. Built to be thrown off a flatcar and still fire. | Rolling Stock: magazine +30%, recoil -25%, handling -15%, reload time +15% | Freight-car oxblood shell (#7d2f24/#8e3a2a), black fittings and metal (#2c2d30/#2a2b2e), brass trim (#d9a441); cream stencilled drive wheel (#efe3c4) |
| `SaintMaud` | SAINT MAUD / Saint Maud's Foundry, Furnace Hollow | The smith-sisters of Saint Maud's mission cast every barrel at her forge and bless it. Slow to make, slow to fire. | Blessed Iron: damage +10%, fire rate -12%, reload time +10%; element parts x3 as likely | Ivory enamel (#e8e0cc), cast-iron fittings and metal (#3a3633/#34302c), gilt brass (#f6dfa0); gilt anvil under a halo (#9a6a1c; was #c9962f, faint on ivory) |
| `Windfall` | WINDFALL / Windfall Salvage, Crossroads Town (Skyreach) | Sky salvagers bolt guns together from whatever the Gravewind drops. Fast, loose and full of odd bits. | Loose Fit: fire rate +15%, reload time -20%, accuracy -20%, damage -5%; effect parts x2 as likely; wear +0.25 | Salvage shells, one per gun (sea teal #4f8f8a, primer rust #9a5534, sky blue #7d9fb5, hazard yellow #d8a832), tape/grey fittings, bare steel (#6c6e70), copper trim; yellow hook-in-the-wind (#e6b43a) |
| `Meridian` | MERIDIAN / Meridian Instrument Co., Highwater Landing | They made the surveyors' transits that laid every mile of track. Since the gang came: rifles that land where the glass says. | True Glass: accuracy +30%, range +20%, handling +10%, fire rate -10%, magazine -20% | Deep blued navy (#26354a/#2b3c55), walnut-dark fittings (#4a3a2e), nickel metal (#a9aeb3), brass (#c99a4a); silver transit reticle with a north star (#dfe6ea) |

## Rules
- **Roll**: `RollWeaponWithRarity` picks the maker from the gun's seed (own stream, salt `0x4D414B52`) before its parts:
  weight = maker's rarity weight x its kind lean. Rarity weights (C U R E L): Roundhouse 4 3 2 2 2, Windfall 3 3 2 1.5
  1.5, Saint Maud 1 2 3 3 3, Meridian 1 2 2.5 3 3 (shares at C: 44/33/11/11%, at E/L: 21/16/32/32%). Kind leans
  (1 unless named): Roundhouse LMG 1.5, Launcher 1.5, Rifle 1.2, Revolver 0.6; Saint Maud Shotgun 1.5, Revolver 1.5,
  Sniper 1.2, SMG 0.6, Launcher 0.8; Windfall SMG 1.8, Shotgun 1.3, Sniper 0.5; Meridian Sniper 2, Revolver 1.5, Rifle
  1.2, Shotgun 0.7, SMG 0.6, LMG 0.4, Launcher 0.4.
- **Quirk**: percentages on the finished stats after parts, outside the parts' caps (so a maker always shows), in
  `UWeaponRollLibrary::ComputeInstanceStats` beside notches and curses (`WeaponMakers::ApplyToStats`). Accuracy divides
  the spread. Magazine: rounded, at least 1; never on kinds whose rounds show (Revolver cylinder, Launcher rocket).
- **Part bias**: in `WeaponParts::Pick`, an option's weight x the maker's ElementPartBias when it carries an element,
  x EffectPartBias when it carries an effect. Every slot still takes one draw. Saved parts never change.
- **Paint**: after the definition's paints, the maker's paints (slot -> weighted colours) override `Look.Colors` from
  a stream of their own (the definition's draws don't move). Slots: GunPolymerSand (shell), GunPolymerGrey
  (fittings), GunBlackMetal + GunSteelBlued (metal), GunBrass (trim). The model component already paints any slot in
  `Look.Colors` (Color + Tint params): no material copies beyond one MID per painted slot. Rarity glow untouched.
- **Mark**: a mask atlas `T_GunMakerMarks` (2x2 cells, from `Art/Icons/MakerMarks.py`, same outlines as the inked UI
  marks), painted by M_Gun from custom primitive data 12-19 on the first part (slot order) with a mark place:
  12 cell (0 none, 1-4), 13-14 mark centre X/Z (cm, part space), 15 size (cm), 16 depth, 17-19 colour (linear). Projected along the part's
  local Y (both sides, faces with |normal.y| > 0.5), scuffed by wear. Mark places per family (setup_gun_parts.py
  `MARKS`, a part option's `MarkPlace`): Bullpup body side, Drover grip panel (the notch tally cuts over it), others
  body side. The numbers are first guesses from the Blender outlines: tune by eye in PIE.
- **Names**: a gun no part names takes the maker's word ("Meridian Longhorn"); part words win ("Scoped Bullpup").
  The floating label shows the maker's mark (Inked, white) and name (caps, dim text) over the gun's name while looked
  at; the loot card and the loadout card show the mark, "MERIDIAN · TRUE GLASS" and the trade's numbers (Inspect adds
  the full name, place and fiction): `UI/Style/WeaponMakerRows`.
- **Saved by name**: `FWeaponInstanceData::Maker` (FName). None in memory = no maker (tests, dev guns made by hand):
  no quirk, stock look. Loading a save with no maker on a rolled gun (`PostSerialize`, `Ar.IsLoading()`, a Definition,
  not Named) sets the default `Roundhouse`: old guns turn oxblood and take Rolling Stock. A key that no longer exists
  reads as no maker.
- **Named guns** (Heirloom): no maker (quirk, paint, mark, word) whatever the field holds.

## Tuning (cvars, live)
`Looter.Makers.<Key>.<Stat>` (Damage, Accuracy, Range, FireRate, Reload, Recoil, Handling, Magazine, in percent;
Wear, ElementPartBias, EffectPartBias), `Looter.Makers.<Key>.Odds` ("4,3,2,2,2"), `Looter.Makers.<Key>.Kinds`
("LMG=1.5,Launcher=1.5"), `Looter.Makers.QuirkScale` [1], `Looter.Makers.Default` [Roundhouse].
Dev: `Looter.GiveWeapon Rifle Rare 5 Maker=Meridian` (needs Main's line in WeaponDevCommands), `Looter.Makers.Dump
[kind] [rarity]` (odds and a sample of 1000 rolls).

## Files
Mine (holding tree): `Affixes/WeaponMakers.h/.cpp` (table, rules, roll, look), `Affixes/WeaponMakerTuning.h/.cpp`
(cvars), `Affixes/WeaponMakerCommands.cpp` (Dump), `Affixes/WeaponRollLibrary.h/.cpp` (roll + stats),
`Weapons/WeaponTypes.h` (Maker field, PostSerialize), `Weapons/WeaponParts.h/.cpp` (bias, paints, MarkPlace, name
word, wear), `UI/Style/WeaponText.h` (MakerString, MakerColor, MakerMark, MakerFullLine, MakerFiction, MakerQuirkLine),
`UI/Style/WeaponMakerRows`, `UI/World/WeaponLabelWidget`, `UI/HUD/PlayerHUDWidgetPickupCard.cpp`,
`Tests/WeaponMakersTests.cpp` (`Looter.Weapons.Makers.*`). In place: `Tools/Unreal/setup_gun_parts.py` (MARKS),
`Tools/Unreal/create_manufacturers.py` (imports T_GunMakerMarks; checks), `Art/Icons/MakerMarks.py`.
Hooks for Main (outside my files; patches in `Saved/MakersCode/ForMain`): M_Gun mark nodes in
`build_world_materials.py` build_gun (GROUND-DETAIL owns the file), the CPD call in `WeaponModelComponent::Assemble`,
the marks joining `InkedIcons.py` (SHIELD also edits it), the loadout card's maker rows, `Maker=` in WeaponDevCommands.
Not done: vendor and bench rows show no maker line (their names carry the maker's word when no part names the gun).
