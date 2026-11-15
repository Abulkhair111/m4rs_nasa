import unreal
ss = unreal.get_editor_subsystem(unreal.EditorActorSubsystem); n = 0
for a in ss.get_all_level_actors():
    if a.get_class().get_name() == "ExponentialHeightFog":
        ss.destroy_actor(a); n += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("NOFOG removed %d" % n)
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
