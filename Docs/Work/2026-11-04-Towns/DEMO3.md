# Lane DEMO 3: round 2's dressing at the new buildings (2026-11-05 07:53-08:00, PIE, town.json = town_next.json, 338 pieces)

PIE `Tools\pie.ps1 -Map RansomsRest`, God, Main1/Main2 complete, `Looter.Landfall.Ship Anchored`; `play.py at/look/what`
teleports, `grab.ps1` single grabs. Day light only. PIE stopped, nothing saved, no dialog left. (The editor's open map is
Lvl_TutorialIsland, not RR; PlayLevel loaded RR from disk.)

## Per building
- bathhouse (-5275,6275) y-90: PROBLEM. `Handbills_A` (-4974, 6380) lift 145 is sunk into the north wall: `what` at it hits
  SM_RR_Bathhouse's face at (-4972, 6377, 158), so the pivot is ~2 cm behind the face; only paper corners poke through the
  planks (`demo3_bath_bills_close.png`). Fix: the frame's north wall out ~5 cm (or the bills' wall offset). Rest OK: Ladder_A
  leans on the north gable (hit 25 cm off the wall at 2.1 m), CrateStack_A under the west front's left window (door clear),
  CrateStack_B at the NE corner, weeds at the feet, nothing through walls (`demo3_bath_nw/front/ladder/e.png`).
- smithyTown (-875,9775) y-15: OK. Open front (SMITH & WHEELWRIGHT, forge lit) clear; handbills read on the east wall; ladder
  by the chimney; CrateStack_B + straw at the south end (`demo3_smithy_n/e/s/w.png`).
- homeL (-4225,6425) y-90: OK. West door and steps clear, weeds at feet, nothing through (`demo3_homeL_sw/n/e.png`).
- homeB (-1750,50) y0: OK. Rocking chair, broom bucket (breezeway's edge, passage open), washtub sit on the porch
  (`demo3_homeB_porch.png`, `_ne`).
- homeI (-1400,7300) y15: OK. Alley vignette at the porch's end corner, door side clear (`demo3_homeI_ne/s.png`).

## Round-2 fixes
- Pen Lane mouth: one alley pile (store side), no plank crossing (`demo3_penlane.png`). East end: trough + hitch rail where
  the yard road comes in (`demo3_eastend.png`). Both OK.

Not checked: dusk/smog lights, the bathhouse's boiler fire, every weed ring elsewhere.
