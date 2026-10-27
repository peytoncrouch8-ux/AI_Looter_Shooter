"""Builds /Game/Maps/Lvl_MenuTeropa, the main menu's backdrop: Teropa from orbit (Docs/Story.md), turning slowly in the
right 60% of the screen, the menu's column and title in the left 40% (OPENING's UI; the contract's split, 2026-10-27).

What it places (each tagged MenuTeropaBuild, in the MenuTeropa outliner folder; a rebuild replaces only those):
  MenuView    a camera at the origin looking along +X (FOV 50 across). ALooterMenuPlayerController looks through it
              (tag MenuView), with a slow drift and the cursor's parallax; the planet's framing is worked out here for it.
  Teropa      the engine sphere as a proxy (tag MenuPlanet) wearing MI_Planet_Teropa: the planet's centre at PLANET_NDC on
              screen, its left limb at PLANET_LIMB_X (right of the column's edge, NDC -0.2 = 40% of the width). Its
              radius is PLANET_RADIUS; the proxy is bigger by the halo and PROXY_MARGIN.
  Moon        the same with MI_Planet_Moon (tag MenuMoon), small, over the planet's upper left shoulder, three times as far.
  Stars       the engine sphere, huge, round the camera (tag MenuStars), wearing MI_Starfield_Teropa.
  MenuPost    an unbound post volume: fixed exposure (nothing in space to adapt to; the planet shouldn't pump as clouds
              pass), bloom for the sun's side and the lights, a vignette that darkens the column's side, no local
              exposure, no motion blur, no lens flare.
  PlayerStart (the game mode needs none, but a level without one warns).
The level's World Settings take ALooterMenuGameMode, so this map is the menu however it is opened, and no precomputed
lighting (it has no lights: every material is unlit).

The scene's numbers go into the instances too (build_menu_materials.py made them): PlanetRadius, SunDirection, North,
East, Facing on both planets, SunDirection, SkyNorth, SkyEast on the stars, so the sun's side, the glare and the
galaxy's band agree with this camera.

Run in the open editor, nothing playing, no other level with unsaved changes, after build_menu_materials.py:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_menu_teropa.py"
Then Config/DefaultEngine.ini's GameDefaultMap may name /Game/Maps/Lvl_MenuTeropa (Main's switch; see the job's README).
"""
import math

import unreal

LEVEL = '/Game/Maps/Lvl_MenuTeropa'
TAG = 'MenuTeropaBuild'
FOLDER = 'MenuTeropa'
SPHERE = '/Engine/BasicShapes/Sphere.Sphere'
SPHERE_RADIUS = 50.0  # the engine sphere's radius, cm
MATERIALS = '/Game/Art/Materials/Menu'
MENU_GAME_MODE = '/Script/AI_Looter_Shooter.LooterMenuGameMode'
# Tags the game reads (ALooterMenuPlayerController's MenuScene names; the test Looter.Menu.Teropa.Scene).
VIEW_TAG, PLANET_TAG, MOON_TAG, STARS_TAG = 'MenuView', 'MenuPlanet', 'MenuMoon', 'MenuStars'

# The camera: horizontal field of view (Unreal keeps it across aspect ratios, so these screen x numbers hold on any
# screen); the framing is worked out for 16:9.
FOV = 50.0
ASPECT = 16.0 / 9.0
# The planet: its centre on screen (NDC: -1 left .. 1 right, -1 bottom .. 1 top), its left limb's x, its radius (cm).
# Limb at -0.05 leaves the column (left of -0.2) its own dark sky, with a little air between; the right limb lands
# near 0.93, so the whole disc shows across and nearly fills the height.
PLANET_NDC = (0.42, -0.06)
PLANET_LIMB_X = -0.05
PLANET_RADIUS = 20000.0
# Its north pole tipped toward the screen's right and toward the eye (degrees): the axis reads, the polar ice shows.
AXIS_TILT = 18.0
AXIS_LEAN = 12.0
# The moon: on screen, its apparent radius (deg) and how much farther than the planet.
MOON_NDC = (0.0, 0.7)
MOON_RADIUS_DEG = 1.2
MOON_FARTHER = 3.0
# The sun, in the camera's own axes (forward, right, up): from the right, a little up and a little behind the planet,
# so the terminator runs just right of the disc's middle and the night side (with the lights) faces the menu.
SUN = (0.2, 0.9, 0.38)
# The galaxy's band: a great circle through these two screen points (NDC), left low to right high.
BAND_POINTS = ((-1.0, -0.35), (1.0, 0.95))
# The proxies: the engine sphere's flat faces sit inside its 50 cm vertices by up to about 2%; the proxy is this much
# bigger than what its material draws (the planet and its halo), so no face cuts the limb or the halo.
PROXY_MARGIN = 1.05
HALO_SHARE = 0.035  # M_Planet's AtmosphereHeight on MI_Planet_Teropa (the halo's thickness, a share of the radius)
# The stars' sphere, in planet distances: far behind everything.
STARS_FARTHER = 12.0
# Post: EV100 0 fixed (the masters' emissive is made for it), bloom, vignette.
POST = dict(auto_exposure_method=unreal.AutoExposureMethod.AEM_HISTOGRAM, auto_exposure_bias=0.0,
            auto_exposure_min_brightness=0.0, auto_exposure_max_brightness=0.0, bloom_intensity=0.8,
            vignette_intensity=0.45, motion_blur_amount=0.0, lens_flare_intensity=0.0,
            local_exposure_highlight_contrast_scale=1.0, local_exposure_shadow_contrast_scale=1.0,
            ambient_occlusion_intensity=0.0, color_saturation=unreal.Vector4(1.05, 1.05, 1.05, 1.0))

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
mel = unreal.MaterialEditingLibrary


def log(message):
    unreal.log(f'Lvl_MenuTeropa: {message}')


# --- Vector math on tuples (the camera's own axes; no engine types needed) ---

def add(*vs):
    return tuple(sum(c) for c in zip(*vs))


def scale(v, s):
    return tuple(c * s for c in v)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def unit(v):
    length = math.sqrt(dot(v, v))
    return scale(v, 1.0 / length)


# The camera looks along +X, unrolled: right +Y, up +Z (Unreal's axes).
FORWARD, RIGHT, UP = (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)
TAN_H = math.tan(math.radians(FOV * 0.5))
TAN_V = TAN_H / ASPECT


def screen_ray(ndc):
    """The direction through a screen point (NDC)."""
    return unit(add(FORWARD, scale(RIGHT, ndc[0] * TAN_H), scale(UP, ndc[1] * TAN_V)))


def framing():
    """Where the planet goes: its centre along the ray through PLANET_NDC, at the distance where its angular radius
    puts the left limb at PLANET_LIMB_X (the limb's sideways angle from the centre, shrunk by the centre's elevation)."""
    ray = screen_ray(PLANET_NDC)
    yaw_centre = math.atan(PLANET_NDC[0] * TAN_H)
    yaw_limb = math.atan(PLANET_LIMB_X * TAN_H)
    pitch = math.asin(dot(ray, UP))
    alpha = (yaw_centre - yaw_limb) * math.cos(pitch)
    distance = PLANET_RADIUS / math.sin(alpha)
    return ray, distance, math.degrees(alpha)


def limb_ndc_x(centre, radius):
    """The leftmost screen x of a sphere's outline (exact: its silhouette circle, sampled)."""
    dist = math.sqrt(dot(centre, centre))
    axis = scale(centre, 1.0 / dist)
    ring_centre = scale(axis, dist - radius * radius / dist)
    ring_radius = radius * math.sqrt(1.0 - (radius / dist) ** 2)
    u = unit(cross(axis, UP))
    v = cross(u, axis)
    best = 10.0
    for k in range(360):
        a = math.radians(k)
        p = add(ring_centre, scale(u, ring_radius * math.cos(a)), scale(v, ring_radius * math.sin(a)))
        best = min(best, dot(p, RIGHT) / dot(p, FORWARD) / TAN_H)
    return best


# --- The level ---

def open_level():
    if levels.is_in_play_in_editor():
        raise RuntimeError('a play session is running: stop it first (hands off while the user plays)')
    world = unreal.EditorLevelLibrary.get_editor_world()
    if world.get_path_name().split('.')[0] != LEVEL:
        unsaved = [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()
                   if not p.get_name().startswith('/Temp/')]
        if unsaved:
            raise RuntimeError(f'unsaved changes in {unsaved}: save or discard them first')
        if unreal.EditorAssetLibrary.does_asset_exist(LEVEL):
            unreal.EditorLoadingAndSavingUtils.load_map(LEVEL)
        else:
            levels.new_level(LEVEL)
            log('made')
    built = [a for a in actors.get_all_level_actors() if unreal.Name(TAG) in a.tags]
    if built:
        actors.destroy_actors(built)
        log(f'removed {len(built)} actors from the last build')


def place(what, location, label, tags=(), scale_by=None, rotation=None):
    loc = unreal.Vector(*location)
    rot = rotation or unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0)
    if isinstance(what, unreal.StaticMesh):
        actor = actors.spawn_actor_from_object(what, loc, rot)
    else:
        actor = actors.spawn_actor_from_class(what, loc, rot)
    if scale_by is not None:
        actor.set_actor_scale3d(unreal.Vector(scale_by, scale_by, scale_by))
    actor.set_editor_property('tags', [unreal.Name(TAG)] + [unreal.Name(t) for t in tags])
    actor.set_folder_path(FOLDER)
    actor.set_actor_label(label)
    return actor


def sphere(label, centre, drawn_radius, material, tags):
    """The engine sphere as a proxy for a body drawn by its material (no collision, no shadow, never culled by size)."""
    mesh = unreal.load_asset(SPHERE)
    actor = place(mesh, centre, label, tags, scale_by=drawn_radius * PROXY_MARGIN / SPHERE_RADIUS)
    component = actor.static_mesh_component
    component.set_material(0, material)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    component.set_editor_property('cast_shadow', False)
    component.set_editor_property('mobility', unreal.ComponentMobility.STATIC)
    return actor


def vec(v):
    return unreal.LinearColor(float(v[0]), float(v[1]), float(v[2]), 0.0)


def set_scene(mi, scalars, vectors):
    for key, value in scalars.items():
        mel.set_material_instance_scalar_parameter_value(mi, key, float(value))
    for key, value in vectors.items():
        mel.set_material_instance_vector_parameter_value(mi, key, vec(value))
    mel.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi, only_if_is_dirty=False)


def planet_frame(ray, tilt, lean):
    """North, East and Facing (toward the camera) for a body along ray: north tipped tilt degrees toward the screen's
    right and lean degrees toward the eye."""
    facing = scale(ray, -1.0)
    north = unit(add(scale(UP, math.cos(math.radians(tilt))), scale(RIGHT, math.sin(math.radians(tilt)))))
    north = unit(add(scale(north, math.cos(math.radians(lean))), scale(facing, math.sin(math.radians(lean)))))
    return north, RIGHT, facing


def build():
    for name in ('MI_Planet_Teropa', 'MI_Planet_Moon', 'MI_Starfield_Teropa'):
        if not unreal.EditorAssetLibrary.does_asset_exist(f'{MATERIALS}/{name}'):
            raise RuntimeError(f'{MATERIALS}/{name} is missing: run build_menu_materials.py first')
    open_level()
    planet_mi = unreal.load_asset(f'{MATERIALS}/MI_Planet_Teropa')
    moon_mi = unreal.load_asset(f'{MATERIALS}/MI_Planet_Moon')
    stars_mi = unreal.load_asset(f'{MATERIALS}/MI_Starfield_Teropa')

    camera = place(unreal.CameraActor, (0.0, 0.0, 0.0), 'MenuView', (VIEW_TAG,))
    lens = camera.camera_component
    lens.set_editor_property('field_of_view', FOV)
    lens.set_editor_property('constrain_aspect_ratio', False)

    ray, distance, alpha = framing()
    centre = scale(ray, distance)
    sun = unit(add(scale(FORWARD, SUN[0]), scale(RIGHT, SUN[1]), scale(UP, SUN[2])))
    sphere('Teropa', centre, PLANET_RADIUS * (1.0 + HALO_SHARE), planet_mi, (PLANET_TAG,))
    north, east, facing = planet_frame(ray, AXIS_TILT, AXIS_LEAN)
    set_scene(planet_mi, {'PlanetRadius': PLANET_RADIUS},
              {'SunDirection': sun, 'North': north, 'East': east, 'Facing': facing})

    moon_ray = screen_ray(MOON_NDC)
    moon_distance = distance * MOON_FARTHER
    moon_radius = moon_distance * math.tan(math.radians(MOON_RADIUS_DEG))
    sphere('Moon', scale(moon_ray, moon_distance), moon_radius, moon_mi, (MOON_TAG,))
    moon_north, moon_east, moon_facing = planet_frame(moon_ray, 0.0, 0.0)
    set_scene(moon_mi, {'PlanetRadius': moon_radius},
              {'SunDirection': sun, 'North': moon_north, 'East': moon_east, 'Facing': moon_facing})

    stars_radius = distance * STARS_FARTHER
    sphere('Stars', (0.0, 0.0, 0.0), stars_radius / PROXY_MARGIN, stars_mi, (STARS_TAG,))
    band_normal = unit(cross(screen_ray(BAND_POINTS[0]), screen_ray(BAND_POINTS[1])))
    set_scene(stars_mi, {}, {'SunDirection': sun, 'SkyNorth': band_normal, 'SkyEast': unit(cross(band_normal, FORWARD))})

    post = place(unreal.PostProcessVolume, (0.0, 0.0, 0.0), 'MenuPost')
    post.set_editor_property('unbound', True)
    settings = post.get_editor_property('settings')
    for name, value in POST.items():
        try:
            settings.set_editor_property(name, value)
            settings.set_editor_property(f'override_{name}', True)
        except Exception as error:  # a renamed setting: the rest still apply
            unreal.log_warning(f'Lvl_MenuTeropa: post setting {name} not set ({error})')
    post.set_editor_property('settings', settings)
    place(unreal.PlayerStart, (0.0, 0.0, 0.0), 'PlayerStart')

    world = unreal.EditorLevelLibrary.get_editor_world()
    world_settings = world.get_world_settings()
    world_settings.set_editor_property('default_game_mode', unreal.load_class(None, MENU_GAME_MODE))
    try:
        world_settings.set_editor_property('force_no_precomputed_lighting', True)
    except Exception as error:
        unreal.log_warning(f'Lvl_MenuTeropa: force_no_precomputed_lighting not set ({error})')

    limb = limb_ndc_x(centre, PLANET_RADIUS)
    log(f'planet {PLANET_RADIUS / 100:.0f} m round, {distance / 100:.0f} m out (angular radius {alpha:.1f} deg), '
        f'left limb at NDC {limb:.3f} (the column ends at -0.2); moon {moon_radius / 100:.0f} m, {moon_distance / 100:.0f} m out')
    if limb < -0.2:
        unreal.log_warning('Lvl_MenuTeropa: the planet reaches into the menu column')
    if not levels.save_current_level():
        raise RuntimeError('the level did not save')
    log('built and saved')


build()
