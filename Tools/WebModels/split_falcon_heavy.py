# blender -b -P split_falcon_heavy.py -- FalconHeavy.glb out_dir
# Splits the Poly Pizza "SpaceX Falcon Heavy" GLB (CC-BY 3.0, Carwyn Pelley) into stage/booster/fairing/tank FBX pieces
# (no geometry edited, whole objects only). Z up, origin bottom-centre, 1 model unit = 1 m = 100 uu.
import bpy, sys, os
from mathutils import Vector
a = sys.argv[sys.argv.index('--')+1:]
src, out = os.path.abspath(a[0]), os.path.abspath(a[1])
ENG = {'group1019908090','group1275527013','group440554346','group1083589953',   # core engine section
       'group318079340','group248325220','group379271175','group1266683896'}    # side booster engine section
def load():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=src)
    ren = {'mat21':'FH_Body','mat22':'FH_EngineDark','mat23':'FH_Structure','mat24':'FH_Skirt'}
    for m in bpy.data.materials:
        if m.name in ren: m.name = ren[m.name]
    info = {}
    for o in bpy.context.scene.objects:
        if o.type != 'MESH': continue
        ws = [o.matrix_world @ Vector(c) for c in o.bound_box]
        mn = Vector([min(w[i] for w in ws) for i in range(3)]); mx = Vector([max(w[i] for w in ws) for i in range(3)])
        info[o.name] = (mn, mx)
    return info
def cls(n, mn, mx):
    cy = (mn.y + mx.y) / 2
    if abs(cy) < 0.25: return 'fairing' if mn.z >= 2.5 else 'core'
    return 'boostA' if cy < 0 else 'boostB'
def export(name, pred):
    info = load()
    keep = [n for n, (mn, mx) in info.items() if pred(n, mn, mx)]
    for o in list(bpy.context.scene.objects):
        if o.name not in keep: bpy.data.objects.remove(o, do_unlink=True)
    mn = Vector((1e9,)*3); mx = Vector((-1e9,)*3)
    for n in keep:
        for i in range(3): mn[i] = min(mn[i], info[n][0][i]); mx[i] = max(mx[i], info[n][1][i])
    off = Vector(((mn.x+mx.x)/2, (mn.y+mx.y)/2, mn.z))
    for o in bpy.context.scene.objects:
        if o.parent is None: o.location -= off
    bpy.context.view_layer.update()
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.export_scene.fbx(filepath=os.path.join(out, name + '.fbx'), use_selection=True, global_scale=1.0, apply_unit_scale=True,
        apply_scale_options='FBX_SCALE_NONE', axis_forward='-Y', axis_up='Z', path_mode='AUTO', embed_textures=False, mesh_smooth_type='FACE')
    print('EXPORTED', name, len(keep), 'size', tuple(round(v*100) for v in (mx-mn)))
os.makedirs(out, exist_ok=True)
C = lambda n, mn, mx: cls(n, mn, mx)
export('Falcon_Heavy_Full', lambda n, mn, mx: True)
export('Falcon_Stage1',  lambda n, mn, mx: C(n, mn, mx) == 'core')
export('Falcon_Fairing', lambda n, mn, mx: C(n, mn, mx) == 'fairing')
export('Falcon_Core_Fairing', lambda n, mn, mx: C(n, mn, mx) in ('core', 'fairing'))
export('Falcon_SideBooster', lambda n, mn, mx: C(n, mn, mx) == 'boostA')
export('Falcon_TankCore', lambda n, mn, mx: C(n, mn, mx) == 'core' and n not in ENG and mx.z < 2.59 and True)
export('Falcon_TankBooster', lambda n, mn, mx: C(n, mn, mx) == 'boostA' and n not in ENG)
