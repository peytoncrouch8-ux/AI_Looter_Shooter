"""A set piece's own data on its spawner (ASetPieceSpawner; FIGHT-LAYOUT, 2026-10-21, the user's few hand-made fights an
area, announced and rewarded, once a session), for build_area_camps.py (an ambush with a "setPiece" block: the Webwood) and
build_area_fights_mountain.py (a fight of spawner SetPieceSpawner: Main Street, the Providence's cave-in). The layout's
block, in the entry's frame (layout cm, or the mine's metres):
    "setPiece": {
      "doneFlag": "SetPiece.<Id>",             the story flag it sets when won: it's off for the rest of the session
      "waves": 2, "pushWhenLeft": 3,           its pushes; the next comes once this many of the last are left
      "pushAfter": 30,                         ...or this long (s) after the last at the latest
      "aroundPlayer": false, "spread": 1300,   its pushes round where the player stood as it sprang, this far out (cm)
      "springRadius": 0,                       above 0: it springs on the player coming this near (cm), not walking in
      "waitFor": [ids], "hush": [actor tags],  spawners that hold it off while they fight; speakers shut while it's on
      "beats": [{"wave": 1, "spots": [[x, y], ...], "speaker": "Hob", "line": "...", "sting": cue, "cue": cue,
                 "shake": 0.3, "shakeSeconds": 0.8, "dust": 0}],
      "cleared": {beat},                       said and heard as it's won
      "placed": [{"class": ..., "rank": "Basic", "wave": 1, "at": [x, y],
                  "high": cm (the highest ground within this of "at", inside its ground) |
                  "balcony": kind (the deck of that placement's balcony over "at"'s street), "z": m (the mine's)}],
      "reward": [x, y],                        where its loot shower bursts (a round-the-player one bursts where it sprang)
      "arena": {"gates": [[[x, y], [x, y]], ...], "margin": 150}
                                               an arena (FIGHTS lane 6, the chapel yard): a fog wall (ABossSeal, Gate shape) across
                                               each opening, each given by its two ends (the entry's frame, cm), the arena on the
                                               spawner's side of it; they rise once the player stands inside every one by "margin"
                                               cm with the fight on, and drop when it is won, on the player's death or if it is off
    }
Its creatures' groups, trigger, entrance and ground are the entry's own, as any ambush's.
"""
import math

import unreal

import build_area_story as story

RANKS = {'Basic': 'BASIC', 'Rare': 'RARE', 'Epic': 'EPIC'}
# A balcony's deck stands this far over the street (cm): the Gilt Spur's is a storey up (FalseFronts.py: 3 m and its floor).
DECK_RISE = (280.0, 440.0)
# Its deck is looked for from the facade (model +X, 6 m out from the pivot) out over the boardwalk, every so far (cm).
FACADE_OUT = 600.0
DECK_REACH = 320.0
DECK_STEP = 20.0
# The traces start this high over the street: over the balcony's rail, under the facade's cornice (cm).
DECK_LOOK_FROM = 520.0


def inside(polygon, x, y):
    """Whether (x, y) is inside the polygon ((x, y) pairs), seen from above."""
    found = False
    for (x0, y0), (x1, y1) in zip(polygon, list(polygon[1:]) + [polygon[0]]):
        if (y0 > y) != (y1 > y) and x < (x1 - x0) * (y - y0) / (y1 - y0) + x0:
            found = not found
    return found


def beat(spec, where):
    """An FSetPieceBeat from the layout's beat: its push, its spots (relative to the spawner), its words, cues and shake."""
    made = unreal.SetPieceBeat()
    made.set_editor_property('wave', int(spec.get('wave', 1)))
    if spec.get('spots'):
        made.set_editor_property('spots', where.relative(spec['spots']))
    made.set_editor_property('speaker', unreal.Text(spec.get('speaker', '')))
    made.set_editor_property('line', unreal.Text(spec.get('line', '')))
    made.set_editor_property('sting_cue', unreal.Name(spec.get('sting', '')))
    made.set_editor_property('world_cue', unreal.Name(spec.get('cue', '')))
    made.set_editor_property('shake', float(spec.get('shake', 0.0)))
    made.set_editor_property('shake_seconds', float(spec.get('shakeSeconds', 0.8)))
    made.set_editor_property('dust_puffs', int(spec.get('dust', 0)))
    return made


def gate_one(build, ends, where, entry):
    """An FSetPieceGate across one of an arena's openings: its two ends (the entry's frame, cm), each on the ground there."""
    import build_area_camps as camps
    points = []
    for end in ends:
        x, y = where.point(end)
        z = camps.ground_z(build, x, y, where.z)
        if z is None:
            build.warn(f'{entry["id"]}: no ground under an arena gate\'s end ({x:.0f}, {y:.0f}): it stands at the set piece\'s height')
            z = where.z
        points.append(unreal.Vector(x, y, z))
    made = unreal.SetPieceGate()
    made.set_editor_property('start', points[0])
    made.set_editor_property('end', points[1])
    return made


def deck_spot(build, kind, x, y):
    """The deck of the balcony on the placement of this kind, over the street near (x, y): traced down from over its rail
    along the facade's middle, the first hit DECK_RISE over the street. None (with a warning) when there's none."""
    import build_area_camps as camps
    host = story.placed(build, kind)
    if host is None:
        return None
    at = host.get_actor_location()
    yaw = math.radians(host.get_actor_rotation().yaw)
    fx, fy = math.cos(yaw), math.sin(yaw)
    world = unreal.EditorLevelLibrary.get_editor_world()
    # The street's own height there (the terrain alone: a trace from above would find the balcony itself).
    street = camps.ground_z(build, x, y)
    if street is None:
        street = at.z
    found = []
    out = FACADE_OUT
    while out <= FACADE_OUT + DECK_REACH:
        px, py = at.x + fx * out, at.y + fy * out
        start = unreal.Vector(px, py, street + DECK_LOOK_FROM)
        hit = unreal.SystemLibrary.line_trace_single(world, start, unreal.Vector(px, py, street - 100.0),
                                                     unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [],
                                                     unreal.DrawDebugTrace.NONE, True)
        if hit is not None:
            z = hit.to_tuple()[5].z
            if DECK_RISE[0] <= z - street <= DECK_RISE[1]:
                found.append((px, py, z))
        out += DECK_STEP
    if not found:
        build.warn(f'no balcony deck found on {kind} (traced {FACADE_OUT:.0f}-{FACADE_OUT + DECK_REACH:.0f} cm out of its pivot): '
                   f'its set piece\'s placed creature stands in the street under it')
        return None
    # The middle of the run of deck hits: clear of the rail at its edge and the wall behind.
    return found[len(found) // 2]


def high_spot(build, where, entry, x, y, reach):
    """The highest terrain within reach (cm) of (x, y), on the entry's own ground: a Spitter's perch."""
    import build_area_camps as camps
    ground = where.polygon(entry.get('ground', {})) or None
    best = None
    steps = 3
    for i in range(-steps, steps + 1):
        for j in range(-steps, steps + 1):
            px, py = x + reach * i / steps, y + reach * j / steps
            if ground and not inside(ground, px, py):
                continue
            z = camps.ground_z(build, px, py, where.z)
            if z is not None and (best is None or z > best[2]):
                best = (px, py, z)
    return best


def placed_one(build, spec, where, entry):
    """An FSetPiecePlaced from the layout's: its class and rank, its push, its spot in the world, facing the fight's middle."""
    import build_area_camps as camps
    cls = story.actor_class(spec['class'])
    if cls is None:
        build.warn(f'no {spec["class"]} class (build the game module first): {entry["id"]} has no placed {spec["class"]}')
        return None
    x, y = where.point(spec['at'])
    spot = None
    if where.in_mine:
        spot = where.mine.at(spec['at'][0], spec['at'][1], float(spec.get('z', 0.0)))
    elif 'balcony' in spec:
        spot = deck_spot(build, spec['balcony'], x, y)
    elif 'high' in spec:
        spot = high_spot(build, where, entry, x, y, float(spec['high']))
    if spot is None:
        z = camps.ground_z(build, x, y, where.z)
        if z is None:
            build.warn(f'{entry["id"]}: no ground under its placed {spec["class"]} at ({x:.0f}, {y:.0f}): left out')
            return None
        spot = (x, y, z)
    made = unreal.SetPiecePlaced()
    made.set_editor_property('creature_class', cls)
    made.set_editor_property('rank', getattr(unreal.CreatureRank, RANKS[spec.get('rank', 'Basic')]))
    made.set_editor_property('wave', int(spec.get('wave', 1)))
    made.set_editor_property('location', unreal.Vector(spot[0], spot[1], spot[2] + 5.0))
    made.set_editor_property('yaw', math.degrees(math.atan2(where.y - spot[1], where.x - spot[0])))
    build.log(f'{entry["id"]}: a placed {spec.get("rank", "Basic")} {spec["class"]} at ({spot[0]:.0f}, {spot[1]:.0f}, {spot[2]:.0f}), '
              f'push {spec.get("wave", 1)}')
    return made


def apply(build, made, entry, where):
    """The entry's setPiece block onto its spawner (made), its spots in where's frame (build_area_fights_mountain.Where)."""
    spec = entry.get('setPiece')
    if not spec:
        return
    made.set_editor_property('done_flag', unreal.Name(spec.get('doneFlag', f'SetPiece.{entry["id"]}')))
    made.set_editor_property('num_waves', int(spec.get('waves', 2)))
    made.set_editor_property('wave_interval', float(spec.get('pushAfter', 30.0)))
    made.set_editor_property('wait_for_clear', False)
    made.set_editor_property('push_when_left', int(spec.get('pushWhenLeft', 3)))
    made.set_editor_property('around_player', bool(spec.get('aroundPlayer', False)))
    if 'spread' in spec:
        made.set_editor_property('spawn_radius', float(spec['spread']))
    if 'springRadius' in spec:
        made.set_editor_property('spring_radius', float(spec['springRadius']))
    made.set_editor_property('wait_for_spawners', [unreal.Name(n) for n in spec.get('waitFor', [])])
    made.set_editor_property('hush_speaker_tags', [unreal.Name(n) for n in spec.get('hush', [])])
    made.set_editor_property('beats', [beat(b, where) for b in spec.get('beats', [])])
    if 'cleared' in spec:
        made.set_editor_property('cleared_beat', beat(spec['cleared'], where))
    placed = [p for p in (placed_one(build, p, where, entry) for p in spec.get('placed', [])) if p is not None]
    made.set_editor_property('placed', placed)
    if 'reward' in spec:
        rx, ry = where.point(spec['reward'])
        made.set_editor_property('reward_spot', unreal.Vector(rx - where.x, ry - where.y, 0.0))
    arena = spec.get('arena')
    if arena:
        made.set_editor_property('arena_gates', [gate_one(build, ends, where, entry) for ends in arena['gates']])
        made.set_editor_property('arena_margin', float(arena.get('margin', 150.0)))
        build.log(f'{entry["id"]}: an arena, {len(arena["gates"])} fog gate(s), up once the player is {arena.get("margin", 150.0):.0f} cm inside')
    pushes = ', '.join(f'{b.get("wave", 1)}' + (f' ({len(b["spots"])} spots)' if b.get('spots') else ' (round the player)')
                       for b in spec.get('beats', []))
    build.log(f'{entry["id"]}: a set piece (flag {spec.get("doneFlag")}), pushes {pushes}, {len(placed)} placed, '
              f'{"springs at " + str(spec["springRadius"]) + " cm" if spec.get("springRadius") else "springs on its ground"}'
              f'{", waits for " + ", ".join(spec["waitFor"]) if spec.get("waitFor") else ""}'
              f'{", shuts " + ", ".join(spec["hush"]) if spec.get("hush") else ""}')
