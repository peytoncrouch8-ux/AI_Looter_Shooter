# Pipeline: Character models

Part of the pipeline notes (index: `Docs/Pipeline.md`). Rules: `CLAUDE.md`.

## Character models (MakeHuman bodies, simulated and draped clothes, baked looks; Ellis, 2026-10-15)

Why: the portraits pick the look (Ellis A, The Revenant); the game model is built the same way, from MakeHuman's CC0 base,
as a scripted model: `Art/Models/Characters/Ellis.py` -> `SK_Ellis`. The user wants to spin it (the spin viewer is a glTF
export of it) and asked for it "less smooth": real folds, creases, seams and wear, not clean shapes. Helpers, for the
other heroes too:
- `Tools/Blender/looter_human.py`: `Body` (MakeHuman shaped by macro and face sliders, sized to a height, standing on the
  origin facing -Y; `lift` raises it onto boot heels). `skeleton()`: the UE5 mannequin's 63 deform bones on MakeHuman's
  joints (pelvis at the hips' middle; spine_01..05 up MakeHuman's spine, the chest's bone halved; clavicle to the shoulder
  joint; finger1 is the thumb, metacarpal1..4 index..pinky). `vertex_weights()`: MakeHuman's own skin weights summed onto
  those bones (spine and hip weights pooled and shared over pelvis..spine_05 by height; every face, eye, jaw and tongue
  bone to `head`), top four. `weights_at(points, faces)`: a garment point takes the weights of the nearest point on the
  body (or on chosen faces). Bind pose: MakeHuman's rest (arms about 49 degrees below level, elbows a little bent).
- `Tools/Blender/looter_garments.py`: `drape()` (cloth round a vertical axis, each column outside the widest point at or above
  each height; `hb`, the wedge a column looks across: 7 degrees bridges the spine's groove and a row of cartridges, 2 left a
  dark seam down the coat's back and bullet tips through it); `shell()` (body faces pushed out along their normals,
  Taubin-smoothed, MakeHuman's UVs kept); `clear()` (pushes points back outside the layers under them); `grid()`, `tube()`,
  `strip()` (UVs in metres; `tube()` faces outward); `solidify()` (plain offset: Blender's even thickness shot vertices metres
  away from sharp corners, the soles' edges and the coat's vent); `simulate()` (Blender cloth, cached, below).
- `Tools/Blender/looter_looks.py`: procedural Cycles looks ending in OUT_COLOR/OUT_ROUGH/OUT_METAL/OUT_NORMAL reroutes:
  oilskin, felt (a sweat stain from the band), leather (creases that crack, scuffs, pores; boots: a stitched toe cap, the
  welt's stitches, scalloped rows on the shaft), wool, brocade, linen, bandana, metal (scratches, dents), walnut (checkered),
  skin (pores, fine wrinkles, veins), scalp, hair, stacked sole, glow. Patterns in object space (metres). Shared touches:
  `ridged()` crease networks (stretched down the body on hanging cloth), `construction()` (hems, topstitching, seams from
  the model's 'Seams' attribute: R closeness to an open edge, G to a sewn seam), `dashes()` (stitches along any line from
  three sines), `stains()`, `splatter()`, dust by height and facing, grime from occlusion, wear from Pointiness (no rays).
  What didn't work: a 380/m twill and 700/m weave aliased into moire at the bake's ~1000 px/m (keep patterns under ~300/m); a
  dense fine crumple network read as cracked stone or marble, not cloth (cloth creases: few, broad, long, soft). The
  waistcoat's brocade (Ellis's alone) is smooth black wool with a faint woven figure (0.12 of the colour, bump 0.04) and
  broad shallow creases (9/m, bump 0.12): with crinkles it read as leather (the user, 2026-10-15: "as cloth").
- `Tools/Blender/looter_bake.py`: `pack()` (one atlas per set: islands averaged to one density, packed); `bake_set()`: the
  occlusion on the game parts themselves (Cycles AO bake at a quarter of the set's size, 32 samples, 0.1 m, scaled up into
  the ORM; the detailed parts hidden, or lying on their game parts they shade them black), then colour, roughness and
  metal as emissions at one sample and the normal (tangent, OpenGL; written green-flipped as DirectX), cast from a joined
  copy of the set's game parts onto the detailed parts (`sources`: selected to active, cage 1.5 cm, reach 4 cm); writes
  `Art/Textures/<Set>/T_<Set>_BC|N|ORM.png`. `load_set()` reads a set back; `digest()` fingerprints the scripts. ~5 min for
  four 2048 sets on the cloud box's 4 cores (it was ~15 min at 1024 with ambient-occlusion nodes live in the looks; grime is
  Pointiness now). Any edit to the scripts changes the digest and bakes again.

The duster's cloth simulation (`settle()`, `looter_garments.simulate()`): the skirt is built fuller than it hangs (flare 0.13)
and Blender's cloth lets it fall onto the body, trousers, waistcoat and belt, pinned above the hips (smoothstep 0.92-1.08 m),
90 frames, stiffness tension/compression 40, shear 15, bending 12, mass 0.3, self-collision on (~50 s; ~28 s on the PC). Kept in
`Art/Models/Characters/Ellis.drape.json` under a digest of the starting cloth, pins, colliders and settings, so an export
simulates again only when one of them changes (as MisterSexton.drape.json).
- Bugs met: the cloth exploded (points metres away) under self-collision because the start mesh crossed itself: `clear()`
  against the waistcoat's armhole edge had folded the coat under the arms (now it clears against the belt only; the drape
  already rests on the waistcoat). Check with `--selfx` (prints crossing face pairs). The lantern and holster are not
  colliders (they started inside the coat's panel).
- The back hangs straight (the user, 2026-10-15: "Back of the cape is deforming in between his legs. It should be
  straight"): the boots are colliders too (the coat reaches the ankle), and `leg_fill()` adds one only the simulation sees:
  per height (every 3 cm, the hem - 4 cm to 0.90 m) the convex hull of both legs' slice, 4 mm out, and behind the legs
  narrowing at most `LEG_FILL_DROP` 0.03 m/m going down (the back falls near plumb from the seat). Its rows must run top
  down (`gm.grid`'s way) so its faces face out: Blender's cloth collides one-sided by the collider's normals, and inward
  they pulled the coat through the fill and round each leg like trousers.
- The vent (closed and straight, the user): simulated, its two edges start together and the cloth falls with them welded
  (`welded()`: a copy with each pair one vertex, the strip between them dropped), then `lap_vent()` laps them as the built
  coat's: the left flap's edge 4 mm out and 0.025 rad round over the right's, the right's 0.02 rad under it (`ob['vent']`
  holds the pairs). Started apart, the flaps fell into each other and tangled, one swept across the other and kicked out.
- The cape is not simulated: on the shoulders it had nowhere to fall but into itself and crumpled. It stays procedural, with
  flutes of uneven depth and spacing. Bending 4 crumpled the skirt and the vent like linen; 12 gives a duster's broad folds.
- Looser (the user, 2026-10-15: "hugging his body too much"): `COAT_EASE` 1.6 cm off the body from under the collar to the
  chest, 1 cm more from the hips (the collar and the cape's top, fitted to the coat's top, keep their size); flare 0.10 ->
  0.13 (0.15 read as a tent); the cape `CAPE_EASE` 1 cm off the shoulders and back, hem flare 2.4 -> 4 cm. Clay before and
  after: `Saved/Concepts/EllisSwing/coat_loose_{before,after}_{front,side,back}.png`.
- The vent never opens in the mesh: `gm.grid` joins its two columns with a strip of narrow faces down to the hem (the weld
  only collapses them above VENT_TOP). Anything that moves its two edges apart stretches that strip into a web.

Ellis.py:
- Order: body, trousers (knee and boot-top creases), boots (below), waistcoat and shirt,
  belt (before the duster, which drapes over it), duster, sleeves (bunched at the cuff, crescents at the elbow), cape
  (draped over the arms let down to the game's idle, `IDLE_ARM` 10 degrees off the body: draped over the bind pose's A,
  its sides stood out like wings and fins once the arms came down; its front corners flare less; each point at least
  8 mm outside what's under it, unsmoothed (the drape's smoothing let the sleeves' tops through by the collar's foot);
  its top carried up into the bind pose by its share of the upper arm (below), cleared off the coat and the sleeves
  as the idle has them (`clear_cape`, `cape_idle`; round 6)),
  holster, lantern (hung from a `BeltHook` piece out from the belt; clear of the thigh from fount to cap, 4.3 cm and 3.3 more:
  with 1.7 a crouch's level thigh, under the hook it rides, pressed 1.5 cm into it), the
  simulation (the belt, cartridges and bullets as colliders), gloves (`thicken_hand`: `GLOVE_GIRTH` 1.15, each point pushed off its
  skinning bones square to them, blended by its weights: fuller palm and fingers, no longer; the user: thin, clawed
  hands, 2026-10-15; the glove runs `GLOVE_CUFF` 6 cm up the forearm inside the gauntlet; the gauntlet's mouth sits at
  the wrist, each ring the smallest ellipse round the forearm's axis holding the glove's slice + `CUFF_ROOM` 4 mm,
  flaring `CUFF_FLARE` 1.8 cm, 3 read as a bell; the sleeve stops `SLEEVE_END` 3 cm up inside it, gathered in under it to
  `SLEEVE_TUCK` 1.2 cm off the arm. The user, 2026-10-15: "his wrists are either invisible or don't match the gloves":
  the game mesh has no wrist, so between the glove's edge and a mouth held open by the loose sleeve's end you saw
  through him, and the coat's colour; fitted angle by angle the mouth stood out in a lobe at the thumb's root; its lip
  rolled out `CUFF_LIP` 3 mm: cut square, a turned and bent hand met it in a sharp notch), hat
  (EllisBlender.build_hat moved into the figure's space; the brim's reach past the crown `BRIM_REACH` 0.88, each spoke
  shortened toward its root so the curl keeps its shape),
  bandana (inside the collar: draped over skin, waistcoat and shirt; its point down the chest), collar and pin (built
  after the bandana: `COLLAR_GAP` 1.2 cm off the bandana, its knot and the neck, its wall no nearer the neck than the
  cape's top edge at the idle + 1.2 cm (its most over a few degrees, smoothed: over the square shoulders that edge is
  18-19 cm out, past the old 16 cm foot, and the sleeves' tops showed between); its top edge as round 5's, the brim's
  clearance; the front ends roll out half as much (they turned out flat like tabs); the throat's points splay half as
  far: more and the flap with the pin jutted like a box; its lining, `collar_lining` (the game's one-sided materials:
  looking down between the neck and the collar you saw past the skin, which stops under the collar, into the neck,
  and a jumble of the cape's, coat's and sleeves' tops): a band `COLLAR_BAND` 2.5 mm off the body round the back
  (`COLLAR_LINING` 85-275 degrees, where the bandana's band is cut) from 1.2 cm over the skin's lower edge down to a floor
  at neck_01's head + `COLLAR_FLOOR` 3 mm, out to the wall, on to the collar's ends from the bandana out; its 'Lining'
  points turn with the skin's weights near the neck, cleared off the reduced skin (`clear_collar`); round 6 tried the
  foot tucked into the cape's top flaring out to the same top edge: it read as a frilled ruff on a stalk), hair (cards,
  below), then
  'Seams' on the sewn pieces; the game mesh (below), its UVs, the bake from the detailed parts (then deleted), thickness,
  weights, one joined mesh `Ellis`.
- Hair cards (round 6, `Tools/Blender/ellis_hair.py`; the user, 2026-10-15: "hair looks like mud"). What failed before:
  flat ribbons read as slabs; 80 narrow ribbons as ribbons; closed lens-shaped tube locks in two layers as dark ropes
  and dreadlocks (mud: no shine, no strands; the detailed locks baked onto the low ones smeared the strands). Now layered
  card strips UV-mapped onto a script-made strand atlas, not baked (the cards are the game's hair as built; `HAIR` and
  `HAIR_LOW` build the same): from the scalp out a sheet (41 columns round the back from temple to temple: the
  underlayer, so no skin shows), main (30 cards, 4 cm, dense strips), outer (36, 2.9 cm, shorter and flicked out at the
  tips: the layered, shaggy cut), wisps (24), fly-aways (12) and the lock across the left brow (3); each card leans round
  the head by a random sweep (all plumb read as a curtain), arched to the head's curve, 8 rows; custom normals out of
  the hair's volume (they survive the join and the export's transforms). 2748 triangles (round 5's locks: 5.8k).
  Roots 1.2 cm up under the hat band (`ellis_hat.band_ring`, the sweatband's inner face), pressed within its room off
  the scalp (7 mm sides and back, 2 mm forehead; logged: points past it, 0), fuller from a little below the band.
  Tips: the jaw by the temples, into the turned-up collar round the back (2.2 cm under its rim: collar length). Grown
  over skin and bandana only; the collar and cape are walls the hair is kept 5 mm inside (as colliders, one-sided, they
  pushed cards out through the collar). The hat tipped 5-20 degrees on its pivot: card points its brim would sweep are
  moved in under the crown (rig check: 0 of 2357). The back's swing chain `hair_01..02` on the cards' median depth
  (`chain_joints`; the outermost card bent it 10 cm out).
- The hair atlas (`ellis_hair.make_textures`; `Art/Textures/EllisHair/T_EllisHair_BC|N|ORM.png`, 2048): top quarter a
  sheet tiling across (the underlayer: solid to 70 % of its length, then strands); below, 8 strips 256 px wide, root at
  the top: 2 dense, 3 clumped, 2 wisps, 1 fly-aways. Strands drawn one by one (numpy, no library art) in clumps and
  sub-clumps (forked, uneven tips), body strands behind for mass; sRGB root 0.08/0.06/0.047, mid 0.16/0.118/0.09, tip
  0.185/0.138/0.104 with a little dust (lighter tips read as dyed). BC alpha = coverage (soft-edged: the game dithers it);
  a strand fades where it reaches its strip's side (clamped there, strands drew the card's edge as a line); N the strands'
  round profile (DirectX); ORM R occlusion (root, deeper strands), G roughness per strand (~0.41), B each strand's random
  id, NOT metal (M_CharacterHair tips the tangent by it). Ellis.py: `MADE_SETS` (not baked; made if missing), slot
  `EllisHair` on `M_CharacterHair`; `--look <tag>` renders the head (Eevee, dithered alpha; `--aniso` Cycles) into
  `Saved/Concepts/EllisRound6/hair/`.
- The coat: option C, the user's pick (2026-10-15), is the build: squarer shoulders and a pointed back on the cape, a
  deeper crown crease and harder brim curl on the hat, the duster to the ankle (`HEM` 0.24; it hangs 0.20-0.23 m).
- Boots (the user, 2026-10-15: "proper boots"; MakeHuman's foot reshaped into a toe box read as a bare foot on a sandal,
  then a last with the calf's shell standing in it as a slipper with a sock): `boot_shaft()` a closed thick-walled tube,
  round each 1 cm slice of the leg (cut through its faces: the shin's vertices lie in rings 4-5 cm apart) its convex
  hull + `BOOT_EASE` 1.2 cm, a quarter rounder, narrowing at most `BOOT_TAPER` 0.08 m/m going down (a stovepipe, not a
  sock), straight on down from 3 cm over the ankle to the insole, the top at `BOOT_TOP` dipping 3 cm front and back, a
  `BOOT_WALL` 5 mm wall round a well down to 0.34 m for the trousers; `boot_last()` 46 rounded-rectangle sections (closer
  at the ends) from the heel to 2.5 cm past the toes, over the foot with 9-10 mm room, a roomy toe box (insole + 4.6 cm),
  ahead of the ball narrowing at most 0.15 m/m (to the toes' own tips it came to a beak), the toe a quarter ellipsoid on
  the sole, a little square in plan. Both fused by a voxel remesh (`BOOT_VOXEL` 1.8 mm), the crease where they meet
  rounded (`fillet()`: vertices within 2.2 cm of both eased toward their neighbours, 60 passes), reduced to `BOOT_TRIS`
  7000 each; the soles' outline is the fused boot's footprint + 4 mm welt. No UVs (smart-projected for the bake); the look
  is object space: the toe cap stitched at y -0.19 (5.5 cm behind the tip), the welt at 2.15 cm. The body's feet aren't
  in the game mesh (its skin is the head and neck).
- Texture sets (2048): EllisSkin (face and scalp), EllisDuster (coat, sleeves, cape, collar), EllisGarb (hat, bandana,
  waistcoat, shirt, trousers), EllisHair (script-made, above), EllisGear (boots, soles, gloves, belt, holster, revolver, lantern, brass, lead, iron). The eyes
  and the lantern's globe and flame are Glow materials (EllisEyeGlow, EllisLanternGlow).
- Skin weights by part (`weights()`): the body's own; the duster above the hips from the torso, its skirt pelvis to the
  thighs (side by x) as looter_heroes' skirts; the cape's top what the clothes under it ride (`cape_weights`: the nearest
  skin's at the idle, its collarbones and upper arms, the rest spine_05, kept as 'CapeW_<bone>'), handing over to its
  chains from the shoulder line to halfway down ('CapeChain'), its top carried up into the bind pose by its share of
  the upper arm (`cape_idle`) so the idle puts it back as draped, nothing riding the upper arm below (round 5's
  'ArmFollow' carried the whole cape up by a share of the arm: blended halfway between the chest and an arm raised to
  aim, the cloth sank into the arm, the clipping the user saw; round 6); collar spine_05 to neck_01 by height, its
  lining's band and floor near the neck the skin's own; bandana the skin's own weights where it lies (head over nose, mouth and chin; head,
  neck_02, neck_01 down the neck; the knot too; the user saw it cut through the neck when the head turned, 2026-10-15), its
  bib onto its chain; hair, eyes head; hat crown, brim and band 100% `hat`; belt pelvis (+spine_01); the holster's skirt
  pelvis over the belt to thigh_l down the pouch, the pouch and its loops all thigh_l, the gun rigid at `HOLSTER_GUN` 60 %
  thigh_l (round 5: blended from the belt down the pouch, the crouch's thigh sank 3.6 cm into it; the gun all thigh, a
  stride swung its grip, in front of the hip joint, 4.5 cm into his belly; all hips, a crouch's thigh rose 5 cm into it;
  at 60 % 0.9 and 1.1 cm);
  `BeltHook` pelvis to thigh_r along it; lantern all on lantern_01 (it stays on thigh_r: on pelvis a crouch's thigh rose
  5 cm into it);
  gauntlets lowerarm to hand; gloves from the hand's faces; then (`twist_weights`) on the sleeves, gauntlets, conchos and
  the glove's cuff the lowerarm and hand weight shared by how far down the forearm a point is: the hand's share coming in
  over `WRIST_BEND` 0.78-1.25 of the forearm (it brings the bend; over its last tenth a turned, bent hand creased the
  gauntlet's lip), the rest over lowerarm, lowerarm_twist_01 (half way down) and lowerarm_twist_02 (90%) (the game turns
  each twist bone by its share of the hand's twist: a gun's grip turns the left hand ~100 degrees, the right ~32;
  unturned, the glove tore off the gauntlet); past the wrist the forearm's share is the hand's (the wrist's own blend there
  candy-wrapped the turned glove). Hit zones:
  head, trunk, skirt round the hips, each limb, hands, feet. LODs 50 % and 25 %.
- Swing bones (`swing_layout`, placed from the built game mesh; the game's springs, Player/Animation/BodySwing, move them):
  skirt_{fl,sl,bl,fr,sr,br}_01..03 on pelvis (each side's f and s chains at 1/6 and 1/2 of its arc from the coat's front
  edge to the back's middle, b straight behind the knee: at 5/6, by the vent, a stride's back leg slid it aside and the
  knee came out through the ankle-length coat; joints at the belt line, mid-thigh 74 cm, the knee 53 cm, the hem 21 cm)
  and skirt_bm_01..03 down the middle of the back over the vent (the same heights; without it a stride's back shin came
  through between the two b chains), skirt_bli and skirt_bri (`INNER_AT` a fifth of the way from each b to bm: there
  the stride's back shin carries them with b; halfway 35 points of it came through, a third 31, a fifth none),
  cape_{f,a,s,b}{l,r}_01..02 and cape_bm_01..02 (`CAPE_CHAINS`, round 6; round 5's two a side, f and b, from the shoulder
  line, let a raised arm come up through the cloth between them): f the front panel (0.09 of the arc from the front
  edge, from the cloth at the shoulder line 143 cm on spine_05), a the front of the shoulder (0.24), s straight out
  over the shoulder joint, b behind it (0.64), these three from the shoulder joint itself on the collarbone (their
  cloth turns about the joint the arm turns about: the springs' share of the arm's turn, no cloth blended halfway);
  bm down the middle of the back from the shoulder line on spine_05; ring_weights reads a joint-rooted chain's place
  round the cape from its cloth points ('ring'); `cape_arm_l/r` (unweighted, under the collarbones: the upper arm as it
  hangs at the idle, `add_cape_arm_bones`) for the springs to take the cape from; bandana_01..02 on neck_02 (under the chin to the
  point), hair_01..02 on head (from 168 cm, a quarter of the way from the skull's bulge to the nape, which is inside the
  collar, down the back at the cards' median depth to where most of the back hair ends, on the nape at ~156 cm),
  lantern_01 on thigh_r (from the bail's top on the hook's end, right above the lantern, to the
  fount: from the hook's belt end the springs' gravity swung it 6 cm into the thigh). 50 bones in 21 chains; each one's
  head at its joint, its Z out of the cloth; and each chain's `<chain>_end`, an unweighted leaf (deforming, so the FBX
  keeps it; 2 cm on along the last bone) with its head where the cloth ends: an FBX bone has no tail, and BodySwing
  guessed the last tip as long as the bone before (the skirt's at mid-shin, the hem 11 cm past it, which then went
  through the ground in a crouch). Weights: today's down to 0.91 m on the skirt (the shoulder line on the cape), all chains from
  mid-thigh (halfway on the cape): past there a vertex already shares two chains' two bones (4 influences); round the body
  between the two nearest chains at the vertex's height; across the back each side's b, bi and then skirt_bm (the cape:
  its two b's to each other; the vent never opens: the cloth simulation's notes); down a chain
  each bone hands over to the next along that one's length by a smoothstep. `soft_top4`: the fifth-largest weight comes
  off the top four (plain top four drops a bone at once: a kink). The bandana's chain was on head (the contract): it swung
  2 cm out through the collar when the head turned or rolled onto the stock; on neck_02 the bib stays inside the
  collar's opening while the mask turns with the face.
- `hat` (on head): from the back of the band at its middle height to the front of the band; the game tips the hat on it
  while aiming (brim front up, up to `HAT_TIP` 20 degrees) so the brim clears the rifle's scope. Tipped 20 degrees the
  brim reaches no hair and no collar (it first touches the hair at ~50 degrees).
- Facing: built facing -Y; `face_like_mannequin`, the last step before the export, turns the armature's bones, the mesh and
  the hit zones a quarter about Z (into the data) to face -X, so SK_Ellis faces +Y in Unreal with his left at +X, as
  SKM_Manny_Simple (facing +X, the retargeter couldn't line his fingers and arms up: the clawed hands). An exported FBX
  read back gives the file's own axes; Unreal flips Y, so Unreal = (x, -y, z) of those.
- The game mesh, 34.9k triangles (the user's call, 2026-10-15: up to 35k; at 20.6k it read coarse): the detailed build
  (~108k) is only the bake source. `make_lows()` copies each part and decimates it by its `LOW` share (Blender's Decimate,
  collapse), spending where the silhouette and close-ups need it (face, hands, collar, brim, coat edges, boots; small hard
  parts whole: reduced, the gun, holster and lantern shredded into shards); the face spares the eyes and the cloth
  (`EDGE_KEEP`: duster, cape, collar, bandana, brim) a band along its open edges (a vertex group, inverted: Decimate
  collapses only what its group weights); `unflip()` turns back the faces Decimate flipped (against the detailed part and
  their neighbours) and dissolves its slivers: Unreal culls a face's back, so each was a hole (the waistcoat's "black
  holes", 116 in the old boots' toes, the duster's back); the shirt keeps only what shows in the waistcoat's V (bent, the rest
  came through); the bandana's back, hidden under collar and hair, goes; the hair cards are built again as they are
  (`HAIR_LOW`; each card's sideways direction carried along it: a card running straight out twisted into a hook); reduced layers are pushed back outside the reduced
  layers under them (a reduced sleeve cut through its reduced gauntlet; the coat over the cartridges and bullets too);
  below the chin the bandana stands 1 cm off the neck's skin (the mask above, 3 mm). The copies take each part's
  transform (the eyes, buttons, pin and conchos are placed by their object, not their mesh: copies without it fell to the
  floor). Biggest shares (round 5; round 6's hair cards 2.7k): hair 5.8k, duster 3.4k, face 2.6k, boots 2.5k (LOW 0.18 of the fused 14k), gloves 2.3k, cape
  2.0k, bandana 2.0k (round 5 paid for the hair's locks with the waistcoat's buttons at 0.35, the bullets 0.25 and the
  lantern's wire 0.4). `--stats`
  prints the triangles per part (no bake). Leather looks keep their grain at 280/m (900-1100 aliased at the bake's ~1000
  px/m into a fuzz that read as suede and fur).
- `--audit <tag>` (`Tools/Blender/ellis_audit.py` on `looter_audit.py`; neither is in the bake's digest): the baked look in
  Cycles (sun ~40 degrees and sky; one dusk shot) as an 8-angle turntable at the game's idle and close-ups (face, under
  the brim, hands, belt, boots front and side, waistcoat, back, collar, hair), the game's poses (`looter_audit.POSES`:
  idle, aim, stride, crouch; grip_a and grip_b, the aiming arms with the hands turned on the forearms as the guns turn
  them, 100 and 32 degrees each way round, the twist bones by their shares: `('twist', side, degrees, bend)`) in clay with the swing chains where BodySwing's springs would rest them (`settle_swing`: its
  constants, limbs, bone push (top 35% left alone, at most half the bone a pass), nothing over its head, and floor
  copied, keep in step; its points the game's: an FBX skeleton has no tails, so a chain's last tip is
  its head plus the bone before, a lone bone's 18 cm down; the pose reached in 12 stages, each point moving at most 1 cm a
  pass and kept out of the limbs on the way, as the springs follow frame by frame: set straight into a pose, a limb
  already past a point pushed it out the far side), and every layer checked against the one over it in each pose (`PAIRS`: a point looks out along its normal, turned
  away from the nearest bone so a thickened part's inner shell counts, and back in; `SOLIDS`: a part inside another; `GROUNDED`: under the ground), into
  `Saved/Concepts/EllisAudit/<tag>/` with `findings.json`. The 2026-10-15 audit: `EllisAudit/defects.md`.
- The cape's and collar's audit (round 6): `Tools\artrun.ps1 -Script Tools\Blender\ellis_cape_audit.py -ScriptArgs
  --audit,<tag>` builds and rigs Ellis.py WITHOUT the bake (any edit changes the bake's digest; a `--audit` run of
  Ellis.py itself bakes and writes the texture sets), then 16 poses (`looter_audit.POSES`, round 6 added carry,
  aim_high, aim_low, crouch_aim, swing, look_l/r/up/down, look_up_tip; `('tip', bone, degrees)` tips the hat as the game
  does while aiming): crossing faces (BVH overlap) of the cape and collar with every layer, counted apart where out of
  the collar's ring; points through (PAIRS); cape points inside the sleeves' tubes; the brim's nearest to the collar;
  close clay views (shoulders, arms, hands, collar; backs culled as Unreal's one-sided materials) and views down into
  the collar against a magenta sky (magenta between the neck and the collar is a hole), into
  `Saved/Concepts/EllisRound6/cape/<tag>/`. Env: `CAPE_POSES` (a+b), `CAPE_SHOW` (parts to draw), `CAPE_RENDERS=0`,
  `CAPE_SWEEP` ('f=0.2,0.3;bulk=4,6': tries the springs' shares and the cape's bulk), `CAPE_SRC` (another version of
  Ellis.py, e.g. `git show HEAD:...`), `CAPE_MIRROR=r5` (BodySwing as round 5 shipped it).
- `--option A|B` (`Tools/Blender/ellis_options.py`, off unless asked; the user picked C, now the build, 2026-10-15, and
  not these): A gear and story (a bandolier over the coat, a satchel at the right hip, spurs, a watch chain, the lantern
  a third bigger; +2.9k triangles), B the coat itself (lapels, pocket flaps, buckled sleeve straps, a second cape tier, a
  frayed and patched hem; +3.3k). An option's cloth is cached apart (`Saved/Concepts/EllisOptions/
  Ellis.drape.<option>.json`); clay views in `Saved/Concepts/EllisOptions/<option>_<view>.png`.
- Bake cache: `Art/Textures/EllisSkin/bake.digest` holds the digest of the scripts, the size and the Blender version the sets
  were baked from; a run with the same digest reads the PNGs back instead of baking (`--rebake` forces it).
- **A bake and an export must build the same mesh** (2026-10-20: Crane's, Pike's and Odessa's Gear sets and Crane's FP
  hand showed scrambled texture in game, 4-15% of their islands on unbaked background). Three causes, all fixed: (1) the
  thread count: Decimate and the push-outs shape parts differently at another `-t`, and bakes ran at artrun's 6 while
  exports ran through models.ps1 at all threads: now `Tools\models.ps1` and `Tools\artrun.ps1` both run Blender at
  `LOOTER_BLENDER_THREADS` (default 6); (2) `bmesh.ops.create_uvsphere` (and `extrude_face_region`) order faces
  differently each run: use `looter_model.uv_sphere` (fixed order, keeps UVs) and `looter_model.fixed_order(bm)` after an
  extrude, for anything that reaches a game mesh; (3) `looter_garments.simulate` cached points rounded to 5 decimals but
  kept full precision on the mesh: it rounds both now. Guard: `looter_bake.pack()` fingerprints each set's packed UVs
  (`Art/Textures/<Set>/uv.digest`); `load_set` refuses a kept bake whose fingerprint differs, so the set rebakes instead
  of drifting. Check tools: `Saved/BakeCheck/` (`analyze.py` paints an export's islands over its BC map: red on unbaked
  background) and `Saved/BakeCheck/fix/` (`run_model.ps1`, `compare_fbx.py`). Every hero now exports bit-identically twice.
  Don't queue a second run of a model while an edited bake source is still baking: the queued run starts on the new
  script and bakes the same sets in parallel (harmless only when its inputs are identical; PIKE, 2026-10-20).
- The cloud box can't push Git LFS: its PNGs are not committed (`.git/info/exclude` there). The PC baked them on
  2026-10-15 with Blender 4.4 (the cloud wrote the scripts against 5.2's module: they ran unchanged; ~25 min for build,
  simulation and four 2048 sets beside the editor) and they're committed. MakeHuman's base.obj (1.75 MB, gitignored) is
  fetched when missing: `Art/ThirdParty/MakeHuman/fetch.py` (the PC has no system Python: run it with Blender's,
  `C:\Program Files\Blender Foundation\Blender 4.4\4.4\python\bin\python.exe -I`). Export beside the editor:
  `Tools\artrun.ps1 -Export Ellis -Family Ellis`, then `Looter.ImportModels <that folder>`: `/Game/Art/Characters/SK_Ellis`
  (142 bones with the root: 63 mannequin, 4 forearm twist, 50 swing, 21 `_end` leaves, 2 `cape_arm`, hat; 3 LODs, Nanite off for the eye glow, 185.5 cm to the hat's top),
  `MI_Ellis*`, `PA_Ellis`.
- Run: `blender -b --factory-startup --python Art/Models/Characters/Ellis.py -- [--size 1024] [--glb <path>] [--preview]
  [--shots full+upper+hips+feet] [--shape] [--sim-only] [--selfx] [--no-sim] [--rebake] [--rig-check] [--audit <tag>]
  [--option A|B]`. `--rig-check`
  (no bake, ~15 s): Workbench clay into `Saved/Concepts/EllisSwing/`: the swing bones drawn at rest, `shares_*` (each
  vertex coloured by its chains), `SWING_TESTS` bends, the hat tipped (`hat_tip_*`), `HEAD_TESTS` (the head alone turned
  35 degrees each way, rolled 20, pitched 20: `bandana_*`); and logs at most 4 influences, the steepest weight steps, the
  tipped brim's reach into hair and collar, and per head pose the bandana's clearance from the neck's skin and anything
  past the collar (at rest 1.7 cm; turned 1.1-1.5; pitched 0.6; nothing through; rolled, 2 points 0.3 cm past the
  collar). `--shape` renders the untextured
  build in clay (the looks unbaked are minutes a frame); `--preview` renders the baked look in the viewer's arms-down pose
  to `Saved/ArtPreviews/Characters/Ellis/`; `--glb` writes the posed, textured figure (WebP maps) for the spin viewer.
- The spin viewer (`Tools/CharacterViewer/index.html`, three.js 0.160 from jsDelivr, the game's LooterUI look): published as
  an artifact (https://claude.ai/artifact/HJCgTaXnSRbEb5k7SXjdDa). Artifacts serve no .glb (nor model/gltf-binary): convert with
  `Tools/CharacterViewer/glb_to_json.py <glb> <dir> ellis` and publish index.html with ellis.json and the ellis_N.webp maps.
  The page reads the `LanternAnchor` node for its lantern light. Check it locally with Playwright's Chromium
  (`--use-gl=swiftshader`), the import map pointed at a local three.js.

Ellis as the player's body (the user, 2026-10-15; their pick "retarget live" over retargeting every clip onto him):
- The mannequin (`GetMesh()`, SKM_Manny_Simple with ABP_Unarmed/ABP_Rifle and the C++ stance layer) stays the animated
  body, hidden: guns, hand sockets, the hands' IK, the jar, footsteps and every test still use it. `UBodyFigureComponent`
  (`Figure` on ALooterCharacter, attached to the mesh) wears SK_Ellis and copies its pose each frame through
  `RTG_MannyToEllis` with `UBodyRetargetAnimInstance`: the engine's Retarget Pose From Mesh node as a native anim
  instance's root (`GetCustomRootNode`; its game-thread `PreUpdate` called by the proxy, since only a Blueprint graph calls
  it for you; the node value-initialized, its counters have no initializers). Ellis ticks after the mannequin (tick
  prerequisite) so the hands don't lag the guns, and mirrors its first/third-person drawing (`bOwnerNoSee`,
  `FirstPersonPrimitiveType`) each tick: invisible to the player in first person, his shadow cast. Without SK_Ellis or the
  retargeter the mannequin shows as before. `ALoadoutStage` wears him too (unused since the redesign).
- `Tools/Unreal/build_hero_retarget.py <Hero>` (default Ellis; first used for Gauge 2026-10-20) makes
  `/Game/Player/Retarget/IK_Manny`, `IK_<Hero>` (the engine's auto chains: the same 29 on both, fingers too; Gauge's
  unweighted ring and pinky bones map too) and `RTG_MannyTo<Hero>`: default ops (Pelvis Motion, FK Chains, Run IK Rig, Root Motion, Remap
  Curves) plus Blend to Source on LeftArm and RightArm (hands go to the mannequin's, the arms reach them by IK: before it
  his hands missed the rifle). Lessons: SK_Ellis faced +X, the mannequin +Y: the retarget pose turned his root 90 degrees
  and nothing else, as the engine's auto align compared them unturned and tipped his head ~25 degrees sideways (both
  rest poses are close enough: arms down, the neck a little forward). Since 2026-10-15 he's exported facing +Y like the
  mannequin (Ellis.py `face_like_mannequin`): the script reads his facing (toes) and auto aligns every chain when he faces
  +Y (the fingers line up: the clawed hands), the root turn only for a +X mesh. The script rebuilds the assets in place: deleting
  and recreating the retargeter nulled the character's pointer and dirtied BP_LooterCharacter (a Save prompt blocked
  `launch.ps1 -Build`), and the component then also loads the assets again at play if its pointers are empty.
- Checking in PIE: `play.py view [n]` cycles first/third/front views (F5); `play.py inventory [close]`; toggling the
  mannequin back on (its `set_hidden_in_game`) compares poses. Keys from `input.ps1` (Tab, number keys) didn't reach the
  game on 2026-10-15, so both have play.py steps.
- Materials (the user: "plastic, stiff and unlike a real fps character"; round 1 approved 2026-10-15): Ellis.py's
  GAME_MATERIALS gives each kind of surface its own slot on a master made for it, several slots sharing one baked set:
  EllisSkin on M_CharacterSkin (Preintegrated Skin since 2026-10-20, `SKIN_SHADING` in build_world_materials.py:
  subsurface colour = skin colour x ScatterColor (linear 1, 0.3, 0.2) x ScatterAmount 0.4, wrapped round each light's
  terminator by the engine's PreintegratedSkinBRDF lookup; ScatterOpacity 0.99, which also sets how thick the skin is to
  shadows: 0.9 let the scatter through Odessa's hat-brim shadow as an orange glow; no screen-space SSS pass, so the same
  look on every quality; Crane and his first-person arms ScatterAmount 0.35, ScatterColor '#D0DCC8', dead skin;
  SubsurfaceAmount and SP_Skin unused, SP_Skin kept for the MSM_SUBSURFACE_PROFILE fallback), EllisDuster and EllisGarbCloth
  on M_CharacterCloth (the engine's Cloth shading: a fibre sheen at grazing angles), EllisGarbHair (the hat band only
  since round 6), EllisGear and EllisHolsterGun on M_Character, EllisHair on M_CharacterHair (masked, two-sided, the
  engine's Hair shading along the cards' V; opacity = the atlas's alpha raised per mip level (MipBoost 0.35) through
  DitherTemporalAA, clip 0.5; Scatter 0.2 on the Metallic pin, Specular 0.45, TangentJitter 0.5 from the ORM's blue;
  Backlit, Custom Data 0, unwired: 1; `HAIR_SHADING = MSM_DEFAULT_LIT` is its fallback, lit by the strands' normals). The masters (build_world_materials.py's characters) share M_World's maps plus
  RoughnessScale/Offset/Breakup (a fine noise: no smooth sheen across a part), NormalStrength, DiffuseAO 0.7. Cloth's sheen
  is its fuzz colour times FuzzStrength (the Cloth amount pin, Custom Data 0, can't be wired from Python; unwired it's 1):
  at 1.2 everything went chalk white; tuned in game to 0.2 (oilskin) and 0.3 (wool, linen, felt). Leather and metal stay
  on default lit.
- The holster: the holstered revolver has its own looks (GunIron, GunWalnut) and slot (EllisHolsterGun); while a revolver
  is the gun in hand, UBodyFigureComponent hides that slot on every LOD (ShowMaterialSection), the user's "empty only while
  drawn".
- Swing (the user's "coat and hair that move", 2026-10-15; `FBodySwing`, after the retarget in UBodyRetargetAnimInstance):
  each swing chain (Ellis.py's 40 swing bones, 16 chains) is a string of points in world space, so they lag and swing as he moves: a
  spring toward the animated pose (an acceleration with its stiffness as a natural frequency, damped; Verlet), a small share
  of gravity (the pose already hangs; the lantern full, a pendulum), each point its length from the one before, out of the
  limbs. Limb clearance needs no radii: a point keeps 95% of its rest distance from each limb segment (thighs, calves,
  pelvis, spine, neck, upper arms, forearms, hands), at most the limb's own (14/10/18/15/9/9/7/5 cm). The bone between two
  points keeps clear too (`BoneRadius`: its nearest rest distance, same 95% and caps; `PushBoneOut` turns the tip out by
  the overlap over how far down the bone it is, at most half the bone's length a step, ignored in the bone's top 35%: at
  20% and uncapped a crouch's thighs swung a coat flap out level with the hip, once up to the shoulder): with points
  alone a full stride's back shin slid between two of them and the boot showed through the ankle-length coat's lower
  back (Ellis.py's audit, 2026-10-15). Nothing rises over what it hangs from (`KeepUnder`: level at most). A floor: no
  point under the component's origin + 2 cm (the mannequin's root stays on the ground, crouched too); a point under it
  swings up about the one above onto it, keeping its lean (`KeepAbove`), so a crouch lays the hem on the ground (it sank
  ~11 cm); a grounded point rests (no speed: kept, it flipped a gathered hem over its head). A thick thing keeps more
  room (`FBodySwingSettings::Bulk`, added to the clearance caps): the lantern 2.5 (its 4.3 cm body sank 1.5 cm into the
  crouched thigh while its line kept clear). Hand and forearm limbs: at
  idle the hands sank 2.5 cm into the coat's sides; the boots' feet (foot to ball, 8 cm): the hem went through a toe.
  The ring (`BodySwingRing.cpp`; the user, 2026-10-15: "cape still clips through the boots and shins"): the cloth
  between neighbouring skirt chains (fl-sl-bl-bm-br-sr-fr; the open front not joined) keeps off the limbs too, at the
  midpoint of each level's two points and the middle of the patch down to them (rest distance x 0.95, capped), on the
  shown points only: the push is level, shared by nearness (the nearer point takes more; both taking all of it folded a
  corner another push had already moved right over), at most 12 cm a point. Audit: the stride's shin through the coat
  34 -> 28 points (what's left: the inner back of the right shin between bm and br with the leg far back). Checked in
  PIE 2026-10-15: back standing, walking, sprinting; crouched front and back (drapes round the knees).
  `play.py press Crouch` toggles crouch (the game's setting); `Looter.ThirdPerson.FrontYaw <deg>` turns the front view
  round him (90 his left side, 270 his right) for inspection. A video for the user (2026-10-15): screen-grab the
  editor's game viewport at ~15 fps while held inputs run (System.Drawing CopyFromScreen in a loop; the held `press
  Sprint`/`press Move` sent before the loop), then encode the PNGs with Blender's bundled FFmpeg (`bpy` sequencer,
  H264 MPEG4, fps/fps_base set to the measured rate); scripts kept in the session scratchpad (`record_run.ps1`,
  `encode_video.py`), not the repo. Lessons: console commands from a second process don't reach the game while the
  capture loop holds the window (plan one camera per take); the front view at the default arm length puts the camera
  inside his head (set `FrontArmLength` ~320 first); start where 40 m of ground lies ahead (a take ran into the barn).
  Check a frame from each half before sending. Measured
  in Ellis.py's audit (its copy in step, round4b -> round4c): square to both bone and limb the push went sideways when
  they shared a plane (the stride's back panel slid round the shin's side), so it goes the chain's way out of the body
  (`Outward`), square to the limb and the bone; the extrapolated last tip let the hem 11 cm past it through the ground,
  so each chain ends in a `<chain>_end`, and a last point gathers (at most a third of its length out on the floor, its
  bone shortened along its length axis); the stride's boot through the coat 48 -> 34 points, the crouch's hem under the
  ground 278 points 15.0 cm -> 57 points 4.9 cm (the panel between two gathered chains).
  Tuning table: `FBodySwing::SettingsFor`
  (skirt 7 rad/s, damping 3.5; cape 12, 5; bandana and hair 9, 3.5; lantern 2, 1.2; hook 7, 5.5, gravity 1, bulk 1.5,
  Carry 0.75; longhair 9, 5, Carry 0.35; tails 10, 4, gravity 0.08, Carry 0.6). The kind is the chain's name but for what
  it hangs from (`FBodySwing::KindOf`, 2026-10-20): a `lantern_` chain on a spine bone is a hook (Crane's toll-chain hook
  on his chest; Ellis's lantern on thigh_r keeps its numbers), `hair*` on a neck bone long hair (Crane's back hair and
  front locks; Ellis's hangs from her head), and a skirt with no front chains (no `skirt_f*`: Odessa's six) is a
  tailcoat's tails (Ellis's, Crane's and Pike's coats all have front chains: `Looter.Player.BodySwing.Ellis` checks the
  four). On the duster's settings Odessa's 44 cm tails flew out level behind her through every run (60-77 degrees held
  in the parent's frame): the damping slows the points' own speed through the air, a drag that holds a short chain far
  further out than a long one; now 40-47 (the pelvis's own lean in it, as the long coats' ~50), settled within 0.5 s. `Carry` hands that share of the anchor's own move straight to the points each step (Points and Previous): the
  body starts and stops at about 3 g, which swung the hook out level on every start and stop and kept it swinging through
  his run (74 -> 31 degrees at a run's start, settled 25 -> 1.5 degrees 2 s after a stop; the hair 70 -> 63, settled 4
  -> 0.6). `Looter.Body.Swing 0` holds the animated pose. Lessons (each found by Looter.Player.BodySwing.* and a Python
  copy of the step): a nudge of the position
  toward the target each step is a stiffer spring the shorter the step (30 vs 120 fps ended 7.5 cm apart); chains with
  length constraints swing differently at different step sizes (1/60 vs 1/240 s steps: 20 cm), so it always takes fixed
  1/120 s steps, the leftover carried, and shows the blend of the last two; a push out of a limb must not become speed (it
  flung a chain right over the thigh), nor may a point keep its speed into the limb (2026-10-20: kept, a point held
  against a limb stored a fall into it step on step and slid round the limb on it: Ellis's cape's back wagged 7 cm and
  her bandana 6 cm side to side at a standstill, Crane's cape 14 cm; the push's inward part of the speed is dropped,
  `Looter.Player.BodySwing.Rest`); three loose links are a chaotic triple pendulum (test frame rates on the skirt).
  Hands and forearms (`FLimb::bHand`, 2026-10-20; the Pike agent found the cause on Pike): their clearances are measured
  in the bind pose's A, where they're far from the coat, so a chain hung where the idle hand rests (Crane's front-left
  skirt, Pike's sides) was kept 5-7 cm off a hand lying on it, and the bone push, taking the hand for a limb come
  through to the outside, flipped the cloth out over it: Crane's skirt_fl end stood 29 cm out and up (the flare at his
  left hand). Now a hand or forearm pushes the shortest way (`FBodySwingCapsule::bShortestWay`: never across to the
  chain's outward side; the ring too) and at most `HandGive` 3 cm past where the animation has the point or bone. Crane's
  skirt_fl at idle 27-29 -> 3-10 cm, the flare gone; Ellis's duster as it was (her idle offsets within the run-to-run
  spread, front shots alike). `Looter.Player.BodySwing.Hand`. `Saved/PolishCode/swing_offsets.py <tag>` (not committed)
  lists every chain end's idle offset from its animated pose (Looter.Body.Swing 1 against 0); it varies a few cm run to
  run with the idle's sway, so measure two or three times.
  Watching swing without eyes on it: `Saved/PolishCode/swing_rec.py <tag>` (not committed) records every chain's tip in
  its parent bone's frame through a scripted walk, stop, sprint, turn, strafe, crouch and jumps to a CSV,
  `swing_sum.awk` sums each move's widest and settled angle.
- The cape hangs from spine_05 through its chains, so it kept the arms-out shape it was modelled over (wings in front):
  its corners' targets also ride the upper arms (front 60%, back 40%: `CapeFollowFront/Back`), so it drapes as the arms
  hang and lifts as a gun comes up. The arm they ride is eased on the cape's own bone (`FollowEaseSeconds` 0.35): riding
  it directly, the corners flapped up at the shoulders with a sprint's arm swing (the user, 2026-10-15); a held raise
  comes through whole, a run's swing (about 1.5 a second) at about a third. Round 6 (the user: "Cape clippings"; the
  CAPE agent's model notes above): nine cape chains, `cape_<key><side>` with the key's follow share (`CapeFollowShare`:
  f 0.2 the front, a 1.0 over the arm, s 1.0 the shoulder's side, b 0.5 the back; `cape_bm` follows nothing); the a, s
  and b chains hang from the shoulder joint on the collarbone, so they turn about the arm's pivot. The bind pose's arms
  are raised 31 degrees off the idle, so a chain's rest on the arm (`TipInFollow`) and the upper arm's clearance are
  measured against Ellis.py's unweighted `cape_arm_<side>`, the arm as it hangs at the idle. A chain riding an arm may
  rise to its target's height (`bRisesToTarget`: `KeepUnder` takes a ceiling; held level with its anchor the cloth
  couldn't follow an arm raised to aim and the arm came up through it). Cape `Bulk` 6 cm. Audit: cape cut by the
  sleeves 331 -> 27 face pairs over 16 poses, 0 in every aiming pose.
  Round 6's model (nine cape chains, `cape_arm_l/r`) needs, in Bind and the step (looter_audit mirrors it, marked
  PROPOSED; the user saw the cape cut by the arms aiming, carrying, crouching): each cape chain's share of its upper
  arm's turn by its key, `cape_<key><side>` (the side the name's last letter): f 0.2, a 1.0, s 1.0, b 0.5, cape_bm none;
  `TipInFollow` from `cape_arm_<side>`'s rest (the arm as it hangs at the idle; the cape is draped over arms hanging
  so: from the bind pose's raised arm the idle pulled it 31 degrees into the arms), and the upper arm limb's rest
  segment for every clearance from there too (shoulder joint to the elbow turned with it); a chain point riding an arm
  may rise as high as its target (KeepUnder held the cape level with its anchor: an arm raised to aim came up through
  it); the cape's `Bulk` 6 cm (its chains keep their room from the forearms and hands: a gun's hands in front of the
  chest came through the front panels). Audit (ellis_cape_audit, 16 poses): the cape cut by the sleeves out from
  under the collar 410 face pairs -> 27, by the gauntlets 12 -> 0.
- The forearms (the user, 2026-10-15: "His wrists are either invisible or don't match the gloves"): the retarget rolls
  his hand about the forearm (left about 100 degrees onto a foregrip or the revolver, right about 32; the bend only
  10-20) and nothing rolled the forearm, so the gauntlet stayed put while the glove turned and the wrist tore open (no
  wrist geometry under them). `FBodyForearmTwist` turns each `lowerarm_twist*` bone under a lowerarm by its place down
  the forearm (0 elbow, 1 wrist) times the hand's twist from rest (swing-twist about the forearm axis), before the swing.
  Ellis.py: twist bones and weights spread along the forearm, the glove's cuff run up into the gauntlet.
- The hat (the user, 2026-10-15: in third-person aiming the brim covered the scope; also the brim 12% smaller in Ellis.py):
  `FBodyHatTip` turns the `hat` bone brim-up about the head's left-right axis as a sight comes up (UPlayerViewComponent's
  aim alpha, eased; 18 degrees at full aim), about the back of the band.
- The hat seated, the head closed (round 6, the user: "can see through the head/hat, hat doesn't seat on his head
  properly"; `Tools/Blender/ellis_hat.py`): three causes. The game head was a "crystal": `make_lows` spared everything
  within 3 cm of the eyes and decimated the rest of the head to 0.30, so the skull's back and top collapsed into ~12
  triangles with holes (clay renders draw both sides of a face and never showed it; Unreal culls backs). Now
  `decimate_skin` goes by region (eyelids whole, skull 0.25, skin under the bandana 0.10, visible skin 0.20; eyelids at
  0.75 tore the slits). The crown was one-sided (culled from below): its felt grows 4 mm inward and a leather sweatband
  lines the foot. The hat stood 2-3.6 cm off the skull and 4.4 cm too high at the back (tipped 9 degrees forward about
  the band's middle): `fit` seats the band's foot on a plane from brow + 2 cm to the occipital bump + 1.2 cm (a 4 degree
  rake; level it lost its menace), 2 mm off the forehead, 7 mm at the sides and back (pressed hair), the crown 8-14%
  narrower; the pivot 4 cm in and 3 cm down from the band's back (tipped 20 degrees the band lifts at most 1 cm; before
  3). `band_ring(fig)` gives the hair its line. Check: `artrun -Script Tools/Blender/ellis_hat_check.py -ScriptArgs
  --rig-check,--tag,<tag>` renders with back faces culled on magenta (`Saved/Concepts/EllisHat/<tag>/`). Lessons: check
  one-sided holes with back faces culled; sparing a region from one global Decimate ratio starves the rest.
- The hair's engine side (round 6, the user: "hair looks like mud"; the cards and their script-made atlas: the HAIR notes
  above, `Tools/Blender/ellis_hair.py`): `M_CharacterHair` (`build_world_materials.py M_CharacterHair`; built before the
  import, or MI_EllisHair fails) is masked (clip 0.5), two-sided, the engine's Hair shading; base colour from the atlas
  with a DiffuseAO lerp; coverage from the BC's alpha raised by the mip level sampled (`HAIR_COVERAGE`: a coarser mip
  averages strands with their gaps, so the mask thinned with distance and the hair went bald a few metres off), through
  the engine's DitherTemporalAA material function (`/Engine/Functions/Engine_MaterialFunctions02/Utility/
  DitherTemporalAA`; there is no expression class of that name in Python: a MaterialFunctionCall with its
  `AlphaThreshold` pin); Scatter on the Metallic pin, a strand tangent jittered by the ORM's blue (each strand's random
  id, not metal). Without Lumen, if the Hair shading reads wrong, `HAIR_SHADING = MSM_DEFAULT_LIT` lights the cards with
  the strands' normal map (both sides). After an import check `T_EllisHair_BC` kept its alpha (BC3) and the 50% LOD
  hasn't collapsed the cards. In the golden backlight the Hair shading glowed the hair light brown: Tint 0.55 (set in
  Ellis.py's `EllisHair` slot, which the importer writes to MI_EllisHair; a value set only on the MI is lost on
  re-import). An import that reparents a bone (round 6: cape chains onto the collarbones) logs "Discarding existing
  LODs" and rebuilds them: check `get_lod_count` is 3 afterwards.
- Looking at him: `Looter.ThirdPerson.FrontArmLength <cm>` brings the front view close (110-120); `play.py aim [0]`.

## Materials pass (CHAR-MAT, 2026-10-22; plan `Docs/Work/2026-10-22-CharArt/PLAN.md`, sheets CharArt_1-3)

The user's OKs (2026-10-22): the painted ghost, Gauge back to pick C, the hero value pass, the Gravemother's and Sexton's
own looks. Before/after renders: `Saved/ArtPreviews/Characters/CharMat_*.png`.
- **Painted ghost** (every M_Ghost user; `build_creature_materials.py` M_Ghost, `PAINTED`): new parameters only, master
  defaults = the look, so no ghost re-imports. Cloth: `T_GhostCloth_M` (`Art/Textures/GhostCloth/make_ghost_cloth.py`,
  numpy, 1024, linear: R weave + seams, G hanging folds, B stains) on UV 0 x ClothScale 1.25, multiplying the zone tint
  (ClothWeave 0.18, SkinCloth 0.35 of it on zone 1, ClothFolds 0.40, ClothStains 0.45 toward StainTint #9C8C78); top
  light by pre-skinned Z (0.72 at 90 cm to 1.18 at 165 cm; a slot on a small prop, bounds under PropSize 60, takes
  PropTopLight 1.10: the hat); cavity 1 - 0.95 x G^0.6; rim lerped ColdRim 0.7 toward ColdRimColor #A4E0EE; inner glow
  in the rank colour round CoalAt (10.9, -5.9, 131.3) cm (SK_Unpaid's coal bone; every ghost's coal is within a few cm),
  InnerGlow 1.0 over 26 cm, times the coal's heat; the shroud's fade noise stretched StreakAcross 3 x StreakAlong 0.35
  over FadeBand 0.28 (dithered). Cost: one texture sample and one interpolator more. The Polymer grain stays (Amos's hat
  reads Hay through BaseColorMap). `Looter.ImportModels` of a ghost keeps it (it sets none of these).
- **Painted reflection** (M_Character, `CHARACTER_REFLECTION`): emissive = a painted sky/horizon/ground gradient picked
  by the reflection vector's Z x Schlick Fresnel on the surface's own F0 (8% x Specular, metal its base colour) x gloss
  (smoothstep GlossFrom 0.55 -> GlossFull 0.15 roughness) x occlusion x the sun's illuminance / pi
  (SkyAtmosphereLightIlluminance: dims with dusk; ReflectionFloor 0 = none without a sky atmosphere). Every M_Character
  slot, gated by gloss: leather and rough iron take ~nothing. Not on the cloth, skin or hair masters, nor M_Gun yet.
  Risk: emissive ignores shadow, so glossy parts keep it indoors (check the mine).
- **Gauge**: `gauge_looks.enamel` bakes edges rubbed a step paler and a top light by normal Z (x0.72 down to x1.5 up).
- **M_CharacterGlow** (unlit, opaque): the heroes' eye glows and Gauge's lamp, Color x Glow shaped by |N.V|: a hot core
  toward white, an iris ring, a dimmer body. `looter_export.CHARACTER_GLOWS` maps those Glow slots (EllisEyeGlow,
  CraneEyeGlow, OdessaEyeGlow, PikeEyeGlow x0.7, GaugeLamp Glow 2.2) to it at export, so no hero script changed for it;
  `Tools/Unreal/build_character_glows.py` moves the instances already in the game. (Their pre-skinned normals were no
  use for a forward core: the exported eye balls and the lens keep no clean forward normal.)
- **Ellis's value pass** (`Ellis.py` LOOKS): three value groups: duster mid warm brown (DUSTER, thread paler), waistcoat
  charcoal (darkest, under the bandana), trousers a warm dark grey a step over it, shirt light, belt a lighter tan, hat
  warm dark brown. `FPArms.py`'s OILSKIN must follow the new sleeves (measure EllisDuster after the bake).
- **Crane's skin** (`crane_looks.dead_skin`, Crane.py SKIN (0.48, 0.565, 0.53), ScatterColor #CCD4DA at 0.3): the
  yellow cut, light painted in by the detailed head's normal (`painted_light`: down-facing planes x0.78, up x1.10,
  turned-away x0.90), his zones' darkening mirrored warm (red <-> blue), the lips and nose flush warmed.
  **Odessa** (`odessa_looks.skin_alive`, colour (0.77, 0.65, 0.555)): the same painted light. Both bake with their next
  export (after CHAR-FACE's shape work).
- **The Gravemother's own look** (`Gravemother.py` GRAVE, `grave_marks`, `root_veins`; `looter_barrow.paint_mud`;
  `looter_barrow_stone._granite`): a pale corpse-bone hide (#C2B9A6) with the spider's marks faded to ghosts and the
  rings to dark knuckles, crazed like old bone, root veins from the stone's roots, lichen round its foot, grave earth
  climbing from below (belly, every leg's underside, the lower legs: wet earth #7C6650/#4A3B2E); the headstone's
  granite darker (#605D59-#6A6661) so it reads against the pale hide. MUD_GREY is gone.
- **Sexton** (`MisterSexton.py`): wool and trousers on M_CharacterCloth (#37332F, #413C38; FuzzStrength 0.45, FuzzLift
  0.3, FuzzTint #8C9488: the folds' flanks catch a grey-green sheen), his face still in shadow with a glint: a bead of
  cold light (SextonGlint, Glow #D4ECF2 2.6, r 3.4 mm) in each eye socket.

## Face planes and Crane's hair cards (CHAR-FACE, 2026-10-22; plan items 5-6, the user's OK)

- **Face planes** (`Tools/Blender/looter_face.py`, `carve`): soft bumps and grooves along the skin's normal, placed from
  landmarks measured on the MakeHuman head at rest (eyes, nose tip, lips by the orbicularis oris weights, chin's front,
  jaw's side; dm), mirrored, depths in game metres. Runs in each Figure after the sliders and lids, before
  `world_verts` (Crane: `crane_common.FACE_PLANES`, after `sunken`; Odessa: `Odessa.py FACE_PLANES`, after
  `head_frame`). Only the face moves: the head's own vertices (the head bone and every bone under it in MakeHuman's
  skeleton), nothing within the lids' ellipse `LID` (0.20 x 0.115 dm round each eye, fading in over 0.07), nothing over
  `LID_TOP` 0.25-0.32 dm above the eyes (the hats' bands sit 3.8-5 cm up: their seat and the hair's band line come out
  the same), nothing behind the face's front 1.05 dm (ears, skull). The skeleton's joints are computed before it, so
  no bone moves. Crane (out 3.5 mm, in 2.5): brow ridge, frown lines, cheekbones over hollows, nose-to-mouth folds with
  the cheek pad beside, a sharper nose bridge, the groove under the lower lip, chin, jaw line. Odessa (out 4, in 3.8):
  brow bone, cheekbones over hollows, lower cheeks slimmed, nose bridge and its sides, philtrum, lip lines, chin, jaw.
  What didn't work: a ridge cut off by the lid mask drew a step (keep each feature's own falloff ending before the lids);
  a long thin cheekbone ridge out to the ear read as a welt (broad, 0.09 dm tall, ending at the outer eye's line).
  Check: `face_probe.py`-style clay of the skin alone (the session scratchpad's; landmarks printed).
- The cloth caches (`Crane.drape.json`) take the whole body as a collider, so a face change re-simulates once: the
  coat came out point for point the same (only the cache's digest changed).
- **Crane's hair** (`crane_hair.py`; round 1 read as slab-ended cards and a thin stiff mustache): the atlas
  (`CraneHair`, still ellis_hair's painter) takes Crane's own strip kinds (`crane_hair.KIND`, swapped in like the
  colours): clumps end at 0.42-0.92 of a strip, so every card ends in uneven points, and the underlayer's fill a mid
  grey (dark slabs between the cards before). Cards narrow toward the tip (`taper`: main 0.62, outer 0.70, wisps 0.72 of
  their width, as t^1.3), main and outer lengths more varied, the sheet stops at 0.86 of the drop (its even edge showed).
  The made set is made only when missing: delete `Art/Textures/CraneHair/*.png` after changing the painter or KIND.
- **Crane's mustache** (`MUSTACHE`): four layers a side (under 13, mid 11, over 8, two long droops at the corners),
  rooted along the upper lip's arch, combed mostly down and out by how far out the root is, 3-9 cm, each card puffing
  4 mm off the skin toward its tip and tapering to 30%: the portrait's walrus to the jaw. 660 triangles.
  **Brows** (`BROWS`): 16 narrow cards a side in three layers, roots jittered, up and out at the inner end, drooping at
  the outer; 192 triangles.
- Results (exports repeat bit-identically, the rebake byte-identical): SK_Crane 33,758 triangles (was 33,511), 144
  bones, names unchanged; the hair chain's joints follow the cards (hair_end 4.3 cm, hair_02 2.2, the front locks' ends
  0.6-1.1). Every Crane set rebaked and repacked: the coin rests on the new brow (Gear), the shirt band and collar sit
  off the new neck by about 1 mm (Garb, Coat), a sleeve's Decimate shifted one vertex (Coat), the hair lies on the cape.
  SK_Odessa 34,823 (was 34,821), 92 bones unmoved, only skin and hair shell changed; OdessaCoat/Garb/Gear differ only in
  occlusion near the face (0.03-0.16% of pixels). Crane.py and crane_common.py are in FPArmsCrane's digest: its next
  export rebakes (same looks). Before/after: `Saved/ArtPreviews/Characters/Face_<Hero>_compare.png`.
