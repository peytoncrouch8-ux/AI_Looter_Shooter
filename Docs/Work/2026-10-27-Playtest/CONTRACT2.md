# Round 2 contract (Main, 2026-10-27): fights, gunfeel, mission gates, round 1 polish

Read first: `CLAUDE.md`, `NOTES.md` here (the user's words, round 2 calls, DEMO's finds), `FINDINGS.md` (where things
live), `CONTRACT.md` "Rules for every agent" (holding trees `Saved/<Job>Code/` + HASHES.txt, in-place for owned Python/Art,
layout patches, no build/editor/git, Edit/Write only, cvars + tests, LooterUI kit, report under 150 words + README).
Holding tree names this round: `Saved/<Job>2Code/` (round 1's trees are merged; don't reuse them). Source/ now has round 1
merged: read files from Source/, not the old trees. Core-loop freeze: no new systems beyond what's asked.

## BOSSES (Opus): notes 21, 34
Owns: `Creatures/Gravemother*`, `Bosses/Abel*`, `Bosses/BossComponent*` (only if needed), `Tools/Unreal/build_area_den.py`,
boss tests. The user: "Gravemother fight and Abel are way to easy and die too quickly as far as progression goes. Also I can
just sit above the sink and shoot her without her aggro'ing to me." "You need to break the tombstone on her back before you
can damage her." Calls: ~1.6x health and harder hits for both (cvars); rim shots do nothing: damage from a shooter off her
hunting ground is ignored, with clear feedback (an "immune" hit look/sound on her hide, maybe a caption hint once) so the
player learns to go down; her tombstone (AGravemotherStone) must break before she takes damage (shots on her body before then:
the same shrug feedback, the stone glows/marks as the target). Keep the stone's existing crack/shatter stages and stagger.
Abel: check his phases still pace well with more health (adds, buckshot) and say what you changed.

## ENEMIES (Opus): notes 23, 39
Owns: `Creatures/Unpaid*`, `Creatures/CreatureBaseFight*`, `CreatureBasePursuit*`, `CreatureBaseSpacing*`,
`Creatures/FightDirector*`, `Creatures/ShriekRing*`, sound recipes for new Unpaid cues (`Art/Sounds`, cues.json entries for
them only), Unpaid tests. The user: "Unpaid have terrible ai, need better attacks and targeting sequences"; "Most mobs are not
frightening enough as enemies, gotta raise the stakes!" Today: one shriek + a straight lunge at the target's current spot.
Give them a real fighting kit: lead the target (predict), a second attack (e.g. a sweeping combo or a short-range grab/slam)
and a ranged pressure move for ones held back on the ring, flankers that come from the sides/behind, a telegraph the player can
read and dodge. Raise the stakes: they come on harder (faster closing, a pack that commits together after a scream), a
frightening entrance (rise from the ground, a scream that carries), but fair (clear tells). Every number a cvar. Keep within
the 8.3 ms budget (8 spiders + a pack of Unpaid). Don't change Gravemother/Abel (BOSSES).

## GRENADES (Opus): note 7 (no grenade classes: later)
Owns: `Combat/GraveSalt*`, `Player/PlayerThrow*` (rules/look only), `Art/Models/Throwables/SaltGrenade.py`, grenade tests.
"Grenades are too strong to start with, projectiles are too small and lacks explosion visuals." Weaker at the start (lower
early damage and/or starting count; say your numbers vs. a Common rifle magazine), the jar about twice as big in hand and in
flight (resize in the .py, re-export by Main), and a proper explosion: fireball/flash, shockwave ring, smoke column, debris,
a ground scorch, sized to the 4 m radius, reusing `FWeaponFX::SpawnExplosion` (Weapons/WeaponFXFire.cpp) where it fits;
within budget; every number a cvar.

## GUNFEEL (Opus): notes 28, 29
Owns: `Weapons/WeaponBaseKick.cpp`, `Weapons/WeaponBaseKinds.cpp`, `Weapons/GunKindsTuning.*`, `Weapons/GunfeelKickTuning.*`,
gunfeel tests. "More recoil so the higher the accuracy stat is, the more prominent the effect" (the user's call: more kick on
every gun; a high accuracy stat calms it a lot, so the stat clearly matters). "Normal weapons should have an idle sway just
like the sniper does when you zoom in": a smaller per-kind ADS sway for every gun (the sniper's stays), moving the aim like the
sniper's does; crouch steadies it. Every number a cvar (`Looter.Gunfeel.*`).

## GATES (Opus): note 38
Owns: `Tools/Unreal/build_area_missionprops_*.py`, `Tools/Unreal/create_mission_assets.py`,
`Tools/Unreal/create_side_mission_assets.py`, `Missions/*`, `World/MissionItem*`, `World/ReadableProp*`, `World/MissionArtifact*`,
`World/MissionFitting*`, mission tests. "As we do missions, there should be 'Gates' that 'Open' as you complete tasks in the
missions. As it stands, you can speed run quests and just spam click the use button." The user's call: steps unlock in order:
a step's items, readables, artifacts and fittings appear or work only from their step (FStoryCondition DuringMission +
FromStep through the scripts' helpers), across every Ransom's Rest main and side mission; key mission actions (taking a
mission item, using a fitting, the step-finishing interactions) need a short hold instead of a tap (a cvar; the existing hold
pattern, e.g. the bell rope). Things to keep: readables that are lore stay readable any time unless they're a step; a carried
item from an earlier step still counts. Also: Back Pay's "Take the tags" prompt stays after the tags are taken: it must go.
List every prop you gated for Main.

## POLISH (Sonnet): round 1's play-check finds
Owns: `UI/Inventory/MapPlaces*`/`MapWidget*` (label overlap only), `UI/World/LootLabel*` + `UI/World/WeaponLabelWidget.*`
(dots), `Loot/LootFanfareShows.cpp` (name pop), `UI/Menus/SettingsMenu*` (hover highlight), `Tools/Unreal/build_area_bounties.py`
(Ransom's Point spots), `Tools/Unreal/build_area_roosts.py` / layout patch `polish_*.json` (Depot roost facing).
Fix: map place labels WIDOW'S PINES and CHAPEL OF SAINT ADA overlap (general fix: labels that would overlap shift or hide the
lesser one); folded pile dots too small and lost in a legendary beam's glow (bigger, outlined, readable); the legendary name pop
appears twice at two spots (find why: the pooled pop plus the gun's own label?); the settings menu's hovered row isn't
highlighted (match the focused row's look); a player fell ~14 m off the Ransom's Point bounty arena mid-fight at
(-8682,-7016,622) (keep its spots and its hunting ground back from the drop, or move the arena); the Depot roost's player spot
faces ~45 deg off the bounty board (face the board).
