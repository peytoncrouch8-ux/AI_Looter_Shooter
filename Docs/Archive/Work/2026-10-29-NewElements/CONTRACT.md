# Two new elements: Thunder and Rot (2026-10-29)

The user's picks (from elements.png, four candidates): **Thunder** (candidate A "Shock", glyph 1 Bolt) and **Rot**
(candidate B "Blight", glyph 2 Skull). They join Cinder (code Ember), Hallow (code SoulLight) and Wardbreak (code GraveSalt).
Code names: `ELooterElement::Thunder`, `ELooterElement::Rot` (after GraveSalt; never reorder). Player words: THUNDER, ROT.

## What they do (ElementRules, FElementStatus; Main builds these)
- **Thunder, a jolt**: a hit that sets it deals a burst of `ThunderShare` (0.35) of the hit `ThunderDelay` (0.2 s) later,
  staggers the creature `ThunderStaggerSeconds` (0.5; never a boss, never a Soulfed) and arcs to the nearest *living* creature
  (not of the dead: `!ElementRules::IsTheDead`) within `ThunderChainRadius` (350 cm) for `ThunderChainShare` (0.5) of the
  burst (damage only, no status: no chain of chains). Up to `ThunderMaxPending` (4) jolts wait on one creature; past that the
  newest joins the last. Weak: Gel, Other. Resists: Unpaid, Undead. Neutral: Chitin.
  Card rows: "Jolt damage" = hit x ThunderShare, "Chance to jolt". Line: "Thunder: jolts them, and the next living thing".
- **Rot, a rot**: damage over time, `RotShare` (0.5) of the hit over `RotSeconds` (6 s), its rate growing (half the mean
  rate at the start, one and a half at the end; the total exact). One rot at a time: a new hit adds its total to what is left
  and sets the time back to RotSeconds. When a rotting creature dies (by anything), the rot spreads: every living creature
  within `RotSpreadRadius` (300 cm) is set rotting as by a hit of `RotSpreadShare` (0.6) of the strongest hit that rotted the
  dead one (their own weakness applies; a spread never spreads again from the same death). Weak: Chitin, Other.
  Resists: Gel. Neutral: the dead. No slow.
  Card rows: "Rot dmg/sec" = hit x RotShare / RotSeconds, "Chance to rot". Line: "Rot: eats at them, and spreads from the dying".
- Chance to set: the gun kind's, as the other three (`GunChance`).
- Cvars `Looter.Element.Thunder.*`, `Looter.Element.Rot.*` for every number above (FElementTuning fields of the same names).

## Colours and glyphs
- UI: `LooterUI::Color::ElementThunder()` #9FB4FF (159,180,255), `ElementRot()` #A8E05A (168,224,90).
- FX hues (`ElementRules::FxColor`): Thunder (0.55, 0.65, 1.0) blue-white; Rot (0.55, 0.95, 0.25) sickly green.
- Gun card glyphs: Bolt and Skull from elements.json (candidate A option 1, candidate B option 2), painted icons like the rest.
- Gun finishes (`UWeaponModelComponent::ElementFinishFor`): Thunder wears the frosted pattern (FinishFrosted) in a cold
  blue-steel tint with the Thunder glow ("Charged"); Rot the scorched pattern (FinishScorched) in a mossy green tint with a
  dim green glow ("Rotten"). No new material kinds this round.

## Status API the looks code against (FElementStatus / UElementStatusComponent; Main adds them)
- `int32 NumJolts() const`, `float GetJoltCharge(const FElementTuning&) const` (0 just set .. 1 bursting, the soonest).
- `bool IsRotting() const`, `float GetRotTimeLeft() const`, `float GetRotGrowth() const` (0 at the start .. 1 at the end).
- `FElementTick` gains `bool bJolt` (a jolt's burst, with its leap) beside `bSmite`.
- Component: `float ThunderLookOwed`, `float RotLookOwed`; `void PlayJoltLook(const FVector& Where, const FVector* LeapTo) const`;
  `void PlayRotSpreadLook(const FVector& From, TConstArrayView<FVector> To) const`.
- `UElementStatusComponent::NumElements` = 6; `LastTickCue[6]`.

## Lane S: sounds (Art/Sounds/recipes/elements.py; Tools\sounds.ps1)
Six cues, synthesized like the three families there (Apply / Tick / Kill each, 3D at the creature, `att='Creature'`):
- `Element.Thunder.Apply` (4): the jolt striking: a sharp electric crack (a dense buzz of fast clicks 1-4 kHz with a bright
  snap), a short hum under it, a zap tail; `Element.Thunder.Tick` (4): the burst and its arc: a crackle-snap that whips
  (pitch falling fast), a brief buzz; `Element.Thunder.Kill` (3): a big crack, a long sizzling buzz dying out, a low thud.
- `Element.Rot.Apply` (4): a wet hiss and bubbling (gurgling noise, soft pops in the low mids), a faint sick drone;
  `Element.Rot.Tick` (5): a few wet pops and a short hiss (quiet, heard often: conc 6, level -16); `Element.Rot.Kill` (3):
  the body collapsing: a wet burst, a gush of bubbling, a cloud's hiss fading.
Keep the 2-5 kHz band low as the file's docstring says. Register with `@cue` (same groups/levels pattern as Ember's: Apply
level -11, Tick -16, Kill -8). Render only these: `powershell -NoProfile -File Tools\sounds.ps1 Element.Thunder.*,Element.Rot.*`
(it writes Art/Sounds/Out and cues.json). Main imports them into Unreal.

## Lane P: gun parts (Art/Models/Weapons/<Gun>.parts.csv and <Gun>.py)
For every gun with element magazines (Bullpup, Drover, Longhorn, Powderkeg, Ranchhand, Rustler, Thresher): two rows beside the
Salt row, same slot, rarity Uncommon, weight 0.35, NamePriority 38, the same stat deltas, Element `Thunder` / `Rot`, Model = the
gun's *plain* magazine of that size (the key the Ember row's Model names; for the Ranchhand the plain tube, never SaltTube6).
Keys (never reuse a key): Bullpup Thunder30/Rot30, Drover Thunder6/Rot6, Longhorn Thunder5/Rot5, Powderkeg Thunderclap/
Rotburst, Ranchhand ThunderTube6/RotTube6, Rustler Thunder40/Rot40, Thresher Thunder100/Rot100. Names like the Salt row's
("30-round thunder rounds", "Thunder cylinder", "5 thunder rounds", "Thunder rocket", "6 thunder shells", "40-round thunder
helix", "100-round thunder box"; "rot" alike). Word (the gun's name prefix): `Charged` for Thunder, `Blighted` for Rot.
Keep the .py part lists in step with the CSVs (find how the CSV is written first). Check Tests/ for part counts that change.

## Lane F: looks (after the core builds; Main says when)
Files: Weapons/WeaponFXElements.cpp (SpawnElementHit / SpawnElementAura / SpawnElementBurst cases; SpawnElementArc for Thunder
jagged and blue-white), Elements/ElementDeathLook.cpp, Elements/ElementStatusComponentLooks.cpp (PlayLooks for a jolt waiting and
a rot; PlayJoltLook; PlayRotSpreadLook), Weapons/WeaponModelElements.cpp (the finishes above + cvars ThunderGlow, RotGlow).
Thunder: blue-white sparks that jump jagged, arcs crawling over the body while a jolt waits (fast flicker glow), the burst a
hard flash with sparks flung out, the arc to the next a jagged line of beads; the death a flash and sparks raining.
Rot: green gobs and a sick puff on the hit; drips falling off the body and slow bubbles while it rots, a dim green glow;
the kill a wet green burst and a spreading cloud; the spread a cloud rolling to each target.
Everything through FWeaponFX's pooled particles (no lights, no Niagara), as the three elements do.

## Rules for every lane
Agents never build, commit, open/close the editor, run tests, or edit CODEMAP.md / Docs. Only your own files. Keep every
public name other code or tests use. Files under ~500 lines. Comments say why. Tabs. Report: files changed, what you could
not do, any test you changed.
