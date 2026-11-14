"""Blender 5.2: rigged GLB -> FBX (skeletal mesh + all actions). Usage:
blender -b -P glb_to_skel_fbx.py -- in.glb out.fbx [scale=1.0] [rotz_deg=0]
Drops stray non-skinned meshes and *_end helper empties, keeps armature + all actions (1 uu = 1 cm)."""
import bpy, sys, math
a = sys.argv[sys.argv.index("--")+1:]
src, dst = a[0], a[1]; sc = float(a[2]) if len(a) > 2 else 1.0; rz = float(a[3]) if len(a) > 3 else 0.0
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
for o in list(bpy.data.objects):
    if (o.type == 'MESH' and len(o.vertex_groups) == 0) or (o.type == 'EMPTY' and o.name.endswith('_end')):
        bpy.data.objects.remove(o, do_unlink=True)
for m in bpy.data.materials:
    if m.use_nodes:
        for n in m.node_tree.nodes:
            if n.type == 'BSDF_PRINCIPLED': print("MAT", m.name, tuple(round(x,3) for x in n.inputs['Base Color'].default_value))
arm = [o for o in bpy.data.objects if o.type == 'ARMATURE'][0]
root = arm.parent
root.scale = (sc,)*3; root.rotation_euler[2] = math.radians(rz)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.fbx(filepath=dst, use_selection=True, global_scale=1.0, apply_unit_scale=True,
    apply_scale_options='FBX_SCALE_NONE', axis_forward='-Y', axis_up='Z', bake_space_transform=False,
    object_types={'ARMATURE','MESH'}, add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=True,
    bake_anim_use_nla_strips=False, bake_anim_force_startend_keying=True, bake_anim_simplify_factor=0.0,
    path_mode='AUTO', embed_textures=False, mesh_smooth_type='FACE')
print("EXPORTED", dst, "actions", len(bpy.data.actions))
