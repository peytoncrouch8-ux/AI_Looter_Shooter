"""Photographs heroes holding guns in third person, for contact sheets (Tools/heroguns.ps1 runs it and tiles them;
Docs/Pipeline.md "The hero pick", the third-person hold). Runs in a play session with the guns in slots 1-3.
Usage: Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/hero_guns.py @" -Until "HEROGUNS (done|failed)"
The arguments are read from Saved/Screenshots/HeroGuns/args.txt (the console's box takes only so much), one line:
  <tag> <heroes Ellis,Odessa> <slots 1,2,3> <states hip,aim,reload,sprint,crouch,crouchaim> <views 330,270,180>
A view is the camera's place round the character in degrees from behind (330 behind the right shoulder, 270 the right
side, 180 the front, 90 the left side; -1 the game's own camera). Each picture lands in Saved/Screenshots/HeroGuns/<tag>/
<hero>_<slot>_<state>_<view>.png. The camera is the player's third-person camera moved each frame onto the view round
the chest (aiming works only in third person, not the front view); a run started again stops the one before."""
import builtins
import math
import os
import sys

import unreal

SAVED = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())
ROOT = os.path.join(SAVED, 'Screenshots', 'HeroGuns')
args = sys.argv[1:]
if args and args[0] == '@':
    with open(os.path.join(ROOT, 'args.txt')) as handle:
        args = handle.read().split()
tag = args[0] if args else 'run'
heroes = (args[1] if len(args) > 1 else 'Ellis').split(',')
slots = [int(s) for s in (args[2] if len(args) > 2 else '1,2,3').split(',')]
states = (args[3] if len(args) > 3 else 'hip,aim').split(',')
views = [float(y) for y in (args[4] if len(args) > 4 else '330,270,180').split(',')]
OUT = os.path.join(ROOT, tag)
os.makedirs(OUT, exist_ok=True)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(world, 0)
pawn = pc.get_controlled_pawn()
view = pawn.get_component_by_class(unreal.PlayerViewComponent)
manager = pawn.get_component_by_class(unreal.WeaponManagerComponent)
figure = pawn.get_component_by_class(unreal.BodyFigureComponent)
camera = next(c for c in pawn.get_components_by_class(unreal.CameraComponent) if c.get_name() == 'ThirdPersonCamera')
boom = camera.get_attach_parent()
socket = camera.get_attach_socket_name()
inputs = next((s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == world), None)
start_loc = pawn.get_actor_location()
start_rot = pc.get_control_rotation()


def action(name):
    """A code-built input action (Crouch, Sprint: no asset) of this play session, or a Content one (Move)."""
    found = unreal.load_asset(f'/Game/Input/Actions/IA_{name}')
    if found is None:
        def ours(a):
            try:
                return a.get_outer().get_world() == world
            except Exception:
                return False
        found = next((a for a in unreal.ObjectIterator(unreal.InputAction) if a.get_name() == f'IA_{name}' and ours(a)), None)
    return found


MOVE, SPRINT, CROUCH = action('Move'), action('Sprint'), action('Crouch')
cam = {'view': None}
held = {'run': False}
plan = []


def place_camera():
    """The camera onto its view round the chest (between the chest and the right hand), level with the aim's facing."""
    if cam['view'] is None:
        return
    a = figure.get_socket_location('spine_05')
    b = figure.get_socket_location('hand_r')
    target = unreal.Vector((a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2)
    behind = cam['view'] < 45.0 or cam['view'] > 315.0
    yaw = math.radians(pc.get_control_rotation().yaw + cam['view'])
    pos = unreal.Vector(target.x - math.cos(yaw) * 115.0, target.y - math.sin(yaw) * 115.0, target.z + (24.0 if behind else 8.0))
    desired = unreal.Transform(pos, unreal.MathLibrary.find_look_at_rotation(pos, target), unreal.Vector(1, 1, 1))
    sock = boom.get_socket_transform(socket, unreal.RelativeTransformSpace.RTS_WORLD)
    camera.set_relative_transform(unreal.MathLibrary.make_relative_transform(desired, sock), False, True)


def set_view(angle):
    def f():
        cam['view'] = angle if angle is not None and angle >= 0 else None
        if cam['view'] is None:
            camera.set_relative_transform(unreal.Transform(), False, True)
    return f


def shot(name):
    def f():
        unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, os.path.join(OUT, f'{name}.png'))
    return f


def call(fn):
    def f():
        fn()
    return f


def weapon_call(method):
    def f():
        weapon = manager.get_active_weapon()
        if weapon:
            getattr(weapon, method)()
    return f


def back_to_start():
    held['run'] = False
    pawn.set_actor_location(start_loc, False, True)
    pc.set_control_rotation(start_rot)


def tap(input_action):
    if input_action:
        inputs.inject_input_vector_for_action(input_action, unreal.Vector(1, 0, 0), [], [])


def views_for(prefix):
    for angle in views:
        plan.append((0.12, set_view(angle)))
        plan.append((0.3, shot(f'{prefix}_{int(angle)}')))


for hero in heroes:
    plan.append((2.0, call(lambda h=hero: unreal.SystemLibrary.execute_console_command(world, f'Looter.Hero {h}'))))
    for slot in slots:
        plan.append((1.8, call(lambda s=slot: manager.equip_slot(s - 1))))
        for state in states:
            prefix = f'{hero}_{slot}_{state}'
            crouched = state.startswith('crouch')
            aimed = state.endswith('aim')
            if crouched:
                plan.append((1.0, call(lambda: tap(CROUCH))))
            if aimed:
                plan.append((0.9, call(view.handle_aim_pressed)))
            if state == 'reload':
                # Each view its own reload (a shot fired first, so there's something to load), photographed 0.6 s in.
                for angle in views:
                    plan += [(0.2, weapon_call('start_fire')), (0.2, weapon_call('stop_fire')), (0.6, weapon_call('reload')),
                             (0.12, set_view(angle)), (2.6, shot(f'{prefix}_{int(angle)}'))]
            elif state == 'sprint':
                plan.append((1.2, call(lambda: held.update(run=True))))
                views_for(prefix)
                plan.append((1.0, call(back_to_start)))
            else:
                views_for(prefix)
            if aimed:
                plan.append((0.6, call(view.handle_aim_released)))
            if crouched:
                plan.append((1.0, call(lambda: tap(CROUCH))))
plan.append((0.2, set_view(None)))
progress = {'i': 0, 'wait': 0.0, 'handle': None}


def finish(words):
    unreal.unregister_slate_post_tick_callback(progress['handle'])
    held['run'] = False
    camera.set_relative_transform(unreal.Transform(), False, True)
    unreal.log(words)


def tick(dt):
    try:
        if held['run']:
            if MOVE:
                inputs.inject_input_vector_for_action(MOVE, unreal.Vector(1, 0, 0), [], [])
            if SPRINT:
                inputs.inject_input_vector_for_action(SPRINT, unreal.Vector(1, 0, 0), [], [])
        place_camera()
        progress['wait'] -= dt
        if progress['wait'] > 0.0:
            return
        if progress['i'] >= len(plan):
            finish(f'HEROGUNS done {tag}: {OUT}')
            return
        wait, fn = plan[progress['i']]
        progress['i'] += 1
        fn()
        progress['wait'] = wait
    except Exception as error:  # noqa: BLE001 (the console shows only the log: say what broke)
        finish(f'HEROGUNS failed: {error!r}')


earlier = getattr(builtins, '_hero_guns_handle', None)
if earlier is not None:
    try:
        unreal.unregister_slate_post_tick_callback(earlier)
    except Exception:
        pass
unreal.log(f'HEROGUNS start {tag}: {len(plan)} steps, about {sum(p[0] for p in plan):.0f} s')
progress['handle'] = unreal.register_slate_post_tick_callback(tick)
builtins._hero_guns_handle = progress['handle']
