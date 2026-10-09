# Borderlands vs. Revenant: what to build to stop feeling empty and wonky

Research for the polish loop, 2026-10-08. The user: "The game lacks polish, it feels empty and wonky... Compare the game to
Borderlands' mechanics and playstyle and make our game better." Written from the code, `CODEMAP.md` and the docs, plus web
research (sources at the end). Nobody has played the build for this note, so the "wonky" findings come from the code and the
handoff; the Demo agent should confirm each in play. We make our own versions of everything: no copied names, art, text or
sounds; sounds are script-made; C++ first; no new levels or areas.

## What makes Borderlands feel good

1. **A tight reward loop.** Kill, loot beam, compare, equip, hit harder, take on a bigger fight. Damage numbers show on
   screen on purpose: Pitchford said they fade, "but when you need them, you can tell" [1].
2. **The guns are the stars.** Every manufacturer has its own look, behaviour and sound [2]. Borderlands 3 won praise for
   "weight and feedback" and for guns that feel different from each other [3][4]. Borderlands 2 was faulted for stiff
   enemies, stiff recoil and hollow sound [5].
3. **Failing is cheap, so aggression pays.** Shields recharge after a short delay, Fight For Your Life lets a kill bring
   you back with a second wind [6][7], New-U stations and fast travel remove the run back, and ammo and health vendors sit
   just outside boss fights [8][9].
4. **Enemies are characters.** 15-20 types in Borderlands 2 (200-300 variants) [10]; Badass versions bigger, glowing and
   elemental [11]; enemies dodge, flank and retreat [10]; bars coloured by flesh, armor and shield teach what element to
   use [12]; psychos shout lines players quote [13].
5. **The world is dense and loud.** Lootable things everywhere, ECHO logs, a hub of vendors with personalities [14], and
   separate ambient and combat music [15].
6. **Where it falls down (avoid these):** bullet-sponge bosses and enemies [3][16], fetch-quest side missions [17], inventory
   friction and weak item comparison [18], repetitive early waves [16].

## Systems: Borderlands vs. ours

| System | Borderlands | Ours |
|---|---|---|
| Gun variety | Many kinds and brands; BL3 "over one billion" guns | **Partial.** Rifle and shotgun only; parts, rarity, notches and curses are good. Pistol, SMG and sniper ammo exists but no such guns |
| Elements and status | Fire, shock, corrosive, slag/radiation, matched to enemy layers | **Missing** (stats only, by the user's call) |
| Crits, damage numbers | Yes | **Has.** Crit spots, 1.5x, floating numbers |
| Hit and kill feedback | Hit marker, flinch, gore, loud guns | **Partial.** Tracers, recoil spring, muzzle flash, hit tick, crosshair kick. No camera shake or hit-stop in the code; kills are quiet |
| Shields | Recharge after a delay; shield types | **Missing** |
| Healing | Shield regen, vials, vending, second wind | **Missing.** I found no regen, health drop or heal in play (only a cursed gun's kill-heal); the HUD has a heal flash but nothing to trigger it |
| Down state, respawn | Fight For Your Life, New-U | **Partial.** Respawn graves, no down state |
| Grenades, melee | Grenade mods and a quick melee; Wonderlands adds melee weapons and spells [25][26] | **Missing** |
| Action skill, skill trees | Core of each hero | **Missing** (ember powers and heroes planned, Phases 10-11) |
| Movement | Sprint, slide, mantle (BL3) | **Partial.** Sprint, crouch, smooth slide; no mantle or vault, no dodge |
| Enemy variety, ranks | 15-20 types, Badass ranks | **Partial.** Spider, slime, Unpaid, two bosses; ranks Restless, Gravebound, Soulfed work like Badass |
| Enemy behaviour | Dodge, flank, cover, retreat, healers | **Partial.** Chase, telegraphed lunge and shriek, phase-step, pack calls. No ranged, flank, retreat or support; no navmesh |
| Armor and shield layers | Coloured bars | **Missing.** One bar with level, rank and name |
| Bosses | Weak points, phases, adds, loot shower | **Partial.** Phases, adds, fog wall, boss bar; little spectacle |
| Rarity beams | White to orange, red chests | **Has.** Beams, toss, two chest kinds (caches, Strongbox) |
| World loot | Lockers, crates, ammo caches everywhere | **Missing.** Chests only, a few per area |
| Economy | Cash, vending machines, bank, storage upgrades | **Missing.** Only the gunsmith's bench; Tilly's shop is planned |
| Item compare | Card with arrows (and a weak spot [18]) | **Partial.** Pickup card and hover card exist, with a lot of info |
| Missions | Givers, turn-in, some reward choices, log | **Has** main and side missions, tracker, log. **Missing:** reward choice, XP per step, finish on last step |
| Story in the world | ECHO logs, voiced barks, NPC chatter | **Partial.** Captions, Hob, headboards, Ledger; no found logs, no ambient talk |
| Ambient sound and music | Ambient and combat tracks [15] | **Missing.** No ambience or music cue exists in `LooterSoundCues.h`; only the Music slider |
| Ambient life | Critters, NPCs, hub bustle | **Missing.** Only hostile creatures; the town is empty by story |
| Map, fast travel | Full map, stations | **Partial.** Minimap; area trips by train; nothing inside an area |
| Meta progression | Badass Rank, Guardian Rank | **Missing** (per-gun notches only) |
| Co-op | Four players | **Missing**, out of scope |

## Prioritized improvements

Sizes: S under a day for one agent, M a day or two, L several days or several agents. Items 1-10 cut the empty and wonky
feeling for the least work.

**Status (2026-10-09, 01:00):** built: 1 (ambience and music), 2 (hit and kill feedback), 3 (wounds that close and
soul-motes; no down state yet), 4 (mantle, vault, coyote time, jump buffer, unstick), 5 (ambient life), 7 (camps,
patrols, ambushes, the rank sting), 8 in part (flanking and retreat; no barks or new kinds yet), 9 (boss shows), 11
(inventory list and card), 12 (turn-ins and the banner), 16 in part (the melee strike; no grenade yet). Next: 6, 10,
the rest of 8, 14, 13, 15.

**1. Ambient sound beds and area music (M).** Build: looping, script-made layers (canyon wind, grass, insects by day,
crickets at dusk, creek, water, creaks, a far bell), placed as positional emitters (windmill, creek, falls, chapel) plus a
level-wide bed that follows the lighting state (Day, Dusk). Add exploration, combat and boss stems with a small director
(calm, fight, boss, dusk). Why: silence under footsteps and gunshots is the first "empty" tell; Borderlands ships separate
ambient and combat music [15]. The sound class `Ambience` and the `World.Windmill.Creak` example already exist in the README.
Touches: `Audio/LooterSoundCues.h`, `Art/Sounds/recipes/world.py` and a new `music.py`, `cues.json`, `build_sound_bank.py`,
a new `Audio/AmbienceSubsystem` and `Audio/MusicDirector`, `World/LightingStateSubsystem::OnChanged`, `World/Windmill`.

**2. Hit and kill feedback pass (S-M).** Build: a small camera kick and FOV punch per shot scaled by gun; a 40-80 ms
hit-stop on the creature for crits and kills (per-actor dilation, not global); a short stagger on heavy hits; a soul-light
burst when an Unpaid dies and a shell crack when a spider does (Borderlands 3 added gore because players wanted to see kills
[19]); bigger, brighter crit numbers; a red-tinted hit marker on a crit spot (no crit sound, the user's call). Why: the
standard juice stack [20][21], and rule 7 of `CLAUDE.md`. Touches: `Player/PlayerViewComponent` (split it first, 566
lines), `Weapons/WeaponBaseFiring.cpp`, `Combat/HealthComponent`, `Creatures/CreatureBase*`, `UI/World/DamageNumberWidget`,
`UI/HUD/PlayerHUDWidget`.

**3. A recovery loop (S, then M).** Build now: "wounds that close", the Revenant's own trait: health refills slowly after
about five seconds without damage, and kills sometimes drop a small green soul-mote that heals. Later: a down state in the
Fight For Your Life mould, rising on a kill within about eight seconds. Why: with no healing, every fight drains a bar that
never refills, so a long hike wears the player down and failing hurts. Needs the user's okay on the fiction. Touches:
`Combat/PlayerVitalsSubsystem`, `Combat/HealthComponent`, `Loot/LootDropComponent`, `UI/HUD/HudPlayerFrameWidget`.

**4. Traversal: mantle, vault and forgiveness (M).** Build: mantle onto chest-high ledges and wall tops, vault low fences,
coyote time and a jump buffer, auto step-up on small lips, and a stuck check for the player and creatures. Why:
Borderlands 3 felt faster with "sliding under gaps and mantling over obstacles" [16]; fences, walls and wagons that stop
the player are the "wonky" part of Ransom's Rest. Touches: `Player/PlayerLocomotionComponent*`, `Core/LooterCharacter`,
`Tests/LocomotionPlayTests.cpp`, `World/InstancedProps` (a tag for climbable tops).

**5. Ambient life (M).** Build: instanced crow and sparrow flocks that scatter at gunfire, grasshoppers, butterflies and
dragonflies by the creek and flowers, tumbleweeds and dust devils, cloth, signs and chimes moved by the wind (vertex or
spring), a swinging saloon sign, Hob's perches used more. Why: motion in the corner of the eye is what makes a map feel
inhabited. Touches: a new `World/AmbientFauna`, `World/InstancedProps`, new models in `Art/Models`, the Day and Dusk
states.

**6. Signs of the living in town (S-M).** Build: muffled voices, a barking dog, a music box or a cough behind shuttered
windows; one-line mutters from the speaker points as the player passes ("Don't look at it, Mae"); lit windows (the house
lights exist). Why: the town is empty by the story, so make it a held breath rather than a void. Touches:
`Story/SpeakerPointComponent`, `Story/CaptionSubsystem`, `World/HouseLights`, `World/WindowShutter`, new sound cues.

**7. Encounter pacing (M).** Build: three to five small hand-placed camps per area (three to five creatures, a loot
container, one Restless leader), roaming packs on patrol loops, ambushes at landmarks (the dead rise from the churchyard
graves, spiders drop from the Webwood), a sting when a Gravebound or Soulfed appears, combat music cut-in. Why: the valley is
42,800 m² with a few scripted fights; the Halo rule is a repeating 30-second loop of enter, plan, execute, reward [22].
Touches: `Creatures/EncounterSpawner*`, `Creatures/UnpaidCreature::RiseIn`, `Tools/Unreal/build_area_story.py`, the
layout json, `Creatures/EncounterSettings`.

**8. Enemy barks and behaviours (M).** Build: short barks (text over the head and a synthesized murmur: the Unpaid
mutter bits of their lives) and callouts on rank-up; a retreat-when-low behaviour; flanking for packs; one ranged kind (a
spitter spider that lobs venom) and one support kind (a bell-ringer Unpaid that raises the dead nearby, kill it first). Each
new creature needs a bestiary page. Why: Borderlands enemies are characters, and the AI dodges, flanks and retreats [10][13].
Touches: `Audio/CreatureVoiceComponent`, `UI/World/CreatureHealthBarWidget`, `Creatures/CreatureBase*`, new
`Creatures/*`, `Bestiary`, `Tests`.

**9. Boss spectacle (M).** Build: a name card with a short slow dolly at the fight's start (the scene timeline fits); exposed
weak-point windows (the Gravemother's abdomen after a missed charge, Abel's lantern after the buckshot, in the way the
Warrior's rock plates open a weak spot [23]); a phase-change sting and music swell; a final-blow slow beat; and a loot
shower of beams. Why: bosses are the user's named gripe; reviewers call bullet-sponge bosses tedious [3][16]. Touches:
`Bosses/BossComponent*`, `Bosses/BossTypes.h`, `Creatures/GravemotherCreature*`, `Bosses/AbelKeeper*`,
`Scenes/SceneTimeline`, `UI/HUD/HudBossBarWidget`.

**10. Loot fanfare and lootable world (S-M).** Build: a rarity-keyed drop sound and pickup chime (purple and orange get a
sting, a lifted beam and a text flash); breakable crates and barrels; coffins and graves you can open for ammo and old
coins; mail boxes and strongboxes; a few more chests per area. Why: a legendary needs to be an event, and every corner
should offer something. Touches: `Loot/Chest`, `Loot/AmmoPickup`, `World/LightBeam`, `LooterSoundCues.h`, `UI/HUD/HudPickupFeedWidget`.

**11. Inventory and loot cards, simplified (S-M).** Build: one card with up/down arrows against the equipped gun and at most
five stats by default (hold a key for the rest); a key to mark junk; auto-suggest "worse than what you hold"; sort and
favorite. Why: the user finds the inventory confusing and boring; Borderlands 2's comparison was a known weak point [18].
Touches: `UI/Inventory/Loadout*`, `UI/HUD/PlayerHUDWidgetPickupCard.cpp`, `UI/Style/WeaponText`.

**12. Missions turned in, with a payoff moment (M).** The user's call (2026-10-08): no experience for steps, and a
mission is turned in to its giver rather than finishing on its last task. Build: a "ready to turn in" state with the
tracker's arrow on the giver, rewards and a "Mission Complete" banner at the turn-in, later a choice of one of two reward
guns on main missions (Borderlands offers choices [24]). Touches: `Missions/MissionRunnerFlow.cpp`,
`Missions/MissionDefinition`, `Missions/MissionRewards`, `UI/HUD/HudMissionTrackerWidget`,
`Tools/Unreal/create_mission_assets.py`. (Being built by the missions agent, 2026-10-08.)

**13. Currency and vendors (L).** Build: old coins ("grave gold", in the Story) dropped by the Unpaid, cash in the HUD,
Pruitt's General Store (ammo, health tonics), Tilly's grave goods (salts and charms), selling junk guns (this also fixes
inventory clutter), a bank or backpack upgrades. Why: the economy turns every drop into a choice; Borderlands puts ammo and
med vendors by every boss [8][9]. Touches: new `Economy/`, a vendor screen reusing the bench widget patterns, the
`Inventory`, `Session` save, `Loot`.

**14. Map and fast travel between graves (M).** Build: a full map page in the inventory with mission pins, graves and
chests; fast travel between opened respawn graves. Why: removes the walk back after dying or finishing a mission [9].
Touches: `World/RespawnMarker`, `UI/Inventory`, `World/MinimapSubsystem`, `Session/SessionSubsystemTravel.cpp`.

**15. Gun variety (L).** Build: revolver, repeater (lever-action), sniper and SMG kinds with their own handling and sound;
alt-fire and behaviours per family. Why: the loot chase needs guns that feel different, not just numbers [2][4]. Touches:
`Weapons/WeaponTypes.h` (`EWeaponKind`), `Art/Models/Weapons/*.py`, `Weapons/WeaponParts`, `Art/Sounds/recipes/guns.py`.

**16. Melee and a throwable (M each).** Build: a quick melee strike that buys space against lunging Unpaid, and a grave-salt
grenade (the "Slag Bomb" ember idea reused). Why: a panic button and a crowd answer [25]. Touches: `Player/`, `Combat/`, `Input`.

**17. Shield or ward layer (L).** Build: a rechargeable layer (a salt ward, as gear) before health, with a blue bar and a
recharge delay. Why: the Borderlands rhythm of duck, recharge, return [6]. Touches: `Combat/HealthComponent`, the damage
path, `UI/HUD/HudPlayerFrameWidget`, `Inventory`, loot tables, `Session` save.

**18. Ember powers and skill trees (L).** The planned Phase 10, the four demo powers first. Why: in Borderlands each hero's
action skill and three skill trees are the build identity [26]. Touches: `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`, `Player/`, `Combat/`.

**19. Elements, resistances and enemy layers (L).** Build: three elements with status effects, coloured bar layers on
enemies [12]. Why: gives loot choices meaning. Do after 15 and 17.

**20. Challenges (M).** A Ledger page of challenges (headshots, kills per kind) for small permanent bonuses, a cheap
answer to Badass Rank, which pays stat tokens for challenges [26].

## Suggested order

Wave A (four agents, disjoint files): audio (1, 6, parts of 8), feedback (2, 10), traversal (4), encounters (7, 8). Wave
B: ambient life (5), boss spectacle (9), missions and inventory (11, 12). Then recovery (3) with the user's okay. Bigger
systems (13-20) one at a time, 13 and 15 first.

## Sources

Opened pages: [1] https://www.gamebanshee.com/news/94709-borderlands-preview-and-interview.html ;
[16] https://gameinformer.com/review/borderlands-3/borderlands-3-review-sticking-to-its-guns ;
[4] https://www.thesixthaxis.com/2019/05/01/borderlands-3-preview-hands-on-gameplay/ ;
[8] https://www.thegamer.com/borderlands-3-lack-of-ammo/ ;
[18] https://thatgamesux.com/borderlands-2-thoughts-on-usability ;
Wikipedia: https://en.wikipedia.org/wiki/Borderlands_2 , https://en.wikipedia.org/wiki/Borderlands_3 ,
https://en.wikipedia.org/wiki/Borderlands_(video_game) , https://en.wikipedia.org/wiki/Tiny_Tina%27s_Wonderlands ,
https://en.wikipedia.org/wiki/Borderlands_4 ;
Dead ECHOs: https://www.dlcompare.com/gaming-news/borderlands-dead-echos-makes-the-world-of-borderlands-4-feel-more-alive-80888 ;
open-world density: https://gmtk.substack.com/p/how-nintendo-solved-zeldas-open-world .

Search summaries only (page not opened or not readable; treat as less certain): [2]
https://www.thefirearmblog.com/blog/2012/11/15/the-fictional-but-honest-gun-industry-of-borderlands-2 ;
[3][5] https://www.pcgamer.com/uk/hands-on-borderlands-3-is-a-bigger-smarter-borderlands-2 ;
[6] https://mentalmars.com/guides/how-do-shields-work-in-borderlands-3/ ;
[7] https://primagames.com/eguides/borderlands-2-game-of-the-year-eguide/claptraps-guide-to-minioning/longevity-on-pandora ;
[9] https://steamcommunity.com/app/49520/discussions/0/3716062344560666596 ;
[10] https://cinemablend.com/games/Borderlands-2-Smarter-AI-Means-Fewer-AI-45172.html and
https://www.playstationlifestyle.net/2012/07/27/enemies-of-borderlands-2-to-be-smarter-than-ever-before-screens-inside/ ;
[11] https://borderlands.fandom.com/wiki/Badass ;
[12] https://steamcommunity.com/app/49520/discussions/0/618463106374911445 ;
[13] https://www.nbcnews.com/tech/tech-news/krieg-psycho-bandit-brings-refreshing-dose-insanity-borderlands-2-flna1C9866276 ;
[14] https://borderlands.fandom.com/wiki/Sanctuary ;
[15] https://www.jesperkyd.com/news/2012/07/jesper-kyd-scores-borderlands-2/ ;
[17] user reviews of Borderlands 2 (Giant Bomb, via search) ;
[19] https://windowscentral.com/five-interesting-things-we-learned-about-borderlands-3 ;
[20] https://www.wayline.io/learn/game-feel/1 ;
[21] https://megacatstudios.com/blogs/game-development/mega-cat-developer-juice-guide-v1-0-08-23-17 ;
[22] https://www.engadget.com/2011-07-14-half-minute-halo-an-interview-with-jaime-griesemer.html ;
[23] https://primagames.com/eguides/borderlands-2-game-of-the-year-eguide/bestiary/hyperion-loaders (the Warrior's
weak-point plates are from a Gamer Guides page seen in search) ;
[24] https://borderlands.fandom.com/wiki/Mission ;
[25] https://borderlands.fandom.com/wiki/Mirv ;
[26] the Wikipedia pages above (Borderlands 2 for action skills, skill trees and Badass Rank; Tiny Tina's Wonderlands
for melee weapons and spells).
