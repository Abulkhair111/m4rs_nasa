import unreal, warnings
warnings.simplefilter("ignore", DeprecationWarning)
SRC = r"C:/Users/abulk/Documents/Unreal Projects/проект/SourceArt/LOLA/LunarTerrain_LOLA_26576_256_smooth.glb"
DEST = "/Game/MissionCrew/Art/Meshes/Terrain"
mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
params = unreal.ImportAssetParameters()
params.is_automated = True
p = unreal.InterchangeGenericAssetsPipeline()
mp = p.get_editor_property("mesh_pipeline")
mp.set_editor_property("combine_static_meshes_behavior", unreal.InterchangeCombineStaticMeshesBehavior.ALL)
try:
    mp.set_editor_property("build_nanite", False)
except Exception as e:
    unreal.log("MC_IMPORT: no build_nanite prop: %s" % e)
mat = p.get_editor_property("material_pipeline")
try:
    mat.set_editor_property("import_materials", False)
except Exception as e:
    unreal.log("MC_IMPORT: material opt: %s" % e)
params.override_pipelines.append(unreal.SoftObjectPath(p.get_path_name()))
ok = mgr.import_asset(DEST, unreal.InterchangeManager.create_source_data(SRC), params)
for a in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True):
    unreal.log("MC_IMPORT: asset %s" % a)
unreal.EditorAssetLibrary.save_directory(DEST, only_if_is_dirty=False, recursive=True)
unreal.log("MC_IMPORT: done %s" % ok)

