"""Play-check helper for a running play session (PIE): moves the player about so a mission can be played through without
walking every road. Lines start with PLAY so console.ps1 -Until 'PLAY' can wait for them.
Usage: Tools\\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/play.py <command> ..." -Until PLAY
  goto <tag|name> [distance cm] [yaw offset deg]  stand in front of the nearest actor tagged so (or whose name or label
                                                  holds the word), on the floor, looking at it; tries other sides when
                                                  the front is blocked or has no floor
  at <x> <y> <z> [yaw]                            stand at a point (z = where to look for the floor from)
  find <word>                                     actors whose name, label or tags hold the word, with where they stand
  where                                           where the player stands and looks
  kill [radius m]                                 kills every living creature within the radius (20 m), as the player's
                                                  shots would, so a Clear objective counts them
  look <yaw> [pitch]                              turns the player's view
  press <action> [seconds] [x] [y]                feeds IA_<action> (Interact, Fire, Jump, Reload, Move) as a tap, or
                                                  held for the seconds given; needs no window focus
  fight [seconds] [range m]                       aims at the nearest living creature in sight and holds Fire, for
                                                  the seconds given (20), reloading when nothing is in sight and
                                                  keeping the ammo topped up
  ammo                                            fills every ammo pool
  guns                                            the gun in hand and every slot
  equip <slot 1-3>                                takes a slot's gun in hand (the number keys don't reach the game
                                                  through input.ps1)
  view [times]                                    cycles the view as F5 does (first -> third -> front -> first)
  inventory [close]                               opens (or closes) the inventory, as Tab does
  lookat <tag|name>                               turns the view onto an actor from where the player stands
  what [yaw offset] [pitch offset]                what the view's middle meets: actor, label, component, mesh

Never set_editor_property on anything in the play session (2026-10-14): a changed value fires the editor's change notices,
and a component's notice reruns its actor's construction script. On the player that rebuilt every Blueprint component, the
weapon manager among them: all guns, the backpack and the ammo vanished, twice. Call the game's own functions instead."""
import math
import sys

import unreal

TRACE = getattr(unreal.TraceTypeQuery, "ECC_VISIBILITY", None) or unreal.TraceTypeQuery.TRACE_TYPE_QUERY1


def say(text):
    unreal.log(f'PLAY {text}')


def game_world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


def fmt(v):
    return f'({v.x:.0f}, {v.y:.0f}, {v.z:.0f})'


def floor_under(world, x, y, top, bottom, ignore):
    """The first thing that blocks the player straight down from top, or None."""
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, top), unreal.Vector(x, y, bottom), TRACE, False,
                                                 ignore, unreal.DrawDebugTrace.NONE, True)
    return None if hit is None else hit.to_tuple()[5]


def blocked(world, start, end, ignore):
    """What blocks the line, or None."""
    hit = unreal.SystemLibrary.line_trace_single(world, start, end, TRACE, False, ignore, unreal.DrawDebugTrace.NONE, True)
    return None if hit is None else hit.to_tuple()[9]


def matches(world, word):
    tagged = list(unreal.GameplayStatics.get_all_actors_with_tag(world, word))
    if tagged:
        return tagged
    low = word.lower()
    found = []
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        label = actor.get_actor_label() if hasattr(actor, 'get_actor_label') else ''
        if low in actor.get_name().lower() or low in label.lower():
            found.append(actor)
    return found


def player(world):
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    controller = unreal.GameplayStatics.get_player_controller(world, 0)
    if pawn is None or controller is None:
        say('no player pawn: start the game first')
        return None, None
    return pawn, controller


def carried(pawn):
    """The player's weapon manager and the guns it carries (slots and backpack), to see that a move kept them."""
    manager = pawn.get_component_by_class(unreal.WeaponManagerComponent)
    if manager is None:
        return None, 0
    return manager, len([w for w in manager.get_weapons() if w]) + len(manager.get_backpack())


def place(world, pawn, controller, spot, look_at=None, yaw=None, pitch=0.0):
    before = carried(pawn)
    capsule = pawn.get_component_by_class(unreal.CapsuleComponent)
    half = capsule.get_scaled_capsule_half_height() if capsule else 80.0
    pawn.set_actor_location(unreal.Vector(spot.x, spot.y, spot.z + half + 5.0), False, True)
    movement = pawn.get_component_by_class(unreal.CharacterMovementComponent)
    if movement:
        # Stopped dead, a hit's knock-back and launch with it, through the game's own calls: never set_editor_property
        # (see the top: on a moving player it rebuilt the pawn and took every gun).
        movement.stop_movement_immediately()
        movement.clear_accumulated_forces()
    # The same component keeps the same Python wrapper (they're cached per object), so a rebuilt one isn't the same.
    after = carried(pawn)
    if before[0] is not None and (after[0] is not before[0] or after[1] < before[1]):
        say(f'WARNING the move lost guns: {before[1]} carried before, {after[1]} after'
            f'{"; the weapon manager is a new one (the pawn was rebuilt)" if after[0] is not before[0] else ""}')
    if look_at is not None:
        eye = unreal.Vector(spot.x, spot.y, spot.z + half * 1.75)
        d = look_at - eye
        yaw = math.degrees(math.atan2(d.y, d.x))
        pitch = math.degrees(math.atan2(d.z, math.hypot(d.x, d.y)))
    controller.set_control_rotation(unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw or 0.0))


def aim_point(target):
    """Where to look on a target: the middle of its static meshes (a door's leaf, a crate), else where it stands."""
    boxes = []
    for mesh in target.get_components_by_class(unreal.StaticMeshComponent):
        if mesh.static_mesh is None or not mesh.is_visible():
            continue
        origin, extent, _ = unreal.SystemLibrary.get_component_bounds(mesh)
        if extent.length() < 400.0:
            boxes.append(origin)
    if not boxes:
        return target.get_actor_location()
    return unreal.Vector(sum(b.x for b in boxes) / len(boxes), sum(b.y for b in boxes) / len(boxes),
                         sum(b.z for b in boxes) / len(boxes))


def goto(world, word, distance=180.0, yaw_offset=0.0):
    pawn, controller = player(world)
    if pawn is None:
        return
    targets = matches(world, word)
    if not targets:
        say(f'goto: nothing tagged or named "{word}"')
        return
    here = pawn.get_actor_location()
    target = min(targets, key=lambda a: (a.get_actor_location() - here).length())
    base = target.get_actor_location()
    forward = target.get_actor_forward_vector()
    start_yaw = math.degrees(math.atan2(forward.y, forward.x)) + yaw_offset
    ignore = [pawn]
    first = None
    for turn in (0, 45, -45, 90, -90, 135, -135, 180):
        a = math.radians(start_yaw + turn)
        x, y = base.x + math.cos(a) * distance, base.y + math.sin(a) * distance
        # From a little over the target down, so a porch roof over a door isn't taken for the floor.
        floor = floor_under(world, x, y, base.z + 60.0, base.z - 600.0, ignore)
        if floor is None:
            continue
        first = first or (turn, floor)
        eye = unreal.Vector(x, y, floor.z + 140.0)
        hit = unreal.SystemLibrary.line_trace_single(world, eye, base, TRACE, False, ignore, unreal.DrawDebugTrace.NONE, True)
        if hit is not None:
            t = hit.to_tuple()
            # A door or frame right at the target doesn't count as in the way.
            if t[9] != target and (t[5] - base).length() > 60.0:
                continue
        place(world, pawn, controller, floor, look_at=aim_point(target))
        say(f'goto {target.get_name()} at {fmt(base)}: standing {distance:.0f} cm off at {turn:+d} deg, floor {fmt(floor)}')
        return
    if first:
        place(world, pawn, controller, first[1], look_at=aim_point(target))
        say(f'goto {target.get_name()} at {fmt(base)}: every side blocked; standing at {first[0]:+d} deg, floor {fmt(first[1])}')
        return
    say(f'goto {target.get_name()} at {fmt(base)}: no floor round it within 6 m down; not moved')


def at(world, x, y, z, yaw=0.0):
    pawn, controller = player(world)
    if pawn is None:
        return
    floor = floor_under(world, x, y, z, z - 2000.0, [pawn]) or unreal.Vector(x, y, z)
    place(world, pawn, controller, floor, yaw=yaw)
    say(f'at {fmt(floor)} yaw {yaw:.0f}')


def find(world, word):
    low = word.lower()
    shown = 0
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        label = actor.get_actor_label() if hasattr(actor, 'get_actor_label') else ''
        tags = [str(t) for t in actor.tags]
        if low in actor.get_name().lower() or low in label.lower() or any(low in t.lower() for t in tags):
            say(f'find {actor.get_name()} [{label}] {actor.get_class().get_name()} at {fmt(actor.get_actor_location())} '
                f'yaw {actor.get_actor_rotation().yaw:.0f} tags {tags}')
            shown += 1
            if shown >= 40:
                say('find: first 40 shown')
                break
    if not shown:
        say(f'find: nothing holds "{word}"')


def where(world):
    pawn, controller = player(world)
    if pawn is None:
        return
    r = controller.get_control_rotation()
    say(f'where {fmt(pawn.get_actor_location())} yaw {r.yaw:.0f} pitch {r.pitch:.0f}')


def kill(world, radius_m=20.0):
    pawn, controller = player(world)
    if pawn is None:
        return
    here = pawn.get_actor_location()
    killed = []
    for actor in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CreatureBase):
        if (actor.get_actor_location() - here).length() > radius_m * 100.0:
            continue
        if hasattr(actor, 'is_dead') and actor.is_dead():
            continue
        unreal.GameplayStatics.apply_damage(actor, 100000.0, controller, pawn, unreal.DamageType)
        killed.append(actor.get_name())
    say(f'kill: {len(killed)} within {radius_m:.0f} m: {", ".join(killed)}')


def look(world, yaw, pitch=0.0):
    pawn, controller = player(world)
    if pawn is None:
        return
    controller.set_control_rotation(unreal.Rotator(roll=0.0, pitch=pitch, yaw=yaw))
    say(f'look yaw {yaw:.0f} pitch {pitch:.0f}')


def press(world, name, seconds=0.0, x=1.0, y=0.0):
    """Feeds the game's own input action (IA_<name>: Interact, Fire, Jump, Reload, Move...) as if its key were pressed:
    a tap for one frame, or held for the seconds given (a Move held walks: x forward, y right). Needs no window focus."""
    pawn, controller = player(world)
    if pawn is None:
        return
    action = unreal.load_asset(f'/Game/Input/Actions/IA_{name}')
    if action is None:
        say(f'press: no input action IA_{name}')
        return
    inputs = next((s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == world), None)
    if inputs is None:
        say('press: the player has no Enhanced Input subsystem')
        return
    value = unreal.Vector(x, y, 0.0)
    inputs.inject_input_vector_for_action(action, value, [], [])
    if seconds <= 0.0:
        say(f'press {name}: tapped')
        return
    # Held: the action is fed again every frame until the time is up (one frame without it releases it).
    state = {'left': seconds}

    def tick(dt):
        state['left'] -= dt
        if state['left'] <= 0.0:
            unreal.unregister_slate_post_tick_callback(state['handle'])
            return
        try:
            inputs.inject_input_vector_for_action(action, value, [], [])
        except Exception:
            unreal.unregister_slate_post_tick_callback(state['handle'])

    state['handle'] = unreal.register_slate_post_tick_callback(tick)
    say(f'press {name}: held for {seconds:.1f} s')


def fill_ammo(pawn):
    """Tops every ammo pool up to its most, so a play check's fights don't stop for pickups."""
    manager = pawn.get_component_by_class(unreal.WeaponManagerComponent)
    if manager is None:
        return 0
    added = 0
    for name in dir(unreal.AmmoType):
        if not name.isupper():
            continue
        kind = getattr(unreal.AmmoType, name)
        try:
            added += manager.add_ammo(kind, max(0, manager.get_max_ammo(kind) - manager.get_ammo(kind)))
        except Exception:
            pass
    return added


def fight(world, seconds=20.0, range_m=40.0):
    """Fights for the player for the seconds given: each frame it turns the view onto the nearest living creature in
    sight within the range and holds Fire, and taps Reload when nothing is in sight. Says what died."""
    pawn, controller = player(world)
    if pawn is None:
        return
    fire = unreal.load_asset('/Game/Input/Actions/IA_Fire')
    reload_action = unreal.load_asset('/Game/Input/Actions/IA_Reload')
    inputs = next((s for s in unreal.ObjectIterator(unreal.EnhancedInputLocalPlayerSubsystem) if s.get_world() == world), None)
    if inputs is None or fire is None:
        say('fight: no input to fight with')
        return
    fill_ammo(pawn)
    state = {'left': seconds, 'idle': 0.0, 'refill': 2.0, 'seen': set()}
    on = unreal.Vector(1.0, 0.0, 0.0)

    def target():
        eye, _ = controller.get_player_view_point()
        best, best_d = None, range_m * 100.0
        for creature in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.CreatureBase):
            if creature.is_dead():
                continue
            origin, _ = creature.get_actor_bounds(True)
            d = (origin - eye).length()
            if d >= best_d:
                continue
            hit = unreal.SystemLibrary.line_trace_single(world, eye, origin, TRACE, False, [pawn], unreal.DrawDebugTrace.NONE, True)
            if hit is not None and hit.to_tuple()[9] != creature:
                continue
            best, best_d = (creature, origin), d
        return best

    def tick(dt):
        try:
            state['left'] -= dt
            state['refill'] -= dt
            if state['refill'] <= 0.0:
                fill_ammo(pawn)
                state['refill'] = 2.0
            found = target()
            if state['left'] <= 0.0:
                unreal.unregister_slate_post_tick_callback(state['handle'])
                return
            if found is None:
                state['idle'] += dt
                if state['idle'] > 1.0 and reload_action:
                    inputs.inject_input_vector_for_action(reload_action, on, [], [])
                    state['idle'] = -2.0
                return
            creature, origin = found
            state['seen'].add(creature.get_name())
            eye, _ = controller.get_player_view_point()
            d = origin - eye
            controller.set_control_rotation(unreal.Rotator(roll=0.0, pitch=math.degrees(math.atan2(d.z, math.hypot(d.x, d.y))),
                                                           yaw=math.degrees(math.atan2(d.y, d.x))))
            # Pressed every other frame, so a semi-automatic gun fires again too.
            state['trigger'] = not state.get('trigger', False)
            if state['trigger']:
                inputs.inject_input_vector_for_action(fire, on, [], [])
        except Exception as error:
            unreal.unregister_slate_post_tick_callback(state['handle'])
            say(f'fight stopped: {error}')

    state['handle'] = unreal.register_slate_post_tick_callback(tick)
    say(f'fight: fighting for {seconds:.0f} s within {range_m:.0f} m')


def what(world, yaw_offset=0.0, pitch_offset=0.0):
    """What the view's middle meets (turned by the offsets): the actor, its label, the component and the distance."""
    pawn, controller = player(world)
    if pawn is None:
        return
    eye, rot = controller.get_player_view_point()
    yaw, pitch = math.radians(rot.yaw + yaw_offset), math.radians(rot.pitch + pitch_offset)
    way = unreal.Vector(math.cos(pitch) * math.cos(yaw), math.cos(pitch) * math.sin(yaw), math.sin(pitch))
    hit = unreal.SystemLibrary.line_trace_single(world, eye, eye + way * 50000.0, TRACE, True, [pawn],
                                                 unreal.DrawDebugTrace.NONE, True)
    if hit is None:
        say('what: nothing within 500 m')
        return
    t = hit.to_tuple()
    actor, component = t[9], t[10]
    label = actor.get_actor_label() if actor and hasattr(actor, 'get_actor_label') else ''
    mesh = ''
    if component and hasattr(component, 'static_mesh') and component.static_mesh:
        mesh = component.static_mesh.get_name()
    say(f'what {actor.get_name() if actor else None} [{label}] {component.get_name() if component else None} {mesh} '
        f'at {fmt(t[5])}, {(t[5] - eye).length():.0f} cm off')


def guns(world):
    """The player's guns: the one in hand and every slot (a play check watches they don't go missing)."""
    pawn, _ = player(world)
    if pawn is None:
        return
    manager = pawn.get_component_by_class(unreal.WeaponManagerComponent)
    held = manager.get_active_weapon() if manager else None
    names = [w.get_name() if w else 'None' for w in manager.get_weapons()] if manager else []
    say(f'guns: in hand {held.get_name() if held else "none"}; slots {names}; '
        f'backpack {len(manager.get_backpack()) if manager else 0}')


def equip(world, slot):
    """Takes slot's gun in hand (1 is the first), as its number key would: a swap plays out."""
    pawn, _ = player(world)
    manager = pawn.get_component_by_class(unreal.WeaponManagerComponent) if pawn else None
    if manager is None:
        return
    manager.equip_slot(slot - 1)
    say(f'equip {slot}: on its way to hand')


def cycle_view(world, times):
    """Cycles the view as F5 does, times over."""
    pawn, _ = player(world)
    view = pawn.get_component_by_class(unreal.PlayerViewComponent) if pawn else None
    if view is None:
        return
    for _ in range(times):
        view.cycle_view_mode()
    say(f'view: cycled {times}')


def inventory(world, close):
    """Opens or closes the inventory through the player's HUD, as Tab does."""
    _, controller = player(world)
    hud = controller.get_hud() if controller else None
    if hud is None:
        return
    if close:
        hud.close_inventory()
    else:
        hud.open_inventory()
    say(f'inventory: {"closed" if close else "opened"}')


def look_at(world, word):
    """Turns the view from where the player stands onto the nearest actor tagged or named so (its location)."""
    pawn, controller = player(world)
    if pawn is None:
        return
    targets = matches(world, word)
    if not targets:
        say(f'lookat: nothing tagged or named "{word}"')
        return
    eye, _ = controller.get_player_view_point()
    target = min(targets, key=lambda a: (a.get_actor_location() - eye).length())
    d = target.get_actor_location() - eye
    controller.set_control_rotation(unreal.Rotator(roll=0.0, pitch=math.degrees(math.atan2(d.z, math.hypot(d.x, d.y))),
                                                   yaw=math.degrees(math.atan2(d.y, d.x))))
    hit = unreal.SystemLibrary.line_trace_single(world, eye, target.get_actor_location(), TRACE, False, [pawn],
                                                 unreal.DrawDebugTrace.NONE, True)
    first = hit.to_tuple()[9] if hit else None
    say(f'lookat {target.get_name()}: {d.length():.0f} cm from the eye; the line first meets '
        f'{first.get_actor_label() if first else "nothing"}')


def main(args):
    world = game_world()
    if world is None:
        say('no play session running')
        return
    if not args:
        say(__doc__)
        return
    command, rest = args[0].lower(), args[1:]
    if command == 'goto' and rest:
        goto(world, rest[0], float(rest[1]) if len(rest) > 1 else 180.0, float(rest[2]) if len(rest) > 2 else 0.0)
    elif command == 'at' and len(rest) >= 3:
        at(world, *[float(v) for v in rest[:4]])
    elif command == 'find' and rest:
        find(world, rest[0])
    elif command == 'where':
        where(world)
    elif command == 'kill':
        kill(world, float(rest[0]) if rest else 20.0)
    elif command == 'press' and rest:
        press(world, rest[0], *[float(v) for v in rest[1:4]])
    elif command == 'fight':
        fight(world, *[float(v) for v in rest[:2]])
    elif command == 'what':
        what(world, *[float(v) for v in rest[:2]])
    elif command == 'lookat' and rest:
        look_at(world, rest[0])
    elif command == 'guns':
        guns(world)
    elif command == 'equip' and rest:
        equip(world, int(rest[0]))
    elif command == 'view':
        cycle_view(world, int(rest[0]) if rest else 1)
    elif command == 'inventory':
        inventory(world, bool(rest) and rest[0].lower() == 'close')
    elif command == 'ammo':
        say(f'ammo: {fill_ammo(player(world)[0])} rounds added')
    elif command == 'look' and rest:
        look(world, *[float(v) for v in rest[:2]])
    else:
        say(f'unknown or incomplete command: {" ".join(args)}')


main(sys.argv[1:])
