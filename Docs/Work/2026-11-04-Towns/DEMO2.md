# Lane DEMO 2: the dressed town (299 pieces), the new buildings, Main 3's street, hits (2026-11-05 morning, build f4a0a3c7)

PIE `Tools\pie.ps1 -Map RansomsRest`, God, `Looter.Mission.Complete Main1 all` / `Main2 all`, `Looter.Landfall.Ship Anchored` (else every
teleport is sent back to the ship), `play.py at` teleports + `grab.ps1` crops. Lights: default (Day), `Looter.Light Smog` (no visible change
in 4 s), `DuskSmog`, back to `Day`. Fights: `Looter.Mission.Start Main3 2` (TownGate engaged at once), `play.py fight 50 40 0.8 strafe`;
the street: `Looter.Encounter.Wave MainStreet force` at (0,1200) (walking in to (0,900) did NOT start it in 70 s), `fight 55 45 0.7 strafe`.
Rifle Common 5 only (`Looter.GiveWeapon Shotgun Common 5` given, not fired). PIE stopped; nothing saved; a Message Log window (not modal)
left open. Git shows Bugs.md + two barge Blender scripts modified: not mine.

## Dressing (day, grabs below)
- Reads as placed, mostly: store's west wall handbills at ~1.45 m sit on the wall; alley junk (barrels, crate, planks) at back corners;
  hotel porch empty-house set (bottle crate, barrel, fallen boards, ring on the hitch rail); undertaker's crate stack by the walk's end;
  canvas wagon (-400,4985) parked clear of the lane, wheels on the ground, LampPost_C 1 m off its tail; water cart (-380,450) fine.
- From the street middle the clutter is small and reads sparse, not noisy: it lives on porches and corners.
- Problems (worst first):
  1. Pen Lane mouth: the same Vignette_Alley_A twice, facing each other across the lane (store (-1365,2508) y270, cabinSouth
     (-1215,1607) y90): mirrored, reads copy-pasted. Grab `demo2_v_penlane.png`.
  2. PlankCrossing_A on dry dirt: Pen Lane (-1300,2060) lies mid-lane crossing nothing (no rut/wet paint yet): reads as a dropped pallet.
  3. East-end street pile: CrateStack_A (430,6420) + BarrelStack_A (400,6700) stand in open street by lamp 04, no door or wagon behind
     them; plausible freight drop, weakest "why is this here". Grab `demo2_c_eaststacks.png`.
- Nothing floating, sunk or through a wall/walk seen in the spots checked; no handbill in air; lamps lit at DuskSmog (LampPost_B Gilt
  Spur corner, LampPost_C by the wagon, the east lamps). Not checked: all 114 weed rings, the restored walks' facade step up close,
  the Smog light (looked like Day).
- Side issue (not the town): the Lamplighter's hanging line over the back lots draws as ~5 separate tube pieces with gaps between,
  floating in the sky (`demo2_sky_logs.png`, from (2100,1500) looking east-up). Dev path (`Landfall.Ship Anchored`); check in a real run.

## Main 3
- Gate (TownGate, 7): cleared by the bot in < 45 s, step 3 reached.
- Street set piece (push 1: 4 Unpaid + Bell-Ringer, push 2: 5): Unpaid rose in the street and reached the player; 12 seen; the player
  never hurt (God). `phase-steps` (stuck -> blink nearer) 10 times in ~60 s: Unpaid 0.1-4.0 s stuck (7 of them), Bell-Ringer 3.2-7.2 s
  (3; it starts on the saloon balcony). No UNPAID BLINKS budget line. Positions of the snags not logged (no monitor run): can't say if
  the new pieces caused them; no before-number for this fight. Street lane looked clear in every grab.
- Cover and spawns: not judged by hand.

## The new buildings
- homeL (-4225,6425): cabin on its stone foot, steps to the ground, nothing through it; chimney smoke; reads as a town house.
  `demo2_b_homeL.png`.
- bathhouse (-5275,6275): "BATHS LAUNDRY" sign, sits on the ground, chimney smoke; boiler fire not seen from the front (NW). `demo2_b_bathhouse.png`.
- smithyTown (-875,9775): forge fire glows inside the open side from the yard road (300,7600) (`demo2_v_yardroad.png`); from the north
  (`demo2_b_smithy2.png`) a dark shed with a sawhorse, fire not visible. The first try put the camera inside a wall at (-250,9250).

## Hits (Unpaid, rifle, bot aim 0.7, one fight)
- 63 hits on Unpaid/Bell-Ringer by bone: pelvis 10, upperarm_l 9, tail_01 9, spine_02 6, coal 5, tail_02 5, upperarm_r 5, spine_01 5,
  hand_l 3, neck 2, lowerarm_r 2, jaw 1, hand_r 1; 10 CRIT lines (x2.00, the coal). 224 world hits (the bot's random misses).
- The Unpaid's crit is the coal (UnpaidCreature.h), not the head: jaw/neck hits are x1.00 by design. Coal crits fire.
- Not checked: spiders, the shotgun, a hit landing through a body by eye, the crit number popping on screen.

## Grabs (`Saved\Screenshots\Tools\`)
`demo2_ms_600_e`, `demo2_ms_2600_e`, `demo2_side_n0/s0/n2200/s2200`, `demo2_c_storeW`, `demo2_c_hotelporch(2)`, `demo2_c_wagon`,
`demo2_c_eaststacks`, `demo2_c_undertaker`, `demo2_v_penlane`, `demo2_v_backlots`, `demo2_v_yardroad`, `demo2_sky_logs`,
`demo2_light_smog`, `demo2_light_dusksmog(_east)`, `demo2_gate_fight`, `demo2_street_rise`, `demo2_street_fight1/2`,
`demo2_b_smithy(2)`, `demo2_b_bathhouse`, `demo2_b_homeL` (.png).
