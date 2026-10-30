"""The mountain's fights (Docs/Areas/RansomsRest_Expansion.md, section 6 "Encounters"), for
build_area_missionprops_mountain.py: layout.json's gameplay.encounters.fights, the story's fights a script stands from data
(the valley's are build_area_story.py's and its neighbours'; the camps, patrols and ambushes between them are
build_area_camps.py's, whose tests ask every one to be on by Main 4, which nothing up the lift is). Each entry:
    {"id": "MineUpper", "spawner": "DeadMinerShiftSpawner" | "EncounterSpawner" | "AmbushSpawner" | "PatrolSpawner" |
                    "SetPieceSpawner" (an ambush with a "setPiece" block: build_area_setpieces.py),
     "frame": "level" (layout cm) | "mine" (the mine's metres: build_area_mine.py),
     "when": {"after": [...], "before": [...], "during": "", "fromStep": 0, "beforeStep": 0, "withFlags": [],
              "withoutFlags": []},
     "groups": [{"class": "DeadMinerCreature", "count": 4, "rank": "Basic" | "Rare" | "Epic" | "Legendary" | "Area",
                 "firstWave": 1, "lastWave": 0, "level": 0, "leader": false (its first creature leads its pack),
                 "leaderTraits": [...] (its leader's traits on top of Rallying; default MOUNTAIN_LEADER_TRAITS by rank),
                 "healthScale": 1.0, "name": "" (a named elite's DisplayName), "tags": [] (only its creatures carry them)}],
     "tags": [...], "legendaryId": "Foreman", "huntOnSpawn": false, "activation": 4500, "despawn": 9000,
     "center": [x, y] (or [x, y, z] in the mine), "spots": [[x, y], ...],
     "ground": {"polygon": [...] | "zone": id | "rooms": [...] (the mine's) | "radius": cm, "maxRise", "margin", "maxStep"},
     "waves": {"count": 2, "interval": 22, "waitForClear": false, "maxAlive": 0},
     "trigger": {"polygon" | "zone", "maxRise"}, "entrance": "Rise" (an ambush),
     "route": [[X, Y], ...], "corridor": cm, "pause": s, "loop": false (a patrol),
     "crate": [[X, Y, yaw, tucked], ...] (a Supply Crate, as a camp's), "conditionOf": "QuarryCrew",
     "pass": "valley" (a valley fight: Main Street's set piece, placed by build_area_camps.py in Gameplay; else the
              mountain's, placed here in Gameplay/Mountain)}
When each is on is the missions' data's first (Tools/Unreal/story_texts.py ENCOUNTERS by its id, or its conditionOf's),
else its own "when"; its own withoutFlags stay on top of either. Each stands as Encounter_<id> (SpawnerId <id>; the Foreman's vigil stone finds Encounter_Foreman), its creatures tagged
with its tags (a mission's KillNamed or an item's carrier finds them: AbeHaskett carries the clamp pin). In the mine
nothing is traced (a trace from above finds the Bench over it): heights come from the frame. Everything goes in the
area's Gameplay/Mountain folder; a class that isn't built is warned of and its fight left out.
"""
import importlib
import math

import unreal

import build_area_story as story

FOLDER = 'Gameplay/Mountain'
SPAWNERS = ('EncounterSpawner', 'AmbushSpawner', 'PatrolSpawner', 'DeadMinerShiftSpawner', 'SetPieceSpawner')
# The kinds sprung as an ambush is (a trigger's ground, an entrance); a set piece is one, with its own data on top.
AMBUSHES = ('AmbushSpawner', 'SetPieceSpawner')
RANKS = {'Basic': 'BASIC', 'Rare': 'RARE', 'Epic': 'EPIC', 'Legendary': 'LEGENDARY'}
ENTRANCES = {'Appear': 'APPEAR', 'Rise': 'RISE', 'Drop': 'DROP'}
# How far over and under an encounter's middle the ground of its spots and corners is looked for on the surface (cm).
LOOK = 1500.0
# What a mountain fight's Restless leader wears on top of Rallying (FEncounterGroup.LeaderTraits): Warded, so the typed wards
# come up up the lift (the RR tuning the user took, 2026-10-29 night). A group's own "leaderTraits" list wins; the valley's
# fights ("pass": "valley", Main Street) keep Rallying alone.
MOUNTAIN_LEADER_TRAITS = {'Rare': ['Warded']}


def when_spec(entry):
    """When a fight is on, as a story_texts condition dict: story_texts.py ENCOUNTERS' by its id (or its conditionOf: Abe
    Haskett's is the lift crew's), the missions' data's, else the entry's own; the entry's withoutFlags kept on top of
    either (the spoil heap's dead stop once the eleven are paid, as the mine's shift does)."""
    import build_area_missionprops_mountain as props
    own = entry.get('when', {})
    spec = dict(after=list(own.get('after', ())), before=list(own.get('before', ())), during=own.get('during', ''),
                from_step=int(own.get('fromStep', 0)), before_step=int(own.get('beforeStep', 0)),
                with_flags=list(own.get('withFlags', ())), without_flags=list(own.get('withoutFlags', ())))
    words = props.texts()
    theirs = getattr(words, 'ENCOUNTERS', {}).get(entry.get('conditionOf', entry['id'])) if words is not None else None
    if theirs is not None:
        kept = spec['without_flags']
        spec = dict(theirs)
        spec['without_flags'] = sorted(set(spec.get('without_flags', ())) | set(kept))
    return spec


def condition(spec):
    import build_area_missionprops_mountain as props
    return props.spec_when(spec)


def leader_traits(entry, each):
    """unreal.CreatureTrait values a leader group's leader wears on top of Rallying: the group's "leaderTraits" (names, as
    "Warded"), else MOUNTAIN_LEADER_TRAITS by its rank for a mountain fight; none for a group that doesn't lead or a valley
    fight's."""
    if not each.get('leader'):
        return []
    names = each.get('leaderTraits')
    if names is None:
        names = [] if entry.get('pass') == 'valley' else MOUNTAIN_LEADER_TRAITS.get(each['rank'], [])
    return [getattr(unreal.CreatureTrait, name.upper()) for name in names]


def groups(build, entry):
    """The entry's FEncounterGroups, or None (with a warning) when a class isn't built."""
    made = []
    for each in entry['groups']:
        cls = story.actor_class(each['class'])
        if cls is None:
            build.warn(f'no {each["class"]} class (build the game module first): no {entry["id"]}')
            return None
        group = unreal.EncounterGroup()
        group.set_editor_property('creature_class', cls)
        group.set_editor_property('count', int(each['count']))
        if each['rank'] == 'Area':
            group.set_editor_property('rank_roll', unreal.EncounterRankRoll.AREA)
        else:
            group.set_editor_property('rank_roll', unreal.EncounterRankRoll.FIXED)
            group.set_editor_property('rank', getattr(unreal.CreatureRank, RANKS[each['rank']]))
        group.set_editor_property('first_wave', int(each.get('firstWave', 1)))
        group.set_editor_property('last_wave', int(each.get('lastWave', 0)))
        if 'level' in each:
            group.set_editor_property('level', int(each['level']))
        # A leader group's first creature leads its pack (FIGHTS lane 6); a named elite wears its name and its own tags.
        if each.get('leader'):
            group.set_editor_property('leader', True)
        # A mountain fight's leader wears its rank's leader traits (MOUNTAIN_LEADER_TRAITS) or the group's own list.
        traits = leader_traits(entry, each)
        if traits:
            group.set_editor_property('leader_traits', traits)
        if 'healthScale' in each:
            group.set_editor_property('health_scale', float(each['healthScale']))
        if each.get('name'):
            group.set_editor_property('display_name', unreal.Text(each['name']))
        if each.get('tags'):
            group.set_editor_property('tags', [unreal.Name(tag) for tag in each['tags']])
        made.append(group)
    return made


class Where:
    """An entry's spots and grounds as world points: the level's layout cm on the terrain, or the mine's metres on its
    floors."""

    def __init__(self, build, entry, mine):
        import build_area_camps as camps
        self.build, self.entry, self.mine, self.camps = build, entry, mine, camps
        self.in_mine = entry.get('frame', 'level') == 'mine'
        if self.in_mine and mine is None:
            raise ValueError('no mine frame')
        center = entry['center'] if 'center' in entry else self._middle()
        if self.in_mine:
            x, y = center[0], center[1]
            z = center[2] if len(center) > 2 else 0.0
            self.x, self.y, self.z = mine.at(x, y, z)
        else:
            self.x, self.y = center[0], center[1]
            found = camps.ground_z(build, self.x, self.y)
            if found is None:
                build.warn(f'no terrain under {entry["id"]}\'s middle ({self.x:.0f}, {self.y:.0f}): it stands at 0')
            self.z = 0.0 if found is None else found

    def _middle(self):
        points = self.entry.get('spots') or self.entry.get('route')
        return (sum(p[0] for p in points) / len(points), sum(p[1] for p in points) / len(points))

    def point(self, p):
        """A spot (x, y) in the entry's frame as world (x, y)."""
        return self.mine.at(p[0], p[1], 0.0)[:2] if self.in_mine else (p[0], p[1])

    def relative(self, points):
        """Spots as the spawner's own (relative to it; the mine's on its floor, the surface's on their ground)."""
        if self.in_mine:
            return [unreal.Vector(x - self.x, y - self.y, 0.0) for x, y in (self.point(p) for p in points)]
        return self.camps.relative(self.build, [self.point(p) for p in points], self.x, self.y, self.z)

    def polygon(self, shape):
        """A ground's or trigger's polygon as world (x, y) corners, or None for a radius."""
        if 'rooms' in shape:
            import build_area_mine as mine_kit
            box = mine_kit.rooms_box(mine_kit.read_layout(), shape['rooms'])
            return self.mine.polygon(*box) if box else None
        if 'polygon' in shape:
            return [self.point(p) for p in shape['polygon']]
        return self.camps.polygon_of(self.build, shape)


def set_ground(where, made, ground):
    corners = where.polygon(ground)
    if corners:
        made.set_editor_property('ground_corners', [unreal.Vector(x, y, where.z) for x, y in corners])
    if 'radius' in ground:
        made.set_editor_property('give_up_radius', float(ground['radius']))
    for key, prop in (('margin', 'ground_margin'), ('maxRise', 'ground_max_rise'), ('maxStep', 'max_ground_step')):
        if key in ground:
            made.set_editor_property(prop, float(ground[key]))


def place_one(build, entry, mine):
    """One fight as its spawner. Returns (its label, the entry, where) for a camp's crate, or None."""
    kind = entry.get('spawner', 'EncounterSpawner')
    label = entry['id']
    if kind not in SPAWNERS:
        build.warn(f'{label}: no spawner kind {kind}: left out')
        return None
    cls = story.actor_class(kind)
    made_groups = groups(build, entry)
    if cls is None or made_groups is None:
        if cls is None:
            build.warn(f'no {kind} class (build the game module first): no {label}')
        return None
    try:
        where = Where(build, entry, mine)
    except ValueError:
        build.warn(f'{label} stands in the mine and there\'s no mine table: left out')
        return None
    folder = FOLDER if is_mountains(entry) else 'Gameplay'
    made = build.place(cls, (where.x, where.y, where.z + 50.0), 0.0, label=f'Encounter_{label}', folder=folder)
    made.set_editor_property('spawner_id', unreal.Name(label))
    made.set_editor_property('groups', made_groups)
    made.set_editor_property('creature_tags', [unreal.Name(t) for t in entry.get('tags', [label])])
    when = when_spec(entry)
    made.set_editor_property('active_when', condition(when))
    if entry.get('legendaryId'):
        made.set_editor_property('legendary_id', unreal.Name(entry['legendaryId']))
    for key, prop in (('activation', 'activation_radius'), ('despawn', 'despawn_radius'), ('spacing', 'spacing')):
        if key in entry:
            made.set_editor_property(prop, float(entry[key]))
    if 'huntOnSpawn' in entry:
        made.set_editor_property('hunt_on_spawn', bool(entry['huntOnSpawn']))
    if entry.get('spots'):
        made.set_editor_property('spawn_points', where.relative(entry['spots']))
    waves = entry.get('waves')
    if waves:
        made.set_editor_property('num_waves', int(waves.get('count', 1)))
        made.set_editor_property('wave_interval', float(waves.get('interval', 0.0)))
        made.set_editor_property('wait_for_clear', bool(waves.get('waitForClear', False)))
        made.set_editor_property('max_alive', int(waves.get('maxAlive', 0)))
    if kind == 'PatrolSpawner':
        route = [where.point(p) for p in entry['route']]
        made.set_editor_property('patrol_route', where.camps.relative(build, route, where.x, where.y, where.z))
        made.set_editor_property('loop', bool(entry.get('loop', False)))
        made.set_editor_property('pause_seconds', float(entry.get('pause', 3.0)))
        set_ground(where, made, {'polygon': where.camps.corridor(route, float(entry.get('corridor', 600.0))),
                                 'maxRise': entry.get('maxRise', 800.0)})
    else:
        set_ground(where, made, entry.get('ground', {}))
    if kind in AMBUSHES:
        trigger = where.polygon(entry.get('trigger', {}))
        if trigger:
            made.set_editor_property('ambush_corners', [unreal.Vector(x, y, where.z) for x, y in trigger])
        elif not entry.get('setPiece', {}).get('springRadius'):
            build.warn(f'{label}: no trigger polygon: it springs on approach')
        if 'maxRise' in entry.get('trigger', {}):
            made.set_editor_property('ambush_max_rise', float(entry['trigger']['maxRise']))
        made.set_editor_property('entrance', getattr(unreal.AmbushEntrance, ENTRANCES[entry.get('entrance', 'Appear')]))
    if kind == 'SetPieceSpawner':
        importlib.reload(importlib.import_module('build_area_setpieces')).apply(build, made, entry, where)
    if not where.in_mine and entry.get('spots'):
        step = float(entry.get('ground', {}).get('maxStep', where.camps.DEFAULT_STEP))
        where.camps.check_levels(build, f'Encounter_{label}', (where.x, where.y, where.z),
                                 [where.point(p) for p in entry['spots']], step)
    build.log(f'Encounter_{label} ({kind}{", Legendary " + entry["legendaryId"] if entry.get("legendaryId") else ""}): '
              f'{", ".join(str(g["count"]) + " " + g["rank"] + " " + g["class"] for g in entry["groups"])} at '
              f'({where.x:.0f}, {where.y:.0f}, {where.z:.0f}); {condition_words(when)}')
    return (label, entry, where) if entry.get('crate') else None


def is_mountains(entry):
    """Whether a fight is the mountain pass's (placed here); a valley one ("pass": "valley") is build_area_camps.py's."""
    return entry.get('pass', 'mountain') == 'mountain'


def condition_words(when):
    parts = []
    if when.get('during'):
        parts.append(f'during {when["during"]} from step {when.get("from_step", 0)}'
                     + (f' before {when["before_step"]}' if when.get('before_step') else ''))
    if when.get('after'):
        parts.append('after ' + ', '.join(when['after']))
    if when.get('without_flags'):
        parts.append('without ' + ', '.join(when['without_flags']))
    if when.get('with_flags'):
        parts.append('with ' + ', '.join(when['with_flags']))
    return '; '.join(parts) or 'always'


def place_crates(build, camps):
    """A camp's Supply Crate on the first of its spots that stands level and clear (build_area_caches' judging)."""
    caches = importlib.reload(importlib.import_module('build_area_caches'))
    cls = story.actor_class('Chest')
    if cls is None or not camps:
        return
    meshes = caches.models(build, 'SupplyCrate')
    if meshes is None:
        return
    site = caches.Site(build)
    for label, entry, where in camps:
        crate_id = f'CampCache_{label}'
        chosen = caches.choose(site, [(x, y, yaw, bool(tucked)) for x, y, yaw, tucked in entry['crate']])
        if chosen is None:
            build.warn(f'no terrain under any of {crate_id}\'s spots: no crate')
            continue
        i, x, y, z, yaw, spread, problems = chosen
        caches.place_chest(build, cls, 'SupplyCrate', meshes, crate_id, (x, y, z), yaw, ('CampCache',))
        if problems:
            build.warn(f'{crate_id} at ({x:.0f}, {y:.0f}): {"; ".join(problems)} (retune its crate spots)')
        else:
            build.log(f'{crate_id} at ({x:.0f}, {y:.0f}, {z:.0f}), level within {spread:.0f} cm')


def place(build, mine):
    """Every entry of gameplay.encounters.fights, then the camps' crates."""
    importlib.reload(importlib.import_module('build_area_camps'))
    fights = [entry for entry in build.settings.get('encounters', {}).get('fights', []) if is_mountains(entry)]
    if not fights:
        build.warn('layout.json has no gameplay.encounters.fights (apply Art/Levels/RansomsRest/patches/mountain.json): '
                   'the mountain has no fights')
        return
    import build_area_camps
    build_area_camps._TILES.clear()
    camps = [made for made in (place_one(build, entry, mine) for entry in fights) if made]
    place_crates(build, camps)
    build.log(f'the mountain\'s fights: {len(fights)} ({", ".join(e["id"] for e in fights)})')
