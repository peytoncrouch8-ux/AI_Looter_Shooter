# Melee tool concepts (CONTRACT3 note 5, 2026-10-27)

The user picks one tool per hero before anything is modeled. Each tool is held and swung in the right hand; the gun drops
to the left during the swing. Original designs only. Feel tags: QUICK (fast and light), STEADY (middle), HEAVY (slow
wind-up, hard hit), REACH (long).

Sheets (1600 x 1000): `MeleeTools_<Hero>.png`. Rebuild: `node Art/Concepts/MeleeTools/sheets.js [Hero...]` (Node and
Playwright as for `Tools\portraits.ps1`). The drawings, captions and swing arrows are all in `sheets.js`.

## Ellis Ransom, the Revenant
- **A Grave Spade** (95 cm, STEADY): Sexton's spare spade, the one that filled their grave. Flat-blade backhand swat, high
  right to low left. Black iron blade worn bright at the edge, ash shaft and D-grip, an ember crack glowing cyan.
- **B Ransom Brand** (80 cm, STEADY): the family ranch's branding iron, kept hot by their own ember. Overhand chop, brand
  first, straight down; a hit leaves a glowing mark. Iron rod and ring brand (an R), walnut grip, brass collar; the head
  burns cyan-white.
- **C Drover's Bowie** (40 cm, QUICK): a cattle drover's knife. Quick forehand slash, right to left at chest height.
  Clip-point steel blade, brass guard with a knuckle bow, stag-horn grip.

## Odessa Lark, the Cardsharp
- **A Razor Fan** (5 cards, QUICK): her steel-edged trick cards, the deck Fifty-Two Pickup throws. Backhand flick, low left
  to high right; the fan snaps open mid-cut. Gold-rimmed cards with honed steel edges, fanned from a black glove.
- **B Croupier's Rake** (110 cm, REACH): the card table's rake, honed sharp. Wide forehand sweep, right to left at waist
  height. Ebony rod, gold ferrules and pommel, brass rake head with a steel lip.
- **C Spade Cane** (85 cm, STEADY): a walking cane with a gold spade head, held upside down. Short overhand rap, straight
  down onto the crosshair. Ebony shaft, cast gold spade set with a garnet, silver ferrule.

## Hollis Crane, the Unpaid
- **A Ferry Gaff** (120 cm, HEAVY): a ferryman's boat hook for the man owed his fare (rhymes with the toll chain).
  Overhand hooking chop, then a yank back toward him. Weathered grey ash pole, black iron spike and hook, a fare coin
  nailed to the pole.
- **B Coin Sap** (30 cm, QUICK): a lawman's sap stuffed with the coins of fares never paid. Short overhand cosh, high right
  to the centre; the coins clack. Cracked black leather, split seam showing coins (faint cyan inside), a riveted fare
  coin, wrist strap.
- **C Marshal's Billy** (60 cm, STEADY): his old lawman's club, forty years of notches in the grip. Forehand clubbing arc,
  right to left and a little down. Turned hardwood, iron end cap, a tin star set in, leather thong.

## Gauge, the Iron Hand (three-fingered hand)
- **A Spike Maul** (110 cm, HEAVY): the track-layer's own hammer. Big overhead drive, straight down; slowest wind-up,
  hardest hit. Hickory handle with leather wrap, narrow iron double head stamped No.7, faces glowing from the firebox.
- **B Track Wrench** (90 cm, STEADY): the railroad's fishplate wrench, sized for a machine's hand. Forehand haymaker,
  wide arc right to left. Forged iron bar, brass-lined open jaw, box-end ring at the foot.
- **C Piston Fist** (built in, QUICK): no held tool; the right forearm's piston (the one behind Rail Spike) drives a
  knuckle plate. Straight jab into the crosshair; fires and vents steam on a hit. Riveted iron knuckle plate, brass
  piston and cylinder. (For HANDS: needs a forearm attachment, not a grip mesh.)

## Wendell Pike, the Surveyor
- **A Rock Pick** (35 cm, QUICK): the surveyor's field hammer for samples and marker stakes. Short overhand peck, pick
  first, down and left. Steel head (square face, curved pick), hickory handle bound in brass wire, a teal band.
- **B Tripod Leg** (120 cm, REACH): one leg off his transit tripod, the spike that stakes his Tripwire. Forward thrust
  into the crosshair, then a short pull back. Red-and-white banded survey pole with marks, brass clamp and shoe, steel
  spike.
- **C Plumb Bob** (1.2 m cord, STEADY): his brass plumb bob on its line. One whirl overhead, then a whip across, right to
  left. Solid brass bob with a steel point, waxed cord on a walnut spool. (For HANDS: a cord and a swinging weight, not a
  rigid tool.)

## Tripo upload views (2026-10-27)

The user remodels the five picked tools in Tripo (paid plan, multi-view image-to-3D). Each tool's folder
(`GraveSpade`, `RazorFan`, `FerryGaff`, `PistonFist`, `RockPick`) holds four renders of the game model:
`upload/` (transparent PNGs: upload these) and `upload_white/` (the same on pure #ffffff), each `front`, `back`, `left`,
`right`.

- Renders of the existing models (`Art/Models/Weapons/Melee/MeleeTools.py`: game materials, textures, baked occlusion),
  stood upright (long axis vertical), orthographic Cycles, 1024 x 1024, 96 samples, denoised, same scale in all four
  views of a tool, the tool ~85% of the frame's height (the Razor Fan is limited by its width), centred; even soft
  studio light riding with the camera, no ground or shadow, Standard view transform (true colours). Glow parts (cyan
  ember crack, orange knuckle caps) show at about a quarter of their game strength so they keep their colour.
- Front: Grave Spade the blade's face (crack on both faces), Razor Fan the card faces (the five cards stay fanned),
  Piston Fist the top (cylinder, rod, yoke, knuckle plate; knuckles up), Ferry Gaff and Rock Pick in profile (hook and
  pick curl to the image's right, hammer to the left). Left shows the tool's own left side, right its right; if Tripo
  mixes them up, swap `left` and `right`.
- Rebuild: `Tools\artrun.ps1 -Script Tools\Blender\melee_views.py [-ScriptArgs GraveSpade,RazorFan,--samples,300]`
  (`Tools/Blender/melee_views.py`; flags `--samples --size --fill --glow --out`).

Tripo steps: 1. Image to 3D, **multi-view**: front = `upload/front.png`, then left, back, right from the same folder
(if the result comes out flat or doubled, try `upload_white/`). 2. Generate, then **Smart UV**, then **Texture** (PBR
if offered). 3. Download the **GLB** and save it in that tool's folder (`Art/Concepts/MeleeTools/<Tool>/<Tool>.glb`,
beside `upload/`).
