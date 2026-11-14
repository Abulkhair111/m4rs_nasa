import bpy, sys
from mathutils import Vector
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=sys.argv[sys.argv.index("--")+1])
for o in bpy.data.objects:
    if o.type=='MESH':
        zs=[(o.matrix_world@Vector(c)) for c in o.bound_box]
        print("R",o.name,len(o.data.polygons),[m.name for m in o.data.materials],"zrange",round(min(z.z for z in zs),2),round(max(z.z for z in zs),2),"parent",o.parent.name if o.parent else None, "vg", len(o.vertex_groups))
    else: print("R",o.type,o.name, tuple(o.scale), o.parent.name if o.parent else None)
