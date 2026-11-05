# Lane DEMO: the TownGate fight with homeB, and both new houses (2026-11-04 night, build f3bd38c7)

PIE `Tools\pie.ps1 -Map RansomsRest`, God, `Looter.GiveWeapon Rifle Common 1`, `play.py ammo`. Story: `Looter.Mission.Complete Main1 all`,
`... Main2 all`, `Looter.Landfall.Ship Anchored` (without it every teleport off the ship is sent back: "off the Lamplighter while she
flies"), `Looter.Mission.Start Main3 1` (TownGate Off -> Engaged); later runs `Looter.Encounter.Wave TownGate force`. Scratch tools (not in
the repo): `house_probe.py` (editor world: ground round each house vs its pivot, floor traces, every ISM instance in/near the footprint)
and `umon.py` (every 0.5 s: each creature's spot, distance, bearing off the player's view, speed, cm outside homeB's box, flags
NEAR_HOMEB < 80 cm / INSIDE / between homeB and Home_J / STUCK). The level was already dirty (`Lvl_RansomsRest*`, "1 Unsaved") when I
started; nothing saved. PIE stopped at the end; a Message Log window is left open (not modal).

## The gate fight (four runs, 11 Unpaid risen)

| Run | Player | How the men fell | Where the Unpaid rose | Arrival | Nearest any Unpaid came to homeB |
|---|---|---|---|---|---|
| A | (0,-2700), still, no shooting | `play.py kill` (all 4 at once) | the 3 rifle posts (330,-560), (0,-620), (-330,-560) | 22 m out, bearings -14/-6/+2 (dead ahead) | 6.2 m (the Rare one rounding the south sandbags: 2.5 s detour east by Home_J's west wall, then a blink out) |
| B | (0,-1300) at the gate | `kill` | the same 3 posts | 8-10 m, bearings -25/-6/+16 | 9.6 m |
| C | (0,-2500), `play.py fight 60 40 0.5 strafe` (a real fight) | one by one | (0,-620) post, (-150,-900) the hose-man's spot, (330,-560) post | 13-18 m ahead (+11, +9; the third rose while the bot faced an attacker) | 10.1 m |
| D | (-1300,-1600) | `Looter.Unpaid.Rise 2 0` (8 m in front, by the house) | (-707,-1062), (-872,-916) | 8 m ahead (-21, -5), straight in | 7.0 m |

- The rise spot by homeB, `UNPAID_SPOTS[3]` = world (-1300,-800), was **never used** in 3 real waves (9 rises): the spawner takes its
  points in order and the dead get the fallen riflemen's posts and the hose-man's spot first (4 out at most, 8 points). It is reached only
  if the first 7 are all occupied or refused.
- No Unpaid inside, against, or between homeB and Home_J in any sample (no NEAR_HOMEB, BETWEEN or STUCK flag); none clipped it.
- Blinks: run A 10 in a minute (budget 15). Run A's standing player got circled (melee ring, flanks at 90): bearings behind come from the
  player never turning, not from blinks behind.
- Not seen: the house-side spot used by a real wave; a player fighting with their back to homeB's porch (the only way the house would be
  in the Unpaid's path). The fights are judged from the bot and the log numbers, not by hand.

## homeB (SM_RR_Home_B, (-1750, 50, -8.2), yaw 0, tags Obstacle)

- Ground 35 cm outside the walls vs the pivot (sill): front -12.6..+24.9 (mean +4), back -14.8..+15.6, east side +6.5..+24.4, west side
  -25.4..-10.3; terrain under the floor -19.4..+18.8. The mesh goes 35 cm under the pivot: no gap anywhere (worst 9.6 cm of foundation
  left buried at the west side); the NE corner's ground stands 25 cm over the sill (inside the 50 cm rule).
- The porch and breezeway floor is +45 cm over the pivot (traces at the door, 60 cm in and out, the middle). Walked: from (-1300, 50)
  facing south, `press Move 2.5` put the player at (-2514, 50): up the porch, through the breezeway and out the back, no snag. A strafe
  in the breezeway stops at Y 138 (the cabin wall blocks).
- Scatter: no instance inside the footprint; nearest grass 41 cm off the walls (GrassClump A/B), TallGrass 45, Clover 54; the stock-pen
  fence 266 cm off the back; no tree within 12 m.
- Looks (grabs): from the gate road it is the log cabin with the stone chimney seen through the arch, behind Home_J; from its porch the
  dogtrot reads right (two cabins, open breezeway, the pen fence through it). Reads as part of the town.

## homeI (SM_RR_Home_I, (-1400, 7300, 42.6), yaw 15)

- Ground outside vs the pivot: front -15.6..+2.2, back -3.3..+6.4, sides -14.9..+3.8 and -0.8..+2.8; under the floor -11.8..+3.1. No gap
  (mesh 35 cm under the pivot), nothing buried over 7 cm.
- Scatter: nothing inside; nearest grass 58 cm (TallGrass), the woodpile 179 cm; no tree within 12 m.
- From Main Street's east end (100, 7150) it stands left of Bright & Daughter, dark (`dark`, as planned), and reads as an empty house of
  the street. Not walked inside (it is shut and empty).

## Grabs (`Saved\Screenshots\Tools\`)

`demo_gate_approach.png` (the fight's start from the road), `demo_gate_rise.png` (at the gate), `demo_homeB_rise.png` (run D),
`demo_homeB_porch.png`, `demo_homeB_breezeway.png` (out the back), `demo_homeB_from_gate.png`, `demo_homeI_from_street.png`.

## Side notes (not the houses)

- Every teleport off the Lamplighter after `Mission.Complete Main1 all` was sent back until `Looter.Landfall.Ship Anchored`: a dev-path
  quirk, not the player's.
- The hose-man reached 3.9 m behind-right (+116) of a still player in run A, before any Unpaid rose (the Dredger, not the Unpaid).
