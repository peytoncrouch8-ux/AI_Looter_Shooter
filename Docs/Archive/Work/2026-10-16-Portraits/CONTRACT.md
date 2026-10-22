# The four heroes' portraits: the build contract (2026-10-16)

Why: the user picks each hero's look from painted bust portraits before any 3D model is built (`Docs/Pipeline.md`
"Character portraits"; Ellis's three were made this way and portrait A became SK_Ellis). Now the other four heroes of the
pick (`Docs/Story.md` "The heroes"; looks, stories and powers in `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`
"Five heroes"): **Odessa Lark** the Cardsharp, **Hollis Crane** the Unpaid, **Gauge** the Iron Hand, **Wendell Pike** the
Surveyor. Three options each (A, B, C), one sheet per hero, for the user's morning. Rules: `CLAUDE.md` (no copyrighted
likenesses or names; nothing from a film or another game).

## The machinery (shared; do not fork it)

- `Art/Portraits/bust.py`: the shared bust code pulled out of `EllisBlender.py` (same names and signatures): MakeHuman
  body from macro sliders + face modifiers (`build_human(opt)`, `opt["face"]`), eyes and lids, `drape` (cloth columns
  round an axis), `strip_on` (lapels, straps, ties projected onto clothes), `bvh_of`, hair curves (`curves_object`,
  `grow`, `grow_combed`), materials by `ID`, `neck_frame`, `shoulder_z`, `point_eyes_at`, `stage(h, opt, size,
  samples)` (world, camera `opt["cam"]`, lights `opt["lights"]`), `render`, `render_aux`, and `run(options, prefix,
  build)`. `EllisBlender.py` is the worked example of a hero script: read it whole before writing yours.
- A hero script `Art/Portraits/<Hero>.py` (`Odessa.py`, `Crane.py`, `Gauge.py`, `Pike.py`) holds: its `FACE` (MakeHuman
  modifiers), `OPTIONS = {"A": {...}, "B": {...}, "C": {...}}` with `label`, `yaw`, `pitch`, `lids`, `skin`, `glow`,
  `world`, `cam`, `lights` and its own keys, its builders, `build(opt)` returning what `run` needs (as Ellis's does),
  three `# PAINT <X> {...}` lines (background colours, `glowAt`, `sparks`, `dust`, `glow`) and `main()` calling
  `bust.run(OPTIONS, "<Hero>", build)`. Output names: `<Hero><X>_color.png`, `<Hero><X>_aux.png`.
- Run (Blender 4.4 here; no bpy module): `"C:\Program Files\Blender Foundation\Blender 4.4\blender.exe" -b --python
  Art/Portraits/<Hero>.py -- <A|B|C> --size 600 --samples 32 --no-aux --out <dir>` for looks (about 20 s), the final at
  `--size 1024 --samples 80` with the aux pass (Main renders the finals). Look at your renders (Read the PNG) and
  iterate until the bust reads at a glance as the description below; three to six looks per option is normal.
- Material ids (`bust.ID`; the aux pass's B channel = id / 16, so ids stay 1..15; `paint.js` gives the crisp render
  back by id: skin 1 and eye 2 most (0.66), bandana 5 and hair 9 some (0.28), brass 8 / lead 11 / leather 4 / linen 12
  (0.45), coat 6 and felt 3 least (0.10)): skin 1, eye 2, felt 3, leather 4, bandana 5 (any neckcloth, cravat, veil),
  coat 6, shirt 7, brass 8, hair 9, feather 10, lead 11 (iron, steel, tin), linen 12, black 13, **14 glass** (lenses,
  the headlamp, the gauge's face), **15 wood/bone** (card case, pipe, stock). Nothing above 15. Glow: whatever the glow
  mask marks blooms in the option's `glow` colour.
- **The Revenant mark, shared by all five:** Sexton raised every hero, so every hero's eyes burn the same cyan as
  Ellis's (`glow` (0.25, 0.85, 1.0)) somewhere in at least options A and B: Odessa's and Pike's eyes, Crane's socket,
  Gauge's headlamp. Option C may show the hero as they were in life (no glow) where the option says so.
- Framing as Ellis's: a bust from the chest up, three-quarter turn (`yaw` 15-35 degrees either way), the head about
  the same size in frame as Ellis's (same `cam` distance, 1.95-2.15), key light close and warm from one side, a cool
  cyan rim, the shoulders falling into dusk. Keep the picture square 1024.
- **Frames and sheets:** `Art/Portraits/frame/gen.js` grows per-hero frames (`Frame_<Hero>.svg`, Ellis's headboard
  and lantern motifs swapped for the hero's: the FRAME agent's work); `sheet.js` takes a title and labels from the hero
  script's `OPTIONS` (`label` plus a one-line `line` key you write per option: what the picture shows, 12-20 words).
  Add `"line"` to each option.

## The heroes (the cloud handoff's looks are the spec; each option is one reading of them)

**Odessa Lark, the Cardsharp** (1.72 m, slight; a riverboat gambler who caught Lucky Ned dealing seconds and caught his
bullet for it; Sexton bought her marker; her embers are Fortune and Risk: Lucky Streak, Fifty-Two Pickup, Double or
Nothing). Face: a woman about thirty, sharp and amused, high brows, a long neck, dark hair, olive skin gone grave-pale.
- **A The Dealer:** the flat-crowned black gambler hat with a playing card tucked in its band; the wine-red tailcoat,
  gold brocade waistcoat with a watch chain, a white high collar and black cravat, gold earrings; one black-gloved hand
  holding a card up beside the face (its face toward us, blank-backed or a plain pip: no real deck's art); a half
  smile; eyes cyan under the brim.
- **B Marked:** hat off, hair pinned up with a strand loose; grave-pale; Ned's bullet mark at the left temple with a
  thread of cyan light in it; the tailcoat torn at one shoulder and the cravat pulled loose; no smile; eyes cyan.
- **C The Riverboat Queen:** as in life, on the boat: the hat tipped back, a black velvet choker, the brocade
  waistcoat open a button, a fan of five cards held low in frame, lamplight gold from below; brown eyes, no glow; a
  cheroot's smoke, not the cheroot.

**Hollis Crane, the Unpaid** (1.95 m, gaunt and stooped; the marshal of a town whose saint went dark, forty years
walking; one eye still wears the ferryman's coin; Sexton promised him a fare; embers Debt, Mercy, Judgment: Toll
Chain, Grave Ward, Last Rites). Face: a man who died at sixty and kept walking: grey-green skin tight on the bone,
a drooping white mustache, long white hair, sunken cheeks, a tarnished coin over the LEFT eye and cyan light in the
RIGHT socket.
- **A The Marshal:** battered slouch hat, the coin on the eye, the tin star on the tattered slate coat, the ragged
  shoulder cape, the toll chain from shoulder to hip with its hook, the ember glowing through a tear in the shirt.
- **B Forty Years Walking:** hat off, hair wild and thin; the coin gone and BOTH sockets cyan; the cape worn to
  threads, the coat open on the ember in the chest, brighter; the skin more sunken; the chain slack over the shoulder.
- **C Lawman, Once:** as he was buried: hat squared, coat brushed, the mustache waxed, the star polished, the coin on
  the eye; the rot only a greyness in the skin; no cape; the ember a warmth under the shirt, the right socket a faint
  glow (the only glow).

**Gauge, the Iron Hand** (1.92 m, massive; the railroad's clockwork track-laying engine, woken in the wreck the gang
left behind by a spark of ember sealed in its firebox; embers Forge and Steam: Slag Bomb, Iron Hide, Rail Spike). No
MakeHuman body: built from bpy primitives (cylinders, domes, plates with bevelled edges, rows of rivets as instanced
spheres, pipes): iron plates and brass bands, a boiler chest with a pressure gauge for a heart, a firebox glowing in
the belly (below frame or at its bottom edge), two stacks, domed pauldrons, a squared head with ONE headlamp eye, a
grille mouth, a steam whistle at the shoulder, three-fingered riveted hands (one may rest in frame). Materials: lead 11
for iron, brass 8, glass 14 for the lamp and gauge face, black 13 for soot; glow mask on the lamp, the firebox and
seams. Its stance in frame as a bust: the head turned, one pauldron forward.
- **A The Track-Layer:** working iron, dulled and oiled, the headlamp lit cyan, the stacks wisping, the gauge's needle
  in the red, brass bands catching the key light.
- **B Woken in the Wreck:** rust-streaked, one pauldron torn off to the works (gears, a piston), a stack bent, the
  firebox door sprung and the ember blazing in it, the headlamp's glass cracked but lit, the company lettering half
  gone from the chest plate (any invented name: "T.R.R." is fine; no real railroad).
- **C Company Iron:** the day it left the works: polished brass, black enamel, an enamelled crest on the chest, a brass
  nameplate GAUGE on the brow, a tidy lamp with warm white light (not cyan: its life before the ember), no soot.

**Wendell Pike, the Surveyor** (1.68 m, stocky; walked every mile the railroad will ever run until the gang left him at
the bottom of a ravine; he still knows the way; embers Sight and Craft: Spyglass, Tripwire, Long Shot). Face: a man of
fifty-five, round and weathered, a big nose, grey mutton chops joined to a big mustache, round spectacles, small
sharp eyes.
- **A The Surveyor:** the brown bowler with brass goggles pushed up on it, spectacles, the teal neckerchief, the khaki
  field coat full of pockets under a cartridge belt, the pack's straps and a tripod's brass-shod legs over one
  shoulder, a map tube; eyes cyan behind the lenses.
- **B Bottom of the Ravine:** the fall: the bowler dented, one lens cracked, a cut over the brow, dust and grave-pale,
  the goggles down round the neck, the coat torn, the brass spyglass clutched to the chest; eyes cyan.
- **C The Mapmaker:** as in life, no hat: thin hair combed over, spectacles pushed up the forehead, a pencil behind the
  ear, the neckerchief loose, a rolled map under the arm and a measuring chain over the shoulder; brown eyes, no glow;
  lamplight warm.

## Paint settings (starting points; the agent tunes them)

- Odessa: wine and gold: bgTop #1a0d14, bgBottom #07050a, bgGlow #5a2a3a, sparks #ffd27a (gilt), dust #c9a06a.
- Crane: swamp and bone: bgTop #0b1412, bgBottom #040807, bgGlow #2a4a40, sparks #a8ffe0 (will-o'-wisp), dust #8fa89c.
- Gauge: forge: bgTop #140d0a, bgBottom #060403, bgGlow #5a2c12, sparks #ff8a1c, dust #6b5a50 (soot).
- Pike: dusk on the line: bgTop #10141c, bgBottom #05070b, bgGlow #3a4a5a, sparks #ffb55c, dust #b8c4cc.
- `glow` stays #5ad8ff (the Revenant cyan) except where an option has no glow; Gauge C's lamp #ffe2a8.

## Who does what (disjoint files)

- **REFACTOR (done first):** `bust.py`, `EllisBlender.py` thinned, `render.js --script`.
- **FRAME:** `frame/gen.js` + `frame/lib.js` (a hero parameter: motifs and the name on the headboard/plate), writes
  `Frame_Odessa.svg`, `Frame_Crane.svg`, `Frame_Gauge.svg`, `Frame_Pike.svg` (Window.svg unchanged); `sheet.js`
  generalized (`--title`, labels from the hero script). Not `render.js` (Main adds `--frame`).
- **ODESSA / CRANE / GAUGE / PIKE:** one agent each, one file each (`<Hero>.py`), plus their renders in
  `Saved/ArtPreviews/Portraits/<Hero>/` at 600 px. No edits to `bust.py` or `EllisBlender.py`: if the shared code
  lacks something, write the helper in your own file and name it in your report (Main merges it into `bust.py` later).
- **Main:** final renders (1024 px, 80 samples, aux), `render.js` painting with the hero's frame, `sheet.js` per hero,
  review, the user's sheets, docs, commits.

## Done means

A hero script renders all three options at 600 px without error in under a minute each, each option reads as its
description at a glance (you looked at it), the Revenant cyan is in A and B, the `# PAINT` lines and `line` texts are
in, and the report (under 30 lines) lists every decision you made (faces, colours, props you added or dropped).
