# Lane RENAMES: Skills, the Warden's Log, Wickcutter, lantern posts (done, not built)

The user's calls (DECISIONS.md "More calls"). Code names, asset names, ids and save data unchanged everywhere: only words on
screen, the post's model and the lamp's look changed. Nothing built, nothing run, nothing committed.

## Files changed (all mine; no new files, so CODEMAP needs only wording: see the end)
- Skills: `UI/Embers/EmbersWidget.cpp`, `UI/Inventory/LoadoutParts.cpp/.h` (tab words + `PageNamed` aliases),
  `UI/Inventory/MissionsWidgetDetails.cpp`, `UI/Bestiary/BestiaryWidget.cpp`, `UI/Menus/HeroSelectWidget.cpp`,
  `UI/Menus/HeroSelectConfirm.cpp`, `UI/Menus/SettingsMenuHelp.cpp`, `Settings/KeyBindingSubsystem.cpp`.
- Warden's Log: `Bestiary/Ledger.cpp/.h`, `Tutorial/ControlHintRulesTable.cpp`, `World/FindRules.cpp`,
  `UI/Inventory/MapWidgetFinds.cpp`, `Missions/MissionPlayerObjectives.cpp`, `Tools/Unreal/build_area_finds.py`.
- Wickcutter: `Tools/Unreal/create_named_weapons.py` (display name only), `Weapons/LegendarySignature.h` (comment).
- Lantern posts: `World/RoostKit.h`, `World/RespawnMarker.h/.cpp`, `World/RespawnMarkerRoost.cpp`, `World/RespawnMarkerSave.cpp`,
  `World/RoostRules.cpp`, `World/GraveTravelRules.cpp`, `UI/HUD/HudGraveMarkedWidget.h/.cpp`, `UI/HUD/LooterHUDGraves.cpp`
  (comment), `UI/Inventory/MapIcons.cpp`, `UI/Inventory/MapPins.cpp`, `UI/Inventory/MapWidgetCard.cpp`,
  `Tools/Unreal/build_area_roosts.py`.
- Tests: `LedgerTests.cpp`, `EmberUITests.cpp`, `EmberTests.cpp` (comment), `PlayCheckFixTests.cpp`, `FindBannerTests.cpp`,
  `FindRulesTests.cpp`, `GraveMarkedBannerTests.cpp`, `RoostPlayTests.cpp`.

## 1. "Skills" (hero abilities)
Changed: inventory tab "Embers" -> "Skills" (`PageTabName` 1), the page title "SKILLS" / "<HERO>'S SKILLS", "No skills yet",
key prompts "2 Skills" (bestiary and missions pages), hero select section "Skills" and its note ("...the inventory's Skills
page"), hero select's in-the-works status ("model and skills"), key binding display name "Ember power" -> "Skill"
(id `Ember` unchanged, saved keys still bind), settings help ("Uses your hero's equipped skill...", inventory help "your
loadout, skills, the Warden's Log, missions and map").
Left: every code/asset name (`UEmberComponent`, `EInventoryPage::Embers`, `DA_Ember_*`, `Ember.*` cues, `Looter.Embers.*`
commands), cvar help and logs, the gun element "Ember" (`ELooterElement::Ember`, a burn: not a hero ability), lore words
("a spark of ember" in create_ember_assets.py's story text, Saint Ada's ember). The HUD socket shows only the skill's own
name and key: no word to change. No `EControlHint` for skills exists.
`{Page.Embers}` still resolves (alias), and `{Page.Skills}` now too.

## 2. "The Warden's Log" (the bestiary / Sexton's Ledger)
Decision (mine; say if wrong): the book is "Warden's Log" on screen from the first page, open or not (a player in Landfall
sees the new name at once; "Bestiary" before Main 2 would look like the rename wasn't done). `Ledger::BookName` returns
"Warden's Log" both ways; `Ledger::Words` is one Warden's-voice set (title "The Warden's Log", "entry/entries", "Not met yet",
help lines; no "friend", no Sexton voice); opened, only the names' blank whereabouts line differs. The gate (Main 2 step 3
opens the story pages, `bLedgerOnly`, `LedgerName`) is unchanged.
Changed also: control hint "Then 3: the Warden's Log" (hint id stays `Ledger`, saves keep it), the find banner "LOG PAGE ·
2 / 6" (was "LEDGER PAGE"), the map's finds block "LOG PAGES 2 / 6" (title box widened 52 -> 68 px past the tally), the
default open-page objective "Open the Warden's Log ({Inventory}, then {Page.Bestiary})", the find spots' names "Log page:
<Town>" (build_area_finds.py; takes effect on the next `build_area.py RansomsRest gameplay`).
`PageNamed` (MissionText's `{Page.X}`): tab words plus aliases Embers, Bestiary, Ledger, WardensLog, Log (mission assets
still write `{Page.Bestiary}`/`{Page.Ledger}`).
Left: code names (`Ledger::`, `bLedgerOnly`, `EBestiaryPage::LedgerName`, `Ledger.Page.<Town>` flags, `LedgerPage_*` actors,
`SM_LedgerPage`), dev command text (`Looter.Story.Ledger`), the settings menu's "Ledger & Brass" look (a style name, not the
book), gun notch names ("Dust Ledger" etc., WeaponNotches.cpp), Sexton's own ledger in his lore/lines.

## 3. "Wickcutter" (the legendary named Lamplighter)
Changed: `create_named_weapons.py` `name='Wickcutter'` (with a comment). Main runs the script to update `DA_Named_Lamplighter`'s
display name. Kept: asset `DA_Named_Lamplighter`, `LegendaryDrops::LamplighterId` ("Lamplighter": rewards, drops,
`Looter.GiveWeapon`, saves), `ELegendarySignature::Lamplighter`, `Looter.Legendary.Lamplighter.*` cvars, the card line
("Scoped kills leave a lantern...") and the flavor line "Leave a light on." No story line names the gun.

## 4. Lantern posts (respawn)
Today's code had already replaced the graves with Hob's roosts ("ROOST FOUND", the Crow's Mast); this turns those into
lantern posts. Same class `ARespawnMarker`, same ids, `FCampaignRecord::LastGraves/MarkedGraves/ActiveRespawns`, same rules
(walk within 9 m: lit; a death wakes at the last lit post; fast travel by Hob's crows): old saves' marked graves / found
roosts are lit posts with no migration.
- Model: `RoostKit::PostPath` = `/Game/Art/Props/SM_KeeperLanternPost` (BurialDeck.py's KeeperLanternPost, 3.2 m post, braced arm
  toward +X, iron lantern, LanternGlow glass, SOCKET_Light): THE one path; the constructor finds it, `build_area_roosts.py` no
  longer sets `roost_mesh` (reads the class default for a Wake socket). A level saved with SM_HobRoost on a roost shows the
  post anyway (`FitRoost` swaps the mast for the class default), so no rebuild is required; rerunning
  `build_area_roosts.py RansomsRest` is cleaner (it saves the level without the mast override).
- The stand-in post's sockets (RoostKit fallbacks, cm in the post frame, +X the arm): Wake (170,0,0) as before (FALLBACK_WAKE
  unchanged), Beacon = the model's Light socket (fallback (90.5,0,236), the lantern glass), Bell (64,0,302) under the arm
  (the old SM_HobRoostBell kept, rung as before), Speaker (0,0,330), six perches on the post top, along the arm, the lantern's
  cap and the brace (all 2.76-3.22 m). Greybox (no model): post + arm in wood, lantern in iron, footing in stone (2/1/1 boxes).
- Lighting moment: the lamp's glass is dark (MI_HouseTrim on the LanternGlow slot, the keeper's posts' trick) until lit;
  lit, its own glow returns as the beacon ignites (column rises from the lamp, point light at the lamp flares 5x then
  settles, bell rings, crows lift). Sound: the existing `World.Roost.Found` (bell, crows) plus the old grave lantern's
  `World.Grave.Marked` at the lamp (0.9; "Again": `World.Grave.Remarked` at 0.7): both already imported.
- Words: banner "POST LIT" (`UHudGraveMarkedWidget::TitleWords`) with a new painted picture (post, arm, lit lantern with its
  rising column and glow, Hob's crow on the arm); map/minimap icon now a post with a hung lantern (was the crow's foot);
  map pins "Lantern post: fast travel", legend "Lantern post"; travel card "Save point"/"Lantern post", "CHOOSE A POST",
  "NO POST LIT" + help, list header "LANTERN POSTS · N" / "NO POSTS LIT YET", prompt "G Next post"; travel refusals "Hob's
  crows don't fly to this post..." / "This post isn't lit yet. Walk up to it first."; Hob's once-only lines: first post
  "That's a Warden's post lit. My crows fly you between them, and back to the last one when you drop.", first death "Up you
  get, sunshine. A Warden doesn't stay down while a post still burns." (was "You can't stay dead with the debt unpaid").
  Left: Hob's four death lines (they have hero variants keyed by text in hero_lines.json; "Easy. Breathe. Well, pretend to."
  hints at undeath: STORY may want to change it there and in RoostRules.cpp together), logs, dev commands, cue names,
  `URoostSettings` ("Hob's Roosts" project settings), the colour `LooterUI::Color::Roost`.

## Edits for Main in other lanes' files (exact)
1. `Tools/Unreal/create_mission_assets.py` line 547 (STORY):
   old `'Open the Ledger: {Inventory}, then {Page.Bestiary}.'`
   new `"Open the Warden's Log: {Inventory}, then {Page.Bestiary}."`
2. `Tools/Unreal/story_texts.py` line 419 (STORY):
   old `title='A page of the Ledger: ' + _page['name'],`
   new `title="A page of the Warden's Log: " + _page['name'],`
   (the pages' own texts may still speak as Sexton's ledger: STORY's call when Main 2 is redone.)
3. Optional, `Tools/Unreal/hero_lines.json` line 643 (Pike): "Sixty-one, if the Ledger's right." -> "...if the Log's right."
   (only if STORY agrees; it's a hero's line about the old book).
4. Docs (Main): `Docs/Pipeline/Game.md` "Hob's roosts" bullet -> lantern posts (model `RoostKit::PostPath`, stand-in
   SM_KeeperLanternPost, POST LIT, lamp glass dark until lit, `World.Grave.Marked` layered); the legendary "Lamplighter" ->
   "Wickcutter (id Lamplighter)"; `Docs/Pipeline/EmberPowers.md`: "Skills" on screen, code Ember*; `Docs/Story.md` line 53
   (the gun's name clash is resolved: Wickcutter).
5. `Docs/Work/2026-10-31-Wardens/TRIPO_MODELS.md`, add under props:
   `**Lantern post.** The Wardens' save and respawn post: a weathered wooden post about 3.2 m tall on a cross foot, a braced
   arm reaching about 1 m out with an iron lantern (glass panes, a cage, a cap) hung from its end, a small iron bell hook
   under the arm, a Wardens' lantern badge burnt into the post at eye height. Arm toward +X, pivot at the foot; glass as its
   own material slot named LanternGlow, a Light socket at the flame (optional sockets: Beacon, Bell, Wake, Perch_1-6,
   Speaker). Swap: World/RoostKit.h PostPath.`

## Main's run order
1. Build. 2. `create_named_weapons.py` (Wickcutter). 3. Optional: `Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_roosts.py RansomsRest"`
(open level; it saves it) or let `build_area.py RansomsRest gameplay` do it; then `build_island_scatter.py` as that script says.
4. Tests: `Looter.Bestiary.Ledger`, `Looter.Embers.Pages`, `Looter.Missions.PageKeys`, `Looter.UI.FindBanner`,
`Looter.Finds.Rules`, `Looter.UI.GraveMarked`, `Looter.Roost.Play`, `Looter.Map` (pins/legend unchanged in shape).
5. PIE: walk to the Depot / Calder's Post / Sheriff's Office posts: the post stands (not the mast), the lantern glass dark, then
lit with the flare, POST LIT banner, the bell under the arm swinging and not clipping the brace or the ribbons, crows on the
arm/post top; check the lantern really is on +X (the player wakes past the lantern facing away; if the arm points the other
way, the bell and perches float: flip the X of the fallbacks in RoostKit.h); open the inventory: tabs "Skills", "Warden's Log".

## Risks I couldn't check
- SM_KeeperLanternPost's arm axis in UE (+X assumed from AKeeperLanternPost's FallbackGlass) and its glass slot name
  (LanternGlow, found by name or by a material named so, as AKeeperLanternPost does).
- MI_HouseTrim on the post's glass reads as dark glass (works on the deck posts).
- `FitRoost` swapping the saved mast in OnConstruction marks the RR level's roosts changed in the editor (harmless).
- The beacon's 1.3 m flame cards (URoostSettings::FlameSize) now sit round a lantern at 2.4 m instead of a basket at 4.8 m:
  maybe too big; tune `FlameSize` (e.g. 70) / `HaloScale` in Project Settings > Hob's Roosts after a look.
- `LoadoutParts.cpp` was 533 lines before; now 537.
