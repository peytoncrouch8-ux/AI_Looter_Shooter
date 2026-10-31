# Sink dens (2026-10-31): the Sink's spiders crawl out of dens in the walls

The user: "When arriving at the sink for the first time for the keepers lantern mission, I want the spiders to crawl out of
dens in the walls instead of being there automatically." Their picks: EVERY time the floor's spiders spawn (Main 5, a death,
a load), and they come out WHEN THE PLAYER REACHES THE FLOOR (the floor looks empty until the player steps onto it; then all six
crawl out round the walls, staggered a little so each reads).

Today: `Tools/Unreal/build_area_sink.py` `place_floor_spiders` places an `AEncounterSpawner` SinkFloor (6 spiders, area
ranks, tag Spider_SinkFloor, active during Main5 before step 3, `hunt_on_spawn` False, appearing as the player comes to the
rim; ground_corners = the sink zone, ground_max_rise FLOOR_RISE). The egg sacs' spiders (Spider_EggSac) are NOT changed.

## Who does what (disjoint files)

| Lane | Model | Files |
|---|---|---|
| A: the den entrance (C++) | Opus | `Creatures/AmbushSpawner.h/.cpp` (+ new `AmbushSpawnerDen.cpp` if it helps), `Creatures/SpiderCreature.h`, new `Creatures/SpiderCreatureDen.cpp`, `Creatures/SpiderAmbushRules.h/.cpp`, `Audio/LooterSoundCues.h` (the cue name only), new `Tests/SpiderDenTests.cpp`, `CODEMAP.md` lines for its files |
| B: the burrow model + sound | Opus | new `Art/Models/Props/SinkBurrows.py` (+ its textures if any, `Art/Textures/<Set>`), new `Art/Sounds/recipes/spider_den.py`; the import (`Tools\models.ps1`), the render (`Tools\sounds.ps1`) |
| Main | - | `Tools/Unreal/build_area_sink.py` (the spawner becomes an ambush, the DENS table, burrow placement), `recipes/__init__.py` GROUPS, running the build, the sound bank, docs, PIE checks, commit |

## Shared names and numbers

- `EAmbushEntrance::Den` appended after `RiseAround` (stored order; never reorder).
- `AAmbushSpawner::Dens` (`TArray<FSpiderPerchSpot>`, Kind Wall): each a den's mouth. `Spot` = the centre of the mouth's
  opening ON the wall plane (world cm); `FaceYaw` = the way the mouth faces out of the wall (deg, 0 = +X). Main's build
  writes them from the burrow it placed (its SOCKET_Mouth). The spawner's `SpawnPoints` stay floor spots (one under each
  den, on the floor at the wall's foot, 1.5 m out), so a spider that can't emerge appears there (PlayEntranceIfSeen).
- Spider i of a spawn goes to den (i mod Dens.Num()); its emergence waits `SpiderAmbushRules::DenDelay(i, Roll)`: i x
  `Looter.Spider.Den.Stagger` (0.45 s) plus up to 0.25 s jitter, so six come out over about 2.5 s.
- The spawner springs on walk-in to its ground: `AmbushCorners` = the sink zone, `AmbushMaxRise` = 250 cm over the
  spawner (it stands 50 cm over the floor's middle), so the rim and the ramp's upper part don't spring it; `bHuntOnSpawn`
  on (they come for the player once down). Existing walk-in rule (a respawned/loaded player walks out and back in) stays.
- `ASpiderCreature::BeginDenEmerge(const FSpiderPerchSpot& Den, float Delay, AAmbushSpawner* Owner)` -> bool (false:
  no wall found; the caller lets it appear on its floor spot). The spider waits hidden (no collision, not targetable, AI
  off) for Delay, then: set inside the throat (Den.Spot - Out x `Looter.Spider.Den.Inside` 110 cm x size), body level,
  facing out; crawls out (front legs reaching out and gripping the lip first, body squeezing out, ~1.1 s at size 1);
  turns down onto the wall face beside or under the mouth and clings head down (the Lurk's wall pose and surface traces,
  SpiderCreatureCling.cpp); climbs down the face to ~1 m over the floor (~150 cm/s, its gait's legs stepping on the
  face); drops the rest and lands (`Creature.Spider.Land`), then hunts the player. Shot while emerging or climbing: it
  takes the hit (hit reactions as on a perch), and falls from the wall if hit hard (FallFromPerch); killed: dies and falls.
  The whole emergence is cancelled cleanly if the spawner is despawned (the mission moves on) or the level unloads.
- Sounds: new `Creature.Spider.Den` (3D, att Creature, a few takes): claws scrabbling on rock inside a hollow, grit and
  pebbles trickling out of the mouth, a low hiss at the end; played at the mouth as the spider starts out. Then the
  existing `Creature.Spider.Climb` on the face, `Creature.Spider.Land` on landing, `Creature.Spider.Alert` once down.
  Cue name in `LooterSoundCues.h` (lane A); recipe `Art/Sounds/recipes/spider_den.py` group `spider_den` (lane B).
- FX: grit and dust spilling from the mouth as it comes out (lane A, existing code-drawn dust/dirt helpers, e.g. the
  Unpaid rise or Gravemother dirt; cheap, kept off the camera like StoneSentinelFX).
- The burrow mesh: `SM_SpiderBurrow_A` and `SM_SpiderBurrow_B` (two shapes), imported to `/Game/Art/Props/` (like the
  Sink's web props). Pivot at the opening's centre on the wall plane; +X points OUT of the wall (Unreal X); +Z up. Opening
  irregular, about 170 cm wide x 95 cm high; a rock lip standing 15-35 cm proud of the wall around it, broken and
  uneven, blending outward into a skirt that hides the seam with the terrain wall (the wall is not cut: everything behind
  the plane sits inside the terrain and is hidden); a throat tapering 220 cm deep into the wall, its inside fading to
  near black so it reads deep. Silk: strands across the upper rim, a torn funnel lining just inside, a silk mat
  spilling down the wall under the mouth. Socket `SOCKET_Mouth` at the pivot, +X out. Collision: none (spiders move by
  code; the player can't reach it). Materials only through instances of existing masters: the Sink's granite look
  (build_area_sink.py BLOCK_LOOK / the walls' granite), the Sink's web look (WEB_LOOKS), a dark throat instance. Must look
  right on the Nanite fallback without Lumen (Medium). "Stylized realism", textured, no faceted low-poly (the user hated
  it). Under ~6k triangles each, LODs 50/25.
- Placement (Main): 4 dens on the Sink's pit walls, mouths 1.6-3.0 m over the floor, apart from the ramp, Den Rock (the
  Gravemother's den), the egg sacs, the lantern's snare and the talus rows; spread round the floor so they surround the
  player. 6 spiders: dens 1 and 3 send two.

## Rules

- Read `CLAUDE.md`, `Docs/Pipeline/Enemies.md` (spider, Lurk, ambush entrances), the code named above, and `Art/README.md`
  (lane B) first. Code rules: C++ first, one class per file, under ~500 lines, comments say why, log to LogLooter.
- Lane A builds with `Tools\launch.ps1 -Build` (editor closed by it; another agent may render sounds or run Blender: fine).
  Tests: `Tools\runtests.ps1 -Filter Looter.Creatures.Spider.Den` (editor open). Don't run the area build or touch the
  level (Main does). Lane B: Blender beside the editor through `Tools\artrun.ps1`; import with `Tools\models.ps1`; never
  edit an imported mesh; render the sound with `Tools\sounds.ps1 Creature.Spider.Den`; don't run build_sound_bank.py.
- Nobody commits; Main commits. Done = a short report (what, files, numbers, checks, anything unsure).
