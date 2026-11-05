# Lane DEMO 6: the barge's rebuilt side walks + Glebe Corner's lane (2026-11-05 ~13:20-13:36, PIE)

PIE `Tools\pie.ps1 -Map RansomsRest -Commands God`, `Looter.Mission.Complete Main1 all` / `Main2 all`, `Looter.Landfall.Ship Anchored`,
`Looter.Landfall 4`, `Looter.Barge.Show 3 aboard`. Barge-local teleports and rays by a scratch script (the nearest visible
SM_DredgeBarge component: BargeBoarding_1's `Hull` at (7492, -2518, 3251) yaw -62.76; a second one, StoryProp_63 at (4600, 6500), is not
the deck's). Leaving the barge: `Looter.Barge.Story` + `Looter.Landfall.Ship Anchored` again (`Looter.Landfall 4` sets her Moored, and
teleports snap back to the deck until then). The floating Content Browser window covered the viewport: moved aside by SetWindowPos for
the grabs, put back at (457, 217). PIE stopped, nothing saved, only the editor's windows as found, git tree clean.

## Barge walks: better, but holes at the starboard house wall's foot
- The walls read as a ship's passage now: straight plated walls, rivets, the strake and head flashing lines, the bulwark's riveted band
  (`demo6_barge_x1600.png` starboard from x 1800 aft; `demo6_barge_port_x1600_aft.png`; `demo6_barge_casing_port.png` from x -900 aft:
  clean). DEMO4's black/brown shards at eye level are gone.
- **Holes at the starboard house walk's wall foot, local x ~1250-1450, y ~660** (the wall/floor seam): bright patches where the ground
  shows through (`demo6_barge_stbd_foot_x1400.png`, from (1520, 840) looking down at local yaw -130; `demo6_barge_x1600_footzoom.png`).
  The eye's ray at the patch passes the barge and meets Dressing_StoneWall_B in town, 97 m down. Visible to a player who looks down
  at DEMO4's spot. The walk's floor box holds the player: no fall.
- Ray scan (visibility, complex) from eye height onto both house walks' floors and wall feet, every 50 cm of x: many rays slip through
  the seam and land 60-170 cm UNDER the floor: starboard x 130, 250, 700-850, 1330, 1870-2180; port x 130-250 (two out to the sky at
  x 200, y -406..-446), 700-850, 1330, 1620-2180 (most). Horizontal rays at z +3..+95 find no wall at port x 1600 and starboard x 1750
  / 1850 (the wall is drawn there: its collision or its lowest strip is missing). So the seam is open in more places than the one seen.
- Above head height the house's old Tripo mass still bulges out over both house walks (soft lumpy brown, ~2.5-4 m): reads as
  superstructure, not shards; a look note only.
- The bulwark's posts are plain faceted blocks (the dark slab beside the player in each view): low-poly next to the new plating.
- A raised spot on the starboard walk: the floor trace at (1800, 865) landed at local z 2299 (40 cm over the walk's 2258); not looked at.
- Not checked: the starboard casing walk (no view, no scan), the casing walks' seams by ray, z-fighting up close.

## Glebe Corner: the road is there, the sign isn't readable
- From the lane's mouth (`demo6_hamlet_lanemouth.png` from (4100, -6150); `demo6_hamlet_road_sign.png` from (4250, -5950) yaw -30):
  a trodden dirt band runs from the mouth past the lantern post to the boarding house: you can see where to walk now. It is subtle
  (darker dirt and pebbles, soft edges); it reads best looking down, little from eye level farther off.
- Reads as a hamlet more than DEMO5's "dropped in a field": the road, the lantern post, the boarding house's lit lantern and smoke, the
  burnt chimney and the dark homes along it. Still thin at the mouth: the near ground is empty, the fallen pine lies along the lane's
  right (no longer across it).
- **Signpost (4583, -6338): unreadable.** Two small boards (red, teal) high on the pole; from ~5 m at 1080p the letters are a smudge
  (`demo6_hamlet_signzoom.png`). Bigger boards and letters, lower, facing the road.
- Nothing floating or through anything seen in these two views. Not checked: the fences, kitchen garden, hand pump, hitch rail and
  trough up close (not reached in the time).

## Blocks the package?
- Nothing breaks play. The barge's seam holes are visible at the spot this round fixed; Main's call whether to ship them in build 4.
