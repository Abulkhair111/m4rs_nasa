import unreal
eas = unreal.EditorAssetLibrary
mesh = None
for p in eas.list_assets("/Game/MissionCrew/Art/Meshes/Stages/PP_Mk12CommandPod", recursive=True, include_folder=False):
    a = eas.load_asset(p)
    if isinstance(a, unreal.StaticMesh): mesh = a; break
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_class().get_name() == "MCReturnView" and mesh:
        c = a.get_editor_property("capsule_mesh")
        c.set_static_mesh(mesh); c.set_editor_property("relative_scale3d", unreal.Vector(0.35, 0.35, 0.35)); n += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("RETMESH set on %d mesh=%s" % (n, mesh))
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
