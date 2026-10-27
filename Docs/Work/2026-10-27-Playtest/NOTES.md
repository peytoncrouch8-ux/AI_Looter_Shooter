# Playtest notes, 2026-10-27 (test build 2026-10-23_0658)

The user's notes, word for word (typed in Excel, one line each):

1. Opneing Up Game should Show creator logos, software, etc. just how gearbox does when you open BL
2. Main Menu Screen is very ai generated and needs some love ( it is using skyreach as the background too so we need to further the game before changing the art) [ IDEA: Teropa the planet is the background?)
3. Upon starting a new game, we need a better story, what is the game about, how did we get here, character models, animations and cut scenes, etc.
4. As we hover over the buttons in the settings, it should display what these settings do specifically and should be tabbed sections instead of one long scroll page
5. Melee animation is jank, we want each character to have a specific tool that they use and hits with the right hand
6. Odessa 52 card pickup shoots tiny particles, needs adjusted with better animations and bigger card like projectiles.
7. Grenades are too strong to start with, projectiles are too small and lacks explosion visuals ( IDEA: Add grenade classes to have grenades do specific things)
8. Opening inventory should pause the game
9. Wanted poster not showing by amos's house
10. terrain is still hella whack we need to take snips and show claude where and how to fix
11. Apples floating around trees
12. Bounty board should be placed elsewhere (By train station??)
13. General store needs "Refill All Ammo" button that displays the total cost to do so
14. Legendaries are super easy to get. they need to be more difficult especially at the beginning of the game
15. Staircases around the map are invisible
16. The map doesn't really prompt the player to use roads, it feels like a free for all
17. When a legendary drops, the game stutters. There should be a galactic explosion instead of a game stutter.
18. What is a reliquary, why am I looking at it?
19. I dont want money showing on HUD
20. Knocking Below Quest spawns an unpaid above the cellar
21. Gravemother fight and Abel are way to easy and die too quickly as far as progression goes. Also I can just sit above the sink and shoot her without her aggro'ing to me. Also we need cutscenes for our bosses
22. All bounties are only the unpaid, and they are only spawning the grove behind the general store.
23. Unpaid have terrible ai, need better attacks and targeting sequences
24. Only one fast travel station in the lower half of RR and one on the mountain
25. Maybe the game should scale as we level up (dont decide this until we have the gilded lily made)
26. Chests shouldn't show on the map and they should be in more distinquished locations
27. Action skill activation animation
28. More recoil so the higher the accuracy stat is, the more prominent the effect
29. Normal weapons should have an idle sway jjust like the sniper does when you zoom in
30. Farm house atop the mountain has floating fences /// When entering the pond up there as well, when you enter it, it is as if the water disappears and there are no swimming or underwater effects.
31. Back pay mission, when I take the tags it gives me a UI to read something
32. Cyrus teage should be apart of the main story line. Abel should be the final boss so that means cyrus is the second boss fight (the story and progression should adjust to this change)
33. Bounty fights should be slightly harder. So enemies need more health or weapons need to do less damage
34. Gravemother fight: You need to break the tombstone on her back before you can damage her
35. Gun iron sights need changed
36. Vigil stones seem to be llinked, each one should have its own countdown
37. There should be a quest to unlock Lost Loot
38. As we do missions, there should be "Gates" that "Open" as you complete tasks in the missions. As it stands, you can speed run quests and just spam click the use button.
39. Most mobs are not frightening enough as enemies, gotta raise the stakes!
40. When completing a bounty, there should not be a chest that spawns that gives the loot, it should be given when the bounty is turned in to the bounty board
41. When there is a large amount of loot in one tile/area, the weapon names just stack and clutter the screen

## The user's calls (2026-10-27)

- Sniper ground ring/column: dropped (done d0e6d58).
- Order: round 1 bugs + quick asks; round 2 fights (bosses, bounties, Unpaid AI, grenades, legendaries, recoil); story, cutscenes,
  logos after.
- Fast travel: keep two (one in lower RR, one on the mountain). Main's reading: the Sheriff's Office roost (the user's 10-23 "Town
  roost" for death respawns) stays as a respawn point only, not a travel stop; the Depot and Calder's Post are the two stations.
- Legendaries: rarer early; normal enemies and chests almost never in RR; bosses (and their bounties) still can.
- Recoil: more kick on every gun; a high accuracy stat calms it a lot (the stat must clearly matter).

## First sort (Main)

- Bugs: 9 poster, 11 apples, 15 stairs, 17 legendary hitch, 20 Knocking Below, 21 Gravemother aggro, 22 bounty kinds/places,
  30 fences + pond, 31 Back Pay tags, 36 vigil stones, 41 loot names.
- Clear asks: 8 inventory pauses, 13 Refill All Ammo, 19 no money on HUD, 26 chests off the map, 40 bounty pay at the board,
  4 settings tabs + hover help, 24 more fast travel, 12 move the board.
- Tuning: 7 grenades, 14 legendaries rarer early, 21/33 bosses and bounties harder, 28 recoil by accuracy, 29 idle sway,
  39 scarier mobs, 23 Unpaid AI, 34 Gravemother tombstone phase, 38 mission gates.
- Art/anim: 5 melee tool per hero, 6 Odessa cards, 27 action skill anim, 35 iron sights, 10 terrain (user sends shots),
  16 roads, 17 legendary "galactic explosion".
- Big (story, new systems; core-loop freeze): 1 boot logos, 2 menu, 3 new-game story + cutscenes, 21 boss cutscenes,
  32 Cyrus second boss / Abel final, 37 Lost Loot quest, 7 grenade classes, 25 level scaling (after the Lily).
- 18: answer the user's question.
