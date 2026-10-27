# Round 3 contract (Main, 2026-10-27): hands and powers, Cyrus as boss two, world readability, opening and menu

The user picked all four groups of remaining notes (NOTES.md items 1-3, 5, 6, 16, 26, 27, 32, 35, 37; boss cutscenes later).
Their calls (2026-10-27): startup logos = a placeholder studio card (renamable later) + the Unreal Engine card per Epic's
logo rules; Cyrus Teague = "promote the mine story": Dead Lift and Back Pay become main missions after Main 5, Cyrus (the
Foreman) the required second boss, then Abel's missions and finale, XP and levels re-paced; melee tools = Main proposes 2-3
per hero on a concept sheet, the user picks before anything is modeled; menu backdrop = Teropa from orbit (the planet turning,
cloud bands, night-side lights).

Read first: `CLAUDE.md`, `NOTES.md`, `FINDINGS.md`, `CONTRACT.md` "Rules for every agent" (holding trees, HASHES.txt,
in-place Python/Art you own, layout patches, no build/editor/git, Edit/Write only, cvars + tests, LooterUI kit, report + README).
Holding trees this round: `Saved/<Job>3Code/`. Read sources from Source/ (rounds 1-2 merged). Original names, art and text only
(no copyrighted content; Borderlands/Gearbox are references for feel, never copied).

## CONCEPTS (Opus): melee tool sheet (note 5)
Owns: `Art/Concepts/MeleeTools/` (new). Read Docs/Story.md (the five heroes), Docs/Pipeline/Heroes.md, CharacterModels.md.
For each hero, 2-3 original melee tool ideas that fit who they are (frontier-tech western on Teropa), each with a one-line why
and how it swings (right hand). Draw one sheet per hero as clean vector/SVG concept art in an HTML page rendered to PNG with
the project's Node + Playwright (`Tools/portraits.ps1` shows how it's run; use the same Node/Playwright; no new tools). Write
`Art/Concepts/MeleeTools/README.md` (the options, lettered A/B/C per hero). Main sends the PNGs to the user.

## HANDS (Opus): notes 5 (code), 27, 6
Owns: `Player/PlayerMelee*`, `Weapons/WeaponBaseMelee.cpp`, `Player/FirstPersonHands*` (melee and power poses only),
`Player/Animation/LooterStancePose*` (only if 3P needs it), `Heroes/HeroDefinition*` (a melee tool field), `Embers/EmberComponent*`
(activation hook), `Embers/Powers/EmberFiftyTwoPickup*`, `Embers/Powers/EmberFX*` (cards), `Embers/Powers/EmberLuckyStreak*`
(cards only), tests. (5) Melee "is jank, we want each character to have a specific tool that they use and hits with the right
hand": a right-hand melee swing with a held tool (a per-hero `MeleeTool` mesh + grip on the hero definition; until the models
exist, a neutral placeholder tool mesh path), the gun lowered/held in the left while the right swings, a proper wind-up, strike
arc and follow-through, hit-stop and impact kept; third person too if the stance layer allows. (27) "Action skill activation
animation": a short hand/arm cast pose when an ember power fires (raise, flare, release), per power kind if cheap, never
delaying the power more than ~0.15 s. (6) Odessa's Fifty-Two Pickup "shoots tiny particles, needs better animations and bigger
card like projectiles": real card quads (a card face/back look, code-drawn or a small mesh), readable size in flight, spin and
flutter, a throw animation (the cast pose), cards that stick and fade on hit. Every number a cvar.

## SIGHTS (Opus): note 35
Owns: `Tools/Blender/looter_guns.py` (sights only), `Art/Models/Weapons/*.py` (sight parts only), `.parts.csv` sight rows
(never rename or reuse a part key). "Gun iron sights need changed": redesign the iron sights so they read cleanly at ADS: a
clear rear notch/aperture and front post that line up on SOCKET_Aim, less clutter, frontier-tech style, per family (shared
'Iron', Bullpup flip-ups, Drover notch/blade, Ranchhand ghost ring). Check the sight picture math (the aim line through rear and
front). Test the export with `Tools\models.ps1 -Only <Gun> -NoImport`; list what Main must import.

## STORY (Opus): notes 32, 37
Owns: `Tools/Unreal/create_mission_assets.py`, `create_side_mission_assets.py`, `create_story_lines.py`, `story_texts.py`,
`Tools/Unreal/build_area_lostloot.py`, `World/LostPropertyWindow*`, `Loot/LostLoot*`, `Docs/Story.md`, `Docs/Progression.md`
(mission XP), `Docs/Areas/RansomsRest*.md` (missions), mission tests. (32) Promote Dead Lift and Back Pay to main missions after
Main 5, so Cyrus Teague (the Foreman) is the required second boss; Abel's missions and finale after; re-gate prerequisites,
offers, the deck, vigil stones and fights that depend on them; adjust mission XP and creature-level pacing (Docs/Progression.md:
the player should arrive at each boss at its level); give Cyrus a short build-up in the main story's words (who he is, why he
matters before Abel), within the talk-cut rule. (37) "There should be a quest to unlock Lost Loot": a short side mission (from
the depot's agent, Mr. Sloane, or whoever fits) that opens the Lost Property window (`OpenWhen`); before it, guns left on the
ground must not vanish into a locked window (hold the ground clock or keep them). Gate it early enough to be useful.

## WORLD (Opus): notes 16, 26 (chest spots)
Owns: `Art/Levels/area_places.py`, `area_mosaic.py` (road paint only), `Tools/Unreal/build_world_materials.py` (M_Terrain road
term only), `Tools/Unreal/build_area_dressing.py` (road edge props only), `World/MinimapSubsystem*` + `UI/Inventory/MapView*`
(roads on the maps), `Tools/Unreal/build_area_caches.py`, `build_area_loot.py`, `story_texts.py` SECRETS anchors only (tell
STORY, who owns the file: put the exact edit in your report), layout patches `world_*.json`. (16) "The map doesn't really
prompt the player to use roads, it feels like a free for all": make roads read and pull: stronger road look in the terrain
(ruts, worn edges, contrast), roadside markers (posts, cairns, signposts at forks with place names, lanterns near town), roads
drawn on the minimap and the map page. Cheap (8.3 ms). (26) "Chests should be in more distinguished locations": move chests to
spots that look like a reward (a ledge, a hidden nook, behind a waterfall, atop a ruin, a camp's tent), each found by a view
or a little climb; list old -> new spots for Main.

## OPENING (Opus): notes 1, 2 (UI)
Owns: new `UI/Menus/StartupLogos*`, `UI/Menus/MainMenu*`, `Core/LooterMenuGameMode*`, `UI/Menus/HeroSelect*` (look only).
(1) "Opening up the game should show creator logos, software, etc. just how gearbox does when you open BL": a skippable startup
sequence before the menu (any key skips one card; first launch shows all): a placeholder studio card (name from a config value,
e.g. "PLACEHOLDER STUDIO", an original simple mark, the user renames later) and a "Powered by Unreal Engine" card that will use
Epic's official logo image (leave an asset slot `/Game/UI/Logos/T_UnrealEngineLogo` and show a plain wordmark until it's
imported; Main asks the user to download the official logo), each with a short sting cue (script-made recipe in Art/Sounds).
(2) "Main Menu Screen is very AI generated and needs some love": redesign the main menu so it feels authored: a strong game
title treatment (an original logo lockup for the working title, Chakra Petch), fewer boxes, the column over the planet scene,
subtle motion, button feedback with sound; Concept C kit only. Screen split with PLANET: the menu column and title live in
the left 40% of the screen at 16:9; PLANET frames the planet in the right 60%. No API between them.

## PLANET (Opus): note 2 (the backdrop)
Owns: new `Tools/Unreal/build_menu_teropa.py` (makes `/Game/Maps/Lvl_MenuTeropa`), new materials via a new
`Tools/Unreal/build_menu_materials.py` (M_Planet_Teropa etc., under /Game/Art/Materials/Menu), `Core/LooterMenuPlayerController*`
(the menu camera), `Art/Models/Menu/` (if a mesh is needed; an engine sphere is fine). "Teropa the planet is the background":
Teropa from orbit: a slowly turning planet (dusty ochre/rust continents and pale seas fitting the frontier world, cloud bands
on their own layer, a thin atmosphere rim, night-side town lights), a star field, the sun's glare; the camera framing leaves the
menu column's side clear; under 4 ms on Medium (it's a menu). Describe the planet in Docs/Story.md terms. Main switches
`GameDefaultMap`/menu map in config after checking it.

## INTRO (Opus): note 3 (first pass; full cutscenes with animated characters later)
Owns: new `Story/NewGameIntro*`, new `UI/Story/IntroWidget*`, its lines (a new set in `Tools/Unreal/create_story_lines.py` is
STORY's file: write them in `Saved/INTRO3Code/lines.py` for Main to paste), recipes for its sounds. "Upon starting a new game,
we need a better story, what is the game about, how did we get here": a skippable 60-90 s intro on a new game (once per
session, before control): who the player is, what Teropa is, why they're in Ransom's Rest, the hook, told in short captions
over in-engine shots (the planet from PLANET's scene, then camera moves over Ransom's Rest), title card at the end. Story from
Docs/Story.md (keep the canon; propose any new line). The hook point into a new game: say the exact call for Main.

## Shared (Main)
Main builds, imports, runs scripts, switches the menu map, play-checks, commits. Order of merges as each lands.
