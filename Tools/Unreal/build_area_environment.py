"""An area's light, sky and fog, for build_area.py: the tutorial island's warm afternoon by default, or what layout.json
level.environment changes (Ransom's Rest's golden hour, step 5a of Docs/Areas/RansomsRest.md). Every key is optional:

  "environment": {
    "sun": {"azimuth": 247.5, "elevation": 15, "intensity": 7, "temperature": 4300},
    "sky": {"intensity": 1.0, "lowerHemisphere": [0.3, 0.25, 0.15]},
    "fog": {"height": -7000, "density": 0.02, "falloff": 0.05, "startDistance": 6000, "maxOpacity": 1.0,
            "inscattering": [0.4, 0.33, 0.27],
            "directional": {"color": [1.0, 0.7, 0.4], "exponent": 8, "startDistance": 6000}},
    "atmosphere": {"planetTop": -9000, "groundAlbedo": [0.45, 0.36, 0.22]},
    "clouds": {"radius": 1500000}
  }

The sun is placed by compass: its azimuth is the bearing it stands at (0 north, 90 east, 247.5 west-southwest) and its
elevation how high it stands over the horizon. Lengths are cm, colors linear RGB. Unreal's +X is north and +Y east.
"""
import unreal

# The sky's clouds: a dome around the level with the painted cloud material.
SKY_DOME = '/Engine/EngineSky/SM_SkySphere'
SKY_CLOUDS = '/Game/Art/Materials/Masters/M_SkyClouds'

# The tutorial island's afternoon: the light from the west-northwest, so the view from the spawn (looking northeast) is
# lit from the side.
DEFAULTS = {
    'sun': {'azimuth': 292.0, 'elevation': 38.0, 'intensity': 7.0, 'temperature': 5300.0,
            # Cascaded shadows (Low and Medium) reach 100 m before the preset's scale (70 m on Medium).
            'shadowDistance': 10000.0, 'cascades': 2},
    'sky': {'intensity': 1.2, 'lowerHemisphere': [0.26, 0.30, 0.18]},  # light bouncing off the meadow
    # Blue, filling the void under the island.
    'fog': {'height': -2000.0, 'density': 0.03, 'falloff': 0.12, 'startDistance': 3000.0, 'maxOpacity': 0.85,
            'inscattering': [0.20, 0.29, 0.44], 'directional': None},
    'atmosphere': None,
    # 1 km: the island's horizon is the sky itself.
    'clouds': {'radius': 100000.0},
}


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


def color(values):
    r, g, b = values[:3]
    return unreal.LinearColor(r, g, b, 1.0)


def place(build):
    """Places the sun, sky light, atmosphere, height fog, cloud dome and post process volume; returns the sky light
    component, which the build recaptures once everything else stands."""
    env = settings(build.source)
    sun_settings = env['sun']
    sun = build.place(unreal.DirectionalLight, (0, 0, 3000), label='Sun', folder='Environment')
    # A directional light shines along its forward axis: from the sun's bearing toward the opposite one, downward.
    sun.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-sun_settings['elevation'],
                                          yaw=(sun_settings['azimuth'] + 180.0) % 360.0), False)
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
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
    # Captured once when the level loads rather than every frame (0.12 ms on Medium): the sun never moves.
    for name, value in (('real_time_capture', False),
                        ('source_type', unreal.SkyLightSourceType.SLS_CAPTURED_SCENE),
                        ('intensity', env['sky']['intensity']), ('lower_hemisphere_is_black', False),
                        ('lower_hemisphere_color', color(env['sky']['lowerHemisphere']))):
        sky_light.set_editor_property(name, value)

    atmosphere = env['atmosphere']
    if atmosphere:
        # A grounded area's planet top sits down on the plains past the canyon, so the horizon and the ground under
        # it take the plains' color rather than the island's void.
        actor = build.place(unreal.SkyAtmosphere, (0, 0, atmosphere.get('planetTop', 0.0)), label='SkyAtmosphere',
                            folder='Environment')
        component = actor.get_component_by_class(unreal.SkyAtmosphereComponent)
        component.set_editor_property('transform_mode', unreal.SkyAtmosphereTransformMode.PLANET_TOP_AT_COMPONENT_TRANSFORM)
        if 'groundAlbedo' in atmosphere:
            r, g, b = (max(0, min(255, round(c * 255.0))) for c in atmosphere['groundAlbedo'][:3])
            component.set_editor_property('ground_albedo', unreal.Color(r=r, g=g, b=b, a=255))
    else:
        build.place(unreal.SkyAtmosphere, (0, 0, 0), label='SkyAtmosphere', folder='Environment')

    fog_settings = env['fog']
    fog = build.place(unreal.ExponentialHeightFog, (0, 0, fog_settings['height']), label='HeightFog',
                      folder='Environment')
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    for name, value in (('fog_density', fog_settings['density']), ('fog_height_falloff', fog_settings['falloff']),
                        ('start_distance', fog_settings['startDistance']),
                        ('fog_max_opacity', fog_settings['maxOpacity']),
                        ('fog_inscattering_luminance', color(fog_settings['inscattering']))):
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
                        ('auto_exposure_bias', 0.4),
                        # Exposure adapts only a little (EV100 0.3 to 1): shade under the trees stays shade instead of
                        # brightening to look like the open meadow.
                        ('auto_exposure_min_brightness', 0.3), ('auto_exposure_max_brightness', 1.0),
                        ('bloom_intensity', 0.6), ('vignette_intensity', 0.3), ('film_slope', 0.88),
                        ('film_toe', 0.55), ('color_saturation', unreal.Vector4(1.05, 1.05, 1.05, 1.0))):
        post_settings.set_editor_property(name, value)
        post_settings.set_editor_property(f'override_{name}', True)
    post.set_editor_property('settings', post_settings)
    return sky_light
