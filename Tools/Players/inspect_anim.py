import bpy, sys
from mathutils import Vector
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=sys.argv[sys.argv.index("--")+1])
ms=[o for o in bpy.data.objects if o.type=='MESH']
print("R meshes",len(ms),"tris",sum(len(o.data.polygons) for o in ms),"arm",[ (o.name,len(o.data.bones)) for o in bpy.data.objects if o.type=='ARMATURE'])
for a in bpy.data.actions: print("R act",a.name,[round(x) for x in a.frame_range])
print("R mats",[m.name for m in bpy.data.materials])
