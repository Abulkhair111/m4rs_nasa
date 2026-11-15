"""Headless: MC_MAP=IMPORT imports the shortlisted S3-S6 FBX models; MC_MAP=L_xxx places them as StaticMeshActors in that map
(opened as the startup map). Run with the editor closed:
UnrealEditor-Cmd.exe <uproject> [/Game/MissionCrew/Maps/L_xxx] -ExecutePythonScript=<this> -unattended -nosplash -nullrhi
Only places imported assets (no primitive geometry). Log: Saved/stage_models.log.txt"""
import unreal, os, random, traceback

SRC = "C:/mcproj/SourceArt/WebModels/"
DEST = "/Game/MissionCrew/Art/Meshes/Stages/"
LOG = os.path.join(unreal.Paths.project_saved_dir(), "stage_models.log.txt")
lines = []
def log(m):
    lines.append(str(m)); unreal.log("STAGEMODELS " + str(m))

PH = "PolyHaven_Raw/%s/fbx/PH_%s.fbx"
MODELS = {}
for i in range(1, 8):
    n = "moon_rock_0%d" % i
    MODELS[n] = PH % (n, n)
for n in ("boulder_01", "metal_toolbox", "plastic_crate_02", "industrial_pastic_container", "propane_tank",
          "ocean_buoy", "lifebuoy"):
    MODELS[n] = PH % (n, n)
MODELS.update({
    "HabDemoUnit": "NASA3D_HabDemoUnit/fbx/NASA_HabDemoUnit.fbx",
    "TallDish": "NASA3D_TallDish/fbx/NASA_TallDish.fbx",
    "PP_LanderA": "PP_LanderA/PP_LanderA.fbx",
    "PP_Mk12CommandPod": "PP_Mk12CommandPod/PP_Mk12CommandPod.fbx",
    "PP_Helicopter": "PP_Helicopter/PP_Helicopter.fbx",
    "PP_CoolerBox": "PP_CoolerBox/PP_CoolerBox.fbx",
    "PP_EnergyCanister": "PP_EnergyCanister/PP_EnergyCanister.fbx",
})

def do_import():
    tasks = []
    for name, rel in MODELS.items():
        path = SRC + rel
        if not os.path.exists(path):
            log("MISSING FILE " + path); continue
        t = unreal.AssetImportTask()
        t.filename = path
        t.destination_path = DEST + name
        t.destination_name = name
        t.automated = True; t.save = True; t.replace_existing = True
        o = unreal.FbxImportUI()
        o.import_mesh = True; o.import_textures = True; o.import_materials = True; o.import_as_skeletal = False
        o.static_mesh_import_data.combine_meshes = True
        t.options = o
        tasks.append(t)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for t in tasks:
        log("imported %s -> %s" % (t.destination_name, list(t.get_editor_property("imported_object_paths"))[:1]))

def mesh(name):
    d = DEST + name
    for p in unreal.EditorAssetLibrary.list_assets(d, recursive=True, include_folder=False):
        a = unreal.EditorAssetLibrary.load_asset(p)
        if isinstance(a, unreal.StaticMesh): return a
    log("NO MESH " + name); return None

ss = None
def put(name, loc, scale=1.0, yaw=0.0, label=None):
    m = mesh(name)
    if m is None: return
    a = ss.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(0, yaw, 0))
    a.static_mesh_component.set_static_mesh(m)
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    a.set_actor_label(label or ("S_" + name))

def place(mapname):
    global ss
    ss = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    random.seed(7)
    if mapname == "L_Surface":
        # decorative ring OUTSIDE the play field (FieldHalfSize 6500)
        for i in range(36):
            ang = random.uniform(0, 6.283); r = random.uniform(7200, 13000)
            n = "moon_rock_0%d" % random.randint(1, 7)
            put(n, (r * unreal.MathLibrary.cos(ang), r * unreal.MathLibrary.sin(ang), 0), random.uniform(6, 14), random.uniform(0, 360), "Rock_%d" % i)
        for i in range(8):
            ang = random.uniform(0, 6.283); r = random.uniform(7500, 12000)
            put("boulder_01", (r * unreal.MathLibrary.cos(ang), r * unreal.MathLibrary.sin(ang), 0), random.uniform(2, 4), random.uniform(0, 360), "Boulder_%d" % i)
        put("HabDemoUnit", (9000, 0, 0), 2.0, 180, "Habitat")
        put("TallDish", (-9500, 3500, 0), 1.0, 0, "Dish")
        put("PP_LanderA", (-8000, -4000, 0), 3.0, 20, "LanderProp")
    elif mapname == "L_Samples":
        put("PP_CoolerBox", (450, -300, 0), 0.04, 0, "Cooler")
        put("PP_EnergyCanister", (450, 80, 0), 2.0, 0, "Canister")
        put("metal_toolbox", (350, 300, 0), 1.5, 30, "SampleCase")
        put("plastic_crate_02", (520, 450, 0), 1.5, 0, "Crate")
        put("industrial_pastic_container", (600, -500, 0), 1.2, 10, "Container")
        put("propane_tank", (300, -450, 0), 1.2, 0, "CryoTank")
    elif mapname == "L_Return":
        put("PP_Mk12CommandPod", (1800, 0, 0), 1.0, 90, "CapsuleProp")
        put("PP_Helicopter", (2800, 1800, 0), 1.5, 40, "RecoveryHeli")
        put("ocean_buoy", (2300, -1400, 0), 1.0, 0, "Buoy")
        put("lifebuoy", (1500, 900, 0), 1.5, 0, "Lifebuoy")
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    log("saved " + mapname)

try:
    w = os.environ.get("MC_MAP", "")
    if w == "IMPORT": do_import()
    else: place(w)
except Exception:
    log("EXCEPTION " + traceback.format_exc())
with open(LOG, "w") as f: f.write("\n".join(lines))
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
