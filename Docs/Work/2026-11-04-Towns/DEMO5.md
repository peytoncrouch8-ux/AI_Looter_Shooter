# Lane DEMO 5: Glebe Corner (the hamlet) + the east row's back (2026-11-05 11:40-11:48, PIE)

PIE `Tools\pie.ps1 -Map RansomsRest`, God, `Looter.Mission.Complete Main1 all` / `Main2 all`, `Looter.Landfall.Ship Anchored`
(without it every teleport puts the player back on the Lamplighter's deck); `play.py at/look/what/goto` + single `grab.ps1`
grabs. PIE stopped twice, nothing saved, only the editor's main window left, git tree as found.

## Blunt read: a cluster of houses in a clearing, not yet a crossroads hamlet
- From the west road (`demo5_lane_mouth.png`, `demo5_from_knoll.png`) it reads as a place, mostly from the boarding house:
  smoke, the ROOMS sign, a lived porch. P and N dark and shut, sheds, the burnt chimney beside it.
- What makes it look dropped in a field: **no lane on the ground.** The grass and dirt are even from the road to the porch; the
  lantern post is the only sign of a way in (town.json `lanes` isn't painted; GROUND's trodden paint not done). No fences, garden or
  yard edges. The pines stand behind and beside, not round it, and the boundary cliff (Cliff_boundaryFoot_90) shows right behind
  the boarding house's yard: the clearing reads as a dead-end pocket.
- Fix order (none blocks the package): the lane's trodden paint/ruts, the log below, a fence line or garden plot by P, the root
  cellar out of the front yard.

## Problems with positions
- **A fallen pine lies across the lane** at about (5233, -5420), 6.7 m in from the road mouth and 2.7 m from the lantern post: layout
  obstacle `westRoadLog` (kind props, (5100,-6000)->(5260,-5300), 80 cm high, spiky branch stubs; `demo5_lane_in.png`,
  `demo5_from_knoll.png`). The lane (4716,-5000)->(5700,-5800) wasn't kept off it. Move the lane's first stretch north of the
  log's end (5260,-5300) or move/shorten the log.
- Boarding house's hitch rail (7584,-6483) and trough (7608,-6595) stand about 2 m in front of the root cellar's door, between
  the cellar and the porch's north end: nothing through anything, but crowded, and the cellar mound hides them from the lane's
  end (`demo5_boarding_yard.png`; `what` meets the cellar's face at (7361,-6501)).

## OK
- Boarding house (`demo5_boarding_front.png`): sill and porch on the ground, bench, chair, washtub on the deck, ROOMS sign, a lamp
  by the door; back wall's stone foundation meets the ground, back door just above it (`demo5_boarding_back.png`). The back yard is
  bare (the laundry, second woodpile and block were held out, round 7).
- Home P (`demo5_homesPN.png`): dark, boarded windows, broken porch boards, stone chimney on the ground, nothing through it.
- Lantern post: stands upright at the road's edge with its lantern, unlit by day (`demo5_from_knoll.png`).
- DuskSmog (`demo5_dusksmog.png`, from (5300,-5600)): the boarding house's upper south window lit (curtained warm pane, window
  life on); P dark. N not in that frame.
- East row's back (`demo5_eastrow_back.png`, M's yard from (2500, 8000)): laundry line, chopping block, firewood, weeds and grass:
  not a bare field. No tree through a house seen.

## Not checked
- Home N up close, P's tipped chair, the sheds' door sides, the root cellar's door, the boarding house's front lamp at dusk.
- The east row from end to end: two teleports to the yards' north edge ((3400, 8000), (2950, 7400)) dropped into the Sink (z -245,
  -380), so the yards end at the Sink rim (fence on top) at about X 2700-2900; the eastRow NoTrees box (to X 3050) reaches over the
  rim (harmless). `demo5_eastrow_wide.png` is a wall close-up: ignore it.
