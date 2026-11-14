"""Import NASA 3D Resources GLBs (SourceArt/NASA3D) as static meshes.

Run (editor may stay open):
  UnrealEditor-Cmd.exe <project>.uproject -run=pythonscript -script=<this file>
Each GLB becomes one combined StaticMesh (+ its materials/textures) in
/Game/MissionCrew/Construction/Env/Meshes/<Name>. Nanite is off (project has no SM6).
Source: https://science.nasa.gov/3d-resources/ (github.com/nasa/NASA-3D-Resources),
"free and without copyright", NASA media usage guidelines apply.
"""
import os
import unreal, warnings
warnings.simplefilter("ignore", DeprecationWarning)

PROJECT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC_DIR = os.path.join(PROJECT, "SourceArt", "NASA3D_Env")
DEST_ROOT = "/Game/MissionCrew/Construction/Env/Meshes"
ONLY = os.environ.get("MC_ONLY", "")  # optional comma list of names

mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
names = sorted(f[:-4] for f in os.listdir(SRC_DIR) if f.lower().endswith(".glb"))
if ONLY:
    names = [n for n in names if n in ONLY.split(",")]
for name in names:
    dest = DEST_ROOT + "/" + name
    if unreal.EditorAssetLibrary.does_directory_exist(dest) and unreal.EditorAssetLibrary.list_assets(dest):
        unreal.log("MC_IMPORT: skip %s (exists)" % name)
        continue
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    p = unreal.InterchangeGenericAssetsPipeline()
    mp = p.get_editor_property("mesh_pipeline")
    mp.set_editor_property("combine_static_meshes_behavior", unreal.InterchangeCombineStaticMeshesBehavior.ALL)
    try:
        mp.set_editor_property("build_nanite", False)
    except Exception as e:
        unreal.log("MC_IMPORT: no build_nanite prop: %s" % e)
    # Some NASA GLBs carry animations and would come in as SkeletalMesh: force static.
    try:
        mp.set_editor_property("force_all_mesh_as_type", unreal.InterchangeForceMeshType.IFMT_STATIC_MESH)
    except Exception as e:
        unreal.log("MC_IMPORT: force static failed: %s" % e)
    try:
        p.get_editor_property("animation_pipeline").set_editor_property("import_animations", False)
    except Exception as e:
        unreal.log("MC_IMPORT: anim off failed: %s" % e)
    params.override_pipelines.append(unreal.SoftObjectPath(p.get_path_name()))
    src = os.path.join(SRC_DIR, name + ".glb").replace("\\", "/")
    ok = mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(src), params)
    for a in unreal.EditorAssetLibrary.list_assets(dest, recursive=True):
        obj = unreal.EditorAssetLibrary.load_asset(a)
        if isinstance(obj, unreal.StaticMesh):
            try:
                ns = obj.get_editor_property("nanite_settings")
                ns.enabled = False
                obj.set_editor_property("nanite_settings", ns)
            except Exception as e:
                unreal.log("MC_IMPORT: nanite off failed %s" % e)
            b = obj.get_bounding_box()
            unreal.log("MC_IMPORT: mesh %s size=%s" % (a, (b.max - b.min)))
    unreal.EditorAssetLibrary.save_directory(dest, only_if_is_dirty=False, recursive=True)
    unreal.log("MC_IMPORT: done %s %s" % (name, ok))
unreal.log("MC_IMPORT: ALL DONE")
