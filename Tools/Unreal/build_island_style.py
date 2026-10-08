"""Puts the tutorial island, and only the tutorial island, in one of the Style Lab's looks, and takes it off again
exactly. Run in the open editor (it opens Lvl_TutorialIsland itself, and stops while anything is playing):
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_island_style.py sunbleached apply"
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_island_style.py sunbleached undo"
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_island_style.py sunbleached status"
A style is three things: its static switch and parameters on the masters, and its post material
(Tools/Unreal/build_world_materials.py: run it first); its numbers (Tools/Unreal/style_<name>.py, which also says which
masters it changes); and the island's settings for it (Art/Levels/TutorialIsland/style_<name>.json). The styles so far:
sunbleached (Style Lab 6). Adding one: its section in build_world_materials.py (a switch named for it wrapping the
masters' outputs it changes, styled()), its style_<name>.py with the same names as style_sunbleached.py's (NAME,
MASTERS, SWITCHED, POST_MASTER, UNSTYLED_ROLES, DEFAULT_EXPOSURE_BIAS, instance_values, post_params), its json.

One style at a time: apply refuses while another style is on (undo that one first), and undo refuses to take off a
style that isn't the one on. Art/Levels/TutorialIsland/style_applied.json records which style is on and everything
apply changed; commit it with the level.

apply:
- Every static mesh slot in the level wearing an instance of a master the style changes (style.MASTERS: the placed
  actors, the dressing, the sky dome and the PCG scatter's instances alike) wears an island-only child of it instead:
  /Game/Art/Materials/IslandStyle/<Style>/MI_<name>_<Style>, with the style's switch on and the style's values for its
  role (by its texture set, else words in its name: style_common.py). The shared instances are never touched, so every
  other level keeps its look; the main menu, which is this island opened with ?game=Menu, shows the style.
- The island's light, through what its lighting states never write (World/LightingTargets.cpp): the lights' colour
  filters and the like (the json's "lights"). The states' values (the sun's turn, intensity and temperature, the sky
  light's intensity, the sky's colour, the fog's colours, the exposure) stay the island's Day, which
  Looter.World.Lighting.Levels requires to equal the default state. The json's "state" block, off by default, writes a
  style's own sun and sky into the lights and Day alike; that test fails until it allows a styled island its own Day.
- A <Style>Post volume (unbound, above IslandPost, tagged IslandStyle) with the style's grade, exposure, bloom and post
  material: the lighting states keep writing IslandPost's exposure, which this one outranks.
The level is saved. Running it again paints what isn't styled yet (the scatter after it generates again, actors a
rebuild placed) and updates every styled instance to the json and the style's numbers; a rebuild of the island
(build_area.py) places its actors unstyled, so run apply after one.

undo: every recorded slot back on its original material (a styled slot nobody recorded on its styled instance's
parent), every recorded property back, the style volume removed, the level saved and the record deleted. The styled
instances stay as assets, unused: to drop a style for good, delete Content/Art/Materials/IslandStyle/<Style> (with the
editor's asset tools or git) once the island no longer wears it.

status: which style is on, what's styled, what isn't, and whether the record still matches the level.
"""
import importlib
import json
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import style_common  # noqa: E402
style_common = importlib.reload(style_common)

PROJECT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
ISLAND = '/Game/Maps/Lvl_TutorialIsland'
MASTERS = '/Game/Art/Materials/Masters'
STYLES_FOLDER = '/Game/Art/Materials/IslandStyle'
LEVEL_DATA = os.path.join(PROJECT, 'Art', 'Levels', 'TutorialIsland')
RECORD = os.path.join(LEVEL_DATA, 'style_applied.json')
BUILD_TAG = 'IslandBuild'
TAG = 'IslandStyle'
POST_FOLDER = 'Island/Style'
POST_PRIORITY = 10.0
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
    unreal.log(f'Island style: {message}')


def warn(message):
    unreal.log_warning(f'Island style: {message}')


class Style:
    """A style by its short name (sunbleached): its numbers (style_<name>.py) and the island's settings for it."""

    def __init__(self, short):
        if not short.replace('_', '').isalnum() or short == 'common':
            raise RuntimeError(f'no style "{short}"')
        try:
            self.look = importlib.reload(importlib.import_module(f'style_{short}'))
        except ModuleNotFoundError:
            raise RuntimeError(f'no Tools/Unreal/style_{short}.py: no style "{short}"')
        self.short = short
        self.name = self.look.NAME
        self.folder = f'{STYLES_FOLDER}/{self.name}'
        self.post_label = f'{self.name}Post'
        with open(os.path.join(LEVEL_DATA, f'style_{short}.json')) as f:
            self.settings = json.load(f)
        self.bias = float(self.settings.get('exposureBias', self.look.DEFAULT_EXPOSURE_BIAS))


# --- Files ---

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


def new_record(style):
    return {'about': 'Written by Tools/Unreal/build_island_style.py apply: which style the tutorial island wears and what '
                     'it changed, so undo puts it back exactly. Slots: actor path|component -> its original override '
                     'list and the slots styled (slot -> the material it wore). Properties: actor path|component|property '
                     '-> the value before. Day: the lighting state Day\'s fields before (the "state" block only).',
            'style': style.short, 'name': style.name, 'level': ISLAND, 'slots': {}, 'properties': {}, 'day': {},
            'post': False}


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
    """Each 'actor path|component' key's (actor, component), the component None for one gone (a rebuild replaced its
    actor)."""
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


def style_volumes():
    return [a for a in level_actors() if tagged(a, TAG)]


# --- Materials ---

def master_of(material, style):
    """The master (by name) a material is an instance of, when it's one the style changes; else None."""
    base = material.get_base_material() if material is not None else None
    if base is None or not base.get_path_name().startswith(MASTERS + '/'):
        return None
    name = base.get_name()
    return name if name in style.look.MASTERS else None


def is_styled(material):
    return material is not None and material.get_path_name().startswith(STYLES_FOLDER + '/')


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


def styles_it(material, style):
    """Whether apply styles a slot wearing this (an instance of a master the style changes, and not a role it leaves)."""
    master = master_of(material, style)
    if master is None or is_styled(material):
        return False
    if style.look.MASTERS[master] != 'surface':
        return True
    role = style_common.role_of(material.get_name(), texture_set(material), master)
    return role not in style.look.UNSTYLED_ROLES


def styled_values(worn, master, style):
    """(role, values) for the styled instance of worn, or (role, None) for a surface the style leaves."""
    return style.look.instance_values(style.look.MASTERS[master], worn.get_name(), texture_set(worn), master,
                                      lambda name: vector_value(worn, name), style.settings, style.bias)


def instance_name(worn, style):
    name = worn.get_name()
    return f'MI_{name[3:] if name.startswith("MI_") else name}_{style.name}'


def set_values(instance, values):
    changed = False
    for key, value in values.get('scalars', {}).items():
        if abs(mel.get_material_instance_scalar_parameter_value(instance, key) - float(value)) > 1e-5:
            mel.set_material_instance_scalar_parameter_value(instance, key, float(value))
            changed = True
    for key, value in values.get('vectors', {}).items():
        color = unreal.LinearColor(*style_common.vec4(value))
        old = mel.get_material_instance_vector_parameter_value(instance, key)
        if any(abs(a - b) > 1e-5 for a, b in zip(old.to_tuple(), color.to_tuple())):
            mel.set_material_instance_vector_parameter_value(instance, key, color)
            changed = True
    return changed


def instance_of(parent, name, folder, values, switch=None):
    """An island-only child of parent (made, or brought up to date and saved only when it changes), with the style's
    switch on when given."""
    path = f'{folder}/{name}'
    created = not unreal.EditorAssetLibrary.does_asset_exist(path)
    if created:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    else:
        instance = unreal.load_asset(path)
    changed = created or instance.get_editor_property('parent') != parent
    if changed:
        mel.set_material_instance_parent(instance, parent)
    if switch and not mel.get_material_instance_static_switch_parameter_value(instance, switch):
        mel.set_material_instance_static_switch_parameter_value(instance, switch, True)
        changed = True
    changed |= set_values(instance, values)
    if changed:
        mel.update_material_instance(instance)
        unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
        log(f'{path} {"made" if created else "updated"}')
    return instance


def styled_instance(worn, master, values, style):
    switch = style.name if master in style.look.SWITCHED else None
    return instance_of(worn, instance_name(worn, style), style.folder, values, switch)


def check_masters(style):
    """Stops unless build_world_materials.py has given the masters the style's switch and made its post material."""
    for name in style.look.SWITCHED:
        master = unreal.load_asset(f'{MASTERS}/{name}')
        switches = [str(n) for n in mel.get_static_switch_parameter_names(master)] if master else []
        if style.name not in switches:
            raise RuntimeError(f'{MASTERS}/{name} has no {style.name} switch: run '
                               f'Tools/Unreal/build_world_materials.py first')
    post = getattr(style.look, 'POST_MASTER', None)
    if post and not unreal.EditorAssetLibrary.does_asset_exist(f'{MASTERS}/{post}'):
        raise RuntimeError(f'no {MASTERS}/{post}: run Tools/Unreal/build_world_materials.py first')


def style_slots(record, style):
    """Every slot the style changes onto its styled instance, recorded."""
    made, skipped, roles = {}, {}, {}
    styled = already = 0
    for actor in level_actors():
        if tagged(actor, TAG):
            continue
        for component in mesh_components(actor):
            key = component_key(actor, component)
            for slot in range(component.get_num_materials()):
                worn = component.get_material(slot)
                if worn is None:
                    continue
                if is_styled(worn):
                    already += 1
                    continue
                master = master_of(worn, style)
                if master is None:
                    continue
                path = worn.get_path_name()
                if path not in made:
                    role, values = styled_values(worn, master, style)
                    made[path] = styled_instance(worn, master, values, style) if values else None
                    if values:
                        roles[instance_name(worn, style)] = role
                    else:
                        skipped[worn.get_name()] = role
                instance = made[path]
                if instance is None:
                    continue
                entry = record['slots'].get(key)
                if entry is None:
                    entry = record['slots'][key] = {'actor': str(actor.get_actor_label()),
                                                    'overrides': overrides(component), 'styled': {}}
                entry['styled'][str(slot)] = path
                component.modify()
                component.set_material(slot, instance)
                styled += 1
    for name, role in sorted(skipped.items()):
        log(f'{name} left as it is (role {role}: {style.name} leaves it)')
    log(f'{styled} slots styled now, {already} were already; {len([m for m in made.values() if m])} styled instances'
        + (': ' + ', '.join(f'{n} ({r})' for n, r in sorted(roles.items())) if roles else ''))


def update_styled_instances(style):
    """Every styled instance on the island brought up to the json and the style's numbers, even ones whose slots were
    styled by an earlier run (style_slots() only sees the unstyled)."""
    seen = set()
    for actor in level_actors():
        for component in mesh_components(actor):
            for slot in range(component.get_num_materials()):
                worn = component.get_material(slot)
                if not is_styled(worn) or worn.get_path_name() in seen:
                    continue
                seen.add(worn.get_path_name())
                parent = worn.get_editor_property('parent')
                master = master_of(parent, style)
                if parent is None or master is None:
                    continue
                role, values = styled_values(parent, master, style)
                if values:
                    styled_instance(parent, master, values, style)


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
    """A json value in the type the property has now (a light's colour filter is an sRGB Color, given linear)."""
    if isinstance(current, unreal.Color):
        r, g, b = value[:3]
        return unreal.Color(r=srgb8(r), g=srgb8(g), b=srgb8(b), a=255)
    if isinstance(current, unreal.LinearColor):
        return unreal.LinearColor(*style_common.vec4(value))
    if isinstance(current, unreal.Vector4):
        return unreal.Vector4(*style_common.vec4(value))
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


def style_lights(record, style):
    """The lights' colour filters and other properties a lighting state never writes (the json's "lights")."""
    state_on = style.settings.get('state', {}).get('enabled', False)
    for target, props in style.settings.get('lights', {}).items():
        if target == 'about':
            continue
        actor, component = environment_target(target)
        if component is None:
            warn(f'no {target} in the level: its settings are skipped')
            continue
        for prop, value in props.items():
            if prop in STATE_PROPERTIES.get(target, ()) and not state_on:
                warn(f'{target}.{prop} is what a lighting state writes (Looter.World.Lighting.Levels checks it against '
                     f'Day): set it in the "state" block instead; skipped')
                continue
            set_recorded(record, actor, component, prop, value)
            log(f'{actor.get_actor_label()}.{prop} = {value}')


def style_state(record, style):
    """The json's "state" block (off by default): the style's own sun and sky in the lights and in the lighting state
    Day alike, so a switch back to Day keeps them. Looter.World.Lighting.Levels expects the island's Day to be the
    default state, so it fails until that test allows a styled island its own Day (C++)."""
    state = style.settings.get('state', {})
    if not state.get('enabled'):
        return
    warn('the "state" block is on: Looter.World.Lighting.Levels fails until it allows a styled island its own Day')
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
        day_changes['sky_luminance'] = unreal.LinearColor(*style_common.vec4(luminance))
        if fog is not None:
            # The haze takes the sky's light without the sky's own correction (as FLightingTargets::Write sets it).
            set_recorded(record, fog_actor, fog, 'sky_atmosphere_ambient_contribution_color_scale',
                         [1.0 / max(c, 0.01) for c in luminance[:3]])
    inscattering = state.get('fog', {}).get('inscattering')
    if fog is not None and inscattering:
        set_recorded(record, fog_actor, fog, 'fog_inscattering_luminance', inscattering)
        day_changes['fog_inscattering'] = unreal.LinearColor(*style_common.vec4(inscattering))
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


def post_instance(style):
    """MI_<Post master>_Island: the style's post material with the json's "post.screen" changes."""
    master = style.look.POST_MASTER
    parent = unreal.load_asset(f'{MASTERS}/{master}')
    values = style.look.post_params(style.settings.get('post', {}).get('screen'), style.bias)
    return instance_of(parent, f'MI_{master[2:]}_Island', style.folder, values)


def place_post(record, style):
    """The style's volume: unbound, above IslandPost, with its grade, exposure and bloom and its post material."""
    post = next(iter(style_volumes()), None)
    if post is None:
        post = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0.0, 0.0, 0.0))
        post.set_actor_label(style.post_label)
        post.set_folder_path(POST_FOLDER)
        # Its own tag, not the build's: a rebuild of the island leaves it, and undo finds it by this.
        post.set_editor_property('tags', [unreal.Name(TAG)])
        log(f'{style.post_label} placed')
    record['post'] = True
    post.set_editor_property('unbound', True)
    post.set_editor_property('priority', POST_PRIORITY)
    post.set_editor_property('blend_weight', 1.0)
    config = style.settings.get('post', {})
    values = {k: v for k, v in config.get('volume', {}).items() if k != 'about'}
    values['auto_exposure_bias'] = style.bias
    use_material = config.get('material', True) and getattr(style.look, 'POST_MASTER', None)
    settings_struct = post.get_editor_property('settings')
    for name, value in values.items():
        settings_struct.set_editor_property(name, as_property(settings_struct.get_editor_property(name), value))
        settings_struct.set_editor_property(f'override_{name}', True)
    blendables = [unreal.WeightedBlendable(weight=1.0, object=post_instance(style))] if use_material else []
    settings_struct.set_editor_property('weighted_blendables', unreal.WeightedBlendables(array=blendables))
    post.set_editor_property('settings', settings_struct)
    log(f'{style.post_label}: ' + ', '.join(f'{k} {v}' for k, v in sorted(values.items()))
        + (f', {style.look.POST_MASTER} on' if use_material else ', no post material'))


def recapture_sky():
    actor, component = environment_target('skyLight')
    if component is not None:
        component.recapture_sky()


# --- Modes ---

def apply(style):
    open_island()
    check_masters(style)
    record = load_record()
    if record and record.get('style') != style.short:
        raise RuntimeError(f'the island wears {record.get("name", record.get("style"))}: take it off first '
                           f'(build_island_style.py {record.get("style")} undo)')
    record = record or new_record(style)
    # Entries for actors a rebuild replaced can't be put back and no longer matter.
    found = find_components(record['slots'])
    stale = [key for key, (actor, component) in found.items() if component is None]
    for key in stale:
        del record['slots'][key]
    if stale:
        log(f'{len(stale)} recorded slots belong to actors no longer in the level (a rebuild): dropped')
    update_styled_instances(style)
    style_slots(record, style)
    style_lights(record, style)
    style_state(record, style)
    place_post(record, style)
    recapture_sky()
    save_level()
    save_record(record)
    log(f'{style.name} applied: tour it (Tools/tour.ps1) and play it')


def undo(style):
    open_island()
    record = load_record()
    if record and record.get('style') != style.short:
        raise RuntimeError(f'the island wears {record.get("name", record.get("style"))}, not {style.name}: '
                           f'build_island_style.py {record.get("style")} undo')
    restored = unrecorded = gone = properties = 0
    if record:
        found = find_components(record['slots'])
        for key, entry in record['slots'].items():
            actor, component = found[key]
            if component is None:
                gone += 1
                continue
            original = entry['overrides']
            all_back = True
            for slot_text in entry['styled']:
                slot = int(slot_text)
                if not is_styled(component.get_material(slot)):
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
    # Styled slots nobody recorded (an actor copied after apply): back on the styled instance's parent, or on the mesh's
    # own material when that is the parent.
    for actor in level_actors():
        for component in mesh_components(actor):
            mesh = component.get_editor_property('static_mesh')
            for slot in range(component.get_num_materials()):
                worn = component.get_material(slot)
                if not is_styled(worn):
                    continue
                parent = worn.get_editor_property('parent')
                own = mesh.get_material(slot) if mesh is not None else None
                component.modify()
                component.set_material(slot, None if own is not None and own == parent else parent)
                unrecorded += 1
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
    posts = style_volumes()
    for post in posts:
        actors.destroy_actor(post)
    recapture_sky()
    save_level()
    if record:
        os.remove(RECORD)
        log(f'{RECORD} deleted')
    log(f'{style.name} undone: {restored} slots back on their own materials, {unrecorded} styled slots nobody recorded '
        f'put back on their parents, {properties} properties back, {len(posts)} style volume(s) removed'
        + (f'; {gone} recorded things were no longer in the level' if gone else ''))
    if not record:
        warn(f'there was no record ({RECORD}): only styled slots and the volume could be undone')
    log(f'the styled instances in {style.folder} stay as assets, unused; to drop the style, delete that folder')


def status(style):
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world.get_path_name().split('.')[0] != ISLAND:
        log(f'the open level is {world.get_path_name()}, not {ISLAND}: open it to see its slots')
    record = load_record()
    log(f'the island wears {record.get("name", record.get("style")) if record else "no style"}')
    for name in style.look.SWITCHED:
        master = unreal.load_asset(f'{MASTERS}/{name}')
        has = master is not None and style.name in [str(n) for n in mel.get_static_switch_parameter_names(master)]
        log(f'{name}: {"has" if has else "has NO"} {style.name} switch')
    styled = unstyled = 0
    loose = {}
    for actor in level_actors():
        for component in mesh_components(actor):
            for slot in range(component.get_num_materials()):
                worn = component.get_material(slot)
                if is_styled(worn):
                    styled += 1
                elif worn is not None and styles_it(worn, style):
                    unstyled += 1
                    loose[worn.get_name()] = loose.get(worn.get_name(), 0) + 1
    log(f'{styled} slots styled, {unstyled} slots {style.name} would style not styled'
        + (': ' + ', '.join(f'{k} x{v}' for k, v in sorted(loose.items())) if loose else ''))
    log(f'style volume: {", ".join(str(a.get_actor_label()) for a in style_volumes()) or "none"}')
    if record is None:
        return
    found = find_components(record['slots'])
    stale = sum(1 for actor, component in found.values() if component is None)
    log(f'record: {len(record["slots"])} components, {len(record["properties"])} properties, '
        f'{len(record.get("day", {}))} Day fields' + (f'; {stale} components gone (run apply again)' if stale else ''))
    if unstyled and record.get('style') == style.short:
        log('some slots are unstyled (the scatter generated again, or a rebuild): run apply again')


MODES = {'apply': apply, 'undo': undo, 'status': status}

if __name__ == '__main__':
    args = sys.argv[1:]
    if len(args) != 2 or args[1] not in MODES:
        raise SystemExit('usage: build_island_style.py <style> apply|undo|status (styles: Tools/Unreal/style_<name>.py)')
    MODES[args[1]](Style(args[0]))
