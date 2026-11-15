import unreal
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_class().get_name() == "MCFlightView":
        a.set_editor_property("flip_text", True); n += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("FLIPTEXT set on %d" % n)
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
