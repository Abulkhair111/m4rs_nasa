import unreal, os
v = os.environ.get("MC_VAL", "0") == "1"
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_class().get_name() in ("MCOrbitManager", "MCSurfaceManager", "MCSamplesManager", "MCReturnManager", "MCTransitManager"):
        a.set_editor_property("debug_auto_test", v); n += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log("SETDEBUG %s on %d" % (v, n))
unreal.SystemLibrary.execute_console_command(None, "QUIT_EDITOR")
