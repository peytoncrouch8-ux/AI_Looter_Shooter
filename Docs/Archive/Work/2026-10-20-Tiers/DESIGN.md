# Enemy tiers: design draft (2026-10-20, for the user's approval; nothing built)

The user's ask (their prompt, word for word in the session): five tiers for every regular enemy (not story bosses, not
the named Legendary monsters): Regular (white, no prefix), Restless (blue), Gravebound (purple), Hallowed (yellow),
Ethereal (red). Each tier up: harder (health, a new defense stat, damage, level bonus, size), better (gun chance, more
drops, luck with Common still the most likely rarity, XP), rarer; readable (tag colour and prefix, a tier glow, sounds
and a bigger death burst and loot pop up top); one trait added per tier above Restless.

Facts it rests on (code): creature health and gun damage both grow 8%/level (`FLevelRules::EnemyGrowth` =
`UWeaponDefinition::DamagePerLevel` 0.08), so an on-level gun takes the same hits at every level; rarity damage
multipliers Common 1.00, Uncommon 1.10, Rare 1.25, Epic 1.45, Legendary 1.75 (parts add a few % more); rifle 20 a round
at 600/min, revolver 52 at 140/min; spider 300 health at level 1, Unpaid 160, dead miner 180, slime 120; crits x1.5;
luck multiplies rarity tier n's weight by (1 + Luck)^n on 60/25/10/4/1 (Luck above ~1.4 makes Uncommon beat Common).

## Toughness (the level bonus at Ransom's Rest's mid level 4: +1 = x1.065, +2 = x1.13)

| Tier | Health | Defense | Effective health vs Regular | Damage | Level | Size | Traits |
|---|---|---|---|---|---|---|---|
| Regular | x1 | 0% | 1.0 | x1.00 | +0 | 1.00 | - |
| Restless | x2.0 | 10% | 2.4 | x1.15 | +1 | 1.08 | - |
| Gravebound | x3.0 | 20% | 4.0 | x1.30 | +1 | 1.15 | 1 |
| Hallowed | x3.5 | 28% | 5.5 | x1.45 | +2 | 1.22 | 2 |
| Ethereal | x4.0 | 35% | 7.0 | x1.60 | +2 | 1.30 | 3 |

Today's Restless (x2.5 health) and Gravebound (x5) get a little softer in raw health (two tiers now sit above them) but
gain defense; Legendary monsters (x12, +2 levels: ~13.6) stay well above Ethereal.

## Shots to kill (body shots, no crits, an on-level gun; common / rare / epic)

| Tier | Rifle vs spider | Revolver vs Unpaid | Rifle seconds of fire |
|---|---|---|---|
| Regular | 15 / 12 / 11 | 4 / 3 / 3 | 1.5 / 1.2 / 1.1 |
| Restless | 36 / 29 / 25 | 8 / 6 / 6 | 3.6 / 2.9 / 2.5 |
| Gravebound | 60 / 48 / 42 | 13 / 10 / 9 | 6.0 / 4.8 / 4.2 |
| Hallowed | 83 / 66 / 57 | 17 / 14 / 12 | 8.3 / 6.6 / 5.7 |
| Ethereal | 105 / 84 / 72 | 22 / 18 / 15 | 10.5 / 8.4 / 7.2 |

## Spawn odds (a share of each regular spawn, rolled on arrival)

| Tier | Ransom's Rest | Rising to (by the late areas) | Alive at once per area |
|---|---|---|---|
| Restless | 8% | 12% | no cap |
| Gravebound | 2% | 4% | 4 |
| Hallowed | 0.4% (1 in 250) | 1% | 2 |
| Ethereal | 0.08% (1 in 1,250) | 0.25% (1 in 400) | 1 |

Plus placed ones as today (a Restless in most scripted fights, a few Gravebound). Promotions: rolled on each arrival,
a promoted creature that dies comes back Regular, no reroll within 20 minutes of play (as today).

## Rewards

| Tier | Gun on a kill | Guns | Luck | Legendary a kill | Ammo boxes | Silver | XP |
|---|---|---|---|---|---|---|---|
| Regular | 30% | 1 | 0 | 0.3% (1 in 333) | 1-2 | x1 | x1 |
| Restless | 60% | 1 | 0.5 | 2.2% (1 in 45) | 2-3 | x1.5 | x2 |
| Gravebound | 90% | 1 | 0.8 | 5.5% (1 in 18) | 2-4 | x2.5 | x3.5 |
| Hallowed | 100% | 1-2 | 1.1 | 13.3% (1 in 7.5) | 3-5 | x4 | x6 |
| Ethereal | 100% | 2 | 1.3 | 21.3% (1 in 4.7) | 4-6 | x6 | x9 |

Rarity of each dropped gun (Common / Uncommon / Rare / Epic / Legendary): Luck 0: 60 / 25 / 10 / 4 / 1; 0.5: 43 / 27 /
16 / 9.7 / 3.7; 0.8: 35 / 26 / 19 / 14 / 6.1; 1.1: 28 / 25 / 21 / 17 / 9.1; 1.3: 24 / 23 / 21 / 20 / 11.3. Common stays
the most likely at every tier. Legendary monsters (x10 XP) and bosses (x20) keep their tables.

## Traits (Gravebound 1, Hallowed 2, Ethereal 3, rolled from what fits the creature)

- Fleet: moves 25% faster, attacks wind up 15% quicker.
- Mending: after 3 s unhit, heals 3% of its health a second (any hit stops it).
- Warded: a soul ward worth 30% of its health takes damage first; crits break it faster; shatters with a pop.
- Grave-burst: on death it swells for 1 s, then bursts 3 m round (step away).
- Rallying: creatures within 8 m take 15% less damage and move 10% faster while it lives (kill it first).
- Vengeful: below 30% health it enrages, +25% damage and speed, its glow flaring.
- Phasing (Ethereal's own, always one of its three): when hit hard it blinks 4-6 m aside (every 4 s at most).

## Readable and rewarding

- Name tag: prefix + name in the tier colour; the tag's frame thicker on Hallowed and Ethereal.
- Glow in the tier colour: spider eyes, slime core, the Unpaid's coal, the miner's lamp; Ethereal also a faint red
  ghost-rim shimmer (overlay, as the objective highlight's figure look).
- Hallowed and Ethereal: a sting when one first notices the player, a bigger death burst in the tier colour, a short
  pillar of soul-light, a loot pop with its own sound, the XP popup in the tier colour.

## What it touches

`ECreatureRank` (new values appended after today's so saved ranks keep their numbers; Basic shown as Regular),
`UCreatureRankSettings`/`FCreatureRankInfo` (defense, trait count, glow colour, spawn odds by area, caps),
`UHealthComponent` (defense removes a share of each hit), `DA_LootTable_Rare`/`_Epic` retuned and new `_Hallowed`,
`_Ethereal` (`create_rank_assets.py`), the name tags and `LooterUI::Color` (two new colours), promotions
(`SessionSubsystemPromotions.cpp`: odds, caps, the area rise) and the saves they write, creature glows per kind, a new
trait component, death bursts and loot pops, new sound cues, kill XP, the bestiary (defeats per tier), the powers' rank
rules (Dust Devil's flings by rank), tests (`Looter.Loot.RankOdds` for five tiers at 200,000 kills, spawn odds and caps,
defense, traits, a save round trip), docs (Story.md's ranks table, Areas). Perf: a horde of Ethereals
(`Looter.Perf.Horde Spider 8 Ethereal`) on Medium inside 8.3 ms.

## The user's calls (2026-10-20): APPROVED, build it

- Colours: **tier-only colours**: yellow (Hallowed) and red (Ethereal) are enemy-tier colours only; a tag's colour no
  longer has to match a gun rarity (blue and purple still happen to).
- Defense: **crits pierce half**: defense counts half against headshots.
- Ethereals on Ransom's Rest: **yes, very rare** (1 in 1,250).
- The numbers above approved as they stand (they live in data for tuning).
