"""An area's light, sky and fog, for build_area.py: the tutorial island's warm afternoon by default, or what layout.json
level.environment changes (Ransom's Rest's golden hour, step 5a of Docs/Areas/RansomsRest.md). Every key is optional:

  "environment": {
    "sun": {"azimuth": 247.5, "elevation": 15, "intensity": 7, "temperature": 4300},
    "sky": {"intensity": 1.0, "lowerHemisphere": [0.3, 0.25, 0.15]},
    "fog": {"height": -7000, "density": 0.02, "falloff": 0.05, "startDistance": 6000, "maxOpacity": 1.0,
            "inscattering": [0.4, 0.33, 0.27], "cutoffDistance": 1350000,
            "directional": {"color": [1.0, 0.7, 0.4], "exponent": 8, "startDistance": 6000}},
    "atmosphere": {"planetTop": -9000, "groundAlbedo": [0.45, 0.36, 0.22], "skyLuminance": [1.0, 1.35, 2.0],
                   "ozone": 1},
    "clouds": {"radius": 1500000, "tint": [1, 1, 1]},
    "backdrop": {"tint": [1, 1, 1]},
    "post": {"exposureBias": 0.4},
    "states": {
      "Dusk": {"sun": {"azimuth": 252, "elevation": 4, "intensity": 4, "temperature": 3200, "shadowDistance": 6000},
               "sky": {"intensity": 0.6}, "atmosphere": {"skyLuminance": [1.2, 2.1, 5.0], "ozone": 3.5},
               "fog": {"inscattering": [0.2, 0.13, 0.08]}, "backdrop": {"tint": [0.3, 0.24, 0.24]}}
    }
  }

The sun is placed by compass: its azimuth is the bearing it stands at (0 north, 90 east, 247.5 west-southwest) and its
elevation how high it stands over the horizon. Lengths are cm, colors linear RGB. Unreal's +X is north and +Y east.

The atmosphere's skyLuminance multiplies the sky's own color. The sun's color temperature tints the whole sky through
the atmosphere (a 4100 K sun turns a blue sky slate grey), so a golden hour cancels most of it here and the sky reddens
only where the atmosphere reddens it, low toward the sun. The correction is the sky's alone: the height fog takes the
sky's light without it, so the haze keeps the fog's own colors. Its ozone is the ozone layer's absorption as a multiple
of Earth's: at sunset it soaks the green out of the light crossing the sky, leaving the sky away from the sun
twilight's blue-violet. The fog's cutoffDistance (none by default) ends the haze before the sky and the cloud dome past
it, which would otherwise wear it as a grey veil; a backdrop inside it still fades into the haze.

Lighting states (step 13): an ALightingStates actor beside the lights holds the level's named states, which
ULightingStateSubsystem switches between (Looter.Light Day|Dusk). Day is always the environment above, the light the
level is built in; it can't be redefined. Each entry of "states" is another state, written as the groups it changes
from Day: a group merges key by key over Day's (fog.directional too), so a dusk lists only what dusk changes. A state
can change only what a switch sets at runtime (STATE_KEYS): the sun's azimuth, elevation, intensity, temperature,
shadowDistance and cascades; the sky light's intensity (its color comes from recapturing the sky); the atmosphere's
skyLuminance and ozone; the fog's density, inscattering and directional glow; the backdrop's and the clouds' tints,
which go through the material parameter collection MPC_Lighting (M_Backdrop multiplies every layer by it); and the post
volume's exposureBias. The rest (the fog's height, falloff and cutoff, the planet and its ground, the dome's size) stays
as placed. Day's tints are best left white, since the editor shows the collection's white defaults. A layout without
"states" gets a Day alone, and is otherwise built as before.
"""
import unreal

# The sky's clouds: a dome around the level with the painted cloud material.
SKY_DOME = '/Engine/EngineSky/SM_SkySphere'
SKY_CLOUDS = '/Game/Art/Materials/Masters/M_SkyClouds'
# The level's lighting states (World/LightingStates.h).
LIGHTING_STATES = '/Script/AI_Looter_Shooter.LightingStates'

# The tutorial island's afternoon: the light from the west-northwest, so the view from the spawn (looking northeast) is
# lit from the side. FLightingState's defaults (World/LightingState.h) are these numbers; Looter.World.Lighting.Defaults
# checks them.
DEFAULTS = {
    'sun': {'azimuth': 292.0, 'elevation': 38.0, 'intensity': 7.0, 'temperature': 5300.0,
            # Cascaded shadows (Low and Medium) reach 100 m before the preset's scale (70 m on Medium).
            'shadowDistance': 10000.0, 'cascades': 2},
    'sky': {'intensity': 1.2, 'lowerHemisphere': [0.26, 0.30, 0.18]},  # light bouncing off the meadow
    # Blue, filling the void under the island. A cutoff of 0 is none: the haze reaches the sky.
    'fog': {'height': -2000.0, 'density': 0.03, 'falloff': 0.12, 'startDistance': 3000.0, 'maxOpacity': 0.85,
            'inscattering': [0.20, 0.29, 0.44], 'directional': None, 'cutoffDistance': 0.0},
    # The engine's Earth, its planet top at the world's origin unless planetTop moves it.
    'atmosphere': {'skyLuminance': [1.0, 1.0, 1.0], 'ozone': 1.0},
    # 1 km: the island's horizon is the sky itself. The tint multiplies the painted clouds (MPC_Lighting CloudTint).
    'clouds': {'radius': 100000.0, 'tint': [1.0, 1.0, 1.0]},
    # Multiplies every backdrop layer's own tint (MPC_Lighting BackdropTint): white is the backdrop as built.
    'backdrop': {'tint': [1.0, 1.0, 1.0]},
    'post': {'exposureBias': 0.4},
}

# What a lighting state can change from Day, group by group (what FLightingState carries).
STATE_KEYS = {
    'sun': {'azimuth', 'elevation', 'intensity', 'temperature', 'shadowDistance', 'cascades'},
    'sky': {'intensity'},
    'atmosphere': {'skyLuminance', 'ozone'},
    'fog': {'density', 'inscattering', 'directional'},
    'backdrop': {'tint'},
    'clouds': {'tint'},
    'post': {'exposureBias'},
}
DIRECTIONAL_KEYS = {'color', 'exponent', 'startDistance'}
# Without a glow toward the sun, the fog keeps the engine's: none, exponent 4, from 100 m.
NO_GLOW = {'color': [0.0, 0.0, 0.0], 'exponent': 4.0, 'startDistance': 10000.0}


def settings(source):
    """The defaults with the layout's level.environment over them, one group at a time."""
    given = source.get('level', {}).get('environment', {})
    merged = {}
    for group, default in DEFAULTS.items():
        value = given.get(group, default)
        if isinstance(default, dict) and isinstance(value, dict):
            value = {**default, **value}
        merged[group] = value
    return merged


def merge(base, changes):
    """base with changes over it, key by key; a dict inside merges the same way (fog.directional)."""
    if not isinstance(base, dict) or not isinstance(changes, dict):
        return changes
    result = dict(base)
    for key, value in changes.items():
        result[key] = merge(base.get(key), value)
    return result


def check_state_group(where, group, value):
    """Stops on anything in a state's group that a switch can't change."""
    allowed = STATE_KEYS.get(group)
    if allowed is None or not isinstance(value, dict):
        raise ValueError(f'{where}: a state changes only the groups {sorted(STATE_KEYS)}, each a dict of settings')
    unknown = sorted(set(value) - allowed)
    if unknown:
        raise ValueError(f"{where}: a state can't change {unknown} (only {sorted(allowed)})")
    glow = value.get('directional') if group == 'fog' else None
    if glow is not None:
        unknown = sorted(set(glow) - DIRECTIONAL_KEYS) if isinstance(glow, dict) else ['directional']
        if unknown:
            raise ValueError(f"{where}.directional: a state can't change {unknown} (only {sorted(DIRECTIONAL_KEYS)})")


def lighting_states(source, env):
    """Every lighting state's settings by name: Day, the environment as placed, then each one level.environment.states
    adds, as Day with the groups it changes. Stops on anything a state can't change, before the level is touched."""
    given = source.get('level', {}).get('environment', {}).get('states', {})
    states = {'Day': env}
    for name, changes in given.items():
        if name.lower() == 'day':
            raise ValueError('level.environment.states: Day is the environment itself; change it there')
        changes = {group: value for group, value in changes.items() if group != 'about'}
        for group, value in changes.items():
            check_state_group(f'level.environment.states.{name}.{group}', group, value)
        states[name] = merge(env, changes)
    return states


def color(values):
    r, g, b = values[:3]
    return unreal.LinearColor(r, g, b, 1.0)


def lighting_state(name, env):
    """One state as ALightingStates holds it (FLightingState)."""
    sun, fog, atmosphere = env['sun'], env['fog'], env['atmosphere']
    glow = {**NO_GLOW, **(fog.get('directional') or {})}
    return unreal.LightingState(
        name=name, sun_bearing=sun['azimuth'], sun_elevation=sun['elevation'], sun_intensity=sun['intensity'],
        sun_temperature=sun['temperature'], shadow_distance=sun['shadowDistance'], shadow_cascades=int(sun['cascades']),
        sky_intensity=env['sky']['intensity'], sky_luminance=color(atmosphere['skyLuminance']),
        ozone=atmosphere['ozone'], fog_density=fog['density'], fog_inscattering=color(fog['inscattering']),
        fog_directional_inscattering=color(glow['color']), fog_directional_exponent=glow['exponent'],
        fog_directional_start_distance=glow['startDistance'], backdrop_tint=color(env['backdrop']['tint']),
        cloud_tint=color(env['clouds']['tint']), exposure_bias=env['post']['exposureBias'])


def place_states(build, states, lights):
    """The level's lighting states beside the lights they drive (sun, sky light, atmosphere, fog, post volume)."""
    cls = unreal.load_class(None, LIGHTING_STATES)
    if cls is None:
        build.warn('no LightingStates class (build the game module first): the level gets no lighting states')
        return
    actor = build.place(cls, (0, 0, 2500), label='LightingStates', folder='Environment')
    actor.set_editor_property('states', [lighting_state(name, s) for name, s in states.items()])
    actor.set_editor_property('initial_state', 'Day')
    for prop, target in zip(('sun', 'sky_light', 'atmosphere', 'height_fog', 'post_volume'), lights):
        actor.set_editor_property(prop, target)
    build.log('lighting states: ' + ', '.join(states))


def place(build):
    """Places the sun, sky light, atmosphere, height fog, cloud dome, post process volume and the lighting states;
    returns the sky light component, which the build recaptures once everything else stands."""
    env = settings(build.source)
    states = lighting_states(build.source, env)
    sun_settings = env['sun']
    sun = build.place(unreal.DirectionalLight, (0, 0, 3000), label='Sun', folder='Environment')
    # A directional light shines along its forward axis: from the sun's bearing toward the opposite one, downward.
    sun.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-sun_settings['elevation'],
                                          yaw=(sun_settings['azimuth'] + 180.0) % 360.0), False)
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    # Movable: the lighting states turn it in the game.
    light.set_mobility(unreal.ComponentMobility.MOVABLE)
    for name, value in (('intensity', sun_settings['intensity']), ('use_temperature', True),
                        ('temperature', sun_settings['temperature']), ('atmosphere_sun_light', True),
                        ('cast_cloud_shadows', False),
                        ('dynamic_shadow_distance_movable_light', sun_settings['shadowDistance']),
                        ('dynamic_shadow_cascades', sun_settings['cascades'])):
        light.set_editor_property(name, value)

    sky = build.place(unreal.SkyLight, (0, 0, 2000), label='SkyLight', folder='Environment')
    sky_light = sky.get_component_by_class(unreal.SkyLightComponent)
    sky_light.set_mobility(unreal.ComponentMobility.MOVABLE)
    # Captured once when the level loads rather than every frame (0.12 ms on Medium): the sun only moves when a
    # lighting state switches, which recaptures it.
    for name, value in (('real_time_capture', False),
                        ('source_type', unreal.SkyLightSourceType.SLS_CAPTURED_SCENE),
                        ('intensity', env['sky']['intensity']), ('lower_hemisphere_is_black', False),
                        ('lower_hemisphere_color', color(env['sky']['lowerHemisphere']))):
        sky_light.set_editor_property(name, value)

    atmosphere_settings = env['atmosphere']
    atmosphere = build.place(unreal.SkyAtmosphere, (0, 0, atmosphere_settings.get('planetTop', 0.0)),
                             label='SkyAtmosphere', folder='Environment')
    air = atmosphere.get_component_by_class(unreal.SkyAtmosphereComponent)
    if 'planetTop' in atmosphere_settings:
        # A grounded area's planet top sits down on the plains past the canyon, so the horizon and the ground under
        # it take the plains' color rather than the island's void.
        air.set_editor_property('transform_mode', unreal.SkyAtmosphereTransformMode.PLANET_TOP_AT_COMPONENT_TRANSFORM)
    if 'groundAlbedo' in atmosphere_settings:
        r, g, b = (max(0, min(255, round(c * 255.0))) for c in atmosphere_settings['groundAlbedo'][:3])
        air.set_editor_property('ground_albedo', unreal.Color(r=r, g=g, b=b, a=255))
    # Ozone as a multiple of the engine's absorption, which is Earth's ozone layer (as FLightingState's Ozone).
    earth_ozone = unreal.get_default_object(unreal.SkyAtmosphereComponent).get_editor_property('other_absorption_scale')
    air.set_editor_property('sky_luminance_factor', color(atmosphere_settings['skyLuminance']))
    air.set_editor_property('other_absorption_scale', earth_ozone * atmosphere_settings['ozone'])

    fog_settings = env['fog']
    fog = build.place(unreal.ExponentialHeightFog, (0, 0, fog_settings['height']), label='HeightFog',
                      folder='Environment')
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    for name, value in (('fog_density', fog_settings['density']), ('fog_height_falloff', fog_settings['falloff']),
                        ('start_distance', fog_settings['startDistance']),
                        ('fog_max_opacity', fog_settings['maxOpacity']),
                        ('fog_cutoff_distance', fog_settings['cutoffDistance']),
                        ('fog_inscattering_luminance', color(fog_settings['inscattering'])),
                        # The sky's color correction is for the sky alone: the haze takes the sky's light without it
                        # (as FLightingTargets sets it for every state).
                        ('sky_atmosphere_ambient_contribution_color_scale',
                         color([1.0 / max(c, 0.01) for c in atmosphere_settings['skyLuminance'][:3]]))):
        fog_component.set_editor_property(name, value)
    directional = fog_settings.get('directional')
    if directional:
        # The haze glows toward the low sun.
        for name, value in (('directional_inscattering_luminance', color(directional['color'])),
                            ('directional_inscattering_exponent', directional.get('exponent', 4.0)),
                            ('directional_inscattering_start_distance', directional.get('startDistance', 10000.0))):
            fog_component.set_editor_property(name, value)

    # Painted clouds on a dome around the level (M_SkyClouds). Volumetric clouds cost Medium 2 ms and more, looking up
    # through their layer, and thinned out enough to be cheap they vanished. The dome is translucent and depth-tested,
    # so it must stand past anything on the horizon (a grounded area's backdrop) or it paints clouds over it.
    dome_mesh = unreal.load_asset(SKY_DOME)
    radius = max(dome_mesh.get_bounding_box().max.x, 1.0)
    dome = build.place(dome_mesh, (0, 0, 0), label='SkyClouds', folder='Environment',
                       scale=(env['clouds']['radius'] / radius,) * 3)
    sky_mesh = dome.static_mesh_component
    sky_mesh.set_material(0, unreal.load_asset(SKY_CLOUDS))
    sky_mesh.set_editor_property('cast_shadow', False)
    sky_mesh.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    post = build.place(unreal.PostProcessVolume, (0, 0, 0), label=f'{build.folder}Post', folder='Environment')
    post.set_editor_property('unbound', True)
    post_settings = post.get_editor_property('settings')
    # No outline material: the new style draws no ink lines.
    for name, value in (('auto_exposure_method', unreal.AutoExposureMethod.AEM_HISTOGRAM),
                        ('auto_exposure_bias', env['post']['exposureBias']),
                        # Exposure adapts only a little (EV100 0.3 to 1): shade under the trees stays shade instead of
                        # brightening to look like the open meadow.
                        ('auto_exposure_min_brightness', 0.3), ('auto_exposure_max_brightness', 1.0),
                        ('bloom_intensity', 0.6), ('vignette_intensity', 0.3), ('film_slope', 0.88),
                        ('film_toe', 0.55), ('color_saturation', unreal.Vector4(1.05, 1.05, 1.05, 1.0))):
        post_settings.set_editor_property(name, value)
        post_settings.set_editor_property(f'override_{name}', True)
    post.set_editor_property('settings', post_settings)

    place_states(build, states, (sun, sky, atmosphere, fog, post))
    return sky_light
