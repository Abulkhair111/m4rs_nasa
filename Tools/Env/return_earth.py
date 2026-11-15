import unreal
eas = unreal.EditorAssetLibrary
mat = None
for p in ("/Game/MissionCrew/Art/Meshes/Space/MI_Earth", "/Game/MissionCrew/Art/Materials/Space/MI_Earth"):
    if eas.does_asset_exist(p): mat = unreal.load_asset(p); break
sph = unreal.load_asset("/Engine/BasicShapes/Sphere")
ss = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in ss.get_all_level_actors():
    if a.get_actor_label() == "ReturnEarth": ss.destroy_actor(a)
a = ss.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(52000, 9000, 3000))
a.static_mesh_component.set_static_mesh(sph)
a.set_actor_scale3d(unreal.Vector(500, 500, 500))
if mat: a.static_mesh_component.set_material(0, mat)
a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
a.set_actor_label("ReturnEarth")
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("RETEARTH mat=%s" % mat)
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
