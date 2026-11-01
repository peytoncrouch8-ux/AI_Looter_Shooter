# Lane CREW: Delia Ransom and the Lamplighter's crew as characters (2026-10-31)

Nothing built, run or committed (lane rules). All new files; no file of another lane touched.

## The body (the choice and why)

Every story person = the hidden mannequin (SKM_Manny_Simple) playing **MM_Idle** (the mannequin's captured unarmed idle)
in a native anim instance, with a **code layer on top** (WardenCrewPose: hips lowered/turned with feet planted, spine
lean/sway/twist, head look, two-bone IK hands to world spots or to a two-handed gun), worn as a **hero's body standing in**
through `UBodyFigureComponent::SetShownHero` (the hero select stand's way), tinted by the character masters' `Tint`.
Picked over AStoryCharacter's code-only posing: a captured idle breathes, shifts weight and settles as no sine does; the
hero bodies are the most finished people in the game; the hands land exactly where the code puts them (the retargeters
pin hero hands to the mannequin's). The Tripo people (rigged to the mannequin skeleton) are worn as the mannequin mesh
itself, same anim, no figure.

## Files (all new)

| File | What |
|---|---|
| `Story/WardenCrewPose.h/.cpp` | `FWardenHand`, `FWardenGunHold`, `FWardenPoseInput`; `WardenCrewPose::Apply` (the layer on a local pose), `GunFrame` (shouldered/carried/broken/kicked gun frame), `GunSocket` "HandGrip_R", limits (look 75/35/40, body turn 60). |
| `Story/WardenCrewAnimInstance.h/.cpp` | `UWardenCrewAnimInstance` + proxy: MM_Idle on the person's own clock, then `Apply`; `SetPoseInput` from the actor each tick. No Anim Blueprint. |
| `Story/WardenCrewLife.h/.cpp` | `FWardenLife`, `FWardenLookSense`: where they look (listener > focus-only > player near, watched 3.5-6.5 s then away 1.2-2.6 s, 0.35 s notice beat > glances every 3.5-8 s > station focus), eased head; `Brace` (on 7/s, off 1.8/s) and a ringing sway (7 deg roll, 3 deg pitch, decay 1.6/s). |
| `Story/WardenCrewBody.h/.cpp` | `FWardenLook` (own-model soft path, scales, stand-in hero, tints, glow), `EWardenDetail`; `WardenCrewBody::MakeBody`, `WearLook`, `DressStandIn` (tints per master: Skin/Hair/Glow/cloth; hides `*HolsterGun`), `DetailFor`/`ApplyDetail` (full <30 m, 15 Hz <80 m, held + no shadow beyond), `PlayerDistance`, `PlayerEyes`, `EyesOf`, `YawTo`. |
| `Story/WardenCrewMember.h/.cpp` | `EWardenCrew {Captain, Keeper, Engineer}`, `FWardenStation`, `AWardenCrewMember : AStoryCharacter`. |
| `Story/WardenCrewStations.cpp` | The one table: names, tags, own-model paths, stand-ins/tints/scales, the three stations. |
| `Story/DeliaRansom.h/.cpp` | `EDeliaFight`, `ADeliaRansom : ACharacter, IInteractable` (body, talk, story shown, turn, eyes, pose). |
| `Story/DeliaRansomFight.cpp` | Targets, blasts through `UBulletSubsystem`, reload, the floor (`OnBulletLanding`), flinch, barks. |
| `Story/DeliaRansomShotgun.cpp` | Stand-in gun from Ranchhand heritage parts / the Tripo gun; muzzle; flash (`FWeaponFX::SpawnFlash` + `SpawnMuzzleBurst` Shotgun). |
| `Story/DeliaRansomRules.h/.cpp` | `EDeliaBark`, `FDeliaTarget`, `DeliaFightRules` (all numbers; `Choose`, `PelletDamage`, `LandingScale`, `SpreadFor`, `PlayerInLine`, `ReloadPose`, `BarkLines`). |
| `Tests/WardenCrewTests.cpp` | 9 tests (below). |

## CODEMAP lines (Main writes; under Story/)

- `Story/WardenCrewPose`: `FWardenPoseInput` (`FWardenHand`, `FWardenGunHold`), `WardenCrewPose::Apply` (the story people's layer over the mannequin's idle: hips down/turned on planted feet, spine lean/sway/twist, head look, hands by two-bone IK to world spots or a two-handed gun on HandGrip_R), `GunFrame`.
- `Story/WardenCrewAnimInstance`: `UWardenCrewAnimInstance`, a story person's mannequin with no Anim Blueprint: MM_Idle on its own clock, then WardenCrewPose; the actor sets the pose each tick.
- `Story/WardenCrewLife`: `FWardenLife`, `FWardenLookSense`: where a story person looks (listener, focus, the player near a few seconds at a time, glances), eased; the brace and its sway.
- `Story/WardenCrewBody`: `FWardenLook`, `EWardenDetail`, `WardenCrewBody` (the mannequin + a hero stand-in via `UBodyFigureComponent::SetShownHero`, tinted; own model on the mannequin skeleton worn as the body; detail by distance).
- `Story/WardenCrewMember`: `AWardenCrewMember` (`EWardenCrew`, `FWardenStation`), the Lamplighter's crew on deck: Captain Holloway at the wheel, Brother Finch at the Great Lantern, Junie Calloway at the engine (wrench stand-in); tags `Speaker_Holloway/_Finch/_Junie`; station hands and work, looks, turn to a talker, `Brace`/`BraceAll`. `WardenCrewStations.cpp` their table (names, tags, model paths, stand-ins, stations).
- `Story/DeliaRansom`: `ADeliaRansom`, Delia on her porch (`Speaker_Delia`, "Delia Ransom"): shotgun carried low, glances, faces a talker; fights creatures within 18 m (`DeliaRansomFight.cpp`: blasts via UBulletSubsystem every ~1.8 s, 25% of a target's health, never crit, never below 10%, reload after two, holds fire with the player in line, barks DA_Lines_DeliaFight); never dies (invulnerable health, flinch); `SetFighting`/`ClearFightingOverride`; `DeliaRansomShotgun.cpp` the gun. `Story/DeliaRansomRules`: `DeliaFightRules`, `EDeliaBark`, `FDeliaTarget`.
- `Tests/WardenCrewTests.cpp`: `Looter.Story.WardenCrew.*`, `Looter.Story.Delia.*`.

## Stand-ins and the one soft path each (swap = put the asset at the path; no code change)

| Who / what | Own model path (Tripo, TRIPO_MODELS.md #) | Stand-in now | Scale (stand-in / own) |
|---|---|---|---|
| Delia | `/Game/Art/Characters/SK_Delia` (1) | Odessa, cloth x(0.3,0.28,0.3), hair x1.8 | 0.79 / 0.85 |
| Holloway | `/Game/Art/Characters/SK_Holloway` (2) | Odessa, cloth x(0.38,0.5,0.5) | 0.86 / 0.85 |
| Finch | `/Game/Art/Characters/SK_Finch` (3) | Crane, cloth x(0.62,0.55,0.45), skin x(1.25,1,0.88), glow 0.08 | 0.78 / 0.85 |
| Junie | `/Game/Art/Characters/SK_Junie` (4) | Pike, cloth x(0.42,0.5,0.68) | 0.79 / 0.85 |
| Delia's shotgun | `/Game/Art/Props/SM_DeliaShotgun` (7; sockets `Grip`, `Foregrip`, `Muzzle`; +X barrel) | Ranchhand Body/Barrel Heritage, Muzzle Crown, Stock Field, Magazine Tube6, Pump Walnut, wear 0.85 | body's |
| Junie's wrench | none on the list (add one if wanted) | engine Cylinder + Cube, steel | - |

Import rule for the Tripo people: **skeleton = SK_Mannequin** (else a warning and the stand-in stays); the mesh needs the
`HandGrip_R` socket if it's a mesh socket on SKM_Manny_Simple rather than on the skeleton (check; Delia's gun hangs there).

## For FLIGHT (the Lamplighter)

- Class `AWardenCrewMember` (`/Script/AI_Looter_Shooter.WardenCrewMember`). Spawn three, `SetCrew(EWardenCrew::Captain|Keeper|Engineer)`
  (or SpawnActorDeferred, set `Crew`, FinishSpawning), then `AttachToActor(Ship, KeepWorldTransform)` (or child components'
  owner: any attach is fine; they're plain actors, no movement component). **Actor origin = their feet on the deck, +X facing
  their station.** Hull: 55 cm wide, 180 x scale tall, blocks pawns (Dredgers walk round), ignores Weapon/Camera.
- Where the props must be (world cm from each one's feet, at their stand-in scale; from WardenCrewStations.cpp x scale):
  wheel rim centre ~(40, 0, 95), rim radius ~22 (Holloway 0.86); lantern cage centre ~(30, 0, 134), hands at its sides
  ±12 (Finch 0.78; the lantern's flame focus (43, 0, 139)); engine works ~(44, 10, 68), left hand braced at (40, -20, 79)
  (Junie 0.79, leaning 22 deg, knees 9 cm). Fit with `HandsShift` (actor-space cm, both hands) instead of moving them.
- Lurches: `AWardenCrewMember::BraceAll(this, Seconds)` (or `Crew->Brace(Seconds)`): grapples biting 1.2 s, the stop 1.5 s,
  bag tear 1.5 s, hard landing 3.0 s. Re-calling rings the sway again.
- Moored for good: `SetAtStation(false)` if they should stand at ease (hands free, no lean); default stays at station.
- Talk: their speaker points are `bMissionsOnly` (no prompt without story words); STORY/WORLD put topics on `SpeakerPoint`
  (`speaker_point` in Python) and clear `missions_only` if they should always talk. Speaker names are exactly
  "Captain Holloway", "Brother Finch", "Junie Calloway" (STORY's line sets use the same).
- Hidden while sailing isn't needed (they're on deck the whole time); `ShownWhen` (AStoryCharacter) works if wanted.

## For WORLD (`build_area_farm.py`)

- Place `unreal.DeliaRansom` (`/Script/AI_Looter_Shooter.DeliaRansom`) on her porch where she should stand: **Z = porch floor
  + 68 cm** (the actor is her capsule's middle; capsule r 26, half-height 68), yaw facing the yard/road (where FarmPorch's
  enemies come from: she turns 220 deg/s onto targets and back to this facing when calm). Her porch (fight centre, 18 m
  flat) = where she's placed. Keep ~70 cm free round her; the porch barricade must not block her eye line at ~1.3 m over
  the floor (she shoots only what her eyes see: Visibility trace).
- **Delete the screen door's `ASpeakerPoint` tagged `Speaker_Delia`**: the tag must be on her alone (the talk/reach/turn-in
  objectives take the first actor with it). Move its topics onto her: `delia.get_editor_property('speaker_point')`
  (STORY.md's `place_delia` edit applies to it; `speaker_name` is already "Delia Ransom"; her `bMissionsOnly` is false).
- Optional: `shown_when` (FStoryCondition) if she should be off the porch in some missions (Main 6/7 still hand things
  through the door: `ADoorHandoff` is separate and untouched). `fight_barks` defaults to `/Game/Data/Story/DA_Lines_DeliaFight`.

## For STORY

- `DA_Lines_DeliaFight` order is load-bearing (`DeliaFightRules::BarkLines`): 0 "Off my porch!" = a fight's first sight;
  3 "Reloading! Cover me" = reload; 4 "Hose-man coming!" = a hose-man in reach; 2 (or 1) = the one she shot at goes down;
  6, 5 = she's hit; 7, 1, 5 = she's knocked one to the floor for the player. Never the same line twice running, 7 s apart at
  least, never over a caption on screen, queued. Reorder the set = change that table. No line yet for the all-clear
  (`EDeliaBark::Clear` stays silent until the table names one).

## Hooks for Main in others' files (exact)

1. **Bestiary** (`Tools/Unreal/create_bestiary_pages.py`, DREDGER's, add-only; words for STORY/Main to check). CLAUDE.md:
   every NPC/friend gets a page. Replace the `'Delia'` dict's
   `name='Grandma Delia Ransom',` -> `name='Delia Ransom',`,
   `habitat="Ransom Farm: behind the screen door",` -> `habitat="Ransom Farm: her porch",`,
   `description=(...)` -> `description=("Abel Ransom's mother. Forty years on that farm, and she's not leaving it to Vane's men or "
   "anything they stir up."),`, `notes=[...]` -> `notes=["She keeps her shotgun loaded and her porch clear.", "She softens them up. Finishing is your job."],`
   and add (beside 'Hob', `section='FRIEND'`, `page='STORY_CHARACTER'`, no actor, `known_when=dict(after=[])` or as STORY says):
   `'Holloway'` (`DA_Bestiary_Holloway`, name 'Captain Mercy Holloway', kind 'The Lamplighter's captain', habitat 'The Lamplighter: the wheel'),
   `'Finch'` (`DA_Bestiary_Finch`, 'Brother Tobiah Finch', 'Lantern-keeper', 'The Lamplighter: the Great Lantern'),
   `'Junie'` (`DA_Bestiary_Junie`, 'Junie Calloway', 'The Lamplighter's engineer', 'The Lamplighter: the engine'),
   each with a one-line description and `sort_order` after Hob. (Story pages need no ActorClass: `Looter.Bestiary.Entries`.)
2. Optional, `Combat/EnemyProjectileSubsystem.cpp` `Victims`: today only players; Dredger rifle pellets aimed at Delia pass
   through her. To see them land (she flinches and shouts), add `ADeliaRansom` actors to `Victims` (her capsule is a
   character's). Not needed for the fight to work.
3. Docs: `Docs/Pipeline/Heroes.md` (or Story.md) a section "Story people's bodies (2026-10-31)": the body paragraph above,
   the stand-in table, the Tripo import rule, the tests.

## For DREDGER (check, not edit)

- Creatures she hurts take her up as prey (`CheckHunt`: a character with health; her shots are blamed on her AI controller).
  The Dredgers' code must not assume `Target` is the player: a hose-man latching Delia should either skip non-players or
  be harmless (her health `Drain`s nothing; her `LaunchCharacter` is a no-op flinch; anything moving her with
  `SetActorLocation`/movement input would move her off the porch). The screen-edge drain effect must be the player's only.
- She warns of hose-men by class name containing "HoseMan" (`ADredgerHoseMan`): no include of theirs.

## Delia's fight (numbers; `DeliaFightRules`)

Reach 18 m flat from her porch; scan 5/s; raise 0.35 s, lower 0.6 s; blast every 1.8 ± 0.2 s; 2 shells, reload 2.4 s
(break open 0-0.15, hand to pocket 0.18-0.38, back 0.48-0.68, shut 0.8-0.92); 9 pellets, each target MaxHealth x 0.25 /
(9 x 0.8) (a 140 HP rifleman: 4.9 a pellet, ~35 a blast); spread atan(35 cm / dist) clamped 1.5-5 deg; pellets 22000 cm/s,
range 39.6 m, on the Weapon channel, never into the player (`IgnoreActor`), never critical, landing scaled so no hit takes
a target below 10% (`LandingScale`); a target at <15% isn't shot (she covers it and calls the finish); aim tolerance 10 deg
(the chest leads the feet up to 35 deg); player within 70 cm of her line, between her and it, holds fire; ready 2.5 s after
the last creature; tops off at ease. Health 500, invulnerable, no numbers; a hit or a shove = flinch (chest back 7, roll 4)
and maybe a bark (60%). Talking is refused only while the gun is up on a target.

## Tests (run once at commit: `Tools\runtests.ps1 -Filter Looter.Story.WardenCrew`, then `-Filter Looter.Story.Delia`)

`Looter.Story.WardenCrew.Roles` (names, tags, model paths, stand-in from the roster not Ellis, anim class, hull channels,
Junie's wrench), `.Looks` (notice/behind/far, watch then look away, listener, glances), `.Brace` (rules + `BraceAll` on an
actor), `.Pose` (on the mannequin's ref pose: wrist on its spot <1.5 cm, hips -10 with feet kept <1 cm, head turned 30-60
deg to a 45-deg look; gun frame shouldered on the aim, carried low-left), `.Station` (hands at the station x scale; a talk
turns them, Finch lets go, the captain keeps her right hand and turns <=35);
`Looter.Story.Delia.Rules` (reach, 25%, the floor, worth, choose/cover, spread, line, reload pose, bark table), `.Fight`
(spider at 25 m calm; at 10 m Ready, blast, no talk; two shells then reload; pellets never below the floor (warns if they
can't land in a test level); SetFighting false/true; calm again), `.NeverDies` (1e6 damage: whole, flinched; a shove
doesn't move her), `.Talks` (tag, "Delia Ransom", usable when clear, turns to face the talker, gun low, eyes on them).
Warnings (not failures) where a checkout lacks the mannequin, a hero body or DA_Lines_DeliaFight.

## PIE check for Main

1. New game / `Looter.Landfall 0`: on deck, F5 third person. Holloway hands on the wheel (rim moves slowly), Finch hands up
   at the lantern trimming, Junie leaning in working the wrench in bursts. Walk up to each from the front: head comes round
   after a beat, a few seconds, looks away, back. Grapple hit / `Looter.Landfall.Ship Grappled`: all three brace and sway.
   Check no T/A-pose (if so: the native anim instance isn't evaluating; see risk 1).
2. `Looter.Landfall 8` at the farm: Delia on the porch, gun low, glancing. FarmPorch spawns: the gun comes up, she turns,
   flash + smoke + 9 warm tracers + `Story.Delia.Shotgun`, every ~1.8 s; after two, the break-open reload with
   `Story.Delia.Reload`; Dredgers drop to ~10% and she stops firing at them ("Keep them off the steps, Warden!"); stand in
   front of her: she holds fire; Unpaid bite her: flinch, bark, no damage. Barks never over Vane/crew captions.
3. Clear the porch: Talk prompt -> TURNIN's card -> her lines; she turns to face Sully, gun low, eyes on Sully.
4. `stat unit` beside the crew and at the porch fight; `Tools\perf.ps1` before/after (4 extra skinned people).

## Risks (couldn't check without a build)

1. The native anim instance has no root node: evaluation through `Evaluate_WithRoot` with a null root is assumed (as a
   graph-less native instance); if the bodies stand in the reference pose, give the proxy a custom root (a
   `FAnimNode_SequencePlayer` on MM_Idle) and keep `Apply` after it.
2. Bone-space signs: the mannequin faces +Y, right = -X in component space (as the player's mesh turn -90 says); lean
   forward = +about right, sway right = -about forward. `.Pose` checks reach/feet/look size, not the lean's direction: look.
3. `HandGrip_R`'s frame = gun +X barrel, +Z up (the player's third-person hold has no rotation offset): the stand-in
   shotgun could sit rotated in the hand; Forend from the receiver's `Foregrip`-`Grip` sockets (41.5, 0, 3).
4. Station targets are guesses until FLIGHT's props exist; tune `HandsShift` or WardenCrewStations.cpp.
5. Stand-in heights (Crane ~1.95 m, Pike ~1.78 m) guessed; tints are multipliers on textures (Crane's skin may still read grey).
6. `UBodyFigureComponent` on non-players: its holster/revolver logic finds no weapons (shown), hidden once by DressStandIn;
   Odessa's card prop needs `ULooterCharacterAnimInstance` so it stays hidden (wanted).
7. Creatures hunting Delia is new: any creature code assuming its target is a player (Dredger hose, fight director pressure
   reads, `FightState`) may act oddly with her; she has an `AAIController` (spawned for placed and spawned; none in test levels).
8. Perf: 4 people x (MM_Idle + layer + a retargeted hero) every frame within 30 m; the crew are all near the player on deck
   during the deck fight (the heaviest view there). Reduce `FullDetailRange` or skip the figure's swing bones if it shows.
9. `FPackageName::DoesPackageExist` runs in each anim instance's constructor (4 at load): negligible but noted.

## Tunables

Life: `FWardenLife` constants (WardenCrewLife.h). Crew: `CrewNoticeRadius` 450, talk turn rate 2.5/s, brace 11 cm / 6 deg,
watch chest share 0.3 (max 20), stations table. Delia: `DeliaFightRules` (all fight numbers), `DeliaNoticeRadius` 600, turn
rates 220/140/70 deg/s, chest lead 35, flash 1.4, `BarkGap` 7 s and per-moment chances (Hurt 0.6, Reload 0.35, Down 0.5,
Finish 0.6, HoseMan 0.9). Detail: `WardenCrewBody::FullDetailRange` 3000, `ReducedDetailRange` 8000, `DrawDistance` 15000.
