# Core-loop playtest pass, Ransom's Rest (DEMO, 2026-10-28 overnight)

PIE, `Tools\pie.ps1 -Map RansomsRest` (Looter.PlayLevel: new game, no saves). Main 1 step 3 to Main 4 step 4 by hand
(teleports between beats, `play.py fight` for the shooting: an aimbot, so damage taken reads low), then Main's melee import
restarted PIE; Main 1-3 skipped with `Looter.Mission.Complete`, Main 4 and Main 5 to its last step played (Uncommon rifle and
shotgun, level 3), chests of every tier opened. Not reached: bosses, Main 6-7, the mountain. Grabs:
`Saved/Screenshots/Tools/pt_*.png`.

## Bugs fixed

1. **The Ledger understated every regular creature's attack** (built and committed by Main). `Bestiary/BestiaryEntry.cpp`
   `ReadStats` showed the class's `AttackDamage` (Brown Spider "11 dmg") while every regular bite lands times
   `Looter.Fight.DamageScale` (1.8: 20-22 a bite in the log). Now times `FightRules::DamageScaleFor(KeepsOwnFight(StartingRank,
   IsBossCreature()), FFightTuning::LiveDamageScale())`; bosses and Legendary monsters keep theirs. Test: `Looter.Bestiary.Entries`.
2. **Ten false warnings at every PIE start** ("Item_CellarKey: no mission has an item CellarKey, so it's never there to take",
   also LampOil, Payroll, LostPropertyKeys, LanternBail, CacheTag1-3, FennsFare, ThaddeusTape). Cause: the editor's Play first
   opens the level under the menu's game mode, where `UMissionRunner` stays idle and knows no mission; `AMissionItem::BeginPlay`
   warned anyway. `World/MissionItem.cpp` warns only when the runner is active. Built; checked in PIE: 0 warnings. No test (a log line).
3. **Tier 5's fountain threw an Epic at the opener's feet.** Widows' strongbox opened from its front corner (`pt_t5_6.png`):
   the -60 deg piece landed under the player, its purple beam filling a third of the view. Cause: `ChestLoot.cpp`'s fan (+-60 deg,
   +18 a pair, round the chest's forward) ignored where the opener stands. Now `ChestShow::FountainTurn` (`Loot/ChestShow.h/.cpp`)
   mirrors any piece within `FountainClearOfOpener` (40 deg) of the opener's bearing to the other side; `ChestLoot.cpp` measures
   the bearing from the first player's pawn. Test: `Looter.Loot.ChestShow.FountainClear` (every bearing in 15 deg steps, pieces
   0-7, jitter +-6). Built; Looter.Loot 47/47 (ChestShow 5/5, ChestTiers 3/3); PIE recheck from the same corner: all pieces to
   the far side, view clear (`pt_t5fix.png`).

Uncommitted (Main commits): `World/MissionItem.cpp`, `Loot/ChestLoot.cpp`, `Loot/ChestShow.h`, `Loot/ChestShow.cpp`,
`Tests/ChestShowTests.cpp`.

## Bugs found, not fixed

1. **Crane's Ferry Gaff (new Tripo import):** on the thrust the green glove sits beside/below the shaft's butt instead of
   closing round it (`pt_mel_Crane_d.png`); the old "fingers open beside the gaff" nit. Sent to Main.
2. **Odessa's Razor Fan:** the swing's first frames show her glove as a large dark shapeless blob near the lens
   (`pt_mel_Odessa_a.png`); later frames fine. Minor.
3. **Sniper scope reads as an empty tube, not glass** (`pt_scope.png`): the objective shows the tube's lit inside (pattern and
   grave-salt rime) through the near-clear `M_Glass` (Opacity 0.15). Suggest a dark inner sleeve in `Longhorn.py`'s objective
   bell, or a lens MI of `M_Glass` with Opacity 0.45 and a faint blue tint.
4. **First chest out-gives its lesson:** Ruth's bluff cache (T3 in `Tools/Unreal/chest_tiers.py`, `RangerCache_BluffPath`)
   gave a Rare Defender Pump Shotgun, an Uncommon sniper and +3 rockets at level 1 (expansion §3 Main 2: a Common Ranchhand for
   the swap lesson, beside the coffin's Common Bullpup). Suggest tier 2 for that cache (or its guaranteed gun kept Common); no
   rocket ammo while no launcher is carried. (A T2 also rolled an Epic once by Luck 0.5: chance, not a bug.)
5. Tools: `play.py equip` is 1-based (`equip 0` says "on its way to hand" and does nothing); `press Melee`/`press Grenade`
   work but print `Failed to find object IA_Melee` first (harmless). The bell rope needs `lookat Bell_Chapel` after `goto`.
   Teleporting onto the Sink floor doesn't count "Climb down into the Sink" (height counted): walk off the rim.

## Feel problems (ranked by impact on fun)

1. **Spider packs kill the player in 3-4 s; the Unpaid barely land a hit.** Spiders: the fight director lets 2-3 bite at once
   ("3 held" on the Sink floor), 24-33 a bite at levels 3-5, each about every 1.1 s: a level-3 player (116 HP + 41 ward) died 3.2 s
   after reaching the Sink floor, twice; the bluff nest's two took 100 HP + 15 ward in 4.2 s at level 1. Unpaid: chapel yard
   (16, Main 4), 73 melee turns, 59 attacks started, 2 hits landed; every rifle hit slows one to 55% for 0.1 s, so steady fire
   keeps lunges short forever. Fights swing from lethal to harmless by kind. Suggest: `Looter.Fight.MeleeTokens` 1 for spiders
   (or bite cadence 1.1 -> 1.6 s while 2+ bite) so a pack takes ~8 s to kill; the Unpaid's lunge commits in its last 0.3 s
   (hit-slow ignored), slow once per 0.5 s at most, and cinders more often when the player stands off their ground.
2. **The sniper at the hip hides the right third of the screen** (`pt_gate1.png`: an Unpaid half behind the scope). Suggest
   per-kind sniper framing like the LMG's (`Looter.FPHands.Sniper.ArmsScale` 0.9, hip 3-4 cm lower).
3. **Unpaid wait at their ground's edge while the player shoots from a doorway** (chapel: from inside the nave, 9 stood at the
   back door in a line, `pt_cf_melee.png`): a shooting gallery. Suggest phase-stepping in (they already blink) when the player is
   within 8 m of their ground's edge, or cinders from the line.
4. **Some guns read flat at first-person distance:** the Defender Pump Shotgun (Saint Maud) is a pale grey block with a big
   boxy sight (`pt_gf1.png`); the camo pattern on the red/cream Reflex rifle reads as cow hide (`pt_sf2.png`). Suggest a darker
   base and stronger wear on Saint Maud's clean finish; camo blotches in two close tones, not red on cream.
5. **The AR's reflex reticle is a tiny orange dot** in a tall frame (`pt_ads_ar_zoom.png`), hard to find against the warm
   ground. Suggest a dot about 2x and a thin ring, or cyan.

## Checked and fine

- Main 1-4 whole (headboard, farmhouse, porch plate, cards; bluff nest 5, stains, Sexton, Ledger via Tab+3; gate 7, Main Street
  set piece 2 pushes + Bell-Ringer and its shower, Tilly's gun out of the window; chapel yard 16, vestry, bell rope hold 1.2 s,
  Reliquary, Aldana turn-in), Main 5 to its last step (walk off the rim counts, Sink floor 6, three egg sacs shot, lantern taken).
- Chests: T1 saloon backlot (lid swings), T2 keepers' bridge (latches, lid), T3 bluff cache, T4 gang strongbox (orb pulse, burst,
  after Calder's Caches step 2), T5 Widows (gold burst ring, lid, fountain). Ammo "+1" labels only because pools were full.
- Weapon swap (NextWeapon: gun drops, the other rises), reload (shotgun shell, rifle mag in the support hand), ADS on rifle and
  shotgun, grenade throw ("reload cut short", 1500 cm/s), melee all five heroes after the Tripo import (`pt_mel_<Hero>_*.png`:
  Ellis's spade D-grip, Odessa's fan, Gauge's fist with the ring off-axis as meant, Pike's pick; Crane: bug 1).
- Level-ups 1->2->3, roost found walking Main Street, death respawn at the level start with Hob's line (no roost found after a
  teleport: expected). No warnings or errors from play in the log beyond the fixed ten (engine scalability notices only).
