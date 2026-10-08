"""Paints the tutorial island, and only the tutorial island, in Painted Frontier (Style Lab style 3), and takes it off
again exactly. Run in the open editor (it opens Lvl_TutorialIsland itself, and stops while anything is playing):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_island_painted.py apply"
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_island_painted.py undo"
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_island_painted.py status"
Run Tools/Unreal/build_world_materials.py first: the masters' Painted switch and M_PaintedPost come from it.

apply:
- Every static mesh slot in the level wearing an instance of M_World, M_WorldFoliage, M_Terrain, M_SkyClouds,
  M_Water, M_Smoke or M_Waterfall (the placed actors, the dressing, the sky dome and the PCG scatter's instances alike)
  wears an island-only child of it instead: /Game/Art/Materials/Painted/MI_<name>_Painted, with Painted on and the
  lab's values for its role (painted_look.py: by its texture set, else words in its name). The shared instances are
  never touched, so Ransom's Rest, Skyreach and every other level keep their look; the main menu, which is this island
  opened with ?game=Menu, shows it painted.
- The island's light, through what its lighting states never write (World/LightingTargets.cpp): the sun's and the sky
  light's colour filters (warm sun, blue-violet fill) and the sky light's lower hemisphere. The states' values (the
  sun's turn, intensity and temperature, the sky light's intensity, the sky's colour, the fog's colours, the exposure)
  stay the island's Day, which Looter.World.Lighting.Levels requires to equal the default state. painted.json's
  "state" block, off by default, writes the lab's own sun and sky into the lights and Day as well; that test fails
  until it learns the island may have a painted Day (C++).
- A PaintedPost volume (unbound, above IslandPost) with the grade, the exposure, the bloom and M_PaintedPost's screen
  pass (MI_PaintedPost_Island): the lighting states keep writing IslandPost's exposure, which this one outranks.
Everything it changes is recorded in Art/Levels/TutorialIsland/painted_applied.json (the original material of every
slot, every property's old value), and the level is saved. Running it again paints what isn't painted yet (the
scatter after it generates again, actors a rebuild placed) and updates every painted instance to painted.json and
painted_look.py; a rebuild of the island (build_area.py) places its actors unpainted, so run apply after one.

undo: every recorded slot back on its original material (a painted slot nobody recorded on its painted instance's
parent), every recorded property back, the PaintedPost volume removed, the level saved and the record deleted. The
painted instances stay as assets, unused: to drop the style for good, delete Content/Art/Materials/Painted with git.

status: what's painted, what isn't, and whether the record still matches the level.

Settings, all tunable without code: Art/Levels/TutorialIsland/painted.json (its "about" says what each block does).
"""
import importlib
import json
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import painted_look  # noqa: E402
painted_look = importlib.reload(painted_look)

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
ISLAND = '/Game/Maps/Lvl_TutorialIsland'
MASTERS = '/Game/Art/Materials/Masters'
FOLDER = '/Game/Art/Materials/Painted'
LEVEL_DATA = os.path.join(PROJECT, 'Art', 'Levels', 'TutorialIsland')
SETTINGS = os.path.join(LEVEL_DATA, 'painted.json')
RECORD = os.path.join(LEVEL_DATA, 'painted_applied.json')
BUILD_TAG = 'IslandBuild'
TAG = 'PaintedFrontier'
POST_LABEL = 'PaintedPost'
POST_FOLDER = 'Island/Painted'
POST_INSTANCE = 'MI_PaintedPost_Island'
POST_PRIORITY = 10.0
# The masters with the Painted switch (build_world_materials.py); the rest only take values.
SWITCHED = ('M_World', 'M_WorldFoliage', 'M_Terrain', 'M_SkyClouds')
# The island's environment actors (build_area_environment.py): their labels and classes.
TARGETS = {'sun': ('Sun', 'DirectionalLight', 'DirectionalLightComponent'),
           'skyLight': ('SkyLight', 'SkyLight', 'SkyLightComponent'),
           'atmosphere': ('SkyAtmosphere', 'SkyAtmosphere', 'SkyAtmosphereComponent'),
           'fog': ('HeightFog', 'ExponentialHeightFog', 'ExponentialHeightFogComponent')}
# What a lighting state writes (FLightingTargets::Write) and Looter.World.Lighting.Levels compares with the island's
# Day: changed only by the "state" block, which changes Day too.
STATE_PROPERTIES = {
    'sun': {'intensity', 'temperature', 'use_temperature', 'dynamic_shadow_distance_movable_light',
            'dynamic_shadow_cascades'},
    'skyLight': {'intensity'},
    'atmosphere': {'sky_luminance_factor', 'other_absorption_scale'},
    'fog': {'fog_density', 'fog_inscattering_luminance', 'fog_inscattering_color', 'directional_inscattering_luminance',
            'directional_inscattering_color', 'directional_inscattering_exponent',
            'directional_inscattering_start_distance', 'sky_atmosphere_ambient_contribution_color_scale'},
}

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
mel = unreal.MaterialEditingLibrary


def log(message):
    unreal.log(f'Painted island: {message}')


def warn(message):
    unreal.log_warning(f'Painted island: {message}')


# --- Files ---

def load_settings():
    with open(SETTINGS) as f:
        return json.load(f)


def load_record():
    if not os.path.exists(RECORD):
        return None
    with open(RECORD) as f:
        return json.load(f)


def save_record(record):
    with open(RECORD, 'w') as f:
        json.dump(record, f, indent=1, sort_keys=True)
        f.write('\n')
    log(f'what it changed is recorded in {RECORD}')


def new_record():
    return {'about': 'Written by Tools/Unreal/build_island_painted.py apply: what painting the tutorial island changed, '
                     'so undo puts it back exactly. Slots: actor path|component -> its original override list and the '
                     'slots painted (slot -> the material it wore). Properties: actor path|component|property -> the '
                     'value before. Day: the lighting state Day\'s fields before (the "state" block only).',
            'level': ISLAND, 'slots': {}, 'properties': {}, 'day': {}, 'post': False}


# --- The level ---

def open_island():
    """The island open in the editor, nothing playing, no other map left unsaved."""
    if levels.is_in_play_in_editor():
        raise RuntimeError('a play session is running: stop it first (hands off while the user plays)')
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world.get_path_name().split('.')[0] == ISLAND:
        return
    unsaved = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
               if not p.get_name().startswith('/Temp/')]
    if unsaved:
        raise RuntimeError(f'unsaved changes in {unsaved}: save or discard them first')
    unreal.EditorLoadingAndSavingUtils.load_map(ISLAND)
    if hasattr(unreal, 'LooterLevelTools'):
        unreal.LooterLevelTools.finish_asset_compilation()
    log(f'opened {ISLAND}')


def save_level():
    if not levels.save_current_level():
        raise RuntimeError('the level did not save')
    log('level saved')


def level_actors():
    return list(actors.get_all_level_actors())


def tagged(actor, tag):
    return unreal.Name(tag) in actor.tags


def component_key(actor, component):
    return f'{actor.get_path_name()}|{component.get_name()}'


def find_components(keys):
    """Each 'actor path|component' key's component (None for one gone: a rebuild replaced its actor)."""
    by_path = {a.get_path_name(): a for a in level_actors()}
    found = {}
    for key in keys:
        actor_path, name = key.split('|')[:2]
        actor = by_path.get(actor_path)
        component = None
        if actor is not None:
            component = next((c for c in actor.get_components_by_class(unreal.ActorComponent) if c.get_name() == name),
                             None)
        found[key] = (actor, component)
    return found


def mesh_components(actor):
    """The actor's static mesh components (instanced and spline ones too) whose materials a level can keep: not the
    ones its construction script makes again each time it runs."""
    out = []
    for component in actor.get_components_by_class(unreal.StaticMeshComponent):
        try:
            if component.get_editor_property('creation_method') == unreal.ComponentCreationMethod.USER_CONSTRUCTION_SCRIPT:
                continue
        except Exception:
            pass
        out.append(component)
    return out


def overrides(component):
    return [m.get_path_name() if m is not None else None for m in component.get_editor_property('override_materials')]


# --- Materials ---

def master_of(material):
    """The master (by name) a material is an instance of, when it's one this paints; else None."""
    base = material.get_base_material() if material is not None else None
    if base is None or not base.get_path_name().startswith(MASTERS + '/'):
        return None
    name = base.get_name()
    return name if name in painted_look.MASTERS else None


def is_painted(material):
    return material is not None and material.get_path_name().startswith(FOLDER + '/')


def texture_set(material):
    """The texture set (T_<Set>_BC) a surface's BaseColorMap reads, or None."""
    try:
        if isinstance(material, unreal.MaterialInstanceConstant):
            texture = mel.get_material_instance_texture_parameter_value(material, 'BaseColorMap')
        else:
            texture = mel.get_material_default_texture_parameter_value(material, 'BaseColorMap')
    except Exception:
        texture = None
    name = texture.get_name() if texture is not None else ''
    return name[2:-3] if name.startswith('T_') and name.endswith('_BC') else None


def vector_value(material, name):
    if isinstance(material, unreal.MaterialInstanceConstant):
        value = mel.get_material_instance_vector_parameter_value(material, name)
    else:
        value = mel.get_material_default_vector_parameter_value(material, name)
    return (value.r, value.g, value.b, value.a)


def scalar_value(material, name):
    if isinstance(material, unreal.MaterialInstanceConstant):
        return mel.get_material_instance_scalar_parameter_value(material, name)
    return mel.get_material_default_scalar_parameter_value(material, name)


def paints(material):
    """Whether apply paints a slot wearing this (an instance of a painted master, and not a role the lab leaves)."""
    master = master_of(material)
    if master is None or is_painted(material):
        return False
    if painted_look.MASTERS[master] != 'surface':
        return True
    return painted_look.role_of(material.get_name(), texture_set(material), master) not in painted_look.UNPAINTED_ROLES


def with_changes(params, changes):
    """params ({'scalars', 'vectors'}) with painted.json's changes over them (same shape)."""
    out = {'scalars': dict(params.get('scalars', {})), 'vectors': dict(params.get('vectors', {}))}
    for kind in ('scalars', 'vectors'):
        for name, value in (changes or {}).get(kind, {}).items():
            out[kind][name] = value
    return out


def painted_values(worn, master, settings, bias):
    """(role, params) for the painted instance of worn: the lab's values for its role with painted.json's changes, or
    None for a surface the lab doesn't paint (glow, glass, creatures)."""
    kind = painted_look.MASTERS[master]
    name = worn.get_name()
    if kind == 'surface':
        set_name = texture_set(worn)
        role = painted_look.role_of(name, set_name, master)
        options = painted_look.lab_surface(role, set_name, painted_look.is_alpha_masked(set_name, master))
        if options is None:
            return role, None
        options.update(settings.get('roles', {}).get(role, {}))
        params = painted_look.surface_params(options)
    elif kind == 'terrain':
        role, params = 'ground', with_changes(painted_look.ground_params(), settings.get('terrain'))
    elif kind == 'sky':
        role, params = 'sky', painted_look.sky_params(bias, settings.get('sky'))
    elif kind == 'water':
        water = settings.get('water', {})
        color = [c * water.get('scale', 1.0) for c in painted_look.rgb(water.get('color', '#2f9fae'))]
        role, params = 'water', {'scalars': {'Roughness': water.get('roughness', 0.1)},
                                 'vectors': {'WaterColor': painted_look.vec4(color)}}
    else:
        # Smoke and the waterfall paler and brighter, as the lab grades them (saturation 0.6, value 1.2).
        names = ('SmokeColor',) if kind == 'smoke' else ('WaterColor', 'FoamColor')
        role = kind
        params = {'scalars': {}, 'vectors': {n: painted_look.vec4(painted_look.adjusted(vector_value(worn, n), 0.6, 1.2))
                                             for n in names}}
    return role, with_changes(params, settings.get('materials', {}).get(name))


def instance_name(worn):
    name = worn.get_name()
    return f'MI_{name[3:] if name.startswith("MI_") else name}_Painted'


def painted_instance(worn, master, values, switch=True):
    """The island's own child of worn (made, or brought up to date and saved only when it changes)."""
    name = instance_name(worn)
    path = f'{FOLDER}/{name}'
    created = not unreal.EditorAssetLibrary.does_asset_exist(path)
    if created:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    else:
        instance = unreal.load_asset(path)
    changed = created or instance.get_editor_property('parent') != worn
    if changed:
        mel.set_material_instance_parent(instance, worn)
    if switch and not mel.get_material_instance_static_switch_parameter_value(instance, 'Painted'):
        mel.set_material_instance_static_switch_parameter_value(instance, 'Painted', True)
        changed = True
    changed |= set_values(instance, values)
    if changed:
        mel.update_material_instance(instance)
        unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
        log(f'{path} ({master}) {"made" if created else "updated"}')
    return instance


def set_values(instance, values):
    changed = False
    for key, value in values.get('scalars', {}).items():
        if abs(mel.get_material_instance_scalar_parameter_value(instance, key) - float(value)) > 1e-5:
            mel.set_material_instance_scalar_parameter_value(instance, key, float(value))
            changed = True
    for key, value in values.get('vectors', {}).items():
        color = unreal.LinearColor(*painted_look.vec4(value))
        old = mel.get_material_instance_vector_parameter_value(instance, key)
        if any(abs(a - b) > 1e-5 for a, b in zip(old.to_tuple(), color.to_tuple())):
            mel.set_material_instance_vector_parameter_value(instance, key, color)
            changed = True
    return changed


def check_masters():
    """Stops unless build_world_materials.py has given the masters their Painted switch and made M_PaintedPost."""
    for name in SWITCHED:
        master = unreal.load_asset(f'{MASTERS}/{name}')
        switches = [str(n) for n in mel.get_static_switch_parameter_names(master)] if master else []
        if 'Painted' not in switches:
            raise RuntimeError(f'{MASTERS}/{name} has no Painted switch: run Tools/Unreal/build_world_materials.py first')
    if not unreal.EditorAssetLibrary.does_asset_exist(f'{MASTERS}/M_PaintedPost'):
        raise RuntimeError(f'no {MASTERS}/M_PaintedPost: run Tools/Unreal/build_world_materials.py first')


def paint_slots(record, settings, bias):
    """Every paintable slot in the level onto its painted instance, recorded."""
    made, skipped, roles = {}, {}, {}
    painted = already = 0
    for actor in level_actors():
        if tagged(actor, TAG):
            continue
        for component in mesh_components(actor):
            key = component_key(actor, component)
            for slot in range(component.get_num_materials()):
                worn = component.get_material(slot)
                if worn is None:
                    continue
                if is_painted(worn):
                    already += 1
                    continue
                master = master_of(worn)
                if master is None:
                    continue
                path = worn.get_path_name()
                if path not in made:
                    role, values = painted_values(worn, master, settings, bias)
                    made[path] = painted_instance(worn, master, values, master in SWITCHED) if values else None
                    if values:
                        roles[instance_name(worn)] = role
                    else:
                        skipped[worn.get_name()] = role
                instance = made[path]
                if instance is None:
                    continue
                entry = record['slots'].get(key)
                if entry is None:
                    entry = record['slots'][key] = {'actor': str(actor.get_actor_label()),
                                                    'overrides': overrides(component), 'painted': {}}
                entry['painted'][str(slot)] = path
                component.modify()
                component.set_material(slot, instance)
                painted += 1
    for name, role in sorted(skipped.items()):
        log(f'{name} left as it is (role {role}: the lab doesn\'t paint it)')
    log(f'{painted} slots painted now, {already} were already; {len([m for m in made.values() if m])} painted '
        f'instances: ' + ', '.join(f'{n} ({r})' for n, r in sorted(roles.items())))
    return painted


def update_painted_instances(settings, bias):
    """Every painted instance on the island brought up to painted.json and painted_look.py, even ones whose slots were
    painted by an earlier run (paint_slots() only sees the unpainted)."""
    seen = set()
    for actor in level_actors():
        for component in mesh_components(actor):
            for slot in range(component.get_num_materials()):
                worn = component.get_material(slot)
                if not is_painted(worn) or worn.get_path_name() in seen:
                    continue
                seen.add(worn.get_path_name())
                parent = worn.get_editor_property('parent')
                master = master_of(parent)
                if parent is None or master is None:
                    continue
                role, values = painted_values(parent, master, settings, bias)
                if values:
                    painted_instance(parent, master, values, master in SWITCHED)


# --- Light, sky and post ---

def encode(value):
    if isinstance(value, unreal.Color):
        return {'Color': [value.r, value.g, value.b, value.a]}
    if isinstance(value, unreal.LinearColor):
        return {'LinearColor': [value.r, value.g, value.b, value.a]}
    if isinstance(value, unreal.Rotator):
        return {'Rotator': [value.pitch, value.yaw, value.roll]}
    if isinstance(value, (bool, int, float)):
        return {'value': value}
    raise TypeError(f'cannot record a {type(value).__name__}')


def decode(stored):
    if 'Color' in stored:
        r, g, b, a = stored['Color']
        return unreal.Color(r=r, g=g, b=b, a=a)
    if 'LinearColor' in stored:
        return unreal.LinearColor(*stored['LinearColor'])
    if 'Rotator' in stored:
        pitch, yaw, roll = stored['Rotator']
        return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)
    return stored['value']


def srgb8(linear):
    v = max(0.0, min(1.0, float(linear)))
    v = v * 12.92 if v <= 0.0031308 else 1.055 * v ** (1.0 / 2.4) - 0.055
    return int(round(v * 255.0))


def as_property(current, value):
    """A painted.json value in the type the property has now (a light's colour filter is an sRGB Color)."""
    if isinstance(current, unreal.Color):
        r, g, b = value[:3]
        return unreal.Color(r=srgb8(r), g=srgb8(g), b=srgb8(b), a=255)
    if isinstance(current, unreal.LinearColor):
        return unreal.LinearColor(*painted_look.vec4(value))
    if isinstance(current, bool):
        return bool(value)
    if isinstance(current, int):
        return int(value)
    return float(value)


def set_recorded(record, actor, component, prop, value):
    """Sets a property, keeping its first value in the record (a second apply keeps the original)."""
    key = f'{component_key(actor, component)}|{prop}'
    current = component.get_editor_property(prop)
    if key not in record['properties']:
        record['properties'][key] = encode(current)
    component.modify()
    component.set_editor_property(prop, as_property(current, value))


def environment_target(name):
    """(actor, component) of one of the island's environment actors: by its build label, else the first of its
    class."""
    label, actor_class, component_class = TARGETS[name]
    cls = getattr(unreal, actor_class)
    candidates = [a for a in level_actors() if isinstance(a, cls)]
    actor = next((a for a in candidates if str(a.get_actor_label()) == label and tagged(a, BUILD_TAG)), None) \
        or next(iter(candidates), None)
    if actor is None:
        return None, None
    return actor, actor.get_component_by_class(getattr(unreal, component_class))


def paint_lights(record, settings):
    """The lights' colour filters and other properties a lighting state never writes (painted.json "lights")."""
    state_on = settings.get('state', {}).get('enabled', False)
    for target, props in settings.get('lights', {}).items():
        if target == 'about':
            continue
        actor, component = environment_target(target)
        if component is None:
            warn(f'no {target} in the level: its painted settings are skipped')
            continue
        for prop, value in props.items():
            if prop in STATE_PROPERTIES.get(target, ()) and not state_on:
                warn(f'{target}.{prop} is what a lighting state writes (Looter.World.Lighting.Levels checks it against '
                     f'Day): set it in the "state" block instead; skipped')
                continue
            set_recorded(record, actor, component, prop, value)
            log(f'{actor.get_actor_label()}.{prop} = {value}')


def paint_state(record, settings):
    """painted.json's "state" block (off by default): the lab's own sun and sky in the lights and in the lighting state
    Day alike, so a switch back to Day keeps them. Looter.World.Lighting.Levels expects the island's Day to be the
    default state, so it fails until that test allows a painted Day (C++)."""
    state = settings.get('state', {})
    if not state.get('enabled'):
        return
    warn('the "state" block is on: Looter.World.Lighting.Levels fails until it allows the island a painted Day')
    states_actor = next((a for a in level_actors() if a.get_class().get_name() == 'LightingStates'), None)
    sun_actor, sun = environment_target('sun')
    sky_actor, sky = environment_target('skyLight')
    air_actor, air = environment_target('atmosphere')
    fog_actor, fog = environment_target('fog')
    day_changes = {}
    s = state.get('sun', {})
    if sun is not None and s:
        if 'azimuth' in s or 'elevation' in s:
            key = f'{component_key(sun_actor, sun)}|@rotation'
            if key not in record['properties']:
                record['properties'][key] = encode(sun_actor.get_actor_rotation())
            old = sun_actor.get_actor_rotation()
            elevation = s.get('elevation', -old.pitch)
            azimuth = s.get('azimuth', (old.yaw + 180.0) % 360.0)
            sun_actor.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-elevation, yaw=(azimuth + 180.0) % 360.0), False)
            day_changes.update(sun_bearing=azimuth, sun_elevation=elevation)
        if 'intensity' in s:
            set_recorded(record, sun_actor, sun, 'intensity', s['intensity'])
            day_changes['sun_intensity'] = s['intensity']
        if 'temperature' in s:
            set_recorded(record, sun_actor, sun, 'temperature', s['temperature'])
            day_changes['sun_temperature'] = s['temperature']
    if sky is not None and 'intensity' in state.get('sky', {}):
        set_recorded(record, sky_actor, sky, 'intensity', state['sky']['intensity'])
        day_changes['sky_intensity'] = state['sky']['intensity']
    luminance = state.get('atmosphere', {}).get('skyLuminance')
    if air is not None and luminance:
        set_recorded(record, air_actor, air, 'sky_luminance_factor', luminance)
        day_changes['sky_luminance'] = unreal.LinearColor(*painted_look.vec4(luminance))
        if fog is not None:
            # The haze takes the sky's light without the sky's own correction (as FLightingTargets::Write sets it).
            set_recorded(record, fog_actor, fog, 'sky_atmosphere_ambient_contribution_color_scale',
                         [1.0 / max(c, 0.01) for c in luminance[:3]])
    inscattering = state.get('fog', {}).get('inscattering')
    if fog is not None and inscattering:
        set_recorded(record, fog_actor, fog, 'fog_inscattering_luminance', inscattering)
        day_changes['fog_inscattering'] = unreal.LinearColor(*painted_look.vec4(inscattering))
    if states_actor is None or not day_changes:
        return
    states = list(states_actor.get_editor_property('states'))
    for i, each in enumerate(states):
        if str(each.get_editor_property('name')).lower() != 'day':
            continue
        for field, value in day_changes.items():
            if field not in record['day']:
                record['day'][field] = encode(each.get_editor_property(field))
            each.set_editor_property(field, value)
        states[i] = each
    states_actor.modify()
    states_actor.set_editor_property('states', states)
    log('Day: ' + ', '.join(f'{k} {v}' for k, v in day_changes.items()))


def post_instance(settings):
    """MI_PaintedPost_Island: M_PaintedPost with painted.json's "post.screen" changes."""
    parent = unreal.load_asset(f'{MASTERS}/M_PaintedPost')
    path = f'{FOLDER}/{POST_INSTANCE}'
    created = not unreal.EditorAssetLibrary.does_asset_exist(path)
    instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        POST_INSTANCE, FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew()) \
        if created else unreal.load_asset(path)
    changed = created or instance.get_editor_property('parent') != parent
    if changed:
        mel.set_material_instance_parent(instance, parent)
    changed |= set_values(instance, painted_look.post_params(settings.get('post', {}).get('screen')))
    if changed:
        mel.update_material_instance(instance)
        unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
        log(f'{path} {"made" if created else "updated"}')
    return instance


def place_post(record, settings, bias):
    """The PaintedPost volume: unbound, above IslandPost, with the painted grade and the screen pass."""
    post = next((a for a in level_actors() if tagged(a, TAG)), None)
    if post is None:
        post = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0.0, 0.0, 0.0))
        post.set_actor_label(POST_LABEL)
        post.set_folder_path(POST_FOLDER)
        # Its own tag, not the build's: a rebuild of the island leaves it, and undo finds it by this.
        post.set_editor_property('tags', [unreal.Name(TAG)])
        log(f'{POST_LABEL} placed')
    record['post'] = True
    post.set_editor_property('unbound', True)
    post.set_editor_property('priority', POST_PRIORITY)
    post.set_editor_property('blend_weight', 1.0)
    config = settings.get('post', {})
    values = dict(config.get('volume', {}))
    values['auto_exposure_bias'] = bias
    use_material = config.get('material', True)
    if not use_material:
        # Without the screen pass, the engine's own (black) vignette.
        values.setdefault('vignette_intensity', painted_look.POST['vignette']['amount'])
    settings_struct = post.get_editor_property('settings')
    for name, value in values.items():
        if name == 'about':
            continue
        value = unreal.Vector4(*painted_look.vec4(value)) if isinstance(value, (list, tuple)) else float(value)
        settings_struct.set_editor_property(name, value)
        settings_struct.set_editor_property(f'override_{name}', True)
    blendables = [unreal.WeightedBlendable(weight=1.0, object=post_instance(settings))] if use_material else []
    settings_struct.set_editor_property('weighted_blendables', unreal.WeightedBlendables(array=blendables))
    post.set_editor_property('settings', settings_struct)
    log(f'{POST_LABEL}: ' + ', '.join(f'{k} {v}' for k, v in sorted(values.items()) if k != 'about')
        + (', screen pass on' if use_material else ', no screen pass'))


def recapture_sky():
    actor, component = environment_target('skyLight')
    if component is not None:
        component.recapture_sky()


# --- Modes ---

def apply():
    open_island()
    check_masters()
    settings = load_settings()
    bias = float(settings.get('exposureBias', painted_look.DEFAULT_EXPOSURE_BIAS))
    record = load_record() or new_record()
    # Entries for actors a rebuild replaced can't be put back and no longer matter.
    found = find_components(record['slots'])
    stale = [key for key, (actor, component) in found.items() if component is None]
    for key in stale:
        del record['slots'][key]
    if stale:
        log(f'{len(stale)} recorded slots belong to actors no longer in the level (a rebuild): dropped')
    update_painted_instances(settings, bias)
    paint_slots(record, settings, bias)
    paint_lights(record, settings)
    paint_state(record, settings)
    place_post(record, settings, bias)
    recapture_sky()
    save_level()
    save_record(record)
    log('applied: tour it (Tools/tour.ps1, Art/Levels/TutorialIsland/views.json) and play it')


def undo():
    open_island()
    record = load_record()
    restored = unrecorded = gone = 0
    if record:
        found = find_components(record['slots'])
        for key, entry in record['slots'].items():
            actor, component = found[key]
            if component is None:
                gone += 1
                continue
            original = entry['overrides']
            all_back = True
            for slot_text in entry['painted']:
                slot = int(slot_text)
                if not is_painted(component.get_material(slot)):
                    all_back = False  # changed since by hand: left as it is
                    continue
                path = original[slot] if slot < len(original) else None
                component.modify()
                component.set_material(slot, unreal.load_asset(path) if path else None)
                restored += 1
            if all_back and overrides(component) != original:
                # The override list exactly as it was (set_material grows it to the slot it sets).
                component.set_editor_property('override_materials',
                                              [unreal.load_asset(p) if p else None for p in original])
    # Painted slots nobody recorded (an actor copied after apply): back on the painted instance's parent, or on the
    # mesh's own material when that is the parent.
    for actor in level_actors():
        for component in mesh_components(actor):
            mesh = component.get_editor_property('static_mesh')
            for slot in range(component.get_num_materials()):
                worn = component.get_material(slot)
                if not is_painted(worn):
                    continue
                parent = worn.get_editor_property('parent')
                own = mesh.get_material(slot) if mesh is not None else None
                component.modify()
                component.set_material(slot, None if own is not None and own == parent else parent)
                unrecorded += 1
    properties = 0
    if record:
        by_path = {a.get_path_name(): a for a in level_actors()}
        for key, stored in record['properties'].items():
            actor_path, name, prop = key.split('|')
            actor = by_path.get(actor_path)
            if actor is None:
                gone += 1
                continue
            if prop == '@rotation':
                actor.set_actor_rotation(decode(stored), False)
            else:
                component = next((c for c in actor.get_components_by_class(unreal.ActorComponent)
                                  if c.get_name() == name), None)
                if component is None:
                    gone += 1
                    continue
                component.modify()
                component.set_editor_property(prop, decode(stored))
            properties += 1
        if record.get('day'):
            states_actor = next((a for a in level_actors() if a.get_class().get_name() == 'LightingStates'), None)
            if states_actor is not None:
                states = list(states_actor.get_editor_property('states'))
                for i, each in enumerate(states):
                    if str(each.get_editor_property('name')).lower() == 'day':
                        for field, stored in record['day'].items():
                            each.set_editor_property(field, decode(stored))
                        states[i] = each
                states_actor.modify()
                states_actor.set_editor_property('states', states)
                log('Day put back')
    posts = [a for a in level_actors() if tagged(a, TAG)]
    for post in posts:
        actors.destroy_actor(post)
    recapture_sky()
    save_level()
    if record:
        os.remove(RECORD)
        log(f'{RECORD} deleted')
    log(f'undone: {restored} slots back on their own materials, {unrecorded} painted slots nobody recorded put back on '
        f'their parents, {properties} properties back, {len(posts)} {POST_LABEL} volume(s) removed'
        + (f'; {gone} recorded things were no longer in the level' if gone else ''))
    if not record:
        warn(f'there was no record ({RECORD}): only painted slots and the volume could be undone')
    log(f'the painted instances in {FOLDER} stay as assets, unused; to drop the style, delete that folder with git')


def status():
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world.get_path_name().split('.')[0] != ISLAND:
        log(f'the open level is {world.get_path_name()}, not {ISLAND}: open it to see its slots')
    for name in SWITCHED:
        master = unreal.load_asset(f'{MASTERS}/{name}')
        has = master is not None and 'Painted' in [str(n) for n in mel.get_static_switch_parameter_names(master)]
        log(f'{name}: {"has" if has else "has NO"} Painted switch')
    painted = unpainted = 0
    loose = {}
    for actor in level_actors():
        for component in mesh_components(actor):
            for slot in range(component.get_num_materials()):
                worn = component.get_material(slot)
                if is_painted(worn):
                    painted += 1
                elif paints(worn):
                    unpainted += 1
                    loose[worn.get_name()] = loose.get(worn.get_name(), 0) + 1
    log(f'{painted} slots painted, {unpainted} paintable slots not painted'
        + (': ' + ', '.join(f'{k} x{v}' for k, v in sorted(loose.items())) if loose else ''))
    posts = [a for a in level_actors() if tagged(a, TAG)]
    log(f'{POST_LABEL} volume: {"present" if posts else "absent"}')
    record = load_record()
    if record is None:
        log('no record: the island is not painted (or was undone)')
        return
    found = find_components(record['slots'])
    stale = sum(1 for actor, component in found.values() if component is None)
    log(f'record: {len(record["slots"])} components, {len(record["properties"])} properties, '
        f'{len(record.get("day", {}))} Day fields' + (f'; {stale} components gone (run apply again)' if stale else ''))
    if unpainted:
        log('some paintable slots are unpainted (the scatter generated again, or a rebuild): run apply again')


MODES = {'apply': apply, 'undo': undo, 'status': status}

if __name__ == '__main__':
    wanted = [a for a in sys.argv[1:] if a in MODES]
    if len(wanted) != 1:
        raise SystemExit('usage: build_island_painted.py apply|undo|status')
    MODES[wanted[0]]()
