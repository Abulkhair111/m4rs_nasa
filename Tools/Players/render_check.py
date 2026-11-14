import bpy, sys, math
from mathutils import Vector
a = sys.argv[sys.argv.index("--")+1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=a[0])
ms=[o for o in bpy.data.objects if o.type=='MESH']
mn=Vector((1e9,)*3);mx=Vector((-1e9,)*3)
for o in ms:
    for c in o.bound_box:
        w=o.matrix_world@Vector(c); mn=Vector(map(min,mn,w)); mx=Vector(map(max,mx,w))
ctr=(mn+mx)/2; r=max(mx-mn)
sc=bpy.context.scene; sc.render.engine='BLENDER_WORKBENCH'; sc.render.resolution_x=600; sc.render.resolution_y=600
sc.display.shading.color_type='TEXTURE'
cam=bpy.data.cameras.new("c"); cam.type='ORTHO'; cam.ortho_scale=r*1.2
co=bpy.data.objects.new("c",cam); sc.collection.objects.link(co); sc.camera=co
views={'front':(0,-1,0),'side':(1,0,0),'top':(0,0,1)}
for n,d in views.items():
    d=Vector(d); co.location=ctr+d*r*3
    co.rotation_euler=(-d).to_track_quat('-Z','Y' if n!='top' else 'Y').to_euler()
    sc.render.filepath=f"{a[1]}_{n}.png"; bpy.ops.render.render(write_still=True)
