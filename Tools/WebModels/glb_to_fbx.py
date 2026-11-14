# blender -b -P glb_to_fbx.py -- in.glb out_dir name max_tex [scale] [info]
# Env ONLY_PREFIX=a,b keeps only objects whose name starts with a prefix (extract a sub-assembly, no geometry edits).
# Converts a GLB to FBX (Z up, 1 m = 100 uu), origin bottom-centre, textures downscaled to max_tex and copied beside the FBX.
import bpy, sys, os, mathutils
a = sys.argv[sys.argv.index('--')+1:]
src, out, name, maxtex = os.path.abspath(a[0]), os.path.abspath(a[1]), a[2], int(a[3])
scale = float(a[4]) if len(a) > 4 else 1.0
info = len(a) > 5
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
ONLY = os.environ.get("ONLY_PREFIX")
if ONLY:
    for o in list(bpy.context.scene.objects):
        if not o.name.startswith(tuple(ONLY.split(","))): bpy.data.objects.remove(o, do_unlink=True)
meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
tris = 0
mn = mathutils.Vector((1e9,)*3); mx = mathutils.Vector((-1e9,)*3)
for o in meshes:
    o.data.calc_loop_triangles(); tris += len(o.data.loop_triangles)
    for c in o.bound_box:
        w = o.matrix_world @ mathutils.Vector(c)
        for i in range(3):
            mn[i] = min(mn[i], w[i]); mx[i] = max(mx[i], w[i])
print('INFO', name, 'meshes', len(meshes), 'tris', tris, 'min', tuple(mn), 'max', tuple(mx), 'size', tuple(mx-mn))
print('INFO images', [(i.name, i.size[0], i.size[1]) for i in bpy.data.images][:40])
if info: sys.exit(0)
# origin bottom centre
off = mathutils.Vector(((mn.x+mx.x)/2, (mn.y+mx.y)/2, mn.z))
for o in bpy.context.scene.objects:
    if o.parent is None:
        o.location -= off
        o.scale *= scale
        o.location *= scale
bpy.context.view_layer.update()
os.makedirs(out, exist_ok=True)
for im in bpy.data.images:
    if im.size[0] > maxtex or im.size[1] > maxtex:
        r = maxtex / max(im.size)
        im.scale(max(1,int(im.size[0]*r)), max(1,int(im.size[1]*r)))
td = os.path.join(out, name + "_tex"); os.makedirs(td, exist_ok=True)
for im in bpy.data.images:
    if im.size[0] == 0: continue
    im.filepath_raw = os.path.join(td, im.name.replace(" ", "_") + ".png"); im.file_format = "PNG"; im.save(); print("SAVED", im.filepath_raw)
bpy.ops.object.select_all(action="SELECT")
bpy.ops.export_scene.fbx(filepath=os.path.join(out, name+'.fbx'), use_selection=True, global_scale=1.0,
    apply_unit_scale=True, apply_scale_options='FBX_SCALE_NONE', axis_forward='-Y', axis_up='Z',
    path_mode='AUTO', embed_textures=False, mesh_smooth_type='FACE')
print('EXPORTED', name)
