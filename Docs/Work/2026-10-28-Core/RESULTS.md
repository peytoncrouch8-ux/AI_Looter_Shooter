# DEMO play-check results (2026-10-29 ~01:00, Sonnet agent, PIE RR, ~25 min, code untouched)

Setup: `Tools\pie.ps1 -Map RansomsRest`, Epic L5 guns by `Looter.GiveWeapon`, play.py `press Fire/aim/equip/fight`, Fight.Sim runs
with God off (the sim arms a level-3 player: Uncommon AR + Common Ward 45). Log clean: no LogLooter warnings/errors in ~7000
lines (only IA_Crouch "failed to find" from play.py's own press helper and my own bad hits-test args). 50 SOUNDBANK warnings at
04:33 are cues whose names live in CreatureHitCues.h (known own-header case, not a bug).

## 1 Guns: PASS (one gap)
- Fired hip + ADS + tapped: Rifle, Shotgun, Revolver (PIE 1), SMG, Sniper, LMG (PIE 2; GiveWeapon fills 3 slots, extras go to the
  backpack so two PIEs). Reloads, equips (Cup fit lines), 80+ Hit/Impact lines each, no warnings. Launcher not tried.
- Identity 0 then 1 (rifle, revolver, shotgun): commands take, fire and reload fine, log clean. Feel itself not judged (no
  recording): single grabs of shotgun ADS right after a shot (view centred, sight arrives), sniper scope after a shot (centred, no
  clip), LMG after a 2 s burst (view has climbed to sky: sustained LMG climb ~30+ deg without pull-down, check by hand).
- Sounds: au.Debug shows only ambience at grab time (shots too short), no log line per cue; cues.json has Punch + AimIn for
  Rifle/Shotgun/Revolver/Sniper/LMG/Launcher and AimIn for SMG, NO `Weapon.SMG.Punch` (ISSUE, known, fix running). By-ear not possible.
- Launcher not exercised; ADS lock under heft looked fine at shotgun/sniper (one frame each only).

## 2 Hits: PASS with one ISSUE
- Spawned 3 Spider + 3 Unpaid + 2 Slime chase (God on), rifle `fight`: 123 hit lines in 17 s, 8 kills in ~17 s, zones named
  (head/leg/arm/abdomen/core/shroud/body), weights body/heavy/big/overkill/kill, `Looter.Hits.Show 1` labels show zone, poise, wound,
  stagger. Kill styles logged (Unpaid style 3). Slime core crit logged (CRIT, core), overkill on slime drops.
- Unpaid head kill (Hits.Test head 400 crit Unpaid, slomo .3): the hat is off and lies on the road (grab hat5), body gone. PASS.
- ISSUE stagger never triggers: ~150 rifle hits and Hits.Test body 70/110 (38-59% of health, "big") on Unpaid, 60 x3 on a spider:
  every hit = "slowed to 55% for 0.1 s" flinch only, no "staggered", poise peaked 0.20 against the 0.25 shown by Hits.Show, mostly
  0.00-0.02. Not tried with shotgun/sniper (they were not carried). So no stun-lock, but also no stagger seen from rifle/SMG.
- Minor: 25 Unpaid hits in the first fight logged "body hit on the body (bone None), 0 cm off axis, poise 0.01": hits landing
  on the capsule (the `fight` bot aims at the middle), so those get no zone. Probably tool-only; worth a glance if a real shot can do it.
- Hits on Unpaid whose head was hit by Hits.Test: "weight 4 strength 1.00" fine; low-damage head hit (1 dmg) = flinch only.

## 3 Fights: PASS on difficulty, one note
Sim arms a level-3 player with 116 HP + 45 Ward; Basic Unpaid rank is level 3 (Sim level arg 1 raises it).
- Unpaid 6 Basic L3 leader, standing still: player died at ~10 s (fight length 10.4 s; target 8-12). 15 attacks started, 11 landed
  (73%), 175 damage in (116 to health after the ward), peak 23.6 dps in 5 s, melee tokens 3 max, one Ease event.
- Unpaid strafing + shooting (bot aim 0.6): 30.1 s, 6 of 7 killed, no death, lowest EHP 63%, 34 attacks started / 11 landed (32%),
  150 damage in (only 15 to health: the ward and its recharge ate it), median time to kill 18.8 s (bot-limited), FadeBack 1,
  LeaderDown 1. Reads "comfortable" for a competent mover: at the low end of the "30-70% health" goal (lowest EHP 63%).
- Slime 6 Restless L2 leader, standing: died at ~15 s (17.4 s incl. lead-in), 11 of 14 attacks landed, 158 damage in. Strafing
  run 1: 10 of 19 killed, lowest EHP 72%, 0 damage to health, Split 1, LeaderDown 1, Dodge 2; strafing run 2: died (1 death) at
  64 s, 6 of 13 killed, Split 3, peak fighting 8, 52 attacks / 22 landed. Splits DO happen. Count by probe: alive peak 8-9 (cap
  holds; the 16 first seen was leftovers of the previous sim). Slimelets die in ~1.6 s median.
- Note: play.py `kill 150` also kills AbelKeeper_0 (a story NPC; he comes back); harmless in PIE, nothing saved.
- Spiders left alone as ordered.

## 4 Finds: PASS with two ISSUES
- `Looter.Finds.List`: 15 finds + 6 ledger pages, regions 4/3/2/3/3, all sources found (chests closed, Tollman creature).
- BoltHole: tell + lantern readable from the front (grab bolt2 shows dolmen gap, glowing lantern, purple chest inside). Standing walk
  stops 1.3 m in at the ceiling slab; crouch-walk (press Crouch + Move) reaches the hollow. Chest: tap Open, gave Rare revolver + Epic
  sniper + ammo + 57 silver, flag set, "1 / 15", FOUND banner, +374 XP. PASS.
- Tarn: stake + tag + rope readable on the north shore ([E] READ shows, tag read logs). Shore wade works (Swim in/out, Hob line
  "The dead won't follow into deep water"). Chest at the bed: opens (Open the chest, Epic SMG + Rare ward + ammo + silver, "Still
  Water" named gun, 3 / 15, +605 XP). Underwater view is murky dark green: the box/glint was not visible in my grab (not
  found by sight; may be fine at the real dive depth, check by hand).
- RangerPost: five flat steps (+109 cm each) climbable by walking + jumping to the deck (z 893 reached in one try, Jump taps every
  1.6 s), chest and note on top; the stack reads as a stair from the south. Crows: RoostCrows_5 circles 7 m over the chest, one
  visible as a small black speck high in the sky from the deck (grab rp4). ISSUE minor: "low circling" crows are ~7 m up and
  tiny against bright sky; weak tell from the farm yard.
- PikesCache: red stone + cairn + spade readable from 7.5 m (grab find_PikesCache). Hold Interact digs ("Dig up the grave" 1.0 s): chest
  opens, Epic + Rare shotguns + ammo + silver + named Bench Mark, 2 / 15, +272 XP. PASS.
- ISSUE: the FOUND banner (centre top) is partly hidden by the gun-compare card when the pop's loot lies in the crosshair: the
  card covers "+XP" and the region line (grabs found1, dig1). The banner draws under the world gun card; either raise it above the card or move the banner up.
- Crow actors exist for unfound finds (16 RoostCrows). Glints not checked by eye. Tollman and MiserGrave not visited (not asked).

## Top issues
1. Stagger never fired in any test (poise <= 0.20 of 0.25): check poise gain/threshold on Basic rank, esp. rifle/SMG.
2. FOUND banner overlapped by the dropped-gun card.
3. No `Weapon.SMG.Punch` cue (known).
4. LMG sustained climb 30+ deg without compensation (confirm by hand); Tarn bed hard to see; Ranger Post crows weak tell.
