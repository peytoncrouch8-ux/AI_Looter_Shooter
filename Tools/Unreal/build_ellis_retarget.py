"""Ellis's body retarget (the user, 2026-10-15: SK_Ellis is the player's body; "retarget live"): the mannequin stays the
player's animated body, hidden, and Ellis copies its pose every frame through the IK retargeter this script makes
(Source/AI_Looter_Shooter/Player/Animation/BodyFigureComponent, BodyRetargetAnimInstance).

    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_ellis_retarget.py" -Until "RETARGET (done|failed)"

Makes in /Game/Player/Retarget (run again, it sets up the same assets afresh in place: deleting them would null and dirty
BP_LooterCharacter, whose Ellis component points at the retargeter):
- IK_Manny: SKM_Manny_Simple's IK rig, its chains and goals made by the engine's own rules for the UE5 mannequin.
- IK_Ellis: SK_Ellis's (his 63 bones carry the mannequin's names, so the same rules map them: the same 29 chains).
- RTG_MannyToEllis: the retargeter, the default ops plus Blend to Source on both arms (the hands go where the mannequin's
  are, so the guns in its hand sockets sit in Ellis's), chains mapped by name. Ellis's model faces +X and the mannequin
  +Y, so his retarget pose turns his root a quarter round; nothing else (the engine's auto align lined his neck and arms
  up as if he hadn't turned, tipping his head sideways; both rest poses are close enough as they are: arms down at the
  sides, the neck a little forward). `--align` runs it anyway.
"""
import sys

import unreal

FOLDER = '/Game/Player/Retarget'
MANNY = '/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple'
ELLIS = '/Game/Art/Characters/SK_Ellis.SK_Ellis'
SOURCE = unreal.RetargetSourceOrTarget.SOURCE
TARGET = unreal.RetargetSourceOrTarget.TARGET


def say(text):
    unreal.log(f'RETARGET {text}')


def existing_or_new(name, asset_class, factory):
    """The asset at FOLDER/name, made when missing."""
    path = f'{FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, asset_class, factory)


def make_rig(name, mesh):
    rig = existing_or_new(name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    rig_controller = unreal.IKRigController.get_controller(rig)
    # Emptied first: its chains, goals and solvers made again below.
    for chain in list(rig_controller.get_retarget_chains()):
        rig_controller.remove_retarget_chain(chain.get_editor_property('chain_name'))
    while rig_controller.get_num_solvers() > 0:
        rig_controller.remove_solver(0)
    for goal in list(rig_controller.get_all_goals()):
        rig_controller.remove_goal(goal.get_editor_property('goal_name'))
    rig_controller.set_skeletal_mesh(mesh)
    if not rig_controller.apply_auto_generated_retarget_definition():
        say(f'{name}: no retarget definition was made (unknown skeleton?)')
    rig_controller.apply_auto_fbik()
    chains = []
    for chain in rig_controller.get_retarget_chains():
        chain_name = chain.get_editor_property('chain_name')
        chains.append(f'{chain_name}({rig_controller.get_retarget_chain_start_bone(chain_name)}-'
                      f'{rig_controller.get_retarget_chain_end_bone(chain_name)})')
    say(f'{name}: root {rig_controller.get_retarget_root()}, {len(chains)} chains: {", ".join(chains)}')
    return rig


def pin_hands(controller):
    """The hands where the mannequin's are (the guns ride its hand sockets): a Blend to Source op on both arms, under the
    Run IK Rig op, so Ellis's arms reach them by IK whatever their length."""
    index = -1
    for op_type in ('/Script/IKRig.IKRetargetBlendToSourceOp', 'IKRetargetBlendToSourceOp', 'FIKRetargetBlendToSourceOp'):
        index = controller.add_retarget_op(op_type)
        if index >= 0:
            break
    if index < 0:
        say('failed: no Blend to Source op could be added')
        return
    name = controller.get_op_name(index)
    controller.set_parent_op_by_name(name, 'Run IK Rig')
    op = controller.get_op_controller(controller.get_index_of_op_by_name(name))
    settings = op.get_settings()
    chains = list(settings.get_editor_property('chains'))
    pinned = []
    for chain in chains:
        if str(chain.get_editor_property('target_chain_name')) in ('LeftArm', 'RightArm'):
            chain.set_editor_property('blend_to_source', 1.0)
            chain.set_editor_property('translation_alpha', 1.0)
            pinned.append(str(chain.get_editor_property('target_chain_name')))
    settings.set_editor_property('chains', chains)
    op.set_settings(settings)
    say(f'hands pinned to the source: {", ".join(pinned) or "none (no arm chains in the op)"}; '
        f'parent {controller.get_parent_op_by_name(name)}')


def main():
    manny = unreal.load_asset(MANNY)
    ellis = unreal.load_asset(ELLIS)
    if not manny or not ellis:
        say(f'failed: missing {"SKM_Manny_Simple" if not manny else "SK_Ellis"}')
        return
    manny_rig = make_rig('IK_Manny', manny)
    ellis_rig = make_rig('IK_Ellis', ellis)

    retargeter = existing_or_new('RTG_MannyToEllis', unreal.IKRetargeter, unreal.IKRetargetFactory())
    controller = unreal.IKRetargeterController.get_controller(retargeter)
    controller.remove_all_ops()
    controller.set_ik_rig(SOURCE, manny_rig)
    controller.set_ik_rig(TARGET, ellis_rig)
    controller.set_preview_mesh(SOURCE, manny)
    controller.set_preview_mesh(TARGET, ellis)
    controller.add_default_ops()
    controller.assign_ik_rig_to_all_ops(SOURCE, manny_rig)
    controller.assign_ik_rig_to_all_ops(TARGET, ellis_rig)
    controller.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    pin_hands(controller)
    ops = [str(controller.get_op_name(i)) for i in range(controller.get_num_retarget_ops())]
    say(f'ops: {", ".join(ops)}')

    # Ellis faces +X, the mannequin +Y: his root turned a quarter round (about Z, X toward Y) to face its way.
    quarter = unreal.Quat()
    quarter.set_from_euler(unreal.Vector(0.0, 0.0, 90.0))
    pose = controller.get_current_retarget_pose_name(TARGET)
    controller.reset_retarget_pose(pose, [], TARGET)
    controller.set_rotation_offset_for_retarget_pose_bone('root', quarter, TARGET)
    if '--align' in sys.argv:
        controller.auto_align_all_bones(TARGET, unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN)
    root_turn = controller.get_rotation_offset_for_retarget_pose_bone('root', TARGET).euler()
    say(f'target root turned {root_turn.z:.1f} degrees')

    for asset in (manny_rig, ellis_rig, retargeter):
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
    say('done')


try:
    main()
except Exception as error:  # the console shows only the log: say what broke
    say(f'failed: {error!r}')
    raise
