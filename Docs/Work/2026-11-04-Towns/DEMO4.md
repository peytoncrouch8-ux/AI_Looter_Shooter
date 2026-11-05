# Lane DEMO 4: the east row on the yard road + the barge's starboard walk (2026-11-05 09:54-10:03, PIE)

PIE `Tools\pie.ps1 -Map RansomsRest`, God, `Looter.Mission.Complete Main1 all` / `Main2 all`, `Looter.Landfall.Ship Anchored`;
`play.py at/look/lookat/what` + single `grab.ps1` grabs. `play.py at` needs a low z (400) here: from z 3000 it lands on
Landfall's barge over the Sink. PIE stopped, nothing saved, no dialog left (only the editor's main window).

## The east row (homeK, homeM, homeO, shedC, smokehouse, privyB): OK, nothing blocks the package
- Reads as the town's continuation: from Main Street's east end K's porch lines up with the street's porches
  (`demo4_eastend_row.png`, `demo4_homeK_side.png`); from the yard road the three fronts face the road, spacing even, the
  dray and lumber stacks sit as the yard's own clutter (`demo4_yardroad_row.png`, `demo4_homeO_front.png`).
- Sills/foundations on the ground; porches on skirts with steps to the dirt; nothing through walls seen. Porch pieces sit on
  the decks behind the rails (K washtub, M chair + tub, O chair; `demo4_homeK_porch.png`, `demo4_homeM_porch.png`). Weeds at
  porch/wall feet, none through a deck seen.
- Back yards (`demo4_backyards.png`): M's laundry line, both posts in the ground, sheets hung; firewood at M's back wall;
  shedC and the smokehouse (log, smoke) stand on the ground; the privy too (`demo4_outbuildings.png`, from beside O's north wall).
- Smoke rises from K's stone chimney and M's chimney (`demo4_k_smoke.png`, `demo4_m_smoke.png`: `lookat Smoke_homeK/M` lands
  on the chimney tops), O's and the smokehouse's too.
- Yard road (layout: (50,7200)->(260,10400), 600 wide) open; the row stands east of it, the depot (865,11400) north of O:
  nothing on the road or the way to the platform.
- DuskSmog: K, M, O windows lit (curtained warm panes), porch lamps lit with a pool on the decks
  (`demo4_dusksmog_row.png`, `demo4_dusk_homeK.png`, `demo4_dusk_homeO.png`). Back to Day after.
- Not checked: K's chair and tools up close, every weed strip, shedC's door side, the TownLife households' sounds.

## The barge's starboard house walk (`Looter.Landfall 4`, `Looter.Barge.Show 3 aboard`)
- Barge_Pilot at (7492, -2518, 3251) yaw -63 in this state; teleports to barge-local (2100 / 1750 / 1350, 865).
- The strap is gone: `what` rays from the walk's mouth (local x 2100) aft at 1.3-1.8 m meet nothing for 500 m; at pitch 0
  the first hit is the hull at local x -1816 (`demo4_barge_stbdwalk.png`, aft; `demo4_barge_fore.png`, fore: walk open to
  the well deck).
- Look problem, not a blocker: at local x ~1250-1700 both sides of the walk are jagged faceted faces up close: black shards at
  the house wall's foot (0.5-1.5 m; `what` hits at y 709-743, inside the wall line) and brown shards along the bulwark
  (`demo4_barge_x1600.png`). The floor between stays clear. Likely Tripo hull roughness or box-cut edges; for REWORK/the user.
- The barge bobs: the player's z read 5509-5672 over a few seconds, so local z here is approximate.
