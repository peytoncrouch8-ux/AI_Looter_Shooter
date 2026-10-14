"""MPC_Wind, the material parameter collection the gusts write (UGustSubsystem, Source/AI_Looter_Shooter/World/
GustSubsystem.h) for the materials that bend in the wind:

  GustStrength   the gust's push now, 0 between gusts, up to 1 at a full gust's height (its strength times its rise and
                 fall). M_WorldFoliage's plants and grass bow downwind and sway harder by it; M_Smoke's plumes lean.
  GustDirection  the way it blows (xy, level and unit; z and w 0): within 25 degrees of the prevailing wind (yaw 19, the
                 way the smoke leans).

The defaults leave everything still (strength 0); the subsystem writes 0 as each level begins and after each gust. The
names are UGustSubsystem's GustStrengthParameter and GustDirectionParameter.

Run it before building a material that reads it (a material can't read a parameter that isn't there yet); the masters'
script imports it the way it imports lighting_collection.py. Materials find a collection's parameter by its id, so an
existing parameter keeps its entry (only its default is set again) and missing ones are added. On its own:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/wind_collection.py"
"""
import unreal

PATH = '/Game/Art/Materials/MPC_Wind'
SCALARS = (
    ('GustStrength', 0.0),
)
VECTORS = (
    # The prevailing wind's way (yaw 19: cos, sin), so a material read before the first gust leans the right way.
    ('GustDirection', (0.9455, 0.3256, 0.0, 0.0)),
)


def _merge(mpc, prop, cls, entries, make_value):
    params = list(mpc.get_editor_property(prop))
    by_name = {str(p.get_editor_property('parameter_name')): p for p in params}
    for key, value in entries:
        param = by_name.get(key)
        if param is None:
            param = cls()
            param.set_editor_property('parameter_name', key)
            params.append(param)
        param.set_editor_property('default_value', make_value(value))
    # Set as a whole and always announced, so the collection rebuilds its layout and the materials reading it recompile
    # even when only defaults changed.
    mpc.set_editor_property(prop, params, unreal.PropertyAccessChangeNotifyMode.ALWAYS)


def collection():
    """MPC_Wind with every parameter the gusts write, saved; returns it."""
    folder, name = PATH.rsplit('/', 1)
    if unreal.EditorAssetLibrary.does_asset_exist(PATH):
        mpc = unreal.load_asset(PATH)
    else:
        mpc = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.MaterialParameterCollection, unreal.MaterialParameterCollectionFactoryNew())
    _merge(mpc, 'scalar_parameters', unreal.CollectionScalarParameter, SCALARS, float)
    _merge(mpc, 'vector_parameters', unreal.CollectionVectorParameter, VECTORS, lambda v: unreal.LinearColor(*v))
    unreal.EditorAssetLibrary.save_loaded_asset(mpc)
    unreal.log(f'LOOTER wind collection: {mpc.get_path_name()} '
               f'({", ".join(key for key, _ in SCALARS + VECTORS)})')
    return mpc


if __name__ == '__main__':
    collection()
