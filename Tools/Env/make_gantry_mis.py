"""Create dark-metal MIs for the NASA Gantry (parent M_Proto) in Construction/Env."""
import unreal
parent = unreal.load_asset("/Game/MissionCrew/Construction/Materials/M_Proto")
at = unreal.AssetToolsHelpers.get_asset_tools()
specs = {"MI_Gantry_Steel": ((0.07, 0.08, 0.10), 0.45, 0.85),
         "MI_Gantry_BlueGrey": ((0.12, 0.15, 0.20), 0.55, 0.6),
         "MI_Gantry_Hazard": ((0.55, 0.20, 0.04), 0.5, 0.4)}
for name, (c, r, m) in specs.items():
    path = "/Game/MissionCrew/Construction/Env/" + name
    mi = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if mi is None:
        f = unreal.MaterialInstanceConstantFactoryNew()
        mi = at.create_asset(name, "/Game/MissionCrew/Construction/Env", unreal.MaterialInstanceConstant, f)
    mi.set_editor_property("parent", parent)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(c[0], c[1], c[2], 1))
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi, "Roughness", r)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi, "Metallic", m)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    unreal.log("MC_MI: made " + name)
